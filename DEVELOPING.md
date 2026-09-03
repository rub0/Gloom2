# Developing Gloom

## Installed Windows toolchain

- Visual Studio 2022 Community: `D:\Microsoft Visual Studio\2022\Community`
- CMake 4.4.2: `D:\Dev\CMake\bin\cmake.exe`
- Ninja 1.13.2: `D:\Dev\Ninja\1.13.2\ninja.exe`
- Vulkan SDK: `D:\Dev\VulkanSDK\1.4.357.0`
- RenderDoc: `D:\Dev\RenderDoc`
- vcpkg: `D:\Dev\vcpkg`

The CMake preset selects the Visual Studio installation on `D:` explicitly.

## Configure, build and test

Milestone 63 adds `gloom.animation_vfx`, `gloom.animation_network` and
`gloom.animation_dedicated`. Scene payload **5** supports validated TRS clips,
bind rigs and eight influences per vertex. Protocol **14** carries cosmetic
pitch and confirmed-shot events while retaining prior character IDs. See
[ANIMATION_VFX.md](docs/ANIMATION_VFX.md) and ADR 0064 for skinning, FPS arms,
particle budgets, temporal reviews and local/host/dedicated acceptance.
Milestone 62's `gloom.character_restoration` and `gloom.character_visual_review`
retain the bind-pose regression path. [CHARACTERS.md](docs/CHARACTERS.md) and
ADR 0063 document conversion, materials and Shadow's explicit rig rebase.

Milestones 59–61 add `gloom.factory_restoration`, `gloom.material_render` and
`gloom.factory_visual_review`, plus original-map coverage in the existing
network and real GNS transport tests. See [FACTORY_RESTORATION.md](docs/FACTORY_RESTORATION.md)
for reproducible offline import, material channels, Factory captures and limits.
Milestones 59–61 introduced cooked scene version 3 (now superseded by 5).
Protocol 12 introduced a collision/spawn/lava fingerprint, retained in 14, rejecting an
incompatible scene before prediction. The usual build cooks the checked-in
converted assets; Ogre/Assimp/Python are only needed to regenerate them.

Milestone 58 adds `gloom.visual_review`, an actual Vulkan image regression gate.
It captures nine fixed Factory/weapon/ability views after asset residency and
compares them with inspected references. Full-resolution output and diagnostics
remain under `build/windows-vs/visual-review/Debug`. See
[VISUAL_REVIEW.md](docs/VISUAL_REVIEW.md) before updating any reference; a
successful smoke run alone is not visual acceptance. Asset tests also check
outward winding and flat normals, and render-scene tests cover camera-relative
props and nonuniform normal transforms.

Milestone 57 closes the currently scheduled network work. See
[GAME_AUTH.md](docs/GAME_AUTH.md) and ADR 0059 for verified game admission.
`GLOOM_GAME_AUTH=tickets` uses a separate Ed25519 capability, bound to a random
server instance and valid once for at most 30 seconds. Keycloak game issuance
requires `gloom.play`; discovery grants retain their existing permissions.
Only the HTTPS service loads the private signing key. The dedicated server
loads its public key, verifies locally and injects cryptographic resume tokens.
Client issuance/renewal runs on a background future before GNS connection.
Account credentials never enter gameplay packets. Session identity binding and
lobby preflight must reject mismatched resumes/duplicates before any mutation.
`gloom.game_tickets` covers cryptographic rejection and real GNS reconnection;
`gloom.keycloak_identity` also covers actual HTTPS ticket issuance and suspension.

```powershell
& "D:\Dev\CMake\bin\cmake.exe" --preset windows
& "D:\Dev\CMake\bin\cmake.exe" --build --preset windows-debug
& "D:\Dev\CMake\bin\ctest.exe" --preset windows-debug
& ".\build\windows-vs\Debug\gloom.exe"
```

Release builds use the equivalent `windows-release` preset.

Milestone 53 adds a standalone `gloom_match_service` target and the
`gloom_match_service_package` distribution target. They link the small match
storage/HTTPS libraries rather than SDL, Vulkan or game simulation. The build
fetches cpp-httplib at `0d3a3d805c8979e3220b42fcbc0ff95980eabc06` and explicitly
uses vcpkg OpenSSL. See [MATCH_SERVICE.md](docs/MATCH_SERVICE.md) for deployment.

The Windows `gloom.match_https` test spawns the actual host twice, verifies real
TLS and HTTP semantics and checks durable restart recovery. It creates a
short-lived local certificate; it neither modifies Windows trust nor uses a
live service. `gloom.match_discovery` also checks corrupt stores, exclusive
writer locks, strict wire validation and rollback after a failed disk write.
Never replace these checks with a disabled TLS-verification mode.

