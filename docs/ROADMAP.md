# Gloom roadmap

## Completed

1. C++23 project foundation, SDL3 window and subsystem lifecycle.
2. Diligent Vulkan backend, validation and swapchain synchronization regression.
3. Jolt 5.6 backend, fixed-step simulation, multithreading and rigid-body tests.
4. Scene-to-render boundary, camera, mesh/material data, immutable snapshots and
   an interpolated visual physics demo.
5. Engine job system with task groups, cooperative waits, exception handling,
   scheduler metrics and parallel render-snapshot preparation.
6. GameNetworkingSockets direct-IP transport backend, independent connection and
   packet API, metrics, dynamic-port listening and real loopback integration tests.
7. Network-simulation foundations: legacy audit, versioned wire envelope,
   sequence handling, snapshot jitter buffer, prediction/reconciliation history
   and deterministic adverse-network laboratory.
8. Headless authoritative movement slice: clock estimator, validated input and
   world-state schemas, fixed-tick server, predicted/reconciled local movement,
   interpolated remote movement and real/adverse-link integration tests.
9. Interactive network laboratory: backend-neutral SDL input, a Jolt
   CharacterVirtual adapter, visible predicted-local/interpolated-remote
   entities and a deterministic Vulkan end-to-end smoke test.
10. Character gameplay and prediction policy: replicated jump/vertical state,
    shared deterministic ground and static-obstacle collision, matching Jolt
    geometry, visual correction smoothing and quantitative reconciliation
    telemetry.
11A. Loss-resilient input transport: bounded redundant command batches,
     wrap-aware server deduplication, one-shot action recovery, protocol v3 and
     client/server bandwidth telemetry.
11B. Snapshot bandwidth and connection scale: protocol v4 fixed-point
     quantization, acknowledged full/delta baselines, entity create/remove
     records, relevance radius, controlled/nearest priority, entity budgets and
     codec telemetry.
11C. Server-authoritative combat validation: protocol v5 timestamped fire
     commands, bounded authoritative history, temporal and duplicate rejection,
     lag-compensated nearest hitscan, static-world occlusion and combat metrics.
11D. Multiclient session layer: protocol v6 admission policy, durable session and
     entity ownership, bounded reconnect with token rotation, live clock exchange,
     isolated per-client replication state and idempotent expiring gameplay events.
12. Asset pipeline foundation: normalized virtual paths, stable typed IDs,
    dependency catalog, versioned/checksummed cooked assets, fastgltf scene and
    PBR material import, external texture dependencies, offline cooker,
    asynchronous loading and typed error placeholders.
13. GPU asset integration and render graph: backend-neutral scene-to-upload
    conversion, renderer-owned mesh/material/texture registries, thread-safe
    upload queue and residency, textured metallic-roughness PBR baseline,
    explicit pass dependencies, state transitions and transient lifetimes.
14. Production texture and residency path: offline image decoding, semantic
    mip generation, KTX2 Basis ETC1S/UASTC payloads, base-color,
    metallic-roughness and normal maps, bounded per-frame uploads, GPU fences,
    deferred destruction, residency metrics and LRU eviction.
15. Asset residency coordinator and imported-scene viewer: dependency-aware
    loading, explicit request priority and cancellation, multithreaded CPU
    preparation, node hierarchy instantiation, generation-based hot reload,
    shared GPU resource lifetimes and device-selected BC7/BC5 transcoding with
    RGBA8 fallback.
16. Production mesh and visibility path: offline MikkTSpace tangents, generated
    normal/UV fallbacks, meshoptimizer cache/overdraw/fetch optimization, bounds,
    two simplified LODs, multithreaded frustum culling, deterministic material/
    mesh batches, visibility telemetry and an imported-scene stress mode.
17. Modern lighting frame: RGBA16F HDR rendering, bounded logarithmic clustered
    point lights, a cubemap environment path, four practical-split directional
    shadow cascades, exposure and ACES tone mapping, asynchronous GPU duration
    queries and render-graph pass/resource declarations.

