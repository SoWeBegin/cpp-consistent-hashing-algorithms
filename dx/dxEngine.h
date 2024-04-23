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
#include <pcg_random.hpp>

class DxEngine final {
public:
    explicit DxEngine(uint32_t capacity, uint32_t size)
        : m_size(size), m_capacity(size * capacity) {
        m_failed.resize(m_capacity);
        m_failed.set(m_size, m_capacity - m_size, true);
    }

    uint32_t getBucketCRC32c(uint64_t key, uint64_t seed) {
        auto hashValue = crc32c_sse42_u64(key, seed);
        pcg32 rng;
        static std::uniform_int_distribution<uint32_t> distribution(0, m_capacity - 1);
        rng.seed(hashValue);
        uint32_t b = distribution(rng);
        while (m_failed.test(b)) {
            b = distribution(rng); // Trova un bucket non fallito
        }
        return b;
    }

    uint32_t addBucket() {
        uint32_t b;
        if (m_removed.empty()) {
            b = m_size;
        }
        else {
            b = m_removed.front();
            m_removed.pop_front();
        }
        m_failed.reset(b);
        ++m_size;

        return b;
    }

    uint32_t removeBucket(uint32_t b) {
        --m_size;
        m_failed.set(b);
        m_removed.push_front(b);

        return b;
    }

    uint32_t size() const noexcept {
        return m_size;
    }

    uint32_t capacity() const noexcept {
        return m_capacity;
    }


private:
    uint32_t m_size;
    uint32_t m_capacity;
    boost::dynamic_bitset<> m_failed;
    std::deque<uint32_t> m_removed;
};


#endif