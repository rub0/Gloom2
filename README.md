# Gloom

Modern revival of the original Gloom project, built as a modular C++23 engine.

Para continuar el desarrollo en una tarea nueva: [estado y traspaso actual](docs/ESTADO_ACTUAL.md).

Milestones 59–61 restore the original Factory mesh, placed static objects,
materials and collision in local, host/join and dedicated play. The offline
Ogre conversion is reproducible; specular/anisotropic shading, textured glow,
UV animation and a baked environment probe run through the real asset pipeline.
See [FACTORY_RESTORATION.md](docs/FACTORY_RESTORATION.md) and the
[video comparison](reports/factory-restoration-2026-09-02/README.md).
Milestone 62 recovers Archangel/Hound, Shadow and Soul Reaper with concept-led
metallic/emissive materials, preserved bind rigs and first-/third-person weapon
anchors. See [CHARACTERS.md](docs/CHARACTERS.md). Milestone 63 adds recovered
clips, procedural Shadow motion, GPU skinning, posed FPS arms and bounded combat
particles. See [ANIMATION_VFX.md](docs/ANIMATION_VFX.md) for reproducible temporal
reviews and limits. Milestone 64 restores the graphical HUD, menu, match browser,
lobby and selection using recovered original artwork and fonts. See
[UI.md](docs/UI.md) and its [acceptance report](reports/ui-2026-09-04/README.md).

Original gameplay migration now starts with milestone 65: class-specific
momentum, ground/air movement, jump and double-tap WASD dodge in Factory.
Authority and prediction share the recovered motion rules; arsenal replication
advances current peers to protocol 16.
[GAMEPLAY_MIGRATION.md](docs/GAMEPLAY_MIGRATION.md) documents the conversion;
[ARSENAL.md](docs/ARSENAL.md) covers the completed five-weapon milestone. Factory
pickups remain scheduled for milestone 67.

Factory visual corrections and reproducible screenshot checks are documented
in [VISUAL_REVIEW.md](docs/VISUAL_REVIEW.md). Run
`gloom.exe --visual-review .cache/visual-review` to capture the nine inspection
views of the retained blockout regression scene. Use
`gloom.exe --factory-review .cache/factory-review` for six views of the restored
map. Image checks complement gameplay/network tests.

The repository is intentionally starting with a small executable foundation.
Its subsystem contracts keep platform, rendering, physics and networking code
independent from their chosen third-party backends.

The executable without arguments opens the graphical menu in a resizable,
high-DPI SDL3 window. Choose local play, browse matches, connect directly or
create a development room. Mouse and keyboard navigation share the same scaled
layout; Esc opens pause during play. Existing CLI tools remain available.

The interactive network laboratory can be launched with:

```powershell
.\build\windows-vs\Debug\gloom.exe --network-demo
```

Move the blue locally predicted Jolt character with WASD or the arrow keys and
jump with Space. Static obstacles, gravity and jumping use shared deterministic
client/server rules; the orange server-controlled character is rendered from
delayed snapshots while a repeatable adverse link injects latency, jitter, loss
and reordering.

The first playable Gloom vertical slice can be launched with:

```powershell
.\build\windows-vs\Debug\gloom.exe --vertical-slice
```

Look with the mouse, move relative to the first-person camera with WASD or the
arrow keys, jump with Space and fire the Soul Reaper with the left mouse button.
Movement and fire are authorized through two entity-owning multiplayer sessions;
the server validates the camera ray against authoritative history and cover.
Soul Reaper is present in every current loadout and has a 0.5-second cooldown. Bite is
available only in `hound-bite` through Q or the right mouse button and has the
legacy 25-second cooldown; `hound-reaper` intentionally has no Q/RMB ability.
`hound-guard` uses Q/RMB to grant 50 shield, with a one-second blue cue and a
15-second cooldown. `archangel-reaper` and `shadow-reaper` select the recovered
original characters with Soul Reaper and no Q/RMB ability. The former
`berserker-reaper` ID remains a compatibility Hound slot; the original source
defines Berserker as a Hound ability, not a separate recovered character.

Run the slice across a real GameNetworkingSockets connection with two processes:

