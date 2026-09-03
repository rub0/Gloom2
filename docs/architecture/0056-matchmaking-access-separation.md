# ADR 0056: Role-separated matchmaking credentials and request budgets

## Status

Accepted. Supersedes the shared-bearer policy in ADRs 0052 and 0055.
ADR 0057 supersedes the shared reader token with per-principal expiring grants,
durable revocation and client renewal; publisher separation and aggregate
request budgets continue to apply.

## Decision

The HTTPS host requires two distinct operator-provisioned opaque bearer tokens,
each 16-512 printable non-space ASCII characters. `GLOOM_MATCH_READER_TOKEN`
grants GET/HEAD discovery and the connector health probe, not readiness or any
mutation. `GLOOM_MATCH_PUBLISHER_TOKEN` grants the existing publisher API and
readiness. Authentication is enforced before route handlers and storage access;
unknown/missing/duplicate Authorization headers fail with 401, authenticated
readers attempting other operations fail with 403. Token values never appear
in diagnostics or gameplay messages. Constant-time comparison remains in use.

The client reads only the reader variable; the dedicated server reads only the
publisher variable. The host reads both. No executable falls back to the old
`GLOOM_MATCH_SERVICE_TOKEN`. Operators must give clients only reader credentials,
never an environment containing publisher secrets. Tokens are not embedded in
assets, executables or URLs. Rotation currently requires process restart.

Three independent process-wide token buckets bound HTTP handler admission:
reader burst 60/refill 20 requests per second, publisher burst 30/refill 10,
failed authentication burst 20/refill 2. All requests in each category count,
including failed routes and authorization failures. Buckets use a monotonic
clock, a mutex and fixed memory; idle refill cannot exceed burst capacity.
Exhaustion returns 429 with Retry-After: 1 before storage access. No IP or
forwarded header can select a new bucket. Publisher and reader credentials
retain separate budgets even when they access the same route.

## Consequences and limits

Leaking a reader token cannot authorize publication or withdrawal, even though
records expose instance identifiers. Publishers remain a single trusted
administrative role, not mutually untrusted tenants. A reader token is a shared
discovery capability, not individual player identity. Its holders share a quota.
This milestone does not provide short-lived issuance, per-user revocation,
per-user quotas, anonymous discovery or gameplay authentication changes.

Rate limits apply after HTTP parsing and TLS establishment, not at the network
edge; existing worker/queue/body/time limits still apply. Role budgets isolate
quota consumption, not sockets, bandwidth or worker capacity. Use a protected
deployment and appropriate external ingress controls; do not claim DDoS
resistance or general public-service readiness. Mutations are not blindly
retried on 429; the provider reports status to its caller.

## Validation

`gloom.match_https` exercises the actual child-process TLS host with both roles:
reader health/list/resolve, denied PUT/DELETE/readiness, duplicate headers,
missing/equal/short startup credentials, publisher writes and durable restart.
Real request bursts check all three budgets, isolation and recovery. Deterministic
bucket checks cover exact refill boundaries, backward time, capped idle refill
and concurrent consumption. Direct-IP graphical/network tests remain service-free.
