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

double convert_ns_to(double ns_time, const std::string& new_unit) {
    if (new_unit == "SECONDS")
        return ns_time * 1e-9;
    else if (new_unit == "MILLISECONDS")
        return ns_time * 1e-6;
    else if (new_unit == "MICROSECONDS")
        return ns_time * 1e-3;
    return ns_time; // default unit
}

