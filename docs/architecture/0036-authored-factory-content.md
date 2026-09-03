# ADR 0036: Authored Factory content remains presentation-only

## Status

Accepted.

## Context

Factory's cargo lift already exists as an authoritative logical entity, a Jolt
kinematic body and protocol-v10 mechanism state. Its builtin cube was useful for
the gameplay migration but did not exercise the production content pipeline in
the playable slice. Replacing the cube must not let render asset availability,
node transforms or artist-authored bounds change collision or network behavior.

## Decision

Keep `assets/factory/cargo_lift.gltf` as source-controlled authoring content and
cook it during the build to `content/factory/cargo_lift.gasset`. The executable
discovers that cooked scene through `game:/` and `cache:/` mounts, loads it with
the asynchronous asset loader and requests critical residency through the
existing coordinator.

The presentation initially uses the builtin cube. Once a resident scene
generation is available, it swaps only the visual mesh. The transform continues
to be driven by `SliceSnapshot::factory_lift`. Failure to discover, load,
prepare or upload the authored scene is reported once and leaves the fallback
visible.

The logical entity composition, ping-pong motion, Jolt box shape, gameplay
dimensions and protocol encoding are unchanged. Authored mesh data is never a
source for collision or replication.

## Consequences

- Normal builds produce the exact cooked asset needed by the playable slice.
- Local, listen-host and joining clients render the same asset while consuming
  their existing authoritative mechanism state.
- Asset streaming cannot remove or relocate the gameplay body.
- Artists may revise presentation independently, provided the established
  gameplay envelope remains visually represented.
- Later Factory surface replacements must preserve this separation, including
  the entity-owned lava sensor and deterministic blockout collision.