Milestone 54 split discovery and publishing credentials. Milestone 55 replaces
the shared reader token with individual identity exchange. The HTTPS host needs
`GLOOM_MATCH_IDENTITIES_FILE` and `GLOOM_MATCH_PUBLISHER_TOKEN`; `gloom` reads
`GLOOM_MATCH_IDENTITY_TOKEN`, while `gloom_slice_server` still reads only the
publisher token. Neither old shared-token variable is read. Never provision a
publisher credential to a client process. `/health` supports both roles for connector startup;
`/ready` and mutations require publisher authority. Reader requests, publisher
requests and failed authentication use independent, fixed-memory token buckets.
HTTP 429 includes `Retry-After: 1`; provider errors remain visible to callers,
without automatic mutation replay. See ADR 0056 for budgets and limitations.
The HTTPS regression also checks missing/short credentials, read-only
access, duplicate authorization rejection, role-isolated throttling and refill.

`POST /reader-grants/exchange` verifies an opaque individual identity credential
against the host's bounded digest registry and issues a cryptographically random
reader grant for at most 60 seconds (also capped by identity expiry). The host
never uses a client-supplied principal. The registry reloads for each exchange;
invalid/unavailable input fails closed. See ADR 0057 and MATCH_SERVICE.md for the
versioned registry, expiry, quota and revocation contracts. Keep identity secrets
out of source control, assets, command-line arguments and diagnostics.

`make_winhttp_match_reader_connector` exchanges and renews credentials using a
serialized `MatchReaderCredential` cache. It uses a monotonic deadline measured
before exchange, with up to one second of renewal margin; failed renewal cannot
reuse the expired grant. Renewal happens on the next read, without background
traffic or silent replay after 401/429. The publisher connector remains static.
The per-principal read bucket (burst 20, refill 5/s) is shared by all active grants;
exchange has its own principal budget and active-grant cap. Principal state is
bounded and never evicted to make room for new identities, preventing quota reset.

Publisher-only `DELETE /reader-grants/{principal}` clears all that principal's
grants and persists a suspension in `STORE.revocations`. It blocks reissuance,
including after restart. The existing match-store lifetime lock protects the
sidecar too. A failed write returns 503, fails readiness and keeps the principal
blocked in memory; retry is idempotent. Back up both stores together.
Tests cover clock boundaries, independent/concurrent quotas, renewal, bounded
state, failed persistence, real TLS exchange/expiry/revocation and restart.

Milestone 56 adds `GLOOM_MATCH_IDENTITY_PROVIDER=keycloak` as an explicit
alternative to registry mode. `KeycloakSignIn` handles device authorization,
pending/slow_down, monotonic expiry and serialized rotating refresh tokens.
Sign-in runs as a console preflight before opening the graphical browser.
The reader connector obtains the current identity through a callback whenever
its independent reader-grant cache needs exchange. No account credentials are
written to disk or passed into game sessions.

`make_keycloak_identity_verifier` introspects tokens using a host-only
confidential client. It validates active/issuer/audience/client/type/scope/
subject/expiry and derives the stable principal from issuer and subject. The
identity header bound increases to 4096 bytes; publisher/registry limits remain
512. The small `gloom_match_identity` library uses WinHTTP, OpenSSL Crypto and
the baseline-pinned simdjson parser, independently of rendering/game simulation.
Packages now include simdjson.dll and its license. See [KEYCLOAK.md](docs/KEYCLOAK.md)
and ADR 0058 for concrete configuration, namespace migration and limitations.

`gloom.keycloak_identity` checks deterministic OAuth timing, malformed claims,
provider failure, concurrent refresh, cancellation and stable quota/revocation
identity. A local TLS provider fixture exercises device exchange/renewal,
introspection and the Gloom HTTPS grant host together. Positive TLS requests use
an injected test transport trusting only its generated certificate; production
WinHTTP must reject that certificate. These tests do not contact a live realm or
alter Windows trust. The original registry path remains independently tested.

The first configure installs SDL3, Jolt Physics, GameNetworkingSockets, KTX and
stb with
vcpkg and fetches the
pinned Diligent Core revision under the build tree on `D:`. It is based on 2.5.6 and includes
the upstream Vulkan swapchain synchronization fix from commit `a255d24d`.
For a quick automated Vulkan rendering check, run:

```powershell
& ".\build\windows-vs\Debug\gloom.exe" --smoke-test
```

The full test preset also runs `gloom.vulkan_sync`. This test disables VSync,
renders 600 fully lit frames including clustered lights and four shadow
cascades, and minimizes and restores the window to exercise irregular swapchain
image acquisition. It fails if Vulkan validation reports a Diligent error or
`VUID-vkQueueSubmit-pSignalSemaphores-00067`.

`gloom.physics` exercises the Jolt backend independently of rendering. It checks
fixed-step accumulation, a multithreaded falling-body collision, transform and
velocity access, and clean body/world shutdown. It also creates logical static,
dynamic, kinematic and trigger components in a headless Jolt world. A dynamic
entity crosses a real Jolt sensor and must produce ordered enter, stay and exit
events without collision response. Destroying the logical trigger entity must
destroy its sensor body. Character coverage also exercises grounded virtual-
capsule movement and jumping. `CharacterPhysicsComponent` owns the backend
character handle and exposes configurable stair height, floor adhesion, maximum
slope, mass and push strength; the world boundary additionally exposes ground
normal/velocity, explicit impulses and queued character contact events.

