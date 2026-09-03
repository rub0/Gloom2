# ADR 0027: Development identity, lobby and match lifecycle

## Status

Accepted for protocol v8.

## Context

ADR 0026 allowed two sockets to control two combatants, but admission immediately
enabled gameplay. There was no player-facing identity, no authoritative point at
which a match began, and no terminal outcome when a disconnected player failed
to return. Those omissions made connection state indistinguishable from match
state and would make later matchmaking or persistent identity difficult to add
without changing gameplay code.

## Decision

`SliceMatchLobby` is a transport-independent state machine with injected
identity validation. It owns two player slots, ready state and the ordered
`waiting`, `active` and `completed` phases. Gameplay commands are accepted only
for a connected, ready entity in an active match. The existing practice
composition retains a one-player lobby, while graphical and dedicated remote
hosts require two players.

Protocol v8 adds reliable `lobby_command` and `lobby_state` messages. A lobby
state contains its revision, phase, completion reason, winner and the stable
account ID, display name, connection and ready flags for each slot. State
updates are coalesced before transmission so a client cannot regress through
obsolete lobby revisions.

The development client puts a bounded account ID and display name in its
admission credential, then automatically sends ready after session and clock
setup. `SliceRemoteHostSettings::validate_identity` is the integration seam for
a trusted identity provider. The bundled policy only validates syntax and the
fixed development secret; it does not prove ownership of the asserted account.

Disconnecting removes the connection's gameplay state immediately but reserves
its lobby slot for the session's ten-second resume grace. A matching resumed
identity restores the same entity. If the deadline expires during an active
match, authority completes the match with reason `abandonment` and awards the
other connected entity. Waiting-room disconnects simply release their slot
after the deadline.

## Consequences and limits

- Connection, session, lobby and gameplay phases now have separate contracts.
- Two clients see the same revisioned roster and cannot move or fire early.
- Names can be supplied on the join command and match transitions are printed by
  clients and the dedicated server.
- The current client auto-readies; character/loadout selection and an interactive
  lobby screen can later drive the same ready command without changing authority.
- The development identity envelope, deterministic resume-token source and
  direct-IP endpoint are not production authentication or matchmaking.
- Rematches, server restart persistence, parties and discovery remain future
  milestones.

## Validation

`gloom.match_lobby` validates identity and protocol-v8 codecs, two-player ready
gating, matching-identity resume, duplicate-account rejection and abandonment
victory. `gloom.vertical_slice_network` and
`gloom.vertical_slice_transport` continue to cover predicted gameplay, two
players and real GNS sockets through the lobby boundary. Vulkan smoke coverage
continues to exercise the local vertical slice independently.

ADR 0047 later replaces direct development-credential decoding at host
admission with an injectable verified-identity provider while preserving this
lobby and reconnect lifecycle.
