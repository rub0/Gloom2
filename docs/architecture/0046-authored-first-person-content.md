# ADR 0046: First-person combat props are residency-safe presentation

## Status

Accepted.

## Context

After authored Factory and roster content, the first-person Soul Reaper and
Hound ability feedback still used builtin cubes. These are the remaining
explicit artistic blockouts in the current playable slice; reticles, hit
markers and bars are procedural UI rather than world content.

## Decision

Version and cook two source scenes through `gloom_project_content`:

- `weapons/soul_reaper.gltf` contains ordered body and barrel meshes;
- `abilities/hound_abilities.gltf` contains ordered Bite-claw and Guard-plate
  meshes.

Each scene owns its catalog, asynchronous loader, residency coordinator, ticket
and generation. Swap its two mesh IDs only when both instances are resident.
Loading, malformed or failed scenes retain both builtin fallbacks.

Use the Soul Reaper meshes in the existing FPS transforms and preserve the
authoritative cooldown-driven muzzle flash. During active Bite, use the claw
mesh for both hands. During active Guard, use the plate mesh for paired blue
shield props. Snapshot state alone selects these modes; asset state cannot
activate commands or change combat.

## Consequences

- Every identified world/character/weapon/ability blockout in the current slice
  now has a cooked-content path.
- Weapon and ability content can fail independently of characters and Factory.
- Procedural HUD elements remain renderer-owned and need no asset residency.
- Replacing these primitive glTF meshes cannot alter hitscan, damage, shield,
  cooldowns, movement, physics or replication.

## Validation

The normal build cooks both two-instance scenes. Gameplay tests validate their
distinct source/cooked slots. The Vulkan vertical-slice smoke requires both GPU
generations in addition to Factory and roster content; the existing weapon,
Bite, Guard, network, death and respawn regressions remain unchanged.
