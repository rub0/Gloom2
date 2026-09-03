# ADR 0047: Host admission resolves credentials through an identity provider

## Status

Accepted.

## Context

The remote host decoded `secret:account:name` directly. That development format
let the client assert its own account and display name and coupled lobby
admission to one credential scheme. A production identity service needs to
verify opaque tokens and return the principal that authority will bind to the
entity and reconnect slot.

## Decision

Define `SliceIdentityProvider::verify(credential)` returning either a verified
`SlicePlayerIdentity` or an error. Inject a shared provider through
`SliceRemoteHostSettings`; default to `make_development_identity_provider` so
local commands and deterministic tests remain usable.

On a client hello, decode the envelope, call the provider exactly once, and
only then admit the generic session and lobby. The lobby receives exclusively
the verified result. It retains format validation, duplicate-account rejection
and exact account/name matching on reconnect. Session resume tokens continue to
prove continuity of an admitted slot but are not identity credentials.

Add `begin_with_credential` and `reconnect_with_credential` for opaque external
tokens. Existing `begin(secret)` helpers still encode the development identity.
The host does not trust the identity configured in a remote client object.

## Consequences

- Platform, service or test verifiers can be integrated without changing
  protocol envelopes, lobby policy or entity ownership.
- One-time credentials are not consumed twice.
- A valid token cannot substitute a client-asserted account or display name.
- The development provider is explicitly non-production despite sharing the
  same interface.
- Transport encryption and provider-specific token acquisition remain external
  responsibilities.

## Validation

Lobby tests exercise the development provider. The remote network test injects
a provider that accepts one opaque token, rejects another, and returns an
identity different from the client's local claim; the authoritative lobby must
contain only the verified principal. Existing two-client, reconnect,
abandonment, GNS and gameplay tests remain unchanged.
