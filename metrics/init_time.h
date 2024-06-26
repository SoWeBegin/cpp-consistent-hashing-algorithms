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

#ifndef INIT_TIME_BENCH_H
#define INIT_TIME_BENCH_H

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
#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered_map.hpp>
#include <cxxopts.hpp>
#include <fmt/core.h>
#include <unordered_map>
#include <gtl/phmap.hpp>
#include "../utils.h"
#include <string_view>

 /*
 * ******************************************
 * Benchmark routine
 * ******************************************
 */
template <typename Algorithm>
inline void bench(const std::string& name,
    std::size_t anchor_set /* capacity */, std::size_t working_set,
    uint32_t total_iterations, uint32_t total_seconds, InitTime& init_time,
    const std::string& time_unit) {

    std::vector<double> results;

    fmt::println("[InitTime] Starting benchmark, num iterations: {}", total_iterations);
    
    const auto start_time = std::chrono::steady_clock::now();
    auto current_time = start_time;
    for (std::size_t i = 0; i < total_iterations
        && std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count() < total_seconds; ++i) {

        const auto start_bench = std::chrono::steady_clock::now();
        Algorithm engine(anchor_set, working_set);
        const auto end_bench = std::chrono::steady_clock::now();

        // prevent optimization
        const auto result = engine.getBucketCRC32c(rand(), rand());

        double elapsed_time_value = convert_elapsed_time_to(end_bench, start_bench, time_unit);
        results.push_back(elapsed_time_value);

        current_time = std::chrono::steady_clock::now();
    }

    // For an explanation, see lookup_time.h.
    double total_elapsed_time = 0.;
    for (double result : results) {
        total_elapsed_time += result;
    }
    init_time.score = total_elapsed_time / results.size();
    double sum_squared_diff = 0.0;
    for (double result : results) {
        const double diff = result - init_time.score;
        sum_squared_diff += diff * diff;
    }
    if (results.size() > 1) {
        const double variance = sum_squared_diff / (results.size() - 1);
        init_time.score_error = sqrt(variance) / sqrt(results.size());
    }
    else {
        init_time.score_error = std::numeric_limits<double>::quiet_NaN();
    }
}

inline void init_time(CsvWriter<InitTime>& init_time_writer,
    const std::string& output_path, const BenchmarkSettings& current_benchmark,
    const std::vector<AlgorithmSettings>& algorithms, const CommonSettings& common_settings) {

    const uint32_t total_iterations = common_settings.totalBenchmarkIterations;
    const uint32_t total_seconds = common_settings.secondsForEachIteration;
    const std::string time_unit = common_settings.unit;

    for (const auto& hash_function : current_benchmark.commonSettings.hashFunctions) {
        for (const auto& current_algorithm : algorithms) {
            for (const auto& working_set : current_benchmark.commonSettings.numInitialActiveNodes) {

                InitTime init_time("init_time => bench", common_settings.mode, 1, total_iterations,
                    common_settings.unit, current_algorithm.name, hash_function, working_set);

                uint32_t capacity = working_set * 10; // default = 10
                if (current_algorithm.args.count("capacity")) {
                    capacity = str_to<uint32_t>(current_algorithm.args.at("capacity"), 10) * working_set;
                }

                if (current_algorithm.name == "anchor") {
                    bench<AnchorEngine>("Anchor", capacity, working_set,
                        total_iterations, total_seconds, init_time, time_unit);
                }
                else if (current_algorithm.name == "memento") {
                    bench<MementoEngine<boost::unordered_flat_map>>(
                        "Memento<boost::unordered_flat_map>", capacity, working_set,
                        total_iterations, total_seconds, init_time, time_unit);
                }
                else if (current_algorithm.name == "mementoboost") {
                    bench<MementoEngine<boost::unordered_map>>(
                        "Memento<boost::unordered_map>", capacity, working_set,
                        total_iterations, total_seconds, init_time, time_unit);
                }
                else if (current_algorithm.name == "mementostd") {
                    bench<MementoEngine<std::unordered_map>>(
                        "Memento<std::unordered_map>", capacity, working_set,
                        total_iterations, total_seconds, init_time, time_unit);
                }
                else if (current_algorithm.name == "mementogtl") {
                    bench<MementoEngine<gtl::flat_hash_map>>(
                        "Memento<std::gtl::flat_hash_map>", capacity, working_set,
                        total_iterations, total_seconds, init_time, time_unit);
                }
                else if (current_algorithm.name == "mementomash") {
                    bench<MementoEngine<MashTable>>("Memento<MashTable>",
                        capacity, working_set,
                        total_iterations, total_seconds, init_time, time_unit);
                }
                else if (current_algorithm.name == "jump") {
                    bench<JumpEngine>("JumpEngine", capacity, working_set,
                        total_iterations, total_seconds, init_time, time_unit);
                }
                else if (current_algorithm.name == "power") {
                    bench<PowerEngine>("PowerEngine", capacity, working_set,
                        total_iterations, total_seconds, init_time, time_unit);
                }
                else if (current_algorithm.name == "dx") {
                    bench<DxEngine>("DxEngine", capacity, working_set,
                        total_iterations, total_seconds, init_time, time_unit);
                }
                else {
                    fmt::println("[ResizeTime] Unknown algorithm {}", current_algorithm.name);
                }

                init_time_writer.add(init_time);
            }
        }
    }
}

#endif