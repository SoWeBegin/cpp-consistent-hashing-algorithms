/*
 * Copyright (c) 2023 Amos Brocco, Tony Kolarek, Tatiana Dal Busco
 * Adapted from cpp-anchorhash Copyright (c) 2020 anchorhash (MIT License)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MONOTONICITY_BENCH_H
#define MONOTONICITY_BENCH_H

#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered_map.hpp>
#include <cxxopts.hpp>
#ifdef USE_PCG32
#include "pcg_random.hpp"
#include <random>
#endif
#include <fstream>
#include <sstream>
#include "anchor/anchorengine.h"
#include "jump/jumpengine.h"
#include "memento/mashtable.h"
#include "memento/mementoengine.h"
#include "power/powerengine.h"
#include <fmt/core.h>
#include <fstream>
#include <gtl/phmap.hpp>    
#include <unordered_map>
#include <vector>
#include "dx/dxEngine.h"
#include "YamlParser/YamlParser.h"
#include "csvWriter.h"
#include "utils.h"

 // monotonicity: Benchmarks the ability of the algorithm to move the minimal number of keys after a resize.

  /*
   * Benchmark routine
   */


template <typename Algorithm>
inline void bench(const std::string& name, const std::string file_name,
    std::size_t anchor_set /* capacity */, std::size_t working_set,
    uint32_t num_removals, uint32_t num_keys, Monotonicity& monotonicity) {

    Algorithm engine(anchor_set, working_set);

    // random removals
    uint32_t* bucket_status = new uint32_t[anchor_set]();

    // all nodes are working
    for (uint32_t i = 0; i < working_set; i++) {
        bucket_status[i] = 1;
    }

    // simulate num_removals removals
    uint32_t i = 0;
    while (i < num_removals) {
#ifdef USE_PCG32
        uint32_t removed = rng() % working_set;
#else
        uint32_t removed = rand() % working_set;
#endif
        if (bucket_status[removed] == 1) {
            auto rnode = engine.removeBucket(removed);
            bucket_status[rnode] = 0; // Remove the actually removed node
            i++;
        }
    }

    boost::unordered_flat_map<std::pair<uint32_t, uint32_t>, uint32_t> bucket;
    std::ofstream results_file;
    results_file.open("anchor.txt", std::ofstream::out | std::ofstream::app);

    // Determine the current key bucket assigment
    for (uint32_t i = 0; i < num_keys;) {
#ifdef USE_PCG32
        auto a{ rng() };
        auto b{ rng() };
#else
        auto a{ rand() };
        auto b{ rand() };
#endif
        if (bucket.contains({ a, b }))
            continue;
        auto target = engine.getBucketCRC32c(a, b);
        bucket[{a, b}] = target;
        // Verify that we got a working bucket
        if (!bucket_status[target]) {
            throw "Crazy bug";
        }
        ++i;
    }
    fmt::println("Done determining initial assignment of {} unique keys",
        num_keys);

    // Remove a random working node
    uint32_t removed{ 0 };
    uint32_t rnode{ 0 };
    for (;;) {
#ifdef USE_PCG32
        removed = rng() % working_set;
#else
        removed = rand() % working_set;
#endif
        if (bucket_status[removed] == 1) {
            rnode = engine.removeBucket(removed);
            fmt::println("Removed node {}", rnode);
            if (!bucket_status[rnode]) {
                throw "Crazy bug";
            }
            bucket_status[rnode] = 0; // Remove the actually removed node
            break;
        }
    }

    uint32_t misplaced{ 0 };
    for (const auto& i : bucket) {
        auto oldbucket = i.second;
        auto a{ i.first.first };
        auto b{ i.first.second };
        auto newbucket = engine.getBucketCRC32c(a, b);
        if (oldbucket != newbucket && (oldbucket != rnode)) {
            fmt::println("(After Removal) Misplaced key {},{}: before in bucket {}, "
                "now in bucket {} (status? old bucket {}, new bucket {})",
                a, b, oldbucket, newbucket, bucket_status[oldbucket],
                bucket_status[newbucket]);
            ++misplaced;
        }
    }

    double m = (double)misplaced / (num_keys);
    monotonicity.keys_in_removed_nodes = m;

#ifdef USE_PCG32
    fmt::println(
        "{}: after removal % misplaced keys are {}% ({} keys out of {})\n", name,
        m * 100, misplaced, num_keys);
    results_file << name << ": "
        << "MisplacedRem: " << misplaced << "\t" << num_keys << "\t" << m
        << "\t" << m << "\tPCG32\n";
#else
    fmt::println("{}: after removal misplaced keys are {}% ({} keys out of {})",
        name, m * 100, misplaced, num_keys);
    results_file << name << ": "
        << "MisplacedRem: " << misplaced << "\t" << num_keys << "\t" << m
        << "\t" << m << "\trand()\n";
#endif

    misplaced = 0;
    // Add back a node
    auto anode = engine.addBucket();
    bucket_status[anode] = 1;
    fmt::println("Added node {}", anode);

    for (const auto& i : bucket) {
        auto oldbucket = i.second;
        auto a{ i.first.first };
        auto b{ i.first.second };
        auto newbucket = engine.getBucketCRC32c(a, b);
        if (oldbucket != newbucket) {
            fmt::println("(After Add) Misplaced key {},{}: before in bucket {}, now "
                "in bucket {} (status? old bucket {}, new bucket {})",
                a, b, oldbucket, newbucket, bucket_status[oldbucket],
                bucket_status[newbucket]);
            ++misplaced;
        }
    }

    m = (double)misplaced / (num_keys);

#ifdef USE_PCG32
    fmt::println(
        "{}: after adding back % misplaced keys are {}% ({} keys out of {})\n",
        name, m * 100, misplaced, num_keys);
    results_file << name << ": "
        << "MisplacedAdd: " << misplaced << "\t" << num_keys << "\t" << m
        << "\t" << m << "\tPCG32\n";
#else
    fmt::println(
        "{}: after adding back misplaced keys are {}% ({} keys out of {})", name,
        m * 100, misplaced, num_keys);
    results_file << name << ": "
        << "MisplacedAdd: " << misplaced << "\t" << num_keys << "\t" << m
        << "\t" << m << "\trand()\n";
#endif

    ////////////////////////////////////////////////////////////////////

    results_file.close();

    delete[] bucket_status;
}


