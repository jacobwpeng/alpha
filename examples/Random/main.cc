/*
 * =============================================================================
 *
 *       Filename:  main.cc
 *        Created:  05/08/15 13:10:22
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#include <iostream>
#include <alpha/random.h>

int main() {
    std::vector<int> v;
    for (int i = 0; i < 100; ++i) {
        v.emplace_back(i);
    }
    alpha::Random::Shuffle(v.begin(), v.end());
    std::for_each(v.begin(), v.end(), [](int v){
            static int i = 0;
            std::cout << v << '\t';
            if (++i % 10 == 0) {
                std::cout << '\n';
            }
    });
    std::cout << '\n';
}
