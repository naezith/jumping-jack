#ifndef UTILITY_H
#define UTILITY_H

#include <random>

static std::mt19937 mt(1337);
static int random_int(int lower, int higher) {
    std::uniform_int_distribution<int> dist(lower, higher);
    return dist(mt);
}

static float random_float(float lower, float higher) {
    std::uniform_real_distribution<float> dist(lower, higher);
    return dist(mt);
}

#endif // UTILITY_H
