#include <gloom/assets/asset_cooker.hpp>
#include <gloom/assets/asset_loader.hpp>
#include <gloom/assets/gltf_importer.hpp>
#include <gloom/assets/mesh_processing.hpp>
#include <gloom/assets/residency_coordinator.hpp>
#include <gloom/assets/scene_catalog.hpp>
#include <gloom/assets/texture_asset.hpp>
#include <gloom/core/types.hpp>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>

#include <string>
#include <thread>

#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _DEBUG
#include <crtdbg.h>
#endif

namespace {

void expect(const bool condition, const char* message) {
    if (!condition) {
        fputs(message, stderr);
        fputc('\n', stderr);
        exit(1);
    }
}

// Native gate releases the worker before scoped owners drain; failed checks terminate the test process.
struct PausedWorker {
    gloom::core::JobSystem& jobs;
    gloom::core::TaskGroup group;
    HANDLE entered, released;
    struct BlockWorker {
        HANDLE entered, released;
        void operator()() const noexcept {
            SetEvent(entered);
            WaitForSingleObject(released, INFINITE);
        }
    };
    explicit PausedWorker(gloom::core::JobSystem& system)
        : jobs{system}, group{jobs.create_group()}, entered{CreateEventW(nullptr, TRUE, FALSE, nullptr)},
          released{CreateEventW(nullptr, TRUE, FALSE, nullptr)} {
        expect(entered && released, "Could not initialize worker gate");
        jobs.schedule(group, BlockWorker{.entered = entered, .released = released});
        expect(WaitForSingleObject(entered, 5000) == WAIT_OBJECT_0, "Worker did not enter gate");
    }
    void release() {
        SetEvent(released);
        jobs.wait(group);
    }
    ~PausedWorker() {
        release();
        CloseHandle(entered);
        CloseHandle(released);
    }
};

class TemporaryDirectory final {
  public:
    TemporaryDirectory() {
        const std::chrono::steady_clock::rep nonce = std::chrono::steady_clock::now().time_since_epoch().count();
        std::error_code error;
        path_ = std::filesystem::temp_directory_path(error) / ("gloom-assets-" + std::to_string(nonce));
        expect(!error, "Locate temporary asset directory");
        std::filesystem::create_directories(path_ / "source/models", error);
        expect(!error, "Create asset source directory");
        std::filesystem::create_directories(path_ / "cache", error);
        expect(!error, "Create asset cache directory");
    }

    ~TemporaryDirectory() {
        std::error_code ignored;
        std::filesystem::remove_all(path_, ignored);
    }

    [[nodiscard]] const std::filesystem::path& path() const noexcept {
        return path_;
    }

  private:
    std::filesystem::path path_;
};

struct ImmediateRenderer {
    gloom::Array<gloom::render::RenderAssetId> states;
    gloom::uint32 mesh_uploads{0}, texture_uploads{0}, material_uploads{0}, releases{0};
    bool check_alias{false};
    static bool compression_bc(const void*) {
        return false;
    }
    static void mesh(void* context, gloom::render::MeshUpload upload) {
        ImmediateRenderer& self = *static_cast<ImmediateRenderer*>(context);
        self.states.push_back(upload.id);
        ++self.mesh_uploads;
    }
    static void texture(void* context, gloom::render::TextureUpload upload) {
        expect(upload.mip_levels.size() == 2, "Coordinator discarded the cooked texture mip chain");
        ImmediateRenderer& self = *static_cast<ImmediateRenderer*>(context);
        self.states.push_back(upload.id);
        ++self.texture_uploads;
    }
    static void material(void* context, gloom::render::MaterialUpload upload) {
        expect(upload.base_color_texture.value != gloom::render::builtin_white_texture.value, "Coordinator did not bind the scene texture");
        ImmediateRenderer& self = *static_cast<ImmediateRenderer*>(context);
        if (self.check_alias)
            expect(upload.extra_textures[0] == upload.base_color_texture, "URI alias did not bind the same GPU texture");
        self.states.push_back(upload.id);
        ++self.material_uploads;
    }
    static void release(void* context, gloom::render::RenderAssetId id) {
        ImmediateRenderer& self = *static_cast<ImmediateRenderer*>(context);
        for (size_t i = 0; i < self.states.size(); ++i) {
            if (self.states[i] == id) {
                self.states[i] = self.states[self.states.size() - 1];
                self.states.resize(self.states.size() - 1);
                break;
            }
        }
        ++self.releases;
    }
    static gloom::render::GpuAssetState state(const void* context, gloom::render::RenderAssetId id) {
        const ImmediateRenderer& self = *static_cast<const ImmediateRenderer*>(context);
        for (gloom::render::RenderAssetId resident : self.states)
            if (resident == id)
                return gloom::render::GpuAssetState::resident;
        return gloom::render::GpuAssetState::missing;
    }
};

void wait_until(
    gloom::assets::AssetResidencyCoordinator& coordinator, const gloom::assets::SceneTicket ticket, const gloom::assets::SceneResidencyState expected) {
    const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds{5};
    while (std::chrono::steady_clock::now() < deadline) {
        coordinator.update();
        const gloom::assets::SceneResidencyState state = coordinator.state(ticket);
        if (state == expected)
            return;
        if (state == gloom::assets::SceneResidencyState::failed)
            expect(false, "Scene residency failed");
        std::this_thread::yield();
    }
    expect(false, "Timed out waiting for scene residency");
}

template <typename Value> void append(std::vector<std::byte>& bytes, const Value& value) {
    const std::byte* begin = reinterpret_cast<const std::byte*>(&value);
    bytes.insert(bytes.end(), begin, begin + sizeof(Value));
}

void write_fixture(const std::filesystem::path& root) {
    std::vector<std::byte> buffer;
    for (const std::array<float, 3> position : {std::array{-1.0F, 0.0F, 0.0F}, std::array{1.0F, 0.0F, 0.0F}, std::array{0.0F, 1.0F, 0.0F}}) {
        for (const float value : position)
            append(buffer, value);
    }
    for (size_t vertex = 0; vertex < 3; ++vertex) {
        for (const float value : {0.0F, 0.0F, 1.0F})
            append(buffer, value);
    }
    for (const std::array<float, 2> uv : {std::array{0.0F, 0.0F}, std::array{1.0F, 0.0F}, std::array{0.5F, 1.0F}}) {
        for (const float value : uv)
            append(buffer, value);
    }
    for (const gloom::uint16 index : {gloom::uint16{0}, gloom::uint16{1}, gloom::uint16{2}}) {
        append(buffer, index);
    }
    std::ofstream binary{root / "source/models/triangle.bin", std::ios::binary};
    binary.write(reinterpret_cast<const char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));