`gloom.render_scene` checks transform interpolation, including clamping and the
shortest quaternion path. The graphical smoke tests render the complete
Jolt-driven cube scene through Vulkan rather than a hard-coded triangle.

`gloom.render_graph` validates pass ordering, resource-state transitions,
read-before-write and cycle failures, lifetimes and transient aliasing.
`gloom.gpu_assets` validates stable conversion from cooked-scene structures to
owned mesh/material upload packets. The graphical smoke test also requires the
built-in mesh, material and white texture to reach resident state on frame one.
`gloom.lighting` validates bounded logarithmic cluster construction and its
telemetry. The graphical smoke test additionally requires local-light workload
metrics and, where supported, asynchronous GPU timings for shadow, opaque or
tone-map work.

Diligent dynamic-buffer allocations are frame-local. All three clustered-light
buffers must be mapped with `MAP_FLAG_DISCARD` before their first bound use in
every frame, including frames whose point-light, cluster or index arrays are
empty. Skipping an empty upload leaves the SRB pointing at an expired Vulkan
dynamic allocation and triggers `DvpVerifyDynamicAllocation`.

`gloom.temporal` validates backend-neutral TAA/FSR/DLSS capability negotiation,
vendor hook selection, fallback, scaled render extents and the deterministic
Halton jitter sequence. Automated graphical modes render HDR, motion and depth
at 75 percent of the output extent, resolve native TAA into ping-pong
output-resolution history and require temporal telemetry. The 600-frame Vulkan
test also recreates and invalidates those resources across minimize/restore.
The production temporal path additionally maintains paired color/depth history,
rejects disocclusions, reacts to luminance changes and applies configurable
contrast-limited sharpening. Automated graphical modes enable the GPU-timed
dynamic-resolution controller; smoke validation requires an observed scale
change and a valid rebuilt history.

Renderer transforms use row-vector order (`world * view * projection`). All
HLSL shaders are therefore compiled with Diligent's row-major matrix packing;
removing that flag transposes constant-buffer matrices and produces geometry
that fans toward `w = 0`. When a graphics smoke test passes but presentation is
suspect, capture both the pre-temporal HDR target and final backbuffer in
RenderDoc rather than relying only on process exit and validation-layer status.

`gloom.jobs` checks parallel ranges, nested waits, exception propagation and
scheduler counters. The application prints completed/caller-assisted jobs,
peak queue depth, aggregate execution time and aggregate wait time at shutdown.

`gloom.entities` checks generational entity identity, typed reusable component
storage, component removal, destruction cleanup, slot reuse and rejection of
stale handles. The registry is backend-neutral and single-owner; Jolt-backed
physics components use this identity without exposing Jolt types.

`gloom.network` starts two independent GameNetworkingSockets transports on
loopback, connects through a dynamically selected UDP port, exchanges reliable
and unreliable messages in both directions, checks counters and verifies clean
disconnect notification. The current vcpkg feature set intentionally excludes
the optional WebRTC/ICE dependency; this milestone covers direct IP transport.

`gloom.network_protocol` checks the versioned binary envelope, malformed input
rejection, sequence wraparound, snapshot interpolation and prediction replay.
It also exercises the deterministic network laboratory with latency, jitter,
loss, duplication, reordering, bandwidth and queue limits. The laboratory uses
simulated time and a fixed PRNG, so it neither sleeps nor depends on machine
timing.

`gloom.network_replication` runs the authoritative movement slice for ten
simulated seconds under latency, jitter, loss, duplication, reordering,
bandwidth and queue constraints. It verifies clock estimation, typed wire
schemas, stale-snapshot rejection, local prediction convergence and remote
snapshot interpolation. `gloom.network` additionally sends those typed schemas
through a real GameNetworkingSockets loopback connection.

`gloom.network_scene_smoke` composes the platform input contract, deterministic
adverse links, prediction/reconciliation, snapshot interpolation, the Jolt
`CharacterVirtual` adapter and Vulkan rendering for 180 fixed-duration frames.
For an interactive run, use:

```powershell
& ".\build\windows-vs\Debug\gloom.exe" --network-demo
```

WASD and the arrow keys move the blue predicted character and Space jumps. The
orange remote character is interpolated from authoritative snapshots. The
scene contains static Jolt obstacles with matching deterministic collision
descriptions used by client and server. At shutdown the demo reports prediction
correction, presentation-smoothing and redundant-input telemetry. Input
batching introduced in protocol v3 remains in the current protocol and includes
up to four unacknowledged commands; the
server deduplicates them and preserves one-shot jump actions across isolated
packet loss.

The milestone-20 gameplay slice runs with:

```powershell
& ".\build\windows-vs\Debug\gloom.exe" --vertical-slice
```

