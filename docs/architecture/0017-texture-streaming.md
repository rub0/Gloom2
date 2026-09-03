# ADR 0017: KTX2 texture pipeline and GPU residency

## Status

Accepted for texture payload v1 and the first bounded residency policy.

## Context

The first asset cooker copied PNG/JPEG bytes unchanged, while the renderer only
accepted one RGBA8 level and released replaced resources immediately. This
prevented correct minification, made upload work unbounded, and could destroy a
Vulkan resource while an earlier frame still referenced it. Core glTF materials
also need distinct color, metallic-roughness and normal texture semantics.

## Decision

### Offline texture processing

`stb_image` is restricted to the offline cooker and decodes common authoring
images to RGBA8. The cooker generates the complete mip chain down to 1x1.
Color textures are filtered in linear light and stored with an sRGB transfer
function; normal and metallic-roughness data are filtered numerically and stay
linear.

KTX 4.4.2 writes the cooked payload as KTX2 Basis Universal. Color textures use
ETC1S for compact distribution. Normal and data textures use UASTC, with the
normal-map encoder mode enabled where appropriate. The `.gasset` envelope and
checksum from ADR 0015 contain the complete KTX2 byte stream.

At runtime libktx validates the container. RGBA8 was the initial compatibility
baseline; ADR 0018 adds device selection and direct BC7/BC5 transcoding without
changing the cooked KTX2 files.

### PBR texture bindings

`MaterialUpload` carries independent base-color, metallic-roughness and normal
texture IDs. glTF's metallic value comes from B and roughness from G. A white
linear texture preserves scalar material factors when a packed map is absent;
a separate `(0.5, 0.5, 1)` fallback supplies a flat tangent-space normal.

The initial vertex format had no tangent and reconstructed a basis from screen
derivatives. ADR 0019 replaces that compatibility path with cooked MikkTSpace
tangents, including vertex splits at mirrored UV seams.

### Bounded residency and lifetime

Renderer settings define per-frame upload bytes and total resident bytes.
Enqueueing remains thread-safe and device-free. At `begin_frame`, the render
thread drains whole uploads until the frame budget is exhausted; a single
oversized first item is allowed so it cannot starve forever. Diligent performs
the physical Vulkan staging copies.

Every frame signals a Diligent fence after submitted graphics work. Replaced,
explicitly released and evicted buffers/textures move to deferred queues tagged
with the last submitted value. Their reference-counted Diligent objects are
destroyed only after `GetCompletedValue` reaches that value.

When resident bytes exceed the configured limit, the renderer evicts the least
recently drawn non-built-in mesh or texture. Built-in cube, default material,
white texture and flat normal texture are pinned. Metrics expose queued,
uploaded and resident bytes, deferred/completed releases, evictions and frames
limited by the upload budget.

## Consequences and limits

Authoring codecs no longer run on the render thread, mip behavior is explicit,
and GPU lifetime is synchronized rather than guessed. Texture data can be
streamed over several frames without an unbounded hitch, and the policy is
observable in tests and telemetry.

RGBA8 remains the fallback when the device lacks a selected compressed format.
There is no dedicated application-owned Vulkan transfer queue or ring;
the byte budget feeds Diligent's internal upload heap on the immediate context.
Renderer eviction remains simple LRU and has no distance or visibility weight
yet. Scene dependency orchestration, request priority, cancellation and hot
reload are defined by ADR 0018.

## Validation

`gloom.assets` cooks a real 2x2 image to ETC1S KTX2, verifies sRGB and both mip
levels, then cooks the same fixture as a linear UASTC normal map. Malformed or
empty KTX2 input is rejected.

The SDL smoke test uses a 1 KiB/frame upload limit and 2 KiB residency limit. It
uploads two 1 KiB textures, verifies that one is deferred to the next frame,
explicitly releases another texture, observes fence-completed destruction and
checks that LRU eviction removes the older unpinned texture. The Vulkan stress
test continues to run 600 frames with validation enabled.

## References

- [KTX Software](https://github.com/KhronosGroup/KTX-Software)
- [KTX 2.0 specification](https://registry.khronos.org/KTX/specs/2.0/ktxspec.v2.html)
- [glTF 2.0 material model](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#materials)
- [Diligent fence interface](https://github.com/DiligentGraphics/DiligentCore/blob/master/Graphics/GraphicsEngine/interface/Fence.h)