std::vector<double> parse_fractions(const std::string& fractions_str) {
    std::vector<double> fractions;
    std::istringstream iss(fractions_str);

    iss.ignore(1);
    double fraction;
    while (iss >> fraction) {
        fractions.push_back(fraction);
        iss.ignore();
    }
    return fractions;
}


inline int monotonicity(const std::string& output_path, std::size_t working_set, const std::string& hash_function,
    const std::string& key_distribution, const std::vector<AlgorithmSettings>& algorithms, 
    const std::unordered_map<std::string, std::string>& args) {

    Monotonicity monotonicity; 

    // Further parsing args to obtain fractions and keyMultiplier (default value for keyMultiplier = 100)
    if (!args.count("fractions")) {
        fmt::println("no fractions key found in the yaml file.");
        return 1;
    }
    std::vector<double> fractions = parse_fractions(args.at("fractions"));

    if (fractions.size() < 1) {
        fmt::println("fractions must have at least 1 value");
        return 1;
    }
    for (auto current_fraction : fractions) {
        if (current_fraction >= 0 && current_fraction < 1) continue;
        fmt::println("fraction values must be >= 0 and < 1");
        return 1;
    }

    std::size_t key_multiplier = 100;
    if (args.count("keyMultiplier") > 0) {
        key_multiplier = parse_key_multiplier(args.at("keyMultiplier"));
    }
    monotonicity.distribution = key_distribution;
    monotonicity.hash_function = hash_function;
    monotonicity.nodes = working_set;
    monotonicity.keys = key_multiplier * working_set;

    for (const auto& current_algorithm : algorithms) {
        for (const auto& current_fraction : fractions) {
            monotonicity.algorithm_name = current_algorithm.name;
            monotonicity.fraction = current_fraction;
            const std::string full_file_path = output_path + "/" + current_algorithm.name + ".txt";
            
            const uint32_t num_removals = static_cast<uint32_t>(current_fraction * working_set);
            uint32_t capacity = 10 * working_set; // default capacity = 10

            if (current_algorithm.args.contains("capacity")) {
                try {
                    capacity = std::stoi(current_algorithm.args.at("capacity")) * working_set;
                } catch (const std::exception& e) {
                    std::cerr << "std::stoi exception: " << e.what() << '\n';
                }
            }

            if (current_algorithm.name == "null") {
                // do nothing
                continue;
            }
            else if (current_algorithm.name == "baseline") {
                 fmt::println("Allocating {} buckets of size {} bytes...", capacity,
                     sizeof(uint32_t));
                 uint32_t* bucket_status = new uint32_t[capacity]();
                 for (uint32_t i = 0; i < working_set; i++) {
                     bucket_status[i] = 1;
                 }
                 uint32_t i = 0;
                 while (i < num_removals) {
                     uint32_t removed = rand() % working_set;
                     if (bucket_status[removed] == 1) {
                         bucket_status[removed] = 0;
                         i++;
                     }
                 }
                 delete[] bucket_status;
             }
            else if (current_algorithm.name == "anchor") {
                bench<AnchorEngine>("Anchor", full_file_path, capacity, working_set,
                    num_removals, key_multiplier * working_set, monotonicity);
            }
            else if (current_algorithm.name == "memento") {
                bench<MementoEngine<boost::unordered_flat_map>>(
                    "Memento<boost::unordered_flat_map>", full_file_path, capacity, working_set,
                    num_removals, key_multiplier * working_set, monotonicity);
            }
            else if (current_algorithm.name == "mementoboost") {
                bench<MementoEngine<boost::unordered_map>>(
                    "Memento<boost::unordered_map>", full_file_path, capacity, working_set,
                    num_removals, key_multiplier * working_set, monotonicity);
            }
            else if (current_algorithm.name == "mementostd") {
                bench<MementoEngine<std::unordered_map>>(
                    "Memento<std::unordered_map>", full_file_path, capacity, working_set,
                    num_removals, key_multiplier * working_set, monotonicity);
            }
            else if (current_algorithm.name == "mementogtl") {
                bench<MementoEngine<gtl::flat_hash_map>>(
                    "Memento<std::gtl::flat_hash_map>", full_file_path, capacity, working_set,
                    num_removals, key_multiplier * working_set, monotonicity);
            }
            else if (current_algorithm.name == "mementomash") {
                bench<MementoEngine<MashTable>>("Memento<MashTable>", full_file_path,
                    capacity, working_set,
                    num_removals, key_multiplier * working_set, monotonicity);
            }
            else if (current_algorithm.name == "jump") {
                bench<JumpEngine>("JumpEngine", full_file_path, capacity, working_set,
                    num_removals, key_multiplier * working_set, monotonicity);
            }
            else if (current_algorithm.name == "power") {
                bench<PowerEngine>("PowerEngine", full_file_path, capacity, working_set,
                    num_removals, key_multiplier * working_set, monotonicity);
            }
            else if (current_algorithm.name == "dx") {
                bench<DxEngine>("DxEngine", full_file_path, capacity, working_set,
                    num_removals, key_multiplier * working_set, monotonicity);
            }
            else {
                fmt::println("Unknown algorithm {}", current_algorithm.name);
                //return 2;
            }
        }
    }
    auto& monotonicity_writer = CsvWriter<Monotonicity>::getInstance("./", "monotonicity.csv");
    monotonicity_writer.add(monotonicity);
    return 0;
}

#endif