# ADR 0020: Modern lighting frame

## Status

ADR 0060 subsequently adds PCF filtering, cascade texel snapping/blending,
aspect-aware coverage and a separate unculled caster list for Factory.

Accepted.

## Context

The initial PBR path rendered directly into the SDR swapchain, used one fixed
directional light and had no GPU-side visibility structure for local lights.
That was sufficient to validate materials, but it could not support a modern
lighting workload, exposure, shadows or reliable pass-level performance data.

The renderer boundary must remain owned by Gloom. Diligent and Vulkan are
implementation details so that lighting policy, scene preparation and telemetry
do not become coupled to one graphics backend.

## Decision

`ClusteredLightingBuilder` prepares a backend-neutral immutable lighting view.
It filters and bounds point lights, assigns them to a logarithmic 16 x 9 x 24
view-space grid and exposes build, saturation and reference-count telemetry.
The renderer uploads only the compact light, range and index arrays.

The Diligent backend uses this frame sequence:

1. render four practical-split directional shadow cascades into a 1024 x 1024
   D32 texture array;
2. shade opaque geometry into an RGBA16F HDR target with directional,
   clustered-local and image-based environment contributions;
3. apply exposure and an ACES approximation into the sRGB swapchain;
4. present using Diligent's swapchain synchronization path.

The built-in environment is a small immutable cubemap so the image-based path
is always valid. Asset-provided HDR environments, diffuse convolution and
prefiltered specular mip chains remain a later production asset feature.

Shadow, opaque and tone-map passes are declared in the render graph. Diligent
duration-query rings collect their GPU time without stalling the current frame;
the public `FrameRenderMetrics` contract reports the most recently available
values and the clustered-light workload.

Material textures are dynamic shader resources. This is required because a
mutable Vulkan descriptor set cannot be rewritten while an earlier submitted
frame still uses it. Frame rendering also stops before issuing graphics work
when resize/minimization has removed the HDR target.

## Consequences

- Lighting preparation and capacity policy are testable without a GPU.
- HDR, exposure, local lights, cascaded shadows and tone mapping have explicit
  pass and resource boundaries.
- GPU timing is asynchronous and may represent an older completed frame.
- The current CPU cluster builder and per-object shadow submission favor
  clarity. Compute light assignment, indirect shadow draws, PCF filtering,
  stable texel snapping and cached static shadows are future optimizations.
- The renderer now has the frame history and feature boundary needed for a
  later TAA/FSR/DLSS integration, but no vendor upscaler is linked yet.