The mouse controls an FPS camera, WASD/arrow keys move in camera space, Space
jumps and left mouse fires. `gloom.vertical_slice` tests the backend-neutral
camera and combat loop plus session ownership; `gloom.vertical_slice_smoke`
drives 360 deterministic SDL/Jolt/Vulkan frames and requires an observed kill
and four-second respawn. Shutdown telemetry reports active sessions, authorized
input/fire commands, rejected commands and prediction reconciliations.

Local mode composes two admitted clients in-process, while host/join mode carries
the player session over GNS. Both paths use the same authoritative session,
prediction and lag-compensated combat contracts without coupling gameplay to a
socket backend.

Snapshots use protocol v4 fixed-point fields and delta compression against the
newest baseline explicitly acknowledged by the client. The server falls back to
a full snapshot when necessary, applies the configured relevance radius/entity
budget, and reports full/delta counts, records and payload bytes at shutdown.

Protocol v5 added timestamped fire-command and fire-result
events without changing the v4 movement and snapshot encodings. The client never
claims a target: the server derives the ray origin from bounded authoritative
history, rewinds character states, intersects configurable vertical character
capsules, selects the nearest hit and checks static world occlusion.
`gloom.combat` tests the wire format, rejection policy, history limits, upper
character silhouette and historical hit validation. `gloom.network` additionally carries a
fire event through a real GameNetworkingSockets loopback connection, and the
network scene smoke test records and queries live authoritative snapshots.

The protocol-v6 session foundation adds the multiclient session foundation.
`gloom.session` verifies
credential-policy admission, connection-to-entity ownership, reconnect token
rotation and expiry, four-timestamp clock exchange, per-client input/snapshot
state and bounded idempotent event retries. `gloom.network` carries the handshake,
clock exchange and authorized gameplay messages through a real
GameNetworkingSockets loopback connection. The credential validator and reconnect
token generator are mandatory injected policies; production integrations must
provide a trusted identity service and a cryptographically secure token source.

Protocol v7 adds a typed gameplay-snapshot message and exposes the slice over a
real direct-IP GameNetworkingSockets connection:

```powershell
& ".\build\windows-vs\Debug\gloom.exe" --vertical-slice-host 0.0.0.0:27020
& ".\build\windows-vs\Debug\gloom.exe" --vertical-slice-join 127.0.0.1:27020
```

Start the host first. Only the joining process captures the mouse and supplies
FPS input. The host window is an authoritative spectator view. The transport
adapter sends protocol envelopes only; session admission, entity ownership,
input/fire authorization, prediction and gameplay snapshot encoding are tested
without depending on GNS, then tested again through two real loopback sockets.
The development credential is intentionally fixed and is not suitable for an
Internet-facing production server.

Remote input is queued by sequence and acknowledged only after the corresponding
host simulation step. Gameplay snapshots include velocity and grounded state so
jump reconciliation does not reset motion. While dead, the client continues
sending neutral input for acknowledgement but suppresses local movement
prediction and fire until the authoritative respawn arrives.

Milestone 23 adds a headless two-player composition:

```powershell
& ".\build\windows-vs\Debug\gloom_slice_server.exe" "0.0.0.0:27020"
& ".\build\windows-vs\Debug\gloom.exe" --vertical-slice-join "127.0.0.1:27020" "Nyx"
& ".\build\windows-vs\Debug\gloom.exe" --vertical-slice-join "127.0.0.1:27020" "Reaper"
```

The server has no SDL or Vulkan runtime composition. It does run the same
headless Jolt Factory/character composition as local and listen-host authority,
and exits with Ctrl+C.
Join mode retries a lost connection after one second and presents the resume
token while the server's ten-second grace period remains open. For automated
startup/shutdown checks, append `--ticks 3` to the server command.

`gloom.vertical_slice_network` also runs protocol-v7 input and snapshots through
a fixed adverse link (45 ms one-way latency, 18 ms jitter, 5% loss, duplication
and reordering) and enforces a 0.75 m maximum-correction ceiling. The observed
deterministic baseline is about 0.085 m. `gloom.vertical_slice_transport` covers
two simultaneous clients over real loopback sockets.

Protocol v8 places both remote clients in a revisioned lobby. The optional final
join argument is a development display name (1-20 ASCII letters, digits, `_` or
`-`); omitting it generates a temporary name and account ID. Clients auto-ready
after admission and clock synchronization, but the server will reject all
movement and fire until both slots are connected and ready. A disconnect keeps
the slot dormant for ten seconds; expiry completes an active match and records
the other entity as winner by abandonment. `gloom.match_lobby` covers the lobby
codec and state machine independently of GNS.

The bundled credential remains self-asserted development identity.
`SliceRemoteHostSettings::validate_identity` is the boundary for a future trusted
identity service; do not expose the fixed development secret to the Internet.

Protocol v9 keeps that lobby contract and adds the first migrated content slice.
Every player is currently Hound. Press `Q` or the right mouse button to activate
Bite; the client predicts its half-second forward charge, while only the server
may start the ability, validate contact, apply 40 damage, heal 40 life and begin
the 25-second cooldown. Active Bite feedback is limited to the claws, reticle and
target tint so it cannot be confused with world geometry. Left mouse still fires
Soul Reaper, so weapon and character ability exercise separate authoritative paths.
Confirmed hits briefly draw four marks around the reticle. Authoritative opponent
life transitions drive a short damage flash, a collapsed corpse during the
four-second respawn delay and a blue respawn tint through the renderer-neutral
`SlicePresentationFeedback` model; presentation never applies damage itself.

