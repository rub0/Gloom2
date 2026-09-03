# ADR 0026: Two-player dedicated vertical slice

## Status

Accepted.

## Context

The first remote slice admitted one player and kept the second combatant under
server control. That proved the direct-IP path, but it could not expose
connection isolation, reciprocal client perspectives or lifecycle behavior
between two real players. Running authority inside a spectator window also
coupled server operation to SDL and Vulkan availability.

## Decision

`VerticalSliceSimulation` accepts an input for each of its two combatants while
retaining the single-input overload used by the local AI slice. The remote host
stores movement, aim, fire, acknowledgement and snapshot sequence state per
connection. It advances both inputs in one authoritative tick and emits one
snapshot per active connection, ordered so that the receiver's owned entity is
always `player` and the other entity is `opponent`.

The session layer remains the sole source of connection-to-entity ownership.
When a connection drops, its input state is removed immediately and the entity
becomes dormant. A client reconnects with its rotated resume token within the
existing ten-second grace period, receives the same entity, and rebuilds
prediction from its last known authoritative state. The graphical join mode
retries the endpoint after one second and uses this resume path automatically.

`gloom_slice_server` is a console composition root containing only the GNS
transport and the transport-independent remote host. It runs the authoritative
slice at 60 Hz, supports two players, shuts down on Ctrl+C and accepts a bounded
`--ticks` mode for automation. No platform, physics-backend or renderer API is
introduced into gameplay or networking.

## Consequences and limits

- Two separate GNS clients can now move, jump, fire, die and respawn against
  each other with reciprocal HUD state.
- A disconnect cannot leave stale movement or fire commands active.
- Reconnection preserves the combatant and world state only while the same
  server process retains the dormant session.
- The fixed development credential and deterministic resume tokens are still
  unsuitable for an Internet-facing production service.
- ADR 0027 adds a two-player lobby and abandonment outcome. Matchmaking,
  persistence across server restarts and more than two combatants remain future
  work.

## Validation

`gloom.vertical_slice_network` covers two isolated clients, reciprocal
snapshots, independent movement, disconnect and same-entity resume. It also
conditions protocol-v7 gameplay traffic with 45 ms one-way latency, 18 ms
jitter, 5% loss, duplication and reordering; the deterministic run must retain
movement and kill/respawn while keeping its maximum correction below 0.75 m.
`gloom.vertical_slice_transport` connects two real GNS clients to one loopback
host. `gloom.slice_server_smoke` starts and cleanly stops the headless server.
