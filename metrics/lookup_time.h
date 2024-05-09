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
#include "../anchor/anchorengine.h"
#include "../memento/mashtable.h"
#include "../memento/mementoengine.h"
#include "../jump/jumpengine.h"
#include "../power/powerengine.h"
#include "../dx/dxEngine.h"
#include "../YamlParser/YamlParser.h"
#include "../CsvWriter/csvWriter.h"
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
#include "../utils.h"
#include <string_view>
#include <limits>


 /*
  * ******************************************
  * Heap allocation measurement
  * ******************************************
  */

MemoryUsage memory_usage;
auto& memory_usage_writer = CsvWriter<MemoryUsage>::getInstance();
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
    memory_usage.allocations = allocations;
    memory_usage.deallocations = deallocations;
    memory_usage.allocated = allocated;
    memory_usage.deallocated = deallocated;
    memory_usage.maximum = maximum;
    memory_usage.type = label;
    memory_usage_writer.add(memory_usage);
    fmt::println("   @{}: Allocations: {}, Allocated: {}, Deallocations: {}, "
        "Deallocated: {}, Maximum: {}",
        label, memory_usage.allocations, memory_usage.allocated,
        memory_usage.deallocations, memory_usage.deallocated, memory_usage.maximum);
    allocations = memory_usage.allocations;
    deallocations = memory_usage.deallocations;
    allocated = memory_usage.allocated;
    deallocated = memory_usage.deallocated;
    maximum = memory_usage.maximum;
}

/*
* ******************************************
* Benchmark routine
* ******************************************
*/
template <typename Algorithm, typename T>
inline void bench(const std::string& name,
    std::size_t anchor_set /* capacity */, std::size_t working_set,
    uint32_t num_removals, uint32_t total_iterations, uint32_t total_seconds, 
    LookupTime& lookup_time, random_distribution_ptr<T> random_fnt,
    const std::string& removal_order, const std::string& time_unit) {

    uint32_t* nodes = new uint32_t[anchor_set]();
    for (uint32_t i = 0; i < working_set; ++i) {
        nodes[i] = 1;
    }

    reset_memory_stats();
    print_memory_stats("StartBenchmark");
    Algorithm engine(anchor_set, working_set);
    print_memory_stats("AfterAlgorithmInit");

    if (num_removals) {
        fmt::println("[LookupTime] Starting to remove {} nodes, with removal order: {}", 
            num_removals, removal_order);
    }

    // See Monotonicity.h for an explanation of this.
    for (std::size_t i = 0; i < num_removals;) {
        uint32_t removed{};
        if (removal_order == "random") removed = (*random_fnt)() % working_set;
        else if (removal_order == "lifo") removed = working_set - 1 - i;
        else /* assume fifo */ removed = i;

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
    print_memory_stats("AfterRemovals");

    // Lazy initialization of the specified random function
    // First call is slower because the random number generator must be initialized, we want to avoid that
    volatile uint32_t lazy_init = (*random_fnt)();

    std::vector<double> results;
    volatile uint32_t bucket = 0;

    // We keep track of both:
    //  - how many seconds the bench should last at max (time.execution)
    //  - how many iterations the benchmark should be repeated (time.execution)
    // The first condition to be satisfied ends the benchmark.
    const auto start_time = std::chrono::high_resolution_clock::now();
    auto current_time = start_time;
    for (std::size_t i = 0; i < total_iterations
        && std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count() < total_seconds; ++i) {
        const auto start_bench = std::chrono::high_resolution_clock::now();
        bucket = engine.getBucketCRC32c((*random_fnt)(), (*random_fnt)());
        const auto end_bench = std::chrono::high_resolution_clock::now();

        const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end_bench - start_bench).count();
        results.push_back(elapsed);

        current_time = std::chrono::high_resolution_clock::now();
    }
    print_memory_stats("EndBenchmark");

    // We need to find the total elapsed time to find the average elapsed time
    // which is lookup_time.score.
    double total_elapsed_time = 0.0;
    for (double result : results) {
        total_elapsed_time += convert_ns_to(result, time_unit);
    }
    lookup_time.score = total_elapsed_time / results.size();

    // We then need to calculate the standard deviation
    // to find the error score, which is lookuptime_score.error.
    double sum_squared_diff = 0.0;
    for (double result : results) {
        const double adjusted_result = convert_ns_to(result, time_unit);
        const double diff = adjusted_result - lookup_time.score;
        sum_squared_diff += diff * diff;
    }

    // We cannot assume that results.size() == 1, otherwise this won't work even though it should,
    // since having one single iteration is a valid setting.
    if (results.size() != 1) {
        const double variance = sum_squared_diff / (results.size() - 1);
        lookup_time.score_error = sqrt(variance) / sqrt(results.size());
    }
    else {
        lookup_time.score_error = std::numeric_limits<double>::quiet_NaN();
    }

    delete[] nodes;
}
        
