# ADR 0042: Berserker enters the roster through an evidence-bounded foundation

## Status

Accepted for protocol v11.

ADR 0063 resolves this slot in protocol 13: retain wire ID 1 and its accepted
tuple for compatibility, present it as Hound and remove it from the offered
original roster. Archangel/Shadow use new IDs; this historic ID is not recycled.

Historical correction (2026-09-02): ADR 0061 audits the original repository and
finds Berserker as a Hound ability, not a separate legacy class. The modern tuple
below remains a provisional implementation; its identity must be resolved before
final art recovery. This note does not change protocol IDs or runtime behavior.

## Context

The roadmap names Berserker as a legacy class, but this repository contains no
Berserker implementation, archetype record, ability ownership or numerical
balance data. The existing selection protocol can safely represent another
character, provided authority accepts only a composition the runtime actually
implements.

## Decision

Add `SliceCharacter::berserker` and accept exactly Berserker / Soul Reaper /
none. Soul Reaper is already an authoritative shared weapon; the empty ability
slot avoids assigning Bite, Guard or an invented mechanic to Berserker.

Rename the entity recipe to `compose_slice_character` and its completeness
check to `has_complete_character_composition`. Berserker uses the same component
set, Jolt capsule, movement, default life, weapon, death/respawn, authority and
replication paths as Hound. These are foundation defaults, not reconstructed
legacy Berserker balance.

Expose the tuple as `berserker-reaper`. Lobby logs and reciprocal gameplay
snapshots retain the selected identity. Primitive presentation uses a distinct
dark armor palette so the replicated choice is visible, without changing the
hit volume or physics.

ADR 0044 subsequently replaces that recolored Hound proxy with a dedicated
upright Berserker fallback selected through a roster-neutral recipe.

## Consequences

- Berserker can join, move, fire, die, respawn and reconnect through the real
  multiplayer path.
- Hound / Bite and Hound / Guard remain unchanged.
- Berserker / Bite and Berserker / Guard are rejected by the tuple allowlist.
- Future Berserker balance or abilities require audited source evidence and an
  explicit authoritative policy.

## Validation

Lobby tests round-trip Berserker and reject its unaudited Hound ability tuples.
The component test composes a complete Berserker entity. The two-client network
test selects Berserker for one player and requires both reciprocal snapshots to
identify it while combat, Guard, respawn and reconnect regressions continue.
