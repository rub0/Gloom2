# ADR 0021: Temporal render-feature foundation

## Status

Accepted.

## Context

The HDR lighting frame rendered at swapchain resolution and discarded all
cross-frame information. It had no motion-vector contract, camera jitter,
history ownership or distinction between render and presentation extents.
Adding a vendor SDK directly to the Diligent backend at that point would have
coupled feature selection, quality policy and public capabilities to one API.

## Decision

### Backend-neutral feature contract

`gloom/render/temporal.hpp` owns temporal settings, capabilities and feature
negotiation. Requests select disabled rendering, native TAA, FSR or DLSS and
carry render scale and history weight. Unsupported vendor techniques fall back
to native TAA when its required motion-vector, jitter and history capabilities
are present, otherwise temporal rendering is disabled safely.

`TemporalFeatureHook` is the discovery boundary for future vendor adapters. A
hook advertises one technique and decides availability from Gloom capability
data; provider SDK types and GPU handles do not enter the renderer API. The
native path needs no hook.

Render extents and an eight-sample Halton jitter sequence are also Gloom-owned,
deterministic utilities. Render scale is bounded to 50-100 percent and a zero
output extent suppresses temporal work during minimization.

### Motion and history contract

`RenderInstance` optionally carries its previous presentation transform. The
opaque vertex stage combines it with the previous unjittered camera matrix and
emits per-pixel screen-space motion into an RG16F target alongside RGBA16F HDR
color. New or static instances omit the previous transform and receive camera
motion only.

The Diligent Vulkan backend owns render-resolution HDR, motion and D32 depth
targets plus two output-resolution RGBA16F history targets. Histories alternate
without rewriting descriptor sets that may still be referenced by submitted
frames. Resize, minimization and large camera discontinuities invalidate
history.

Native TAA reprojects the previous output with motion vectors, clamps it to a
current-color neighborhood and blends it with the current sample. The resolved
output then feeds the existing exposure and ACES pass. The render graph declares
motion, history and the temporal resolve explicitly between opaque shading and
tone mapping.

### Resolution and telemetry

`RendererSettings::temporal` separates internal render scale from swapchain
extent. `FrameRenderMetrics` reports both extents, history validity and the
asynchronous temporal-resolve duration. Automated graphical modes render at 75
percent and resolve to full output size so the scaled path is continuously
exercised.

## Consequences and limits

- TAA, camera and object motion, resize-safe history and spatial upsampling are
  available without a vendor runtime.
- FSR and DLSS can implement the negotiation hook and a private backend adapter
  later; neither SDK is linked by this milestone.
- Previous transforms are explicit rather than inferred from transient array
  positions, keeping scene ownership outside the renderer.
- The current neighborhood clamp is intentionally compact. Reactive masks,
  transparency motion, disocclusion depth tests, sharpening and dynamic
  resolution policy remain later refinements.

## Validation

`gloom.temporal` checks negotiation, vendor-hook selection, fallback, bounded
render extents and deterministic jitter without a GPU. The SDL smoke test
requires native temporal capabilities, distinct render/output extents and valid
history. The network scene smoke and 600-frame Vulkan synchronization test run
the full motion/TAA path, including minimize, restore and resource recreation.

