# ADR 0038: Authored trim and lava do not redefine the Factory hazard

## Status

Accepted.

## Context

ADR 0037 moved floor and industrial presentation to cooked modules but left
Factory trim and lava on builtin geometry. Lava is a particularly sensitive
boundary: its visible surface, the opening in static floor collision and the
entity-owned damage sensor describe related space, but only the latter two may
control gameplay.

## Decision

Extend `assets/factory/surface_modules.gltf` with two more stable mesh entries:
an oxidized trim module and a dedicated lava plane. Keep the scene's instance
order as floor, industrial, trim and lava. Presentation records are collected
from immutable `ArenaMaterial` classifications and all four meshes are applied
only when one complete resident scene generation is available.

Trim uses the same normalized box-envelope mapping as the existing authored
modules. Lava uses its own upward-facing XZ plane and the existing
`ArenaSurface` transform. Until residency, or after a loading failure, trim uses
the builtin cube and lava uses the builtin horizontal quad.

The authored plane is not collision. The split `ArenaBox` floor remains the
source of the open channel, and the logical lava entity retains its independent
Jolt `TriggerComponent` and `DamageVolumeComponent`.

## Consequences

- The selected Factory blockout presentation is now fully served by cooked
  assets: mechanism, floor, industrial blocks, trim and lava.
- One residency request atomically supplies all static modular surfaces.
- Local, host and remote views need no new protocol data for immutable modules.
- Visual revisions cannot close the channel, enlarge damage coverage or alter
  environmental kill attribution.
- Character/loadout work can follow without carrying an unfinished Factory
  presentation migration.

## Validation

The cooker must accept the four-mesh scene, including the indexed upward-facing
lava plane. The vertical-slice graphical smoke only records a successful module
generation when all four instances exist. Existing headless tests independently
verify the absence of a lava box, the one-sided surface metadata, Jolt overlap
damage and respawn behavior.
