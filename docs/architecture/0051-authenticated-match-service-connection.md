# ADR 0051: Provider connectors own matchmaking authentication

## Status

Accepted.

## Context

ADR 0050 exposed a persistent service contract but did not define how a process
obtains an authenticated service session or behaves during transient provider
startup failures. Putting bearer-token parsing into the directory would mix
provider authentication with match policy and risk leaking credentials into
advertisements or gameplay messages.

## Decision

Define `SliceMatchServiceConnector::connect(service_url, bearer_token)` as the
provider integration boundary. A successful connector returns an authenticated
`SliceMatchService` session; the directory never sees the credential.

`connect_match_directory` validates non-empty URL/token configuration and uses
a bounded exponential connection retry policy. Callers choose maximum attempts,
initial and maximum delay, and inject the wait operation so tests and event-loop
compositions do not sleep implicitly. A null service or exhausted attempts is a
hard, descriptive error. Success is immediately adapted through the existing
service directory.

Only connection establishment is retried here. Individual provider operations
need provider-specific idempotency keys and retry classification; blindly
replaying publication could turn an acknowledged write into a stale revision.

## Consequences

- Credentials remain confined to the provider composition boundary.
- Startup tolerates a bounded transient outage without hanging forever.
- Tests can verify timing policy deterministically.
- TLS, token refresh, secure credential storage and request idempotency belong
  to a concrete connector implementation.
- The graphical executable still has no selected provider connector, so its
  supported runtime composition remains direct IP.

## Validation

`gloom.match_discovery` verifies opaque URL/token forwarding, recovery after two
failures, capped exponential delays, successful access to persistent records,
bounded exhaustion and rejection of missing credentials.
