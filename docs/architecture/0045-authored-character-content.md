# ADR 0045: Authored character scenes substitute complete recipes after residency

## Status

Accepted.

ADR 0063 supersedes the nine-part runtime art contract for original characters.
Their complete resident scenes retain authored materials/transforms and bind
rigs. The nine-part libraries remain as fallbacks and regression fixtures.

## Context

ADR 0044 established distinct Hound and Berserker recipes but both still drew
builtin cubes. Character content needs to exercise the same offline cooking and
asynchronous GPU path as Factory without letting a partial load remove the
fallback or letting authored bounds redefine combat.

## Decision

Version `assets/characters/hound.gltf` and `berserker.gltf`, each containing
nine ordered part instances corresponding to its fallback recipe. Cook both
during the normal build to `content/characters/*.gasset`.

Discover both through the existing source/cache VFS. Give each character an
independent catalog, loader, residency coordinator, ticket and generation.
Until a resident scene exposes exactly nine instances, render every builtin
part. Once complete, replace the nine mesh IDs atomically for that character.
Failure is reported once and retains the fallback.

Continue to derive part placement, facing, death collapse, damage/respawn cues
and ability tint from recipe and snapshot state. Authored node transforms,
bounds and materials do not alter Jolt physics, lag-compensated hit volumes,
health, movement or network state.

## Consequences

- Normal builds always produce the two runtime character payloads.
- Hound and Berserker can load or fail independently.
- Runtime never imports glTF and never presents a half-loaded character.
- Artists can replace the primitive source meshes within the nine-part contract
  without touching gameplay code.

## Validation

The build must cook both source scenes. `gloom.vertical_slice` verifies distinct
source and cooked recipe paths. The Vulkan vertical-slice smoke fails unless
both complete scenes reach GPU residency, while existing combat, physics,
network and respawn tests verify that gameplay behavior remains unchanged.
