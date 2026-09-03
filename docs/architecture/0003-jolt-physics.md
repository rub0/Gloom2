# ADR 0003: Jolt physics backend and fixed simulation step

## Status

Accepted

## Context

Gameplay and networking need a stable simulation boundary that does not depend
on the rendering frame rate. Jolt supports parallel simulation, but its types,
body identifiers, global initialization and job system should not become part of
Gloom's public API.

## Decision

Gloom owns the types in `gloom::physics`, including body handles, transforms,
shape descriptions and simulation statistics. `JoltWorld` translates those
types to Jolt 5.6 internally through a private implementation.

Simulation uses an accumulator and a configurable fixed step, initially 60 Hz.
A frame may execute at most four substeps by default so a stalled frame cannot
create an unbounded simulation backlog. The remaining fraction is exposed as an
interpolation alpha for a future render snapshot system.

Jolt's `JobSystemThreadPool` is the initial scheduler. A worker count of zero in
Gloom settings means automatic selection of all detected hardware threads except
the calling thread. The scheduler remains a backend detail so it can later be
adapted to a shared engine job system.

## Consequences

- Rendering, gameplay and networking never include Jolt headers.
- Physics progresses deterministically with respect to the number of fixed
  steps, independently of render-frame timing.
- Static, dynamic and kinematic bodies and box/sphere shapes form the initial
  narrow API; more shapes will be added when ported gameplay requires them.
- The next rendering milestone must consume immutable/interpolated physics
  transforms rather than reaching into Jolt directly.
