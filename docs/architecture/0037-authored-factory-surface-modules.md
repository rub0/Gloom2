# ADR 0037: Factory surface modules are selected presentation assets

## Status

Accepted.

## Context

ADR 0036 introduced one authored mechanism without allowing asset data to own
gameplay. Factory's floor and industrial records still used the builtin cube,
even though their material classification already provided a stable seam for a
modular presentation pass. The lava channel also makes it important that a
visual replacement cannot close collision gaps or alter damage coverage.

## Decision

Cook `assets/factory/surface_modules.gltf` during the normal Factory content
build. It exposes distinct floor-panel and industrial-module mesh identities in
one resident scene. The vertical-slice presentation records the visual indices
for `ArenaMaterial::floor` and `ArenaMaterial::industrial`, retains builtin cube
meshes as startup/failure fallbacks and substitutes the corresponding cooked
mesh only after the scene exposes a resident generation.

Authored geometry is normalized and scaled onto the existing presentation
record. `ArenaBox` remains the sole source of deterministic dimensions and
authoritative static collision. Trim records and the one-sided lava
`ArenaSurface` are outside this replacement. The lava entity's Jolt sensor and
`DamageVolumeComponent` are never derived from render instances.

## Consequences

- One cooked package supplies reusable floor and industrial module identities.
- Asset loading and GPU failure cannot remove authoritative collision.
- Local, host and joining clients select modules from identical immutable arena
  metadata without adding protocol state.
- The floor channel and hazard coverage stay exact even if authored geometry is
  revised.
- Trim and lava presentation remain explicit work for the next milestone.

## Validation

The build must cook `surface_modules.gasset` before linking the playable
executable. The 360-frame vertical-slice Vulkan smoke requires a nonzero
resident module generation in addition to its kill/respawn loop. Existing
headless vertical-slice tests continue to validate arena collision, the open
channel and Jolt-backed lava damage independently of rendering.
