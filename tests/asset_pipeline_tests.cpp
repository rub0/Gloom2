#include <gloom/assets/asset_cooker.hpp>
#include <gloom/assets/asset_loader.hpp>
#include <gloom/assets/gltf_importer.hpp>
#include <gloom/assets/mesh_processing.hpp>
#include <gloom/assets/residency_coordinator.hpp>
#include <gloom/assets/scene_catalog.hpp>
#include <gloom/assets/texture_asset.hpp>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace {

void expect(const bool condition, const char *message) {
  if (!condition) {
    throw std::runtime_error{message};
  }
}

class TemporaryDirectory final {
public:
  TemporaryDirectory() {
    const auto nonce =
        std::chrono::steady_clock::now().time_since_epoch().count();
    path_ = std::filesystem::temp_directory_path() /
            ("gloom-assets-" + std::to_string(nonce));
    std::filesystem::create_directories(path_ / "source/models");
    std::filesystem::create_directories(path_ / "cache");
  }

  ~TemporaryDirectory() {
    std::error_code ignored;
    std::filesystem::remove_all(path_, ignored);
  }

  [[nodiscard]] const std::filesystem::path &path() const noexcept {
    return path_;
  }

private:
  std::filesystem::path path_;
};

class ImmediateRenderer final : public gloom::render::Renderer {
public:
  [[nodiscard]] std::string_view name() const noexcept override {
    return "Immediate test renderer";
  }
  [[nodiscard]] gloom::core::SubsystemState state() const noexcept override {
    return state_;
  }
  void start() override { state_ = gloom::core::SubsystemState::running; }
  void tick(double) override {}
  void stop() noexcept override { state_ = gloom::core::SubsystemState::stopped; }
  [[nodiscard]] gloom::render::RenderCapabilities capabilities() const noexcept override {
    return {};
  }
  void resize(std::uint32_t, std::uint32_t) override {}
  void enqueue(gloom::render::MeshUpload upload) override {
    states_[upload.id] = gloom::render::GpuAssetState::resident;
    ++mesh_uploads;
  }
  void enqueue(gloom::render::TextureUpload upload) override {
    expect(upload.mip_levels.size() == 2,
           "Coordinator discarded the cooked texture mip chain");
    states_[upload.id] = gloom::render::GpuAssetState::resident;
    ++texture_uploads;
  }
  void enqueue(gloom::render::MaterialUpload upload) override {
    expect(upload.base_color_texture.value !=
               gloom::render::builtin_white_texture.value,
           "Coordinator did not bind the scene texture to its material");
    states_[upload.id] = gloom::render::GpuAssetState::resident;
    ++material_uploads;
  }
  void release(const gloom::render::RenderAssetId id) override {
    states_.erase(id);
    ++releases;
  }
  [[nodiscard]] gloom::render::GpuAssetState asset_state(
      const gloom::render::RenderAssetId id) const noexcept override {
    const auto found = states_.find(id);
    return found == states_.end() ? gloom::render::GpuAssetState::missing
                                 : found->second;
  }
  [[nodiscard]] gloom::render::GpuResidencyMetrics residency_metrics()
      const noexcept override {
    return {};
  }
  void begin_frame() override {}
  void draw(const gloom::render::RenderSnapshot &) override {}
  void end_frame() override {}

  std::uint32_t mesh_uploads{0};
  std::uint32_t texture_uploads{0};
  std::uint32_t material_uploads{0};
  std::uint32_t releases{0};

private:
  gloom::core::SubsystemState state_{gloom::core::SubsystemState::stopped};
  std::unordered_map<gloom::render::RenderAssetId,
                     gloom::render::GpuAssetState,
                     gloom::render::RenderAssetIdHash>
      states_;
};

void wait_until(gloom::assets::AssetResidencyCoordinator &coordinator,
                const gloom::assets::SceneTicket ticket,
                const gloom::assets::SceneResidencyState expected) {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{5};
  while (std::chrono::steady_clock::now() < deadline) {
    coordinator.update();
    const auto state = coordinator.state(ticket);
    if (state == expected)
      return;
    if (state == gloom::assets::SceneResidencyState::failed)
      throw std::runtime_error{"Scene residency failed: " +
                               std::string{coordinator.error(ticket)}};
    std::this_thread::yield();
  }
  throw std::runtime_error{"Timed out waiting for scene residency"};
}

