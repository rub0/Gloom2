# ADR 0009: Interactive network scene and character adapter

## Status

Accepted as the first interactive network laboratory

## Context

The headless movement slice proves protocol and reconciliation semantics, but
does not expose their behaviour through the platform, physics and rendering
boundaries. The next step needs to remain useful as an experiment: networking
must not acquire SDL, Jolt or Diligent types, and the deterministic reference
simulation must not be confused with a complete physics rollback system.

## Decision

The platform API now exposes a backend-neutral input snapshot. The SDL3 backend
maps WASD, the arrow keys and Space into that contract; gameplay reduces the
directional buttons to planar axes without including SDL headers.

The physics API owns an opaque character handle and a small character state.
Its Jolt backend uses `CharacterVirtual`, a capsule translated so its public
position represents the feet, a supporting-volume plane and `ExtendedUpdate`
with the world's collision filters and gravity. This follows Jolt's official
CharacterVirtual sample while keeping Jolt types behind the backend boundary.

`gloom --network-demo` runs an in-process laboratory with:

- a 60 Hz authoritative server and 20 Hz snapshots;
- immediate local prediction and reconciliation;
- delayed interpolation for a server-controlled remote entity;
- independent deterministic upstream and downstream links with latency,
  jitter, loss, duplication, reordering, bandwidth and queue limits;
- a blue local entity driven through the Jolt character adapter and an orange
  remote entity rendered from interpolated snapshots.

The authoritative and predicted movement model remains the deterministic
planar model from ADR 0008. The Jolt character is currently a presentation and
collision adapter driven by predicted velocity. If its horizontal position
diverges by more than 0.25 metres, it is corrected to the predicted position
while preserving its physical height. This is deliberately not described as
Jolt rollback or authoritative physics prediction.

Space is present in the input contract for the next character gameplay slice,
but jumping is not implemented in this milestone.

## Verification

Unit tests cover neutral, combined and cancelling input directions. Physics
tests create a virtual character, let it settle on a rigid floor, verify the
ground state, drive it horizontally for one second and destroy it cleanly.

`gloom.network_scene_smoke` runs 180 fixed-duration frames of scripted input,
the adverse-link simulation, character movement, snapshot interpolation and
Vulkan rendering. The real GameNetworkingSockets path remains covered by the
separate loopback integration test; the visual laboratory intentionally uses
the deterministic link so it is repeatable and needs no sockets or sleeps.

## Consequences

- Input, character physics, network simulation and rendering remain replaceable
  behind independent APIs.
- Network effects and local/remote presentation can now be studied visually.
- The automated graphical test guards the full interactive composition in both
  Debug and Release builds.
- Obstacles, jumping, authoritative collision agreement, correction smoothing,
  input redundancy and quantitative prediction telemetry remain future work.

## References

- [Jolt CharacterVirtual sample](https://github.com/jrouwe/JoltPhysics/blob/master/Samples/Tests/Character/CharacterVirtualTest.cpp)
- [Jolt architecture and character controllers](https://github.com/jrouwe/JoltPhysics/blob/master/Docs/Architecture.md)
