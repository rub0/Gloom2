# ADR 0013: Server-authoritative lag-compensated combat

## Status

Accepted for protocol v5.

## Context

Hitscan combat has to feel responsive under latency without trusting a client to
choose the target it hit. The authoritative server also needs bounded memory and
CPU costs, deterministic rejection rules, and a contract that is independent of
GameNetworkingSockets, Jolt and the renderer.

Valve's Source implementation keeps a bounded history of player states, derives
a target simulation time from the command, temporarily evaluates other players
at that time, and restores the current world afterwards. Its default maximum
rewind window is one second. The general technique and its fairness trade-offs
are described in Valve's latency-compensation documentation.

## Decision

Protocol v5 adds backend-neutral fire-command and fire-result event payloads.
A fire command contains a monotonically increasing per-shooter sequence, shooter
entity, estimated server tick, normalized aim direction and maximum range. It
does not contain a target entity or a claimed hit.

The authoritative movement server exposes a sorted full-world snapshot after a
simulation tick. `LagCompensatedCombatServer` records those snapshots in a
fixed-capacity history. On a command it:

1. Rejects malformed, duplicate, future and expired requests.
2. Selects the newest recorded frame at or before the requested server tick.
3. Derives the ray origin from the shooter's authoritative historical state.
4. Tests the ray against every other historical character sphere and selects
   the nearest intersection within the validated range.
5. Rejects that hit when a configured static world AABB intersects the ray
   first.
6. Returns the requested, evaluated and current server ticks with the result.

The default history is 60 ticks, the future tolerance is two ticks, and all
limits are explicit settings. Sequence deduplication is scoped per shooter.
Metrics distinguish hits, misses, duplicates, expired commands, future commands
and invalid commands.

The combat API uses only Gloom protocol and movement types. It does not expose
transport handles, physics bodies or renderer objects. This preserves the
subsystem boundaries needed to replace any backend later.

## Transport policy

Fire events are latency-sensitive and should not be placed behind old reliable
movement data. The current loopback test proves that protocol v5 fire events can
travel over the GameNetworkingSockets unreliable lane. A production session
layer will add bounded resends and idempotent result caching around the existing
sequence number so isolated packet loss does not silently discard a shot.

## Consequences and limits

The server, rather than the client, decides whether and what was hit. Rewind is
bounded and observable, and static geometry can occlude a historical character.
The implementation never mutates the live physics world during validation.

This milestone deliberately uses character spheres and static movement AABBs.
It does not yet rewind animation bones, capsules or dynamic rigid bodies, apply
damage or teams, or integrate live-session clock synchronization. Those can be
added behind the same combat contract. Multi-client ownership, authentication
and loss-resilient gameplay-event delivery belong to the next session-layer
milestone.

## Validation

`gloom.combat` covers protocol round trips, malformed payload rejection,
historical hits that miss in the current frame, nearest-hit selection, static
occlusion, bounded history and every temporal rejection class. `gloom.network`
sends a fire command over a real loopback transport. The automated network scene
records the authoritative world each tick and validates scripted shots while
rendering through Vulkan.

## References

- [Valve Source SDK player lag compensation](https://github.com/ValveSoftware/source-sdk-2013/blob/master/src/game/server/player_lagcompensation.cpp)
- [Valve: Source Multiplayer Networking](https://developer.valvesoftware.com/wiki/Source_Multiplayer_Networking)
- [Valve: Latency Compensating Methods](https://developer.valvesoftware.com/wiki/Latency_Compensating_Methods_in_Client/Server_In-game_Protocol_Design_and_Optimization)
