# Deploying the matchmaking host (milestones 53-57)

Milestone 57 optionally adds signed game-ticket issuance. See
[GAME_AUTH.md](GAME_AUTH.md) for the separate Ed25519 signing key, public-key
distribution, `gloom.play` scope and verified admission/reconnection flow.
The distribution package includes this guide. Without a configured signing key,
ticket issuance returns 503; discovery keeps its existing behavior.

`gloom_match_service` is a standalone C++ HTTPS process. It does not start the
game simulation, SDL or Vulkan. Run `gloom_slice_server` separately: discovery
only returns its reachable GNS address.

Milestone 56 adds Keycloak account sign-in and host introspection. See
[KEYCLOAK.md](KEYCLOAK.md) for that mode. The registry instructions below remain
valid for `GLOOM_MATCH_IDENTITY_PROVIDER=registry` (also the compatibility
default). Selecting `keycloak` requires its own configuration and never falls
back to a registry credential.

## Build and package

```powershell
cmake --build --preset windows-release --target gloom_match_service_package
```

The package is at `build/windows-vs/package/match-service/Release`. It contains
the executable, OpenSSL/simdjson runtime DLLs, dependency notices and guides. Install
the Visual C++ 2022 x64 redistributable on the target Windows machine. Debug
packages are for development machines only. The source pins cpp-httplib 0.37.0
to commit `0d3a3d805c8979e3220b42fcbc0ff95980eabc06`; OpenSSL comes from vcpkg.

## Configuration and startup

Provision a PEM certificate chain and matching PEM private key. The certificate
must cover the hostname used in `GLOOM_MATCH_SERVICE_URL` and be trusted by the
client's Windows certificate store. Never disable certificate validation for
deployment. Protect the private key and storage directory with OS permissions.

Inject `GLOOM_MATCH_PUBLISHER_TOKEN` through your process supervisor or secret
manager. Set `GLOOM_MATCH_IDENTITIES_FILE` to the absolute path of a protected
identity registry (format below). Publisher and individual identity credentials
must contain 16-512 printable non-space ASCII characters; generate independent
random values with at least 128 bits of entropy. Missing/malformed configuration
blocks startup. Do not put tokens in command-line arguments, source control or logs.

Provision credentials separately for each process:

| Process | Required credential variables |
| --- | --- |
| `gloom_match_service` | Publisher and identities-file path |
| `gloom_slice_server` | Publisher only |
| `gloom` discovery client | Individual `GLOOM_MATCH_IDENTITY_TOKEN` only |

Do not launch a client from a shell/supervisor environment containing publisher
secrets. The client's individual credential is exchanged over TLS for a grant
valid for at most 60 seconds. The client renews before its next read when needed.
Never compile credentials into the game or assets. Publisher rotation requires
restart; individual identity registry changes are loaded on the next exchange.
Changing an individual credential must preserve its trusted principal id so it
retains the same quota and suspension state. A distributed client cannot conceal
a token from its own user; provision only that user's identity credential.

Migration is intentionally fail-closed: `GLOOM_MATCH_SERVICE_TOKEN` and
`GLOOM_MATCH_READER_TOKEN` are ignored. Provision individual credentials and a
trusted identity registry; neither a shared reader token nor a publisher token
can be used for the client exchange.

### Trusted identity registry

Use UTF-8 without BOM (LF or CRLF), beginning with `GLOOM_MATCH_IDENTITIES_V1`.
Each following nonempty line has three tab-separated fields: trusted principal
id, lowercase SHA-256 digest of its credential, and identity expiry in Unix
milliseconds. Ids contain 1-64 ASCII letters, digits, `_` or `-`. Digests must
be exactly 64 lowercase hex digits. Ids and digests must be unique. The file is
limited to 160 KiB and 1024 identities; the whole file is validated before any
identity is accepted. Use a high-entropy random credential, never a password.
The digest is computed over the credential's ASCII bytes without a newline.

