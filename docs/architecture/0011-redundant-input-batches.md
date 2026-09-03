# ADR 0011: Redundant unreliable input batches

## Status

Accepted as bandwidth and loss-resilience phase A

## Context

Movement commands are intentionally sent unreliably because retransmitting an
old axis state behind newer traffic adds latency without improving gameplay.
However, a one-shot action such as jump must not depend on one datagram: losing
that packet previously removed the action completely.

GameNetworkingSockets explicitly permits unreliable messages to be dropped,
duplicated or delivered out of order. Valve's Source SDK also represents a
received player update as a collection of user commands and records dropped
packets, establishing command batching as a proven application-layer pattern.

## Decision

Protocol version 3 changes movement input payloads from one command to a bounded
batch:

- Every new datagram contains the newest input and up to three older inputs that
  remain unacknowledged by an authoritative snapshot.
- Redundancy is configurable from 1 to 32 commands and defaults to 4.
- Each command carries its own sequence, client simulation tick, axes and jump
  flag. Commands must be strictly sequence-ordered and the envelope identifies
  the newest command.
- The server discards commands already acknowledged or already seen before its
  next simulation tick.
- Continuous axes come from the newest accepted command. One-shot jump flags
  are accumulated across newly accepted commands and consumed once.
- The server acknowledges the newest applied sequence, allowing the client to
  remove all older prediction history and stop resending it.

The transport remains unreliable. This is bounded forward error recovery, not
a reliable ordered stream, so a new command is never held behind an old packet.

Client and server expose batch count, encoded command count, redundant command
count, duplicates, payload bytes and maximum batch size. These measurements
make the loss-resilience/bandwidth trade-off observable before introducing
quantization.

## Verification

- Schema tests round-trip single-command and multi-command batches and reject
  empty, malformed and incorrectly decoded batches.
- A deterministic integration test deliberately discards the only datagram that
  originally carries a jump. The following batch recovers that command, the
  server jumps once, acknowledges the newer sequence and clears client history.
- Existing adverse-link, real GameNetworkingSockets loopback and graphical
  Vulkan/Jolt tests exercise protocol version 3.

## Consequences

- Isolated and short bursts of packet loss no longer usually remove one-shot
  locomotion actions.
- Four full commands increase movement payload bandwidth. Current commands are
  deliberately uncompressed so telemetry can establish a baseline; bit packing
  and axis quantization belong to phase B.
- The aggregation rule is valid for continuous state plus idempotent one-shot
  actions. Gameplay abilities with parameters or ordering dependencies will
  need explicit event identifiers rather than additional boolean flags.
- Loss exceeding the configured redundancy window can still remove an action.
  Critical non-expiring events require a different reliability policy.

## References

- [Valve Source SDK: queued player user commands](https://github.com/ValveSoftware/source-sdk-2013/blob/master/src/game/server/player.h)
- [GameNetworkingSockets delivery guarantees](https://github.com/ValveSoftware/GameNetworkingSockets/blob/master/include/steam/isteamnetworkingsockets.h)
- [GameNetworkingSockets wire format and duplicate handling](https://github.com/ValveSoftware/GameNetworkingSockets/blob/master/src/steamnetworkingsockets/clientlib/SNP_WIRE_FORMAT.md)
