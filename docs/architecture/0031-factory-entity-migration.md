# ADR 0031: Factory entities, surfaces and authoritative damage volumes

## Status

Accepted.

## Context

The Factory blockout represented lava as a thin `ArenaBox`. The renderer drew
all six cube faces and also created a Jolt body for every visual record. Damage
logic could therefore only be coupled to map metadata or duplicated as manual
overlap tests, neither of which matches the reusable entity/component model in
ADRs 0029 and 0030.

## Decision

Factory separates box geometry from one-sided surfaces. Its continuous floor is
split around a physical channel and the lava is an `ArenaSurface` rendered with
the built-in upward-facing quad mesh. Static map presentation is submitted
directly to rendering and no longer creates visual-only Jolt bodies.

The authoritative Factory composition creates logical entities for its static
collision bodies. The lava entity composes a Jolt-backed `TriggerComponent`
with a renderer-independent `DamageVolumeComponent`. Gameplay drains Jolt
enter/stay/exit events after each fixed physics step and applies the volume's
damage policy to the owning combatant. Environmental deaths do not award a kill.

This decision initially used a kinematic Jolt proxy. ADR 0033 replaced it with
the entity-owned `CharacterPhysicsComponent` capsule, and ADR 0034 made Jolt
Factory/character composition consistent across every authoritative path.

## Consequences

- Lava visibility is one-sided and lies below the surrounding floor.
- Trigger detection and contact lifetime come exclusively from Jolt callbacks.
- Visual map primitives no longer consume physics bodies.
- Damage-volume policy is reusable independently of Factory and Jolt types.
- ADR 0033 now supplies falling, stepping, slope and contact behavior through
  the migrated Jolt character contract.

## Validation

`gloom.vertical_slice` verifies that Factory exposes no lava box, exposes one
lava surface, crosses the channel with a Jolt-backed authoritative simulation
and observes damage. Existing combat, collision and respawn regressions remain
unchanged. The graphical slice smoke exercises the quad through Vulkan, and the
dedicated-server smoke starts the headless authoritative Jolt composition.
