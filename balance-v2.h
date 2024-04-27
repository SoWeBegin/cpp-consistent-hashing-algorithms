/*
 * Copyright (c) 2023 Amos Brocco, Tony Kolarek, Tatiana Dal Busco.
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

#ifndef BALANCE_BENCH_H
#define BALANCE_BENCH_H

#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered_map.hpp>
#include <cxxopts.hpp>
#ifdef USE_PCG32
#include "pcg_random.hpp"
#include <random>
#endif
#include "anchor/anchorengine.h"
#include "memento/mashtable.h"
#include "memento/mementoengine.h"
#include "jump/jumpengine.h"
#include "power/powerengine.h"
#include <fmt/core.h>
#include <fstream>
#include <unordered_map>
#include <gtl/phmap.hpp>
#include "dx/dxEngine.h"
#include "csvWriter.h"
#include "utils.h"

 /*
  * Benchmark routine
  */
template <typename Algorithm>
inline void bench(const std::string& name, const std::string file_name,
    std::size_t anchor_set /* capacity */, std::size_t working_set,
    uint32_t num_removals, uint32_t num_keys, Balance& balance) {
#ifdef USE_PCG32
    pcg_extras::seed_seq_from<std::random_device> seed;
    pcg32 rng{ seed };
#else
    srand(time(NULL));
#endif
    Algorithm engine(anchor_set, working_set);

    // for lb
    uint32_t* anchor_ansorbed_keys = new uint32_t[anchor_set]();

    // random removals
    uint32_t* bucket_status = new uint32_t[anchor_set]();

    for (uint32_t i = 0; i < working_set; i++) {
        bucket_status[i] = 1;
    }

    uint32_t i = 0;
    while (i < num_removals) {
#ifdef  USE_PCG32
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

    std::ofstream results_file;
    results_file.open(name, std::ofstream::out | std::ofstream::app);

    ////////////////////////////////////////////////////////////////////
    for (uint32_t i = 0; i < num_keys; ++i) {
#ifdef USE_PCG32
        anchor_ansorbed_keys[engine.getBucketCRC32c(rng(), rng())] += 1;
#else
        anchor_ansorbed_keys[engine.getBucketCRC32c(rand(), rand())] += 1;
#endif
    }

    // check load balancing
    double mean = (double)num_keys / (working_set - num_removals);

    double lb = 0;
    for (uint32_t i = 0; i < anchor_set; i++) {

        if (bucket_status[i]) {

            if (anchor_ansorbed_keys[i] / mean > lb) {
                lb = anchor_ansorbed_keys[i] / mean;
            }

        }

        else {
            if (anchor_ansorbed_keys[i] > 0) {
                fmt::println("{}: crazy bug!", name);
            }
        }
    }


    // print lb res
#ifdef USE_PCG32
    fmt::println("{}: LB is {}\n", name, lb);
    results_file << name << ": "
        << "Balance: " << lb << "\tPCG32\n";
#else
    fmt::println("{}: LB is {}\n", name, lb);
    results_file << name << ": "
        << "Balance: " << lb << "\trand()\n";
#endif

    ////////////////////////////////////////////////////////////////////

    results_file.close();

    delete[] bucket_status;
    delete[] anchor_ansorbed_keys;

    return 0;
}


inline int balance(const std::string& output_path, std::size_t working_set, const std::string& hash_function,
    const std::string& key_distribution, const std::vector<AlgorithmSettings>& algorithms,
    const std::unordered_map<std::string, std::string>& args) {

    std::size_t key_multiplier = 100;
    if (args.count("keyMultiplier") > 0) {
        key_multiplier = parse_key_multiplier(args.at("keyMultiplier"));
    }

    Balance balance;
    balance.distribution = key_distribution;
    balance.hash_function = hash_function;
    balance.nodes = working_set;
    balance.keys = key_multiplier * working_set;
   
    srand(time(NULL));
  
    for (const auto& current_algorithm : algorithms) {
        const std::string filename = output_path + "/" + current_algorithm.name + ".txt";
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
            bench<AnchorEngine>("Anchor", filename, capacity, working_set,
                num_removals, key_multiplier * working_set, balance);
        }
        else if (current_algorithm.name == "memento") {
            bench<MementoEngine<boost::unordered_flat_map>>(
                "Memento<boost::unordered_flat_map>", filename, capacity, working_set,
                num_removals, key_multiplier * working_set, balance);
        }
        else if (current_algorithm.name == "mementoboost") {
            bench<MementoEngine<boost::unordered_map>>(
                "Memento<boost::unordered_map>", filename, capacity, working_set,
                num_removals, key_multiplier * working_set, balance);
        }
        else if (current_algorithm.name == "mementostd") {
            bench<MementoEngine<std::unordered_map>>(
                "Memento<std::unordered_map>", filename, capacity, working_set,
                num_removals, key_multiplier * working_set, balance);
        }
        else if (current_algorithm.name == "mementogtl") {
            bench<MementoEngine<gtl::flat_hash_map>>(
                "Memento<std::gtl::flat_hash_map>", filename, capacity, working_set,
                num_removals, key_multiplier * working_set, balance);
        }
        else if (current_algorithm.name == "mementomash") {
            bench<MementoEngine<MashTable>>("Memento<MashTable>", filename,
                capacity, working_set,
                num_removals, key_multiplier * working_set, balance);
        }
        else if (current_algorithm.name == "jump") {
            bench<JumpEngine>("JumpEngine", filename,
                capacity, working_set,
                num_removals, key_multiplier * working_set, balance);
        }
        else if (current_algorithm.name == "power") {
            bench<PowerEngine>("PowerEngine", filename,
                capacity, working_set,
                num_removals, key_multiplier * working_set, balance);
        }
        else if (current_algorithm.name == "dx") {
            bench<DxEngine>("DxPower", filename, capacity, working_set, 
                num_removals, key_multiplier * working_set, balance);
        }
        else {
            fmt::println("Unknown algorithm {}", current_algorithm.name);
          
        }
    }
    auto& balance_writer = CsvWriter<Balance>::getInstance("./", "balance.csv");
    balance_writer.add(balance);

}

#endif