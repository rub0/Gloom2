# ADR 0016: GPU assets and render graph

## Status

Accepted for the first resident GPU asset and forward-PBR path.

## Context

The renderer previously owned one compile-time cube and issued clear, draw and
present operations directly. Cooked scenes stopped at CPU payloads, so gameplay
could not refer to imported geometry without exposing Diligent objects. Adding
more passes in that form would hide resource hazards and make a later D3D12
backend, temporal upscaler or asynchronous compute path expensive to isolate.

## Decision

### Backend-neutral GPU assets

Render snapshots now contain `RenderAssetId` values. `Renderer` accepts owned
`MeshUpload`, `TextureUpload` and `MaterialUpload` packets and exposes only a
small missing/queued/resident/failed state. Diligent owns the actual buffer,
texture, view and material registries. Queue insertion is mutex-protected and
does no graphics work; the render thread drains it at `begin_frame`.

`build_gpu_scene_uploads` converts an imported/cooked scene representation into
owned vertex/index and metallic-roughness material packets. Child IDs are
derived deterministically from the scene `AssetId`, kind and primitive/material
index. This conversion is suitable for loader jobs and does not include any
Diligent or Vulkan type.

The built-in cube follows exactly the same upload route as imported meshes.
There are no longer special cube buffers in the draw path. A white texture and
default material provide deterministic fallbacks, while missing meshes skip a
draw instead of dereferencing an invalid GPU resource.

### Forward PBR baseline

The opaque pipeline consumes position, normal and UV data and evaluates a
compact metallic-roughness Cook-Torrance BRDF. Base-color textures are mutable
shader resources with an immutable sampler; material color, emissive, metallic
and roughness remain per-draw constants for this baseline. The shader is a
functional material path, not the final lighting system: it has one fixed
directional light and no IBL, shadows or normal maps yet.

### Render graph

`RenderGraph` is a Gloom-owned declarative API. Passes declare resource access
and required state, plus optional explicit dependencies. Compilation rejects
invalid handles, transient read-before-write and cycles; it produces a stable
topological pass order, state-transition schedule and first/last-use lifetime.
Compatible, non-overlapping transient resources receive the same alias slot.

The Vulkan backend currently lets Diligent execute the compiled transitions
through its `RESOURCE_STATE_TRANSITION_MODE_TRANSITION` tracking. The graph is
the source of pass ordering and desired states; a future lower-level backend
can translate its barrier list directly to Vulkan or D3D12 barriers.

## Consequences and limits

Gameplay, asset loading and scene preparation never retain backend pointers.
Upload ownership is explicit, arbitrary loader threads can enqueue work, and
all device creation remains on the rendering thread. Meshes, materials and
textures can be replaced by ID without changing snapshots.

The first implementation creates immutable resources while draining the queue.
It does not yet have a bounded staging-ring budget, copy-queue fences, eviction,
hot reload, bindless descriptors or deferred destruction. Texture packets must
already be decoded RGBA8; the cooker still preserves source PNG/JPEG bytes.
KTX2/Basis processing, mip generation, normal/metallic-roughness texture
bindings, IBL and a GPU-safe streaming lifecycle are the next asset-rendering
slice.

## Validation

`gloom.render_graph` checks ordering, transitions, invalid reads, cycles and
transient aliasing. `gloom.gpu_assets` checks deterministic scene-to-upload
conversion and primitive bindings. The SDL smoke test asserts that the built-in
mesh, material and texture become resident on the first frame. The 600-frame
Vulkan validation stress still covers minimize/restore and fails on any
Diligent validation error, including the previous swapchain semaphore VUID.

## References

- [Vulkan synchronization examples](https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples)
- [Diligent Engine resource state transitions](https://diligentgraphics.com/diligent-engine/architecture/diligent-engine-api/)
- [Khronos glTF 2.0 metallic-roughness material model](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#materials)
