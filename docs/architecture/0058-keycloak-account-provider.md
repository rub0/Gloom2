# ADR 0058: Keycloak device sign-in and introspected matchmaking identity

## Status

Accepted. Extends ADR 0057 with a concrete external account provider. Registry
mode remains available; selecting Keycloak never falls back to registry tokens.
ADR 0059 extends this provider to separate gameplay-ticket issuance; the
discovery-only admission limitation below describes this ADR's original scope.

## Decision

Select Keycloak through `GLOOM_MATCH_IDENTITY_PROVIDER=keycloak` in the discovery
client and HTTPS host. Use an explicitly configured HTTPS realm issuer and its
documented device authorization, token and introspection endpoints. Endpoint
discovery and arbitrary URLs obtained from bearer claims are deliberately absent.
The provider contract follows [Keycloak's OIDC endpoints](https://www.keycloak.org/securing-apps/oidc-layers).

The desktop client is public: it supplies a client id and requests `openid
gloom.discovery`, with no client secret. `KeycloakSignIn` obtains a device
challenge and exposes only the verification URI and user code. The URI must be
HTTPS on the configured issuer's origin; the code has a bounded safe alphabet.
The console shows both before a graphical window is created, and the user signs
in through their browser. Account passwords and MFA never enter Gloom. Ctrl+C
cancels the executable's preflight; library cancellation clears pending and
cached credentials. A new process requires a new sign-in.

Polling follows [RFC 8628](https://www.rfc-editor.org/info/rfc8628/): wait the
advertised interval (five seconds by default), retain pending state, increase
the interval by five seconds on slow_down, and back off on transport failure.
Denial, invalid/expired responses or the monotonic device deadline terminate the
attempt. Challenges are capped at 15 minutes. There is no automatic restart loop.

Successful responses require bounded bearer access/refresh tokens and finite
`expires_in` and `refresh_expires_in`. Deadlines start before each HTTP request,
and a response that arrives after its usable lifetime is rejected. Access tokens
renew on demand with up to five seconds of margin. The session serializes refresh
so concurrent discovery cannot replay a rotating refresh token. Renewal replaces
both tokens; failure clears the session and requires another explicit sign-in.
Nothing is written to credential files, assets or diagnostics.

The existing WinHTTP reader connector now accepts an identity-acquisition
callback. Whenever its separate reader-grant cache needs renewal, it obtains the
current access token from the account session, then performs the existing
`/reader-grants/exchange`. Publication remains on its static operator credential.
Direct-IP and offline graphical tests never initialize the account provider.

The HTTPS host uses a separate confidential Keycloak client, whose secret exists
only in the host environment. It introspects the access token over authenticated
HTTPS, following [RFC 7662](https://www.rfc-editor.org/info/rfc7662/). Introspection
must return active=true, the exact issuer, expected audience, allowed public
client_id, bearer token type, the exact gloom.discovery scope, a nonempty stable
subject, a future integral exp, and no future nbf. simdjson validates bounded JSON;
duplicate top-level fields and malformed types fail closed. Gloom does not parse
unverified JWT claims or implement signing-key selection itself.

Principal ids are lowercase SHA-256 of `issuer + LF + sub` (no final newline).
They are 64 safe characters, independent of token rotation and username/email
changes, and namespace accounts from different issuers. Existing per-principal
quotas and durable suspensions apply unchanged. Introspection is performed on
every exchange; invalid identity returns 401, while provider transport/status
failures return 503 without exposing the response body. Existing grants remain
valid until their at-most-60-second expiry unless locally revoked.

Identity headers are bounded to 4096 printable non-space ASCII bytes to admit
ordinary provider access tokens; publisher and registry credentials retain their
512-byte limit. Introspection/device/token response bodies are bounded to 64 KiB.
WinHTTP enforces system certificate trust, finite connect/read/write timeouts
and disabled redirects. URLs with embedded credentials, query or fragments are
rejected. Provider credentials use URL-encoded POST bodies, never query strings.
The host's global exchange budget still applies before introspection.

The small `gloom_match_identity` library is shared by client and host, with the
existing baseline-pinned simdjson dependency now declared explicitly. The host
package includes its DLL and license, in addition to OpenSSL. No renderer or
game simulation is linked into the host.

## Consequences and limits

Operators must configure Keycloak, its public device client, confidential
introspection client, audience mapper, scope and trusted TLS. `KEYCLOAK.md`
contains the concrete settings and migration procedure. No external realm or
account is created by building/testing this milestone.

Registry and Keycloak principal ids are different namespaces. Existing registry
suspensions must be deliberately mapped to the corresponding issuer/subject
ids when migrating; changing issuer or subject changes identity. The local
suspension sidecar must accompany backups as before. Provider account logout
affects future exchanges, not already issued grants; use local revocation for
immediate discovery suspension.

Sign-in currently uses the console and the user's browser, with credentials
kept only in process memory. A graphical account screen, explicit remote logout,
secure persistent sessions and recovery without restarting the client remain
separate work. Game-session admission still uses ADR 0047 and is not authorized
by these discovery credentials. The service retains bounded single-host quotas,
trusted publishers and the protected-ingress limitations from ADRs 0056-0057.

## Validation

`gloom.keycloak_identity` covers pending/slow_down/denial, monotonic deadlines,
transport backoff, malformed/duplicate claims, issuer/audience/client/scope/type
rejection, credential-size limits, concurrent refresh rotation and failure,
cancellation, stable principal quotas and suspension. A local TLS provider fixture
implements the Keycloak endpoint contract and exercises device exchange, refresh,
host introspection and actual HTTPS reader grants/revocation together.

Positive TLS requests trust the generated certificate only in the test's injected
OpenSSL transport. The production WinHTTP transport must reject that certificate.
No test disables certificate validation, changes Windows trust or contacts an
external account. These are protocol-contract tests, not a live Keycloak realm
acceptance test; deployment should also validate the configured realm's audience
and introspection mappings. The original registry HTTPS and full engine suite
remain regression gates.
