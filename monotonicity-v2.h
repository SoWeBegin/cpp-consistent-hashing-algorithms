/*

unordered_map<key,bucket> initial_assignment;

random_key_list = generate_key_list(num_keys)

for key in random_key_list:
    initial_assignment[key] = get_bucket(key)


nodes[size ] = {1...}
for n in range(0, num_removals):
    remove random node i
    nodes[i] = 0

for key in random_key_list :
    b = get_bucket(key)
    original_b = initial_assignment[key]
    if b != original_b && nodes[original_b] == 0;
# nodo rimosso-> OK


*/


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

#include <algorithm>
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

 /* this function generates a sequence of random numbers pairs
 * num_keys: total keys to generate where each key is a pair of 2 random numbers
 * return: a vector of pairs of random numbers
 */
std::vector<std::pair<uint32_t, uint32_t>> generate_random_sequence (std::size_t num_keys){
    std::vector<std::pair<uint32_t, uint32_t>> ret;
    for (uint32_t i = 0; i < num_keys;++i) {
#ifdef USE_PCG32
        auto a{ rng() };
        auto b{ rng() };
#else
        auto a{ rand() };
        auto b{ rand() };
#endif
        ret.push_back(std::pair<uint32_t,uint32_t>{a,b});     
    }
    return ret;
}

  /*
   * Benchmark routine
   */
template <typename Algorithm>
inline void bench(const std::string& name, const std::string file_name,
    std::size_t anchor_set /* capacity */, std::size_t working_set,
    uint32_t num_removals, uint32_t num_keys, double current_fraction,
    Monotonicity& monotonicity) {


    /*

unordered_map<key,bucket> initial_assignment;

random_key_list = generate_key_list(num_keys)

for key in random_key_list:
    initial_assignment[key] = get_bucket(key)


nodes[size ] = {1...}
for n in range(0, num_removals):
    remove random node i
    nodes[i] = 0

for key in random_key_list :
    b = get_bucket(key)
    original_b = initial_assignment[key]
    if b != original_b && nodes[original_b] == 0;
# nodo rimosso-> OK


*/



    Algorithm engine(anchor_set, working_set);

    // random removals
    uint32_t* nodes = new uint32_t[anchor_set]();

    // all nodes are working
    for (uint32_t i = 0; i < working_set; i++) {
        nodes[i] = 1;
    }

    // 1. Assign buckets and check keys_per_node
    boost::unordered_flat_map<std::pair<uint32_t, uint32_t>, uint32_t> bucket;

    // Generiamo una sequenza di numeri casuali e la salviamo in una variabile perchè
    // per ogni bucket che togliamo dopo dobbiamo rimetterlo nei nodi 
    // con la stessa chiave
    auto random_sequence = generate_random_sequence(num_keys);

    // Inizializziamo i nodi con i rispettivi buckets che ogni nodo avrà
    for (const auto& current_random_number : random_sequence) {
        auto a = current_random_number.first;
        auto b = current_random_number.second;
        if (bucket.contains(current_random_number))
            continue;
        // target: indice (numero) del nodo a cui assegnare il bucket 
        auto target = engine.getBucketCRC32c(a, b);
        // bucket.add(key--> current_random_number, valore)
        //bucket--> mappa chiavi valore 
        bucket[current_random_number] = target;
        // Verify that we got a working bucket
        // se il nodo = 0 vuol dire che è spento quindi non posso aggiungere il bucket
        // nodes: array contenente 1 o 0 per capire se il server è accesso o spento
        if (!nodes[target]) {
            throw "Crazy bug";
        }
    }

    // 2. Remove num_removals working nodes, update moved_from
    // simulate num_removals removals of nodes
    //num_removals: quanti nodi dobbiamo togliere
    for (std::size_t i = 0; i < num_removals;) {
#ifdef USE_PCG32
        uint32_t removed = rng() % working_set;
#else
        uint32_t removed = rand() % working_set;
#endif
        if (nodes[removed] == 1) {
            auto rnode = engine.removeBucket(removed);
            if (!nodes[rnode]) {
                throw "Crazy bug";
            }
            nodes[rnode] = 0; // Remove the actually removed 
            i++;
        }
    }


    // rimettiamo i nodi con le stesse keys
    for (const auto& current_random_number : random_sequence) {
        auto a = current_random_number.first;
        auto b = current_random_number.second;
        // dovrebbe tornare lo stesso target
        auto target = engine.getBucketCRC32c(a, b);
        // se l'indice ritornato non è l'indice che abbiamo salvato prima e se il nodo originale è spento
        // il nodo si è spostato
        if (target != bucket[current_random_number] && nodes[bucket[current_random_number]] == 0) {

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
                a, b, oldbucket, newbucket, nodes[oldbucket],
                nodes[newbucket]);
            ++misplaced;
        }
    }

    double m = (double)misplaced / (num_keys);

#ifdef USE_PCG32
    fmt::println(
        "{}: after removal % misplaced keys are {}% ({} keys out of {})\n", name,
        m * 100, misplaced, num_keys);
#else
    fmt::println("{}: after removal misplaced keys are {}% ({} keys out of {})",
        name, m * 100, misplaced, num_keys);
