# ADR 0050: Match services adapt into one client join flow

## Status

Accepted.

## Context

The in-memory directory proved discovery and lease semantics but could not
represent provider failures or share records between processes. A concrete
vendor, HTTP stack and authentication scheme have not been selected. The client
also needs a stable browse/select/resolve sequence without creating a second
transport path or weakening the direct-IP development workflow.

## Decision

Define `SliceMatchService` as the provider-facing persistence boundary. It
stores leased records atomically, performs owner-checked erasure and exposes
list/get operations with explicit errors. Providers must enforce live-instance
ownership and monotonic revision ordering in their atomic store operation.
`make_service_match_directory` adapts that contract to `SliceMatchDirectory`,
validates outgoing data and filters expired, incompatible, full or non-waiting
records returned by the service.

`SliceMatchSelection` owns the client browse state. Refresh replaces its list
with deterministic match-id ordering, selection is bounds checked, and connect
resolves the selected id again so stale capacity or phase is rejected.
`resolve_slice_join_target` accepts either `match:<id>` with a configured
directory or a validated direct endpoint. Both produce the same
`SliceJoinResolution::endpoint` consumed by GameNetworkingSockets.

The graphical executable now routes its existing direct-IP argument through
this resolver. No production service is configured in that composition yet, so
`match:<id>` correctly reports that discovery is unavailable instead of
silently treating it as an endpoint.

## Consequences

- HTTP, platform and vendor SDKs remain outside engine/gameplay headers.
- Provider outages are visible to publication and browsing callers.
- Browsed state is advisory; resolution immediately before connection is the
  capacity, phase, protocol and lease gate.
- Direct-IP host/join remains available without any service.
- Service authentication, retry/backoff, UI presentation and a concrete
  deployed provider remain composition work.

## Validation

`gloom.match_discovery` uses a persistent fake service to exercise publication,
deterministic browsing, index selection, selected/named resolution, expiry,
missing-provider failure and direct-IP fallback. The full graphical and network
suite verifies that the shared resolver does not change current joining.
