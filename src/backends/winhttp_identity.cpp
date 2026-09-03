#include <gloom/backends/keycloak_identity.hpp>
#include <algorithm>
#include <limits>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winhttp.h>

namespace gloom::backends {
namespace {
struct Handle {
    HINTERNET value{nullptr};
    ~Handle() { if (value) WinHttpCloseHandle(value); }
};
}
IdentityHttpPost make_winhttp_identity_post() {
    return [](std::string_view url, std::string_view form) -> std::expected<IdentityHttpResponse, std::string> {
        if (url.size() > 2048 || form.size() > 32 * 1024 ||
            !std::ranges::all_of(url, [](unsigned char c) { return c > 32 && c < 127 && c != '\\'; }))
            return std::unexpected{"Invalid identity HTTP request"};
        const std::wstring wide{url.begin(), url.end()};
        URL_COMPONENTS parts{sizeof(parts)};
        parts.dwSchemeLength = parts.dwHostNameLength = parts.dwUrlPathLength = parts.dwUserNameLength =
            parts.dwPasswordLength = parts.dwExtraInfoLength = static_cast<DWORD>(-1);
        if (!WinHttpCrackUrl(wide.c_str(), 0, 0, &parts) || parts.nScheme != INTERNET_SCHEME_HTTPS ||
            parts.dwUserNameLength || parts.dwPasswordLength || parts.dwExtraInfoLength || !parts.dwHostNameLength)
            return std::unexpected{"Identity endpoint must be HTTPS without embedded credentials or query"};
        const std::wstring host{parts.lpszHostName, parts.dwHostNameLength};
        const std::wstring path{parts.lpszUrlPath, parts.dwUrlPathLength};
        Handle session{WinHttpOpen(L"GloomIdentity/1", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                  WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0)};
        if (!session.value || !WinHttpSetTimeouts(session.value, 3000, 3000, 5000, 5000))
            return std::unexpected{"Identity HTTPS session failed"};
        Handle connection{WinHttpConnect(session.value, host.c_str(), parts.nPort, 0)};
        if (!connection.value) return std::unexpected{"Identity HTTPS connection failed"};
        Handle request{WinHttpOpenRequest(connection.value, L"POST", path.c_str(), nullptr,
            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE)};
        DWORD redirect = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
        if (!request.value || !WinHttpSetOption(request.value, WINHTTP_OPTION_REDIRECT_POLICY, &redirect, sizeof(redirect)))
            return std::unexpected{"Identity HTTPS request setup failed"};
        constexpr auto headers = L"Content-Type: application/x-www-form-urlencoded\r\nAccept: application/json";
        if (!WinHttpSendRequest(request.value, headers, static_cast<DWORD>(-1),
            form.empty() ? WINHTTP_NO_REQUEST_DATA : const_cast<char*>(form.data()),
            static_cast<DWORD>(form.size()), static_cast<DWORD>(form.size()), 0) ||
            !WinHttpReceiveResponse(request.value, nullptr)) return std::unexpected{"Identity HTTPS request failed"};
        IdentityHttpResponse response;
        DWORD status = 0, size = sizeof(status);
        if (!WinHttpQueryHeaders(request.value, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                 nullptr, &status, &size, nullptr)) return std::unexpected{"Identity HTTPS status failed"};
        response.status = status;
        for (;;) {
            DWORD available = 0;
            if (!WinHttpQueryDataAvailable(request.value, &available)) return std::unexpected{"Identity HTTPS read failed"};
            if (!available) break;
            if (available > 64 * 1024 - response.body.size()) return std::unexpected{"Identity response too large"};
            const auto offset = response.body.size(); response.body.resize(offset + available);
            DWORD read = 0;
            if (!WinHttpReadData(request.value, response.body.data() + offset, available, &read))
                return std::unexpected{"Identity HTTPS read failed"};
            response.body.resize(offset + read);
        }
        return response; // Protocol layer handles OAuth errors without exposing bodies.
    };
}
} // namespace gloom::backends
#else
namespace gloom::backends {
IdentityHttpPost make_winhttp_identity_post() {
    return [](std::string_view, std::string_view) -> std::expected<IdentityHttpResponse, std::string> {
        return std::unexpected{"Identity HTTPS is supported on Windows"};
    };
}
}
#endif
