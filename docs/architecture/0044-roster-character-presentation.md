# ADR 0044: Character identity selects a presentation recipe

## Status

Accepted.

## Context

Berserker replicated correctly but the renderer still assembled the same nine
Hound body parts and changed only their colors. That made roster identity hard
to read and embedded Hound-specific offsets directly in the executable's frame
loop. Future authored models need a stable selection seam that cannot influence
physics or combat.

## Decision

Define a renderer-neutral `CharacterPresentationRecipe` selected only from the
replicated `SliceCharacter`. A recipe owns a stable authored-scene URI and a
builtin fallback made from local part offsets, scales and colors.

Keep Hound's existing quadruped silhouette and ability-tint roles. Give
Berserker an upright humanoid fallback with a torso, head, paired shoulders,
arms and legs, plus a chest accent. Both currently use nine parts so the fixed
slice presentation allocation remains bounded.

The renderer applies world transform, facing, damage flash, death collapse and
respawn feedback after recipe selection. Ability tint remains a presentation
effect on marked Hound parts. Recipe data never defines the Jolt capsule,
hitscan volume, movement, health or any authoritative state.

## Consequences

- Berserker is identifiable by silhouette instead of color alone.
- Adding or replacing a roster visual no longer adds character-specific body
  assembly branches to the frame loop.
- Each character has an independent path for future cooked content and retains
  a visible fallback during loading or failure.
- Gameplay envelopes remain stable regardless of authored mesh bounds.

ADR 0045 connects source-controlled cooked scenes to these recipe slots while
preserving the fallback and gameplay separation defined here.

## Validation

`gloom.vertical_slice` requires both recipes to contain nine parts, use distinct
authored URIs and encode different quadruped/humanoid landmarks. The Vulkan
vertical-slice and synchronization tests render through the common recipe loop;
network tests continue to establish that the chosen identity is authoritative.
