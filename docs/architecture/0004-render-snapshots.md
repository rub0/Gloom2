# ADR 0004: Immutable render snapshots

## Status

Accepted

## Context

The renderer originally owned a procedural triangle. Connecting gameplay and
physics directly to Diligent would make both simulation and graphics difficult
to replace, test or schedule on different threads.

## Decision

Gloom owns a small scene description made of camera data, opaque mesh handles,
transforms, colors and render instances. A frame is submitted to `Renderer` as
a read-only `RenderSnapshot` whose storage remains owned by the caller for the
duration of the draw call.

The first mesh is a backend-owned indexed cube. Diligent owns its GPU buffers,
pipeline state, shaders and per-draw constant buffer. No Diligent or Jolt type
appears in the scene API.

Physics poses are copied into previous/current render transforms after fixed
simulation steps. Rendering uses normalized quaternion interpolation and the
physics accumulator fraction, smoothing motion when rendering occurs between
simulation ticks.

## Consequences

- Renderer inputs can later be produced on a worker thread and double-buffered.
- The current immediate span is deliberately non-owning; the renderer must not
  retain it after `draw` returns.
- The built-in cube and flat color are scaffolding. Asset-backed mesh and
  material registries can replace them without changing physics.
- Draw submission is currently one call per instance. Instancing, sorting and
  batching will be measured and introduced when scene scale requires them.
