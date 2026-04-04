#pragma once
#include <cstdlib>
#include <ctime>
#include <vector>

// Random number utilities for KonEngine / KonScript
namespace Random {

// Seed the random number generator (call once at startup)
inline void Seed()            { std::srand((unsigned)std::time(nullptr)); }
inline void Seed(int seed)    { std::srand((unsigned)seed); }

// Random integer in [min, max] (inclusive)
inline int Range(int min, int max) {
    if (min >= max) return min;
    return min + std::rand() % (max - min + 1);
}

// Random float in [min, max]
inline float RangeF(float min, float max) {
    if (min >= max) return min;
    return min + ((float)std::rand() / (float)RAND_MAX) * (max - min);
}

// Random element from a vector
template<typename T>
inline const T& From(const std::vector<T>& v) {
    return v[std::rand() % v.size()];
}

// Random bool with optional probability (0.0 to 1.0)
inline bool Bool(float probability = 0.5f) {
    return ((float)std::rand() / (float)RAND_MAX) < probability;
}

// Random float 0.0 to 1.0
inline float Value() {
    return (float)std::rand() / (float)RAND_MAX;
}

} // namespace Random
