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
#include <type_traits>
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
#include <string>
#include <gtl/phmap.hpp>    
#include <unordered_map>
#include <vector>
#include "dx/dxEngine.h"
#include "YamlParser/YamlParser.h"
#include "csvWriter.h"
#include "utils.h"


/* Uenerates the map for storing detailed benchmark results
 * [key = specific benchmark measured, value = vector of nodes indexes]
 */
inline std::unordered_map<std::string, std::vector<uint32_t>>
initialize_bench_results(std::size_t working_set, std::initializer_list<std::string> init_list) {

    std::unordered_map<std::string, std::vector<uint32_t>> ret;
    for (auto current : init_list) {
        ret[current] = std::vector<uint32_t>(working_set);
    }
    return ret;
}

// utility function to avoid having to repeat the same loops over and over again
template<typename Map, typename T>
inline void count_keys_greater_than_one(const Map& results, const std::string& key, T& counter) {
    static_assert(std::is_same_v<Map, std::unordered_map<std::string, std::vector<uint32_t>>>,
        "Map type must be std::unordered_map<std::string, std::vector<uint32_t>>");
    static_assert(std::is_same_v<decltype(++counter), T&>, "Counter type must be incrementable");
   
    for (const auto& total_keys : results.at(key)) {
        if (total_keys >= 1) {
            ++counter;
        }
    }
}

  /*
   * Benchmark routine
   */
