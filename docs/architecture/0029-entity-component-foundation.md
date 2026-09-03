# ADR 0029: Logical entity/component foundation

## Status

Accepted.

## Context

The vertical slice proved gameplay and networking boundaries, but its map and
combatants are still assembled from feature-specific records. Extending
`ArenaBox` with hazard behavior would conflate visual geometry, solid collision
and trigger semantics. Likewise, the private combatant structure cannot become
the long-term home for reusable health, abilities or physics behavior.

Gloom needs logical entities composed from reusable components. Jolt remains the
physics implementation, including sensors and contact callbacks, but third-party
types must stay behind Gloom-owned component contracts. The definitive character
physics will later migrate from the original Gloom behavior rather than being
defined accidentally by the current slice scaffolding.

## Decision

`EntityRegistry` owns generational `EntityId` handles and typed component pools.
Entity slots may be reused, but their generation advances on destruction so an
old handle cannot access a replacement entity. Destroying an entity removes all
of its components. Clearing the registry invalidates every live handle.

Components are ordinary, independently reusable C++ value types. They do not
inherit from a common base and the registry does not know about SDL, Jolt,
Diligent, networking or gameplay. Backend-owning subsystems will introduce
Gloom-facing adapter components containing opaque Gloom handles in later
milestones. Structural mutation is single-owner policy; parallel systems may be
added later with explicit scheduling or deferred command buffers.

This milestone intentionally does not migrate the vertical slice. Establishing
and testing identity/lifetime semantics first prevents the trigger, character
and replication migrations from inventing incompatible entity models.

## Consequences and limits

- Logical identity and component lifetime now have one reusable foundation.
- Stale entity handles fail safely after destruction, clearing and slot reuse.
- Component storage remains independent from backend and gameplay inheritance
  hierarchies.
- Queries, systems, serialization, editor metadata and deferred structural
  mutation are not yet part of the contract.
- Factory lava remains presentation-only until it can be migrated correctly as
  a surface entity plus Jolt sensor and damage-volume component.

## Ordered migration

1. Add Jolt-backed body and trigger components with queued contact events.
2. Rebuild Factory floor/lava as logical entities and remove blockout coupling.
3. Compose Hound from logical components while retaining provisional movement.
4. Port the original Gloom character physics behind the component boundary.
5. Move authority, snapshots and reconciliation onto component state.
6. Add reusable kinematic components and Factory mechanisms.

## Validation

`gloom.entities` verifies distinct identity, multiple typed components,
const lookup, removal, destruction cleanup, generational slot reuse, registry
clearing and rejection of stale handles.
