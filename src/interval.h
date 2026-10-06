#pragma once

#include <limits>

constexpr double infinity = std::numeric_limits<double>::infinity();

struct interval {
    double min, max;

    interval() : min(+infinity), max(-infinity) {}  // empty
    interval(double lo, double hi) : min(lo), max(hi) {}

    // tightest interval around both
    interval(const interval& a, const interval& b)
        : min(a.min <= b.min ? a.min : b.min), max(a.max >= b.max ? a.max : b.max) {}

    double size() const { return max - min; }
    bool contains(double x) const { return min <= x && x <= max; }
    bool surrounds(double x) const { return min < x && x < max; }

    double clamp(double x) const {
        if (x < min) return min;
        if (x > max) return max;
        return x;
    }
};
