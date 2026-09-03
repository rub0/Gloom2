# ADR 0048: Match discovery resolves to the existing direct-IP boundary

## Status

Accepted.

## Context

Remote play required players to exchange a GameNetworkingSockets endpoint out
of band. Identity verification, lobby admission and transport were already
separate concerns, so adding discovery must not let a directory admit players,
authenticate credentials or introduce a second gameplay connection path.

## Decision

Define `SliceMatchDirectory` as a backend-neutral boundary for publishing,
withdrawing, listing and resolving `SliceMatchAdvertisement` records. Records
contain an opaque match id, display name, direct-IP endpoint, wire-protocol
version, authoritative connected-player count, capacity, lobby phase and a
monotonic revision.

Only waiting, non-full records for the caller's exact protocol version appear
in discovery or resolve successfully. The directory rejects malformed bounded
metadata and stale revisions. Resolution returns `SliceJoinResolution`, whose
endpoint is passed unchanged to the existing `network::Transport::connect`;
the subsequent credential verification, session ownership and lobby admission
remain unchanged.

Provide a deterministic, thread-safe in-memory implementation for composition
tests, local tools and future service-adapter conformance. It is not a global
registry and does not make separate processes discover one another. External
directory services implement the same interface at a composition root.

`make_slice_match_advertisement` derives phase and connected occupancy from an
authoritative lobby snapshot. Callers still own publication cadence, endpoint
reachability, leases and withdrawal.

## Consequences

- Matchmaking providers do not leak SDK types into gameplay or networking.
- Discovery compatibility fails before opening a socket, while the protocol
  codec retains its own mandatory version check.
- Full, active and completed matches cannot be selected from stale UI state if
  resolution sees their latest advertisement.
- Revision ordering prevents delayed provider writes from reopening old state.
- Ranking, regions, NAT traversal, service persistence and allocation remain
  future backend/composition work.

## Validation

`gloom.match_discovery` covers deterministic listing, direct endpoint
resolution, incompatible protocols, stale revisions, full/active filtering,
withdrawal, malformed metadata and authoritative lobby-state derivation.
