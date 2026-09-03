# ADR 0033: Legacy character physics on Jolt

## Status

Accepted.

## Context

The Hound composition still used deterministic replication movement for its
gameplay state and a teleported box solely so Factory sensors could see it. That
proxy did not reproduce the original controller's stair traversal, floor
adhesion, slope policy, contact reporting or external impulses.

## Decision

`CharacterPhysicsComponent` is the stable owner of an optional backend character
handle. Server compositions attach it to a Jolt `CharacterVirtual`; data-only and
client compositions can retain the same component without constructing physics.
It releases the handle safely across entity destruction and physics-world
restart generations.

The controller uses a foot-origin capsule and configurable step-up, step-down,
maximum-slope, mass and push-force values. Fixed-step movement preserves ground
velocity, applies gravity only while unsupported, rejects motion into steep
slopes through Jolt's character solver, walks stairs, sticks to nearby floor and
accepts explicit velocity impulses. State reports ground normal and velocity.

Each character has a Jolt inner capsule body carrying its logical entity ID.
This makes it visible to rigid bodies and sensors without a second kinematic box.
A character contact listener queues deterministic enter/stay/exit events after
simulation, including both logical identities, body ID, point and normal.

The vertical slice attaches both Hounds through this contract and consumes
character/sensor contacts for Factory damage volumes. ADR 0034 subsequently
moved snapshot capture/application policy onto component state while retaining
the established fixed-tick movement seam.

## Consequences

- Character collision shape, trigger identity and lifetime are one composition.
- Dynamic bodies can be pushed subject to the configured strength and characters
  can receive gameplay impulses.
- Stair, floor and slope tuning is explicit and backend-independent at the public
  contract.
- Network schemas and prediction policy are deliberately unchanged.
- A dead character releases its virtual capsule and recreates it at the spawn;
  this prevents gravity/contact updates from leaving a stale physical hitbox
  away from the corpse or respawn position.

## Validation

`gloom.physics` covers fixed stepping, grounded motion and jumping while the
vertical-slice regression covers component composition, Factory collision,
lava damage, death and respawn using the attached Jolt characters.