18. Temporal render-feature foundation: motion vectors, jittered camera and
    history resources, TAA, render/output resolution separation, capability
    negotiation and interchangeable hooks for later FSR/DLSS integrations.
19. Production temporal reconstruction: depth-history disocclusion rejection,
    reactive luminance weighting, contrast-adaptive sharpening, backend-neutral
    dynamic resolution with hysteresis and temporal performance telemetry.
20. First Gloom gameplay vertical slice: audited legacy life, shield, Soul
    Reaper and respawn rules; a deterministic player-versus-combatant arena;
    backend-neutral fixed-tick gameplay; SDL controls, Jolt level geometry,
    snapshot-derived HUD feedback and a Vulkan kill/respawn smoke test.
21. First-person authoritative slice composition: relative-mouse FPS camera,
    camera-space movement and explicit 3D aim; two admitted entity-owning
    sessions; session-authorized input and fire; predicted presentation,
    lag-compensated server hitscan and multiplayer telemetry.
22. Remote vertical-slice host/join: protocol-v7 gameplay snapshots, direct-IP
    GameNetworkingSockets orchestration, remote session admission and clock
    exchange, redundant predicted input, authoritative fire ownership and real
    loopback host/join coverage.
23. Two-player dedicated vertical slice: per-connection input, fire,
    acknowledgement and snapshot state; reciprocal owned-entity snapshots; two
    simultaneous human clients; grace-period entity resume and automatic join
    retry; a headless 60 Hz server; deterministic adverse-link and real
    two-client GNS regressions.
24. Development identity and authoritative match lobby: protocol-v8 revisioned
    rosters, bounded account/display identity with an injected validation seam,
    ready-gated two-player match start, gameplay-phase authorization, preserved
    reconnect slots and explicit abandonment victory after grace expiry.
25. First migrated Gloom content: protocol-v9 Hound identity and facing state;
    server-authoritative Bite charge, 40 damage, 40 life steal, half-second
    active window and 25-second cooldown; matching remote prediction and
    sequencing; a recognizable data-only Factory blockout; and primitive Hound,
    FPS claws, ability and cooldown presentation without coupling gameplay to a
    renderer, transport or legacy runtime.
26. Logical entity/component foundation: generational entity handles, type-safe
    reusable component storage, destruction cleanup, stale-handle rejection and
    backend-neutral ownership suitable for later physics and gameplay adapters.
27. Jolt physics components and events: reusable static, dynamic, kinematic and
    trigger components; sensor bodies; queued enter/stay/exit contact events;
    entity/body lookup; and Jolt-backed headless authority.
28. Factory entity migration: split floor geometry with a real channel, a
    one-sided lava surface, a sensor entity and reusable damage-volume gameplay
    component. Remove visual-only Jolt bodies and `ArenaBox` behavior coupling.
29. Character entity composition: migrate Hound into transform, provisional
    character-physics, health, shield, movement, weapon, ability, authority and
    presentation components without changing current gameplay behavior.
30. Legacy Gloom character physics migration: Jolt CharacterVirtual-backed
    Hound components with configurable stair/floor adhesion, slope rejection,
    ground/contact state, external impulses, dynamic-body pushing and sensor
    participation through an owned capsule proxy.
31. Component authority and replication: explicit server, predicted-owner and
    interpolated-remote policies; component-derived movement snapshots; guarded
    snapshot application and reconciliation metadata; client-side replicated
    Hound compositions; and identical owned/injected Jolt Factory composition
    across local, listen-host and dedicated authority paths.
32. Kinematic gameplay foundation: reusable tick-driven ping-pong motion,
    velocity-derived Jolt kinematic movement, an authoritative Factory cargo
    lift, protocol-v10 mechanism state and consistent local/two-client visual
    presentation.