```powershell
# Host
.\build\windows-vs\Debug\gloom.exe --vertical-slice-host 0.0.0.0:27020

# Join (same machine; replace the address for another host)
.\build\windows-vs\Debug\gloom.exe --vertical-slice-join 127.0.0.1:27020

# Optional alternate loadout: name followed by hound-reaper
.\build\windows-vs\Debug\gloom.exe --vertical-slice-join 127.0.0.1:27020 Nyx hound-reaper

# Shield ability loadout
.\build\windows-vs\Debug\gloom.exe --vertical-slice-join 127.0.0.1:27020 Nyx hound-guard

# Compatibility Hound slot (weapon only)
.\build\windows-vs\Debug\gloom.exe --vertical-slice-join 127.0.0.1:27020 Nyx berserker-reaper

# Original character presentations (weapon only)
.\build\windows-vs\Debug\gloom.exe --vertical-slice-join 127.0.0.1:27020 Nyx archangel-reaper
.\build\windows-vs\Debug\gloom.exe --vertical-slice-join 127.0.0.1:27020 Nyx shadow-reaper
```

With a compatible HTTPS match service, set `GLOOM_MATCH_SERVICE_URL` in both
processes, `GLOOM_MATCH_IDENTITY_TOKEN` only in the client and
`GLOOM_MATCH_PUBLISHER_TOKEN` only in the dedicated server. The HTTPS host needs
the publisher token and `GLOOM_MATCH_IDENTITIES_FILE`, an operator-controlled
registry of individual credential digests, trusted principal ids and expiry.
The client exchanges its individual credential for a reader grant lasting at
most 60 seconds and renews it before subsequent reads. Neither
`GLOOM_MATCH_READER_TOKEN` nor `GLOOM_MATCH_SERVICE_TOKEN` is read.
Alternatively, set `GLOOM_MATCH_IDENTITY_PROVIDER=keycloak` in the client and
match host to use browser device sign-in and automatic account-token renewal.
The host verifies access tokens with Keycloak before issuing reader grants.
See [Keycloak setup](docs/KEYCLOAK.md) for the public client, confidential host
client, realm, scope and audience configuration. This mode does not use the
registry or manually provisioned client identity tokens.
List waiting matches with
`gloom.exe --vertical-slice-list` and join one with
`gloom.exe --vertical-slice-join match:<id> [name] [loadout]`. Direct IP does
not require either variable.

For the graphical browser use
`gloom.exe --vertical-slice-browse [name] [loadout]`. Left/Right selects a
waiting match and Enter revalidates it before connecting; without a loadout the
normal roster selector follows.

The matchmaking host is now available as `gloom_match_service.exe`, separate
from the game server. See [the deployment guide](docs/MATCH_SERVICE.md) for TLS
certificates, role-specific bearer configuration, durable storage and packaging.
Readers cannot publish or withdraw matches. Grants share their principal's
quota across renewal; publisher-authorized revocation suspends the principal
and survives service restart. Aggregate request budgets remain in place.
Enable `GLOOM_GAME_AUTH=tickets` in the client and dedicated server to require
verified account admission. The HTTPS service issues separate, signed,
single-use gameplay tickets lasting at most 30 seconds. Reconnect obtains a new
ticket and preserves account/session ownership. See [game authentication setup](docs/GAME_AUTH.md)
for signing keys, the `gloom.play` scope and process configuration. Discovery
grants and account access/refresh tokens are never sent to the game server.

When no loadout argument is supplied, the joining client opens the pre-match
roster selector. Use Left/Right to choose among the four validated loadouts and
Enter to confirm. The window title then shows the authoritative selections and
ready state for both lobby slots. Supplying a loadout argument keeps the useful
automatic path for scripted runs.

The joining process owns and predicts the player. The host admits the connection,
authorizes movement and fire, advances combat and returns gameplay snapshots. A
bar above the opponent shows confirmed authoritative damage; the center marker
grows red while the local player is dead and normal input resumes on respawn.

For two human players, run the headless authority and open two join processes:

```powershell
.\build\windows-vs\Debug\gloom_slice_server.exe 0.0.0.0:27020
.\build\windows-vs\Debug\gloom.exe --vertical-slice-join 127.0.0.1:27020 Nyx
.\build\windows-vs\Debug\gloom.exe --vertical-slice-join 127.0.0.1:27020 Reaper
```