template <typename Algorithm>
inline void bench(const std::string& name,
    std::size_t anchor_set, std::size_t working_set,
    uint32_t num_removals, uint32_t num_keys, double current_fraction,
    Monotonicity& monotonicity) {

    auto bench_results = initialize_bench_results(working_set,
        { "keys_per_node", "moved_from_removed_nodes",
        "moved_from_other_nodes", "moved_to_restored_nodes",
        "moved_to_other_nodes", "relocated_after_resize"});

    Algorithm engine(anchor_set, working_set);

    // anchor_set = total nodes, not necessarily all used now
    uint32_t* nodes = new uint32_t[anchor_set]();

    // working_set = nodes that we currently use
    for (uint32_t i = 0; i < working_set; i++) {
        nodes[i] = 1; // 1 = working node; 0 = non-working node
    }

    // bucket_before_remove: represents the buckets {Key, Value} before removing the nodes.
    // Key = represents a piece of data that we want to store. In our case, this is a random value.
    // Value = index of the node to which the Key is linked to, before we remove nodes.
    boost::unordered_flat_map<std::pair<uint32_t, uint32_t>, uint32_t> bucket_before_remove;

    const auto random_keys = generate_random_keys_sequence(num_keys);

    // First, we start by linking each key of the bucket to the available nodes.
    // One node can end up having multiple keys linked to it.
    for (const auto& current_random_number : random_keys) {
        const auto a = current_random_number.first;
        const auto b = current_random_number.second;
        if (bucket_before_remove.contains(current_random_number)) {
            continue; // we found a key that we already inserted
        }

        // target_node_pos: index of the node where the key {a,b} will be placed into.
        const auto target_node_pos = engine.getBucketCRC32c(a, b);

        // We link the key {a,b} to the target_node_pos.
        // Equivalent to bucket.insert(key, value) where key=current_random_number aka 
        // {a,b} and target=index of node
        bucket_before_remove[current_random_number] = target_node_pos;

        // We added a key inside target_node_pos, so we increment the number of keys for target_node_pos.
        bench_results["keys_per_node"][target_node_pos]++;

        // Verify that we got a working node
        if (!nodes[target_node_pos]) {
            delete[] nodes;
            throw "Crazy bug";
        }
    }

    // Next, we remove num_removals working nodes
    // num_removals: how many nodes we should remove 
    for (std::size_t i = 0; i < num_removals;) {
        // removed = random value, which represents a random node to remove
#ifdef USE_PCG32
        const uint32_t removed = rng() % working_set;
#else
        const uint32_t removed = rand() % working_set;
#endif
        // check that this node has not been removed yet.
        if (nodes[removed] == 1) { 
            const auto removed_node = engine.removeBucket(removed);
            if (!nodes[removed_node]) {
                // engine.removeBucket(removed) returned a node that
                // was already removed by the engine: this can't be a valid case.
                delete[] nodes;
                throw "Crazy bug";
            }
            nodes[removed_node] = 0; // Actually turn off the removed node.

            // We take the total keys inside removed_node and add that to our monotonicity.keys_in_removed_node,
            // since we are turning this removed_node off.
            monotonicity.keys_in_removed_nodes += bench_results["keys_per_node"][removed_node]; 

            ++i;
        }
    }

    // bucket_after_remove: represents the buckets {Key, Value} after removing the nodes.
    // Since we removed some working nodes, we now need to know in which new nodes the keys
    // of the removed nodes were moved to.
    boost::unordered_flat_map<std::pair<uint32_t, uint32_t>, uint32_t> bucket_after_remove;

    // Next, we check how many keys were moved from the removed nodes to other nodes
    for (const auto& current_random_number : random_keys) {
        const auto a = current_random_number.first;
        const auto b = current_random_number.second;

        // We removed some nodes, thus their keys are moved to new nodes. We store the nodes
        // where the keys were moved to, since we need this information later.
        bucket_after_remove[current_random_number] = engine.getBucketCRC32c(a, b);
        const auto target_pos_after_remove = bucket_after_remove[current_random_number];
        const auto target_pos_before_remove = bucket_before_remove[current_random_number];
  
        // The key was moved since the node it came from was removed
        if (target_pos_after_remove != target_pos_before_remove && !nodes[target_pos_before_remove]) {
            bench_results["moved_from_removed_nodes"][target_pos_before_remove]++;
            monotonicity.keys_moved_from_removed_nodes++;
        }
        // The key remained in the same node, even if we removed that node
        // This case is not valid: we can't have a key inside a node that was removed
        else if (target_pos_after_remove == target_pos_before_remove && !nodes[target_pos_before_remove]) {
            delete[] nodes;
            throw "Crazy bug";
        }
        // The key remained in the same node, and that node is still active
        // We don't store this information, but ideally this should always happen, or at least in most cases
        else if (target_pos_after_remove == target_pos_before_remove && nodes[target_pos_before_remove]) {
        }
        // The key was moved even though the node it came from is still active: this should ideally happen sporadically
        else if (target_pos_after_remove != target_pos_before_remove && nodes[target_pos_before_remove]) {
            bench_results["moved_from_other_nodes"][target_pos_before_remove]++;
            monotonicity.keys_moved_from_other_nodes++;
        }
    }

    // Next, we re-add the nodes that we removed before.
    for (std::size_t i = 0; i < num_removals; ++i) {
       nodes[engine.addBucket()] = 1; 
    }

    // We now want to check which keys are moved back to their original nodes and which aren't.
    // Ideally, most if not all keys are moved back to their original nodes.
    for (const auto& current_random_number : random_keys) {
        const auto a = current_random_number.first;
        const auto b = current_random_number.second;

        const auto target_pos_after_restore = engine.getBucketCRC32c(a, b);
        const auto target_pos_before_remove = bucket_before_remove[current_random_number];
        const auto target_pos_after_remove = bucket_after_remove[current_random_number];

        // The key moved from the node where it was after the removal to the newly restored node,
        // and the newly restored node is the same node that was removed initially
        // NOTE: we have an additional condition compared to the code in Java (the second one)
        // since we don't remove the last N nodes, but instead we remove them randomly, and thus have no way
        // to know whether the keys were moved back to the original nodes or not otherwise
        if (target_pos_after_restore != target_pos_after_remove && target_pos_after_restore == target_pos_before_remove) {
            bench_results["moved_to_restored_nodes"][target_pos_after_restore]++; 
            monotonicity.keys_moved_to_restored_nodes++;
        }
    
        // The key moved from the node where it was after the removal to the newly restored node,
        // and the newly restored node is not the same that was removed initially
        // thus the key was moved to a completely differet node, this should ideally happen sporadically
        else if (target_pos_after_restore != target_pos_after_remove && target_pos_after_restore != target_pos_before_remove) {
            bench_results["moved_to_other_nodes"][target_pos_after_restore]++; 
            monotonicity.keys_moved_to_other_nodes++;
        }

        // The key moved from the original removed node to a new node, but after that it still holds that
        // target_pos_after_restore == target_pos_after_remove, and both are different than target_pos_before_remove, 
        // meaning that the key that was originally in the removed node did not move back to it even after we restored it.
        else if (target_pos_after_restore != target_pos_before_remove) {
            bench_results["relocated_after_resize"][target_pos_before_remove]++;
            monotonicity.keys_relocated_after_resize++;
        }
    }
 
    count_keys_greater_than_one(bench_results, "moved_from_removed_nodes", monotonicity.nodes_losing_keys);
    count_keys_greater_than_one(bench_results, "moved_from_other_nodes", monotonicity.nodes_losing_keys);
    count_keys_greater_than_one(bench_results, "moved_to_restored_nodes", monotonicity.nodes_gaining_keys);
    count_keys_greater_than_one(bench_results, "moved_to_other_nodes", monotonicity.nodes_gaining_keys);
    count_keys_greater_than_one(bench_results, "relocated_after_resize", monotonicity.nodes_changed_after_resize);

    const auto total_moved_from = monotonicity.keys_moved_from_other_nodes + monotonicity.keys_moved_from_removed_nodes;
    const auto total_moved_to = monotonicity.keys_moved_to_restored_nodes + monotonicity.keys_moved_to_other_nodes;

    monotonicity.keys_moved_from_other_nodes_percentage = monotonicity.keys_moved_from_other_nodes / static_cast<double>(total_moved_from);
    monotonicity.keys_moved_from_removed_nodes_percentage = monotonicity.keys_moved_from_removed_nodes / static_cast<double>(total_moved_from);
    monotonicity.nodes_losing_keys_percentage = monotonicity.nodes_losing_keys / working_set;
    monotonicity.keys_moved_to_restored_nodes_percentage = monotonicity.keys_moved_to_restored_nodes / static_cast<double>(total_moved_to);
    monotonicity.keys_moved_to_other_nodes_percentage = monotonicity.keys_moved_to_other_nodes / static_cast<double>(total_moved_to);
    monotonicity.nodes_gaining_keys_percentage = monotonicity.nodes_gaining_keys / working_set;
    monotonicity.keys_relocated_after_resize_percentage = monotonicity.keys_relocated_after_resize / static_cast<double>(num_keys);
    monotonicity.nodes_changed_after_resize_percentage = monotonicity.nodes_changed_after_resize / static_cast<double>(working_set);

    delete[] nodes;
}