The displayed arena is a data-only Factory blockout with industrial lanes,
service blocks, overhead pipework and a lava channel cut into the floor. It uses
builtin geometry: the legacy Ogre meshes, PhysX collision and map parser are
reference material, not runtime dependencies. Boxes and one-sided surfaces are
separate presentation records, and static presentation no longer creates Jolt
bodies. `ArenaBox::solid` still generates deterministic movement and hitscan
obstacles for vertical walls and blocks.

Local, host and dedicated authority separately compose Factory logical entities
in Jolt. The lava entity owns a sensor `TriggerComponent` and a reusable
`DamageVolumeComponent`; queued Jolt contact events are the only source of lava
damage. Each Hound now owns a `CharacterVirtual` capsule through its
`CharacterPhysicsComponent`; its inner Jolt body carries the same logical entity
identity to sensors, so no separate kinematic proxy or manual `ArenaBox` overlap
behavior is permitted.

Each playable combatant is one logical entity created by
`compose_slice_character`.
Gameplay state belongs to focused transform, character physics, health, shield,
movement, weapon, ability, authority, replication, score and presentation
components; the private combatant handle owns no duplicate state.

`AuthorityComponent::mode` distinguishes server authority, a predicted owner
and an interpolated remote. `ReplicationComponent` stores simulation/receipt
ticks, input acknowledgement and grounded state. Capture outgoing movement with
`capture_component_snapshot`; apply received state with
`apply_component_snapshot` so server components cannot be overwritten, predicted
owners reconcile only when requested and stale remote frames are ignored. The
remote client composes the same Hound entities after admission, applies the
authoritative frame through this policy and then overlays local prediction.

Protocol v10 adds the first replicated Factory mechanism. The cargo lift owns
`TransformComponent`, `KinematicMotionComponent`, server authority, replication
state and a `KinematicBodyComponent`. Motion is a tick-addressed ping-pong path
with endpoint dwell, so authority never integrates wall-clock drift. Advance it
with `advance_kinematic_motion`; this updates component transform/velocity and
uses `World::move_kinematic_body` so Jolt derives physical velocity and can carry
or push contacts. Do not animate a gameplay body with repeated teleports.
`SliceSnapshot::factory_lift` is the protocol/presentation view used identically
by local, host and joining clients.

When entity collision is enabled, feed living remote movement states to
`PredictedMovementClient::set_collision_entities`; otherwise the camera predicts
through a combatant that the server pushes away and repeated reconciliation is
visible. Treat authoritative respawn teleports as discontinuities and call
`reset_local_state`, which clears stale pending inputs before prediction resumes.
Dead Hounds must not remain in Jolt or lag-compensated combat hitbox sets:
`CharacterPhysicsComponent` is detached while the respawn timer is active and
reattached at the spawn before the first live tick. Component combat snapshots
likewise contain living entities only. Preserve this lifecycle when adding new
death states or character controllers.

Milestone 33 moves the Factory cargo lift from builtin-only presentation to
project-authored content. `assets/factory/cargo_lift.gltf` is an authoring input;
the `gloom_factory_content` build target invokes `gloom_asset_cooker` and writes
`content/factory/cargo_lift.gasset` under the active binary directory. Build
`gloom` normally; its dependency guarantees the cooked payload is current.

At runtime the vertical slice mounts the source and binary content roots,
discovers the cooked catalog and requests the scene at critical priority. The
lift keeps drawing the builtin cube until the residency coordinator exposes a
resident generation, then only its mesh presentation is replaced. Its position
still comes from `SliceSnapshot::factory_lift`; its logical entity,
`KinematicBodyComponent`, Jolt shape and protocol state remain authoritative.
Do not derive collision, motion bounds or replication from authored mesh data.
Missing or invalid content must remain a presentation failure with the logical
blockout as fallback.

Milestone 34 adds `assets/factory/surface_modules.gltf` to the same
`gloom_factory_content` target. It contains two independently identified cooked
meshes: a floor panel and an industrial module. The vertical-slice adapter
records the visual indices selected by `ArenaMaterial`, requests this scene at
high priority and swaps those mesh IDs only after a resident generation is
available. The graphical slice smoke requires that generation, preventing a
missing or broken module package from passing unnoticed.

The authored module is normalized and presentation scaling maps it onto each
existing `ArenaBox`; do not feed that transform or its cooked bounds back into
Factory composition. Static Jolt bodies continue to use the original box data.
Trim records deliberately retain their builtin fallback for the next authored
pass, and `ArenaSurface::lava` remains the one-sided builtin quad over its
unchanged entity-owned Jolt sensor and `DamageVolumeComponent`.

