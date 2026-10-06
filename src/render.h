#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>

#include "camera.h"
#include "hittable.h"
#include "material.h"
#include "rng.h"

// Bands = one fixed block of rows per thread
// Dynamic = threads take the next free row from an atomic counter
enum class Schedule { Bands, Dynamic };

struct RenderSettings {
    int samples_per_pixel = 100;
    int max_depth = 50;
    int threads = 1;
    Schedule schedule = Schedule::Dynamic;
    std::uint64_t seed = 1;
    bool progress = false;
};

struct RenderStats {
    double seconds = 0;
    std::vector<double> thread_seconds;  // busy time per thread
};

struct Image {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> rgb;
};

inline color ray_color(const ray& r, int depth, const hittable& world, Rng& rng) {
    if (depth <= 0) return color(0, 0, 0);

    hit_record rec;
    // 0.001 not 0, otherwise shadow acne
    if (world.hit(r, interval(0.001, infinity), rec)) {
        ray scattered;
        color attenuation;
        if (rec.mat->scatter(r, rec, rng, attenuation, scattered))
            return attenuation * ray_color(scattered, depth - 1, world, rng);
        return color(0, 0, 0);
    }

    // sky
    const vec3 unit_direction = unit_vector(r.direction());
    const double a = 0.5 * (unit_direction.y() + 1.0);
    return (1.0 - a) * color(1.0, 1.0, 1.0) + a * color(0.5, 0.7, 1.0);
}

// gamma 2
inline std::uint8_t to_byte(double linear) {
    const double g = linear > 0 ? std::sqrt(linear) : 0;
    static const interval intensity(0.000, 0.999);
    return static_cast<std::uint8_t>(256 * intensity.clamp(g));
}

inline Image render(const camera& cam, const hittable& world, const RenderSettings& s,
                    RenderStats* stats = nullptr) {
    using clock = std::chrono::steady_clock;

    Image img;
    img.width = cam.image_width();
    img.height = cam.image_height();
    // each row is only written by one thread, so no locking needed
    img.rgb.assign(static_cast<size_t>(img.width) * img.height * 3, 0);

    const int w = img.width;
    const int h = img.height;
    const double sample_scale = 1.0 / s.samples_per_pixel;
    const int n = std::clamp(s.threads, 1, h);

    std::atomic<int> next_row{0};
    std::atomic<int> rows_done{0};
    std::vector<double> busy(n, 0.0);

    auto render_row = [&](int j, Rng& rng) {
        // reseed per row so it doesn't matter which thread gets the row
        rng.reseed(row_seed(s.seed, j));

        std::uint8_t* out = &img.rgb[static_cast<size_t>(j) * w * 3];
        for (int i = 0; i < w; i++) {
            color pixel(0, 0, 0);
            for (int k = 0; k < s.samples_per_pixel; k++)
                pixel += ray_color(cam.get_ray(i, j, rng), s.max_depth, world, rng);
            pixel *= sample_scale;

            out[3 * i + 0] = to_byte(pixel.x());
            out[3 * i + 1] = to_byte(pixel.y());
            out[3 * i + 2] = to_byte(pixel.z());
        }
        rows_done.fetch_add(1, std::memory_order_relaxed);
    };

    const auto t0 = clock::now();
    std::vector<clock::time_point> finished(n, t0);

    auto worker = [&](int t) {
        const auto start = clock::now();
        Rng rng;

        if (s.schedule == Schedule::Bands) {
            const int rows_per_thread = (h + n - 1) / n;
            const int begin = t * rows_per_thread;
            const int end = std::min(h, begin + rows_per_thread);
            for (int j = begin; j < end; j++) render_row(j, rng);
        } else {
            // relaxed is fine, the counter only has to hand out unique rows
            for (int j; (j = next_row.fetch_add(1, std::memory_order_relaxed)) < h;)
                render_row(j, rng);
        }

        finished[t] = clock::now();
        busy[t] = std::chrono::duration<double>(finished[t] - start).count();
    };

    std::vector<std::thread> pool;
    pool.reserve(n);
    for (int t = 0; t < n; t++) pool.emplace_back(worker, t);

    if (s.progress) {
        int done;
        while ((done = rows_done.load(std::memory_order_relaxed)) < h) {
            std::fprintf(stderr, "\rRows remaining %4d ", h - done);
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
        std::fprintf(stderr, "\r                    \r");
    }

    for (auto& th : pool) th.join();

    if (stats) {
        // stop at the last thread finishing, not after the progress loop's sleep
        const auto last = *std::max_element(finished.begin(), finished.end());
        stats->seconds = std::chrono::duration<double>(last - t0).count();
        stats->thread_seconds = busy;
    }
    return img;
}
