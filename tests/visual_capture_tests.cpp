#include "visual_review.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
struct Image { unsigned width, height; std::vector<unsigned char> rgb; };
Image read(const std::filesystem::path& path) {
    std::ifstream input{path, std::ios::binary};
    std::string format; unsigned width = 0, height = 0, max = 0;
    input >> format >> width >> height >> max;
    if (!input || format != "P6" || !width || !height || width > 8192 || height > 8192 || max != 255 || input.get() != '\n')
        throw std::runtime_error{"Invalid/missing capture: " + path.string()};
    Image image{width, height, std::vector<unsigned char>(static_cast<std::size_t>(width)*height*3)};
    input.read(reinterpret_cast<char*>(image.rgb.data()), static_cast<std::streamsize>(image.rgb.size()));
    if (!input || input.peek() != std::char_traits<char>::eof()) throw std::runtime_error{"Truncated/oversized capture: " + path.string()};
    return image;
}
Image thumbnail(const Image& source) {
    constexpr unsigned width = 160, height = 90;
    if (source.width % width || source.height % height) throw std::runtime_error{"Capture dimensions must be multiples of 160x90"};
    Image result{width, height, std::vector<unsigned char>(width*height*3)};
    const unsigned sx = source.width/width, sy = source.height/height;
    for (unsigned y = 0; y < height; ++y) for (unsigned x = 0; x < width; ++x) for (unsigned c = 0; c < 3; ++c) {
        unsigned sum = 0;
        for (unsigned dy = 0; dy < sy; ++dy) for (unsigned dx = 0; dx < sx; ++dx)
            sum += source.rgb[((y*sy+dy)*source.width+x*sx+dx)*3+c];
        result.rgb[(y*width+x)*3+c] = static_cast<unsigned char>((sum+sx*sy/2)/(sx*sy));
    }
    return result;
}
void write(const Image& image, const std::filesystem::path& path) {
    std::ofstream out{path, std::ios::binary};
    out << "P6\n" << image.width << ' ' << image.height << "\n255\n";
    out.write(reinterpret_cast<const char*>(image.rgb.data()), static_cast<std::streamsize>(image.rgb.size()));
    if (!out) throw std::runtime_error{"Could not write reference"};
}
struct Difference { double mean{0}, changed{0}, worst_tile{0}; };
Difference difference(const Image& image, const Image& reference) {
    if (image.width != reference.width || image.height != reference.height) throw std::runtime_error{"Reference size mismatch"};
    Difference result;
    for (unsigned ty = 0; ty < 6; ++ty) for (unsigned tx = 0; tx < 8; ++tx) {
        double tile = 0;
        for (unsigned y = ty*15; y < (ty+1)*15; ++y) for (unsigned x = tx*20; x < (tx+1)*20; ++x) {
            double pixel = 0;
            for (unsigned c = 0; c < 3; ++c) {
                const auto at = (y*image.width+x)*3+c;
                pixel += std::abs(static_cast<int>(image.rgb[at])-static_cast<int>(reference.rgb[at]))/3.0;
            }
            tile += pixel;
            result.changed += pixel > 32.0 ? 1.0 : 0.0;
        }
        result.mean += tile;
        result.worst_tile = std::max(result.worst_tile, tile/300.0);
    }
    result.mean /= image.width*image.height;
    result.changed /= image.width*image.height;
    return result;
}
bool acceptable(const Difference& d) { return d.mean <= 4.0 && d.changed <= 0.015 && d.worst_tile <= 12.0; }
}
int main(int argc, const char* const* argv) try {
    if (argc < 3 || argc > 5) throw std::invalid_argument{"Usage: visual_capture_tests CAPTURES REFERENCES [--factory] [--write-reference]"};
    bool approve=false,factory=false,characters=false,ui=false;
    for(int i=3;i<argc;++i) {
        if(std::string_view{argv[i]}=="--write-reference") approve=true;
        else if(std::string_view{argv[i]}=="--factory") factory=true;
        else if(std::string_view{argv[i]}=="--characters") characters=true;
        else if(std::string_view{argv[i]}=="--ui") ui=true;
        else throw std::invalid_argument{"Unknown visual comparison option"};
    }
    if (approve) std::filesystem::create_directories(argv[2]);
    bool failed = false;
    const auto views=characters ? std::span<const gloom::review::View>{gloom::review::character_views} : factory ? std::span<const gloom::review::View>{gloom::review::factory_views} : std::span<const gloom::review::View>{gloom::review::views};
    std::vector<std::string> names;
    if(ui){for(const auto width:{1280,1920,2560})for(int page=0;page<16;++page)names.push_back(std::to_string(width)+"/page-"+std::to_string(page)+".ppm");}
    else for(const auto& view:views)names.push_back(std::string{view.name}+".ppm");
    for (const auto& name : names) {
        const auto actual = thumbnail(read(std::filesystem::path{argv[1]} / name));
        const auto path = std::filesystem::path{argv[2]} / name;
        if (approve) { std::filesystem::create_directories(path.parent_path());write(actual, path); continue; }
        const auto reference = read(path);
        const auto diff = difference(actual, reference);
        std::cout << name << ": mean=" << diff.mean << "/255 changed=" << diff.changed*100
                  << "% worst_tile=" << diff.worst_tile << "/255\n";
        failed |= !acceptable(diff);
        // Ensure a blank image or a missing foreground quadrant cannot pass a
        // tolerant comparison even when most of the frame is static sky/floor.
        auto missing = reference;
        std::fill(missing.rgb.begin(), missing.rgb.end(), 0);
        if (acceptable(difference(missing, reference))) throw std::runtime_error{"Blank-frame negative control passed"};
        missing = reference;
        for (unsigned y = 45; y < 90; ++y) for (unsigned x = 80; x < 160; ++x)
            for (unsigned c = 0; c < 3; ++c) missing.rgb[(y*160+x)*3+c] = 0;
        if (acceptable(difference(missing, reference))) throw std::runtime_error{"Missing-foreground negative control passed"};
    }
    if (failed) throw std::runtime_error{"Visual regression: inspect full-resolution captures and render.log; references were not changed"};
    return 0;
} catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