Milestone 35 extends `surface_modules.gltf` rather than introducing another
catalog and residency request. Scene instance order is the runtime contract:
floor, industrial, trim, then lava. The first three are normalized box modules;
their authored Y extent is one quarter, so presentation applies the established
four-times Y compensation exactly once per resident scene lifetime. The lava
module is a separate four-vertex XZ plane with upward-facing triangle winding
and is scaled directly from `ArenaSurface` half extents.

The four-instance check happens before any replacement and the graphical smoke
requires a resident surface generation. A malformed partial module library must
therefore retain all builtin fallbacks and fail automated validation. Changing
the visible lava mesh must never change `slice_surfaces`, the physical channel,
trigger bounds, damage cadence or environmental-kill rules.

Protocol v11 adds `SlicePlayerSelection` in `slice_selection.hpp`. Character,
weapon and ability are separate bounded enums even though the initial valid
tuple was Hound / Soul Reaper / Bite. `valid_slice_selection` is the shared
authority and codec allowlist; never accept unknown enum values for forward
compatibility, because an older server cannot compose their gameplay safely.

Lobby commands now carry a discriminator: ready is `[0, bool]` and selection is
`[1, character, weapon, ability]`. Each replicated `SliceLobbyPlayer` carries
the accepted selection. Clients send selection after clock synchronization and
send ready only after observing their tuple in authoritative lobby state.
Selection changes are waiting-phase only and clear an already-ready flag before
the lobby can start. Reconnect during an active match retains the slot's
selection and ready state; it must not rebuild or mutate the combatant.

Network tests must count simulated match ticks only while
`SliceMatchPhase::active`. Session admission is intentionally earlier than
selection and ready, so using `ClientSession::active()` as a match-clock proxy
can terminate a regression before enough gameplay snapshots exist.

Milestone 37 makes the accepted tuple part of character composition through
`CharacterLoadoutComponent`. `CombatantView` now carries character, weapon and
ability, so protocol-v11 gameplay snapshots confirm the active loadout as well
as the pre-match roster. `VerticalSliceSimulation::set_selection` is the only
adapter used by the remote host to apply a lobby choice to its existing logical
entity before match start. It must not recreate the Jolt character or disturb
the death/respawn attachment lifecycle.

Hound currently accepts Soul Reaper with Bite, Guard or no ability. The empty
slot is exposed by the optional join argument
`hound-reaper`; `hound-bite` remains the default. Ability `none` clears any
ability state, rejects activation, reports inactive in snapshots and uses zero
HUD readiness. It deliberately introduces no replacement mechanic. Both
loadouts retain the same movement, capsule, life and Soul Reaper balance.

When extending the roster, update the shared allowlist, authoritative component
policy, snapshot codec validation, client replicated composition and lobby/GNS
tests together. Never accept a tuple merely because its individual enum values
exist; only explicitly composable combinations belong in
`valid_slice_selection`.

Soul Reaper is the LMB weapon in both current loadouts; its authoritative
cooldown is 30 ticks (0.5 seconds). Bite belongs only to `hound-bite` and its
legacy cooldown is 1,500 ticks (25 seconds). `hound-reaper` means “Hound with
Soul Reaper and an empty ability slot”, not a separate Soul Reaper character.

Do not maintain a long independent client ability deadline. The client gates Q/
RMB from the latest authoritative HUD readiness and keeps only a one-snapshot
pending-command latch to prevent duplicate sends in an input burst. This lets a
server respawn reset become usable immediately and also releases the latch after
a rejected command. First-person slots 12 and 13 show the Soul Reaper body and
barrel/muzzle flash; during active Bite they temporarily become the two claws.

Milestone 38 replaces `HoundAbilityComponent` and `activate_bite` with the
selection-driven `AbilityComponent` and `activate_ability`. Cooldown duration is
chosen from the accepted ability tuple. Bite alone overrides movement and runs
lag-compensated contact/damage/life steal; a generic active flag must not imply
the Bite charge policy.

`hound-guard` adds `SliceAbility::guard`. Q/RMB grants 50 shield up to the
existing 150 maximum, exposes a 60-tick blue presentation cue and starts a
900-tick (15-second) cooldown. It uses the same authorized reliable ability
command, pending-snapshot latch, lobby selection and gameplay snapshot fields.
Guard never changes the Jolt character, deals damage or awards a kill.

When adding another ability, provide its explicit cooldown denominator, server
effect and presentation meaning. Keep selection validation tuple-based and do
not branch transport or session ownership by ability type.

Milestone 39 adds `SliceCharacter::berserker` without fabricating legacy
balance. No Berserker source rules or numerical data exist in this repository,
so its only accepted development tuple is `berserker-reaper`: the already
defined Soul Reaper plus `SliceAbility::none`. It intentionally reuses the
current life, movement, capsule, respawn and weapon policies as foundation
defaults, not as claims about original Berserker balance.

