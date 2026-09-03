# ADR 0063: Original character bind rigs and material identity

## Status

Accepted for milestone 62. Extends ADRs 0044/0045 and 0061/0062. Dynamic
skinning, clip evaluation/blending and particles were deferred to milestone 63;
their current implementation is specified by [ADR 0064](0064-animation-and-combat-effects.md).
The decisions below retain the historical milestone-62 bind-pose contract.

## Decision

Recover the two available source models: Archangel and Shadow. Preserve their
geometry, normal/UV data, skeleton hierarchy, inverse bind matrices and every
positive bone influence. Archangel has 43 bones and up to four influences;
Shadow has 17 bones and five influences on some vertices. Support two glTF
JOINTS/WEIGHTS channels (eight influences) rather than truncating the source.
Scene payload version 4 carries these data and node-to-skin bindings. LOD and
tangent processing preserve the joint/weight tuple when splitting/remapping
vertices. Residency retains the cooked rig for attachment lookup.

The runtime currently displays the authored bind pose, whose skin palette is
validated to reproduce the static mesh. This is not animated skinning support.
Animation-bearing glTF is still rejected explicitly. The offline manifest
records Archangel's forward, idle, jump and strafe_right clips; Shadow has none.
Their playback, completion and GPU deformation belong to 63.

Read normalized Ogre vertex declarations directly during this conversion.
This preserves original vertex order/weights and handles Shadow's packed color
layout, which this Assimp binding rejects after normalization. Original shaders
do not consume that vertex color. Preserve Ogre UV orientation rather than
Assimp's automatic V inversion; the recovered Soul Reaper presentation uses
this same corrected route. Prior Factory conversion outputs remain unchanged.

Archangel is already upright and faces engine +Z. Shadow's source head points
toward -Y; rotate its presentation root 180 degrees about X. Its skeleton is
also displaced far from its mesh: explicitly rebase the pelvis to
`(-0.29533267, -17, -101.80451965)` in source mesh coordinates and record the
translation delta. Recompute inverse bind matrices consistently. This is an
authored recovery adjustment, not evidence of a usable original animation set.
Both visual bodies have an explicit 1.8 m presentation height. Physics capsules,
damage, range and camera height are not inferred from model bounds or changed.

Use the supplied concept drawings to select materials. Archangel's gold armor
is metallic; cyan regions in the source diffuse atlas become a masked cyan HDR
emission map, including shoulders and wing tips. Dark undersuit stays mostly
dielectric. Shadow's armor is metallic with its original weathering; the source
eye mask produces red HDR emission, leaving body/cloth non-emissive. Split alpha
specular data from the legacy packed normal/specular texture into roughness;
keep normal data in BC5. These are parameterized material translations, not
particle effects or painted replacement meshes.

Add Archangel and Shadow to selection with Soul Reaper and no new ability
mechanics. Hound shares the Archangel body as in the original archetypes. Keep
wire ID 1 / `berserker-reaper` as a compatibility-only Hound presentation slot;
remove it from the offered original roster. Do not rename or recycle that ID
as Shadow. Protocol 13 prevents silently mixing clients with different roster
contracts. Original Archangel/Shadow powers and Screamer need separate content.

Attach third-person Soul Reaper to the recovered right-hand position with an
explicit grip offset. First-person presentation uses its own camera-relative
scale/offset and separate depth; muzzle feedback has an explicit visual anchor.
All authoritative rays still originate from the existing camera/player rules.
The presentation is rigid until 63; hand poses, dedicated FPS arms and weapon
grip animation are not represented as completed animation work.

## Validation

`gloom.character_restoration` checks both rigs, eight-channel serialization,
five-influence retention, head/hand placement, finite/normalized weights,
invalid joint references, lost skin bindings, altered inverse matrices,
cycles and old payload rejection. The real two-client GNS test uses Archangel
and Shadow and checks reciprocal identity and movement on original Factory.

`gloom.character_visual_review` captures seven inspected views through normal
residency and Vulkan: front/back of both bodies and three weapon camera angles.
Two explicit inspection lights make metal response readable; they are not
added to normal gameplay. Older Factory/blockout image gates remain intact.
See [CHARACTERS.md](../CHARACTERS.md) for commands, references and known limits.

Technical references: [glTF skins](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#skins),
[Ogre vertex declarations](https://github.com/OGRECave/ogre/blob/v14.5.2/OgreMain/include/OgreHardwareVertexBuffer.h).
