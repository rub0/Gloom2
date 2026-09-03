# ADR 0057: Per-principal matchmaking reader grants

## Status

Accepted. Supersedes ADR 0056's shared reader credential. Publisher authority,
aggregate role budgets, match record wire format and gameplay admission remain
as defined by the earlier ADRs.
ADR 0058 adds Keycloak as an alternative identity provider and account-token
acquisition/renewal in the client while preserving these grant contracts.

## Decision

Public clients supply an individual opaque identity credential to
`POST /reader-grants/exchange` in a single Authorization bearer header. The
request has no body or query parameters: the client cannot select a principal,
scope, expiry or quota. `MatchIdentityVerifier` resolves the credential to a
trusted principal and an absolute identity expiry. This boundary is independent
of gameplay identity; matchmaking grants cannot admit a game session.

The executable uses an operator-owned, bounded file verifier. Its versioned
registry contains unique principal ids, SHA-256 digests of high-entropy individual
credentials and Unix-millisecond expiries. It reloads and validates the whole
file on every exchange, rejects duplicate ids/digests or corrupt trailing data,
and fails closed if unavailable. It has no development/self-asserted fallback.
This supplies a runnable trusted-identity integration without inventing an
external account provider, JWT parser or password login protocol. Other trusted
verifiers can be injected at the same boundary.

Successful exchange returns `gr1_<64 lowercase hex digits>\t<lifetime_ms>`.
OpenSSL RAND_bytes generates 256 random bits. Only a SHA-256 digest is retained
by the host; constant-time digest comparison authorizes requests. Grant lifetime
is at most 60 seconds, capped by the verified identity's remaining lifetime.
Expiry uses a monotonic clock after issuance; publisher/client timestamps cannot
extend it. Responses are no-store. Grants carry only read authority for
GET/HEAD health/list/resolve. Publisher credentials are explicitly denied at the
identity exchange endpoint, even if misconfigured in the identity registry.

Each trusted principal has a read bucket (burst 20, refill 5/s), an exchange
bucket (burst 4, refill 1/s), at most four unexpired grants and a suspension bit.
Renewal or identity-credential rotation preserves the principal's budgets.
Mutexes serialize issue/consume/revoke; state is capped at 1024 principals and
4096 grants. Principal quota/suspension entries are never evicted during a run;
capacity exhaustion returns 503 instead of resetting a quota. Expired grants
are pruned at the next exchange for their principal.

The existing aggregate reader (60/20), publisher (30/10) and failed-auth (20/2)
budgets remain. Exchange has a separate global bucket (20/2) applied before file
verification, including malformed and invalid attempts. All costs remain bounded
after HTTP parsing. Per-principal checks precede aggregate reader admission, so
a throttled principal cannot drain the shared reader bucket. All valid-grant
requests count against that principal, including denied writes and unknown
routes. 429 has Retry-After: 1; no IP or forwarded header selects a bucket.

Publisher-only `DELETE /reader-grants/{principal}` suspends an id (including one
not yet seen) and invalidates all its grants. Suspensions persist in the bounded
versioned `STORE.revocations` sidecar, under the same lifetime store lock.
Acknowledgement follows flush and atomic replacement on Windows. Write failure
returns 503 and marks readiness unavailable; in-memory suspension remains in
force and an idempotent retry can recover. Corrupt/unwritable revocations block
startup. Successful revocation prevents both access and reissuance after restart.
All grants, including non-revoked ones, are invalidated by restart; only
suspensions persist. Unsuspension currently requires a deliberate offline
operator edit while the host is stopped, followed by restart.

`gloom` reads only `GLOOM_MATCH_IDENTITY_TOKEN` and uses the dedicated WinHTTP
reader connector. Its serialized cache obtains a grant before the health probe
and renews on a later read at the monotonic deadline minus up to one second.
The deadline starts before the exchange request; an already expired response is
rejected. A failed renewal never reuses expired credentials. 401/429 remain
visible and do not trigger hidden replay. Publisher operations still use the
static publisher connector, and the reader connector rejects mutations locally.
WinHTTP retains TLS validation and disabled redirects; credential characters and
URLs with embedded credentials/query/fragment are rejected before network I/O.

## Consequences and limits

No shared long-lived reader secret needs to be distributed to clients. Individual
identity credentials must still be acquired/provisioned securely; their digests
are only suitable for high-entropy tokens, not user passwords. The registry is
not a public login/account system or a deployed platform identity integration.
Removing/expiring an identity blocks future exchange; use the revocation endpoint
to invalidate already issued grants immediately and persistently.

Quotas and active grants reset on process restart; suspensions do not. This is a
bounded single-host service with mutually trusted publishers, not multitenant
publisher authentication, distributed quota accounting or network-edge abuse
protection. Unknown exchange traffic can exhaust the shared exchange budget;
per-principal discovery quotas do not guarantee bandwidth, worker availability
or login fairness. Existing protected-ingress requirements still apply.

## Validation

The existing `gloom.match_https` test now also exercises deterministic grant and
client-cache clocks, identity-expiry capping, multiple grants sharing a quota,
independent principals, concurrent consumption, grant/principal capacity,
revocation persistence/failure and malformed grant responses. Real child-process
TLS checks verify exchange, expiry, corrupt registry rejection, read-only scope,
renewal without quota reset, publisher revocation, blocked reissuance, restart
invalidation and persistent suspension. Certificates remain confined to the test
client; no Windows trust changes or external accounts are needed.
