# ADR 0028: Hound, Bite and Factory content slice

## Status

Accepted for protocol v9.

## Context

The vertical slice proved movement, combat, reconnect and lobby authority, but
its combatants and arena were generic proxies. The legacy project contains four
player classes and two maps, yet importing its Ogre, PhysX and message-component
runtime would reverse the modern engine's backend-neutral boundaries.

The legacy `archetypes.txt`, `Hound.cpp` and `Bite.cpp` establish Hound's first
skill as a directional half-second bite charge with a 25-second primary-skill
cooldown. Its contact deals 40 damage and grants 40 life. `Factory_server.txt`
establishes the broad industrial play space, opposing spawn distribution,
elevated routes and lava as recognizable map features.

## Decision

Hound is the first `SliceCharacter` and is part of every combatant snapshot.
Facing and the active-ability flag are replicated independently of renderer
state. `SliceInput::use_primary_ability` is a gameplay command; SDL maps it to
`Q` and right mouse, but the simulation has no SDL dependency.

Bite is owned by fixed-tick authority. A valid activation stores a normalized
horizontal direction, starts a 30-tick charge window and a 1,500-tick cooldown,
overrides movement along that direction, and permits one lag-compensated contact.
That contact applies the audited 40 damage and heals the attacker by the audited
40 life, capped at the legacy maximum life. Death, score and respawn continue
through the existing combatant rules.

Protocol v9 adds a distinct `ability_command` envelope with sequence, owned
entity and aim. The remote host validates session ownership, lobby phase and
monotonic sequence before forwarding it. Clients predict the same 30 movement
inputs and reconcile normally. Gameplay snapshots add character, facing,
ability-active/cooldown and authorization telemetry. Weapon fire remains a
separate command and cooldown.

`ArenaBox` now carries a presentation-neutral material role. The initial Factory
blockout uses builtin boxes for the floor, furnace, service blocks, pipe/trim
silhouette and lava channel. The furnace, service blocks and four perimeter
walls are explicitly marked solid and generate the deterministic authoritative
obstacle list from the same arena records; overhead
trim and lava remain non-solid landmarks pending authored collision and hazard
rules. The renderer composes a primitive multi-part Hound, FPS claws and ability
feedback from snapshot data.

Hitscan validation represents each character with a configurable vertical
capsule rather than a torso-only sphere. The slice capsule covers the visible
Hound body through its upper silhouette while preserving nearest-target choice,
historical rewind and static-world occlusion.
The zero-latency local slice presents the current authoritative opponent state;
it does not apply a synthetic interpolation delay that would move the rendered
Hound behind its hit volume. Network clients continue to identify the presented
snapshot tick so the host can rewind to it.

`SlicePresentationFeedback` observes monotonic authoritative snapshots and emits
short renderer-neutral damage, death and respawn cues. The renderer maps those
cues to a damage flash, collapsed corpse and respawn tint; the existing
server-confirmed hit-marker bit drives the reticle marker. None of these cues
predicts or mutates combat outcomes.

Authoritative movement also resolves enabled combatants as horizontal capsules
before producing snapshots. Dead combatants are disabled from that contact set
until respawn. This prevents players from crossing or remaining interpenetrated
without making the generic movement layer depend on combat state.
Clients receive the currently visible living combatants as prediction collision
entities and reproduce their half of the server separation. Respawn is an
explicit prediction reset: pre-respawn inputs are discarded instead of replayed
from the new spawn point. This keeps the FPS camera, authoritative shot origin
and rendered target aligned through death/contact cycles.

## Consequences and limits

- Character mechanics, command authority and map metadata remain independent of
  SDL, Diligent, Jolt and GameNetworkingSockets.
- Hound's damage, life steal, duration and cooldown match the legacy data. The
  original `biteSpeed = 4.0` cannot be transferred literally until a common
  legacy-to-modern unit policy exists; this slice uses normal movement speed
  during the authoritative charge.
- Factory is recognizable blockout data, not a conversion of its original mesh,
  detailed collision, pickups, vertical navigation or lava damage.
- Primitive Hound geometry is presentation scaffolding, not the final cooked
  character asset or animation rig.
- Character selection, Berserker, the other classes and their abilities remain
  future milestones.

## Validation

`gloom.vertical_slice` checks the Factory/lava metadata, Hound identity, Bite
activation, cooldown, one-hit 40 damage, 40 life steal, solid Factory boxes,
combatant separation, upper-silhouette weapon hits and authority metrics.
It also verifies that confirmed damage, death and respawn transitions create and
expire their presentation cues.
`gloom.vertical_slice_network` verifies exactly-once protocol-v9 ability routing,
replicated character/cooldown state and prediction correction alongside the
existing kill, respawn, reconnect and adverse-link cases. The Vulkan vertical
slice smoke validates the expanded scene and FPS presentation.
