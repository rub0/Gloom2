# ADR 0055: Deployable HTTPS match host with single-writer durability

## Status

Accepted.

## Context

Milestones 46-52 provided directories, lease ownership, a WinHTTP connector and
client presentation, but no HTTP listener could run the provider contract.
Milestone 53 must implement that host rather than defer it behind another
interface. Service traffic must not drag the renderer or authoritative gameplay
loop into the host process.

## Decision

Build `gloom_match_service` with a pinned cpp-httplib/OpenSSL TLS listener. Keep
the HTTP adapter in `gloom_match_https` and the durable store/shared record codec
in `gloom_match_storage`, separately linked from the graphical engine. Package
the executable and runtime DLLs using `gloom_match_service_package`.

The host exposes the ADR 0052 eleven-field contract, authenticated health and
readiness, bounded request bodies/workers/timeouts, owner-checked mutations and
graceful draining. Authorization compares an operator-provided bearer using
OpenSSL's constant-time comparison. Host wall time, never caller `now_ms`,
decides ownership and a maximum 60-second publication expiry. HTTP reads hide
expired records, leaving directory protocol/phase filtering in the client.

The durable service retains instance/revision and exact-latest-mutation replay
semantics from the in-memory core. A lifetime OS lock enforces one process per
store; startup validates records and probes persistence. Updates flush a
same-directory temporary file before atomic replacement on Windows and roll
back memory on failure. A corrupt store blocks startup. The data format is
versioned and bounded; old interrupted temporary files are not committed state.

WinHTTP and the host share one validated codec. WinHTTP now bounds responses,
uses finite request timeouts and rejects redirects rather than forwarding the
bearer to another destination.

## Consequences and limits

This is a runnable, restartable Windows service binary, not an external cloud
deployment. Operators provision trusted certificates, secrets, firewall rules,
backups and supervision. ADR 0056 supersedes the initial shared bearer with
reader/publisher credentials and bounded role request budgets. Individual
credential issuance and high availability remain follow-up work.
The 1024-record store retains expired history and needs
operator lifecycle management. Full deployment instructions are in
`docs/MATCH_SERVICE.md`.

## Validation

`gloom.match_https` starts the executable in a child process over real TLS with
an ephemeral test certificate. It checks trust rejection, authentication,
readiness, oversized/malformed requests, round trips, idempotency, clock-spoof
ownership rejection, wrong-owner deletion and persistence after process restart.
Its child process exits through the normal bounded-shutdown path.
`gloom.match_discovery` checks exclusive storage ownership, codec validation,
write-failure rollback, restart recovery and corrupt-store rejection. Tests do
not install certificates or need external accounts.
