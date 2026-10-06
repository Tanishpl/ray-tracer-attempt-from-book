#pragma once

#include <algorithm>
#include <cmath>

#include "ray.h"
#include "rng.h"

constexpr double pi = 3.1415926535897932385;  // M_PI isn't standard

struct CameraSettings {
    double aspect_ratio = 16.0 / 9.0;
    int image_width = 400;
    double vfov = 90;  // degrees
    point3 lookfrom = point3(0, 0, 0);
    point3 lookat = point3(0, 0, -1);
    vec3 vup = vec3(0, 1, 0);
    double defocus_angle = 0;  // 0 = no blur
    double focus_dist = 10;
};

// only read after construction, so all the threads can share one
class camera {
public:
    explicit camera(const CameraSettings& s) : defocus_angle_(s.defocus_angle) {
        image_width_ = s.image_width;
        image_height_ = std::max(1, static_cast<int>(s.image_width / s.aspect_ratio));
        center_ = s.lookfrom;

        const double theta = s.vfov * pi / 180.0;
        const double viewport_height = 2 * std::tan(theta / 2) * s.focus_dist;
        const double viewport_width = viewport_height * (static_cast<double>(image_width_) / image_height_);

        // camera basis, w points backwards
        w_ = unit_vector(s.lookfrom - s.lookat);
        u_ = unit_vector(cross(s.vup, w_));
        v_ = cross(w_, u_);

        const vec3 viewport_u = viewport_width * u_;
        const vec3 viewport_v = viewport_height * -v_;
        pixel_delta_u_ = viewport_u / image_width_;
        pixel_delta_v_ = viewport_v / image_height_;

        const point3 viewport_upper_left = center_ - s.focus_dist * w_ - viewport_u / 2 - viewport_v / 2;
        pixel00_loc_ = viewport_upper_left + 0.5 * (pixel_delta_u_ + pixel_delta_v_);

        const double defocus_radius = s.focus_dist * std::tan(s.defocus_angle / 2 * pi / 180.0);
        defocus_disk_u_ = u_ * defocus_radius;
        defocus_disk_v_ = v_ * defocus_radius;
    }

    int image_width() const { return image_width_; }
    int image_height() const { return image_height_; }

    // random point inside pixel (i, j), averaging lots of these is the antialiasing
    ray get_ray(int i, int j, Rng& rng) const {
        const double ox = rng.uniform() - 0.5;
        const double oy = rng.uniform() - 0.5;
        const point3 pixel_sample = pixel00_loc_ + (i + ox) * pixel_delta_u_ + (j + oy) * pixel_delta_v_;

        const point3 origin = (defocus_angle_ <= 0) ? center_ : defocus_disk_sample(rng);
        return ray(origin, pixel_sample - origin);
    }

private:
    int image_width_ = 0;
    int image_height_ = 0;
    double defocus_angle_;
    point3 center_;
    point3 pixel00_loc_;
    vec3 pixel_delta_u_, pixel_delta_v_;
    vec3 u_, v_, w_;
    vec3 defocus_disk_u_, defocus_disk_v_;

    // thin lens, rays start from a random point on a disk around the camera
    point3 defocus_disk_sample(Rng& rng) const {
        const vec3 p = random_in_unit_disk(rng);
        return center_ + p[0] * defocus_disk_u_ + p[1] * defocus_disk_v_;
    }
};
