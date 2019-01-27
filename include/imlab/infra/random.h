#ifndef _INCLUDE_RANDOM_H_
#define _INCLUDE_RANDOM_H_

#include <cstdint>

// https://stackoverflow.com/a/1640399
uint64_t xorshf96() {
    static uint64_t x = 123456789, y = 362436069, z = 521288629;

    x ^= x << 16;
    x ^= x >> 5;
    x ^= x << 1;

    uint64_t t = x;
    x = y;
    y = z;
    z = t ^ x ^ y;

    return z;
}

#endif  // _INCLUDE_RANDOM_H_
