# ADR 0049: A server instance owns a leased match publication

## Status

Accepted.

## Context

ADR 0048 defined discovery records but left their lifetime to callers. A
dedicated process can crash without withdrawing, a delayed shutdown can race a
replacement server, and lobby transitions must promptly stop new joins. A plain
match id and revision cannot distinguish two server instances competing over
the same logical match.

## Decision

Every advertisement carries an opaque `instance_id`. Directory publication
acquires or renews a bounded lease at a caller-supplied monotonic time. While
the lease is live, only that instance may advance the record revision or
withdraw it. Once it expires, another instance may reclaim the logical
`match_id` with its own revision sequence. Listing and resolution first discard
expired records.

`SliceMatchPublication` owns this protocol for a server composition. It
publishes after the transport has produced its real bound endpoint, republishes
when the authoritative lobby revision changes, renews before lease expiry and
performs an owner-checked withdrawal on explicit stop or destruction. The
refresh interval must be shorter than the lease.

The dedicated slice server composes this lifecycle around its existing host
loop. Its current directory is intentionally process-local; replacing it with a
service adapter changes reach and persistence, not publication semantics,
transport, identity verification or lobby authority.

## Consequences

- A crashed server disappears after a bounded period without cleanup code.
- An old process cannot overwrite or withdraw a live replacement instance.
- Lobby phase and occupancy changes invalidate discovery promptly.
- Wall-clock synchronization is not required by the in-process lifecycle;
  service adapters may translate their own lease clock internally.
- Stable allocation ids, external directory persistence, regions and NAT
  traversal remain separate concerns.

## Validation

`gloom.match_discovery` covers lease refresh and expiry, live-owner exclusion,
post-expiry takeover, stale-owner withdrawal rejection, lobby-transition
publication and orderly stop. `gloom.slice_server_smoke` exercises publication
startup, periodic update and shutdown in the real headless composition.
