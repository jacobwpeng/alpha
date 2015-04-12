/*
 * ==============================================================================
 *
 *       Filename:  main.cc
 *        Created:  04/12/15 10:19:00
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#include <cassert>
#include <iostream>
#include <type_traits>
#include "slice.h"

struct PODType {
    int val;
};

int main() {
    static_assert(std::is_pod<PODType>::value, "Invalid PODType");
    PODType obj;
    alpha::Slice data(&obj);
    assert (reinterpret_cast<intptr_t>(data.data()) == reinterpret_cast<intptr_t>(&obj));
    assert (data.size() == sizeof(obj));

    auto pobj = data.as<PODType>();
    assert (pobj == &obj);
}
