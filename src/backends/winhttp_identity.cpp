#include <gloom/backends/keycloak_identity.hpp>
#include <gloom/backends/winhttp_request.hpp>
#include <assert.h>
#include <stdlib.h>
#include <wchar.h>
#include <algorithm>
#include <limits>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winhttp.h>

namespace gloom::backends {
namespace {
struct RequestHandles {
    HINTERNET session{nullptr}, connection{nullptr}, request{nullptr};
    wchar_t* address{nullptr};
    ~RequestHandles() {
        if (request)
            WinHttpCloseHandle(request);
        if (connection)
            WinHttpCloseHandle(connection);
        if (session)
            WinHttpCloseHandle(session);
        free(address);
    }
};
}

WinHttpResponse winhttp_request(const wchar_t* url, const wchar_t* suffix, const wchar_t* method, const wchar_t* headers, const char* body, uint32 body_size,
    uint32 response_limit, const wchar_t* agent) {
    assert(url && suffix && method && headers && agent && (body || body_size == 0));
    URL_COMPONENTS parts{sizeof(parts)};
    parts.dwSchemeLength = parts.dwHostNameLength = parts.dwUrlPathLength = parts.dwUserNameLength = parts.dwPasswordLength = parts.dwExtraInfoLength =
        static_cast<DWORD>(-1);
    if (!WinHttpCrackUrl(url, 0, 0, &parts) || parts.nScheme != INTERNET_SCHEME_HTTPS || parts.dwUserNameLength || parts.dwPasswordLength ||
        parts.dwExtraInfoLength || !parts.dwHostNameLength)
        return {.error = "HTTPS endpoint must have a host and no embedded credentials or query"};
    RequestHandles handles;
    handles.address = static_cast<wchar_t*>(malloc((parts.dwHostNameLength + parts.dwUrlPathLength + wcslen(suffix) + 2) * sizeof(wchar_t)));
    wmemcpy(handles.address, parts.lpszHostName, parts.dwHostNameLength);
    handles.address[parts.dwHostNameLength] = 0;
    wchar_t* path = handles.address + parts.dwHostNameLength + 1;
    uint32 length = parts.dwUrlPathLength;
    if (suffix[0] && length && parts.lpszUrlPath[length - 1] == L'/')
        --length;
    if (length)
        wmemcpy(path, parts.lpszUrlPath, length);
    wmemcpy(path + length, suffix, wcslen(suffix) + 1);
    handles.session = WinHttpOpen(agent, WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!handles.session || !WinHttpSetTimeouts(handles.session, 3000, 3000, 5000, 5000))
        return {.error = "HTTPS session setup failed"};
    handles.connection = WinHttpConnect(handles.session, handles.address, parts.nPort, 0);
    if (!handles.connection)
        return {.error = "HTTPS connection failed"};
    handles.request = WinHttpOpenRequest(handles.connection, method, path, nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    DWORD redirect = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    if (!handles.request || !WinHttpSetOption(handles.request, WINHTTP_OPTION_REDIRECT_POLICY, &redirect, sizeof(redirect)))
        return {.error = "HTTPS request setup failed"};
    if (!WinHttpSendRequest(
            handles.request, headers, static_cast<DWORD>(-1), body_size ? const_cast<char*>(body) : WINHTTP_NO_REQUEST_DATA, body_size, body_size, 0) ||
        !WinHttpReceiveResponse(handles.request, nullptr))
        return {.error = "HTTPS request failed"};
    WinHttpResponse response;
    DWORD status = 0, size = sizeof(status);
    if (!WinHttpQueryHeaders(handles.request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, nullptr, &status, &size, nullptr))
        return {.error = "HTTPS status failed"};
    response.status = status;
    for (;;) {
        DWORD available = 0, read = 0;
        if (!WinHttpQueryDataAvailable(handles.request, &available)) {
            response.error = "HTTPS read failed";
            break;
        }
        if (!available)
            return response;
        if (available > response_limit - response.size) {
            response.error = "HTTPS response too large";
            break;
        }
        response.body = static_cast<char*>(realloc(response.body, response.size + available));
        if (!WinHttpReadData(handles.request, response.body + response.size, available, &read)) {
            response.error = "HTTPS read failed";
            break;
        }
        response.size += read;
    }
    free(response.body);
    response.body = nullptr;
    response.size = 0;
    return response;
}

IdentityHttpPost make_winhttp_identity_post() {
    return [](std::string_view url, std::string_view form) -> std::expected<IdentityHttpResponse, std::string> {
        if (url.size() > 2048 || form.size() > 32 * 1024 || !std::ranges::all_of(url, [](unsigned char c) {
                return c > 32 && c < 127 && c != '\\';
            }))
            return std::unexpected{"Invalid identity HTTP request"};
        const std::wstring wide{url.begin(), url.end()};
        const WinHttpResponse received =
            winhttp_request(wide.c_str(), L"", L"POST", L"Content-Type: application/x-www-form-urlencoded\r\nAccept: application/json", form.data(),
                static_cast<uint32>(form.size()), 64 * 1024, L"GloomIdentity/1");
        if (received.error)
            return std::unexpected{received.error};
        IdentityHttpResponse response{.status = received.status};
        if (received.size)
            response.body.assign(received.body, received.size);
        free(received.body);
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
