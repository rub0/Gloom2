# ADR 0034: Component authority and replication

## Status

Accepted.

## Context

Hound gameplay state lived in logical components, but movement snapshots and
combat history were still read directly from the legacy movement server. There
was no component-level rule preventing received state from overwriting server
authority, and remote clients reconstructed transient movement records without
the same logical Hound composition used by the host.

## Decision

Every replicated Hound composes `AuthorityComponent` and
`ReplicationComponent`. Authority is one of `server`, `predicted_owner` or
`interpolated_remote`. Replication state stores simulation/receipt ticks, input
acknowledgement and grounded state next to the component transform.

`capture_component_snapshot` serializes movement state exclusively from logical
components. `apply_component_snapshot` resolves wire IDs through authority
components, rejects writes to server authority, preserves predicted ownership
unless reconciliation is requested, ignores stale remote state and reports its
application result. The slice synchronizes the current movement seam into these
components once per tick; presentation snapshots and lag-compensated combat
history then consume the component-derived frame.

Remote clients create the same Hound component recipes after admission. Their
controlled entity is marked predicted and its peer interpolated. Authoritative
frames pass through the component application policy before prediction overlays
the controlled transform.

Every authoritative slice now has a Jolt world. Local and listen-host paths own
one when none is injected; dedicated servers inject their existing headless
world. Both paths run the same Factory bodies, sensors and attached character
composition.

## Consequences

- Wire identity, ownership and transform state have one component boundary.
- Server state cannot be mutated accidentally by received snapshots.
- Prediction/reconciliation and remote interpolation have explicit policies.
- Combat-history component snapshots contain living entities only; respawn
  republishes the entity from its authoritative spawn transform.
- This decision retained protocol v9; ADR 0035 later extends gameplay snapshots
  to protocol v10 with replicated mechanism state. Input redundancy and movement
  delta formats remain compatible.
- The movement server remains the fixed-tick input/prediction seam and publishes
  its result into components; replacing that seam is not required for component
  replication ownership.

## Validation

`gloom.vertical_slice` verifies component capture, remote application, stale and
server-authority protection. Existing local, adverse-network, host/join,
two-client and dedicated-server tests validate the unchanged protocol behavior
over the new component state path.
