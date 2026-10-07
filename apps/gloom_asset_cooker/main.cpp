#include <gloom/assets/asset_cooker.hpp>
#include <filesystem>
#include <iostream>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#ifdef _WIN32
int wmain(int argument_count, const wchar_t* const* arguments) {
#else
int main(int argument_count, const char* const* arguments) {
#endif
    if (argument_count != 5) {
        std::cerr << "Usage: gloom_asset_cooker <source-root> <cache-root> <game:/source.gltf> <cache:/output.gasset>\n";
        return 2;
    }
#ifdef _WIN32
    for (int i = 1; i < argument_count; ++i)
        if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, arguments[i], -1, nullptr, 0, nullptr, nullptr)) {
            std::cerr << "Asset cooker argument is not valid Unicode.\n";
            return 2;
        }
#endif
    const std::u8string source_utf8 = std::filesystem::path{arguments[3]}.generic_u8string();
    const std::u8string output_utf8 = std::filesystem::path{arguments[4]}.generic_u8string();
    const std::expected<gloom::assets::VirtualPath, std::string> source =
        gloom::assets::VirtualPath::parse({reinterpret_cast<const char*>(source_utf8.data()), source_utf8.size()});
    const std::expected<gloom::assets::VirtualPath, std::string> output =
        gloom::assets::VirtualPath::parse({reinterpret_cast<const char*>(output_utf8.data()), output_utf8.size()});
    if (!source || !output || source->mount() != "game" || output->mount() != "cache") {
        std::cerr << "Source and output must be valid game:/ and cache:/ virtual paths.\n";
        return 2;
    }
    gloom::assets::VirtualFileSystem filesystem;
    const std::expected<void, std::string> game = filesystem.mount("game", std::filesystem::path{arguments[1]});
    if (!game) {
        std::cerr << game.error() << '\n';
        return 1;
    }
    const std::expected<void, std::string> cache = filesystem.mount("cache", std::filesystem::path{arguments[2]});
    if (!cache) {
        std::cerr << cache.error() << '\n';
        return 1;
    }
    const std::expected<gloom::assets::GltfCookResult, std::string> result = gloom::assets::cook_gltf(filesystem, *source, *output);
    if (!result) {
        std::cerr << "Asset cooking failed: " << result.error() << '\n';
        return 1;
    }
    std::cout << "Cooked scene " << result->scene.id.value << " with " << result->dependencies.size() << " external dependencies.\n";
    return 0;
}
