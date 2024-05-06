/*
 * Copyright (c) 2023 Amos Brocco.
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

#include "utils.h"
#include <sstream>


uint32_t crc32c_sse42_u64(uint64_t key, uint64_t seed) {
#ifdef __x86_64
    __asm__ volatile("crc32q %[key], %[seed];"
                     : [seed] "+r"(seed)
                     : [key] "rm"(key));
#elif __aarch64__
    __asm__ volatile("crc32x %[key], %[seed];"
                     : [seed] "+r"(seed)
                     : [key] "rm"(key));
#endif
    return seed;
}

std::size_t parse_key_multiplier(const std::string& key_multiplier_str) {
    std::istringstream iss(key_multiplier_str);
    std::size_t keyMultiplier;
    if (!(iss >> keyMultiplier)) {
        return 100;
    }
    return keyMultiplier;
}

std::vector<double> parse_fractions(const std::string& fractions_str) {
    std::vector<double> fractions;
    std::istringstream iss(fractions_str);

    iss.ignore(1);
    double fraction;
    while (iss >> fraction) {
        fractions.push_back(fraction);
        iss.ignore();
    }
    return fractions;
}


std::vector<std::pair<uint32_t, uint32_t>>
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
        ret.push_back(std::pair<uint32_t, uint32_t>{a, b});
    }
    return ret;
}