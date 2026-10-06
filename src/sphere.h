#pragma once

#include <cmath>
#include <memory>

#include "hittable.h"

class sphere : public hittable {
public:
    sphere(const point3& center, double radius, std::shared_ptr<material> mat)
        : center_(center), radius_(std::fmax(0, radius)), mat_(std::move(mat)) {
        vec3 rvec(radius_, radius_, radius_);
        bbox_ = aabb(center_ - rvec, center_ + rvec);
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        // quadratic in t, using h = b / -2 so the 2s cancel
        vec3 oc = center_ - r.origin();
        double a = r.direction().length_squared();
        double h = dot(r.direction(), oc);
        double c = oc.length_squared() - radius_ * radius_;

        double discriminant = h * h - a * c;
        if (discriminant < 0) return false;

        double sqrtd = std::sqrt(discriminant);

        // nearest root in range, the far one if we're inside the sphere
        double root = (h - sqrtd) / a;
        if (!ray_t.surrounds(root)) {
            root = (h + sqrtd) / a;
            if (!ray_t.surrounds(root)) return false;
        }

        rec.t = root;
        rec.p = r.at(rec.t);
        vec3 outward_normal = (rec.p - center_) / radius_;
        rec.set_face_normal(r, outward_normal);
        rec.mat = mat_.get();
        return true;
    }

    aabb bounding_box() const override { return bbox_; }

private:
    point3 center_;
    double radius_;
    std::shared_ptr<material> mat_;
    aabb bbox_;
};