Each join owns a distinct combatant and receives a reciprocal local-player HUD.
Names accept 1-20 ASCII letters, digits, `_` or `-`. Each interactive client
submits its confirmed character / Soul Reaper loadout and becomes ready only
after authority echoes that exact tuple;
authority starts simulation only after the two-player lobby is complete. Lobby
transitions and replicated selections are printed in every console.
If its connection drops, it retries after one second and resumes the same entity
within the server's ten-second grace period. If it does not return, the server
awards the other player an abandonment victory. Stop the dedicated server with
Ctrl+C.

## Current stack

- C++23 with Visual C++
- Gloom task groups and multithreaded job scheduler
- Generational logical entities and type-safe reusable component storage
- CMake + Visual Studio/MSBuild
- SDL3 platform layer
- Diligent Engine renderer with Vulkan, pinned to the upstream swapchain
  synchronization fix (`a255d24d`)
- Jolt Physics 5.6 backend with fixed-step, multithreaded simulation
- Entity-owned static, dynamic, kinematic and trigger components with queued
  Jolt sensor events
- Entity-owned Jolt virtual characters with capsule contacts, configurable
  stairs/floor adhesion and slopes, dynamic-body pushing and gameplay impulses
- GameNetworkingSockets 1.6 direct-IP transport backend
- fastgltf 0.9 offline glTF importer and Gloom's versioned cooked asset format
- Gloom render graph and renderer-owned asynchronous GPU asset registries
- KTX2 Basis texture cooking, semantic mipmaps and fence-safe GPU residency
- Priority/cancellation-aware scene residency, hot reload and BC7/BC5 selection
- MikkTSpace/meshoptimizer cooking, LODs, multithreaded culling and draw batches
- Motion vectors, jittered-camera native TAA, output-resolution history and
  backend-neutral temporal feature negotiation
- Depth-aware temporal rejection, reactive history, contrast-limited sharpening
  and GPU-timed dynamic resolution
- First-person Gloom combat slice with session-authorized movement, Soul Reaper
  hitscan, legacy balance and respawn
- Factory lava as a one-sided surface over a floor channel, with authoritative
  Jolt trigger and reusable damage-volume components
- Hound logical entities composed from transform, Jolt character physics,
  health, shield, movement, weapon, ability, authority, replication and
  presentation components
- Component-derived movement snapshots with explicit server, predicted-owner
  and interpolated-remote authority policies
- Tick-deterministic Jolt kinematic mechanisms, including Factory's replicated
  cargo lift in protocol v10
- Build-cooked Factory glTF presentation, beginning with a resident authored
  cargo lift that follows the existing replicated/Jolt mechanism and falls back
  to builtin geometry while loading or on failure
- Authored floor-panel and industrial modules selected by Factory arena material
  after GPU residency, with deterministic logical dimensions and lava behavior
  kept independent from their meshes
- Authored oxidized trim and an upward-facing lava surface in the same cooked
  module package, while the open channel, Jolt hazard sensor and damage volume
  remain authoritative
- Protocol-v11 authoritative pre-match character/loadout selection, currently
  validating Hound / Soul Reaper / Bite before the existing ready gate
- Selection-driven character components and gameplay snapshots, with
  `hound-reaper` as a supported Soul Reaper loadout whose ability slot is empty
- Visible first-person Soul Reaper with cooldown-driven muzzle feedback and
  authoritative ability readiness reconciled after commands and respawns
- Generalized selected ability component with Bite, empty-slot and Guard
  policies; Guard grants 50 authoritative shield on a 15-second cooldown
- Berserker roster foundation through the shared composition, physics, lobby
  and snapshot paths; only the evidence-safe Soul Reaper / empty ability tuple
  is accepted
- Interactive pre-match roster carousel with authoritative selection-before-
  ready ordering and both lobby choices visible in the window title
- Roster-neutral character presentation recipes with separate Hound quadruped
  and Berserker humanoid fallbacks plus stable per-character authored-asset
  slots
- Source-controlled Hound and Berserker glTF part libraries, cooked during the
  normal build and substituted only after independent GPU residency while the
  recipe geometry remains the loading/failure fallback
- Cooked first-person Soul Reaper body/barrel and Hound Bite/Guard props with
  independent residency, builtin fallbacks and unchanged authoritative combat
