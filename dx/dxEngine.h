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
#ifndef DXENGINE_H
#define DXENGINE_H
#include <cstdint>
#include <boost/dynamic_bitset.hpp>
#include "../utils.h"
#include <deque>
#include <random>

class DxEngine final {
public:
    DxEngine(uint32_t , uint32_t size)
        : m_size(size), m_capacity(size * 1000) {
        m_failed.set(capacity);
    }

    uint32_t getBucket(const uint64_t key, const uint64_t seed) o {
        uint64_t hashValue = crc32c_sse42_u64(key, seed);
        std::mt19937 generator(hashValue);
        std::uniform_int_distribution<uint32_t> distribution(0, capacity - 1);

        uint32_t b = distribution(generator);
        while (failed.test(b)) {
            b = distribution(generator); // Trova un bucket non fallito
        }
        return b;
    }

    uint32_b addBucket() {
        uint32_t b;
        if (removed.empty()) {
            b = size++;
        }
        else {
            b = removed.front();
            removed.pop_front();
        }
        failed.reset(b);
        return b;
    }

    uint32_t removeBucket(uint32_t b) {
        --size;
        failed.set(b);
        removed.push_front(b);
        return b;
    }

    uint32_t size() const {
        return size;
    }

    uint32_t capacity() const {
        return capacity;
    }


private:
    uint32_t m_size;
    uint32_t m_capacity;
    boost::dynamic_bitset<> m_failed;
    std::deque<uint32_t> removed;
};


#endif