`compose_slice_character` and `has_complete_character_composition` are now the
roster-neutral component seam used by authority and replicated clients. Lobby
and gameplay snapshots must preserve the Berserker enum in both local and
remote views. Hound-only Bite and Guard tuples must be rejected for Berserker.
The dark armor palette is provisional presentation scaffolding; it carries no
collision or gameplay meaning.

Milestone 40 makes selection interactive when `--vertical-slice-join` has no
loadout argument. Left/Right cycles the explicit four-entry allowlist and Enter
calls `VerticalSliceRemoteClient::confirm_selection`. Until then the client may
complete admission and clock synchronization but emits neither a lobby
selection nor ready. After confirmation it sends the selected tuple, waits for
the authoritative lobby echo, and only then sends ready.

`set_desired_selection` is waiting-phase and pre-confirmation only. Keep this
policy separate from movement keys at the gameplay boundary: the arrow-key menu
fields are interpreted only while the selector is open. The SDL window title is
the current lightweight UI surface and displays the pending choice, then every
authoritative lobby player's tuple and ready state. An explicit CLI loadout
retains automatic confirmation for tests and unattended launches.

Milestone 41 moves opponent geometry into `character_presentation_recipe`.
Each `SliceCharacter` owns a stable authored-scene URI and a renderer-neutral
nine-part builtin fallback. Hound remains a forward quadruped; Berserker is a
distinct upright humanoid with torso, head, shoulders, arms and legs. The
renderer applies facing, death collapse, respawn/damage feedback and Hound
ability tint after selecting the recipe from the replicated snapshot identity.

Presentation recipes must not provide collision dimensions, health, movement
or combat policy. Their offsets and scales are fallback visuals only. When
authored character content is connected, replace the mesh selected by the
recipe's URI after residency while retaining the same authoritative transform,
Jolt capsule and lag-compensated hit volume.

Milestone 42 adds `assets/characters/hound.gltf` and
`assets/characters/berserker.gltf`. The normal content target cooks them to
`content/characters/*.gasset`. Each source scene contains exactly nine ordered
part instances matching its recipe; runtime requires that count before swapping
any mesh, so a partial or malformed package leaves the whole builtin fallback.

Both character scenes are discovered through the shared source/cache VFS but
own independent catalogs, loaders, residency coordinators and generations.
This lets either character fail or reload without hiding the other. The Vulkan
vertical-slice smoke requires both generations even though its combat script
presents only Hound. The authored primitive has a quarter-height normalized
source profile, so presentation compensates Y scale once after residency; this
is visual normalization only and must never feed component or collision state.

Milestones 43 and 44 complete the currently identified cooked-content
blockouts. `assets/weapons/soul_reaper.gltf` provides ordered body/barrel
instances; `assets/abilities/hound_abilities.gltf` provides Bite-claw and
Guard-plate instances. The `gloom_project_content` target cooks both alongside
Factory and roster scenes.

Soul Reaper keeps its builtin body/barrel until both cooked instances are
resident. Muzzle scale/color still comes from authoritative weapon readiness.
Bite displays the cooked claw in both hands during its active charge. Guard
temporarily displays paired blue plates during its authoritative one-second
active cue. The ability scene swaps only when both entries are resident, and
neither asset can activate an ability, change a cooldown or affect damage.

The vertical-slice smoke requires weapon and ability generations in addition
to both character and Factory generations. Reticle, hit marker and health bars
remain procedural UI and are intentionally not asset-cooking milestones.

Milestone 45 introduces `SliceIdentityProvider`. The host verifies each opaque
hello credential exactly once and uses only the returned
`SlicePlayerIdentity` for lobby admission, duplicate-account checks and
reconnect matching. Never decode a client-asserted display name alongside a
provider result or use client configuration as authority.

`make_development_identity_provider` preserves the current
`secret:account:name` workflow for local host/join. External integrations inject
a shared provider through `SliceRemoteHostSettings::identity_provider` and may
send opaque tokens with `begin_with_credential` / `reconnect_with_credential`.
The generic session manager is reached only after provider verification and
continues to own entity allocation, resume-token rotation and command ownership.
Provider implementations should bound parsing, return stable non-zero account
IDs and valid display names, and must not treat resume tokens as identity proof.

Milestone 46 adds `SliceMatchDirectory`. Publish
`SliceMatchAdvertisement` records from a composition root and derive their
occupancy and phase with `make_slice_match_advertisement`; never let directory
metadata become gameplay or lobby authority. Every update must increase the
record revision. Listing and resolution intentionally expose only waiting,
non-full matches whose protocol exactly matches the client.

`make_in_memory_match_directory` is deterministic and thread-safe, but it is a
development/test implementation scoped to one process. It does not remove the
need to start `gloom_slice_server` and it does not provide Internet discovery.
A future service adapter implements the same interface and supplies leases,
regional policy and persistence. Once resolved, pass `SliceJoinResolution::endpoint`
unchanged to `network::Transport::connect`; verified identity, session
ownership and lobby admission then proceed through their existing boundaries.

