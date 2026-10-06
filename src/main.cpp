#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>

#include "bvh.h"
#include "render.h"
#include "scene.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace {

struct Options {
    int width = 600;
    RenderSettings render;
    bool use_bvh = true;
    std::string out = "render.png";
};

void usage(const char* prog) {
    std::printf(
        "Usage %s [options]\n"
        "  --width N          image width in pixels (default 600, height follows 16 by 9)\n"
        "  --spp N            samples per pixel (default 100)\n"
        "  --depth N          maximum bounces per path (default 50)\n"
        "  --threads N        worker threads (default all hardware threads)\n"
        "  --schedule MODE    bands or dynamic (default dynamic)\n"
        "  --no-bvh           test every object for every ray instead of using the BVH\n"
        "  --seed N           render seed, same seed gives the same image (default 1)\n"
        "  --out FILE         output file, .png or .ppm (default render.png)\n"
        "  --quiet            no progress output\n",
        prog);
}

bool ends_with(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool write_image(const Image& img, const std::string& path) {
    if (ends_with(path, ".ppm")) {
        FILE* f = std::fopen(path.c_str(), "wb");
        if (!f) return false;
        std::fprintf(f, "P6\n%d %d\n255\n", img.width, img.height);
        const size_t n = std::fwrite(img.rgb.data(), 1, img.rgb.size(), f);
        std::fclose(f);
        return n == img.rgb.size();
    }
    return stbi_write_png(path.c_str(), img.width, img.height, 3, img.rgb.data(), img.width * 3) != 0;
}

}  // namespace

int main(int argc, char** argv) {
    Options opt;
    opt.render.threads = static_cast<int>(std::max(1u, std::thread::hardware_concurrency()));
    opt.render.progress = true;

    for (int i = 1; i < argc; i++) {
        const std::string arg = argv[i];
        auto next = [&]() -> const char* {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "Missing value after %s\n", arg.c_str());
                std::exit(1);
            }
            return argv[++i];
        };

        if (arg == "--width") opt.width = std::atoi(next());
        else if (arg == "--spp") opt.render.samples_per_pixel = std::atoi(next());
        else if (arg == "--depth") opt.render.max_depth = std::atoi(next());
        else if (arg == "--threads") opt.render.threads = std::atoi(next());
        else if (arg == "--seed") opt.render.seed = std::strtoull(next(), nullptr, 10);
        else if (arg == "--out") opt.out = next();
        else if (arg == "--no-bvh") opt.use_bvh = false;
        else if (arg == "--quiet") opt.render.progress = false;
        else if (arg == "--schedule") {
            const std::string mode = next();
            if (mode == "bands") opt.render.schedule = Schedule::Bands;
            else if (mode == "dynamic") opt.render.schedule = Schedule::Dynamic;
            else {
                std::fprintf(stderr, "Unknown schedule %s (use bands or dynamic)\n", mode.c_str());
                return 1;
            }
        } else if (arg == "--help" || arg == "-h") {
            usage(argv[0]);
            return 0;
        } else {
            std::fprintf(stderr, "Unknown option %s\n", arg.c_str());
            usage(argv[0]);
            return 1;
        }
    }

    if (opt.width < 1 || opt.render.samples_per_pixel < 1 || opt.render.max_depth < 1 || opt.render.threads < 1) {
        std::fprintf(stderr, "width, spp, depth and threads must all be at least 1\n");
        return 1;
    }

    hittable_list scene = final_scene();
    const size_t object_count = scene.objects.size();

    // same objects either way, so --no-bvh only changes the speed
    std::shared_ptr<hittable> world;
    if (opt.use_bvh) world = std::make_shared<bvh_node>(scene);
    else world = std::make_shared<hittable_list>(scene);

    const camera cam(final_camera(opt.width));

    RenderStats stats;
    const Image img = render(cam, *world, opt.render, &stats);

    if (!write_image(img, opt.out)) {
        std::fprintf(stderr, "Could not write %s\n", opt.out.c_str());
        return 1;
    }

    const auto [lo, hi] = std::minmax_element(stats.thread_seconds.begin(), stats.thread_seconds.end());
    std::printf("%dx%d, %d spp, %zu objects, BVH %s, %zu threads (%s)\n",
                img.width, img.height, opt.render.samples_per_pixel, object_count,
                opt.use_bvh ? "on" : "off", stats.thread_seconds.size(),
                opt.render.schedule == Schedule::Bands ? "bands" : "dynamic");
    std::printf("Rendered in %.2f s. Thread busy time min %.2f s, max %.2f s. Wrote %s\n",
                stats.seconds, *lo, *hi, opt.out.c_str());
    return 0;
}
