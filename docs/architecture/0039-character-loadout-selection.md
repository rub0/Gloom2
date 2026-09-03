# ADR 0039: Character/loadout selection is authoritative lobby state

## Status

Accepted for protocol v11.

## Context

Every player currently composes Hound with Soul Reaper and Bite. That default
was implicit in simulation and snapshots, leaving no safe pre-match seam for
additional characters or loadouts. Client-local choices cannot be allowed to
change component composition after the match begins or bypass the existing
two-player ready gate.

## Decision

Define a backend-neutral `SlicePlayerSelection` containing bounded character,
weapon and ability enums. Protocol v11 adds a typed lobby selection command and
includes the accepted tuple in each replicated `SliceLobbyPlayer`. The current
allowlist contains exactly Hound / Soul Reaper / Bite, which remains the
default and therefore does not change gameplay balance or composition.

Authority accepts selection only for a connected player while the lobby is
waiting. A changed selection clears that player's ready flag. Match start
requires the required connected records to have both a valid selection and
ready state. The remote client submits selection after clock synchronization,
waits until authoritative lobby state reflects the tuple, and only then sends
ready.

ADR 0043 later makes the initial choice explicitly interactive for human join
clients; the selection-before-ready authority ordering remains unchanged.

Reconnect preserves the existing lobby record. An active match does not accept
selection changes and continues using its original composition.

## Consequences

- Unsupported enum values and tuples fail decoding or authority validation.
- Selection is observable identically by both clients and the dedicated host.
- Ready cannot race ahead of a future changed loadout.
- Hound gameplay, prediction, abilities and protocol-v10 mechanism snapshots
  remain unchanged inside the new protocol-v11 envelope.
- The next roster milestone can map accepted tuples to component factories
  without redesigning admission or lobby replication.

## Validation

`gloom.match_lobby` round-trips selection commands and roster state, rejects an
unsupported character and verifies selection before ready. The deterministic
remote-slice test performs selection then ready for initial admission while
preserving active-match reconnect. The real two-client GNS regression exercises
the same exchange and counts gameplay time only after the lobby becomes active.
