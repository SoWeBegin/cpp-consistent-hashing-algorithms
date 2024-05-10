#ifndef UTILS_H
#define UTILS_H
/*
 * Copyright (c) 2023 Amos Brocco, Tony Kolarek, Tatiana Dal Busco
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

#include <stdint.h>
#include <cstddef>
#include <vector>
#include <string>
#include <utility>
#include <random>
#include <type_traits> 
#include <limits> 
#include <charconv> // std::from_chars
#include <chrono>
#include <stdexcept>

#include <iostream>
template<typename T>
using random_distribution_ptr = T(*)();

uint32_t crc32c_sse42_u64(uint64_t key, uint64_t seed);

std::vector<double> parse_fractions(const std::string& fractions_str);

double convert_ns_to(double ns_time, const std::string& unit);


template<typename Ret>
Ret str_to(const std::string& to_parse, Ret default_value_if_fail) {
    Ret ret;
    auto [ptr, ec] = std::from_chars(to_parse.data(), to_parse.data() + to_parse.size(), ret);
    if (ec != std::errc()) {
        return default_value_if_fail;
    }
    return ret;
}


/*
 * this function generates a sequence of random keys, with key = {random(a), random(b)}
 * num_keys: total keys to generate randomly
 * return: a vector of keys
 */
template<typename T>
std::vector<std::pair<T, T>>
generate_random_keys_sequence(std::size_t num_keys, random_distribution_ptr<T> rand) {

    std::vector<std::pair<T, T>> ret;
    for (std::size_t i = 0; i < num_keys; ++i) {
        const auto a{ (*rand)() };
        const auto b{ (*rand)() };
        ret.push_back(std::pair<T, T>{a, b});
    }
    return ret;
}

template<typename T>
typename std::enable_if<std::is_integral<T>::value, T>::type
random_uniform_distribution() {
    static std::random_device rand_dev;
    static std::mt19937 generator(rand_dev());
    std::uniform_int_distribution<T> distr(0, std::numeric_limits<T>::max());
    return distr(generator);
}

double convert_elapsed_time_to(auto end_time, auto start_time, const std::string& time_unit) {
    double elapsed_time_value{};
    if (time_unit == "NANOSECONDS") {
        std::chrono::duration<double, std::nano> fp_ns = end_time - start_time;
        return fp_ns.count();
    }
    else if (time_unit == "MICROSECONDS") {
        std::chrono::duration<double, std::micro> fp_mcs = end_time - start_time;
        return fp_mcs.count();
    }
    else if (time_unit == "MILLISECONDS") {
        std::chrono::duration<double, std::milli> fp_ms = end_time - start_time;
        return fp_ms.count();
    }
    else if (time_unit == "SECONDS") {
        std::chrono::duration<double, std::nano> fp_ns = end_time - start_time;
        return fp_ns.count() * 1e-9;
    }
    else {
        throw std::invalid_argument("Invalid time unit specified");
    }
    return elapsed_time_value;
}



#endif // UTILS_H
