#include <gloom/core/allocation_profile.hpp>
#include <gloom/assets/residency_coordinator.hpp>

#include <gloom/assets/gltf_importer.hpp>
#include <gloom/assets/scene_gpu_bridge.hpp>
#include <gloom/assets/texture_asset.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <math.h>
#include <expected>
#include <future>
#include <limits>
#include <memory>
#include <stdlib.h>
#include <unordered_set>

namespace gloom::assets {
namespace {

using Matrix = Matrix4;

[[nodiscard]] Matrix multiply(const Matrix& left, const Matrix& right) noexcept {
    Matrix result{};
    for (size_t column = 0; column < 4; ++column) {
        for (size_t row = 0; row < 4; ++row) {
            for (size_t inner = 0; inner < 4; ++inner) {
                result[column * 4 + row] += left[inner * 4 + row] * right[column * 4 + inner];
            }
        }
    }
    return result;
}

[[nodiscard]] Matrix identity_matrix() noexcept {
    return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
}

[[nodiscard]] render::Transform decompose(const Matrix& matrix) noexcept {
    float scale_x = sqrtf(matrix[0] * matrix[0] + matrix[1] * matrix[1] + matrix[2] * matrix[2]);
    const float scale_y = sqrtf(matrix[4] * matrix[4] + matrix[5] * matrix[5] + matrix[6] * matrix[6]);
    const float scale_z = sqrtf(matrix[8] * matrix[8] + matrix[9] * matrix[9] + matrix[10] * matrix[10]);
    const float determinant = matrix[0] * (matrix[5] * matrix[10] - matrix[6] * matrix[9]) - matrix[4] * (matrix[1] * matrix[10] - matrix[2] * matrix[9]) +
                              matrix[8] * (matrix[1] * matrix[6] - matrix[2] * matrix[5]);
    if (determinant < 0.0F) {
        scale_x = -scale_x;
    }
    const float inverse_x = fabsf(scale_x) > 1.0e-7F ? 1.0F / scale_x : 0.0F;
    const float inverse_y = scale_y > 1.0e-7F ? 1.0F / scale_y : 0.0F;
    const float inverse_z = scale_z > 1.0e-7F ? 1.0F / scale_z : 0.0F;
    const float m00 = matrix[0] * inverse_x;
    const float m01 = matrix[4] * inverse_y;
    const float m02 = matrix[8] * inverse_z;
    const float m10 = matrix[1] * inverse_x;
    const float m11 = matrix[5] * inverse_y;
    const float m12 = matrix[9] * inverse_z;
    const float m20 = matrix[2] * inverse_x;
    const float m21 = matrix[6] * inverse_y;
    const float m22 = matrix[10] * inverse_z;
    render::Quaternion rotation;
    const float trace = m00 + m11 + m22;
    if (trace > 0.0F) {
        const float factor = sqrtf(trace + 1.0F) * 2.0F;
        rotation = {(m21 - m12) / factor, (m02 - m20) / factor, (m10 - m01) / factor, 0.25F * factor};
    } else if (m00 > m11 && m00 > m22) {
        const float factor = sqrtf(1.0F + m00 - m11 - m22) * 2.0F;
        rotation = {0.25F * factor, (m01 + m10) / factor, (m02 + m20) / factor, (m21 - m12) / factor};
    } else if (m11 > m22) {
        const float factor = sqrtf(1.0F + m11 - m00 - m22) * 2.0F;
        rotation = {(m01 + m10) / factor, 0.25F * factor, (m12 + m21) / factor, (m02 - m20) / factor};
    } else {
        const float factor = sqrtf(1.0F + m22 - m00 - m11) * 2.0F;
        rotation = {(m02 + m20) / factor, (m12 + m21) / factor, 0.25F * factor, (m10 - m01) / factor};
    }
    return {
        .position = {matrix[12], matrix[13], matrix[14]},
        .rotation = rotation,
        .scale = {scale_x, scale_y, scale_z},
    };
}

[[nodiscard]] bool path_ends_with(std::string_view path, const std::string& uri) {
    if (path.size() < uri.size())
        return false;
    for (size_t index = 0; index < uri.size(); ++index)
        if (path[path.size() - uri.size() + index] != (uri[index] == '\\' ? '/' : uri[index]))
            return false;
    return true;
}
int compare_resource(const void* left, const void* right) {
    const uint64 a = static_cast<const render::RenderAssetId*>(left)->value;
    const uint64 b = static_cast<const render::RenderAssetId*>(right)->value;
    return a < b ? -1 : a > b ? 1 : 0;
}

} // namespace

struct AssetResidencyCoordinator::PreparedScene {
    GpuSceneUploads gpu;
    std::vector<render::TextureUpload> textures;
    std::vector<render::RenderInstance> instances;
    std::vector<render::RenderAssetId> resources;
    std::shared_ptr<const ImportedScene> bind_rig;
};

struct AssetResidencyCoordinator::Request {
    AssetId asset;
    AssetPriority priority{AssetPriority::normal};
    SceneResidencyState state{SceneResidencyState::queued};
    std::uint64_t generation{1};
    std::shared_future<AssetLoadResult> scene_future;
    std::vector<std::shared_future<AssetLoadResult>> dependency_futures;
    std::shared_future<std::expected<PreparedScene, std::string>> preparation_future;
    std::vector<render::RenderAssetId> resources;
    std::optional<ResidentScene> resident;
    std::string error;
};

// Preparation owns decoded data independently of Request. Reload/cancel can discard
// the future while the context finishes; completion never reads or publishes into Request.
// ImportedScene/GpuSceneUploads and cached futures keep their legacy representation until 117/118.
struct AssetResidencyCoordinator::PreparationContext {
    ImportedScene scene;
    std::vector<CookedAsset> dependencies;
    std::vector<AssetRecord> dependency_records;
    AssetId scene_id;
    bool use_bc{false};
    std::promise<std::expected<PreparedScene, std::string>> promise;

