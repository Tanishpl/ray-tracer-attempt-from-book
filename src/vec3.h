#pragma once

#include <cmath>

struct vec3 {
    double e[3];

    vec3() : e{0, 0, 0} {}
    vec3(double x, double y, double z) : e{x, y, z} {}

    double x() const { return e[0]; }
    double y() const { return e[1]; }
    double z() const { return e[2]; }

    vec3 operator-() const { return vec3(-e[0], -e[1], -e[2]); }
    double operator[](int i) const { return e[i]; }
    double& operator[](int i) { return e[i]; }

    vec3& operator+=(const vec3& v) {
        e[0] += v.e[0];
        e[1] += v.e[1];
        e[2] += v.e[2];
        return *this;
    }

    vec3& operator*=(double t) {
        e[0] *= t;
        e[1] *= t;
        e[2] *= t;
        return *this;
    }

    double length_squared() const { return e[0] * e[0] + e[1] * e[1] + e[2] * e[2]; }
    double length() const { return std::sqrt(length_squared()); }

    // true if close to zero in every dimension
    bool near_zero() const {
        const double s = 1e-8;
        return std::fabs(e[0]) < s && std::fabs(e[1]) < s && std::fabs(e[2]) < s;
    }
};

using point3 = vec3;
using color = vec3;

inline vec3 operator+(const vec3& u, const vec3& v) { return vec3(u.e[0] + v.e[0], u.e[1] + v.e[1], u.e[2] + v.e[2]); }
inline vec3 operator-(const vec3& u, const vec3& v) { return vec3(u.e[0] - v.e[0], u.e[1] - v.e[1], u.e[2] - v.e[2]); }
inline vec3 operator*(const vec3& u, const vec3& v) { return vec3(u.e[0] * v.e[0], u.e[1] * v.e[1], u.e[2] * v.e[2]); }
inline vec3 operator*(double t, const vec3& v) { return vec3(t * v.e[0], t * v.e[1], t * v.e[2]); }
inline vec3 operator*(const vec3& v, double t) { return t * v; }
inline vec3 operator/(const vec3& v, double t) { return (1 / t) * v; }

inline double dot(const vec3& u, const vec3& v) { return u.e[0] * v.e[0] + u.e[1] * v.e[1] + u.e[2] * v.e[2]; }

inline vec3 cross(const vec3& u, const vec3& v) {
    return vec3(u.e[1] * v.e[2] - u.e[2] * v.e[1],
                u.e[2] * v.e[0] - u.e[0] * v.e[2],
                u.e[0] * v.e[1] - u.e[1] * v.e[0]);
}

inline vec3 unit_vector(const vec3& v) { return v / v.length(); }

inline vec3 reflect(const vec3& v, const vec3& n) { return v - 2 * dot(v, n) * n; }

// snell's law, split into the parts perpendicular and parallel to n
inline vec3 refract(const vec3& uv, const vec3& n, double etai_over_etat) {
    double cos_theta = std::fmin(dot(-uv, n), 1.0);
    vec3 r_out_perp = etai_over_etat * (uv + cos_theta * n);
    vec3 r_out_parallel = -std::sqrt(std::fabs(1.0 - r_out_perp.length_squared())) * n;
    return r_out_perp + r_out_parallel;
}