Provision ids and credential hashes from your trusted operator/account workflow.
The client does not send or choose an id. Protect the file against client writes,
and publish changes by atomic replacement so exchanges see a complete registry.
Missing or corrupt input rejects exchange. Credential expiry/removal blocks new
grants; revoke the principal below for immediate invalidation of existing grants.
This file adapter is runnable trusted provisioning, not an end-user login UI.

```powershell
.\gloom_match_service.exe 127.0.0.1 8443 C:\GloomSecrets\fullchain.pem C:\GloomSecrets\key.pem C:\GloomData\matches.store
```

For another machine, bind an explicitly selected reachable address and configure
the firewall separately. No ports, certificates, firewall rules or system
services are installed automatically. Use an HTTPS root URL such as
`https://matches.example.org:8443` (no `/v1` prefix).

Set the URL and publisher token in the dedicated server process. Set the URL and
individual identity token in a client to use `--vertical-slice-list`, `--vertical-slice-join match:<id>` or
`--vertical-slice-browse`. Direct-IP joining does not contact this service.
The current dedicated publisher advertises `local-factory-duel` and its bound
endpoint: bind it to a reachable address, not `0.0.0.0`, for remote discovery.
Do not run multiple publishers with that same logical id simultaneously.

## HTTP contract

All routes require `Authorization: Bearer ...`, including probes. Requests are
bounded to 2048 body bytes. No credential or provider exception body is logged
or returned. The host never trusts publisher `now_ms` to decide lease ownership.
Reader grants allow GET/HEAD `/health`, `/matches` and `/matches/{id}`
only. Individual identity credentials authorize only exchange. Publishers may
use all other routes below; `/ready` and principal revocation are publisher-only.
Missing, invalid or multiple Authorization headers return 401; a reader trying
any other operation gets 403 (or 429 if its request budget is already exhausted).

| Method/path | Behavior |
| --- | --- |
| `POST /reader-grants/exchange` | Individual bearer, empty body/query; `200` with grant and lifetime; `401` invalid/expired/suspended identity; `429` quota/cap; `503` capacity/verifier failure |
| `DELETE /reader-grants/{principal}` | Publisher only; `204` after durable suspension; `400` invalid id; `503` persistence/capacity failure |
| `GET /health` | `200 alive` while serving |
| `GET /ready` | `200 ready`; `503` after a persistence failure or while draining |
| `GET /matches` | Unexpired records, sorted by id, one per line |
| `GET /matches/{id}` | Record or `404` if absent/expired |
| `PUT /matches/{id}` | `204` on committed write/exact replay; `400` malformed; `409` ownership/revision/idempotency conflict; `503` storage failure |
| `DELETE /matches/{id}?instance_id=...` | `204` owner withdrawal, `404` absent/non-owner, `503` storage failure |

Exchange response is `gr1_<64 lowercase hex digits><TAB><lifetime_ms>`, with
no newline and `Cache-Control: no-store`. Lifetime is positive and at most
60000 ms, bounded by identity expiry. Use only the returned grant as bearer on
discovery reads. The WinHTTP reader connector manages this automatically,
measuring expiry from before the exchange and renewing up to one second early.
401 and 429 are surfaced without automatic replay. Grant renewal preserves
principal quotas; it neither extends old grants nor invalidates another device's
grant. Each principal can hold at most four active grants.

Revocation is idempotent and invalidates every grant for the principal, then
blocks future exchanges, even with a rotated identity credential. It can also
suspend an id before first login. Success is acknowledged only after persisting
the suspension. On a failed write, 503 is returned and `/ready` fails; in-memory
suspension still applies. Retry the revocation to recover. To unsuspend, stop the
host, remove that id from the revocation sidecar, then restart. This is an
explicit operator operation; there is no public unsuspension endpoint.

Records are eleven tab-separated fields with no terminating newline on single
record responses: match id, instance id, display name, endpoint, protocol,
player count, capacity, phase, revision, Unix lease expiry (ms), mutation id.
Lists append a newline per record. Text fields are bounded printable ASCII.
Phase is `0` waiting, `1` active, `2` completed. The directory adapter filters
capacity/protocol/phase; the host independently hides expired records.
The response header `X-Gloom-Match-Schema: 1` identifies this representation.
The shared C++ codec is the authoritative implementation of these constraints.

