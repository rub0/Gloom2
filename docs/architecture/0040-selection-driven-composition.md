# ADR 0040: Accepted selection drives character components and snapshots

## Status

Accepted for protocol v11.

## Context

ADR 0039 replicated a validated pre-match tuple, but gameplay still implicitly
treated every entity as Hound / Soul Reaper / Bite. Adding an unimplemented
fictional character would conceal that gap. The first alternative instead needs
to demonstrate that accepted selection changes authoritative behavior without
changing stable physics, weapon or respawn contracts.

## Decision

Add `CharacterLoadoutComponent` to Hound composition and bind each authoritative
lobby selection to the existing entity before the ready-gated match starts.
Derive `CombatantView::character`, `weapon` and `ability` from that component and
encode all three in gameplay snapshots. Client replicated entities apply the
same roster selections.

Keep Hound / Soul Reaper / Bite as the default. Add Hound / Soul Reaper / none
as the first alternate tuple, exposed by the development CLI name
`hound-reaper`. Ability `none` disables activation and hit resolution, clears
cooldown/active state, reports no active ability and renders HUD readiness as
zero. It does not substitute an unreviewed mechanic.

Both tuples share the same `CharacterPhysicsComponent`, movement rules, health,
Soul Reaper damage and death/respawn lifecycle. Selection never recreates a
character body and remains immutable once the lobby becomes active.

Soul Reaper remains the 30-tick LMB weapon for both tuples and has an explicit
first-person body/barrel plus cooldown-driven muzzle feedback. Bite retains its
1,500-tick cooldown. The remote client gates ability input from authoritative
HUD readiness and uses only a pending-command latch until a newer snapshot;
this prevents burst duplicates without retaining a stale deadline after the
server resets cooldowns on respawn.

## Consequences

- Lobby choice now changes authoritative gameplay rather than presentation
  alone.
- Snapshots let every client verify the composed weapon and ability.
- The alternate loadout is intentionally weaker but provides a safe end-to-end
  seam for future abilities.
- No-ability input cannot increment authorized ability metrics or produce Bite
  damage/life steal.
- ADR 0041 subsequently replaces Hound-specific plumbing without another lobby
  or snapshot protocol redesign.

## Validation

`gloom.vertical_slice` applies the alternate tuple, attempts activation and
requires no active ability, zero HUD readiness and no authorized command.
`gloom.match_lobby` changes a ready player's tuple and verifies that ready is
cleared. `gloom.vertical_slice_network` admits one alternate-loadout client and
requires both reciprocal snapshots to identify its `none` ability. Existing
physics, kill/respawn, reconnect, GNS and Vulkan tests remain unchanged.
