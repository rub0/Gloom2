# ADR 0030: Jolt-backed physics components and trigger events

## Status

Accepted.

## Context

ADR 0029 established logical entity identity and component lifetime, but physics
bodies still had no logical owner and gameplay had no reusable trigger contract.
Encoding lava overlap directly in `ArenaBox` would duplicate Jolt collision and
bypass its sensor/contact machinery. Running gameplay from a Jolt callback would
also be unsafe because contact callbacks may execute concurrently while physics
bodies are locked.

The component boundary must support future static, dynamic and kinematic map
entities as well as triggers, while keeping Jolt types out of public engine
headers. Character physics was subsequently migrated from the original Gloom
behavior by ADR 0033.

## Decision

Gloom defines distinct move-only `StaticBodyComponent`, `DynamicBodyComponent`,
`KinematicBodyComponent` and `TriggerComponent` types. Each component owns a
Gloom `BodyId`, associates it with a generational `EntityId`, and releases the
backend body when the component is removed or its entity is destroyed. Physics
world lifetime must enclose the lifetime of its attached components; stopping
the world first releases all backend bodies and makes later component cleanup a
no-op. Components retain the physics-world instance generation so stale handles
from a stopped world cannot destroy bodies after that world is restarted.

`BodyDesc::owner` carries logical identity through the backend boundary.
`JoltWorld` stores the packed identity in Jolt body user data and exposes
body-to-entity lookup. Entity-to-body lookup remains the normal typed component
lookup in `EntityRegistry`.

`TriggerComponent` creates a static Jolt body with `mIsSensor`. A single backend
`ContactListener` receives `OnContactAdded`, `OnContactPersisted` and
`OnContactRemoved`. It aggregates sub-shape contacts into logical body pairs,
preserves the entity/body identities required by removal callbacks, and queues
`entered`, `stayed` and `exited` events. Callbacks only append backend data under
a mutex; gameplay drains deterministically sorted events after
`PhysicsSystem::Update` returns.

Only contacts for logically owned bodies become logical trigger events. Sensors
never produce collision response. Multiple fixed substeps retain their exact
simulation-step number in the event stream.

## Consequences and limits

- Reusable trigger semantics now come from Jolt rather than manual map overlap.
- Contact callbacks never execute gameplay while Jolt holds body locks.
- Entity destruction releases component-owned Jolt bodies automatically.
- Static, dynamic and kinematic body roles share one lifetime implementation but
  remain distinct component types in the registry.
- Trigger components remain static; ADR 0035 establishes reusable kinematic
  solid-body motion. Moving sensors can build on the same tick policy when needed.
- ADR 0033 adds a distinct queued `CharacterVirtual` contact contract and an
  inner capsule body for participation in body sensors.
- Factory is not migrated in this decision. Its existing lava blockout remains
  visual-only until ADR 0031 can introduce the floor opening, surface, trigger
  entity and damage-volume component together.

## Validation

`gloom.physics` runs without SDL or rendering. It verifies all four component
roles, bidirectional entity/body association, sensor enter/stay/exit ordering,
pass-through behavior, and sensor destruction through logical entity lifetime.
The fixed-step, rigid-body and migrated character tests remain in the same
headless regression.