template <typename Value>
void append(std::vector<std::byte> &bytes, const Value &value) {
  const auto *begin = reinterpret_cast<const std::byte *>(&value);
  bytes.insert(bytes.end(), begin, begin + sizeof(Value));
}

void write_fixture(const std::filesystem::path &root) {
  std::vector<std::byte> buffer;
  for (const std::array<float, 3> position :
       {std::array{-1.0F, 0.0F, 0.0F}, std::array{1.0F, 0.0F, 0.0F},
        std::array{0.0F, 1.0F, 0.0F}}) {
    for (const float value : position)
      append(buffer, value);
  }
  for (std::size_t vertex = 0; vertex < 3; ++vertex) {
    for (const float value : {0.0F, 0.0F, 1.0F})
      append(buffer, value);
  }
  for (const std::array<float, 2> uv :
       {std::array{0.0F, 0.0F}, std::array{1.0F, 0.0F},
        std::array{0.5F, 1.0F}}) {
    for (const float value : uv)
      append(buffer, value);
  }
  for (const std::uint16_t index :
       {std::uint16_t{0}, std::uint16_t{1}, std::uint16_t{2}}) {
    append(buffer, index);
  }
  std::ofstream binary{root / "source/models/triangle.bin", std::ios::binary};
  binary.write(reinterpret_cast<const char *>(buffer.data()),
               static_cast<std::streamsize>(buffer.size()));

  std::vector<std::byte> bitmap;
  bitmap.push_back(std::byte{'B'});
  bitmap.push_back(std::byte{'M'});
  append(bitmap, std::uint32_t{70});
  append(bitmap, std::uint32_t{0});
  append(bitmap, std::uint32_t{54});
  append(bitmap, std::uint32_t{40});
  append(bitmap, std::int32_t{2});
  append(bitmap, std::int32_t{2});
  append(bitmap, std::uint16_t{1});
  append(bitmap, std::uint16_t{32});
  append(bitmap, std::uint32_t{0});
  append(bitmap, std::uint32_t{16});
  append(bitmap, std::int32_t{0});
  append(bitmap, std::int32_t{0});
  append(bitmap, std::uint32_t{0});
  append(bitmap, std::uint32_t{0});
  const std::array<std::uint8_t, 16> pixels{
      0, 0, 255, 255, 0, 255, 0, 255, 255, 0, 0, 255, 255, 255, 255, 255};
  bitmap.insert(bitmap.end(), reinterpret_cast<const std::byte*>(pixels.data()),
                reinterpret_cast<const std::byte*>(pixels.data() + pixels.size()));
  std::ofstream image{root / "source/models/albedo.bmp", std::ios::binary};
  image.write(reinterpret_cast<const char *>(bitmap.data()),
              static_cast<std::streamsize>(bitmap.size()));

  constexpr std::string_view gltf = R"({
  "asset": {"version": "2.0"},
  "buffers": [{"uri": "triangle.bin", "byteLength": 102}],
  "bufferViews": [
    {"buffer": 0, "byteOffset": 0, "byteLength": 36},
    {"buffer": 0, "byteOffset": 36, "byteLength": 36},
    {"buffer": 0, "byteOffset": 72, "byteLength": 24},
    {"buffer": 0, "byteOffset": 96, "byteLength": 6}
  ],
  "accessors": [
    {"bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3", "min": [-1,0,0], "max": [1,1,0]},
    {"bufferView": 1, "componentType": 5126, "count": 3, "type": "VEC3"},
    {"bufferView": 2, "componentType": 5126, "count": 3, "type": "VEC2"},
    {"bufferView": 3, "componentType": 5123, "count": 3, "type": "SCALAR"}
  ],
  "images": [{"uri": "albedo.bmp", "name": "Albedo"}],
  "textures": [{"source": 0}],
  "materials": [{
    "name": "Copper",
    "pbrMetallicRoughness": {
      "baseColorFactor": [0.8, 0.4, 0.2, 1.0],
      "metallicFactor": 0.9,
      "roughnessFactor": 0.25,
      "baseColorTexture": {"index": 0}
    }
  }],
  "meshes": [{"name": "Triangle", "primitives": [{
    "attributes": {"POSITION": 0, "NORMAL": 1, "TEXCOORD_0": 2},
    "indices": 3,
    "material": 0
  }]}],
  "nodes": [{"name": "TriangleNode", "mesh": 0, "translation": [2, 0, 0]}],
  "scenes": [{"name": "Main", "nodes": [0]}],
  "scene": 0
})";
  std::ofstream document{root / "source/models/triangle.gltf",
                         std::ios::binary};
  document.write(gltf.data(), static_cast<std::streamsize>(gltf.size()));
}