    std::vector<std::byte> bitmap;
    bitmap.push_back(std::byte{'B'});
    bitmap.push_back(std::byte{'M'});
    append(bitmap, gloom::uint32{70});
    append(bitmap, gloom::uint32{0});
    append(bitmap, gloom::uint32{54});
    append(bitmap, gloom::uint32{40});
    append(bitmap, gloom::int32{2});
    append(bitmap, gloom::int32{2});
    append(bitmap, gloom::uint16{1});
    append(bitmap, gloom::uint16{32});
    append(bitmap, gloom::uint32{0});
    append(bitmap, gloom::uint32{16});
    append(bitmap, gloom::int32{0});
    append(bitmap, gloom::int32{0});
    append(bitmap, gloom::uint32{0});
    append(bitmap, gloom::uint32{0});
    const std::array<gloom::uint8, 16> pixels{0, 0, 255, 255, 0, 255, 0, 255, 255, 255, 0, 255, 255, 255, 255, 255};
    bitmap.insert(bitmap.end(), reinterpret_cast<const std::byte*>(pixels.data()), reinterpret_cast<const std::byte*>(pixels.data() + pixels.size()));
    std::ofstream image{root / "source/models/albedo.bmp", std::ios::binary};
    image.write(reinterpret_cast<const char*>(bitmap.data()), static_cast<std::streamsize>(bitmap.size()));

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
    std::ofstream document{root / "source/models/triangle.gltf", std::ios::binary};
    document.write(gltf.data(), static_cast<std::streamsize>(gltf.size()));
}

