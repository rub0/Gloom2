#include <gloom/core/allocation_profile.hpp>
#include <gloom/assets/residency_coordinator.hpp>
#include <gloom/assets/scene_gpu_bridge.hpp>
#include <gloom/assets/texture_asset.hpp>
#include <gloom/assets/scene_catalog.hpp>
#include <math.h>
#include <stdlib.h>

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

int compare_resource(const void* left, const void* right) {
    const uint64 a = static_cast<const render::RenderAssetId*>(left)->value;
    const uint64 b = static_cast<const render::RenderAssetId*>(right)->value;
    return a < b ? -1 : a > b ? 1 : 0;
}
}
struct AssetResidencyCoordinator::PreparedScene {
    GpuSceneUploads gpu;
    Array<render::TextureUpload> textures;
    Array<render::RenderInstance> instances;
    Array<render::RenderAssetId> resources;
    AnimationRig animation_rig;
    Array<char> error;
};
struct AssetResidencyCoordinator::Request {
    AssetId asset;
    AssetPriority priority{AssetPriority::normal};
    SceneResidencyState state{SceneResidencyState::queued};
    uint64 generation{1};
    AssetLoadHandle scene;
    Array<AssetLoadHandle> dependencies;
    PreparedScene* completed{nullptr}; // Only accessed under publication_.
    Array<render::RenderAssetId> resources;
    ResidentScene resident;
    Array<char> error;
};
struct AssetResidencyCoordinator::PreparationContext {
    AssetResidencyCoordinator* owner;
    Request* request;
    uint64 generation;
    AssetLoadHandle input;
    Array<AssetLoadHandle> dependencies;
    VirtualPath source;
    AssetId scene_id;
    bool use_bc{false};
    bool matches(const ImportedScene& scene, uint32 texture, Span<const uint8> images) const {
        return texture < scene.textures.size() && scene.textures[texture].image < images.size() && images[scene.textures[texture].image];
    }
    void prepare(PreparedScene& prepared) {
        // Immutable loader handles pin payloads through the complete job; no CookedAsset copies.
        decltype(decode_imported_scene(input.get().asset.payload)) decoded = decode_imported_scene(input.get().asset.payload);
        if (!decoded) {
            set_asset_error(prepared.error, decoded.error().c_str());
            return;
        }
        ImportedScene scene = static_cast<ImportedScene&&>(*decoded);
        Array<render::RenderAssetId> image_assets;
        Array<uint8> matched;
        Array<AssetId> image_ids;
        image_assets.reserve(scene.images.size());
        image_assets.resize(scene.images.size());
        matched.reserve(scene.images.size());
        matched.resize(scene.images.size());
        image_ids.reserve(scene.images.size());
        image_ids.resize(scene.images.size());
        for (size_t image = 0; image < scene.images.size(); ++image) {
            if (scene.images[image].external_uri.empty())
                continue;
            decltype(dependency_source_path(source, scene.images[image].external_uri)) path = dependency_source_path(source, scene.images[image].external_uri);
            if (!path) {
                set_asset_error(prepared.error, path.error().c_str());
                return;
            }
            image_ids[image] = make_asset_id(*path, AssetType::texture);
        }
        prepared.textures.reserve(dependencies.size());
        prepared.resources.reserve(dependencies.size() + scene.primitives.size() * 4 + scene.materials.size());
        // ponytail: linear matching during cold preparation; add an ID index only if large image fanout is measured.
        for (const AssetLoadHandle& dependency : dependencies) {
            const CookedAsset& asset = dependency.get().asset;
            const render::RenderAssetId gpu_id{.value = asset.id.value};
            for (size_t image = 0; image < scene.images.size(); ++image) {
                matched[image] = image_ids[image] == asset.id;
                if (matched[image])
                    image_assets[image] = gpu_id;
            }
            bool normal = false, other = false;
            for (const ImportedMaterial& material : scene.materials) {
                normal = normal || matches(scene, material.normal_texture, {matched.data(), matched.size()});
                other = other || matches(scene, material.base_color_texture, {matched.data(), matched.size()}) ||
                        matches(scene, material.metallic_roughness_texture, {matched.data(), matched.size()});
                for (uint32 texture : material.extra_textures)
                    other = other || matches(scene, texture, {matched.data(), matched.size()});
            }
            const TextureTranscodeTarget target =
                use_bc ? normal && !other ? TextureTranscodeTarget::bc5 : TextureTranscodeTarget::bc7 : TextureTranscodeTarget::rgba8;
            decltype(decode_texture_ktx2(gpu_id, asset.payload, target)) upload = decode_texture_ktx2(gpu_id, asset.payload, target);
            if (!upload) {
                set_asset_error(prepared.error, upload.error().c_str());
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
        const uint32 selected = scene.default_scene == no_asset_index ? 0 : scene.default_scene;
        if (!scene.scenes.empty()) {
            assert(selected < scene.scenes.size());
            struct Frame {
                uint32 node{0}, child{0};
                Matrix world;
                bool entered{false};
            };
            Array<Frame> stack;
            Array<uint8> visiting;
            stack.reserve(scene.nodes.size());
            visiting.reserve(scene.nodes.size());
            visiting.resize(scene.nodes.size());
            size_t instance_count = 0;
            for (const ImportedNode& node : scene.nodes)
                if (node.mesh != no_asset_index)
                    instance_count += scene.meshes[node.mesh].primitive_count;
            prepared.instances.reserve(instance_count);
            for (uint32 root : scene.scenes[selected].roots) {
                stack.push_back({.node = root, .world = identity_matrix()});
                while (stack.size()) {
                    Frame& frame = stack[stack.size() - 1];
                    if (frame.node >= scene.nodes.size() || (!frame.entered && visiting[frame.node])) {
                        set_asset_error(prepared.error, "Scene node hierarchy contains a cycle");
                        return;
                    }
                    const ImportedNode& node = scene.nodes[frame.node];
                    if (!frame.entered) {
                        frame.entered = true;
                        visiting[frame.node] = 1;
                        frame.world = multiply(frame.world, node.local_transform);
                        if (node.mesh != no_asset_index) {
                            const ImportedMesh& mesh = scene.meshes[node.mesh];
                            for (uint32 primitive = 0; primitive < mesh.primitive_count; ++primitive) {
                                const uint32 index = mesh.first_primitive + primitive;
                                const GpuScenePrimitive& binding = prepared.gpu.primitives[index];
                                render::RenderInstance instance{.mesh = binding.mesh,
                                    .material = binding.material,
                                    .transform = decompose(frame.world),
                                    .local_bounds = binding.bounds,
                                    .lod_count = binding.lod_count,
                                    .source_node = frame.node,
                                    .source_primitive = index,
                                    .arms_mesh = binding.arms_mesh};
                                for (size_t lod = 0; lod < 3; ++lod)
                                    instance.lod_meshes[lod] = binding.lod_meshes[lod];
                                prepared.instances.push_back(static_cast<render::RenderInstance&&>(instance));
                            }
                        }
                    }
                    if (frame.child < node.children.size()) {
                        const Frame child{.node = node.children[frame.child++], .world = frame.world};
                        stack.push_back(child);
                    } else {
                        visiting[frame.node] = 0;
                        stack.resize(stack.size() - 1);
                    }
                }
            }
        }
        if (prepared.resources.size() > 1)
            qsort(prepared.resources.data(), prepared.resources.size(), sizeof(render::RenderAssetId), compare_resource);
        size_t count = 0;
        for (size_t index = 0; index < prepared.resources.size(); ++index)
            if (!count || prepared.resources[index] != prepared.resources[count - 1])
                prepared.resources[count++] = prepared.resources[index];
        prepared.resources.resize(count);
        if (!scene.skins.empty() || !scene.animations.empty()) {
            if (!prepare_animation_rig(scene, prepared.animation_rig, generation)) {
                set_asset_error(prepared.error, "Invalid animation rig");
                return;
            }
        }
    }
    void operator()() noexcept {
        GLOOM_PROFILE_SCOPE(::gloom::AllocationPhase::assets);
        PreparedScene* prepared = new PreparedScene;
        prepare(*prepared);
        AcquireSRWLockExclusive(&owner->publication_);
        if (request->generation == generation) {
            assert(!request->completed);
            request->completed = prepared;
            prepared = nullptr;
        } else
            ++owner->metrics_.stale_preparations;
        ReleaseSRWLockExclusive(&owner->publication_);
        delete prepared;
    }
};
namespace {
bool renderer_compression(const void* context) {
    return static_cast<const render::Renderer*>(context)->capabilities().texture_compression_bc;
}
void renderer_mesh(void* context, render::MeshUpload upload) {
    static_cast<render::Renderer*>(context)->enqueue(static_cast<render::MeshUpload&&>(upload));
}
void renderer_texture(void* context, render::TextureUpload upload) {
    static_cast<render::Renderer*>(context)->enqueue(static_cast<render::TextureUpload&&>(upload));
}
void renderer_material(void* context, render::MaterialUpload upload) {
    static_cast<render::Renderer*>(context)->enqueue(static_cast<render::MaterialUpload&&>(upload));
}
void renderer_release(void* context, render::RenderAssetId id) {
    static_cast<render::Renderer*>(context)->release(id);
}
render::GpuAssetState renderer_state(const void* context, render::RenderAssetId id) {
    return static_cast<const render::Renderer*>(context)->asset_state(id);
}
}
AssetResidencyCoordinator::AssetResidencyCoordinator(
    core::JobSystem& jobs, AsyncAssetLoader& loader, const AssetCatalog& catalog, render::Renderer& renderer, ResidencyCoordinatorSettings settings)
    : AssetResidencyCoordinator(jobs, loader, catalog,
          {.context = &renderer,
              .texture_compression_bc = renderer_compression,
              .mesh = renderer_mesh,
              .texture = renderer_texture,
              .material = renderer_material,
              .release = renderer_release,
              .state = renderer_state},
          settings) {}
AssetResidencyCoordinator::AssetResidencyCoordinator(
    core::JobSystem& jobs, AsyncAssetLoader& loader, const AssetCatalog& catalog, AssetUploadSink renderer, ResidencyCoordinatorSettings settings)
    : jobs_{jobs}, loader_{loader}, catalog_{catalog}, renderer_{renderer}, settings_{settings}, preparation_tasks_{jobs.create_group()} {
    assert(renderer.texture_compression_bc && renderer.mesh && renderer.texture && renderer.material && renderer.release && renderer.state);
}
AssetResidencyCoordinator::~AssetResidencyCoordinator() {
    jobs_.wait(preparation_tasks_);
    for (Request* request : requests_) {
        release_resources(*request);
        delete request->completed;
        delete request;
    }
}
AssetResidencyCoordinator::Request* AssetResidencyCoordinator::find(SceneTicket ticket) const {
    return ticket.value && ticket.value <= requests_.size() ? requests_[ticket.value - 1] : nullptr;
}
SceneTicket AssetResidencyCoordinator::request_scene(AssetId scene, AssetPriority priority) {
    requests_.push_back(new Request{.asset = scene, .priority = priority});
    queued_.reserve(requests_.size());
    ++metrics_.requested;
    return {.value = requests_.size()};
}
void AssetResidencyCoordinator::discard(Request& request) {
    AcquireSRWLockExclusive(&publication_);
    ++request.generation;
    PreparedScene* completed = request.completed;
    request.completed = nullptr;
    ReleaseSRWLockExclusive(&publication_);
    delete completed;
    request.scene = {};
    request.dependencies.resize(0);
    request.resident = {};
}
void AssetResidencyCoordinator::cancel(SceneTicket ticket) {
    Request* request = find(ticket);
    if (!request || request->state == SceneResidencyState::cancelled)
        return;
    discard(*request);
    release_resources(*request);
    request->state = SceneResidencyState::cancelled;
    ++metrics_.cancelled;
}
void AssetResidencyCoordinator::reload(SceneTicket ticket) {
    Request* request = find(ticket);
    if (!request)
        return;
    discard(*request);
    release_resources(*request);
    static_cast<void>(loader_.invalidate(request->asset));
    if (const AssetRecord* record = catalog_.find(request->asset))
        for (AssetId dependency : record->dependencies)
            static_cast<void>(loader_.invalidate(dependency));
    request->error.resize(0);
    request->state = SceneResidencyState::queued;
    ++metrics_.reloaded;
}
size_t AssetResidencyCoordinator::resource_position(render::RenderAssetId id) const {
    size_t begin = 0, end = references_.size();
    while (begin < end) {
        const size_t middle = begin + (end - begin) / 2;
        if (references_[middle].id.value < id.value)
            begin = middle + 1;
        else
            end = middle;
    }
    return begin;
}
void AssetResidencyCoordinator::release_resources(Request& request) {
    for (render::RenderAssetId resource : request.resources) {
        const size_t index = resource_position(resource);
        assert(index < references_.size() && references_[index].id == resource && references_[index].count);
        if (--references_[index].count == 0)
            renderer_.release(renderer_.context, resource);
    }
    request.resources.resize(0);
}
void AssetResidencyCoordinator::fail(Request& request, const char* error) {
    set_asset_error(request.error, error);
    release_resources(request);
    request.scene = {};
    request.dependencies.resize(0);
    request.resident = {};
    request.state = SceneResidencyState::failed;
    ++metrics_.failed;
}
void AssetResidencyCoordinator::update() {
    GLOOM_PROFILE_SCOPE(::gloom::AllocationPhase::assets);
    queued_.resize(0);
    for (size_t index = 0; index < requests_.size(); ++index)
        if (requests_[index]->state == SceneResidencyState::queued)
            queued_.push_back({.ticket = index + 1, .request = requests_[index]});
    struct Order {
        static int compare(const void* a, const void* b) {
            const Queued& left = *static_cast<const Queued*>(a);
            const Queued& right = *static_cast<const Queued*>(b);
            if (left.request->priority != right.request->priority)
                return left.request->priority > right.request->priority ? -1 : 1;
            return left.ticket < right.ticket ? -1 : left.ticket > right.ticket ? 1 : 0;
        }
    };
    if (queued_.size() > 1)
        qsort(queued_.data(), queued_.size(), sizeof(Queued), Order::compare);
    for (size_t index = 0; index < queued_.size() && index < settings_.new_scene_requests_per_update; ++index) {
        Request& request = *queued_[index].request;
        request.scene = loader_.request(request.asset);
        request.state = SceneResidencyState::loading_scene;
        ++metrics_.dispatched;
    }
    for (Request* entry : requests_) {
        Request& request = *entry;
        if (request.state == SceneResidencyState::loading_scene && request.scene.ready()) {
            const AssetLoadResult& result = request.scene.get();
            if (result.state != AssetLoadState::ready || result.asset.type != AssetType::scene) {
                fail(request, result.error.size() ? result.error.data() : "Scene asset failed to load");
                continue;
            }
            request.dependencies.reserve(result.asset.dependencies.size());
            for (AssetId dependency : result.asset.dependencies)
                request.dependencies.push_back(loader_.request(dependency));
            request.state = SceneResidencyState::loading_dependencies;
        }
        if (request.state == SceneResidencyState::loading_dependencies) {
            bool ready = true;
            for (const AssetLoadHandle& dependency : request.dependencies)
                ready = ready && dependency.ready();
            if (!ready)
                continue;
            for (const AssetLoadHandle& dependency : request.dependencies) {
                const AssetLoadResult& result = dependency.get();
                if (result.state != AssetLoadState::ready || result.asset.type != AssetType::texture) {
                    fail(request, result.error.size() ? result.error.data() : "Scene texture dependency failed");
                    break;
                }
                if (!catalog_.find(result.asset.id)) {
                    fail(request, "Scene dependency disappeared from the catalog");
                    break;
                }
            }
            if (request.state == SceneResidencyState::failed)
                continue;
            const AssetRecord* record = catalog_.find(request.asset);
            if (!record) {
                fail(request, "Scene disappeared from the catalog");
                continue;
            }
            PreparationContext* context = new PreparationContext{.owner = this,
                .request = &request,
                .generation = request.generation,
                .input = static_cast<AssetLoadHandle&&>(request.scene),
                .dependencies = static_cast<Array<AssetLoadHandle>&&>(request.dependencies),
                .source = record->source,
                .scene_id = request.asset,
                .use_bc = renderer_.texture_compression_bc(renderer_.context)};
            request.state = SceneResidencyState::preparing;
            jobs_.schedule(preparation_tasks_, core::JobSystem::Job{context});
        }
        if (request.state == SceneResidencyState::preparing) {
            AcquireSRWLockExclusive(&publication_);
            PreparedScene* prepared = request.completed;
            request.completed = nullptr;
            ReleaseSRWLockExclusive(&publication_);
            if (!prepared)
                continue;
            if (prepared->error.size()) {
                fail(request, prepared->error.data());
                delete prepared;
                continue;
            }
            request.resident.asset = request.asset;
            request.resident.generation = request.generation;
            request.resident.instances = static_cast<Array<render::RenderInstance>&&>(prepared->instances);
            request.resident.animation_rig = static_cast<AnimationRig&&>(prepared->animation_rig);
            request.resources = static_cast<Array<render::RenderAssetId>&&>(prepared->resources);
            references_.reserve(references_.size() + request.resources.size());
            for (render::RenderAssetId resource : request.resources) {
                const size_t index = resource_position(resource);
                if (index == references_.size() || references_[index].id != resource) {
                    references_.resize(references_.size() + 1);
                    for (size_t next = references_.size() - 1; next > index; --next)
                        references_[next] = references_[next - 1];
                    references_[index] = {.id = resource};
                }
                if (references_[index].count++)
                    ++metrics_.shared_resource_hits;
            }
            for (render::TextureUpload& texture : prepared->textures)
                // Existing IDs stay shared until their last reference; GPU replacement/versioning belongs to 119.
                if (references_[resource_position(texture.id)].count == 1)
                    renderer_.texture(renderer_.context, static_cast<render::TextureUpload&&>(texture));
            for (render::MeshUpload& mesh : prepared->gpu.meshes)
                if (references_[resource_position(mesh.id)].count == 1)
                    renderer_.mesh(renderer_.context, static_cast<render::MeshUpload&&>(mesh));
            for (render::MaterialUpload& material : prepared->gpu.materials)
                if (references_[resource_position(material.id)].count == 1)
                    renderer_.material(renderer_.context, static_cast<render::MaterialUpload&&>(material));
            delete prepared;
            request.state = SceneResidencyState::uploading;
        }
        if (request.state == SceneResidencyState::uploading) {
            bool ready = true;
            for (render::RenderAssetId resource : request.resources) {
                const render::GpuAssetState state = renderer_.state(renderer_.context, resource);
                if (state == render::GpuAssetState::failed) {
                    fail(request, "Renderer rejected a scene GPU resource");
                    ready = false;
                    break;
                }
                ready = ready && state == render::GpuAssetState::resident;
            }
            if (ready) {
                request.state = SceneResidencyState::ready;
                ++metrics_.ready;
            }
        }
    }
}
SceneResidencyState AssetResidencyCoordinator::state(SceneTicket ticket) const {
    const Request* request = find(ticket);
    assert(request);
    return request->state;
}
const ResidentScene* AssetResidencyCoordinator::scene(SceneTicket ticket) const noexcept {
    const Request* request = find(ticket);
    return request && request->state == SceneResidencyState::ready ? &request->resident : nullptr;
}
const char* AssetResidencyCoordinator::error(SceneTicket ticket) const noexcept {
    const Request* request = find(ticket);
    return request && request->error.size() ? request->error.data() : "";
}
ResidencyCoordinatorMetrics AssetResidencyCoordinator::metrics() const noexcept {
    AcquireSRWLockShared(&publication_);
    const ResidencyCoordinatorMetrics result = metrics_;
    ReleaseSRWLockShared(&publication_);
    return result;
}
}
