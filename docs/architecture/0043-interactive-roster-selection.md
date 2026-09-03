# ADR 0043: Roster choice requires explicit client confirmation

## Status

Accepted for protocol v11.

## Context

The lobby protocol already ordered selection before ready, but the executable
derived the tuple only from a command-line argument and submitted it
automatically. Players could not inspect the validated roster or deliberately
confirm a choice, and authoritative choices were visible only in console logs.

## Decision

When join mode has no loadout argument, expose a four-entry carousel containing
Hound/Bite, Hound/Guard, Hound/no ability and Berserker/no ability. Left and
Right change the pending tuple; Enter confirms it. Admission and clock sync may
finish while this UI is open, but the remote adapter sends no selection or
ready message before confirmation.

After confirmation, send the reliable selection command. Send ready only after
a newer authoritative lobby state contains the exact desired tuple. Freeze the
pending tuple after confirmation and once the match leaves waiting phase.

Use the SDL window title as the current text UI: before confirmation it names
the pending tuple and controls; afterward it lists each authoritative lobby
identity, tuple and ready state. An explicit CLI loadout remains automatically
confirmed for deterministic tests and unattended clients.

## Consequences

- Human players choose without memorizing loadout argument names.
- UI state cannot bypass the server tuple allowlist or ready gate.
- Both player choices are visible from replicated lobby state.
- Reconnect retains the confirmed tuple and existing active-match policy.
- A future rendered menu can replace the title surface without changing the
  remote-client state machine.

## Validation

The remote network test completes admission and clock sync with automatic
selection disabled, verifies that no selection is emitted, changes the pending
tuple, confirms it, rejects later changes and requires authoritative echo before
the ready command. Existing lobby, GNS transport and SDL smoke tests cover the
same protocol and platform paths.
