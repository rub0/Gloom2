#include <gloom/assets/residency_coordinator.hpp>
#include <gloom/assets/gltf_importer.hpp>
#include <gloom/assets/scene_gpu_bridge.hpp>
#include <gloom/core/allocation_profile.hpp>
#include <stdio.h>
#include <stdlib.h>

using namespace gloom;
using namespace gloom::assets;
namespace {
void check(bool condition, const char* message) {
    if (!condition) {
        fputs(message, stderr);
        fputc('\n', stderr);
        exit(1);
    }
}
struct Gate {
    HANDLE entered, released;
    void operator()() const noexcept {
        SetEvent(entered);
        WaitForSingleObject(released, INFINITE);
    }
};
struct Sink {
    Array<render::RenderAssetId> ids;
    bool pending{false}, reject{false};
    uint32 uploads{0}, releases{0};
    static bool compression(const void*) {
        return false;
    }
    static void mesh(void* context, render::MeshUpload upload) {
        Sink& self = *static_cast<Sink*>(context);
        self.ids.push_back(upload.id);
        ++self.uploads;
    }
    static void texture(void* context, render::TextureUpload upload) {
        Sink& self = *static_cast<Sink*>(context);
        self.ids.push_back(upload.id);
        ++self.uploads;
    }
    static void material(void* context, render::MaterialUpload upload) {
        Sink& self = *static_cast<Sink*>(context);
        self.ids.push_back(upload.id);
        ++self.uploads;
    }
    static void release(void* context, render::RenderAssetId id) {
        Sink& self = *static_cast<Sink*>(context);
        for (size_t index = 0; index < self.ids.size(); ++index)
            if (self.ids[index] == id) {
                self.ids[index] = self.ids[self.ids.size() - 1];
                self.ids.resize(self.ids.size() - 1);
                break;
            }
        ++self.releases;
    }
    static render::GpuAssetState state(const void* context, render::RenderAssetId id) {
        const Sink& self = *static_cast<const Sink*>(context);
        for (render::RenderAssetId present : self.ids)
            if (present == id)
                return self.reject ? render::GpuAssetState::failed : self.pending ? render::GpuAssetState::queued : render::GpuAssetState::resident;
        return render::GpuAssetState::missing;
    }
    AssetUploadSink view() {
        return {
            .context = this, .texture_compression_bc = compression, .mesh = mesh, .texture = texture, .material = material, .release = release, .state = state};
    }
};
void wait_for(AssetResidencyCoordinator& coordinator, SceneTicket ticket, SceneResidencyState state) {
    const uint64 deadline = GetTickCount64() + 5000;
    while (GetTickCount64() < deadline) {
        coordinator.update();
        if (coordinator.state(ticket) == state)
            return;
        check(coordinator.state(ticket) != SceneResidencyState::failed, coordinator.error(ticket));
        SwitchToThread();
    }
    check(false, "Residency timed out");
}
}
int main() {
    ImportedScene named_arm;
    named_arm.nodes.resize(1);
    const char full_name[]{'L', 'e', 'f', 't', 0, 'H', 'a', 'n', 'd'};
    named_arm.nodes[0].name.assign(full_name, sizeof(full_name));
    named_arm.skins.push_back({.joints = {0}});
    named_arm.primitives.push_back({.vertices = {{.weights = {1}}, {.weights = {1}}, {.weights = {1}}}, .indices = {0, 1, 2}});
    const GpuSceneUploads named_uploads = build_gpu_scene_uploads(named_arm, {.value = 45});
    check(named_uploads.primitives[0].arms_mesh.value != 0, "Arm classification truncated a counted imported name");
    char directory[MAX_PATH], path[MAX_PATH];
    check(GetTempPathA(MAX_PATH, directory) != 0, "Temporary directory unavailable");
    snprintf(path, sizeof(path), "%sgloom-residency-%lu", directory, GetCurrentProcessId());
    check(CreateDirectoryA(path, nullptr) != 0, "Create temporary directory");
    VirtualFileSystem filesystem;
    check(filesystem.mount("test", path).has_value(), "Mount fixture");
    const VirtualPath source = *VirtualPath::parse("test:/scene.gltf");
    const VirtualPath cooked = *VirtualPath::parse("test:/scene.gasset");
    ImportedScene scene;
    scene.materials.push_back({});
    scene.primitives.push_back({.vertices = {{.position = {0, 0, 0}}, {.position = {1, 0, 0}}, {.position = {0, 1, 0}}}, .indices = {0, 1, 2}, .material = 0});
    scene.meshes.push_back({.first_primitive = 0, .primitive_count = 1});
    // Deep hierarchy exercises the iterative traversal rather than recursive stack growth.
    scene.nodes.resize(2048);
    for (uint32 index = 0; index < 2047; ++index)
        scene.nodes[index].children.push_back(index + 1);
    scene.nodes[2047].mesh = 0;
    scene.scenes.push_back({.roots = {0}});
    CookedAsset asset{.id = make_asset_id(source, AssetType::scene), .type = AssetType::scene, .payload = encode_imported_scene(scene)};
    asset.source_fingerprint = fingerprint(asset.payload);
    check(filesystem.write(cooked, encode_cooked_asset(asset)).has_value(), "Write cooked fixture");
    AssetCatalog catalog;
    catalog.add({.id = asset.id, .type = asset.type, .source = source, .cooked = cooked, .source_fingerprint = asset.source_fingerprint});
    core::JobSystem jobs{{.worker_threads = 1, .queue_capacity = 256}};
    check(jobs.start() == nullptr, "Start jobs");
    HANDLE entered = CreateEventW(nullptr, TRUE, FALSE, nullptr), released = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    check(entered && released, "Initialize gates");
    {
        AsyncAssetLoader loader{jobs, filesystem, catalog};
        core::TaskGroup group = jobs.create_group();
        jobs.schedule(group, Gate{.entered = entered, .released = released});
        check(WaitForSingleObject(entered, 5000) == WAIT_OBJECT_0, "Gate did not start");
        AssetLoadHandle first = loader.request(asset.id), duplicate = loader.request(asset.id);
        const AssetLoadTicket retired = first.ticket();
        check(first.ticket() == duplicate.ticket() && !first.ready(), "Duplicate pending load did not coalesce");
        check(loader.invalidate(asset.id), "Invalidate pending load");
        AssetLoadHandle replacement = loader.request(asset.id);
        check(replacement.ticket() != retired && !replacement.ready(), "Pending invalidation reused the old generation");
        SetEvent(released);
        jobs.wait(group);
        loader.wait();
        check(&first.get() == &duplicate.get() && &first.get() != &replacement.get() && first.get().asset.payload == replacement.get().asset.payload,
            "Retired result lost ownership or duplicate result was copied");
        AssetLoadHandle copy = first;
        first = {};
        duplicate = {};
        check(copy.get().asset.id == asset.id && loader.metrics().stale_completions == 1, "Retired result did not survive its pins");
        copy = {};
        AssetLoadHandle missing = loader.request({.value = 123});
        check(missing.ticket().index == retired.index && missing.ticket().generation != retired.generation &&
                  missing.get().state == AssetLoadState::error_placeholder && missing.get().error.size(),
            "Freed slot did not advance its generation");
        Sink sink;
        {
            AssetResidencyCoordinator coordinator{jobs, loader, catalog, sink.view(), {.new_scene_requests_per_update = 0}};
            for (uint32 index = 0; index < 64; ++index)
                static_cast<void>(coordinator.request_scene(asset.id));
            coordinator.update();
            allocation_profile_reset();
            allocation_profile_window(true);
            for (uint32 index = 0; index < 1000; ++index)
                coordinator.update();
            allocation_profile_window(false);
            const AllocationStats warm = allocation_profile_read(AllocationPhase::other);
            check(warm.calls == 0 && warm.bytes == 0, "Stable queued updates allocated");
            allocation_profile_reset();
            allocation_profile_window(true);
            static_cast<void>(coordinator.request_scene(asset.id));
            allocation_profile_window(false);
            check(allocation_profile_read(AllocationPhase::other).calls != 0, "Allocation growth control did not observe allocation");
            printf("queued_updates=1000 allocations=%llu bytes=%llu growth_control=positive\n", warm.calls, warm.bytes);
        }
        {
            AssetResidencyCoordinator coordinator{jobs, loader, catalog, sink.view()};
            const SceneTicket first_scene = coordinator.request_scene(asset.id);
            wait_for(coordinator, first_scene, SceneResidencyState::ready);
            check(coordinator.scene(first_scene)->instances.size() == 1, "Deep hierarchy lost its instance");
            const SceneTicket second_scene = coordinator.request_scene(asset.id);
            wait_for(coordinator, second_scene, SceneResidencyState::ready);
            check(sink.uploads == 2 && coordinator.metrics().shared_resource_hits == 2, "Shared scene duplicated uploads");
            coordinator.reload(first_scene);
            wait_for(coordinator, first_scene, SceneResidencyState::ready);
            check(coordinator.scene(first_scene)->generation == 2 && coordinator.state(second_scene) == SceneResidencyState::ready && sink.uploads == 2 &&
                      sink.releases == 0,
                "Reload retired another scene's shared resources");
            allocation_profile_reset();
            allocation_profile_window(true);
            for (uint32 index = 0; index < 1000; ++index)
                coordinator.update();
            allocation_profile_window(false);
            check(allocation_profile_read(AllocationPhase::other).calls == 0, "Ready updates allocated");
            coordinator.cancel(first_scene);
            check(sink.releases == 0, "First reference released shared resources");
            coordinator.cancel(second_scene);
            check(sink.releases == 2 && sink.ids.size() == 0, "Last reference left resources alive");
            sink.pending = true;
            const SceneTicket queued = coordinator.request_scene(asset.id);
            wait_for(coordinator, queued, SceneResidencyState::uploading);
            coordinator.cancel(queued);
            check(sink.ids.size() == 0 && coordinator.scene(queued) == nullptr, "Cancel during upload retained resources");
            coordinator.reload(queued);
            wait_for(coordinator, queued, SceneResidencyState::uploading);
            sink.pending = false;
            coordinator.update();
            check(coordinator.state(queued) == SceneResidencyState::ready && coordinator.scene(queued)->generation == 3,
                "Upload reload published wrong generation");
            coordinator.cancel(queued);
            sink.reject = true;
            const SceneTicket failed = coordinator.request_scene(asset.id);
            // Failed GPU publication retires every resource in the transaction.
            const uint64 deadline = GetTickCount64() + 5000;
            while (coordinator.state(failed) != SceneResidencyState::failed && GetTickCount64() < deadline) {
                coordinator.update();
                SwitchToThread();
            }
            check(coordinator.state(failed) == SceneResidencyState::failed && sink.ids.size() == 0 && coordinator.scene(failed) == nullptr,
                "Partial GPU failure retained resources");
            sink.reject = false;
        }
        ResetEvent(entered);
        ResetEvent(released);
        jobs.schedule(group, Gate{.entered = entered, .released = released});
        check(WaitForSingleObject(entered, 5000) == WAIT_OBJECT_0, "Second gate did not start");
        {
            AssetResidencyCoordinator coordinator{jobs, loader, catalog, sink.view()};
            const SceneTicket ticket = coordinator.request_scene(asset.id);
            coordinator.update();
            check(coordinator.state(ticket) == SceneResidencyState::preparing, "Preparation was not queued");
            coordinator.cancel(ticket);
            SetEvent(released);
            // Destructor drains a discarded preparation before destroying its request owner.
        }
        jobs.wait(group);
        check(sink.ids.size() == 0, "Shutdown published discarded uploads");
    }
    jobs.stop();
    CloseHandle(entered);
    CloseHandle(released);
    char file[MAX_PATH];
    snprintf(file, sizeof(file), "%s/scene.gasset", path);
    remove(file);
    RemoveDirectoryA(path);
    puts("Residency ownership, generations, sharing, failure and shutdown passed");
    return 0;
}
