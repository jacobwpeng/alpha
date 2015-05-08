/*
 * =============================================================================
 *
 *       Filename:  random.cc
 *        Created:  05/08/15 13:03:35
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#include "random.h"

namespace alpha {
    class ThreadLocalPRNG::LocalInstancePRNG {
        public:
            LocalInstancePRNG()
                :rng(Random::Create()) {
            }

            Random::DefaultRNG rng;
    };

    ThreadLocalPRNG::result_type ThreadLocalPRNG::impl(
            ThreadLocalPRNG::LocalInstancePRNG* local) {
        return local->rng();
    }

    ThreadLocalPRNG::ThreadLocalPRNG() {
        thread_local static LocalInstancePRNG local;
        local_ = &local;
    }
}
