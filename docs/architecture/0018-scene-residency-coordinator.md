# ADR 0018: Scene residency coordination and device-native textures

## Status

Accepted for the first cooked-scene runtime path.

## Context

The asynchronous loader could read individual cooked files, and the renderer
could own individual GPU resources, but no layer owned the operation between
them. A game-facing scene request needs to load a dependency tree, perform CPU
decoding away from the render thread, instantiate the imported hierarchy and
keep shared resources alive until their last scene user is gone.

## Decision

`AssetResidencyCoordinator` is the backend-neutral owner of scene requests. A
ticket moves through queued, scene loading, dependency loading, preparation,
GPU upload and ready states. Queued work is dispatched by explicit priority and
can be cancelled at any state. CPU preparation runs through Gloom task groups;
only owned upload packets cross into the renderer's thread-safe queue.

The coordinator resolves cooked texture dependencies through the catalog,
decodes the scene, composes parent/child node matrices and emits one render
instance per mesh primitive. Mesh, material and texture IDs are stable, and a
coordinator-level reference count prevents duplicate uploads and premature
release when several resident scenes share resources.

Reload invalidates completed loader entries, releases the old generation and
re-enters the same state machine. Each successful reload exposes a monotonically
increasing generation, so callers can discard cached scene views safely. A
filesystem watcher is deliberately outside this API: editor tooling can call
`AssetCatalog::upsert` and then request reload without coupling the runtime to a
particular watcher library.

At device startup Diligent reports BC texture support. When available, KTX2
Basis payloads transcode directly to BC7 blocks for color/data textures and BC5
blocks for normal maps. RGBA8 remains the universal fallback. Diligent's current
cross-API texture enum does not expose ASTC, so an eventual mobile backend will
add ASTC when that format is representable through the selected rendering
backend; no cooked asset change is required.

`gloom_scene_viewer` reconstructs a catalog from a scene and the dependency
layout emitted by `gloom_asset_cooker`, then displays the cooked hierarchy via
the same coordinator and Vulkan renderer used by the engine.

## Consequences

Asset I/O, CPU preparation and GPU ownership now have separate APIs and thread
boundaries. Gameplay code deals in scene tickets rather than Diligent or KTX
objects. Cancellation does not stop an already executing loader job, but it
does prevent its result from reaching the GPU; this keeps the job system simple
while preserving observable cancellation semantics.

The first coordinator loads the complete scene dependency set before
instantiation. Fine-grained texture mip streaming, distance-driven priority and
automatic file watching remain later policy layers. Imported transforms are
decomposed for the current render snapshot, so shear cannot be represented
exactly by its translation/rotation/scale form.

## Validation

`gloom.assets` cooks a glTF scene and texture, reconstructs the runtime catalog,
verifies priority and queued cancellation, loads its dependency tree, composes
its node transform and checks all GPU packets using an immediate test renderer.
It then replaces the cooked scene, reloads generation two, verifies the changed
transform and confirms shared resources survive until the final ticket is
cancelled. The same test transcodes real ETC1S/UASTC payloads to BC7 and BC5.

The complete Debug and Release suites retain the SDL, network-scene and
600-frame Vulkan synchronization tests.
