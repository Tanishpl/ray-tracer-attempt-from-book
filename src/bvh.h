#pragma once

#include <algorithm>
#include <memory>
#include <vector>

#include "hittable.h"
#include "hittable_list.h"

// if the ray misses a node's box it can't hit anything under it, so the whole subtree is skipped
class bvh_node : public hittable {
public:
    explicit bvh_node(hittable_list list) : bvh_node(list.objects, 0, list.objects.size()) {}

    bvh_node(std::vector<std::shared_ptr<hittable>>& objects, size_t start, size_t end) {
        for (size_t i = start; i < end; i++)
            bbox_ = aabb(bbox_, objects[i]->bounding_box());

        const int axis = bbox_.longest_axis();
        const size_t span = end - start;

        if (span == 1) {
            left_ = right_ = objects[start];
        } else if (span == 2) {
            left_ = objects[start];
            right_ = objects[start + 1];
        } else {
            // nth_element instead of the book's full sort, O(n) per level so the build is O(n log n)
            const size_t mid = start + span / 2;
            std::nth_element(objects.begin() + start, objects.begin() + mid, objects.begin() + end,
                             [axis](const std::shared_ptr<hittable>& a, const std::shared_ptr<hittable>& b) {
                                 return a->bounding_box().axis(axis).min < b->bounding_box().axis(axis).min;
                             });
            left_ = std::make_shared<bvh_node>(objects, start, mid);
            right_ = std::make_shared<bvh_node>(objects, mid, end);
        }
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        if (!bbox_.hit(r, ray_t)) return false;

        bool hit_left = left_->hit(r, ray_t, rec);
        // right side only counts if it's closer than whatever the left hit
        bool hit_right = right_->hit(r, interval(ray_t.min, hit_left ? rec.t : ray_t.max), rec);
        return hit_left || hit_right;
    }

    aabb bounding_box() const override { return bbox_; }

private:
    std::shared_ptr<hittable> left_;
    std::shared_ptr<hittable> right_;
    aabb bbox_;
};
