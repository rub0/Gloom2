#include <gloom/backends/winhttp_match_service.hpp>
#include <gloom/backends/match_reader_credential.hpp>
#include <gloom/backends/match_reader_grants.hpp>
#include <gloom/gameplay/match_service_wire.hpp>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winhttp.h>

#include <charconv>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <limits>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace gloom::backends {
namespace {

struct HttpResult { DWORD status{0}; std::string body; };

[[nodiscard]] std::wstring widen(const std::string_view text) {
    if (text.empty()) return {};
    const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                                         static_cast<int>(text.size()), nullptr, 0);
    if (size <= 0) throw std::runtime_error{"Invalid UTF-8 in match service configuration"};
    std::wstring result(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                        static_cast<int>(text.size()), result.data(), size);
    return result;
}

[[nodiscard]] std::string encode_component(const std::string_view value) {
    constexpr char hex[] = "0123456789ABCDEF";
    std::string result;
    for (const unsigned char c : value) {
        if (std::isalnum(c) != 0 || c == '-' || c == '_' || c == '.') result += static_cast<char>(c);
        else { result += '%'; result += hex[c >> 4U]; result += hex[c & 15U]; }
    }
    return result;
}

class WinHttpService final : public gameplay::SliceMatchService {
public:
    WinHttpService(std::string url, std::string token, bool reader = false,
                   std::function<std::expected<std::string, std::string>()> acquire_identity = {})
        : url_{std::move(url)}, token_{std::move(token)}, reader_{reader}, acquire_identity_{std::move(acquire_identity)} {}

    std::expected<void, std::string> store(gameplay::SliceStoredMatch value,
                                            const std::uint64_t now) override {
        const auto response = request("PUT", "/matches/" + encode_component(value.advertisement.match_id) +
            "?now_ms=" + std::to_string(now), serialize(value));
        if (!response) return std::unexpected{response.error()};
        return {};
    }
    std::expected<bool, std::string> erase(const std::string_view id,
                                            const std::string_view instance) override {
        const auto response = request("DELETE", "/matches/" + encode_component(id) +
            "?instance_id=" + encode_component(instance), {});
        if (!response) return std::unexpected{response.error()};
        return response->status != 404;
    }
    std::expected<std::vector<gameplay::SliceStoredMatch>, std::string> list() const override {
        const auto response = request("GET", "/matches", {});
        if (!response) return std::unexpected{response.error()};
        std::vector<gameplay::SliceStoredMatch> result;
        std::istringstream lines{response->body};
        for (std::string line; std::getline(lines, line);) {
            if (line.empty()) continue;
            auto item = deserialize(line);
            if (!item) return std::unexpected{item.error()};
            result.push_back(std::move(*item));
        }
        return result;
    }
    std::expected<gameplay::SliceStoredMatch, std::string>
    get(const std::string_view id) const override {
        const auto response = request("GET", "/matches/" + encode_component(id), {});
        if (!response) return std::unexpected{response.error()};
        return deserialize(response->body);
    }

    [[nodiscard]] std::expected<void, std::string> health() const {
        auto response = request("GET", "/health", {});
        if (!response) return std::unexpected{response.error()};
        return {};
    }

    std::expected<GameJoinTicket, std::string> game_ticket(std::string_view match) const {
        const auto response = send_request("POST", "/game-tickets/" + encode_component(match), {}, token_);
        if (!response) return std::unexpected{response.error()};
        const auto& body = response->body;
        const auto first = body.find('\t'), second = body.find('\t', first == std::string::npos ? 0 : first + 1);
        if (first == std::string::npos || second == std::string::npos || body.find('\t', second + 1) != std::string::npos || body.size() > 1024)
            return std::unexpected{"Invalid game ticket response"};
        GameJoinTicket result{body.substr(0, first), body.substr(first + 1, second - first - 1), body.substr(second + 1)};
        const auto endpoint = gameplay::resolve_slice_join_target(result.endpoint, nullptr, 0);
        if (!endpoint || result.instance.empty() || !valid_match_credential(result.credential) || !result.credential.starts_with("gt1."))
            return std::unexpected{"Invalid game ticket response"};
        return result;
    }
private:
    static std::string serialize(const gameplay::SliceStoredMatch& item) {
        return gameplay::encode_stored_match(item);
    }
    static std::expected<gameplay::SliceStoredMatch, std::string> deserialize(std::string_view line) {
        return gameplay::decode_stored_match(line);
    }

    std::expected<HttpResult, std::string> request(const std::string_view method,
                                                   const std::string_view suffix,
                                                   const std::string_view body) const {
        if (!reader_) return send_request(method, suffix, body, token_);
        if (method != "GET") return std::unexpected{"Reader service cannot mutate matches"};
        std::lock_guard lock{reader_mutex_};
        const auto grant = reader_credential_.acquire([&]() -> std::expected<std::string, std::string> {
            const auto identity = acquire_identity_ ? acquire_identity_() : std::expected<std::string, std::string>{token_};
            if (!identity) return std::unexpected{identity.error()};
            const auto exchanged = send_request("POST", "/reader-grants/exchange", {}, *identity);
            if (!exchanged) return std::unexpected{exchanged.error()};
            return exchanged->body;
        });
        if (!grant) return std::unexpected{grant.error()};
        // No replay on 401/429: revocation and quota errors must remain visible.
        return send_request(method, suffix, body, *grant);
    }

