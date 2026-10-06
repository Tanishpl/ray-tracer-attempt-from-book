#pragma once

#include <memory>
#include <vector>

#include "hittable.h"

// checks every object, O(n) per ray. used with --no-bvh
class hittable_list : public hittable {
public:
    std::vector<std::shared_ptr<hittable>> objects;

    void add(std::shared_ptr<hittable> object) {
        bbox_ = aabb(bbox_, object->bounding_box());
        objects.push_back(std::move(object));
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        hit_record temp;
        bool hit_anything = false;
        double closest_so_far = ray_t.max;

        for (const auto& object : objects) {
            if (object->hit(r, interval(ray_t.min, closest_so_far), temp)) {
                hit_anything = true;
                closest_so_far = temp.t;
                rec = temp;
            }
        }
        return hit_anything;
    }

    aabb bounding_box() const override { return bbox_; }

private:
    aabb bbox_;
};
