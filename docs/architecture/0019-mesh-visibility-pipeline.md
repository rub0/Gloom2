# ADR 0019: Production mesh and visibility pipeline

## Status

Accepted for cooked scene payload version 2 and the first CPU visibility path.

## Context

Imported meshes previously preserved their original vertex/index order, carried
no tangent or bounds data and always rendered their highest-detail index stream.
The pixel shader reconstructed tangent space from screen-space derivatives, and
the renderer repeated mesh, material and texture binding work for every instance,
including objects outside the camera frustum.

## Decision

### Offline mesh processing

The asset pipeline uses MikkTSpace to generate a four-component tangent for each
triangle corner. Vertices are split at tangent discontinuities and then
deduplicated. Meshoptimizer performs vertex remapping, post-transform cache
optimization, conservative overdraw reordering and vertex-fetch optimization.

When a glTF primitive omits normals, the cooker generates smooth area-weighted
normals. When it omits UV coordinates, a stable orthogonal tangent is generated
instead of rejecting an otherwise valid untextured mesh.

Each primitive stores a local bounding sphere and up to two simplified index
streams, targeted at 50% and 20% of the source index count. Simplification only
keeps a level when it is valid and strictly smaller than the preceding level.
The cooked scene payload is now version 2; version 1 scenes must be recooked.

### Runtime visibility and LOD

`VisibilitySystem` is independent from Diligent. It transforms each local sphere,
tests it against the camera frustum and selects an available LOD using distance
measured in object radii. Defaults select LOD1 at 30 radii and LOD2 at 80 radii,
which keeps behavior scale-independent and exposes both thresholds as policy.

The input is divided into configurable chunks and processed through Gloom task
groups. Per-job output avoids synchronization in the hot loop. Visible instances
are merged deterministically, sorted by material and mesh, and emitted with
contiguous `RenderBatch` records. Metrics report submitted/visible/culled counts,
LOD distribution, job and batch counts, and CPU build time.

The Diligent backend consumes these batches and binds their mesh, material and
textures once per batch. It still issues one indexed draw per instance because
per-instance constants use the existing dynamic uniform buffer. Hardware
instancing and indirect draws can replace that inner loop without changing the
visibility API.

### Stress laboratory

`gloom_scene_viewer` accepts an optional copy count. It replicates the imported
scene over a grid, runs the normal residency and visibility paths, and prints the
last visible, culled, batch, LOD and CPU-time measurements when it closes.

## Consequences and limits

Normal mapping now uses the same tangent convention as common content tools,
and static mesh work moves to the cooker. Runtime cost scales with candidate
instances across worker jobs, while GPU submissions avoid off-frustum objects
and unnecessary state rebinding.

The LOD generator does not yet preserve authored LODs, skinning, morph targets
or meshlets. Culling is CPU frustum-only: there is no occlusion hierarchy or GPU
indirect command generation. Non-uniform scale uses the largest scale component
for conservative spheres. These choices avoid false-negative visibility.

## Validation

`gloom.assets` processes a grid mesh, validates tangent handedness, bounds and a
strictly smaller LOD, and verifies safe normal/tangent fallbacks without source
attributes. Its real glTF round trip checks that payload v2 preserves tangents
and bounds.

`gloom.visibility` uses three worker chunks to verify near, medium and far LODs,
behind-camera and side-plane culling, deterministic grouping and owned snapshot
data. The SDL, network scene and 600-frame Vulkan validation tests exercise the
new vertex layout and batched renderer path.

## References

- [MikkTSpace](https://github.com/mmikk/MikkTSpace)
- [meshoptimizer](https://github.com/zeux/meshoptimizer)