#endif

    misplaced = 0;
    // Add back a node
    auto anode = engine.addBucket();
    nodes[anode] = 1;
    fmt::println("Added node {}", anode);

    for (const auto& i : bucket) {
        auto oldbucket = i.second;
        auto a{ i.first.first };
        auto b{ i.first.second };
        auto newbucket = engine.getBucketCRC32c(a, b);
        if (oldbucket != newbucket) {
            fmt::println("(After Add) Misplaced key {},{}: before in bucket {}, now "
                "in bucket {} (status? old bucket {}, new bucket {})",
                a, b, oldbucket, newbucket, nodes[oldbucket],
                nodes[newbucket]);
            ++misplaced;
        }
    }

    m = (double)misplaced / (num_keys);

#ifdef USE_PCG32
    fmt::println(
        "{}: after adding back % misplaced keys are {}% ({} keys out of {})\n",
        name, m * 100, misplaced, num_keys);
#else
    fmt::println(
        "{}: after adding back misplaced keys are {}% ({} keys out of {})", name,
        m * 100, misplaced, num_keys);
#endif

    ////////////////////////////////////////////////////////////////////

    delete[] nodes;
}


inline int monotonicity(const std::string& output_path, const BenchmarkSettings& current_benchmark,
    const std::vector<AlgorithmSettings>& algorithms) {

    const auto& fractions = parse_fractions(current_benchmark.args.at("fractions"));
    if (!current_benchmark.args.count("fractions")) {
        fmt::println("no fractions key found in the yaml file.");
        return 1;
    }
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
    if (current_benchmark.args.count("keyMultiplier") > 0) {
        key_multiplier = parse_key_multiplier(current_benchmark.args.at("keyMultiplier"));
    }

    for (double current_fraction : fractions) {
        for (const auto& hash_function : current_benchmark.commonSettings.hashFunctions) { // Done for all benchmarks
            for (const auto& current_algorithm : algorithms) {
                for (const auto& key_distribution : current_benchmark.commonSettings.keyDistributions) { // Done for all benchmarks
                    for (const auto& working_set : current_benchmark.commonSettings.numInitialActiveNodes) {
                        Monotonicity monotonicity;
                        monotonicity.distribution = key_distribution;
                        monotonicity.hash_function = hash_function;
                        monotonicity.nodes = working_set;
                        monotonicity.keys = key_multiplier * working_set;
                        monotonicity.algorithm_name = current_algorithm.name;
                        monotonicity.fraction = current_fraction;

                        auto& monotonicity_writer = CsvWriter<Monotonicity>::getInstance("./", "monotonicity.csv");

                        const std::string full_file_path = output_path + "/" + current_algorithm.name + ".txt";

                        const uint32_t num_removals = static_cast<uint32_t>(current_fraction * working_set);
                        uint32_t capacity = 10 * working_set; // default capacity = 10

                        if (current_algorithm.args.contains("capacity")) {
                            try {
                                capacity = std::stoi(current_algorithm.args.at("capacity")) * working_set;
                            }
                            catch (const std::exception& e) {
                                std::cerr << "std::stoi exception: " << e.what() << '\n';
                            }
                        }

                        if (current_algorithm.name == "null") {
                            // do nothing
                            return 0;
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
                                num_removals, key_multiplier * working_set, current_fraction,
                                monotonicity);
                        }
                        else if (current_algorithm.name == "memento") {
                            bench<MementoEngine<boost::unordered_flat_map>>(
                                "Memento<boost::unordered_flat_map>", full_file_path, capacity, working_set,
                                num_removals, key_multiplier * working_set, current_fraction,
                                monotonicity);
                        }
                        else if (current_algorithm.name == "mementoboost") {
                            bench<MementoEngine<boost::unordered_map>>(
                                "Memento<boost::unordered_map>", full_file_path, capacity, working_set,
                                num_removals, key_multiplier * working_set, current_fraction,
                                monotonicity);
                        }
                        else if (current_algorithm.name == "mementostd") {
                            bench<MementoEngine<std::unordered_map>>(
                                "Memento<std::unordered_map>", full_file_path, capacity, working_set,
                                num_removals, key_multiplier * working_set, current_fraction,
                                monotonicity);
                        }
                        else if (current_algorithm.name == "mementogtl") {
                            bench<MementoEngine<gtl::flat_hash_map>>(
                                "Memento<std::gtl::flat_hash_map>", full_file_path, capacity, working_set,
                                num_removals, key_multiplier * working_set, current_fraction,
                                monotonicity);
                        }
                        else if (current_algorithm.name == "mementomash") {
                            bench<MementoEngine<MashTable>>("Memento<MashTable>", full_file_path,
                                capacity, working_set,
                                num_removals, key_multiplier * working_set, current_fraction,
                                monotonicity);
                        }
                        else if (current_algorithm.name == "jump") {
                            bench<JumpEngine>("JumpEngine", full_file_path, capacity, working_set,
                                num_removals, key_multiplier * working_set, current_fraction,
                                monotonicity);
                        }
                        else if (current_algorithm.name == "power") {
                            bench<PowerEngine>("PowerEngine", full_file_path, capacity, working_set,
                                num_removals, key_multiplier * working_set, current_fraction,
                                monotonicity);
                        }
                        else if (current_algorithm.name == "dx") {
                            bench<DxEngine>("DxEngine", full_file_path, capacity, working_set,
                                num_removals, key_multiplier * working_set, current_fraction,
                                monotonicity);
                        }
                        else {
                            fmt::println("Unknown algorithm {}", current_algorithm.name);
                            //return 2;
                        }
                        monotonicity_writer.add(monotonicity);
                    }
                }
            }
        }
    }
    return 0;
}

#endif