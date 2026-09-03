# ADR 0059: Verified account admission with single-use game tickets

## Status

Accepted. Completes roadmap milestone 57 and extends ADRs 0047, 0056–0058.

## Decision

Keep account access/refresh tokens in the client and HTTPS account service.
Discovery grants remain read-only capabilities. A separate POST
`/game-tickets/<match-id>` exchanges the current account access token for an
Ed25519-signed gameplay credential. Keycloak introspection requires the same
issuer, audience, client, subject and expiry checks as discovery, with the exact
`gloom.play` scope. Registry mode remains an explicit development alternative.
Publisher tokens and reader grants cannot obtain tickets. Principal suspension
and the principal exchange budget apply to both discovery and gameplay issuance;
the existing global exchange budget bounds provider requests too.

The service looks up the current leased advertisement and returns its endpoint,
instance and ticket. Missing, expired, completed or incompatible records fail.
An active/full advertisement can issue a ticket because reconnecting players
need access; the game authority decides whether a player owns a dormant slot.
Tickets expire at the earliest of 30 seconds, account expiry and lease expiry.
There is no blind retry of issuance after an ambiguous HTTP response.

Wire format is `gt1.principal.instance.issued_ms.expires_ms.nonce.signature`.
The principal is a 64-character lowercase SHA-256 digest; the instance is a
bounded safe identifier; the nonce is 16 random bytes encoded in lowercase hex.
Ed25519 signs the exact bytes before the final period, without a newline. The
64-byte signature is lowercase hex. All fields and the 512-byte total bound
are checked before admission. This is a versioned internal capability format,
not a general JWT implementation.

The dedicated server generates a fresh random 128-bit instance on each start
and receives only the public verification key. It verifies signatures and UTC
deadlines locally, consuming each nonce once. Expired replay entries are pruned;
the replay table is capped at 4096. No provider or HTTPS request occurs inside
game admission or the simulation loop. The full principal remains bound to the
existing compact 64-bit lobby account id in a table capped at 1024; a collision
is rejected, never treated as the same account. The authoritative display name
is `P_` followed by the first 16 digest characters. Client names cannot select
an account. The client adopts the identity returned in the authoritative lobby.

The session manager binds admission to the verified account/name. Duplicate
admission and a resume belonging to another identity fail before consuming a
slot or rotating its token. Lobby eligibility is checked before session
mutation. Dedicated-server resume tokens use OpenSSL randomness and rotate on
successful resume. Pending session registrations are bounded to four times
the configured session capacity; invalid/unregistered/active hellos do not
invoke the identity provider. Development library defaults remain available
for service-free tests; production composition injects the secure generator.

`GLOOM_GAME_AUTH=tickets` selects this mode in client and dedicated server.
Joining requires `match:<id>` or browse. The client requests a fresh ticket on
each initial join and reconnect, using its serialized account renewal callback.
The request runs on one background future and failures are surfaced without
falling back to development credentials. The HTTPS transport validates Windows
trust, disables redirects and keeps credentials out of URLs and diagnostics.

## Operations and limits

See [GAME_AUTH.md](../GAME_AUTH.md) for key distribution and process settings.
The signing key belongs only to the HTTPS host; it is distinct from its TLS key.
Rotate keys by coordinating host/server restarts. A new game instance invalidates
old tickets and resume state. Synchronize service and game-server UTC clocks;
future-issued tickets fail closed, with no implicit clock-skew allowance.

Suspension blocks future issuance; a ticket already issued can remain usable
for up to 30 seconds. Existing active game sessions are not forcibly kicked by
discovery suspension. Reconnect always requires a fresh verified ticket. This
does not add account administration, durable game-state recovery, a relay/NAT
service, multi-region storage or authenticated GNS peer certificates. Direct-IP
GNS retains its existing encrypted but unauthenticated-peer transport boundary.
The bounded application tables are not a network-edge denial-of-service defense.

These are explicit deployment/architecture limits, not additional numbered
network milestones. All networking milestones currently scheduled in ROADMAP
are implemented; operating an external realm and public infrastructure still
requires the operator's environment and acceptance checks.

## Validation

`gloom.game_tickets` covers tampering, malformed credentials, wrong keys,
cross-instance use, replay, future/expired tickets, account expiry, compact-id
collision rejection, duplicate admission, identity-mismatched resume, unchanged
ownership after rejection, token rotation, and a real GNS join/reconnect.
`gloom.keycloak_identity` extends the local TLS provider contract through actual
HTTPS ticket issuance, scope separation, offline verification and suspension.
The existing registry HTTPS/restart, transport, lobby, simulation and graphics
suite remains the regression gate. No test contacts or claims acceptance against
a live external Keycloak realm.
