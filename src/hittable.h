#pragma once

#include "aabb.h"
#include "interval.h"
#include "ray.h"

class material;

struct hit_record {
    point3 p;
    vec3 normal;
    const material* mat = nullptr;  // raw pointer not shared_ptr, so threads don't all hit the same atomic refcount
    double t = 0;
    bool front_face = true;

    // normal always points against the ray. outward_normal has to be unit length
    void set_face_normal(const ray& r, const vec3& outward_normal) {
        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

class hittable {
public:
    virtual ~hittable() = default;

    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const = 0;
    virtual aabb bounding_box() const = 0;
};
