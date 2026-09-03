# ADR 0054: Match browsing is a pre-lobby presentation state

## Status

Accepted.

## Decision

Add `--vertical-slice-browse [name] [loadout]` as a graphical join composition.
It starts `AsyncSliceMatchBrowser` while the normal Vulkan frame continues and
uses the window title for loading, provider error, empty and selected-match
states. Left/Right changes the bounded selection; Enter refreshes an empty/error
state or re-resolves the selected match before opening GameNetworkingSockets.

Mouse capture remains disabled during browsing. After resolution, an explicit
loadout enters connection immediately; otherwise control passes to the existing
authoritative roster selector and captures the mouse only after confirmation.
Reconnect continues to use the resolved endpoint, not cached advertisement
metadata.

## Consequences

Matchmaking is usable without blocking the render/event loop or creating a
second lobby. Title-based presentation is intentionally minimal and can later
be replaced by authored UI without changing browser state. A deployable HTTPS
service host remains separate from the Windows client shipped here.

## Validation

The async model tests cover overlap and result publication. The executable
build and existing graphical/network smokes cover unchanged direct-IP and lobby
paths; provider-free CI does not contact external HTTPS services.