template<typename T>
inline void speed_test(CsvWriter<LookupTime>& lookuptime_writer,
    const std::string& output_path, const BenchmarkSettings& current_benchmark,
    const std::vector<AlgorithmSettings>& algorithms, const CommonSettings& common_settings, 
    const std::unordered_map<std::string, random_distribution_ptr<T>>& distribution_function) {

    // Further parse "removal-rate", aka initial nodes to remove.
    double removal_rate{}; // default value = 0, nothing to remove
    if (current_benchmark.args.count("removal-rate")) {
        removal_rate = str_to<double>(current_benchmark.args.at("removal-rate"), 0); 
    }
    // Sanity check
    if (removal_rate < 0 || removal_rate >= 1) {
        fmt::println("[LookupTime] Removal rate must be in the range [0, 1[. Continuing with default value removal-rate = 0.");
        removal_rate = 0;
    }

    // Further parse "removal-order". Default value is "lifo".
    std::string removal_order = "lifo";
    if (current_benchmark.args.count("removal-order")) {
        removal_order = current_benchmark.args.at("removal-order");
    }
    // Sanity check
    if (removal_order != "lifo" && removal_order != "fifo" && removal_order != "random") {
        fmt::println("[LookupTime] Removal order must be one of [lifo, fifo, random]. Continuing with default value removal-order = lifo.");
        removal_order = "lifo";
    }

    const uint32_t total_iterations = common_settings.totalBenchmarkIterations; 
    const uint32_t total_seconds = common_settings.secondsForEachIteration;
    const std::string time_unit = common_settings.unit;
    memory_usage.iterations = common_settings.totalBenchmarkIterations;

    for (const auto& hash_function : current_benchmark.commonSettings.hashFunctions) {
        for (const auto& current_algorithm : algorithms) {
            for (const auto& key_distribution : current_benchmark.commonSettings.keyDistributions) { 
                for (const auto& working_set : current_benchmark.commonSettings.numInitialActiveNodes) {
            
                    LookupTime lookup_time("speed_test => bench", common_settings.mode, 1, total_iterations,
                        common_settings.unit, current_algorithm.name, "lookuptime",
                        key_distribution, hash_function, working_set);

                    memory_usage.algorithm = current_algorithm.name;
                    memory_usage.nodes = working_set;
                    memory_usage.hash_function = hash_function;
                    
                    random_distribution_ptr<T> random_gen_fnt_ptr;
                    if (distribution_function.count(key_distribution)) {
                        random_gen_fnt_ptr = distribution_function.at(key_distribution);
                    }
                    else {
                        fmt::println("[LookupTime] The specified distribution is not available. Proceeding with default UNIFORM");
                        random_gen_fnt_ptr = distribution_function.at("uniform");
                    }

                    uint32_t capacity = working_set * 10; // default = 10
                    if (current_algorithm.args.count("capacity")) {
                        capacity = str_to<uint32_t>(current_algorithm.args.at("capacity"), 10) * working_set;
                    }

                    const uint32_t num_removals = static_cast<uint32_t>(removal_rate * working_set);

                    if (current_algorithm.name == "anchor") {
                        bench<AnchorEngine>("Anchor", capacity, working_set,
                            num_removals, total_iterations, total_seconds, 
                            lookup_time, random_gen_fnt_ptr, removal_order, time_unit);
                    }
                    else if (current_algorithm.name == "memento") {
                        bench<MementoEngine<boost::unordered_flat_map>>(
                            "Memento<boost::unordered_flat_map>", capacity, working_set,
                            num_removals, total_iterations, total_seconds,
                            lookup_time, random_gen_fnt_ptr, removal_order, time_unit);
                    }
                    else if (current_algorithm.name == "mementoboost") {
                        bench<MementoEngine<boost::unordered_map>>(
                            "Memento<boost::unordered_map>", capacity, working_set,
                            num_removals, total_iterations, total_seconds,
                            lookup_time, random_gen_fnt_ptr, removal_order, time_unit);
                    }
                    else if (current_algorithm.name == "mementostd") {
                        bench<MementoEngine<std::unordered_map>>(
                            "Memento<std::unordered_map>", capacity, working_set,
                            num_removals, total_iterations, total_seconds,
                            lookup_time, random_gen_fnt_ptr, removal_order, time_unit);
                    }
                    else if (current_algorithm.name == "mementogtl") {
                        bench<MementoEngine<gtl::flat_hash_map>>(
                            "Memento<std::gtl::flat_hash_map>", capacity, working_set,
                            num_removals, total_iterations, 
                            total_seconds, lookup_time,
                            random_gen_fnt_ptr, removal_order, time_unit);
                    }
                    else if (current_algorithm.name == "mementomash") {
                        bench<MementoEngine<MashTable>>("Memento<MashTable>",
                            capacity, working_set,
                            num_removals, total_iterations, 
                            total_seconds, lookup_time,
                            random_gen_fnt_ptr, removal_order, time_unit);
                    }
                    else if (current_algorithm.name == "jump") {
                        bench<JumpEngine>("JumpEngine",
                            capacity, working_set,
                            num_removals, total_iterations,
                            total_seconds, lookup_time,
                            random_gen_fnt_ptr, removal_order, time_unit);
                    }
                    else if (current_algorithm.name == "power") {
                        bench<PowerEngine>("PowerEngine",
                            capacity, working_set,
                            num_removals, total_iterations, 
                            total_seconds, lookup_time,
                            random_gen_fnt_ptr, removal_order, time_unit);
                    }
                    else if (current_algorithm.name == "dx") {
                        bench<DxEngine>("DxEngine", capacity, working_set,
                            num_removals, total_iterations, 
                            total_seconds, lookup_time,
                            random_gen_fnt_ptr, removal_order, time_unit);
                    }
                    else {
                        fmt::println("[LookupTime] Unknown algorithm {}", current_algorithm.name);
                    }

                    lookuptime_writer.add(lookup_time);
                }
            }
        }
    }
}
         
#endif