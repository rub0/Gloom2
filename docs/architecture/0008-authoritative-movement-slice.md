# ADR 0008: First authoritative movement slice

## Status

Accepted as a headless vertical slice

## Context

The transport, wire envelope and reusable prediction/snapshot primitives need
to be exercised together before coupling them to a player controller, Jolt or
rendering. Otherwise transport timing, reconciliation mistakes and physics
integration failures become difficult to distinguish.

## Decision

The first slice implements planar kinematic movement as a small deterministic
reference simulation:

- The server advances at 60 Hz and emits full snapshots at 20 Hz.
- A client creates sequenced normalized movement inputs and predicts its local
  entity immediately.
- The server accepts only newer input state, owns canonical entity movement and
  acknowledges the newest input applied at a fixed tick.
- The client ignores duplicate or out-of-order snapshot sequences, restores its
  local authoritative state and replays only pending inputs.
- Other entities enter the delayed snapshot buffer and are interpolated by
  server time. They are never fed into local prediction.
- Input and world-state payloads use explicit little-endian fields and validate
  type, size, finite floats, normalized axes, entity count and duplicate IDs.

The reference movement model is intentionally independent of Jolt. Its purpose
is to prove protocol semantics deterministically. A later interactive slice
will adapt the same input/state contracts to a dedicated Jolt character
controller without making the network layer depend on Jolt types.

Clock synchronization uses the standard four-timestamp offset and round-trip
calculation. The estimator gives high-delay samples less weight to reduce bias
from transient asymmetric queues. Rendering converts client time to estimated
server time before applying the interpolation delay.

## Verification

The end-to-end test runs ten seconds of local and remote movement with:

- 45 ms base one-way latency;
- 18 ms jitter;
- 5% loss;
- 2% duplication;
- 10% explicit reordering;
- a 32 KiB/s link and bounded queues.

It then drains both directions and requires the predicted client to converge
to the authoritative state with no acknowledged input left in history. It also
requires hundreds of interpolated remote samples and observed packet loss.
Separately, typed movement inputs and snapshots cross a real GNS loopback
connection using reliable and unreliable delivery respectively.

## Consequences

- The protocol path is now tested from typed input through serialization,
  adverse delivery, authoritative simulation, acknowledgement, reconciliation
  and remote interpolation.
- Current inputs describe axis state. A lost change is corrected by the next
  newer input; redundant input batches are still desirable for burst loss.
- Full snapshots are deliberately simple. Relevance, baselines, deltas and
  quantization remain later bandwidth work.
- The next slice can focus on SDL input, Jolt character movement and a visible
  local/remote scene instead of debugging the underlying network algorithm.
