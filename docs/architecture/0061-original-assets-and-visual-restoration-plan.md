# ADR 0061: Original assets and visual restoration scope

## Status

Accepted as the plan for milestones 59–64. No completion or runtime support is
implied. Implements the user's narrowed scope: environment, models, animation/
effects and graphical UI (points 5–8); later work remains deferred.

Implementation update: ADR 0062 closes milestones 59–61 and records their
validated scope and remaining limitations. Milestones 62–64 remain planned.

## Evidence

Read-only legacy commit `fe59e723594cc13e0fc95a1f390d0f81f58dc45b` and the first
two minutes of the supplied video establish recoverable source content. Factory
is an Ogre v1.8 mesh with 16 submeshes and 16,351 triangles, accompanied by
material/texture scripts, client/server map definitions and textual PhysX RepX
collision. A static Assimp-to-glTF experiment recovers its geometry and selected
texture bindings. The old shader semantics are not automatically preserved.

Legacy model inventory contains 117 v1.8 and 16 v1.41 meshes. The experiment's
Assimp build rejects v1.41; these require offline normalization. Skeletons exist,
but the current Gloom cooker rejects animation/skinning. The experiment does not
validate bone/animation export.

## Decision

Preserve legacy sources read-only. Convert offline to glTF plus explicit Gloom
material/map metadata, then use the existing cooker and residency pipeline.
Normalize unsupported Ogre versions on copies. Keep source hashes, resource
resolution and conversion diagnostics. Do not import Ogre, PhysX or Flash into
the game runtime.

Extend material contracts through importer, cooked format, residency and GPU
shading together: specular and anisotropy maps are first-class scope, alongside
normal, roughness/metallic, AO, textured emission, UV controls and transparency.
Do not reinterpret legacy specular maps as metallic/roughness maps. Use standard
glTF material extensions where applicable and explicit metadata for procedural
effects. Unreal-like visual features do not imply Unreal asset/graph execution.

Reconstruct Factory from its original mesh and a normalized entity manifest,
with shared unit/axis policy across rendering, collision, hitscan and prediction.
The multiplayer TXT pair is the source of runtime placement; reconcile the
different standalone Factory.map instead of merging duplicates. Apply RepX actor
and shape transforms explicitly. Validate original routes and source placement
before replacing the modern blockout.

Correct the art reference behind ADRs 0028/0042/0044: legacy Hound uses the
Archangel humanoid mesh, and Berserker is a Hound ability in the inspected source.
The existing modern roster tuple remains unchanged in this planning revision;
its identity/compatibility must be resolved before final character art. Model
bounds must never silently determine authoritative hit volumes.

Implement rig import and skinning before calling character recovery complete.
Drive animation/VFX from the existing authoritative/predicted presentation
contract and rebuild graphical UI around existing matchmaking/lobby behavior.

## Acceptance and sequencing

See [ART_RESTORATION.md](../ART_RESTORATION.md) for source-level findings and
separate gates: 59 asset pipeline, 60 materials, 61 Factory, 62 characters/weapons,
63 animation/VFX, 64 UI. Milestone 58's image comparisons remain a regression
gate; new references require inspection, not automatic replacement. Networking
tests remain required when integrating map/collision and roster presentation.