inline void monotonicity(const std::string& output_path, const BenchmarkSettings& current_benchmark,
    const std::vector<AlgorithmSettings>& algorithms) {

    const auto& fractions = parse_fractions(current_benchmark.args.at("fractions"));
    if (!current_benchmark.args.count("fractions")) {
        fmt::println("no fractions key found in the yaml file.");
    }
    if (fractions.size() < 1) {
        fmt::println("fractions must have at least 1 value");
    }
    for (double current_fraction : fractions) {
        if (current_fraction >= 0 && current_fraction < 1) continue;
        fmt::println("fraction values must be >= 0 and < 1");
    }

    std::size_t key_multiplier = 100;
    if (current_benchmark.args.count("keyMultiplier") > 0) {
        key_multiplier = parse_key_multiplier(current_benchmark.args.at("keyMultiplier"));
    }

    auto& monotonicity_writer = CsvWriter<Monotonicity>::getInstance("./", "monotonicity.csv");

    for (double current_fraction : fractions) {
        for (const auto& hash_function : current_benchmark.commonSettings.hashFunctions) { // Done for all benchmarks
            for (const auto& current_algorithm : algorithms) {
                for (const auto& key_distribution : current_benchmark.commonSettings.keyDistributions) { // Done for all benchmarks
                    for (const auto& working_set : current_benchmark.commonSettings.numInitialActiveNodes) {

                        Monotonicity monotonicity(hash_function, current_algorithm.name, current_fraction,
                            key_multiplier * working_set, key_distribution, working_set);

                        const uint32_t num_removals = static_cast<uint32_t>(current_fraction * working_set);
                        uint32_t capacity = working_set * 10; // default capacity = 10
                        if (current_algorithm.args.contains("capacity")) {
                            try {
                                capacity = std::stoi(current_algorithm.args.at("capacity")) * working_set;
                            } catch (const std::exception& e) {
                                std::cerr << "std::stoi exception: " << e.what() << '\n';
                            }
                        }

                        if (current_algorithm.name == "null") {
                            return;
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
                            bench<AnchorEngine>("Anchor", capacity, working_set,
                                num_removals, key_multiplier * working_set, current_fraction,
                                monotonicity);
                        }
                        else if (current_algorithm.name == "memento") {
                            bench<MementoEngine<boost::unordered_flat_map>>(
                                "Memento<boost::unordered_flat_map>", capacity, working_set,
                                num_removals, key_multiplier * working_set, current_fraction,
                                monotonicity);
                        }
                        else if (current_algorithm.name == "mementoboost") {
                            bench<MementoEngine<boost::unordered_map>>(
                                "Memento<boost::unordered_map>", capacity, working_set,
                                num_removals, key_multiplier * working_set, current_fraction,
                                monotonicity);
                        }
                        else if (current_algorithm.name == "mementostd") {
                            bench<MementoEngine<std::unordered_map>>(
                                "Memento<std::unordered_map>", capacity, working_set,
                                num_removals, key_multiplier * working_set, current_fraction,
                                monotonicity);
                        }
                        else if (current_algorithm.name == "mementogtl") {
                            bench<MementoEngine<gtl::flat_hash_map>>(
                                "Memento<std::gtl::flat_hash_map>", capacity, working_set,
                                num_removals, key_multiplier * working_set, current_fraction,
                                monotonicity);
                        }
                        else if (current_algorithm.name == "mementomash") {
                            bench<MementoEngine<MashTable>>("Memento<MashTable>",
                                capacity, working_set,
                                num_removals, key_multiplier * working_set, current_fraction,
                                monotonicity);
                        }
                        else if (current_algorithm.name == "jump") {
                            bench<JumpEngine>("JumpEngine", capacity, working_set,
                                num_removals, key_multiplier * working_set, current_fraction,
                                monotonicity);
                        }
                        else if (current_algorithm.name == "power") {
                            bench<PowerEngine>("PowerEngine", capacity, working_set,
                                num_removals, key_multiplier * working_set, current_fraction,
                                monotonicity);
                        }
                        else if (current_algorithm.name == "dx") {
                            bench<DxEngine>("DxEngine", capacity, working_set,
                                num_removals, key_multiplier * working_set, current_fraction,
                                monotonicity);
                        }
                        else {
                            fmt::println("Unknown algorithm {}", current_algorithm.name);
                        }
                        monotonicity_writer.add(monotonicity);
                    }
                }
            }
        }
    }
}

#endif