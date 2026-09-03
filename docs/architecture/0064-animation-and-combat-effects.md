# ADR 0064: Animation, GPU skinning and bounded combat effects

## Status

Accepted for milestone 63; 38/38 Debug tests and visual acceptance recorded in
[the delivery report](../../reports/animation-vfx-2026-09-03/README.md).
Extends ADRs 0021, 0044/0045 and 0063. This ADR number does not designate UI milestone 64.

## Decision

Recover Archangel's four Ogre clips as absolute local TRS channels. Preserve
Shadow's documented root rotation/rebase and all eight influence slots; author
its motion procedurally because the source has no clips. Cooked scene version 5
carries validated LINEAR/STEP channels, with explicit rejection of unsupported
interpolation, morphs and malformed data. Ogre remains an offline tool.

Evaluate and blend on CPU, including short-arc quaternion interpolation, a
speed-driven locomotion blend, procedural combat layers and two-bone arm IK.
Suppress recovered pelvis translation as authoritative root motion. Weapon
grip and muzzle attachments follow the evaluated pose. Reuse recovered arm
triangles for FPS, with authored shoulder placement below the camera and the
same arm lengths/materials. Equip is a visual transition on identity/lifetime
changes; there is no fabricated reload or weapon-switch mechanic.

Use GPU vertex skinning for position, inverse-transpose normals and forward
tangents, previous-frame deformation for motion vectors, and the same current
pose in shadows. Compute conservative animated bounds from deformed vertices.
Keep a separate vertex-stage skin buffer: putting a 48 KiB palette in every
static draw exhausted Diligent's 8 MiB Vulkan dynamic heap during acceptance.
The corrected path allocates skin data only for animated draws and one initial
frame binding. Static shaders take the unchanged zero-skin branch.

Store previous poses by presented instance, invalidating on discontinuities.
Death collapse is applied around a normalized presentation-space pivot rather
than Shadow's distant source origin. Camera and authoritative physics remain
independent of the skeleton.

Choose bounded CPU analytical particles for this two-player scene. Data recipes
control bursts/rates, life, initial direction/speed/spread, gravity, size/color,
trail length and distortion. Stable seeds and chronological births make life
and saturation independent of the frame partition. Drop newest at capacity;
cap 2,048 live particles and 64 emitters. Entity cancellation and scene teardown
remove persistent state. The dedicated server contains no cosmetic simulation.

Translate original textures/material intent, not Particle Universe bytecode.
Use sorted alpha/additive quads with depth tests, soft depth intersections and
HDR/bloom. Refraction samples explicit opaque color/depth snapshots with linear
normal data, foreground rejection and an edge mask. Separate world/FPS passes
preserve weapon depth. Do not write transparent depth or fictitious particle
motion into temporal buffers. Refraction does not recursively refract other
transparent particles.

Protocol 14 appends cosmetic pitch and accepted-shot sequence/tick/impact data,
without recycling character IDs or changing combat rules. Emit transient fire
effects only from confirmed snapshots. This deliberately accepts confirmation
latency instead of showing speculative muzzle/impact effects that could duplicate
or survive rejection. Repeated or stale snapshots cannot replay events; reconnect
establishes a baseline. Other effects derive from authoritative health, grounded,
alive, shield and lifetime transitions. Blood/explosion remain review samples.

## Validation and consequences

Tests cover recovered clips, serialization/rejection, bind/deformed vertices,
actual five-influence Shadow data, grip reach, bounds, temporal cuts, fixed-rate
poses, deterministic/saturated particles and event deduplication/cleanup. Real
GNS clients traverse original Factory, fire, kill, wait 240 ticks and reconnect,
both through host/join and the dedicated executable. Fixed-step Vulkan sequences
compare 30/60/144 Hz under inspection/game lighting; earlier visual references
remain unchanged. See [commands and limitations](../ANIMATION_VFX.md).

The pre-existing HTTPS quota test also exposed a wall-clock assumption during
baseline verification. Its deterministic test now renews an exhausted quota
with a fixed clock; HTTPS acceptance allows only the refill justified by actual
elapsed time. Production quotas and authorization are unchanged.

Technical source for Ogre keyframe composition:
[Ogre NodeAnimationTrack::applyToNode](https://github.com/OGRECave/ogre/blob/v14.5.2/OgreMain/src/OgreAnimationTrack.cpp).
