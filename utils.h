#ifndef UTILS_H
#define UTILS_H
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

#include <stdint.h>
#include <cstddef>
#include <vector>
#include <string>

uint32_t crc32c_sse42_u64(uint64_t key, uint64_t seed);

std::size_t parse_key_multiplier(const std::string& key_multiplier_str);

std::vector<double> parse_fractions(const std::string& fractions_str);

#endif // UTILS_H
