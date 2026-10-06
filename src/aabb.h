#pragma once

#include <utility>

#include "interval.h"
#include "ray.h"

class aabb {
public:
    interval x, y, z;

    aabb() = default;
    aabb(const interval& x, const interval& y, const interval& z) : x(x), y(y), z(z) {}

    // a and b are opposite corners
    aabb(const point3& a, const point3& b)
        : x(a[0] <= b[0] ? interval(a[0], b[0]) : interval(b[0], a[0])),
          y(a[1] <= b[1] ? interval(a[1], b[1]) : interval(b[1], a[1])),
          z(a[2] <= b[2] ? interval(a[2], b[2]) : interval(b[2], a[2])) {}

    aabb(const aabb& a, const aabb& b) : x(a.x, b.x), y(a.y, b.y), z(a.z, b.z) {}

    const interval& axis(int n) const {
        if (n == 1) return y;
        if (n == 2) return z;
        return x;
    }

    int longest_axis() const {
        if (x.size() > y.size()) return x.size() > z.size() ? 0 : 2;
        return y.size() > z.size() ? 1 : 2;
    }

    // slab test, the ray has to be inside all three slabs over some shared range of t
    bool hit(const ray& r, interval ray_t) const {
        const point3& orig = r.origin();
        const vec3& dir = r.direction();

        for (int a = 0; a < 3; a++) {
            const interval& ax = axis(a);
            const double inv_d = 1.0 / dir[a];  // inf if dir[a] is 0, still works

            double t0 = (ax.min - orig[a]) * inv_d;
            double t1 = (ax.max - orig[a]) * inv_d;
            if (t0 > t1) std::swap(t0, t1);

            if (t0 > ray_t.min) ray_t.min = t0;
            if (t1 < ray_t.max) ray_t.max = t1;
            if (ray_t.max <= ray_t.min) return false;
        }
        return true;
    }
};