void test_virtual_paths_and_catalog() {
  const auto normalized =
      gloom::assets::VirtualPath::parse("Game:\\models//./ship.gltf");
  expect(normalized && normalized->string() == "game:/models/ship.gltf" &&
             normalized->mount() == "game" &&
             normalized->relative() == "models/ship.gltf",
         "Virtual path normalization is incorrect");
  expect(!gloom::assets::VirtualPath::parse("game:/../secret.txt"),
         "Virtual path traversal was accepted");
  expect(!gloom::assets::VirtualPath::parse("C:\\absolute.txt"),
         "Native absolute path was accepted as a virtual path");

  const auto source_a = *gloom::assets::VirtualPath::parse("game:/a.bin");
  const auto source_b = *gloom::assets::VirtualPath::parse("game:/b.bin");
  const auto cooked_a = *gloom::assets::VirtualPath::parse("cache:/a.gasset");
  const auto cooked_b = *gloom::assets::VirtualPath::parse("cache:/b.gasset");
  const auto id_a =
      gloom::assets::make_asset_id(source_a, gloom::assets::AssetType::binary);
  const auto id_b =
      gloom::assets::make_asset_id(source_b, gloom::assets::AssetType::binary);
  gloom::assets::AssetCatalog catalog;
  catalog.add({.id = id_a,
               .type = gloom::assets::AssetType::binary,
               .source = source_a,
               .cooked = cooked_a});
  catalog.add({.id = id_b,
               .type = gloom::assets::AssetType::binary,
               .source = source_b,
               .cooked = cooked_b,
               .dependencies = {id_a}});
  const auto order = catalog.dependency_order();
  expect(order && order->size() == 2 && order->front() == id_a &&
             order->back() == id_b,
         "Asset catalog did not produce dependency-first order");

  gloom::assets::AssetCatalog cycle;
  cycle.add({.id = id_a,
             .type = gloom::assets::AssetType::binary,
             .source = source_a,
             .cooked = cooked_a,
             .dependencies = {id_b}});
  cycle.add({.id = id_b,
             .type = gloom::assets::AssetType::binary,
             .source = source_b,
             .cooked = cooked_b,
             .dependencies = {id_a}});
  expect(!cycle.dependency_order(), "Asset dependency cycle was accepted");
}

void test_offline_mesh_processing() {
  gloom::assets::ImportedPrimitive primitive;
  constexpr std::uint32_t side = 12;
  primitive.vertices.reserve(side * side);
  for (std::uint32_t y = 0; y < side; ++y) {
    for (std::uint32_t x = 0; x < side; ++x) {
      primitive.vertices.push_back({
          .position = {static_cast<float>(x), 0.0F, static_cast<float>(y)},
          .normal = {0.0F, 1.0F, 0.0F},
          .texture_coordinate = {static_cast<float>(x) / (side - 1U),
                                 static_cast<float>(y) / (side - 1U)},
      });
    }
  }
  for (std::uint32_t y = 0; y + 1U < side; ++y) {
    for (std::uint32_t x = 0; x + 1U < side; ++x) {
      const auto first = y * side + x;
      const auto second = first + side;
      primitive.indices.insert(primitive.indices.end(),
                               {first, second, first + 1U,
                                first + 1U, second, second + 1U});
    }
  }
  const auto processed = gloom::assets::process_imported_primitive(primitive);
  expect(processed && primitive.bounds_radius > 7.0F &&
             !primitive.lod_indices.empty() &&
             primitive.lod_indices.front().size() < primitive.indices.size() &&
             primitive.vertices.front().tangent[3] != 0.0F &&
             processed->lod_indices >= primitive.lod_indices.front().size(),
         "Offline tangent, bounds, optimization or LOD generation failed");

  gloom::assets::ImportedPrimitive attribute_less{
      .vertices = {{.position = {0.0F, 0.0F, 0.0F}},
                   {.position = {0.0F, 1.0F, 0.0F}},
                   {.position = {0.0F, 0.0F, 1.0F}}},
      .indices = {0, 1, 2}};
  const auto generated = gloom::assets::process_imported_primitive(
      attribute_less,
      {.source_normals = false, .source_texture_coordinates = false});
  expect(generated && std::abs(attribute_less.vertices.front().normal[0]) > 0.9F &&
             std::isfinite(attribute_less.vertices.front().tangent[0]),
         "Missing glTF normals or UVs did not receive safe offline fallbacks");
  gloom::assets::ImportedPrimitive corner{
      .vertices = {{.position = {0,0,0}}, {.position = {1,0,0}},
                   {.position = {0,1,0}}, {.position = {0,0,1}}},
      .indices = {0,1,2, 0,3,1}};
  expect(gloom::assets::process_imported_primitive(corner,
      {.source_normals = false, .source_texture_coordinates = false}).has_value(), "Hard-edge normal generation failed");
  expect(corner.vertices.size() == 6, "Missing normals incorrectly smoothed a shared hard edge");
  for (std::size_t i = 0; i < corner.indices.size(); i += 3) {
    const auto& a = corner.vertices[corner.indices[i]].normal;
    expect(a == corner.vertices[corner.indices[i+1]].normal && a == corner.vertices[corner.indices[i+2]].normal,
           "Flat triangle has interpolated corner normals");
  }
}

