#pragma once

#include <cmath>

#include "hittable.h"
#include "rng.h"

class material {
public:
    virtual ~material() = default;

    // false means the ray was absorbed
    virtual bool scatter(const ray& r_in, const hit_record& rec, Rng& rng,
                         color& attenuation, ray& scattered) const = 0;
};

// matte. normal + random unit vector gives the cosine distribution
class lambertian : public material {
public:
    explicit lambertian(const color& albedo) : albedo_(albedo) {}

    bool scatter(const ray&, const hit_record& rec, Rng& rng,
                 color& attenuation, ray& scattered) const override {
        vec3 dir = rec.normal + random_unit_vector(rng);
        if (dir.near_zero()) dir = rec.normal;  // would give NaNs later
        scattered = ray(rec.p, dir);
        attenuation = albedo_;
        return true;
    }

private:
    color albedo_;
};

// fuzz 0 is a perfect mirror
class metal : public material {
public:
    metal(const color& albedo, double fuzz) : albedo_(albedo), fuzz_(fuzz < 1 ? fuzz : 1) {}

    bool scatter(const ray& r_in, const hit_record& rec, Rng& rng,
                 color& attenuation, ray& scattered) const override {
        vec3 reflected = reflect(r_in.direction(), rec.normal);
        reflected = unit_vector(reflected) + fuzz_ * random_unit_vector(rng);
        scattered = ray(rec.p, reflected);
        attenuation = albedo_;
        return dot(scattered.direction(), rec.normal) > 0;  // absorbed if fuzz pushed it under the surface
    }

private:
    color albedo_;
    double fuzz_;
};

// glass
class dielectric : public material {
public:
    explicit dielectric(double refraction_index) : refraction_index_(refraction_index) {}

    bool scatter(const ray& r_in, const hit_record& rec, Rng& rng,
                 color& attenuation, ray& scattered) const override {
        attenuation = color(1.0, 1.0, 1.0);
        double ri = rec.front_face ? (1.0 / refraction_index_) : refraction_index_;

        vec3 unit_direction = unit_vector(r_in.direction());
        double cos_theta = std::fmin(dot(-unit_direction, rec.normal), 1.0);
        double sin_theta = std::sqrt(1.0 - cos_theta * cos_theta);

        bool cannot_refract = ri * sin_theta > 1.0;  // total internal reflection

        vec3 direction;
        if (cannot_refract || reflectance(cos_theta, ri) > rng.uniform())
            direction = reflect(unit_direction, rec.normal);
        else
            direction = refract(unit_direction, rec.normal, ri);

        scattered = ray(rec.p, direction);
        return true;
    }

private:
    double refraction_index_;

    // schlick's approximation
    static double reflectance(double cosine, double ri) {
        double r0 = (1 - ri) / (1 + ri);
        r0 = r0 * r0;
        return r0 + (1 - r0) * std::pow(1 - cosine, 5);
    }
};
