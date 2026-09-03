# ADR 0041: Selected abilities share one authoritative policy slot

## Status

Accepted for protocol v11.

## Context

Selection and snapshots could identify an ability, but the entity component and
activation function were still named for Bite. Treating every active ability as
a forward charge would make a second option unsafe, while creating a separate
network command per ability would duplicate ownership and sequencing policy.

## Decision

Replace `HoundAbilityComponent` with `AbilityComponent` and route Q/RMB through
one authoritative `activate_ability` policy selected by
`CharacterLoadoutComponent`. The existing reliable command, entity ownership,
sequence checks, cooldown state and authoritative HUD reconciliation remain
shared.

Bite keeps its 30-tick charge, lag-compensated one-hit resolution, 40 damage,
40 life steal and 1,500-tick cooldown. Only Bite may override movement from the
generic active state.

Add Guard as the first second policy. It immediately grants 50 shield, bounded
by the existing 150 maximum, sets a 60-tick presentation cue and starts a
900-tick cooldown. It does not move the character, query combat history, damage
another entity or affect kill attribution. `hound-guard` selects it;
`hound-reaper` remains the explicit empty slot.

## Consequences

- Ability transport and ownership no longer encode Bite-specific behavior.
- Cooldown fractions use the selected policy's duration.
- Active-state presentation can distinguish orange Bite from blue Guard.
- Guard reuses existing shield absorption and replication without new wire
  fields.
- New abilities still require an explicit server policy; unknown enum values or
  tuples remain invalid.

## Validation

`gloom.vertical_slice` requires Guard to grant exactly 50 shield, enter its
active/cooldown state and increment authorized ability metrics. Existing tests
retain Bite damage, life steal and 25-second reuse plus empty-slot rejection.
`gloom.vertical_slice_network` selects Guard for one real remote composition,
activates it and requires reciprocal snapshots plus the authoritative shield.
Lobby, GNS, respawn and Vulkan suites continue through the shared command path.

ADR 0046 later gives Bite and Guard cooked presentation props without changing
any authoritative policy in this decision.