void test_project_mesh_orientation() {
  for (const char* name : {"factory/cargo_lift.gltf", "factory/surface_modules.gltf", "characters/hound.gltf",
                          "characters/berserker.gltf", "weapons/soul_reaper.gltf", "abilities/hound_abilities.gltf"}) {
    const auto scene = gloom::assets::import_gltf(std::filesystem::path{GLOOM_TEST_ASSETS} / name);
    expect(scene.has_value(), "Project glTF did not import");
    for (const auto& primitive : scene->primitives) {
      for (const auto& v : primitive.vertices) {
        float outward = 0;
        for (std::size_t axis = 0; axis < 3; ++axis) outward += v.normal[axis] * (v.position[axis] - primitive.bounds_center[axis]);
        // Factory lava is an open plane with an explicit upward normal.
        const bool plane = primitive.vertices.size() == 4;
        expect(plane ? v.normal[1] > 0.99F : outward > 0.01F, "Project mesh faces inward or lava faces down");
      }
    }
  }
}

void test_gltf_cooking_and_async_loading() {
  TemporaryDirectory temporary;
  write_fixture(temporary.path());
  gloom::assets::VirtualFileSystem filesystem;
  filesystem.mount("game", temporary.path() / "source");
  filesystem.mount("cache", temporary.path() / "cache");
  const auto source =
      *gloom::assets::VirtualPath::parse("game:/models/triangle.gltf");
  const auto output =
      *gloom::assets::VirtualPath::parse("cache:/models/triangle.gasset");
  const auto cooked = gloom::assets::cook_gltf(filesystem, source, output);
  expect(cooked && cooked->dependencies.size() == 1 &&
             cooked->scene.dependencies.size() == 1,
         "glTF cooker did not discover and cook its image dependency");
  const auto discovered =
      gloom::assets::discover_cooked_scene(filesystem, source, output);
  expect(discovered && discovered->scene == cooked->scene.id &&
             discovered->catalog.size() == 2,
         "Runtime could not reconstruct the cooked scene catalog");

  const auto encoded = filesystem.read(output);
  const auto scene_asset =
      encoded ? gloom::assets::decode_cooked_asset(*encoded)
              : std::expected<gloom::assets::CookedAsset, std::string>{
                    std::unexpected{"Missing cooked scene"}};
  expect(scene_asset && scene_asset->type == gloom::assets::AssetType::scene &&
             scene_asset->dependencies == cooked->scene.dependencies,
         "Cooked scene envelope is invalid");
  const auto scene = gloom::assets::decode_imported_scene(scene_asset->payload);
  expect(scene && scene->primitives.size() == 1 && scene->meshes.size() == 1 &&
             scene->materials.size() == 1 && scene->nodes.size() == 1 &&
             scene->primitives.front().vertices.size() == 3 &&
             scene->primitives.front().indices ==
                 std::vector<std::uint32_t>({0, 1, 2}) &&
             scene->primitives.front().bounds_radius > 1.0F &&
             std::abs(scene->primitives.front().vertices.front().tangent[0]) > 0.9F &&
             scene->materials.front().base_color_texture == 0 &&
             scene->nodes.front().local_transform[12] == 2.0F,
         "Cooked glTF scene lost geometry, material or hierarchy data");

  auto corrupted = *encoded;
  corrupted.back() ^= std::byte{1};
  expect(!gloom::assets::decode_cooked_asset(corrupted),
         "Cooked asset payload corruption was not detected");

  gloom::assets::AssetCatalog catalog;
  for (const auto &dependency : cooked->dependencies)
    catalog.add(dependency);
  catalog.add(cooked->scene);
  const auto dependency_order = catalog.dependency_order();
  expect(dependency_order && dependency_order->back() == cooked->scene.id,
         "Cooked scene dependency was not ordered before the scene");

  const auto texture_encoded = filesystem.read(cooked->dependencies.front().cooked);
  const auto texture_asset =
      texture_encoded ? gloom::assets::decode_cooked_asset(*texture_encoded)
                      : std::expected<gloom::assets::CookedAsset, std::string>{
                            std::unexpected{"Missing cooked texture"}};
  expect(texture_asset && texture_asset->type == gloom::assets::AssetType::texture,
         "Cooked texture envelope is invalid");
  const auto texture_upload = texture_asset
                                  ? gloom::assets::decode_texture_ktx2(
                                        {.value = texture_asset->id.value}, texture_asset->payload)
                                  : std::expected<gloom::render::TextureUpload, std::string>{
                                        std::unexpected{"Missing texture payload"}};
  expect(texture_upload && texture_upload->srgb &&
             texture_upload->mip_levels.size() == 2 &&
             texture_upload->mip_levels[0].width == 2 &&
             texture_upload->mip_levels[1].width == 1,
         "KTX2 texture did not preserve dimensions, color space and mip chain");
  const auto source_texture = filesystem.read(
      *gloom::assets::VirtualPath::parse("game:/models/albedo.bmp"));
  const auto normal_payload =
      source_texture ? gloom::assets::cook_texture_ktx2(
                           *source_texture, gloom::assets::TextureSemantic::normal)
                     : std::expected<std::vector<std::byte>, std::string>{
                           std::unexpected{"Missing normal-map fixture"}};
  const auto normal_upload = normal_payload
                                 ? gloom::assets::decode_texture_ktx2(
                                       {.value = 0x44}, *normal_payload)
                                 : std::expected<gloom::render::TextureUpload, std::string>{
                                       std::unexpected{"Missing normal-map payload"}};
  expect(normal_upload && !normal_upload->srgb && normal_upload->mip_levels.size() == 2,
         "Normal-map KTX2 did not use linear UASTC data with mipmaps");
  const auto bc7_upload = gloom::assets::decode_texture_ktx2(
      {.value = 0x46}, texture_asset->payload,
      gloom::assets::TextureTranscodeTarget::bc7);
  const auto bc5_upload = normal_payload
                              ? gloom::assets::decode_texture_ktx2(
                                    {.value = 0x47}, *normal_payload,
                                    gloom::assets::TextureTranscodeTarget::bc5)
                              : std::expected<gloom::render::TextureUpload, std::string>{
                                    std::unexpected{"Missing normal-map payload"}};
  expect(bc7_upload && bc7_upload->format == gloom::render::TextureFormat::bc7 &&
             bc7_upload->mip_levels.front().data.size() == 16 && bc5_upload &&
             bc5_upload->format == gloom::render::TextureFormat::bc5 &&
             !bc5_upload->srgb && bc5_upload->mip_levels.front().data.size() == 16,
         "KTX2 did not transcode directly to native BC7/BC5 GPU blocks");
  expect(!gloom::assets::decode_texture_ktx2({.value = 0x45}, {}),
         "Empty KTX2 payload was accepted");

  gloom::core::JobSystem jobs{{.worker_threads = 2}};
  jobs.start();
  {
    gloom::assets::AsyncAssetLoader loader{jobs, filesystem, catalog};
    const auto scene_future = loader.request(cooked->scene.id);
    const auto cached_future = loader.request(cooked->scene.id);
    const auto missing_future = loader.request({999'999});
    loader.wait();
    const auto loaded = scene_future.get();
    const auto cached = cached_future.get();
    const auto missing = missing_future.get();
    expect(loaded.state == gloom::assets::AssetLoadState::ready &&
               cached.state == gloom::assets::AssetLoadState::ready &&
               missing.state ==
                   gloom::assets::AssetLoadState::error_placeholder &&
               !missing.error.empty() && !missing.asset.payload.empty(),
           "Asynchronous loader did not return ready and placeholder assets "
           "correctly");
    const auto metrics = loader.metrics();
    expect(metrics.requests == 3 && metrics.cache_hits == 1 &&
               metrics.loaded == 1 && metrics.failed == 1 &&
               metrics.bytes_loaded == scene_asset->payload.size(),
           "Asynchronous asset loader metrics are incorrect");

    ImmediateRenderer renderer;
    renderer.start();
    {
      gloom::assets::AssetResidencyCoordinator coordinator{
          jobs, loader, catalog, renderer,
          {.new_scene_requests_per_update = 1}};
      const auto background = coordinator.request_scene(
          {.value = 999'998}, gloom::assets::AssetPriority::background);
      const auto critical = coordinator.request_scene(
          cooked->scene.id, gloom::assets::AssetPriority::critical);
      coordinator.update();
      expect(coordinator.state(background) ==
                 gloom::assets::SceneResidencyState::queued &&
                 coordinator.state(critical) !=
                     gloom::assets::SceneResidencyState::queued,
             "Scene request priority was not respected");
      coordinator.cancel(background);
      expect(coordinator.state(background) ==
                 gloom::assets::SceneResidencyState::cancelled,
             "Queued scene cancellation failed");

      wait_until(coordinator, critical,
                 gloom::assets::SceneResidencyState::ready);
      const auto *resident = coordinator.scene(critical);
      expect(resident != nullptr && resident->generation == 1 &&
                 resident->instances.size() == 1 &&
                 resident->instances.front().transform.position.x == 2.0F &&
                 renderer.mesh_uploads == 1 && renderer.texture_uploads == 1 &&
                 renderer.material_uploads == 1,
             "Imported scene hierarchy or GPU dependencies were not instantiated");

      auto changed_scene = *scene;
      changed_scene.nodes.front().local_transform[12] = 4.0F;
      auto changed_payload = gloom::assets::encode_imported_scene(changed_scene);
      auto changed_asset = *scene_asset;
      changed_asset.payload = std::move(changed_payload);
      changed_asset.source_fingerprint =
          gloom::assets::fingerprint(changed_asset.payload);
      auto changed_record = cooked->scene;
      changed_record.source_fingerprint = changed_asset.source_fingerprint;
      catalog.upsert(changed_record);
      const auto changed_envelope = gloom::assets::encode_cooked_asset(changed_asset);
      const auto write_result = filesystem.write(output, changed_envelope);
      expect(write_result.has_value(), "Could not replace the cooked scene fixture");

      coordinator.reload(critical);
      wait_until(coordinator, critical,
                 gloom::assets::SceneResidencyState::ready);
      resident = coordinator.scene(critical);
      expect(resident != nullptr && resident->generation == 2 &&
                 resident->instances.front().transform.position.x == 4.0F &&
                 loader.metrics().invalidations >= 2,
             "Hot reload did not replace the resident scene generation");

      const auto shared = coordinator.request_scene(
          cooked->scene.id, gloom::assets::AssetPriority::high);
      wait_until(coordinator, shared,
                 gloom::assets::SceneResidencyState::ready);
      expect(coordinator.metrics().shared_resource_hits >= 3 &&
                 renderer.mesh_uploads == 2 && renderer.texture_uploads == 2 &&
                 renderer.material_uploads == 2,
             "Resident GPU resources were uploaded again instead of shared");
      const auto releases_before_cancel = renderer.releases;
      coordinator.cancel(critical);
      expect(renderer.releases == releases_before_cancel,
             "Cancelling one scene released resources still used by another");
      coordinator.cancel(shared);
      expect(renderer.releases == releases_before_cancel + 3,
             "Last scene reference did not release its GPU resources");

      const auto coordinator_metrics = coordinator.metrics();
      expect(coordinator_metrics.requested == 3 &&
                 coordinator_metrics.cancelled == 3 &&
                 coordinator_metrics.reloaded == 1 &&
                 coordinator_metrics.ready == 3,
             "Residency coordinator metrics are incorrect");
    }
    renderer.stop();
  }
  jobs.stop();
}

} // namespace

int main() try {
  test_virtual_paths_and_catalog();
  test_offline_mesh_processing();
  test_project_mesh_orientation();
  test_gltf_cooking_and_async_loading();
  std::cout << "Gloom asset pipeline tests completed successfully.\n";
  return 0;
} catch (const std::exception &error) {
  std::cerr << "Asset pipeline test failure: " << error.what() << '\n';
  return 1;
}