    std::expected<HttpResult, std::string> send_request(const std::string_view method,
        const std::string_view suffix, const std::string_view body, const std::string_view token) const {
        if (!(reader_ ? valid_match_identity_credential(token) : valid_match_credential(token)))
            return std::unexpected{"Invalid match service credential"};
        URL_COMPONENTS parts{sizeof(parts)}; parts.dwSchemeLength = parts.dwHostNameLength =
            parts.dwUrlPathLength = parts.dwUserNameLength = parts.dwPasswordLength =
            parts.dwExtraInfoLength = static_cast<DWORD>(-1);
        const auto wide_url = widen(url_);
        if (!WinHttpCrackUrl(wide_url.c_str(), 0, 0, &parts) || parts.nScheme != INTERNET_SCHEME_HTTPS ||
            parts.dwUserNameLength || parts.dwPasswordLength || parts.dwExtraInfoLength)
            return std::unexpected{"Match service URL must be valid HTTPS"};
        const std::wstring host{parts.lpszHostName, parts.dwHostNameLength};
        std::wstring path{parts.lpszUrlPath, parts.dwUrlPathLength};
        if (!path.empty() && path.back() == L'/') path.pop_back();
        path += widen(suffix);
        HINTERNET session = WinHttpOpen(L"Gloom/0.1", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!session) return std::unexpected{"WinHTTP session failed"};
        WinHttpSetTimeouts(session, 3000, 3000, 5000, 5000);
        HINTERNET connection = WinHttpConnect(session, host.c_str(), parts.nPort, 0);
        HINTERNET request_handle = connection ? WinHttpOpenRequest(connection, widen(method).c_str(),
            path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE) : nullptr;
        DWORD redirect_policy = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
        if (request_handle) WinHttpSetOption(request_handle, WINHTTP_OPTION_REDIRECT_POLICY,
                                             &redirect_policy, sizeof(redirect_policy));
        const auto auth = widen("Authorization: Bearer " + std::string{token} + "\r\nContent-Type: text/plain; charset=utf-8");
        BOOL ok = request_handle && WinHttpSendRequest(request_handle, auth.c_str(), static_cast<DWORD>(-1),
            body.empty() ? WINHTTP_NO_REQUEST_DATA : const_cast<char*>(body.data()),
            static_cast<DWORD>(body.size()), static_cast<DWORD>(body.size()), 0) &&
            WinHttpReceiveResponse(request_handle, nullptr);
        HttpResult result;
        if (ok) {
            DWORD size = sizeof(result.status);
            ok = WinHttpQueryHeaders(request_handle, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                     nullptr, &result.status, &size, nullptr);
        }
        while (ok) {
            DWORD available = 0;
            if (!WinHttpQueryDataAvailable(request_handle, &available)) { ok = FALSE; break; }
            if (available == 0) break;
            if (result.body.size() + available > 4 * 1024 * 1024) { ok = FALSE; break; }
            const auto offset = result.body.size(); result.body.resize(offset + available); DWORD read = 0;
            if (!WinHttpReadData(request_handle, result.body.data()+offset, available, &read)) { ok = FALSE; break; }
            result.body.resize(offset + read);
        }
        if (request_handle) WinHttpCloseHandle(request_handle);
        if (connection) WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        if (!ok) return std::unexpected{"Match service HTTPS request failed"};
        if (result.status < 200 || result.status >= 300) {
            if (result.status == 404 && method == "DELETE") return result;
            return std::unexpected{"Match service returned HTTP " + std::to_string(result.status)};
        }
        return result;
    }
    std::string url_; std::string token_;
    bool reader_{false};
    std::function<std::expected<std::string, std::string>()> acquire_identity_;
    mutable std::mutex reader_mutex_;
    mutable MatchReaderCredential reader_credential_;
};

class Connector final : public gameplay::SliceMatchServiceConnector {
public:
    explicit Connector(bool reader = false, std::function<std::expected<std::string, std::string>()> acquire = {})
        : reader_{reader}, acquire_identity_{std::move(acquire)} {}
    std::expected<std::shared_ptr<gameplay::SliceMatchService>, std::string>
    connect(const std::string_view url, const std::string_view token) override {
        auto service = std::make_shared<WinHttpService>(std::string{url}, std::string{token}, reader_, acquire_identity_);
        if (auto result = service->health(); !result) return std::unexpected{result.error()};
        return service;
    }
private:
    bool reader_;
    std::function<std::expected<std::string, std::string>()> acquire_identity_;
};
} // namespace

std::unique_ptr<gameplay::SliceMatchServiceConnector> make_winhttp_match_service_connector() {
    return std::make_unique<Connector>();
}
std::expected<GameJoinTicket, std::string> request_game_ticket(
    std::string_view url, std::string_view identity, std::string_view match) {
    return WinHttpService{std::string{url}, std::string{identity}, true}.game_ticket(match);
}
std::unique_ptr<gameplay::SliceMatchServiceConnector> make_winhttp_match_reader_connector(
    std::function<std::expected<std::string, std::string>()> acquire) {
    return std::make_unique<Connector>(true, std::move(acquire));
}
} // namespace gloom::backends
#else
namespace gloom::backends {
std::expected<GameJoinTicket, std::string> request_game_ticket(std::string_view, std::string_view, std::string_view) {
    return std::unexpected{"WinHTTP is unavailable"};
}
std::unique_ptr<gameplay::SliceMatchServiceConnector> make_winhttp_match_service_connector() { return {}; }
std::unique_ptr<gameplay::SliceMatchServiceConnector> make_winhttp_match_reader_connector(
    std::function<std::expected<std::string, std::string>()>) { return {}; }
}
#endif
