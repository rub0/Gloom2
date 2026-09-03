# ADR 0025: Remote vertical-slice host/join

## Status

Accepted.

## Context

ADR 0024 composed the FPS slice with session ownership, prediction and
lag-compensated authority, but both clients still lived in one process. The
transport contract and GameNetworkingSockets backend already supported direct-IP
connections, so remote play should reuse those boundaries instead of moving GNS
types into gameplay or inventing a privileged input packet.

## Decision

Protocol v7 adds `gameplay_snapshot`, a fixed, validated wire representation of
`SliceSnapshot` carrying life, shield, death, cooldown, combatants and telemetry.
The envelope acknowledgement identifies the newest remote movement input applied
by the host.

`VerticalSliceRemoteHost` and `VerticalSliceRemoteClient` are transport-independent
adapters. The client performs the existing session handshake and clock exchange,
generates redundant `input_command` batches through `PredictedMovementClient`,
and sends the existing fire event with its owned shooter entity. The host admits
the connection, authorizes both command types through `ServerSessionManager`,
queues each unseen movement command, acknowledges it only after one authoritative
simulation step, rejects stale duplicate fire sequences, advances the same
vertical slice and publishes gameplay snapshots at 20 Hz. Position, velocity and
grounded state let the client reconcile its predicted player without resetting a
jump or discarding unsimulated prediction history.

The executable supplies only connection events and encoded bytes from
`network::Transport`. `--vertical-slice-host IP:port` listens and presents a
spectator view; `--vertical-slice-join IP:port` connects, captures the mouse and
controls the predicted player. The concrete GNS backend is therefore replaceable
without changing FPS, session or gameplay code.

## Consequences and limits

- The playable slice now crosses a real OS socket between separate processes.
- A spoofed shooter entity is rejected before it reaches gameplay.
- Host authority and client prediction have both deterministic and real-GNS
  loopback regression coverage.
- This milestone initially accepted one remote player and retained a
  server-driven opponent. ADR 0026 supersedes that limit with two reciprocal
  human clients while preserving this protocol-v7 boundary.
- The fixed `gloom-slice` development credential, deterministic resume tokens,
  absent discovery/NAT traversal and absent automatic reconnect are not
  production Internet security or lifecycle policy.
- Gameplay snapshots are full and small. Delta compression can be added behind
  the codec once migrated state grows.
- The server-driven opponent makes a fire decision at a slower cadence while
  retaining the audited Soul Reaper cooldown; this is temporary test-agent
  behavior, not a different weapon rule.

## Validation

`gloom.vertical_slice_network` wire-round-trips every message and checks session
admission, clock sync, prediction, reconciliation, kill/respawn and spoof
rejection. `gloom.vertical_slice_transport` repeats handshake, sustained input
and authoritative snapshot delivery through two real GameNetworkingSockets
loopback endpoints. The existing Vulkan slice and synchronization smokes remain
unchanged.
