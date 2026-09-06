# ADR 0065: Original graphical interface

Status: accepted. Milestone 64.

## Context

The existing discovery, identity, lobby and game-admission services were exposed
through command-line entry points. The original presentation is stored in 17
Flash movies, which cannot become a runtime dependency of the modern client.

## Decision

Recover embedded bitmaps and rasterize the original fonts offline. Retain a
source-hashed, reproducible inventory and one RGBA atlas. Draw UI quads through
Diligent after tone mapping and temporal reconstruction. A device-local vertex
buffer receives only the active prefix, avoiding large allocations in the shared
dynamic constant-buffer heap. Atlas descriptors bind once per texture lifetime.

Use one 1280×720 safe-area transform for rendering and mouse hit testing. Keep
keyboard focus and activation edges across frames. Menus release mouse capture;
local pause stops simulation, while online menus submit neutral input.

Compose the existing asynchronous match browser, authentication, directory and
ticket/session APIs. Keep identity context across menu/game window lifetimes.
The existing development host still accepts two remote players. Preserve all
CLI tools and protocol 14; this decision introduces no gameplay or backend rules.

## Consequences and validation

No Flash/Hikari runtime is required. The atlas occupies 32 MiB before GPU overhead;
geometry is bounded to 65,536 vertices per frame. Unsupported glyphs fall back to
`?`; the recovered font atlas covers Latin-1. Window reconstruction on entering
and leaving a match remains visible, while access context survives in memory.

Unit checks cover input edges, focus, text editing and DPI transforms. Forty-eight
Vulkan captures cover sixteen states at three resolutions. Two graphical clients
exercise discovery, selection, ready, play, reconnect with the same entity, and
confirmed leave against a real dedicated server. Gallery authentication is a
fixture; public account deployment remains governed by GAME_AUTH.md.

See [UI.md](../UI.md) and the [acceptance report](../../reports/ui-2026-09-04/README.md).
