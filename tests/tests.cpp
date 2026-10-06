// run with ./build/tests

#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>

#include "bvh.h"
#include "render.h"
#include "scene.h"

static int failures = 0;

#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            std::printf("  FAIL %s (line %d)\n", #cond, __LINE__);        \
            failures++;                                                   \
        }                                                                 \
    } while (0)

static bool near(double a, double b, double eps = 1e-9) { return std::fabs(a - b) < eps; }

// ray down -z from the origin should hit a radius 0.5 sphere at z = -1 at t = 0.5
static void test_sphere_hit() {
    std::printf("sphere intersection\n");
    auto mat = std::make_shared<lambertian>(color(0.5, 0.5, 0.5));
    sphere s(point3(0, 0, -1), 0.5, mat);
    hit_record rec;

    CHECK(s.hit(ray(point3(0, 0, 0), vec3(0, 0, -1)), interval(0.001, infinity), rec));
    CHECK(near(rec.t, 0.5));
    CHECK(near(rec.normal.z(), 1.0));
    CHECK(rec.front_face);

    // from inside, should get the far root and a flipped normal
    CHECK(s.hit(ray(point3(0, 0, -1), vec3(0, 0, -1)), interval(0.001, infinity), rec));
    CHECK(near(rec.t, 0.5));
    CHECK(!rec.front_face);

    CHECK(!s.hit(ray(point3(0, 0, 0), vec3(0, 0, 1)), interval(0.001, infinity), rec));
}

static void test_reflect_refract() {
    std::printf("reflect and refract\n");
    const vec3 n(0, 1, 0);
    const vec3 v = unit_vector(vec3(1, -1, 0));

    const vec3 r = reflect(v, n);
    CHECK(near(r.x(), v.x()) && near(r.y(), -v.y()));

    // same index both sides, should go straight through
    const vec3 t = refract(v, n, 1.0);
    CHECK(near(t.x(), v.x()) && near(t.y(), v.y()) && near(t.z(), v.z()));

    // snell's law
    const vec3 g = refract(v, n, 1.0 / 1.5);
    const double sin_in = std::sqrt(1 - dot(-v, n) * dot(-v, n));
    const double sin_out = std::sqrt(1 - dot(-g, n) * dot(-g, n));
    CHECK(near(sin_out, sin_in / 1.5));
}

// bvh has to give exactly the same closest hit as brute force
static void test_bvh_matches_list() {
    std::printf("BVH gives the same hits as the plain list\n");
    hittable_list list = final_scene();
    bvh_node bvh(list);

    Rng rng(7);
    int mismatches = 0, hits = 0;
    const int n = 200000;
    for (int k = 0; k < n; k++) {
        const point3 origin = random_vec3(rng, -12, 12) + vec3(0, 12, 0);
        const ray r(origin, random_unit_vector(rng));

        hit_record a, b;
        const bool ha = list.hit(r, interval(0.001, infinity), a);
        const bool hb = bvh.hit(r, interval(0.001, infinity), b);
        if (ha != hb || (ha && (a.t != b.t || a.mat != b.mat))) mismatches++;
        hits += ha;
    }
    std::printf("  %d rays, %d hits, %d mismatches\n", n, hits, mismatches);
    CHECK(mismatches == 0);
    CHECK(hits > n / 4);  // make sure enough rays actually hit something
}

// image should be byte-identical for any thread count, schedule and bvh setting
static void test_render_is_deterministic() {
    std::printf("render is identical across thread counts, schedules and BVH\n");
    hittable_list list = final_scene();
    bvh_node bvh(list);
    const camera cam(final_camera(96));

    RenderSettings s;
    s.samples_per_pixel = 4;
    s.max_depth = 10;
    s.threads = 1;
    s.schedule = Schedule::Bands;
    const Image reference = render(cam, list, s);

    int combos = 0;
    for (int threads : {2, 3, 5, 8}) {
        for (Schedule sched : {Schedule::Bands, Schedule::Dynamic}) {
            for (int use_bvh = 0; use_bvh <= 1; use_bvh++) {
                s.threads = threads;
                s.schedule = sched;
                const Image img = use_bvh ? render(cam, bvh, s) : render(cam, list, s);
                if (img.rgb != reference.rgb)
                    std::printf("  differs with %d threads, %s, BVH %s\n", threads,
                                sched == Schedule::Bands ? "bands" : "dynamic", use_bvh ? "on" : "off");
                CHECK(img.rgb == reference.rgb);
                combos++;
            }
        }
    }
    std::printf("  %d configurations compared against the single-threaded render\n", combos);

    // different seed should give a different image, otherwise the seed is ignored
    s.threads = 2;
    s.seed = 2;
    CHECK(render(cam, bvh, s).rgb != reference.rgb);
}

int main() {
    test_sphere_hit();
    test_reflect_refract();
    test_bvh_matches_list();
    test_render_is_deterministic();

    if (failures) {
        std::printf("%d check(s) failed\n", failures);
        return 1;
    }
    std::printf("All tests passed\n");
    return 0;
}
