# ADR 0007: Authoritative network simulation model

## Status

Accepted in phases

## Context

The original Gloom already generated fixed-tick snapshots, buffered remote
transforms and attached sequence numbers to control messages. Its transport
manager nevertheless sent almost all traffic reliably on one ENet channel.
Client acknowledgement/reconciliation was unfinished, interpolation operated
directly on transform matrices, and there was no versioned wire envelope or
repeatable adverse-network test layer.

Gloom now uses Jolt and floating-point simulation. Requiring deterministic
lockstep across future platforms would couple every gameplay and physics choice
to cross-machine determinism. It would also make every participant wait for the
slowest input stream. The game instead needs responsive local control while an
authoritative server remains the source of truth.

## Decision

Gloom uses an authoritative client/server model:

- The server advances gameplay at a fixed simulation tick and owns canonical
  state.
- Clients send monotonically sequenced input commands. The local player may
  predict those commands immediately.
- Authoritative player state acknowledges the last processed input. A client
  corrects to that state and replays only newer inputs.
- Remote entities use a delayed snapshot buffer and interpolation. Buffer
  underruns hold the latest state; unbounded blind extrapolation is forbidden.
- Replaceable state snapshots are unreliable and discarded when stale. Ordered
  gameplay events, session setup and lifecycle messages are reliable.
- Simulation correction and visual smoothing are separate. Physics state is
  corrected authoritatively; render-space error can decay without inventing a
  false physics state.

The first implementation phase adds four backend-independent primitives:

1. A little-endian `GLOM` wire envelope with protocol version, message kind,
   simulation tick, sequence, acknowledged sequence and bounded payload.
2. Wrap-aware comparison for 32-bit sequence numbers.
3. A sorted, capacity-bounded snapshot buffer with explicit exact,
   interpolated and held results.
4. A prediction history that drops acknowledged inputs and deterministically
   replays pending inputs from an authoritative correction.

`NetworkSimulator` is a deterministic simulated-time laboratory. It can apply
latency, symmetric jitter, loss, duplication, reordering, bandwidth limits and
queue limits without sleeping or reading wall-clock time. A fixed seed makes
failures reproducible.

## Consequences

- Game protocol semantics remain independent of GameNetworkingSockets.
- Invalid magic, versions, kinds, lengths and configured payload limits are
  rejected before gameplay deserialization.
- Snapshots can arrive out of order; duplicate ticks/timestamps are rejected.
- Prediction is generic, but its simulation callback must match the server's
  player movement rules closely enough for useful replay.
- Overflow in prediction history is observable and must trigger recovery (for
  example, a full authoritative reset) in the future session layer.
- Clock synchronization, entity schemas, baseline/delta compression, relevance
  prioritization and lag-compensated hit validation remain later phases.

## References

- Yahn Bernier, [Latency Compensating Methods in Client/Server In-game Protocol
  Design and Optimization](https://www.gamedevs.org/uploads/latency-compensation-in-client-server-protocols.pdf)
- Glenn Fiedler, [Snapshot Interpolation](https://www.gafferongames.com/post/snapshot_interpolation/)
- Glenn Fiedler, [State Synchronization](https://www.gafferongames.com/post/state_synchronization/)
- Timothy Ford, [Overwatch Gameplay Architecture and Netcode](https://gdcvault.com/play/1024001/-Overwatch-Gameplay-Architecture-and)
- [Original Gloom repository](https://github.com/rub0/Gloom)