33. Authored Factory content foundation: a source-controlled glTF cargo-lift
    presentation cooked as part of the build, loaded through the asynchronous
    residency path and substituted only after GPU residency, with the existing
    logical/Jolt mechanism and protocol-v10 state retained as authority and a
    builtin blockout fallback retained for loading or failure.
34. Authored Factory surface pass: source-controlled floor-panel and industrial
    module meshes cooked with the Factory content target, streamed at runtime
    and applied by arena material only after residency, while logical box
    dimensions, entity-owned Jolt collision and the lava surface/trigger remain
    unchanged.
35. Authored Factory trim and hazard presentation: extend the modular cooked
    scene with oxidized trim and a dedicated upward-facing lava mesh; select
    both by immutable arena material only after residency while preserving the
    open floor channel, one-sided surface record, Jolt sensor and reusable
    damage-volume authority.
36. Character and loadout selection foundation: protocol-v11 typed
    Hound/Soul Reaper/Bite selection commands, authoritative waiting-lobby
    validation, replicated per-player selection, selection-before-ready client
    sequencing and unchanged ready-gated/reconnect gameplay with the current
    loadout retained as the valid default.
37. Selection-driven character composition: entity-owned
    `CharacterLoadoutComponent`, selection-derived weapon/ability snapshot
    fields and authoritative ability gating; add the validated Hound / Soul
    Reaper / no-ability loadout, expose it as `hound-reaper`, preserve identical
    character physics and weapon balance, and replicate it across reciprocal
    local/remote compositions. Add visible Soul Reaper presentation and derive
    client ability reuse from authoritative readiness so respawn resets cannot
    leave a stale 1,500-tick local lockout.
38. Generalized authoritative ability slot: replace the Bite-specific component
    and activation entry point with selected `AbilityComponent` policy; add the
    Hound / Soul Reaper / Guard loadout, granting 50 authoritative shield with
    a one-second cue and 15-second cooldown; preserve Bite charge/damage/life
    steal, the no-ability loadout and shared command/reconciliation plumbing.
39. Berserker character foundation: audited the available repository evidence
    and added the bounded Berserker / Soul Reaper / no-ability tuple, generic
    character composition, reciprocal snapshot validation and provisional
    distinct presentation without inventing balance or ability rules.
40. Interactive pre-match roster selection: Left/Right client-side carousel
    over the four validated tuples, explicit Enter confirmation, no selection
    or ready before confirmation, authoritative echo before ready, and both
    lobby choices/readiness exposed in the window title. Explicit CLI loadouts
    retain automatic selection for automation.
41. Roster-neutral character presentation: immutable per-character recipes
    with separate authored-scene slots, preserved Hound quadruped fallback and
    a genuinely distinct upright Berserker fallback; shared death, damage,
    respawn and facing presentation continues from authoritative snapshots
    without changing physics or hit volumes.
42. Authored character content foundation: source-controlled nine-part Hound
    and Berserker glTF scenes cooked by the normal build, independently streamed
    to GPU and atomically substituted by roster recipe only after complete
    residency; builtin silhouettes remain loading/failure fallbacks and no
    authored bounds enter gameplay.
43. Authored Soul Reaper presentation: source-controlled two-part weapon scene
    cooked by the project content target, streamed independently and substituted
    only when body and barrel are resident; authoritative hitscan, cooldown and
    muzzle timing remain unchanged.
44. Authored Hound ability props: source-controlled Bite claw and Guard plate
    scene, atomic two-instance residency, cooked props in both first-person
    hands during their authoritative active cues, and builtin fallback without
    any ability, damage, shield or cooldown authority in presentation.
45. Trusted identity integration foundation: injectable credential-to-principal
    provider, exactly-once host verification before session admission, opaque
    client token entry points, server-trusted lobby identity and preserved
    duplicate-account, entity ownership, reconnect-slot and resume-token policy;
    retain the existing credential format only as the development provider.
