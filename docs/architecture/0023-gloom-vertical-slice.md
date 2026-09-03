# ADR 0023: Gloom gameplay vertical slice

## Status

Accepted.

The camera, aim and session limits below were subsequently replaced by
[ADR 0024](0024-fps-authoritative-slice.md).

## Context

The modern engine foundations were validated in isolation, but they did not yet
prove that recognizable Gloom gameplay could cross platform input, deterministic
simulation, physics representation, immutable presentation and Vulkan rendering
in one loop. Porting every legacy subsystem before exercising that loop would
delay feedback and risk preserving obsolete implementation boundaries.

The legacy project is multiplayer-only. Its archetype data defines 120 default
life, 250 maximum life, 150 maximum shield, 70 percent shield absorption and a
four-second respawn. The base Soul Reaper uses no conventional ammunition and
defines 80 primary damage, 15 units of range and a 0.5-second cooldown.

## Decision

Milestone 20 is a deliberately narrow, locally playable combat slice:

- one player and one deterministic hostile combatant in a compact arena;
- shared fixed-tick movement and static-obstacle collision;
- Soul Reaper hitscan, cooldown, occlusion, damage, death, kill credit and
  respawn using the audited legacy values;
- WASD/arrow movement, Space jump and left mouse primary fire;
- snapshot-derived life, weapon-readiness and hit feedback bars;
- a deterministic graphical smoke path that completes a kill/respawn loop.

`VerticalSliceSimulation` owns gameplay state and depends only on Gloom's
backend-neutral movement contract. It never includes SDL, Jolt, Diligent or
Vulkan types. SDL translates devices into `SliceInput`; Jolt owns matching arena
geometry; render instances are built from the immutable `SliceSnapshot`.

The opponent is a test combatant rather than invented single-player campaign
AI. It stands in for a remote player until the slice is moved onto the existing
authoritative session and lag-compensated combat path.

## Consequences and limits

- Gloom now has a short executable gameplay loop instead of only technology
  laboratories.
- Legacy balance constants have executable regression coverage.
- Gameplay can be tested headlessly and the same state can be presented by
  different platform, physics and renderer adapters.
- The arena is inspired by the spatial rhythm of the legacy dungeon data but is
  not a direct coordinate conversion; legacy units and assets need a separate
  content migration policy.
- Primary aim currently locks to the only opponent. Mouse look, character
  selection, abilities, authored HUD, audio and network-session integration are
  intentionally outside this first slice.

## Validation

`gloom.vertical_slice` checks audited constants, occlusion, shield absorption,
cooldown enforcement, HUD readiness, death, kill credit and four-second
respawn. `gloom.vertical_slice_smoke` renders 360 fixed frames through Vulkan,
uses SDL and Jolt adapters and fails unless it observes both a kill and the
opponent's respawn.
