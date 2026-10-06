#pragma once

#include <cstdint>
#include <random>

#include "vec3.h"

// replaces the book's static random_double(), which is a data race with multiple threads.
// every thread owns one of these and passes it down
class Rng {
public:
    explicit Rng(std::uint64_t seed = 0) : engine_(seed) {}

    void reseed(std::uint64_t seed) { engine_.seed(seed); }

    // [0, 1) from the top 53 bits, same result on every compiler unlike uniform_real_distribution
    double uniform() { return static_cast<double>(engine_() >> 11) * 0x1.0p-53; }
    double uniform(double lo, double hi) { return lo + (hi - lo) * uniform(); }

private:
    std::mt19937_64 engine_;
};

// splitmix64 hash so nearby seeds give unrelated streams
inline std::uint64_t splitmix64(std::uint64_t x) {
    x += 0x9E3779B97F4A7C15ULL;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    return x ^ (x >> 31);
}

// depends on the row not the thread, so the image is the same for any thread count
inline std::uint64_t row_seed(std::uint64_t base, int row) {
    return splitmix64(base ^ splitmix64(static_cast<std::uint64_t>(row)));
}

inline vec3 random_vec3(Rng& rng, double lo, double hi) {
    return vec3(rng.uniform(lo, hi), rng.uniform(lo, hi), rng.uniform(lo, hi));
}

// rejection sample inside the unit sphere, then normalise
inline vec3 random_unit_vector(Rng& rng) {
    while (true) {
        vec3 p = random_vec3(rng, -1, 1);
        double len_sq = p.length_squared();
        if (1e-160 < len_sq && len_sq <= 1) return p / std::sqrt(len_sq);
    }
}

inline vec3 random_in_unit_disk(Rng& rng) {
    while (true) {
        vec3 p(rng.uniform(-1, 1), rng.uniform(-1, 1), 0);
        if (p.length_squared() < 1) return p;
    }
}
