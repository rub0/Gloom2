# ADR 0032: Hound logical character composition

## Status

Accepted.

## Context

After Factory became a logical entity composition, each Hound still stored all
gameplay state in a private monolithic `Combatant` structure. Transform state
lived in the movement server, the Jolt trigger proxy was a separate anonymous
entity, and authority, weapon, ability, health and presentation lifetimes could
not be reused independently.

The character controller itself is replaced by ADR 0033. This decision records
the component composition that provided its stable migration boundary.

## Decision

`compose_hound_character` creates one generational logical entity containing:

- `TransformComponent`;
- `CharacterPhysicsComponent` (migrated by ADR 0033);
- `HealthComponent` and `ShieldComponent`;
- `CharacterMovementComponent`;
- `WeaponComponent` and `AbilityComponent` (generalized by ADR 0041);
- `AuthorityComponent`;
- `ReplicationComponent` (added by ADR 0034);
- `CharacterPresentationComponent` and `ScoreComponent`.

The vertical slice's `Combatant` is now only a short-lived handle into these
registry-owned components. It owns no gameplay state. Combat, respawn, HUD,
ability and session routing mutate or read the relevant components. The
authoritative movement result is synchronized into `TransformComponent` each
tick.

When Jolt authority is present, ADR 0033 attaches the `CharacterPhysicsComponent`
to a virtual capsule owned by the same Hound entity. The former provisional
kinematic box is no longer used.

## Consequences and limits

- Character state has a reusable entity/component lifetime and composition
  recipe independent of the vertical-slice implementation.
- Authority and physics identity now belong to the same logical Hound entity.
- Existing network protocol, prediction, combat balance, HUD and respawn
  behavior are unchanged.
- ADR 0034 derives presentation/combat movement snapshots from component state
  and applies received frames under explicit authority policy.
- ADR 0033 completes the pending physics migration without changing replication.
- ADR 0042 renames the reusable recipe to `compose_slice_character` and applies
  it to Berserker without changing this original Hound composition decision.

## Validation

`gloom.vertical_slice` verifies the complete composition recipe and its initial
authority, spawn and migrated-physics policy, then runs the existing combat,
collision, lava damage, death and respawn regressions. Local, host and dedicated
smoke paths continue to exercise the same composition.
