# ADR 0010: Character prediction and collision policy

## Status

Accepted for the character gameplay foundation

## Context

The interactive laboratory initially predicted planar movement while Jolt
provided only its physical presentation. Adding jumping or obstacles solely to
Jolt would make the client predict rules the authoritative server did not run,
causing unavoidable corrections and making tests dependent on a particular
physics backend.

At the same time, attempting to rewind the complete multithreaded Jolt world at
this stage would add substantial state capture and determinism requirements
before the game has established which interactions actually need prediction.

## Decision

Gloom uses a selective prediction policy:

- The local player's locomotion is deterministic shared simulation used by the
  authoritative server and predicting client.
- Its replicated state contains three-dimensional position and velocity plus
  grounded state. Input contains planar axes and a one-shot jump action.
- The shared simulation owns gravity, jump impulse, ground support and a small
  backend-neutral set of static axis-aligned obstacles. Horizontal collision
  uses a character-radius test and vertical movement can land on obstacle tops.
- The Jolt `CharacterVirtual` adapter mirrors predicted horizontal velocity and
  jump impulses against equivalent scene geometry. It does not define network
  truth.
- Dynamic rigid-body interactions are server-authoritative and are not locally
  predicted yet. A later gameplay feature may opt into deterministic prediction
  or selective rollback only when its responsiveness requires it.

Protocol version 2 records the incompatible addition of vertical state,
grounded state and jump input.

Reconciliation always updates canonical simulation immediately. Presentation
keeps a decaying inverse offset so small corrections remain visually continuous;
the default half-life is 100 ms. Corrections of 1.5 metres or more snap instead
of hiding a major divergence. Counts, last/maximum/accumulated correction
distance, smoothed corrections and snaps are exposed as telemetry.

This follows the established server-authoritative model described by Valve:
the client and server run the same rules for predictable local actions, the
server remains final authority, and small prediction errors may be corrected
gradually in presentation. Blizzard's Overwatch architecture is an additional
reference for using deterministic command-to-state simulation selectively to
obtain responsiveness and precision.

## Verification

- Wire tests round-trip jump, vertical position/velocity and grounded state.
- Deterministic tests verify that a grounded character is blocked by a static
  obstacle, can jump over a low obstacle and lands again.
- Presentation tests verify continuity, half-life decay, snap thresholds and
  telemetry.
- Jolt tests verify a supported character receives a positive jump impulse.
- The adverse-network test continues to converge after reconciliation and the
  graphical smoke test executes scripted jumps around Jolt obstacles.

## Consequences

- Locomotion remains testable without SDL, Jolt, rendering, sockets or wall
  clock timing.
- Static collision data must be generated consistently for gameplay prediction
  and the Jolt scene; asset cooking should produce both representations from one
  source rather than maintaining them manually.
- The current obstacle solver is intentionally narrow and must not grow into a
  second general-purpose physics engine.
- A lost packet can currently lose a one-shot jump action. Redundant input
  batches are therefore the first requirement of the bandwidth/scale milestone.
- Moving platforms, pushes and other dynamic interactions remain authoritative
  and may visibly correct the local player until a feature-specific strategy is
  implemented.

## References

- [Valve: Source Multiplayer Networking](https://developer.valvesoftware.com/wiki/Source_Multiplayer_Networking)
- [Valve: Latency Compensating Methods in Client/Server Protocol Design](https://developer.valvesoftware.com/wiki/Latency_Compensating_Methods_in_Client/Server_In-game_Protocol_Design_and_Optimization)
- [GDC Vault: Overwatch Gameplay Architecture and Netcode](https://gdcvault.com/play/1024001/-Overwatch-Gameplay-Architecture-and)
