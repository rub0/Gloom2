# ADR 0052: Windows matchmaking uses an HTTPS REST composition

## Status

Accepted.

## Decision

Provide a concrete WinHTTP connector requiring HTTPS and sending the opaque
service credential only as an `Authorization: Bearer` header. The REST base
exposes `GET /health`, `GET /matches`, `GET/PUT/DELETE /matches/{id}`. Records
use one UTF-8, tab-separated line containing the eleven bounded advertisement,
lease and mutation-id fields (extended by ADR 0053); list returns one record per line. PUT supplies `now_ms`, DELETE
supplies the owner `instance_id`, both percent encoded where applicable.

`GLOOM_MATCH_SERVICE_URL` and a role-specific token opt executables into this
composition. ADR 0056 replaces the initial `GLOOM_MATCH_SERVICE_TOKEN` with
`GLOOM_MATCH_READER_TOKEN` for clients and `GLOOM_MATCH_PUBLISHER_TOKEN` for
dedicated publishers, with no legacy fallback. The dedicated server publishes to it. The graphical
client supports `--vertical-slice-list` and resolves `match:<id>` through it;
raw `host:port` remains service-free. Lease times use Unix milliseconds so
separate machines share the provider clock convention.

The token is never printed or copied into gameplay protocol data. TLS
certificate validation remains WinHTTP's system-default policy.

## Consequences

The client half is now deployable against the documented service API, while the
service implementation is now provided by ADR 0055. ADRs 0053-0054 add mutation
keys, credential-refresh hooks and title-based in-window browsing.

## Validation

The connector compiles under `/W4 /WX`; existing fake-provider tests cover the
service/directory semantics without requiring Internet access. All networking
and graphical smokes retain their direct-IP configuration.
