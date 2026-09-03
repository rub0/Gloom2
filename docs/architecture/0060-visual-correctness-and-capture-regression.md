# ADR 0060: Visual correctness and reproducible GPU acceptance

## Status

Accepted. Milestone 58 addresses the visual defects reported in the Factory
recording and extends ADRs 0019, 0020, 0024 and 0046. The existing authored
resources remain primitive models; this milestone fixes their presentation,
not their artistic fidelity.

## Findings

All six closed project glTF mesh sources had inward triangle winding. Each
shared eight-vertex primitive contained twelve inward-facing triangles; the
separate lava plane already faced upward. With back-face culling this exposed
the wrong surfaces, and generated normals pointed inward. The importer also
averaged normals across shared corners when NORMAL was absent, smoothing hard
edges. The vertex shader multiplied normals by World, which is incorrect for
the highly nonuniform scales used by the level and props. Builtin tangents
could be parallel to normals, creating an invalid tangent frame.

First-person props translated with aim but retained world-axis orientation;
their horizontal offset also used the opposite sign to the camera's right
vector. Their shadow participation and world depth allowed close props to
produce misleading world shadows or disappear into nearby geometry.

## Decisions

Correct triangle winding in the existing closed glTF sources without changing
positions, bounds, collision or gameplay data. Preserve the one-sided lava
plane. Missing normals are generated per triangle, splitting corners before
tangent generation and vertex optimization. Authored normals remain intact.
Use reciprocal scale followed by rotation for the normal matrix, orthogonalize
the tangent against the normal, and provide a finite perpendicular fallback.
Hidden zero-scale instances are skipped before constructing normal matrices.

`camera_relative_transform` derives a complete orthonormal camera basis and
quaternion, including pitch. Soul Reaper, Bite and Guard use bounded offsets
and dimensions in that basis. `RenderInstance::view_model` selects a separate
depth pass after the world; these props neither cast nor receive world shadows.
This deliberately keeps the weapon visible against a wall without moving its
authoritative shot origin or collision. Procedural HUD objects also stop
casting shadows. First-person previous transforms feed existing motion vectors.

Directional shadows use 2048-square maps, a 3x3 comparison kernel on actual
depth samples, receiver slope bias, cascade blending and light-space texel
snapping. Cascade coverage accounts for both aspect ratio and vertical FOV;
selection uses camera-forward depth, matching the split definition. Shadow
distance is bounded to 80 world units. Factory supplies the unculled source
instances separately from the visible draw list so offscreen objects can cast
shadows. A modest Factory-specific ambient fill improves shaded-face readability.

## Visual acceptance

`gloom --visual-review OUTPUT_DIRECTORY` freezes simulation and input, waits for
all authored scenes to become resident, and renders nine fixed camera/ability
views at native resolution. Dynamic resolution is disabled. Each view settles
for 32 frames before an explicit Vulkan staging readback after tone mapping and
before Present. Normal gameplay does not perform GPU readback or disk writes.
The PPM files contain display RGB values and preserve full output resolution.
Closing the window before all captures complete is a failure.

`gloom.visual_review` runs the executable, records Vulkan diagnostics, and
compares each capture with manually inspected 160x90 RGB references. Box
averaging reduces subpixel noise. Limits are mean absolute error 4/255, at most
1.5% of pixels exceeding 32/255 average channel error, and worst 20x15 tile error
12/255. Both whole-image and regional checks are required. Reference generation
is an explicit separate tool mode and is never performed by CTest.

Blank frames and missing foreground quadrants are negative controls. The seven
available pre-fix captures also fail against the corrected references, with
mean error from 14.5 to 42.8/255. This establishes that the test detects the
reported defects instead of only confirming that assets were uploaded.

`gloom.assets` checks outward project normals and upward lava, plus flat hard
edges. `gloom.render_scene` checks prop orientation/offset through a full yaw
turn and steep pitch, and normal/tangent perpendicularity under nonuniform
scale. The existing gameplay, network and Vulkan tests remain regression gates.

## Limits

Image references are tolerant GPU checks, not proof of artistic quality or
universal pixel identity across drivers. Inspect a hardware-specific mismatch
before changing thresholds or references. Static checkpoints complement manual
movement inspection; they do not measure every temporal artifact. The current
models and HUD are still provisional, and sophisticated shadow filtering,
animated view models and full authored environment lighting remain later work.
See [VISUAL_REVIEW.md](../VISUAL_REVIEW.md) for reproducible commands and review
rules.
