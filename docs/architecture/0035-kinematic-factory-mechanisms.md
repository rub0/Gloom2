# ADR 0035: Tick-driven kinematic Factory mechanisms

## Status

Accepted.

## Context

Factory contained static collision, a damage sensor and character bodies but no
world mechanism. Teleporting a body every frame would provide no physical
velocity for characters or dynamic contacts, and integrating elapsed wall time
would diverge between local, listen-host and dedicated simulation.

## Decision

`KinematicMotionComponent` describes a ping-pong path using start/end positions,
integer travel and dwell durations, and an integer phase tick. Sampling is a pure
function of tick. `advance_kinematic_motion` advances one fixed tick, writes the
logical transform and velocity, and calls `World::move_kinematic_body`.

The Jolt backend implements that operation with `BodyInterface::MoveKinematic`
and rejects non-kinematic bodies or invalid deltas. Jolt therefore derives the
body velocity used by contact solving instead of observing a teleport.

Factory composes a cargo lift from transform, motion, server authority,
replication and `KinematicBodyComponent`. It moves vertically between two stops,
dwells at both endpoints and owns network entity 100. Its authoritative position
and velocity are included in `SliceSnapshot`.

Protocol v10 appends one bounded `KinematicMechanismView` to gameplay snapshots.
Local, host and remote presentation consume the same view; joining clients do
not advance an independent clock for the lift.

## Consequences

- Kinematic gameplay motion is reusable and deterministic at fixed tick rate.
- Moving bodies expose meaningful Jolt velocity and can carry/push contacts.
- Mechanism state is server authoritative and identical for both remote clients.
- The first wire shape supports one Factory lift; a later generalized entity
  record can replace it when multiple authored mechanisms require relevance or
  delta budgeting.

## Validation

`gloom.physics` verifies velocity-derived movement and rejects use on a dynamic
body. `gloom.vertical_slice` verifies the lift composition and deterministic
motion. `gloom.vertical_slice_network` verifies both clients receive identical
authoritative lift identity and position through the real gameplay snapshot
codec.