- Injectable verified-identity provider at host admission; development
  credentials remain the local default while opaque external tokens resolve to
  server-trusted lobby principals
- Backend-neutral match directory with revision-ordered advertisements,
  joinable protocol/capacity/phase filtering and resolution to the existing
  GameNetworkingSockets direct-IP endpoint
- Server-owned leased match publication with crash expiry, instance-safe
  takeover, lobby-transition refresh and orderly dedicated-server withdrawal
- Injectable persistent matchmaking-service adapter and deterministic client
  browse/select/re-resolve flow, converging with direct IP before transport
- Authenticated provider-connector boundary with opaque bearer credentials and
  bounded exponential startup retry before exposing a match directory
- Idempotent provider-side match core, refreshable service credentials and a
  non-blocking browser model for future in-window matchmaking presentation

Cook and inspect a glTF scene through the real runtime asset and Vulkan path:

```powershell
.\build\windows-vs\Debug\gloom_asset_cooker.exe D:\MyGame\assets D:\MyGame\cache game:/models/scene.gltf cache:/models/scene.gasset
.\build\windows-vs\Debug\gloom_scene_viewer.exe D:\MyGame\assets D:\MyGame\cache game:/models/scene.gltf cache:/models/scene.gasset
```

Pass an optional final copy count to turn the viewer into an imported-scene
visibility stress laboratory. For example, `10000` lays out ten thousand scene
copies and reports visible instances, LOD distribution, batch count and CPU
culling time when the window closes. Existing payload-v1 scenes need recooking.
Project presentation content is cooked automatically by the normal build into
`build/windows-vs/content`; the executable never imports glTF at runtime.