void test_virtual_paths_and_catalog() {
    const std::expected<gloom::assets::VirtualPath, std::string> normalized = gloom::assets::VirtualPath::parse("Game:\\models//./ship.gltf");
    expect(normalized && normalized->string() == "game:/models/ship.gltf" && normalized->mount() == "game" && normalized->relative() == "models/ship.gltf",
        "Virtual path normalization is incorrect");
    const std::expected<gloom::assets::VirtualPath, std::string> dependency = gloom::assets::dependency_source_path(*normalized, "..\\textures/./diffuse.png");
    expect(dependency && dependency->string() == "game:/textures/diffuse.png", "Shared dependency resolver changed parent/separator normalization");
    expect(!gloom::assets::dependency_source_path(*normalized, "../../secret.png"), "Shared dependency resolver escaped its mount");
    const std::expected<gloom::assets::VirtualPath, std::string> cooked_dependency =
        gloom::assets::dependency_cooked_path(*gloom::assets::VirtualPath::parse("cache:/models/ship.gasset"), {.value = 0x1234});
    expect(cooked_dependency && cooked_dependency->string() == "cache:/models/dependencies/1234.gasset", "Shared cooked dependency layout changed");
    expect(!gloom::assets::VirtualPath::parse("game:/../secret.txt"), "Virtual path traversal was accepted");
#ifdef _WIN32
    expect(!gloom::assets::VirtualPath::parse("game:/.. /secret.txt") && !gloom::assets::VirtualPath::parse("game:/folder./secret.txt"),
        "Win32 path normalization bypassed traversal policy");
#endif
    expect(!gloom::assets::VirtualPath::parse("C:\\absolute.txt"), "Native absolute path was accepted as a virtual path");
    expect(!gloom::assets::VirtualPath::parse("game:/\xC0\xAF") && !gloom::assets::VirtualPath::parse("game:/\xED\xA0\x80") &&
               !gloom::assets::VirtualPath::parse("game:/\xF4\x90\x80\x80"),
        "Malformed UTF-8 path was accepted");
    TemporaryDirectory directory;
    gloom::assets::VirtualFileSystem filesystem;
    expect(filesystem.mount("game", directory.path() / "source").has_value(), "Mount Unicode fixture");
    expect(!filesystem.mount("GAME", directory.path() / "source") && !filesystem.mount("missing", directory.path() / "absent"), "Invalid mount accepted");
    std::expected<gloom::assets::VirtualPath, std::string> unicode = gloom::assets::VirtualPath::parse("game:/\xC3\xA1rbol/\xE7\x8C\xAB-\xF0\x9F\x90\xBA.bin");
    expect(unicode.has_value(), "Parse Unicode filename");
    gloom::assets::VirtualPath moved_path = std::move(*unicode);
    const std::array<std::byte, 3> bytes{std::byte{0}, std::byte{42}, std::byte{255}};
    expect(filesystem.write(moved_path, bytes).has_value(), "Write Unicode filename");
    const std::expected<std::vector<std::byte>, std::string> read = filesystem.read(moved_path);
    expect(read && read->size() == bytes.size() && memcmp(read->data(), bytes.data(), bytes.size()) == 0, "Moved path or Unicode bytes lost ownership");
    const std::expected<std::filesystem::path, std::string> resolved = filesystem.resolve(moved_path);
    expect(resolved && resolved->filename() == std::filesystem::path{L"\u732B-\U0001F43A.bin"}, "Unicode path was converted through ANSI");
    expect(filesystem.write(moved_path, {}).has_value() && filesystem.read(moved_path)->empty(), "Empty owned buffer could not roundtrip");
    std::error_code link_error;
    std::filesystem::create_directory_symlink(directory.path() / "cache", directory.path() / "source/escape", link_error);
    if (!link_error)
        expect(!filesystem.resolve(*gloom::assets::VirtualPath::parse("game:/escape/outside.bin")), "Symlink escaped the mount");

    const gloom::assets::VirtualPath source_a = *gloom::assets::VirtualPath::parse("game:/a.bin");
    const gloom::assets::VirtualPath source_b = *gloom::assets::VirtualPath::parse("game:/b.bin");
    const gloom::assets::VirtualPath cooked_a = *gloom::assets::VirtualPath::parse("cache:/a.gasset");
    const gloom::assets::VirtualPath cooked_b = *gloom::assets::VirtualPath::parse("cache:/b.gasset");
    const gloom::assets::AssetId id_a = gloom::assets::make_asset_id(source_a, gloom::assets::AssetType::binary);
    const gloom::assets::AssetId id_b = gloom::assets::make_asset_id(source_b, gloom::assets::AssetType::binary);
    gloom::assets::AssetCatalog catalog;
    catalog.add({.id = id_a, .type = gloom::assets::AssetType::binary, .source = source_a, .cooked = cooked_a});
    catalog.add({.id = id_b, .type = gloom::assets::AssetType::binary, .source = source_b, .cooked = cooked_b, .dependencies = {id_a}});
    gloom::Array<gloom::assets::AssetId> order;
    const char* error = nullptr;
    expect(catalog.dependency_order(order, error) && !error && order.size() == 2 && order[0] == id_a && order[1] == id_b,
        "Asset catalog did not produce dependency-first order");

    gloom::assets::AssetCatalog cycle;
    cycle.add({.id = id_a, .type = gloom::assets::AssetType::binary, .source = source_a, .cooked = cooked_a, .dependencies = {id_b}});
    cycle.add({.id = id_b, .type = gloom::assets::AssetType::binary, .source = source_b, .cooked = cooked_b, .dependencies = {id_a}});
    expect(!cycle.dependency_order(order, error) && error && order.size() == 0, "Asset dependency cycle was accepted");

    const gloom::assets::AssetRecord* stable = catalog.find(id_a);
    gloom::assets::AssetId previous = id_b;
    for (gloom::uint32 index = 0; index < 4096; ++index) {
        char path[64];
        snprintf(path, sizeof(path), "game:/catalog/chain-%u.bin", index);
        const decltype(gloom::assets::VirtualPath::parse(path)) parsed = gloom::assets::VirtualPath::parse(path);
        const gloom::assets::AssetId id = gloom::assets::make_asset_id(*parsed, gloom::assets::AssetType::binary);
        catalog.add({.id = id, .source = *parsed, .cooked = cooked_a, .dependencies = {previous}});
        previous = id;
    }
    expect(catalog.find(id_a) == stable && !catalog.find({}), "Catalog growth moved a record or accepted a missing ID");
    expect(catalog.dependency_order(order, error) && !error && order.size() == 4098 && order[0] == id_a && order[order.size() - 1] == previous,
        "Iterative dependency traversal lost a deep dependency chain");
    catalog.upsert({.id = id_a, .source = source_a, .cooked = cooked_a, .source_fingerprint = 42});
    expect(catalog.find(id_a) == stable && stable->source_fingerprint == 42, "Catalog replacement changed the record address or missed its update");
    gloom::assets::AssetCatalog moved{static_cast<gloom::assets::AssetCatalog&&>(catalog)};
    expect(catalog.size() == 0 && moved.find(id_a) == stable, "Catalog move did not transfer its records");
    cycle = static_cast<gloom::assets::AssetCatalog&&>(moved);
    expect(moved.size() == 0 && cycle.find(id_a) == stable && cycle.dependency_order(order, error), "Catalog move assignment lost ownership");
    gloom::assets::AssetCatalog missing;
    missing.add({.id = id_a, .source = source_a, .cooked = cooked_a, .dependencies = {id_b}});
    expect(!missing.dependency_order(order, error) && error && order.size() == 0, "Missing dependency was accepted or left a partial output");
}