Milestone 47 wraps publication in `SliceMatchPublication`. Give every running
server a fresh opaque `instance_id`; do not reuse it after a process restart.
Call `start` only after the transport returns its bound endpoint, call `update`
from the server loop, and call `stop` before transport shutdown. Updates are
emitted immediately when the authoritative lobby revision changes and otherwise
at the configured heartbeat. Keep `refresh_interval_ms` strictly below
`lease_duration_ms`.

Directory implementations must enforce live instance ownership and must remove
expired records before list/resolve. An expired logical match id may be claimed
by a new instance; the previous instance must no longer be able to update or
withdraw it. `SliceMatchPublication` performs best-effort RAII withdrawal, but
correctness after a crash comes from lease expiry, not destructors.

The dedicated server currently composes the in-memory backend, so publication
is lifecycle validation rather than cross-process matchmaking. It still needs
to be started before clients connect. A service-backed implementation is the
next composition step and does not change the host, GNS or identity boundaries.

Milestone 48 defines `SliceMatchService` for an external persistent provider.
Its `store` operation must atomically enforce lease ownership and revision
ordering; `erase` must compare the instance id. Provider errors are returned,
not converted into empty listings. Adapt a shared service with
`make_service_match_directory`; do not put provider SDK types, HTTP concerns or
credentials into the gameplay contract.

Client browsers use `SliceMatchSelection::refresh`, choose a bounded index and
call `resolve` immediately before transport connection. Do not connect directly
from cached listing metadata. `resolve_slice_join_target` unifies this as
`match:<id>` while retaining validated `host:port` input. A match target without
a configured directory is an explicit error. The graphical composition still
configures only direct IP until an authenticated provider and interactive
browser are selected; therefore the dedicated server remains required.

Milestone 49 adds `SliceMatchServiceConnector` and
`connect_match_directory`. Provider integrations own TLS and bearer-token
authentication and return an already authenticated service session. Never copy
the token into `SliceMatchAdvertisement`, logs, gameplay credentials or protocol
messages. Empty URLs/tokens and null sessions are rejected.

Connection setup uses bounded exponential retry configured through
`SliceMatchServiceConnectionSettings`. Inject the wait callback from the
composition root; tests should record delays rather than sleep. This layer does
not retry service mutations: a concrete provider must classify failures and
attach idempotency keys before replaying an advertisement. No concrete TLS
provider is selected yet, so normal client/server commands remain direct IP.

Milestone 50 supplies the Windows WinHTTP connector and executable wiring. Set
`GLOOM_MATCH_SERVICE_URL` to an HTTPS REST base and
the role-specific credential (`GLOOM_MATCH_IDENTITY_TOKEN` for clients,
`GLOOM_MATCH_PUBLISHER_TOKEN` for publishers, as of milestone 55); never commit it.
The REST contract is recorded in ADR 0052. Server and client use Unix
milliseconds for leases. `--vertical-slice-list` is a non-graphical diagnostic;
`--vertical-slice-join match:<id>` re-resolves before opening GNS. Leaving both
variables unset preserves direct-IP development and all offline tests.

Milestone 51 adds a provider-side `make_match_service_core`. Persist and compare
the `mutation_id` with every record: an identical replay is success, while the
same id with different bytes is an error. The WinHTTP line contract therefore
contains eleven fields. This makes provider-classified mutation retries safe
after an ambiguous response.

Use `acquire_bearer_token(refresh)` when credentials rotate. Attempt zero asks
for the cached/current token; retries pass `true`. Acquisition errors consume
the same bounded retry budget and delay. `AsyncSliceMatchBrowser` owns at most
one background listing and must be polled on the presentation thread before
reading its published matches or error. Always re-resolve the chosen match
before connecting.

Milestone 52 wires that model through `--vertical-slice-browse`. Keep provider
work off the render thread and retain explicit loading, empty and error states.
Left/Right only changes cached presentation selection; Enter must call
`resolve` again before `Transport::connect`. Do not enable relative mouse mode
until matchmaking and, when applicable, roster confirmation are complete.
Provider-free direct-IP smokes must never read matchmaking environment values.

`gloom.assets` creates a real glTF fixture, imports indexed geometry and PBR
material data with fastgltf, cooks its external texture dependency, validates
corruption handling and loads the result through the job system. To cook project
content manually:

```powershell
& ".\build\windows-vs\Debug\gloom_asset_cooker.exe" `
  "D:\ProjectAssets" "D:\ProjectCache" `
  "game:/models/scene.gltf" "cache:/models/scene.gasset"
```

The source and cache roots must already exist. Runtime code uses only virtual
paths and `.gasset` data; fastgltf remains an offline import dependency.
Decoded scenes are converted to backend-neutral upload packets away from the
render thread; Diligent drains those packets and owns every GPU object.

External PNG/JPEG/BMP authoring images are now decoded offline, mipmapped and
Basis-compressed into KTX2 payloads. Color uses ETC1S+sRGB; normal/data textures
use linear UASTC. Runtime KTX validation/transcoding produces the owned mip
packets consumed by the renderer. The graphical smoke test additionally forces
upload deferral, fence-safe release and LRU eviction with small test budgets.
