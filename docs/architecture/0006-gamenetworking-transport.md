# ADR 0006: GameNetworkingSockets transport boundary

## Status

Accepted

## Context

Gloom needs a maintained UDP-based transport without coupling gameplay and
simulation code to a vendor API. The transport must support reliable and
unreliable messages, connection lifecycle events and future evolution of the
replication model. Replication, prediction and reconciliation are higher-level
simulation concerns and should not be embedded in the socket backend.

GameNetworkingSockets is connection-oriented but message-oriented. It provides
reliable and unreliable delivery, fragmentation/reassembly, congestion control
and encrypted direct-IP connections. The engine still needs its own stable
identifiers, owned packet memory and lifecycle events so that replacing the
backend does not change callers.

## Decision

`gloom::network::Transport` is the engine-owned boundary. It exposes opaque
connection IDs, reliable/unreliable packet delivery, connection events,
received packets and aggregate byte/packet metrics. It does not expose GNS
handles, callback structs or allocator-owned message buffers.

`GnsTransport` implements that contract using direct IPv4/IPv6 sockets and a
poll group. The engine thread serializes public calls and pumps GNS callbacks
and received messages through `tick()`. Incoming GNS buffers are copied into
engine-owned storage before being released. A process-level reference count
protects GNS global initialization while multiple transport instances coexist.

Listening on port zero is part of Gloom's API even though GNS rejects it. The
backend probes the IANA dynamic/private range and returns the actual bound
endpoint. This is useful for tests and local server processes.

The vcpkg dependency disables default features for now. In particular,
WebRTC/ICE traversal is outside this direct-IP milestone and can be evaluated
separately without changing the transport contract.

## Consequences

- Gameplay and future replication code depend only on Gloom packet and event
  types.
- Reliable and unreliable traffic is tested through a real client/server
  loopback connection, including disconnect propagation.
- The current backend is single-owner-thread by contract. Multithreaded game
  systems hand messages to that owner rather than calling GNS concurrently.
- Encryption alone does not authenticate an unknown direct-IP peer. A future
  Internet deployment needs certificates, a trusted identity scheme or another
  explicit authentication layer to prevent man-in-the-middle attacks.
- The next networking milestone owns protocol versioning, serialization,
  replication, prediction, reconciliation, snapshot interpolation and adverse
  network simulation. Those policies remain above the transport.

## References

- [Valve GameNetworkingSockets](https://github.com/ValveSoftware/GameNetworkingSockets)
- [ISteamNetworkingSockets interface](https://github.com/ValveSoftware/GameNetworkingSockets/blob/master/include/steam/isteamnetworkingsockets.h)
- [Official example chat application](https://github.com/ValveSoftware/GameNetworkingSockets/blob/master/examples/example_chat.cpp)