    bool matches(uint32 texture, Span<const uint8> images) const {
        return texture < scene.textures.size() && scene.textures[texture].image < images.size() && images[scene.textures[texture].image];
    }
    const char* visit(uint32 index, const Matrix& parent, PreparedScene& prepared, Span<uint8> visiting) const {
        if (index >= scene.nodes.size() || visiting[index])
            return "Scene node hierarchy contains a cycle";
        visiting[index] = 1;
        const ImportedNode& node = scene.nodes[index];
        const Matrix world = multiply(parent, node.local_transform);
        if (node.mesh != no_asset_index) {
            if (node.mesh >= scene.meshes.size())
                return "Scene node references an invalid mesh";
            const ImportedMesh& mesh = scene.meshes[node.mesh];
            for (uint32 primitive = 0; primitive < mesh.primitive_count; ++primitive) {
                const uint32 primitive_index = mesh.first_primitive + primitive;
                if (primitive_index >= prepared.gpu.primitives.size())
                    return "Scene mesh range is invalid";
                const GpuScenePrimitive& binding = prepared.gpu.primitives[primitive_index];
                prepared.instances.push_back({.mesh = binding.mesh,
                    .material = binding.material,
                    .transform = decompose(world),
                    .local_bounds = binding.bounds,
                    .lod_meshes = binding.lod_meshes,
                    .lod_count = binding.lod_count,
                    .source_node = index,
                    .source_primitive = primitive_index,
                    .arms_mesh = binding.arms_mesh});
            }
        }
        for (uint32 child : node.children)
            if (const char* error = visit(child, world, prepared, visiting))
                return error;
        visiting[index] = 0;
        return nullptr;
    }
    void operator()() noexcept {
        PreparedScene prepared;
        Array<render::RenderAssetId> image_assets;
        Array<uint8> matched, visiting;
        image_assets.reserve(scene.images.size());
        image_assets.resize(scene.images.size());
        matched.reserve(scene.images.size());
        matched.resize(scene.images.size());
        for (size_t dependency = 0; dependency < dependencies.size(); ++dependency) {
            const render::RenderAssetId gpu_id{dependencies[dependency].id.value};
            for (size_t image = 0; image < scene.images.size(); ++image) {
                matched[image] = path_ends_with(dependency_records[dependency].source.relative(), scene.images[image].external_uri);
                if (matched[image])
                    image_assets[image] = gpu_id;
            }
            bool used_as_normal = false, used_as_color_or_data = false;
            for (const ImportedMaterial& material : scene.materials) {
                used_as_normal = used_as_normal || matches(material.normal_texture, {matched.data(), matched.size()});
                used_as_color_or_data = used_as_color_or_data || matches(material.base_color_texture, {matched.data(), matched.size()}) ||
                                        matches(material.metallic_roughness_texture, {matched.data(), matched.size()});
                for (uint32 texture : material.extra_textures)
                    used_as_color_or_data = used_as_color_or_data || matches(texture, {matched.data(), matched.size()});
            }
            const TextureTranscodeTarget target =
                use_bc ? used_as_normal && !used_as_color_or_data ? TextureTranscodeTarget::bc5 : TextureTranscodeTarget::bc7 : TextureTranscodeTarget::rgba8;
            decltype(decode_texture_ktx2(gpu_id, dependencies[dependency].payload, target)) upload =
                decode_texture_ktx2(gpu_id, dependencies[dependency].payload, target);
            if (!upload) {
                promise.set_value(std::unexpected{upload.error()});
                return;
            }
            prepared.resources.push_back(gpu_id);
            prepared.textures.push_back(static_cast<render::TextureUpload&&>(*upload));
        }
        prepared.gpu = build_gpu_scene_uploads(scene, scene_id, {image_assets.data(), image_assets.size()});
        for (const render::MeshUpload& mesh : prepared.gpu.meshes)
            prepared.resources.push_back(mesh.id);
        for (const render::MaterialUpload& material : prepared.gpu.materials)
            prepared.resources.push_back(material.id);
        const uint32 scene_index = scene.default_scene == no_asset_index ? 0 : scene.default_scene;
        if (!scene.scenes.empty() && scene_index >= scene.scenes.size()) {
            promise.set_value(std::unexpected{"Scene selects an invalid root set"});
            return;
        }
        visiting.reserve(scene.nodes.size());
        visiting.resize(scene.nodes.size());
        if (!scene.scenes.empty())
            for (uint32 root : scene.scenes[scene_index].roots)
                if (const char* error = visit(root, identity_matrix(), prepared, {visiting.data(), visiting.size()})) {
                    promise.set_value(std::unexpected{error});
                    return;
                }
        if (prepared.resources.size() > 1)
            qsort(prepared.resources.data(), prepared.resources.size(), sizeof(render::RenderAssetId), compare_resource);
        size_t count = 0;
        for (size_t index = 0; index < prepared.resources.size(); ++index)
            if (!count || prepared.resources[index] != prepared.resources[count - 1])
                prepared.resources[count++] = prepared.resources[index];
        prepared.resources.resize(count);
        if (!scene.skins.empty() || !scene.animations.empty())
            prepared.bind_rig = std::make_shared<ImportedScene>(static_cast<ImportedScene&&>(scene));
        promise.set_value(static_cast<PreparedScene&&>(prepared));
    }
};

AssetResidencyCoordinator::AssetResidencyCoordinator(
    core::JobSystem& jobs, AsyncAssetLoader& loader, const AssetCatalog& catalog, render::Renderer& renderer, const ResidencyCoordinatorSettings settings)
    : jobs_{jobs}, loader_{loader}, catalog_{catalog}, renderer_{renderer}, settings_{settings}, preparation_tasks_{jobs.create_group()} {}

AssetResidencyCoordinator::~AssetResidencyCoordinator() {
    if (preparation_tasks_.valid()) {
        jobs_.wait(preparation_tasks_);
    }
    for (auto& [ticket, request] : requests_) {
        static_cast<void>(ticket);
        release_resources(request);
    }
}

SceneTicket AssetResidencyCoordinator::request_scene(const AssetId scene, const AssetPriority priority) {
    const SceneTicket ticket{next_ticket_++};
    requests_.emplace(ticket.value, Request{.asset = scene, .priority = priority});
    ++metrics_.requested;
    return ticket;
}

void AssetResidencyCoordinator::cancel(const SceneTicket ticket) {
    const auto found = requests_.find(ticket.value);
    if (found == requests_.end() || found->second.state == SceneResidencyState::cancelled) {
        return;
    }
    release_resources(found->second);
    found->second.resident.reset();
    found->second.state = SceneResidencyState::cancelled;
    ++found->second.generation;
    ++metrics_.cancelled;
}

void AssetResidencyCoordinator::reload(const SceneTicket ticket) {
    const auto found = requests_.find(ticket.value);
    if (found == requests_.end()) {
        return;
    }
    Request& request = found->second;
    release_resources(request);
    if (const auto* record = catalog_.find(request.asset); record != nullptr) {
        static_cast<void>(loader_.invalidate(request.asset));
        for (const auto dependency : record->dependencies) {
            static_cast<void>(loader_.invalidate(dependency));
        }
    }
    request.scene_future = {};
    request.dependency_futures.clear();
    request.preparation_future = {};
    request.resident.reset();
    request.error.clear();
    request.state = SceneResidencyState::queued;
    ++request.generation;
    ++metrics_.reloaded;
}

void AssetResidencyCoordinator::release_resources(Request& request) {
    for (const auto resource : request.resources) {
        const auto found = resource_references_.find(resource);
        if (found == resource_references_.end()) {
            continue;
        }
        if (--found->second == 0) {
            renderer_.release(resource);
            resource_references_.erase(found);
        }
    }
    request.resources.clear();
}

void AssetResidencyCoordinator::fail(Request& request, std::string error) {
    release_resources(request);
    request.error = std::move(error);
    request.state = SceneResidencyState::failed;
    ++metrics_.failed;
}

void AssetResidencyCoordinator::update() {
    GLOOM_PROFILE_SCOPE(::gloom::AllocationPhase::assets);
    std::vector<std::pair<std::uint64_t, Request*>> queued;
    for (auto& [ticket, request] : requests_) {
        if (request.state == SceneResidencyState::queued) {
            queued.emplace_back(ticket, &request);
        }
    }
    std::ranges::sort(queued, [](const auto& left, const auto& right) {
        if (left.second->priority != right.second->priority) {
            return left.second->priority > right.second->priority;
        }
        return left.first < right.first;
    });
    const auto dispatch_count = std::min<std::size_t>(queued.size(), settings_.new_scene_requests_per_update);
    for (std::size_t index = 0; index < dispatch_count; ++index) {
        Request& request = *queued[index].second;
        request.scene_future = loader_.request(request.asset);
        request.state = SceneResidencyState::loading_scene;
        ++metrics_.dispatched;
    }

    for (auto& [ticket, request] : requests_) {
        static_cast<void>(ticket);
        if (request.state == SceneResidencyState::loading_scene && request.scene_future.wait_for(std::chrono::seconds{0}) == std::future_status::ready) {
            const auto result = request.scene_future.get();
            if (result.state != AssetLoadState::ready || result.asset.type != AssetType::scene) {
                fail(request, result.error.empty() ? "Scene asset failed to load" : result.error);
                continue;
            }
            request.dependency_futures.clear();
            request.dependency_futures.reserve(result.asset.dependencies.size());
            for (const auto dependency : result.asset.dependencies) {
                request.dependency_futures.push_back(loader_.request(dependency));
            }
            request.state = SceneResidencyState::loading_dependencies;
        }

        if (request.state == SceneResidencyState::loading_dependencies) {
            const bool ready = std::ranges::all_of(request.dependency_futures, [](const auto& future) {
                return future.wait_for(std::chrono::seconds{0}) == std::future_status::ready;
            });
            if (!ready) {
                continue;
            }
            const auto scene_result = request.scene_future.get();
            auto decoded_scene = decode_imported_scene(scene_result.asset.payload);
            if (!decoded_scene) {
                fail(request, decoded_scene.error());
                continue;
            }
            std::vector<CookedAsset> dependencies;
            std::vector<AssetRecord> dependency_records;
            bool dependency_error = false;
            for (std::size_t index = 0; index < request.dependency_futures.size(); ++index) {
                const auto result = request.dependency_futures[index].get();
                if (result.state != AssetLoadState::ready || result.asset.type != AssetType::texture) {
                    fail(request, result.error.empty() ? "Scene texture dependency failed" : result.error);
                    dependency_error = true;
                    break;
                }
                const auto* record = catalog_.find(result.asset.id);
                if (record == nullptr) {
                    fail(request, "Scene dependency disappeared from the catalog");
                    dependency_error = true;
                    break;
                }
                dependencies.push_back(result.asset);
                dependency_records.push_back(*record);
            }
            if (dependency_error) {
                continue;
            }
            PreparationContext* context = new PreparationContext{.scene = static_cast<ImportedScene&&>(*decoded_scene),
                .dependencies = static_cast<decltype(dependencies)&&>(dependencies),
                .dependency_records = static_cast<decltype(dependency_records)&&>(dependency_records),
                .scene_id = request.asset,
                .use_bc = renderer_.capabilities().texture_compression_bc};
            request.preparation_future = context->promise.get_future().share();
            jobs_.schedule(preparation_tasks_, core::JobSystem::Job{context});
            request.state = SceneResidencyState::preparing;
        }

        if (request.state == SceneResidencyState::preparing && request.preparation_future.wait_for(std::chrono::seconds{0}) == std::future_status::ready) {
            auto prepared = request.preparation_future.get();
            if (!prepared) {
                fail(request, prepared.error());
                continue;
            }
            request.resident = ResidentScene{
                .asset = request.asset,
                .generation = request.generation,
                .instances = std::move(prepared->instances),
                .bind_rig = std::move(prepared->bind_rig),
            };
            if (request.resident->bind_rig && !prepare_animation_rig(*request.resident->bind_rig, request.resident->animation_rig, request.generation)) {
                fail(request, "Invalid animation rig");
                continue;
            }
            request.resources = prepared->resources;
            for (const auto resource : request.resources) {
                auto [reference, inserted] = resource_references_.try_emplace(resource, 0);
                ++reference->second;
                if (!inserted) {
                    ++metrics_.shared_resource_hits;
                }
            }
            const auto is_first_reference = [&](const render::RenderAssetId id) {
                return resource_references_.at(id) == 1;
            };
            for (auto& texture : prepared->textures) {
                if (is_first_reference(texture.id)) {
                    renderer_.enqueue(std::move(texture));
                }
            }
            for (auto& mesh : prepared->gpu.meshes) {
                if (is_first_reference(mesh.id)) {
                    renderer_.enqueue(std::move(mesh));
                }
            }
            for (auto& material : prepared->gpu.materials) {
                if (is_first_reference(material.id)) {
                    renderer_.enqueue(std::move(material));
                }
            }
            request.state = SceneResidencyState::uploading;
        }

        if (request.state == SceneResidencyState::uploading) {
            bool all_resident = true;
            for (const auto resource : request.resources) {
                const auto state = renderer_.asset_state(resource);
                if (state == render::GpuAssetState::failed) {
                    fail(request, "Renderer rejected a scene GPU resource");
                    all_resident = false;
                    break;
                }
                all_resident = all_resident && state == render::GpuAssetState::resident;
            }
            if (all_resident && request.state == SceneResidencyState::uploading) {
                request.state = SceneResidencyState::ready;
                ++metrics_.ready;
            }
        }
    }
}

SceneResidencyState AssetResidencyCoordinator::state(const SceneTicket ticket) const {
    const auto found = requests_.find(ticket.value);
    assert(found != requests_.end());
    return found->second.state;
}

const ResidentScene* AssetResidencyCoordinator::scene(const SceneTicket ticket) const noexcept {
    const auto found = requests_.find(ticket.value);
    return found == requests_.end() || found->second.state != SceneResidencyState::ready ? nullptr : &*found->second.resident;
}

std::string_view AssetResidencyCoordinator::error(const SceneTicket ticket) const noexcept {
    const auto found = requests_.find(ticket.value);
    return found == requests_.end() ? std::string_view{} : std::string_view{found->second.error};
}

ResidencyCoordinatorMetrics AssetResidencyCoordinator::metrics() const noexcept {
    return metrics_;
}

} // namespace gloom::assets