46. Discovery and matchmaking foundation: backend-neutral, revision-ordered
    advertised match records; deterministic joinable listing; protocol,
    capacity and lobby-phase gates; authoritative occupancy derivation; and
    join resolution to the existing direct-IP transport endpoint without
    duplicating identity, session or lobby admission.
47. Leased remote match lifecycle: explicit server-instance ownership,
    owner-checked updates and withdrawal, expiry and safe instance takeover,
    lobby-revision/heartbeat publication, plus dedicated-server composition
    from bound endpoint through orderly shutdown.
48. Service matchmaking and client flow: injectable persistent-service
    contract with explicit failures, directory adapter and lease filtering,
    deterministic browse/select/re-resolve state, `match:<id>` resolution and
    preserved direct-IP joining through one transport target.
49. Authenticated matchmaking connection foundation: provider-owned opaque
    bearer authentication, validated endpoint/credential configuration,
    bounded exponential connection retry with injectable waits, explicit
    exhaustion/null-session failures and adaptation into the existing browser.
50. Concrete HTTPS matchmaking composition: WinHTTP TLS/bearer service client,
    documented REST record contract, environment-based secret configuration,
    dedicated publication, console match listing and `match:<id>` graphical
    joining while preserving service-free direct IP.
51. Matchmaking hardening foundation: provider-side atomic service core,
    instance/revision mutation idempotency, credential refresh within bounded
    backoff and non-blocking match-list refresh suitable for presentation.
52. In-window matchmaking presentation: asynchronous loading/error/empty and
    selection states, Left/Right navigation, Enter refresh/confirmation,
    mandatory final re-resolution, deferred mouse capture and handoff to the
    existing roster lobby and reconnect path.
53. Deployable matchmaking host: standalone C++ HTTPS executable with pinned
    TLS dependency, shared validated record codec, bounded authenticated REST,
    exclusive-writer durable storage, health/readiness, graceful shutdown,
    runtime packaging and real child-process TLS/restart integration tests.

54. Matchmaking access separation: distinct reader/publisher authorization,
    role-specific process credentials without legacy-token fallback, read-only
    discovery, publisher-only mutations/readiness, independent bounded request
    budgets and real HTTPS authorization/throttling regressions.

55. Per-principal matchmaking credentials: trusted individual identity exchange
    through a bounded reloadable digest registry; cryptographically random reader
    grants lasting at most 60 seconds; automatic WinHTTP renewal; durable
    publisher-authorized principal suspension; shared quotas across each
    principal's grants; and deterministic/real HTTPS expiry, concurrency,
    throttling and restart regressions. Full Debug suite: 27/27 passed.

56. External account-provider composition: Keycloak device sign-in, bounded
    polling and serialized access/refresh credential renewal; confidential
    host introspection with issuer/audience/client/scope validation; stable
    issuer/subject principals preserving reader quotas and durable suspension;
    explicit registry compatibility; deployment/package documentation and
    deterministic plus local HTTPS provider-contract coverage. Full Debug
    suite: 28/28 passed.

57. Verified account admission: connect provider-backed player identity to
    authoritative game-session admission through signed, single-use,
    instance-bound tickets; preserve stable account/session ownership on
    reconnect; reject cross-account resumes before mutation; keep discovery
    grants separate from gameplay credentials; and validate with local HTTPS
    provider-contract and real GNS join/reconnect tests. Full Debug suite:
    29/29 passed. See ADR 0059.

58. Visual correctness and reproducible acceptance: camera-oriented and properly
    framed first-person props with separate depth; outward project mesh winding;
    flat missing-normal generation and inverse-transpose normal transforms;
    filtered/stabilized directional shadows with offscreen casters; and nine
    manually inspected Vulkan image references with automated comparison and
    negative controls. Full Debug suite: 30/30 passed. See ADR 0060 and
    VISUAL_REVIEW.md.

