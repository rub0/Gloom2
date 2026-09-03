# ADR 0053: Match mutations are idempotent and browsing is asynchronous

## Status

Accepted.

## Decision

Each stored match carries a mutation id derived from server instance and
advertisement revision. The provider-side `make_match_service_core` atomically
accepts an exact replay, rejects the same key with different data, retains
lease ownership and revision rules, and supplies the persistence semantics that
an HTTPS host can wrap. The REST record grows from ten to eleven fields.

Connection configuration may acquire tokens through a callback. The first
attempt requests the current token; later attempts request refresh, and token
acquisition failures participate in the same bounded backoff. Static bearer
configuration remains supported.

`AsyncSliceMatchBrowser` runs directory listing away from the presentation
thread, prevents overlapping refreshes and atomically publishes either a new
list or an error when polled. Selection and final resolution remain separate so
a UI never connects from stale cached metadata.

## Consequences

Lost write responses may be retried safely by a provider transport using the
mutation id. Token rotation does not enter directory or gameplay state. An
in-window UI can poll without blocking rendering; the current executable still
offers its console browser until that presentation is authored.

## Validation

Discovery tests cover exact replay, changed-payload rejection, refreshed-token
reconnection, overlap prevention and asynchronous result publication.
