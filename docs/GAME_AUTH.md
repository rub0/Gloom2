# Verified game admission (milestone 57)

Configure the HTTPS service and account provider first using
[MATCH_SERVICE.md](MATCH_SERVICE.md) and [KEYCLOAK.md](KEYCLOAK.md).
Development direct-IP hosting remains available when `GLOOM_GAME_AUTH` is unset.
To require verified accounts, enable ticket mode on both the dedicated server
and joining client. The graphical `--vertical-slice-host` mode is for development;
ticket mode uses `gloom_slice_server.exe`.

## Keys and accounts

Generate a separate Ed25519 signing key using an OpenSSL installation:

```powershell
openssl genpkey -algorithm ED25519 -out game-ticket-private.pem
openssl pkey -in game-ticket-private.pem -pubout -out game-ticket-public.pem
```

Protect the private PEM with filesystem permissions for the HTTPS service
account. Keep it outside the repository and package. Distribute only the public
PEM to game servers. This key is separate from the HTTPS TLS certificate/key.
Encrypted PEMs requiring interactive passwords are rejected. Restart both
services when rotating the signing key; the new random server instance also
invalidates tickets issued for the old instance.

In Keycloak, add a client scope named `gloom.play` with **Include in token scope**
enabled and assign it to `gloom-desktop` as an optional scope, alongside
`gloom.discovery`. Ticket-mode desktop sign-in requests
`openid gloom.discovery gloom.play`. Retain the audience mapper and confidential
introspection setup from KEYCLOAK.md. Use distinct accounts for the two players;
the same account cannot occupy both slots. Display names come from verified
identity and are currently pseudonyms such as `P_0123456789abcdef`.

## Process configuration

In the HTTPS host environment, in addition to its existing Keycloak, publisher
and TLS configuration:

```powershell
$env:GLOOM_GAME_TICKET_PRIVATE_KEY = 'D:\GloomSecrets\game-ticket-private.pem'
# Start gloom_match_service with its usual TLS and durable-store arguments.
```

In the dedicated server environment:

```powershell
$env:GLOOM_GAME_AUTH = 'tickets'
$env:GLOOM_GAME_TICKET_PUBLIC_KEY = 'D:\GloomConfig\game-ticket-public.pem'
$env:GLOOM_MATCH_SERVICE_URL = 'https://matches.example.test'
# Supply GLOOM_MATCH_PUBLISHER_TOKEN only through the server's protected environment.
.\build\windows-vs\Debug\gloom_slice_server.exe 0.0.0.0:27020
```

The current dedicated executable publishes `local-factory-duel`. Configure a
routable listen endpoint for remote clients; `0.0.0.0` is a local binding example,
not a public address to advertise to another machine.

In each desktop client's environment (no publisher/signing/introspection secret):

```powershell
$env:GLOOM_GAME_AUTH = 'tickets'
$env:GLOOM_MATCH_SERVICE_URL = 'https://matches.example.test'
$env:GLOOM_MATCH_IDENTITY_PROVIDER = 'keycloak'
$env:GLOOM_KEYCLOAK_ISSUER = 'https://accounts.example.test/realms/gloom'
$env:GLOOM_KEYCLOAK_CLIENT_ID = 'gloom-desktop'
.\build\windows-vs\Debug\gloom.exe --vertical-slice-browse
# Or: gloom.exe --vertical-slice-join match:local-factory-duel
```

Registry mode can exercise the same tickets with an operator-provisioned identity
file and individual token. It does not perform Keycloak scope checks. Selecting
Keycloak never falls back to registry mode. Missing keys, unknown auth modes,
failed identity verification and ticket failures never fall back to self-asserted
game identity. When ticket mode is disabled, direct-IP development credentials
continue to work without the account service.

## Service contract and failures

`POST /game-tickets/<match-id>` takes one `Authorization: Bearer <account-token>`
header and an empty body, with no query parameters. Success is HTTP 200 with
`endpoint<TAB>instance<TAB>ticket`, no trailing newline, and `Cache-Control:
no-store`. Only the ticket is sent in the reliable GNS client hello. Reader grants,
publisher credentials and account access/refresh tokens are never game tickets.

| Status | Meaning |
| --- | --- |
| 400 | Nonempty body or query parameters |
| 401 | Invalid account/scope, suspended principal, or wrong credential role |
| 404 | Missing, expired, completed or protocol-incompatible match |
| 429 | Shared principal or aggregate exchange budget exhausted |
| 503 | Tickets disabled, provider unavailable, or principal table full |

Tickets last at most 30 seconds and never beyond account/lease expiry. Each is
bound to one server instance and consumed once. Reconnection obtains a fresh
ticket and proves the same account plus possession of the current resume token.
Issuance can target a full/active match for this purpose; it does not reserve a
slot or override game admission. Keep UTC clocks synchronized. No credential
or complete ticket should be copied into support logs.
The desktop client stops an unacknowledged ticket admission after ten seconds
and displays an error. Select again in the browser or restart a direct
`match:<id>` join to retry; no development credential is substituted.

Local principal suspension blocks future ticket issuance immediately and
survives service restart. Already issued tickets retain their bounded lifetime;
active game sessions are not automatically kicked. Key rotation and server
restart interrupt admission/reconnection and should be coordinated.

## Acceptance

Run `ctest --preset windows-debug`. Ticket tests exercise offline verification
and real GNS, while the Keycloak test uses an isolated trusted TLS fixture for
the provider and service contract. Production WinHTTP still rejects that fixture
certificate. Tests do not modify the Windows trust store or provision a realm.

For your deployed environment, check two distinct accounts joining, movement,
disconnect/reconnect retaining the same slot, duplicate-account rejection,
scope denial and suspension before new admission. Verify publicly reachable
advertised endpoints and trusted TLS before testing from another machine.
These operator acceptance checks are separate from the completed repository
milestones; public ingress, GNS peer authentication, relay/NAT traversal and
multi-host availability are not provided by this milestone.
