#include <gloom/assets/asset_cooker.hpp>

#include <filesystem>
#include <iostream>

int main(const int argument_count, const char *const *arguments) try {
  if (argument_count != 5) {
    std::cerr << "Usage: gloom_asset_cooker <source-root> <cache-root> "
                 "<game:/source.gltf> <cache:/output.gasset>\n";
    return 2;
  }
  const auto source = gloom::assets::VirtualPath::parse(arguments[3]);
  const auto output = gloom::assets::VirtualPath::parse(arguments[4]);
  if (!source || !output || source->mount() != "game" ||
      output->mount() != "cache") {
    std::cerr << "Source and output must be valid game:/ and cache:/ virtual "
                 "paths.\n";
    return 2;
  }
  gloom::assets::VirtualFileSystem filesystem;
  filesystem.mount("game", std::filesystem::path{arguments[1]});
  filesystem.mount("cache", std::filesystem::path{arguments[2]});
  const auto result = gloom::assets::cook_gltf(filesystem, *source, *output);
  if (!result) {
    std::cerr << "Asset cooking failed: " << result.error() << '\n';
    return 1;
  }
  std::cout << "Cooked scene " << result->scene.id.value << " with "
            << result->dependencies.size() << " external dependencies.\n";
  return 0;
} catch (const std::exception &error) {
  std::cerr << "Asset cooker failed: " << error.what() << '\n';
  return 1;
}