void test_offline_mesh_processing() {
    gloom::assets::ImportedPrimitive primitive;
    constexpr gloom::uint32 side = 12;
    primitive.vertices.reserve(side * side);
    for (gloom::uint32 y = 0; y < side; ++y) {
        for (gloom::uint32 x = 0; x < side; ++x) {
            primitive.vertices.push_back({
                .position = {static_cast<float>(x), 0.0F, static_cast<float>(y)},
                .normal = {0.0F, 1.0F, 0.0F},
                .texture_coordinate = {static_cast<float>(x) / (side - 1U), static_cast<float>(y) / (side - 1U)},
            });
        }
    }
    for (gloom::uint32 y = 0; y + 1U < side; ++y) {
        for (gloom::uint32 x = 0; x + 1U < side; ++x) {
            const gloom::uint32 first = y * side + x;
            const gloom::uint32 second = first + side;
            primitive.indices.insert(primitive.indices.end(), {first, second, first + 1U, first + 1U, second, second + 1U});
        }
    }
    const std::expected<gloom::assets::MeshProcessingMetrics, std::string> processed = gloom::assets::process_imported_primitive(primitive);
    expect(processed && primitive.bounds_radius > 7.0F && !primitive.lod_indices.empty() && primitive.lod_indices.front().size() < primitive.indices.size() &&
               primitive.vertices.front().tangent[3] != 0.0F && processed->lod_indices >= primitive.lod_indices.front().size(),
        "Offline tangent, bounds, optimization or LOD generation failed");

    gloom::assets::ImportedPrimitive attribute_less{
        .vertices = {{.position = {0.0F, 0.0F, 0.0F}}, {.position = {0.0F, 1.0F, 0.0F}}, {.position = {0.0F, 0.0F, 1.0F}}}, .indices = {0, 1, 2}};
    const std::expected<gloom::assets::MeshProcessingMetrics, std::string> generated =
        gloom::assets::process_imported_primitive(attribute_less, {.source_normals = false, .source_texture_coordinates = false});
    expect(generated && std::abs(attribute_less.vertices.front().normal[0]) > 0.9F && std::isfinite(attribute_less.vertices.front().tangent[0]),
        "Missing glTF normals or UVs did not receive safe offline fallbacks");
    gloom::assets::ImportedPrimitive corner{
        .vertices = {{.position = {0, 0, 0}}, {.position = {1, 0, 0}}, {.position = {0, 1, 0}}, {.position = {0, 0, 1}}}, .indices = {0, 1, 2, 0, 3, 1}};
    expect(gloom::assets::process_imported_primitive(corner, {.source_normals = false, .source_texture_coordinates = false}).has_value(),
        "Hard-edge normal generation failed");
    expect(corner.vertices.size() == 6, "Missing normals incorrectly smoothed a shared hard edge");
    for (size_t i = 0; i < corner.indices.size(); i += 3) {
        const std::array<float, 3>& a = corner.vertices[corner.indices[i]].normal;
        expect(a == corner.vertices[corner.indices[i + 1]].normal && a == corner.vertices[corner.indices[i + 2]].normal,
            "Flat triangle has interpolated corner normals");
    }
}

