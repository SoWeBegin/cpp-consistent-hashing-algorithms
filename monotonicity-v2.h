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

/*
 * this function generates a sequence of random keys, with key = {random(a), random(b)}
 * num_keys: total keys to generate randomly
 * return: a vector of keys
 */
inline std::vector<std::pair<uint32_t, uint32_t>> 
generate_random_keys_sequence(std::size_t num_keys) {

    std::vector<std::pair<uint32_t, uint32_t>> ret;
    for (uint32_t i = 0; i < num_keys; ++i) {
#ifdef USE_PCG32
        const auto a{ rng() };
        const auto b{ rng() };
#else
        const auto a{ rand() };
        const auto b{ rand() };
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
    std::size_t anchor_set, std::size_t working_set,
    uint32_t num_removals, uint32_t num_keys, double current_fraction,
    Monotonicity& monotonicity) {

    uint32_t keys_moved_from_removed_nodes = 0; // OK
    uint32_t keys_moved_from_other_nodes = 0; // OK
    uint32_t keys_moved_to_restored_nodes = 0; //OK
    uint32_t keys_moved_to_other_nodes = 0; // Ok
    uint32_t nodes_losing_keys = 0;
    uint32_t nodes_gaining_keys = 0;
    uint32_t keys_relocated_after_resize = 0;
    uint32_t nodes_changed_after_resize = 0;
    uint32_t keys_in_removed_nodes = 0; // OK

    std::vector<uint32_t> keys_per_node;
    std::vector<uint32_t> moved_from_removed_nodes;
    std::vector<uint32_t> moved_from_other_nodes;
    std::vector<uint32_t> moved_to_restored_nodes;
    std::vector<uint32_t> moved_to_other_nodes;

    Algorithm engine(anchor_set, working_set);

    // anchor_set = total nodes, not necessarily all used now
    uint32_t* nodes = new uint32_t[anchor_set]();

    // working_set = nodes that we currently use
    for (uint32_t i = 0; i < working_set; i++) {
        nodes[i] = 1; // 1 = working node; 0 = non-working node
    }

    // bucket: represents a map of buckets, then assigned to a given node.
    // the value of the bucket is the index of the node to which the
    // respective key is linked to. The key is the "data", which in our case
    // is a randomly generated key.
    boost::unordered_flat_map<std::pair<uint32_t, uint32_t>, uint32_t> bucket;

    const auto random_keys = generate_random_keys_sequence(num_keys);

    // 1. First, we start by assigning the bucket to the nodes.
    // Basically we need to link each bucket to a node.
    for (const auto& current_random_number : random_keys) {
        const auto a = current_random_number.first;
        const auto b = current_random_number.second;
        if (bucket.contains(current_random_number)) {
            continue; // we found a key that we already inserted
        }
        // target: index of the node to which we assign this bucket.
        const auto target_node = engine.getBucketCRC32c(a, b);

        // This bucket is linked to the target node. 
        // Equivalent to bucket.add(key, value) 
        // where key=current_random_number and target=index of node
        // bucket.insert(current_random_number, target_node); => same as insert{Key, Value}
        bucket[current_random_number] = target_node;
        keys_per_node[target_node]++;

        // Verify that we got a working node
        if (!nodes[target_node]) {
            delete[] nodes;
            throw "Crazy bug";
        }
    }

    // 2. Remove num_removals working nodes
    // num_removals: how many nodes we should remove 
    for (std::size_t i = 0; i < num_removals;) {
        // removed = random value, which represents a random node to remove
#ifdef USE_PCG32
        const uint32_t removed = rng() % working_set;
#else
        const uint32_t removed = rand() % working_set;
#endif
        // check that this node has not been removed yet.
        // this check is only needed for those algorithms
        // that support random removals, since they return
        // the same value (removed) that we passed to removeBucket.
        // i.e. nodes[removed_node] in such cases is the same as nodes[removed].

        if (nodes[removed] == 1) { 
            const auto removed_node = engine.removeBucket(removed);
            if (!nodes[removed_node]) {
                // engine.removeBucket(removed) returned a node that
                // was already removed by the engine: this can't be a valid case.
                delete[] nodes;
                throw "Crazy bug";
            }
            nodes[removed_node] = 0;
            ++i;
            nodes_losing_keys++;
            //keys_in_removed_nodes++;
        }
    }

    // Keys in removed nodes: number of keys in nodes to be removed before the removal of the nodes.
    // Keys moved from removed nodes: number of keys moved from a removed node to another after the removal. Ideally near 1000.
    // Keys moved from other nodes: number of keys moved from a working node to another node after removal of nodes. Ideally near 0.
    for (const auto& current_random_number : random_keys) {
        const auto a = current_random_number.first;
        const auto b = current_random_number.second;
        // ideally we should get the same target as before before.
        const auto new_target = engine.getBucketCRC32c(a, b);
        const auto old_target = bucket[current_random_number];
        
  
        if (new_target != old_target && nodes[old_target] == 0) {
            // All the keys that were moved from the removed node nodes[old_target].
            // => KeysMovedFromRemovedNodes
            //keys_moved_from_removed_nodes++;
            moved_from_removed_nodes[old_target]++;
        }
        else if (new_target == old_target && nodes[old_target] == 0) {
 
           
        }
        else if (new_target == old_target && nodes[old_target] == 1) {

            
        }
        else if (new_target != old_target && nodes[old_target] == 1) {
            // Other keys that were moved from the nodes that are still active, nodes[new_target]
            // => KeysMovedFromOtherNodes
            //keys_moved_from_other_nodes++;
            moved_from_other_nodes[old_target]++;
        }
    }

    // Add num_removals nodes back (restore the nodes)
    for (std::size_t i = 0; i < num_removals; ++i) {
       auto added_node = engine.addBucket();     
       nodes[added_node] = 1;   // enable node
    }

    for (const auto& current_random_number : random_keys) {
        const auto a = current_random_number.first;
        const auto b = current_random_number.second;
        // ideally we should get the same target as before before.
        const auto new_target = engine.getBucketCRC32c(a, b);
        const auto old_target = bucket[current_random_number];

        // readded node and the key doesn't change
        if (new_target == old_target && nodes[old_target] == 1) {
            // KeysMovedToRestoreNodes
            //keys_moved_to_restored_nodes++;
            nodes_gaining_keys++;
            moved_to_restored_nodes[old_target]++;
        }
    
        // readded node and the key change
        else if (new_target != old_target && nodes[old_target] == 1) {
            // KeysMovedToOtherNodes
            //keys_moved_to_other_nodes++;
            nodes_gaining_keys++;
            keys_relocated_after_resize++;
            nodes_changed_after_resize++;
            moved_to_other_nodes[old_target]++;
        }
    }

    ////////////////////////////////////////////////////////////////////
    
    for (auto i : keys_per_node) {
        monotonicity.keys_in_removed_nodes += i;
    }

    for (auto i : moved_from_removed_nodes) {
        monotonicity.keys_moved_from_removed_nodes += i;
    }

    for (auto i : moved_from_other_nodes) {
        monotonicity.keys_moved_from_other_nodes += i;
    }

    for (auto i : moved_to_restored_nodes) {
        monotonicity.keys_moved_to_restored_nodes += i;
    }

    for (auto i : moved_to_other_nodes) {
        monotonicity.keys_moved_to_other_nodes += i;
    }


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