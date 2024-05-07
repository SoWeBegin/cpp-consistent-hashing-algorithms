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

#ifndef SPEED_TEST_BENCH_H
#define SPEED_TEST_BENCH_H

#include <algorithm>
#include <chrono>
#include "anchor/anchorengine.h"
#include "memento/mashtable.h"
#include "memento/mementoengine.h"
#include "jump/jumpengine.h"
#include "power/powerengine.h"
#include "dx/dxEngine.h"
#include "YamlParser/YamlParser.h"
#include "csvWriter.h"
#ifdef USE_PCG32
#include "pcg_random.hpp"
#include <random>
#endif
#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered_map.hpp>
#include <cxxopts.hpp>
#include <fmt/core.h>
#include <fstream>
#include <unordered_map>
#include <gtl/phmap.hpp>
#include "utils.h"


 /*
  * ******************************************
  * Heap allocation measurement
  * ******************************************
  */

#ifdef USE_HEAPSTATS
static unsigned long allocations{ 0 };
static unsigned long deallocations{ 0 };
static unsigned long allocated{ 0 };
static unsigned long deallocated{ 0 };
static unsigned long maximum{ 0 };

inline void* operator new(size_t size) {
    void* p = malloc(size);
    allocations += 1;
    allocated += size;
    maximum = allocated > maximum ? allocated : maximum;
    return p;
}

inline void* operator new[](size_t size) {
    void* p = malloc(size);
    allocations += 1;
    allocated += size;
    maximum = allocated > maximum ? allocated : maximum;
    return p;
}

inline void operator delete(void* ptr, std::size_t size) noexcept {
    deallocations += 1;
    deallocated += size;
    free(ptr);
}

inline void operator delete[](void* ptr, std::size_t size) noexcept {
    deallocations += 1;
    deallocated += size;
    free(ptr);
}

inline void reset_memory_stats() noexcept {
    allocations = 0;
    allocated = 0;
    deallocations = 0;
    deallocated = 0;
    maximum = 0;
}

inline void print_memory_stats(std::string_view label) noexcept {
    auto alloc{ allocations };
    auto dealloc{ deallocations };
    auto asize{ allocated };
    auto dsize{ deallocated };
    auto max{ maximum };
    fmt::println("   @{}: Allocations: {}, Allocated: {}, Deallocations: {}, "
        "Deallocated: {}, Maximum: {}",
        label, alloc, asize, dealloc, dsize, max);
    allocations = alloc;
    deallocations = dealloc;
    allocated = asize;
    deallocated = dsize;
    maximum = max;
}
#endif

/*
* ******************************************
* Benchmark routine
* ******************************************
*/
template <typename Algorithm, typename T>
inline void bench(const std::string& name,
    std::size_t anchor_set /* capacity */, std::size_t working_set,
    uint32_t num_removals, uint32_t num_keys, uint32_t total_iterations,
    LookupTime& lookup_time, random_distribution_ptr<T> random_fnt) {

    uint32_t* nodes = new uint32_t[anchor_set]();
    for (uint32_t i = 0; i < working_set; ++i) {
        nodes[i] = 1;
    }

    Algorithm engine(anchor_set, working_set);

    // See Monotonicity.h for an explanation of this.
    for (std::size_t i = 0; i < num_removals;) {
        const uint32_t removed = (*random_fnt)() % working_set;
        if (nodes[removed] == 1) {
            const auto removed_node = engine.removeBucket(removed);
            if (!nodes[removed_node]) {
                delete[] nodes;
                throw "Crazy bug";
            }
            nodes[removed_node] = 0; 
            ++i;
        }
    }

    std::vector<double> results(total_iterations);
    volatile int64_t bucket{ 0 };

    for (uint32_t i = 0; i < total_iterations; ++i) { 
        const auto start = std::chrono::high_resolution_clock::now();
        bucket = engine.getBucketCRC32c((*random_fnt)(), (*random_fnt)());
        const auto end = std::chrono::high_resolution_clock::now();
        const auto elapsed_nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        results.push_back(elapsed_nanoseconds);
    }

    const double total_elapsed_nanoseconds = std::accumulate(results.begin(), results.end(), 0.0);
    lookup_time.score = total_elapsed_nanoseconds / results.size();

    delete[] nodes;
}

inline double parse_removal_rate(const std::string& removal_rate_str) {
    std::istringstream iss(removal_rate_str);
    double removal_rate;
    if (!(iss >> removal_rate)) {
        return 0.;
    }
    return removal_rate;
}
        
