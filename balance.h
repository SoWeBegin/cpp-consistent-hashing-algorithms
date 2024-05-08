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
#include "YamlParser/YamlParser.h"
#include <vector>

 /*
 * ******************************************
 * Benchmark routine
 * ******************************************
 */
template <typename Algorithm, typename T>
inline void bench(const std::string& name,
    std::size_t anchor_set /* capacity */, std::size_t working_set,
    uint32_t num_keys, std::size_t iterations, Balance& balance,
    random_distribution_ptr<T> random_fnt) {

    Algorithm engine(anchor_set, working_set);

    // For each given iteration, we want to store the total number of keys for each node
    // since we need to find out which nodes have the min/max number of keys for each iteration,
    // and thus calculate the average for both min and max.
    std::vector<std::vector<uint32_t>> keys_per_node(iterations, std::vector<uint32_t>(working_set));

    for (std::size_t current_iteration = 0; current_iteration < iterations; ++current_iteration) {
        const auto random_keys = generate_random_keys_sequence(num_keys, random_fnt);

        for (const auto& current_random_key : random_keys) {
            const auto a = current_random_key.first;
            const auto b = current_random_key.second;
            const auto target_node = engine.getBucketCRC32c(a, b);

            // Keep track of the number of keys for each node, for all iterations.
            keys_per_node[current_iteration][target_node]++;
        }
    }

    // Finds the max or min value (depending on the compare functor passed) in a given vector.
    // In our case, the vector represents the nodes and their respective number of stored keys.
    auto find_maxmin = [](const std::vector<uint32_t>& vec, std::function<bool(uint32_t, uint32_t)> compare) {
        std::size_t maxmin = vec[0];
        for (const auto& val : vec) {
            if (compare(val, maxmin)) {
                maxmin = val;
            }
        }
        return maxmin;
        };

    // Calculates the average value given a vector of nodes containing K keys.
    auto calculate_average = [](const std::vector<uint32_t>& vec) {
        double avg = 0.;
        for (const auto& value : vec) {
            avg += value;
        }
        return avg / vec.size();
        };

    std::vector<uint32_t> max_values_per_iteration(iterations);
    std::vector<uint32_t> min_values_per_iteration(iterations);
    for (std::size_t idx = 0; idx < iterations; ++idx) {
        max_values_per_iteration[idx] = find_maxmin(keys_per_node[idx], std::greater<uint32_t>());
        min_values_per_iteration[idx] = find_maxmin(keys_per_node[idx], std::less<uint32_t>());
    }

    balance.max = calculate_average(max_values_per_iteration);
    balance.min = calculate_average(min_values_per_iteration);
    balance.max_percentage = balance.max * working_set / num_keys;
    balance.min_percentage = balance.min * working_set / num_keys;
    balance.expected = num_keys / working_set;
}

template<typename T>
inline void balance(CsvWriter<Balance>& balance_writer,
    const std::string& output_path, const BenchmarkSettings& current_benchmark,
    const std::vector<AlgorithmSettings>& algorithms, std::size_t iterations,
    const std::unordered_map<std::string, random_distribution_ptr<T>>& distribution_function) {
    
    uint32_t key_multiplier = 100;
    if (current_benchmark.args.count("keyMultiplier")) {
        key_multiplier = str_to<uint32_t>(current_benchmark.args.at("keyMultiplier"), 100);
    }
        
    for (const auto& hash_function : current_benchmark.commonSettings.hashFunctions) { // Done for all benchmarks
        for (const auto& current_algorithm : algorithms) {
            for (const auto& key_distribution : current_benchmark.commonSettings.keyDistributions) { // Done for all benchmarks
                for (const auto& working_set : current_benchmark.commonSettings.numInitialActiveNodes) {

                    Balance balance(hash_function, current_algorithm.name, working_set * key_multiplier,
                        key_distribution, working_set, iterations);

                    // Callback function to generate a random value (key) depending on which
                    // distribution was found inside the yaml file.
                    random_distribution_ptr<T> random_gen_fnt_ptr;
                    if (distribution_function.count(key_distribution)) {
                        random_gen_fnt_ptr = distribution_function.at(key_distribution);
                    } else {
                        fmt::println("[Balance] The specified distribution is not available. Proceeding with default UNIFORM");
                        random_gen_fnt_ptr = distribution_function.at("uniform");
                    }

                    // Even though not all algorithms actually use this value, we still pass it
                    // so that we can instantiate any algorithm in a generic way.
                    uint32_t capacity = working_set * 10; // default = 10
                    if (current_algorithm.args.count("capacity")) {
                        capacity = str_to<uint32_t>(current_algorithm.args.at("capacity"), 10) * working_set;
                    }

                    if (current_algorithm.name == "anchor") {
                        bench<AnchorEngine>("Anchor", capacity, working_set,
                            key_multiplier * working_set, iterations, balance, random_gen_fnt_ptr);
                    }
                    else if (current_algorithm.name == "memento") {
                        bench<MementoEngine<boost::unordered_flat_map>>(
                            "Memento<boost::unordered_flat_map>", capacity, working_set,
                            key_multiplier * working_set, iterations, balance, random_gen_fnt_ptr);
                    }
                    else if (current_algorithm.name == "mementoboost") {
                        bench<MementoEngine<boost::unordered_map>>(
                            "Memento<boost::unordered_map>", capacity, working_set,
                            key_multiplier * working_set, iterations, balance, random_gen_fnt_ptr);
                    }
                    else if (current_algorithm.name == "mementostd") {
                        bench<MementoEngine<std::unordered_map>>(
                            "Memento<std::unordered_map>", capacity, working_set,
                            key_multiplier * working_set, iterations, balance, random_gen_fnt_ptr);
                    }
                    else if (current_algorithm.name == "mementogtl") {
                        bench<MementoEngine<gtl::flat_hash_map>>(
                            "Memento<std::gtl::flat_hash_map>", capacity, working_set,
                            key_multiplier * working_set, iterations, balance, random_gen_fnt_ptr);
                    }
                    else if (current_algorithm.name == "mementomash") {
                        bench<MementoEngine<MashTable>>("Memento<MashTable>",
                            capacity, working_set,
                            key_multiplier * working_set, iterations, balance, random_gen_fnt_ptr);
                    }
                    else if (current_algorithm.name == "jump") {
                        bench<JumpEngine>("JumpEngine",
                            capacity, working_set,
                            key_multiplier * working_set, iterations, balance, random_gen_fnt_ptr);
                    }
                    else if (current_algorithm.name == "power") {
                        bench<PowerEngine>("PowerEngine",
                            capacity, working_set,
                            key_multiplier * working_set, iterations, balance, random_gen_fnt_ptr);
                    }
                    else if (current_algorithm.name == "dx") {
                        bench<DxEngine>("DxPower", capacity, working_set,
                            key_multiplier * working_set, iterations, balance, random_gen_fnt_ptr);
                    }
                    else {
                        fmt::println("[Balance] Unknown algorithm {}", current_algorithm.name);
                    }

                    balance_writer.add(balance);
                }
            }
        }
    }
}

#endif