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

// KEEP
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
template <typename Algorithm>
inline void bench(const std::string& name, const std::string file_name,
    std::size_t anchor_set /* capacity */, std::size_t working_set,
    uint32_t num_removals, uint32_t num_keys, LookupTime& lookup_time) {

#ifdef USE_PCG32
    pcg_extras::seed_seq_from<std::random_device> seed;
    pcg32 rng{ seed };
#else
    srand(time(NULL));
#endif

    std::ofstream results_file;
    results_file.open("anchor-lookuptime.txt", std::ofstream::out | std::ofstream::app);
    
    double norm_keys_rate = (double)num_keys / 1000000.0;

    uint32_t* bucket_status = new uint32_t[anchor_set]();

    for (uint32_t i = 0; i < working_set; i++) {
        bucket_status[i] = 1;
    }

#ifdef USE_HEAPSTATS
    reset_memory_stats();
    print_memory_stats("StartBenchmark");
#endif

    Algorithm engine(anchor_set, working_set);

#ifdef USE_HEAPSTATS
    print_memory_stats("AfterAlgorithmInit");
#endif

    uint32_t i = 0;
    while (i < num_removals) {
#ifdef USE_PCG32
        uint32_t removed = rng() % working_set;
#else
        uint32_t removed = rand() % working_set;
#endif
        if (bucket_status[removed] == 1) {
            engine.removeBucket(removed);
            bucket_status[removed] = 0;
            i++;
        }
    }

#ifdef USE_HEAPSTATS
    print_memory_stats("AfterRemovals");
#endif

    volatile int64_t bucket{ 0 };
    auto start{ clock() };
    for (uint32_t i = 0; i < num_keys; ++i) {
#ifdef USE_PCG32
        bucket = engine.getBucketCRC32c(rng(), rng());
#else
        bucket = engine.getBucketCRC32c(rand(), rand());
#endif
    }
    auto end{ clock() };

#ifdef USE_HEAPSTATS
    print_memory_stats("EndBenchmark");
#endif

    auto elapsed{ static_cast<double>(end - start) / CLOCKS_PER_SEC };
#ifdef USE_HEAPSTATS
    auto maxheap{ maximum };
    fmt::println("{} Elapsed time is {} seconds, maximum heap allocated memory is {} bytes, sizeof({}) is {}", name, elapsed, maxheap, name, sizeof(Algorithm));
    results_file << name << ":\tAnchor\t" << anchor_set << "\tWorking\t"
        << working_set << "\tRemovals\t" << num_removals << "\tRate\t"
        << norm_keys_rate / elapsed << "\tMaxHeap\t" << maxheap << "\tAlgoSizeof\t" << sizeof(Algorithm) << "\n";
#else
    fmt::println("{} Elapsed time is {} seconds", name, elapsed);
    results_file << name << ":\tAnchor\t" << anchor_set << "\tWorking\t"
        << working_set << "\tRemovals\t" << num_removals << "\tRate\t"
        << norm_keys_rate / elapsed << "\n";
#endif



    results_file.close();

    delete[] bucket_status;
}

double parse_removal_rate(const std::string& removal_rate_str) {
    std::istringstream iss(removal_rate_str);
    double removal_rate;
    if (!(iss >> removal_rate)) {
        return 0.;
    }
    return removal_rate;
}
        
inline int speed_test(const std::string& output_path, const AlgorithmSettings& current_algorithm,
    std::size_t working_set, const std::string& hash_function, const std::string& key_distribution,
    const std::unordered_map<std::string, std::string>& args, const CommonSettings& common_settings) {

    // Further parse "removal-rate", aka initial nodes to remove.
    double removal_rate{}; // default value
    if (args.count("removal-rate")) {
        removal_rate = parse_removal_rate(args.at("removal-rate"));
    }
    // Sanity check
    if (removal_rate < 0 || removal_rate >= 1) {
        fmt::println("Removal rate must be in the range [0, 1[. Continuing with default value removal-rate = 0.");
        removal_rate = 0;
    }

    // Further parse "removal-order". Default value is "lifo".
    std::string removal_order = "lifo";
    if (args.count("removal-order")) {
        removal_order = args.at("removal-order");
    }
    // Sanity check
    if (removal_order != "lifo" && removal_order != "fifo" && removal_order != "random") {
        fmt::println("Removal order must be one of [lifo, fifo, random]. Continuing with default value removal-order = lifo.");
        removal_order = "lifo";
    }

    LookupTime lookup_time;
    lookup_time.param_distribution = key_distribution;
    lookup_time.param_function = hash_function;
    lookup_time.param_init_nodes = working_set;
    lookup_time.mode = common_settings.mode;
    lookup_time.unit = common_settings.unit;
    lookup_time.param_benchmark = "lookuptime";
    lookup_time.benchmark = "speed_test=>bench";

    lookup_time.param_algorithm = current_algorithm.name;

    const std::string full_file_path = output_path + "/" + current_algorithm.name + ".txt";
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
        return 0;
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
        bench<AnchorEngine>("Anchor", full_file_path, capacity, working_set,
            num_removals, working_set, lookup_time);
    }
    else if (current_algorithm.name == "memento") {
        bench<MementoEngine<boost::unordered_flat_map>>(
            "Memento<boost::unordered_flat_map>", full_file_path, capacity, working_set,
            num_removals, working_set, lookup_time);
    }
    else if (current_algorithm.name == "mementoboost") {
        bench<MementoEngine<boost::unordered_map>>(
            "Memento<boost::unordered_map>", full_file_path, capacity, working_set,
            num_removals, working_set, lookup_time);
    }
    else if (current_algorithm.name == "mementostd") {
        bench<MementoEngine<std::unordered_map>>(
            "Memento<std::unordered_map>", full_file_path, capacity, working_set,
            num_removals, working_set, lookup_time);
    }
    else if (current_algorithm.name == "mementogtl") {
        bench<MementoEngine<gtl::flat_hash_map>>(
            "Memento<std::gtl::flat_hash_map>", full_file_path, capacity, working_set,
            num_removals, working_set, lookup_time);
    }
    else if (current_algorithm.name == "mementomash") {
        bench<MementoEngine<MashTable>>("Memento<MashTable>", full_file_path,
            capacity, working_set,
            num_removals, working_set, lookup_time);
    }
    else if (current_algorithm.name == "jump") {
        bench<JumpEngine>("JumpEngine", full_file_path,
            capacity, working_set,
            num_removals, working_set, lookup_time);
    }
    else if (current_algorithm.name == "power") {
        bench<PowerEngine>("PowerEngine", full_file_path,
            capacity, working_set,
            num_removals, working_set, lookup_time);
    }
    else if (current_algorithm.name == "dx") {
        bench<DxEngine>("DxEngine", full_file_path, capacity, working_set,
            num_removals, working_set, lookup_time);
    }
    else {
        fmt::println("Unknown algorithm {}", current_algorithm.name);
    }
    
    auto& lookuptime_writer = CsvWriter<LookupTime>::getInstance("./", "lookup_time.csv");
    lookuptime_writer.add(lookup_time);
    return 0;
}
         

#endif