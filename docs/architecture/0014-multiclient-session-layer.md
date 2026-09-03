# ADR 0014: Multiclient session and gameplay-event delivery

## Status

Accepted for protocol v6.

## Context

The transport exposes connection handles, but a handle is not a durable player
identity and must not grant authority by itself. Protocol v5 also kept movement
input acknowledgements and snapshot baselines in a single-client server state.
Clock estimation existed only as a standalone algorithm, and an unreliable fire
event could disappear permanently when one packet was lost.

GameNetworkingSockets provides encrypted, connection-oriented reliable and
unreliable messages. Its open-source direct-IP configuration does not provide a
game account or admission policy for Gloom, and higher-level entity replication
remains the application's responsibility. Its separate delivery modes make it
possible to keep latency-sensitive traffic away from an ordered reliable stream.

## Decision

Protocol v6 introduces a backend-neutral session layer with four responsibilities.

### Admission, identity and reconnect

A new connection is pending until it supplies a `ClientHello`. The application
injects a credential validator; the session library deliberately does not know
whether the credential came from Steam, another identity service, a development
server or a local test. Successful admission creates a durable session ID and one
server-assigned controlled entity.

The welcome includes an opaque reconnect token. Disconnect makes a session
dormant for a bounded tick window instead of immediately transferring or deleting
its entity. A successful reconnect preserves the session and entity, binds a new
connection and rotates the token. Expired dormant state is removed. The token
generator is also injected and production code must use a cryptographically
secure source. Raw credentials are neither stored nor logged by this layer.

Every movement batch is routed from connection to its server-owned entity. Every
fire command is accepted only when its shooter matches that ownership mapping.
Connection IDs supplied inside gameplay data are never trusted.

### Per-client replication

`AuthoritativeMovementServer` now runs one shared world while retaining separate
input queues, input acknowledgements, snapshot sequences, acknowledged baselines,
snapshot history and telemetry for every controlled entity. Acknowledging a delta
baseline on one connection cannot change the encoding selected for another.

The original single-client methods remain compatibility shims for the interactive
laboratory. New session servers use the entity-qualified input and snapshot APIs.

### Clock exchange

Clock request and response messages carry the four timestamps required by the
existing NTP-style estimator. Nonces bind responses to outstanding requests and
make duplicate or replayed responses invalid. The resulting server-time estimate
can timestamp fire commands and choose interpolation time without assuming both
machines share a wall clock.

### Idempotent gameplay events

The session event channel assigns a 32-bit sequence to an opaque gameplay
payload. A sender transmits immediately, retries after a configurable interval
and discards the event after a short lifetime. The receiver remembers a bounded
window of sequences, delivers the payload once, and acknowledges both the first
copy and duplicates. The sender removes acknowledged events.

This channel is intended for short-lived actions sent with the unreliable
transport mode. It must not be layered on the ordered reliable lane: doing so
would duplicate GameNetworkingSockets retransmission and reintroduce
head-of-line delay. Configuration bounds pending events, duplicate memory and
event lifetime.

## Consequences and limits

The engine now has explicit boundaries for account authentication, connection
ownership and reconnect without depending on a particular online service.
Multiclient inputs and delta snapshots share one authoritative simulation while
retaining independent network state. Live loopback tests exercise the handshake,
authorization and four-timestamp clock exchange through GameNetworkingSockets.

This is an authentication framework, not an account service. Production still
needs a trusted credential issuer, secure token generation, rate limiting and
operational revocation. Reconnect state is in memory and will not survive a server
restart. Event acknowledgement is individual rather than a compressed ACK vector;
that is appropriate for the bounded low-rate gameplay-event queue but not bulk
data transfer.

## Validation

`gloom.session` verifies protocol schemas, two simultaneous owners, spoofed-fire
rejection, capacity, reconnect and token rotation, expiry, clock replay rejection,
independent snapshot baselines and exactly-once event delivery under a lost ACK.
`gloom.network` performs admission, clock synchronization and authorized movement
and fire exchange over a real encrypted loopback connection.

## References

- [Valve GameNetworkingSockets](https://github.com/ValveSoftware/GameNetworkingSockets)
- [GameNetworkingSockets public API and identity hooks](https://github.com/ValveSoftware/GameNetworkingSockets/blob/master/include/steam/isteamnetworkingsockets.h)
- [RFC 5905: NTPv4 four-timestamp offset and delay](https://www.rfc-editor.org/info/rfc5905/)
- [RFC 8085: UDP usage, retransmission and duplicate suppression](https://www.rfc-editor.org/rfc/rfc8085.html)
