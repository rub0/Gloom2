# ADR 0022: Production temporal reconstruction

## Status

Accepted.

## Context

The first temporal foundation reprojected color using motion vectors and a
small neighborhood clamp. It could not distinguish a stable surface from a
newly revealed one, retained too much history across abrupt lighting changes
and used a fixed render scale. Spatial upsampling also lacked a controlled
sharpening stage.

The policy for changing resolution and advertising these features must remain
independent from Diligent and Vulkan. GPU resource ownership and shader
execution stay inside the backend.

## Decision

### Depth-aware and reactive history

Opaque rendering writes device depth to a shader-readable R32F target alongside
HDR color and motion. Native TAA resolves both output-resolution color and depth
into paired ping-pong histories. Reprojected history is rejected when current
and historical depth differ beyond a depth-relative threshold, covering camera
and object disocclusions without reading the hardware depth buffer.

History weight is reduced continuously when current and reprojected luminance
differ. This reactive heuristic handles emissive and illumination changes in
the current opaque pipeline without requiring scene code to know about a
backend texture. Explicit material reactive masks can extend this input later.

The ACES pass applies a configurable, luminance-limited four-neighbor sharpening
filter before tone mapping. Sharpness is part of `TemporalSettings` and is
bounded during feature negotiation.

### Dynamic resolution policy

`DynamicResolutionController` is a backend-neutral controller driven by delayed
GPU frame duration. It filters samples, uses separate overload and headroom
thresholds, waits a configurable number of settling frames and changes scale in
bounded steps between the configured minimum and full resolution.

The Vulkan backend feeds the asynchronous shadow, opaque, temporal and tone-map
durations into the controller. A scale change recreates only size-dependent
frame resources, invalidates temporal history and preserves output resolution.
The current implementation waits for the immediate context before recreation;
scale changes are deliberately infrequent, favoring correctness over a more
complex deferred resource-generation scheme.

Capabilities expose depth disocclusion, reactive history, contrast-adaptive
sharpening and dynamic resolution independently. Frame metrics publish the
active scale, filtered GPU duration and cumulative scale changes.

## Consequences and limits

- Revealed surfaces and fast depth discontinuities no longer blend unrelated
  historical color.
- Abrupt HDR changes reduce their own history contribution, limiting trails.
- Resolution policy is deterministic and testable without graphics APIs.
- Dynamic changes currently cause a GPU idle point. A future resource pool can
  retain generations behind fences without changing the public controller.
- Transparent surfaces do not yet emit independent motion, depth or authored
  reactive masks.

## Validation

`gloom.temporal` verifies overload hysteresis, minimum-scale bounds and recovery
under GPU headroom. The SDL smoke test requires every production temporal
capability, observes a real dynamic scale change and checks valid history after
recreation. Network and 600-frame Vulkan tests exercise depth/color history,
sharpening, repeated scale changes, minimization and restoration.

