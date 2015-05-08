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
}
