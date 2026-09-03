# ADR 0012: Quantized delta snapshots and connection relevance

## Status

Accepted as bandwidth and scale phase B

## Context

Full floating-point state for every entity was simple and robust, but its cost
grew linearly with the complete world even when most values were unchanged or
irrelevant to one client. Input acknowledgements already provide a return path
that can also identify which authoritative snapshot a client has reconstructed.

Valve documents delta compression against the last acknowledged update, with
full snapshots used initially or as recovery. Large established engines also
filter relevance per connection and prioritize important actors when bandwidth
cannot carry every update. Gloom needs those policies behind its own protocol
rather than inheriting them from a rendering or transport backend.

## Decision

Protocol version 4 adds connection-specific snapshot baselines:

- Every input batch acknowledges the newest snapshot successfully reconstructed
  by that client.
- The server retains a bounded history and encodes against that exact baseline.
  If it is unavailable, it sends a self-contained full snapshot.
- Delta records carry entity creation, removal and a seven-bit field-change
  mask. Unchanged entities require no record.
- Positions are signed 32-bit fixed point at 1/256 metre. Velocities are signed
  16-bit fixed point at 1/128 metre per second. Grounded state uses one bit of
  semantic information in a validated byte.
- Decoding reconstructs a complete immutable `WorldSnapshot` before prediction
  reconciliation or interpolation sees it. Those systems do not depend on the
  wire representation.

Per-connection selection always includes the controlled entity. Other entities
may be culled by a configurable planar relevance radius, then sorted by distance
and capped by a configurable entity budget. Entity IDs provide a stable tie
break. A zero radius means unlimited distance for small scenes.

Both endpoints count full/delta snapshots, baseline misses, considered entities,
encoded records and payload bytes. This supplies a measurable baseline for
future spatial indexing and adaptive budgets.

## Verification

- Codec tests change one entity, remove another and create a third, then rebuild
  the exact logical snapshot from its baseline within the documented
  quantization error.
- A delta must be smaller than the equivalent full snapshot and cannot decode
  without its named baseline.
- Relevance tests prove the controlled entity and nearest relevant entity win a
  two-entity budget while distant entities are excluded.
- A client acknowledgement makes the next server snapshot a delta.
- The adverse-link integration test exercises both full recovery and delta paths.
- The graphical laboratory prints the observed full/delta mix, records and bytes.

## Consequences

- Position error is bounded to half a quantization step (about 1.95 mm) and
  velocity error to about 3.91 mm/s.
- Delta loss does not corrupt later state: the server continues referencing the
  last baseline actually acknowledged by the client.
- History is currently stored as complete snapshots for clarity. Many clients
  will eventually require per-connection storage budgeting or shared immutable
  world frames.
- Distance sorting still scans all entities. A spatial replication graph is the
  next scaling step once profiling shows this O(clients × entities) pass matters.
- Distance alone is not sufficient gameplay priority. Ownership, teams, recent
  events, starvation prevention and view direction remain future policies.

## References

- [Valve: Source Multiplayer Networking](https://developer.valvesoftware.com/wiki/Source_Multiplayer_Networking)
- [Epic: detailed actor replication flow](https://dev.epicgames.com/documentation/unreal-engine/detailed-actor-replication-flow-in-unreal-engine)
- [Epic: actor relevancy](https://dev.epicgames.com/documentation/unreal-engine/actor-relevancy-in-unreal-engine)
- [Epic: actor priority](https://dev.epicgames.com/documentation/en-us/unreal-engine/actor-priority-in-unreal-engine)
