# ADR 0024: FPS and authoritative session slice

## Status

Accepted.

The in-process transport limit below was subsequently removed by
[ADR 0025](0025-remote-slice-host-join.md).

## Context

Milestone 20 proved the local combat loop, but its presentation used an overhead
camera and its primary attack selected the only opponent automatically. The
existing protocol-v6 session, prediction and lag-compensated combat foundations
were still exercised by separate laboratories rather than by the playable slice.

## Decision

The playable slice uses a device-independent `FirstPersonController`. SDL only
supplies relative look deltas and button/axis state. The controller owns clamped
yaw and pitch, transforms local movement into world axes and produces the
explicit normalized 3D camera direction sent in `SliceInput`. No target identity
or auto-aim result crosses the gameplay boundary.

`VerticalSliceSimulation` composes two `ClientSession` instances with a
`ServerSessionManager`. Both clients complete encoded handshake and clock
exchange paths, own distinct entities and route redundant movement input and
fire commands through the session authorization policy. An
`AuthoritativeMovementServer` advances the world, prediction clients consume
their per-client snapshots, and `LagCompensatedCombatServer` validates fire
against recorded authoritative history and static occlusion before gameplay
applies legacy damage.

The composition is deliberately in-process. It exercises the same encoded
messages, ownership checks and per-client authority seams without making the
backend-neutral gameplay layer depend on GameNetworkingSockets. A later host/join
adapter may carry those messages across remote peers without changing the FPS,
movement, session or combat APIs.

## Consequences

- Mouse look and movement now behave as an FPS, and the local body is not drawn
  through its own camera.
- Missing, invalid or occluded aim no longer damages the opponent.
- The hostile combatant is a second admitted session rather than privileged AI
  state; its generated input follows the same authorization path.
- Snapshots expose active sessions, authorized input/fire, rejection and
  reconciliation metrics for tests and shutdown diagnostics.
- Relative mouse capture is a platform capability, not an SDL type exposed to
  gameplay.
- Remote connection lifecycle and host/join UI are not claimed by this decision.

## Validation

`gloom.vertical_slice` checks camera-relative movement and mouse orientation,
session admission, explicit aim, cover, authorized commands, shield, cooldown,
death and respawn. `gloom.vertical_slice_smoke` sidesteps cover, aims from the
presented player to the opponent, renders through Vulkan for 360 fixed frames
and fails unless it observes both a kill and the four-second respawn.