template<typename T>
inline void speed_test(const std::string& output_path, const BenchmarkSettings& current_benchmark,
    const std::vector<AlgorithmSettings>& algorithms, const CommonSettings& common_settings, 
    const std::unordered_map<std::string, random_distribution_ptr<T>>& distribution_function) {

    // Further parse "removal-rate", aka initial nodes to remove.
    double removal_rate{}; // default value = 0, nothing to remove
    if (current_benchmark.args.count("removal-rate")) {
        removal_rate = parse_removal_rate(current_benchmark.args.at("removal-rate"));
    }
    // Sanity check
    if (removal_rate < 0 || removal_rate >= 1) {
        fmt::println("Removal rate must be in the range [0, 1[. Continuing with default value removal-rate = 0.");
        removal_rate = 0;
    }

    // Further parse "removal-order". Default value is "lifo".
    std::string removal_order = "lifo";
    if (current_benchmark.args.count("removal-order")) {
        removal_order = current_benchmark.args.at("removal-order");
    }
    // Sanity check
    if (removal_order != "lifo" && removal_order != "fifo" && removal_order != "random") {
        fmt::println("Removal order must be one of [lifo, fifo, random]. Continuing with default value removal-order = lifo.");
        removal_order = "lifo";
    }
    auto& lookuptime_writer = CsvWriter<LookupTime>::getInstance("./", "lookup_time.csv");
    for (const auto& hash_function : current_benchmark.commonSettings.hashFunctions) { // Done for all benchmarks
        for (const auto& current_algorithm : algorithms) {
            for (const auto& key_distribution : current_benchmark.commonSettings.keyDistributions) { // Done for all benchmarks
                for (const auto& working_set : current_benchmark.commonSettings.numInitialActiveNodes) {
            
                    LookupTime lookup_time;
                    lookup_time.param_distribution = key_distribution;
                    lookup_time.param_function = hash_function;
                    lookup_time.param_init_nodes = working_set;
                    lookup_time.mode = common_settings.mode;
                    lookup_time.unit = common_settings.unit;
                    lookup_time.param_benchmark = "lookuptime";
                    lookup_time.benchmark = "speed_test=>bench";
                    lookup_time.param_algorithm = current_algorithm.name;

                    random_distribution_ptr<T> ptr = distribution_function.at(key_distribution);

                    uint32_t total_iterations = common_settings.totalBenchmarkIterations;

                    const uint32_t num_removals = static_cast<uint32_t>(removal_rate * working_set);

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
                        return;
                    }
                    else if (current_algorithm.name == "baseline") {
#ifdef USE_PCG32
                        pcg_extras::seed_seq_from<std::random_device> seed;
                        pcg32 rng(seed);
#else
                        srand(time(NULL));
#endif
                        fmt::println("Allocating {} buckets of size {} bytes...", capacity,
                            sizeof(uint32_t));
                        uint32_t* bucket_status = new uint32_t[capacity]();
                        for (uint32_t i = 0; i < working_set; i++) {
                            bucket_status[i] = 1;
                        }
                        uint32_t i = 0;
                        while (i < num_removals) {
#ifdef USE_PCG32
                            uint32_t removed = rng() % working_set;
#else
                            uint32_t removed = rand() % working_set;
#endif
                            if (bucket_status[removed] == 1) {
                                bucket_status[removed] = 0;
                                i++;
                            }
                        }
                        delete[] bucket_status;
                    }
                    else if (current_algorithm.name == "anchor") {
                        bench<AnchorEngine>("Anchor", capacity, working_set,
                            num_removals, working_set, total_iterations, lookup_time, ptr);
                    }
                    else if (current_algorithm.name == "memento") {
                        bench<MementoEngine<boost::unordered_flat_map>>(
                            "Memento<boost::unordered_flat_map>", capacity, working_set,
                            num_removals, working_set, total_iterations, lookup_time, ptr);
                    }
                    else if (current_algorithm.name == "mementoboost") {
                        bench<MementoEngine<boost::unordered_map>>(
                            "Memento<boost::unordered_map>", capacity, working_set,
                            num_removals, working_set, total_iterations, lookup_time, ptr);
                    }
                    else if (current_algorithm.name == "mementostd") {
                        bench<MementoEngine<std::unordered_map>>(
                            "Memento<std::unordered_map>", capacity, working_set,
                            num_removals, working_set, total_iterations, lookup_time, ptr);
                    }
                    else if (current_algorithm.name == "mementogtl") {
                        bench<MementoEngine<gtl::flat_hash_map>>(
                            "Memento<std::gtl::flat_hash_map>", capacity, working_set,
                            num_removals, working_set, total_iterations, lookup_time, ptr);
                    }
                    else if (current_algorithm.name == "mementomash") {
                        bench<MementoEngine<MashTable>>("Memento<MashTable>",
                            capacity, working_set,
                            num_removals, working_set, total_iterations, lookup_time, ptr);
                    }
                    else if (current_algorithm.name == "jump") {
                        bench<JumpEngine>("JumpEngine",
                            capacity, working_set,
                            num_removals, working_set, total_iterations, lookup_time, ptr);
                    }
                    else if (current_algorithm.name == "power") {
                        bench<PowerEngine>("PowerEngine",
                            capacity, working_set,
                            num_removals, working_set, total_iterations, lookup_time, ptr);
                    }
                    else if (current_algorithm.name == "dx") {
                        bench<DxEngine>("DxEngine", capacity, working_set,
                            num_removals, working_set, total_iterations, lookup_time, ptr);
                    }
                    else {
                        fmt::println("Unknown algorithm {}", current_algorithm.name);
                    }

                    lookuptime_writer.add(lookup_time);

                }
            }
        }
    }
}
         
#endif