59. Reproducible original-asset import: source manifests, Ogre version
    normalization, exact resource resolution, static geometry/material bindings
    and explicit rig/missing-map diagnostics. Factory, Soul Reaper and v1.41
    armour are recovered; 80 output files reproduce byte-for-byte and 111
    source hashes verify against the read-only legacy checkout.
60. Extended materials: specular and anisotropic shading, textured emission/AO,
    UV0/UV1 controls, detail/lightmap inputs, alpha modes, original glow/lava
    translation and a baked HDR environment probe. Import, version-3 cooked
    scenes, residency and Vulkan consume the same contracts; GPU acceptance
    covers visible effects, zero anisotropy and transparent submission order.
61. Faithful Factory: original mesh, normalized client/server placements,
    transformed RepX collision, shared 0.15 units, nine spawns, lava and
    consistent Jolt movement/hitscan/prediction. Local, real GNS host/join and
    dedicated composition use the restored map. Six inspected Vulkan captures
    complement the video comparison and retained nine blockout references.
    Unimplemented object mechanics remain explicitly inventoried. See
    [FACTORY_RESTORATION.md](FACTORY_RESTORATION.md) and ADR 0062.
    Full Debug suite after milestones 59–61: **33/33 passed**.

62. Original characters and weapons: resolve legacy roster identity, recover
    Archangel/Hound, Shadow and Soul Reaper with metallic armor and concept-led
    cyan/red emission. Preserve 43/17-bone bind rigs and all 4/5 influences in
    version-4 scenes, validate Shadow's axis/origin repair, and integrate rigid
    first-/third-person weapon anchors. Protocol 13 adds original selections
    while retaining the former Berserker ID as a compatibility Hound slot.
    Seven inspected capture views complement real two-client GNS coverage.
    Rest-pose presentation is deliberate; animation and dedicated FPS arm
    posing follow in 63. See [CHARACTERS.md](CHARACTERS.md) and ADR 0063.
    Full Debug suite: **35/35 passed**.

63. Animation and combat effects: recover Archangel's four clips, author Shadow
    motion, evaluate/blend TRS and skin eight influences on GPU with previous
    poses, normals/tangents, shadows and animated bounds. Integrate original
    FPS arms, grip IK, equip/recoil/death and confirmed-shot effects. Fourteen
    bounded particle recipes cover character energy/smoke, combat, lava and
    heat distortion. Scene version 5 and protocol 14 preserve existing gameplay.
    Fixed-step 30/60/144 Hz Vulkan sequences, concept/video comparisons and
    local/host/dedicated two-client combat/reconnect complement unchanged
    visual references. Full Debug suite: **38/38 passed**. See
    [ANIMATION_VFX.md](ANIMATION_VFX.md), ADR 0064 and the
    [acceptance report](../reports/animation-vfx-2026-09-03/README.md).

## Next

The remaining visual restoration scope is milestone 64. See
[ART_RESTORATION.md](ART_RESTORATION.md) and ADR 0061 for its source audit
and acceptance criteria. This milestone is planned, not completed:

64. Graphical HUD, menu, browser, lobby and roster selection: original visual
    identity, scalable layout and complete existing online flow inside the UI.

Audio, broader UX/settings, new gameplay/balance, external deployment acceptance
and optimization/distribution are deferred until this scope is complete.

No scheduled networking milestone remains. Milestones 45–57 cover verified
identity, dedicated sessions/reconnect, discovery, leased publication, durable
HTTPS hosting, role separation, per-principal grants, external account-provider
composition and verified game admission. Deployment acceptance against an
operator-managed Keycloak realm and public endpoints is documented in
GAME_AUTH.md; no external deployment is claimed by the repository tests.

All currently identified Factory, roster, weapon and ability-prop blockouts now
have cooked-content paths. Later milestones will add remaining abilities when
their rules are established. Future network architecture choices such as
relay/NAT services, multi-host availability and durable game-state recovery
require a separately scoped milestone; they are not scheduled work in this list.
Optional FSR/DLSS integrations remain behind the render-feature layer.