### Request throttling

The host enforces independent process-wide token buckets. Readers have a burst
of 60 and refill at 20 requests/second; publishers have a burst of 30 and refill
at 10/second; failed authentication has a burst of 20 and refill of 2/second.
Exchange has a separate global burst of 20 and refill of 2/s, applied before
verifier work to both valid and invalid attempts. Each verified principal also
has a read burst of 20/refill 5/s, and an exchange burst of 4/refill 1/s. All its
grants share those budgets, independently of another principal's budgets.
All requests in a category count, including probes, invalid routes and denied
operations. Exhaustion returns `429` with `Retry-After: 1`; wait at least that
long before retrying. WinHTTP reports the status; it does not silently replay
mutations. Reader traffic cannot consume publisher tokens or vice versa.

State is bounded to 1024 principals and four active grants per principal. A new
identity beyond that cap receives 503; quota/suspension entries are never evicted
during a run, so identity churn cannot reset them. Restart resets volatile quotas
and invalidates all grants; durable suspensions remain. IP and forwarded headers
cannot bypass the quotas. Shared role/exchange budgets remain aggregate ceilings;
individual quotas do not guarantee fair service under arbitrary aggregate load.
They run after TLS/HTTP parsing, so they do not protect handshake cost, network
bandwidth or guarantee worker availability under attack. Keep ingress protected.

New writes require an expiry in the next 60 seconds, using the host clock.
Synchronize participating machines' clocks. Identical retries must preserve the
entire original body, including expiry and mutation id. Idempotency covers the
latest stored revision; it is not an unbounded mutation-history service.

## Persistence and shutdown

One process owns each storage file, enforced with an OS-held `.lock` handle.
The lock file may remain after exit; its existence alone is not a held lock.
The store keeps up to 1024 logical records, validates the versioned file at
startup and refuses corrupt input rather than silently starting empty. Expired
records remain on disk for owner/revision history, but are hidden by HTTP reads.

Startup verifies writable storage before readiness. Writes flush a same-directory
`.tmp` file, flush it to disk on Windows, then replace the committed file.
If replacement fails, the in-memory mutation is rolled back. An interrupted
temporary file is never loaded as committed data. Keep normal backups; this is
single-host durability, not replication or a high-availability database.

Principal suspensions live in `STORE.revocations` under the same exclusive store
lock. This is a bounded file with header `GLOOM_MATCH_REVOCATIONS_V1` followed by
one principal id per line; it contains no credentials or grants. Its writes also
flush and atomically replace the file on Windows. Corrupt/unwritable revocation
state blocks startup. Back up/restore it with the match store so restored matches
do not inadvertently restore a revoked principal's ability to discover them.

Ctrl+C/SIGTERM stops admission and drains request workers. `--run-for-ms N`
provides bounded execution for automation. The process returns nonzero on failed
startup/bind; never delete a corrupt store automatically. Stop the process before
restoring a backup, then restart and check `/ready`.

## Validation and deployment limits

`ctest --preset windows-debug -R "gloom.match_(https|discovery)"` exercises the
actual HTTPS executable, trusted identity exchange, grant expiry, durable
revocation, isolated principal throttling/refill, rejected startup credentials,
TLS trust, validation, ownership, idempotency, restart persistence and withdrawal.
Deterministic tests check renewal timing, concurrency and bounded state.
Tests generate an ephemeral
certificate/key, trust it only in the test client and do not alter Windows trust.
Positive HTTPS tests use an OpenSSL client and the same record codec as WinHTTP;
they are not a test of public certificate issuance or Internet deployment.

Keep the service behind protected ingress. Identity exchange and principal
quotas do not protect network-edge resources or guarantee login fairness.
Keycloak account login is available through the separate configuration guide;
automated certificate rotation and multi-host storage/quotas remain future work.
Publishers remain mutually trusted operators.
Matchmaking credentials do not change gameplay/session authentication. No service
has been deployed to an external account.