void test_project_mesh_orientation() {
    for (const char* name : {"factory/cargo_lift.gltf", "factory/surface_modules.gltf", "characters/hound.gltf", "characters/berserker.gltf",
             "weapons/soul_reaper.gltf", "abilities/hound_abilities.gltf"}) {
        const std::expected<gloom::assets::ImportedScene, std::string> scene = gloom::assets::import_gltf(std::filesystem::path{GLOOM_TEST_ASSETS} / name);
        expect(scene.has_value(), "Project glTF did not import");
        for (const gloom::assets::ImportedPrimitive& primitive : scene->primitives) {
            for (const gloom::assets::ImportedVertex& v : primitive.vertices) {
                float outward = 0;
                for (size_t axis = 0; axis < 3; ++axis)
                    outward += v.normal[axis] * (v.position[axis] - primitive.bounds_center[axis]);
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
    expect(filesystem.mount("game", temporary.path() / "source").has_value(), "Could not mount asset directory");
    expect(filesystem.mount("cache", temporary.path() / "cache").has_value(), "Could not mount asset directory");
    const gloom::assets::VirtualPath source = *gloom::assets::VirtualPath::parse("game:/models/triangle.gltf");
    const gloom::assets::VirtualPath output = *gloom::assets::VirtualPath::parse("cache:/models/triangle.gasset");
    const std::expected<gloom::assets::GltfCookResult, std::string> cooked = gloom::assets::cook_gltf(filesystem, source, output);
    expect(cooked && cooked->dependencies.size() == 1 && cooked->scene.dependencies.size() == 1, "glTF cooker did not discover and cook its image dependency");
    const std::expected<gloom::assets::DiscoveredSceneCatalog, std::string> discovered = gloom::assets::discover_cooked_scene(filesystem, source, output);
    expect(discovered && discovered->scene == cooked->scene.id && discovered->catalog.size() == 2, "Runtime could not reconstruct the cooked scene catalog");

    const std::expected<std::vector<std::byte>, std::string> encoded = filesystem.read(output);
    const std::expected<gloom::assets::CookedAsset, std::string> scene_asset =
        encoded ? gloom::assets::decode_cooked_asset(*encoded)
                : std::expected<gloom::assets::CookedAsset, std::string>{std::unexpected{"Missing cooked scene"}};
    expect(scene_asset && scene_asset->type == gloom::assets::AssetType::scene && scene_asset->dependencies == cooked->scene.dependencies,
        "Cooked scene envelope is invalid");
    const std::expected<gloom::assets::ImportedScene, std::string> scene = gloom::assets::decode_imported_scene(scene_asset->payload);
    expect(scene && scene->primitives.size() == 1 && scene->meshes.size() == 1 && scene->materials.size() == 1 && scene->nodes.size() == 1 &&
               scene->primitives.front().vertices.size() == 3 && scene->primitives.front().indices == std::vector<gloom::uint32>({0, 1, 2}) &&
               scene->primitives.front().bounds_radius > 1.0F && std::abs(scene->primitives.front().vertices.front().tangent[0]) > 0.9F &&
               scene->materials.front().base_color_texture == 0 && scene->nodes.front().local_transform[12] == 2.0F,
        "Cooked glTF scene lost geometry, material or hierarchy data");

    std::vector<std::byte> corrupted = *encoded;
    corrupted.back() ^= std::byte{1};
    expect(!gloom::assets::decode_cooked_asset(corrupted), "Cooked asset payload corruption was not detected");
    for (size_t size = 0; size < encoded->size(); ++size)
        expect(!gloom::assets::decode_cooked_asset(gloom::Span<const std::byte>{*encoded}.first(size)), "Truncated asset envelope accepted");
    for (size_t size = 0; size < scene_asset->payload.size(); ++size)
        expect(!gloom::assets::decode_imported_scene(gloom::Span<const std::byte>{scene_asset->payload}.first(size)), "Truncated scene accepted");
    gloom::assets::ImportedScene unskinned_cycle = *scene;
    unskinned_cycle.nodes[0].children.push_back(0);
    expect(!gloom::assets::validate_imported_scene(unskinned_cycle), "Unskinned cycle accepted");
    const std::vector<std::byte> empty_scene = gloom::assets::encode_imported_scene({});
    expect(gloom::assets::decode_imported_scene(empty_scene).has_value(), "Empty owned scene changed format");
    const gloom::assets::ImportedNode default_node;
    expect(default_node.local_transform[0] == 1 && default_node.local_transform[5] == 1 && default_node.local_transform[10] == 1 &&
               default_node.local_transform[15] == 1 && default_node.local_transform[12] == 0,
        "Default node transform is not identity");
    std::vector<std::byte> invalid_counts = empty_scene;
    invalid_counts[16] = std::byte{1};
    expect(!gloom::assets::decode_imported_scene(invalid_counts), "Scene reserved elements absent from its payload");

    gloom::assets::AssetCatalog catalog;
    for (const gloom::assets::AssetRecord& dependency : cooked->dependencies)
        catalog.add(dependency);
    catalog.add(cooked->scene);
    gloom::Array<gloom::assets::AssetId> dependency_order;
    const char* dependency_error = nullptr;
    expect(catalog.dependency_order(dependency_order, dependency_error) && dependency_order[dependency_order.size() - 1] == cooked->scene.id,
        "Cooked scene dependency was not ordered before the scene");

    const std::expected<std::vector<std::byte>, std::string> texture_encoded = filesystem.read(cooked->dependencies.front().cooked);
    const std::expected<gloom::assets::CookedAsset, std::string> texture_asset =
        texture_encoded ? gloom::assets::decode_cooked_asset(*texture_encoded)
                        : std::expected<gloom::assets::CookedAsset, std::string>{std::unexpected{"Missing cooked texture"}};
    expect(texture_asset && texture_asset->type == gloom::assets::AssetType::texture, "Cooked texture envelope is invalid");
    const std::expected<gloom::render::TextureUpload, std::string> texture_upload =
        texture_asset ? gloom::assets::decode_texture_ktx2({.value = texture_asset->id.value}, texture_asset->payload)
                      : std::expected<gloom::render::TextureUpload, std::string>{std::unexpected{"Missing texture payload"}};
    expect(texture_upload && texture_upload->srgb && texture_upload->mip_levels.size() == 2 && texture_upload->mip_levels[0].width == 2 &&
               texture_upload->mip_levels[1].width == 1,
        "KTX2 texture did not preserve dimensions, color space and mip chain");
    const std::expected<std::vector<std::byte>, std::string> source_texture = filesystem.read(*gloom::assets::VirtualPath::parse("game:/models/albedo.bmp"));
    const std::expected<std::vector<std::byte>, std::string> normal_payload =
        source_texture ? gloom::assets::cook_texture_ktx2(*source_texture, gloom::assets::TextureSemantic::normal)
                       : std::expected<std::vector<std::byte>, std::string>{std::unexpected{"Missing normal-map fixture"}};
    const std::expected<gloom::render::TextureUpload, std::string> normal_upload =
        normal_payload ? gloom::assets::decode_texture_ktx2({.value = 0x44}, *normal_payload)
                       : std::expected<gloom::render::TextureUpload, std::string>{std::unexpected{"Missing normal-map payload"}};
    expect(normal_upload && !normal_upload->srgb && normal_upload->mip_levels.size() == 2, "Normal-map KTX2 did not use linear UASTC data with mipmaps");
    const std::expected<gloom::render::TextureUpload, std::string> bc7_upload =
        gloom::assets::decode_texture_ktx2({.value = 0x46}, texture_asset->payload, gloom::assets::TextureTranscodeTarget::bc7);
    const std::expected<gloom::render::TextureUpload, std::string> bc5_upload =
        normal_payload ? gloom::assets::decode_texture_ktx2({.value = 0x47}, *normal_payload, gloom::assets::TextureTranscodeTarget::bc5)
                       : std::expected<gloom::render::TextureUpload, std::string>{std::unexpected{"Missing normal-map payload"}};
    expect(bc7_upload && bc7_upload->format == gloom::render::TextureFormat::bc7 && bc7_upload->mip_levels.front().data.size() == 16 && bc5_upload &&
               bc5_upload->format == gloom::render::TextureFormat::bc5 && !bc5_upload->srgb && bc5_upload->mip_levels.front().data.size() == 16,
        "KTX2 did not transcode directly to native BC7/BC5 GPU blocks");
    // Bottom-up BMP: cyan/white, then red/green. R != G in the 1x1 mip too.
    const gloom::uint8 expected_xy[5][2]{{0, 255}, {255, 255}, {255, 0}, {0, 255}, {128, 191}};
    for (gloom::uint32 mip = 0; mip < 2; ++mip) {
        for (gloom::uint32 pixel = 0; pixel < (mip == 0 ? 4U : 1U); ++pixel) {
            for (gloom::uint32 channel = 0; channel < 2; ++channel) {
                const int expected = expected_xy[mip == 0 ? pixel : 4][channel];
                const int rgba = static_cast<gloom::uint8>(normal_upload->mip_levels[mip].data[pixel * 4 + channel]);
                expect(rgba >= expected - 4 && rgba <= expected + 4, "Normal XY changed in RGBA8 fallback or its mip chain");
                const gloom::uint8* block = reinterpret_cast<const gloom::uint8*>(bc5_upload->mip_levels[mip].data.data()) + channel * 8;
                gloom::uint64 selectors = 0;
                for (gloom::uint32 byte = 0; byte < 6; ++byte)
                    selectors |= static_cast<gloom::uint64>(block[byte + 2]) << (byte * 8);
                const gloom::uint32 selector = (selectors >> (((pixel / 2) * 4 + pixel % 2) * 3)) & 7U;
                const int bc5 = selector < 2          ? block[selector]
                                : block[0] > block[1] ? ((8 - selector) * block[0] + (selector - 1) * block[1]) / 7
                                : selector < 6        ? ((6 - selector) * block[0] + (selector - 1) * block[1]) / 5
                                : selector == 6       ? 0
                                                      : 255;
                expect(bc5 >= expected - 4 && bc5 <= expected + 4, "Normal XY changed in BC5 or its mip chain");
            }
        }
    }
    expect(!gloom::assets::decode_texture_ktx2({.value = 0x45}, {}), "Empty KTX2 payload was accepted");

    gloom::core::JobSystem jobs{{.worker_threads = 1, .queue_capacity = 1}};
    expect(jobs.start() == nullptr, "JobSystem initialization failed");
    {
        gloom::assets::AsyncAssetLoader loader{jobs, filesystem, catalog};
        const std::shared_future<gloom::assets::AssetLoadResult> scene_future = loader.request(cooked->scene.id);
        const std::shared_future<gloom::assets::AssetLoadResult> cached_future = loader.request(cooked->scene.id);
        const std::shared_future<gloom::assets::AssetLoadResult> missing_future = loader.request({.value = 999'999});
        loader.wait();
        const gloom::assets::AssetLoadResult& loaded = scene_future.get();
        const gloom::assets::AssetLoadResult& cached = cached_future.get();
        const gloom::assets::AssetLoadResult& missing = missing_future.get();
        expect(loaded.state == gloom::assets::AssetLoadState::ready && cached.state == gloom::assets::AssetLoadState::ready &&
                   missing.state == gloom::assets::AssetLoadState::error_placeholder && !missing.error.empty() && !missing.asset.payload.empty(),
            "Asynchronous loader did not return ready and placeholder assets "
            "correctly");
        const gloom::assets::AssetLoaderMetrics metrics = loader.metrics();
        expect(metrics.requests == 3 && metrics.cache_hits == 1 && metrics.loaded == 1 && metrics.failed == 1 &&
                   metrics.bytes_loaded == scene_asset->payload.size(),
            "Asynchronous asset loader metrics are incorrect");

        ImmediateRenderer renderer;

        {
            gloom::assets::AssetResidencyCoordinator coordinator{jobs, loader, catalog,
                {.context = &renderer,
                    .texture_compression_bc = ImmediateRenderer::compression_bc,
                    .mesh = ImmediateRenderer::mesh,
                    .texture = ImmediateRenderer::texture,
                    .material = ImmediateRenderer::material,
                    .release = ImmediateRenderer::release,
                    .state = ImmediateRenderer::state},
                {.new_scene_requests_per_update = 1}};
            const gloom::assets::SceneTicket background = coordinator.request_scene({.value = 999'998}, gloom::assets::AssetPriority::background);
            const gloom::assets::SceneTicket critical = coordinator.request_scene(cooked->scene.id, gloom::assets::AssetPriority::critical);
            coordinator.update();
            expect(coordinator.state(background) == gloom::assets::SceneResidencyState::queued &&
                       coordinator.state(critical) != gloom::assets::SceneResidencyState::queued,
                "Scene request priority was not respected");
            coordinator.cancel(background);
            expect(coordinator.state(background) == gloom::assets::SceneResidencyState::cancelled, "Queued scene cancellation failed");

            wait_until(coordinator, critical, gloom::assets::SceneResidencyState::ready);
            const gloom::assets::ResidentScene* resident = coordinator.scene(critical);
            expect(resident != nullptr && resident->generation == 1 && resident->instances.size() == 1 &&
                       resident->instances.front().transform.position.x == 2.0F && renderer.mesh_uploads == 1 && renderer.texture_uploads == 1 &&
                       renderer.material_uploads == 1,
                "Imported scene hierarchy or GPU dependencies were not instantiated");

            gloom::assets::ImportedScene changed_scene = *scene;
            changed_scene.nodes.front().local_transform[12] = 4.0F;
            std::vector<std::byte> changed_payload = gloom::assets::encode_imported_scene(changed_scene);
            gloom::assets::CookedAsset changed_asset = *scene_asset;
            changed_asset.payload = std::move(changed_payload);
            changed_asset.source_fingerprint = gloom::assets::fingerprint(changed_asset.payload);
            gloom::assets::AssetRecord changed_record = cooked->scene;
            changed_record.source_fingerprint = changed_asset.source_fingerprint;
            catalog.upsert(changed_record);
            const std::vector<std::byte> changed_envelope = gloom::assets::encode_cooked_asset(changed_asset);
            const std::expected<void, std::string> write_result = filesystem.write(output, changed_envelope);
            expect(write_result.has_value(), "Could not replace the cooked scene fixture");

            coordinator.reload(critical);
            wait_until(coordinator, critical, gloom::assets::SceneResidencyState::ready);
            resident = coordinator.scene(critical);
            expect(resident != nullptr && resident->generation == 2 && resident->instances.front().transform.position.x == 4.0F &&
                       loader.metrics().invalidations >= 2,
                "Hot reload did not replace the resident scene generation");

            const gloom::assets::SceneTicket shared = coordinator.request_scene(cooked->scene.id, gloom::assets::AssetPriority::high);
            wait_until(coordinator, shared, gloom::assets::SceneResidencyState::ready);
            expect(coordinator.metrics().shared_resource_hits >= 3 && renderer.mesh_uploads == 2 && renderer.texture_uploads == 2 &&
                       renderer.material_uploads == 2,
                "Resident GPU resources were uploaded again instead of shared");
            const gloom::uint32 releases_before_cancel = renderer.releases;
            coordinator.cancel(critical);
            expect(renderer.releases == releases_before_cancel, "Cancelling one scene released resources still used by another");
            coordinator.cancel(shared);
            expect(renderer.releases == releases_before_cancel + 3, "Last scene reference did not release its GPU resources");

            const gloom::assets::ResidencyCoordinatorMetrics coordinator_metrics = coordinator.metrics();
            expect(
                coordinator_metrics.requested == 3 && coordinator_metrics.cancelled == 3 && coordinator_metrics.reloaded == 1 && coordinator_metrics.ready == 3,
                "Residency coordinator metrics are incorrect");

            {
                PausedWorker pause{jobs};
                const gloom::assets::SceneTicket cancelled = coordinator.request_scene(cooked->scene.id);
                coordinator.update();
                expect(coordinator.state(cancelled) == gloom::assets::SceneResidencyState::preparing, "Preparation was not held in the queue");
                coordinator.cancel(cancelled);
                coordinator.reload(cancelled);
                // Capacity one forces execution of the discarded generation. Reload also
                // invalidates cached files, so explicitly assist those reads before preparation.
                for (gloom::uint32 attempt = 0; attempt < 10 && coordinator.state(cancelled) != gloom::assets::SceneResidencyState::preparing; ++attempt) {
                    coordinator.update();
                    loader.wait();
                }
                expect(coordinator.state(cancelled) == gloom::assets::SceneResidencyState::preparing, "Reload did not enqueue a new preparation");
                coordinator.cancel(cancelled);
                const gloom::uint64 uploads_before = renderer.mesh_uploads + renderer.texture_uploads + renderer.material_uploads;
                pause.release();
                coordinator.update();
                expect(coordinator.state(cancelled) == gloom::assets::SceneResidencyState::cancelled && coordinator.scene(cancelled) == nullptr &&
                           renderer.mesh_uploads + renderer.texture_uploads + renderer.material_uploads == uploads_before,
                    "Discarded preparation published assets into a cancelled generation");
            }
            {
                PausedWorker pause{jobs};
                gloom::assets::AssetId last;
                for (gloom::uint32 index = 0; index < 100; ++index) {
                    gloom::assets::AssetRecord record = cooked->scene;
                    char path[96];
                    snprintf(path, sizeof(path), "game:/models/saturation-%u.gltf", index);
                    decltype(gloom::assets::VirtualPath::parse(path)) parsed = gloom::assets::VirtualPath::parse(path);
                    expect(parsed.has_value(), "Invalid saturation fixture path");
                    record.source = *parsed;
                    record.id = gloom::assets::make_asset_id(record.source, record.type);
                    last = record.id;
                    catalog.add(record);
                    static_cast<void>(loader.request(record.id));
                }
                pause.release();
                loader.wait();
                expect(loader.request(last).get().state == gloom::assets::AssetLoadState::error_placeholder,
                    "Invalid external asset lost its explicit result during saturation");
            }
        }
    }
    jobs.stop();

    std::string aliases;
    {
        const std::expected<std::vector<std::byte>, std::string> document = filesystem.read(source);
        expect(document.has_value(), "Read alias fixture");
        aliases.assign(reinterpret_cast<const char*>(document->data()), document->size());
    }
    const std::string_view image = "{\"uri\": \"albedo.bmp\", \"name\": \"Albedo\"}";
    aliases.replace(aliases.find(image), image.size(), "{\"uri\": \"albedo.bmp\", \"name\": \"Albedo\"}, {\"uri\": \"./albedo.bmp\"}");
    expect(filesystem.write(source, {reinterpret_cast<const std::byte*>(aliases.data()), aliases.size()}).has_value(), "Write alias fixture");
    const std::expected<gloom::assets::GltfCookResult, std::string> alias_cook = gloom::assets::cook_gltf(filesystem, source, output);
    expect(alias_cook && alias_cook->dependencies.size() == 1 && gloom::assets::discover_cooked_scene(filesystem, source, output).has_value(),
        "Canonical image aliases created duplicate dependencies");
    const std::string_view texture = "\"textures\": [{\"source\": 0}]";
    aliases.replace(aliases.find(texture), texture.size(), "\"textures\": [{\"source\": 0}, {\"source\": 1}]");
    const std::string_view material = "\"name\": \"Copper\",";
    std::string used_aliases = aliases;
    used_aliases.replace(used_aliases.find(material), material.size(), "\"name\": \"Copper\", \"emissiveTexture\": {\"index\": 1},");
    expect(filesystem.write(source, {reinterpret_cast<const std::byte*>(used_aliases.data()), used_aliases.size()}).has_value(), "Write used alias fixture");
    expect(gloom::assets::cook_gltf(filesystem, source, output).has_value(), "Cook color aliases");
    std::expected<gloom::assets::DiscoveredSceneCatalog, std::string> used_catalog = gloom::assets::discover_cooked_scene(filesystem, source, output);
    expect(used_catalog.has_value(), "Discover color aliases");
    expect(jobs.start() == nullptr, "Restart asset workers");
    {
        gloom::assets::AsyncAssetLoader loader{jobs, filesystem, used_catalog->catalog};
        ImmediateRenderer renderer;
        renderer.check_alias = true;
        gloom::assets::AssetResidencyCoordinator coordinator{jobs, loader, used_catalog->catalog,
            {.context = &renderer,
                .texture_compression_bc = ImmediateRenderer::compression_bc,
                .mesh = ImmediateRenderer::mesh,
                .texture = ImmediateRenderer::texture,
                .material = ImmediateRenderer::material,
                .release = ImmediateRenderer::release,
                .state = ImmediateRenderer::state}};
        const gloom::assets::SceneTicket ticket = coordinator.request_scene(used_catalog->scene);
        wait_until(coordinator, ticket, gloom::assets::SceneResidencyState::ready);
        expect(renderer.texture_uploads == 1, "Color alias uploaded the texture twice");
    }
    jobs.stop();
    aliases.replace(aliases.find(material), material.size(), "\"name\": \"Copper\", \"normalTexture\": {\"index\": 1},");
    expect(filesystem.write(source, {reinterpret_cast<const std::byte*>(aliases.data()), aliases.size()}).has_value(), "Write conflicting alias fixture");
    expect(!gloom::assets::cook_gltf(filesystem, source, output), "Incompatible alias semantics silently replaced a cooked texture");
}

} // namespace

int main(int argc, const char* const* argv) {
#ifdef _DEBUG
    if (argc == 2) {
        _set_error_mode(_OUT_TO_STDERR);
        _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
        _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
        const decltype(gloom::assets::VirtualPath::parse("game:/a")) path = gloom::assets::VirtualPath::parse("game:/a");
        gloom::assets::AssetCatalog catalog;
        gloom::assets::AssetRecord record{.id = gloom::assets::make_asset_id(*path, gloom::assets::AssetType::binary), .source = *path, .cooked = *path};
        if (strcmp(argv[1], "invalid-scene") == 0) {
            gloom::assets::ImportedScene invalid;
            invalid.default_scene = 1;
            static_cast<void>(gloom::assets::encode_imported_scene(invalid));
            return 0;
        }
        if (strcmp(argv[1], "duplicate") == 0)
            catalog.add(record);
        else if (strcmp(argv[1], "self") == 0)
            record.dependencies = {record.id};
        else if (strcmp(argv[1], "wrong-id") == 0)
            record.id.value = 0;
        else if (strcmp(argv[1], "duplicate-dependency") == 0)
            record.dependencies = {{.value = 1}, {.value = 1}};
        catalog.add(record);
        return 0;
    }
    char executable[MAX_PATH];
    expect(GetModuleFileNameA(nullptr, executable, MAX_PATH) > 0, "Locate catalog precondition fixture");
    const char* modes[]{"duplicate", "self", "wrong-id", "duplicate-dependency", "invalid-scene"};
    for (const char* mode : modes) {
        char command[MAX_PATH + 64];
        snprintf(command, sizeof(command), "\"%s\" %s", executable, mode);
        STARTUPINFOA startup{.cb = sizeof(startup)};
        PROCESS_INFORMATION process{};
        expect(CreateProcessA(nullptr, command, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process), "Start catalog fixture");
        expect(WaitForSingleObject(process.hProcess, 5000) == WAIT_OBJECT_0, "Catalog assert hung");
        DWORD result = 0;
        GetExitCodeProcess(process.hProcess, &result);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        expect(result == 3, "Invalid catalog record did not assert");
    }
#else
    static_cast<void>(argc);
    static_cast<void>(argv);
#endif
    test_virtual_paths_and_catalog();
    test_offline_mesh_processing();
    test_project_mesh_orientation();
    test_gltf_cooking_and_async_loading();
    std::cout << "Gloom asset pipeline tests completed successfully.\n";
    return 0;
}
