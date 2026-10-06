#pragma once

#include <cstdint>
#include <memory>

#include "camera.h"
#include "hittable_list.h"
#include "material.h"
#include "rng.h"
#include "sphere.h"

// final scene from the end of the book. fixed seed so it's the same scene every run
inline hittable_list final_scene(std::uint64_t scene_seed = 42) {
    Rng rng(scene_seed);
    hittable_list world;

    auto ground = std::make_shared<lambertian>(color(0.5, 0.5, 0.5));
    world.add(std::make_shared<sphere>(point3(0, -1000, 0), 1000, ground));

    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            const double choose_mat = rng.uniform();
            const point3 center(a + 0.9 * rng.uniform(), 0.2, b + 0.9 * rng.uniform());

            if ((center - point3(4, 0.2, 0)).length() <= 0.9) continue;

            std::shared_ptr<material> mat;
            if (choose_mat < 0.8) {
                const color albedo = random_vec3(rng, 0, 1) * random_vec3(rng, 0, 1);
                mat = std::make_shared<lambertian>(albedo);
            } else if (choose_mat < 0.95) {
                const color albedo = random_vec3(rng, 0.5, 1);
                const double fuzz = rng.uniform(0, 0.5);
                mat = std::make_shared<metal>(albedo, fuzz);
            } else {
                mat = std::make_shared<dielectric>(1.5);
            }
            world.add(std::make_shared<sphere>(center, 0.2, mat));
        }
    }

    world.add(std::make_shared<sphere>(point3(0, 1, 0), 1.0, std::make_shared<dielectric>(1.5)));
    world.add(std::make_shared<sphere>(point3(-4, 1, 0), 1.0, std::make_shared<lambertian>(color(0.4, 0.2, 0.1))));
    world.add(std::make_shared<sphere>(point3(4, 1, 0), 1.0, std::make_shared<metal>(color(0.7, 0.6, 0.5), 0.0)));

    return world;
}

inline CameraSettings final_camera(int image_width) {
    CameraSettings c;
    c.aspect_ratio = 16.0 / 9.0;
    c.image_width = image_width;
    c.vfov = 20;
    c.lookfrom = point3(13, 2, 3);
    c.lookat = point3(0, 0, 0);
    c.vup = vec3(0, 1, 0);
    c.defocus_angle = 0.6;
    c.focus_dist = 10.0;
    return c;
}
