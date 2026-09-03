# Keycloak accounts for matchmaking (milestone 56)

Milestone 57 extends this setup to game admission. Add the optional `gloom.play`
scope and separate ticket signing keys as described in [GAME_AUTH.md](GAME_AUTH.md).
Discovery and gameplay remain distinct capabilities; access/refresh credentials
are used only with the HTTPS services.

This integration replaces manually provisioned client identity tokens with
browser sign-in and in-memory account-token renewal. The setup below covers
discovery; milestone 57 also uses the account session for separate game tickets.
The dedicated game server's publisher token remains an independent credential.
The implementation uses Keycloak's [device, token and introspection endpoints](https://www.keycloak.org/securing-apps/oidc-layers).

## Configure the realm

Use a managed or self-hosted Keycloak realm with an HTTPS issuer such as
`https://accounts.example.org/realms/gloom`. Its certificate must be trusted by
Windows on both client and match-host machines. Use the issuer exactly as
Keycloak reports it, without a trailing slash. The account verification page
must use the same origin. Do not disable TLS verification.

Create these clients in that realm using the
[Keycloak administration console](https://www.keycloak.org/docs/latest/server_admin/index.html):

1. `gloom-desktop`: public OIDC client (client authentication off). Enable OAuth
   2.0 Device Authorization Grant. Disable password/direct grants, implicit flow
   and standard authorization-code flow for this device-only integration.
2. `gloom-match`: confidential OIDC client with client-secret authentication.
   Generate its secret in Keycloak and inject it only into `gloom_match_service`.
   Interactive grants and service-account token issuance are not needed to
   introspect tokens.
3. Add optional OIDC client scope `gloom.discovery` to `gloom-desktop`, with
   Include in token scope enabled. Add an Audience mapper for included client
   audience `gloom-match`, enabled in both access tokens and introspection.
   This ensures the confidential introspector is also in the access audience.
   Do not rely on the default `account` audience or disable audience checks.

Keep ordinary online refresh tokens enabled, with a finite session lifetime.
Rotating refresh tokens are supported; Gloom serializes renewal. Do not request
offline_access: unbounded/offline lifetimes are not supported. Configure device
challenge lifetime at no more than 900 seconds and interval between 1 and 60
seconds. Gloom accepts access lifetimes up to 24 hours and refresh lifetimes up
to 30 days, but those are validation ceilings rather than suggested policies.
Manage users, MFA and enrollment through your normal Keycloak workflow.

The verifier requires introspection fields `active`, `iss`, `sub`, `aud`,
`client_id`, `token_type`, `scope` and `exp`; optional `nbf` is validated. The
audience may be a string or string array. The expected token type is `Bearer`.
Keep access and refresh token values within 4096 ASCII characters, for example
by limiting unnecessary role/profile claims. Gloom does not send id_tokens as
discovery credentials.

## Process configuration

Client and match host both require:

```powershell
$env:GLOOM_MATCH_IDENTITY_PROVIDER = "keycloak"
$env:GLOOM_KEYCLOAK_ISSUER = "https://accounts.example.org/realms/gloom"
$env:GLOOM_KEYCLOAK_CLIENT_ID = "gloom-desktop"
```

The match host additionally needs these public settings and its secret from
your protected process supervisor/secret manager:

```powershell
$env:GLOOM_KEYCLOAK_INTROSPECTION_CLIENT_ID = "gloom-match"
$env:GLOOM_KEYCLOAK_AUDIENCE = "gloom-match"
# Inject GLOOM_KEYCLOAK_INTROSPECTION_CLIENT_SECRET securely, host only.
# Inject GLOOM_MATCH_PUBLISHER_TOKEN securely, host and dedicated publisher only.
```

Never launch a discovery client from an environment containing either host
secret. The public client does not read the introspection secret or publisher
token. In Keycloak mode neither process consults the registry or static
`GLOOM_MATCH_IDENTITY_TOKEN`; configuration errors do not fall back to them.

Start the HTTPS match host normally as described in [MATCH_SERVICE.md](MATCH_SERVICE.md).
In the discovery client set `GLOOM_MATCH_SERVICE_URL`, then run any of:

```powershell
.\gloom.exe --vertical-slice-list
.\gloom.exe --vertical-slice-browse Nyx
.\gloom.exe --vertical-slice-join match:local-factory-duel Nyx
```

The console prints a verification URL and short user code. Open that URL in
your browser, enter the code and approve the sign-in for this Gloom instance.
Gloom waits at the provider's interval, then proceeds automatically. Ctrl+C
cancels waiting. Account passwords and MFA are entered only in the browser.
Graphical discovery creates its window after this preflight, so no window is
frozen waiting for account approval. Direct-IP commands never request sign-in.

Access and refresh tokens remain in process memory. Renewal occurs when the
reader grant needs another identity exchange. Denied/expired sign-in or failed
renewal produces a generic error; restart the client to sign in again. No hidden
password fallback, disk credential cache or automatic refresh replay occurs.
Closing Gloom clears its local session but does not log out other browser sessions.

## Revocation and migration

The trusted principal id is SHA-256, lowercase hexadecimal, of the issuer's
ASCII bytes, one LF byte, then Keycloak's immutable `sub`, without a final
newline. This id does not depend on username, email or token contents.
Use it with publisher-authorized `DELETE /reader-grants/{principal}` to suspend
discovery immediately and persistently. The protocol and `.revocations` sidecar
are unchanged from milestone 55.

Disabling/logging out an account in Keycloak prevents subsequent successful
introspection; a previously issued reader grant can remain valid for up to 60
seconds. Use local principal suspension when that delay is unacceptable.
Provider outage fails new exchanges with 503; existing grants follow their
normal lifetime. No identity verification results are cached between exchanges.

For migration, map each registry principal's suspension to the corresponding
Keycloak issuer/subject id before allowing that account to discover matches.
Back up revocations, stop the host and change the provider configuration, then
restart. Old grants are invalidated by restart. Keep issuer and subject stable;
recreating accounts or changing issuer creates different principals.

`GLOOM_MATCH_IDENTITY_PROVIDER=registry` explicitly selects the old registry
path with `GLOOM_MATCH_IDENTITIES_FILE` on the host and
`GLOOM_MATCH_IDENTITY_TOKEN` on the client. Unset provider retains this mode for
compatibility. Unknown provider names fail startup. There is no automatic mode
switch on provider failure.

## Build and validation

The normal CMake build includes the provider. simdjson is an explicit dependency
at the repository's vcpkg baseline. Match-service packages include its runtime
DLL/license along with OpenSSL.

```powershell
ctest --preset windows-debug -R "gloom.(keycloak_identity|match_https)"
```

The tests use deterministic clocks and a local HTTPS provider fixture, including
the real Gloom HTTPS adapter. They cover the documented protocol and rejection
paths, and check that WinHTTP rejects the fixture's untrusted certificate.
They do not create a realm, modify system trust or claim a live Keycloak
deployment. Before external use, validate the configured realm's sign-in,
audience/scope mapping, renewal and suspension with your actual trusted TLS.
No external service has been deployed as part of this milestone.