See [DEVELOPING.md](DEVELOPING.md) for build commands and
[docs/architecture/0001-foundation.md](docs/architecture/0001-foundation.md) for
the architectural decision. Vulkan synchronization details and the regression
test are recorded in
[docs/architecture/0002-vulkan-swapchain-synchronization.md](docs/architecture/0002-vulkan-swapchain-synchronization.md).
The physics boundary is documented in
[docs/architecture/0003-jolt-physics.md](docs/architecture/0003-jolt-physics.md).
The scene snapshot boundary is documented in
[docs/architecture/0004-render-snapshots.md](docs/architecture/0004-render-snapshots.md).
The engine scheduler is documented in
[docs/architecture/0005-job-system.md](docs/architecture/0005-job-system.md).
The message transport boundary is documented in
[docs/architecture/0006-gamenetworking-transport.md](docs/architecture/0006-gamenetworking-transport.md).
The authoritative simulation, protocol and adverse-network test model are
documented in
[docs/architecture/0007-network-simulation-model.md](docs/architecture/0007-network-simulation-model.md).
The first authoritative movement slice is documented in
[docs/architecture/0008-authoritative-movement-slice.md](docs/architecture/0008-authoritative-movement-slice.md).
The interactive input, CharacterVirtual adapter and visible network laboratory
are documented in
[docs/architecture/0009-interactive-network-scene.md](docs/architecture/0009-interactive-network-scene.md).
The selective prediction policy, deterministic static collision model and
correction smoothing are documented in
[docs/architecture/0010-character-prediction-policy.md](docs/architecture/0010-character-prediction-policy.md).
Protocol v3 introduced redundant input batches and their loss/bandwidth trade-off, as
documented in
[docs/architecture/0011-redundant-input-batches.md](docs/architecture/0011-redundant-input-batches.md).
Protocol v4 added quantized delta snapshots, acknowledged baselines and
per-connection relevance, as documented in
[docs/architecture/0012-quantized-delta-snapshots.md](docs/architecture/0012-quantized-delta-snapshots.md).
Protocol v5 added backend-neutral fire events, bounded authoritative
history and server-side lag-compensated hit validation, as documented in
[docs/architecture/0013-lag-compensated-combat.md](docs/architecture/0013-lag-compensated-combat.md).
Protocol v6 added authenticated session admission, entity ownership, reconnect,
live clock exchange, per-client replication state and bounded idempotent gameplay
events, as documented in
[docs/architecture/0014-multiclient-session-layer.md](docs/architecture/0014-multiclient-session-layer.md).
The virtual filesystem, asset catalog, glTF cooker and asynchronous runtime
loader are documented in
[docs/architecture/0015-asset-pipeline-foundation.md](docs/architecture/0015-asset-pipeline-foundation.md).
GPU residency, the PBR material baseline and render-graph scheduling are
documented in
[docs/architecture/0016-gpu-assets-and-render-graph.md](docs/architecture/0016-gpu-assets-and-render-graph.md).
KTX2 texture processing, PBR maps and bounded GPU residency are documented in
[docs/architecture/0017-texture-streaming.md](docs/architecture/0017-texture-streaming.md).
Scene coordination, hot reload and device-native texture selection are documented in
[docs/architecture/0018-scene-residency-coordinator.md](docs/architecture/0018-scene-residency-coordinator.md).
Offline mesh processing, LODs and the visibility path are documented in
[docs/architecture/0019-mesh-visibility-pipeline.md](docs/architecture/0019-mesh-visibility-pipeline.md).
The clustered HDR lighting frame, shadow cascades, tone mapping and GPU timing
contract are documented in
[docs/architecture/0020-modern-lighting-frame.md](docs/architecture/0020-modern-lighting-frame.md).
The temporal feature contract, motion vectors, history ownership, native TAA
and render/output resolution separation are documented in
[docs/architecture/0021-temporal-render-features.md](docs/architecture/0021-temporal-render-features.md).
Production temporal reconstruction and dynamic resolution are documented in
[docs/architecture/0022-production-temporal-reconstruction.md](docs/architecture/0022-production-temporal-reconstruction.md).
The first playable gameplay composition is documented in
[docs/architecture/0023-gloom-vertical-slice.md](docs/architecture/0023-gloom-vertical-slice.md).
Its first-person camera and authoritative multiplayer-session composition are
documented in
[docs/architecture/0024-fps-authoritative-slice.md](docs/architecture/0024-fps-authoritative-slice.md).
Protocol v7 and the remote GameNetworkingSockets host/join adapter are documented
in
[docs/architecture/0025-remote-slice-host-join.md](docs/architecture/0025-remote-slice-host-join.md).
The two-player authority, reconnect lifecycle and headless server are documented
in
[docs/architecture/0026-two-player-dedicated-slice.md](docs/architecture/0026-two-player-dedicated-slice.md).
Protocol v8 identity, lobby readiness and abandonment are documented in
[docs/architecture/0027-identity-lobby-match-lifecycle.md](docs/architecture/0027-identity-lobby-match-lifecycle.md).
The first migrated Hound, Bite and Factory content slice and protocol v9 are
documented in
[docs/architecture/0028-hound-bite-factory-content-slice.md](docs/architecture/0028-hound-bite-factory-content-slice.md).
The logical entity/component foundation is documented in
[docs/architecture/0029-entity-component-foundation.md](docs/architecture/0029-entity-component-foundation.md).
Jolt-backed physics components, logical ownership and trigger events are
documented in
[docs/architecture/0030-jolt-physics-components.md](docs/architecture/0030-jolt-physics-components.md).
Factory entity composition, its one-sided lava surface and Jolt-backed damage
volume are documented in
[docs/architecture/0031-factory-entity-migration.md](docs/architecture/0031-factory-entity-migration.md).
Hound logical character composition is documented in
[docs/architecture/0032-hound-character-composition.md](docs/architecture/0032-hound-character-composition.md).
The migrated legacy character controller, contact/impulse contract and owned
Jolt capsule are documented in
[docs/architecture/0033-legacy-character-physics.md](docs/architecture/0033-legacy-character-physics.md).
Component authority, component-derived snapshots and reconciliation policy are
documented in
[docs/architecture/0034-component-authority-replication.md](docs/architecture/0034-component-authority-replication.md).
Reusable kinematic motion and Factory's first networked mechanism are documented
in
[docs/architecture/0035-kinematic-factory-mechanisms.md](docs/architecture/0035-kinematic-factory-mechanisms.md).
The authored Factory asset boundary and presentation fallback are documented in
[docs/architecture/0036-authored-factory-content.md](docs/architecture/0036-authored-factory-content.md).
The modular Factory floor/industrial surface pass is documented in
[docs/architecture/0037-authored-factory-surface-modules.md](docs/architecture/0037-authored-factory-surface-modules.md).
The authored trim and hazard presentation boundary is documented in
[docs/architecture/0038-authored-factory-trim-hazard.md](docs/architecture/0038-authored-factory-trim-hazard.md).
Protocol v11 selection ordering and lobby authority are documented in
[docs/architecture/0039-character-loadout-selection.md](docs/architecture/0039-character-loadout-selection.md).
Selection-driven composition and the first alternate loadout are documented in
[docs/architecture/0040-selection-driven-composition.md](docs/architecture/0040-selection-driven-composition.md).
The generalized ability slot and Guard policy are documented in
[docs/architecture/0041-generalized-ability-guard.md](docs/architecture/0041-generalized-ability-guard.md).
The evidence-bounded Berserker roster foundation is documented in
[docs/architecture/0042-berserker-character-foundation.md](docs/architecture/0042-berserker-character-foundation.md).
Interactive roster confirmation is documented in
[docs/architecture/0043-interactive-roster-selection.md](docs/architecture/0043-interactive-roster-selection.md).
Roster-neutral character presentation is documented in
[docs/architecture/0044-roster-character-presentation.md](docs/architecture/0044-roster-character-presentation.md).
Authored roster content and its residency boundary are documented in
[docs/architecture/0045-authored-character-content.md](docs/architecture/0045-authored-character-content.md).
Authored first-person weapon and ability props are documented in
[docs/architecture/0046-authored-first-person-content.md](docs/architecture/0046-authored-first-person-content.md).
The verified identity-provider boundary is documented in
[docs/architecture/0047-verified-identity-provider.md](docs/architecture/0047-verified-identity-provider.md).
The advertised-match and join-resolution boundary is documented in
[docs/architecture/0048-match-discovery-directory.md](docs/architecture/0048-match-discovery-directory.md).
The leased dedicated-server publication lifecycle is documented in
[docs/architecture/0049-match-publication-lifecycle.md](docs/architecture/0049-match-publication-lifecycle.md).
The external-service adapter and unified client join flow are documented in
[docs/architecture/0050-service-matchmaking-client-flow.md](docs/architecture/0050-service-matchmaking-client-flow.md).
Authenticated provider connection and bounded startup retry are documented in
[docs/architecture/0051-authenticated-match-service-connection.md](docs/architecture/0051-authenticated-match-service-connection.md).
The concrete WinHTTP service and executable composition are documented in
[docs/architecture/0052-winhttp-matchmaking-composition.md](docs/architecture/0052-winhttp-matchmaking-composition.md).
Idempotent service mutations, token refresh and asynchronous browsing are
documented in
[docs/architecture/0053-idempotent-service-core-async-browser.md](docs/architecture/0053-idempotent-service-core-async-browser.md).
The in-window matchmaking state and lobby handoff are documented in
[docs/architecture/0054-in-window-match-browser.md](docs/architecture/0054-in-window-match-browser.md).
The runnable HTTPS host and durable store are documented in
[docs/architecture/0055-durable-https-match-host.md](docs/architecture/0055-durable-https-match-host.md).
Reader/publisher separation and bounded request budgets are documented in
[docs/architecture/0056-matchmaking-access-separation.md](docs/architecture/0056-matchmaking-access-separation.md).
Per-principal identity exchange, expiring grants, durable revocation and client
renewal are documented in
[docs/architecture/0057-per-principal-matchmaking-credentials.md](docs/architecture/0057-per-principal-matchmaking-credentials.md).
Keycloak account sign-in, renewal and host introspection are documented in
[docs/architecture/0058-keycloak-account-provider.md](docs/architecture/0058-keycloak-account-provider.md).
The ordered implementation milestones are tracked in
[docs/ROADMAP.md](docs/ROADMAP.md).
Original movement and the five-weapon authoritative arsenal are documented in
[docs/GAMEPLAY_MIGRATION.md](docs/GAMEPLAY_MIGRATION.md) and
[docs/ARSENAL.md](docs/ARSENAL.md). Gameplay uses `1`–`5`, left/right mouse and
`Q` for the selected class ability.
Verified game tickets, authoritative identity and reconnect ownership are
documented in [ADR 0059](docs/architecture/0059-verified-game-admission.md).
