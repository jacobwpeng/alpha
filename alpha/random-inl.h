/*
 * =============================================================================
 *
 *       Filename:  random-inl.h
 *        Created:  05/08/15 11:57:17
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#ifndef __RANDOM_H__
#error This file should only be included from alpha/random.h
#endif

#include "random.h"
#include <set>
#include <algorithm>

namespace alpha {
    template<typename RNG>
    Random::ValidRNG<RNG> Random::Create() {
        auto rng = Random::DefaultRNG();
        Random::Seed(rng);
        return rng;
    }

    template<typename RNG>
    void Random::Seed(ValidRNG<RNG>& rng) {
        std::random_device rdev;
        rng.seed(rdev());
    }

    template<typename InputIterator, typename RNG>
    std::vector<InputIterator> Random::SampleImpl(InputIterator first,
            InputIterator last, size_t k, std::true_type,
            Random::ValidRNG<RNG> rng) {
        std::vector<InputIterator> res;
        auto distance = std::distance(first, last);
        if (static_cast<size_t>(distance) <= k) {
            while (first != last) {
                res.push_back(first);
                ++first;
            }
            return res;
        }

        std::set<uint32_t> random_numbers;
        while (random_numbers.size() < k) {
            random_numbers.insert(Random::Rand32(distance, rng));
        }

        std::for_each(random_numbers.begin(), random_numbers.end(),
                [&res, first](uint32_t num) {
            auto it = first;
            std::advance(it, num);
            res.push_back(it);
        });
        return res;
    }

    template<typename InputIterator, typename RNG>
    std::vector<InputIterator> Random::SampleImpl(InputIterator first,
            InputIterator last, size_t k, std::false_type,
            Random::ValidRNG<RNG> rng) {
        // Reservoir sampling
        // http://en.wikipedia.org/wiki/Reservoir_sampling
        std::vector<InputIterator> res;
        size_t i = 0;
        while (i < k && first != last) {
            res.push_back(first);
            ++i;
            ++first;
        }

        if (i == k) {
            while (first != last) {
                auto r = Random::Rand32(i, rng);
                if (r < k) {
                    res[r] = first;
                }
                ++i;
                ++first;
            }
        }
        return res;
    }
}
