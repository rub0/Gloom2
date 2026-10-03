#pragma once
#include <gloom/core/types.hpp>
#include <stddef.h>

namespace gloom::backends {
struct WinHttpResponse {
    uint32 status{0};
    char* body{nullptr};
    uint32 size{0};
    const char* error{nullptr};
};
// Caller frees body with free(). Base URL is HTTPS without credentials/query;
// suffix may contain the match API query. TLS verification and redirects are never relaxed.
// Text pointers must be nonnull; body may be null only when body_size is zero.
[[nodiscard]] WinHttpResponse winhttp_request(const wchar_t* url, const wchar_t* suffix, const wchar_t* method,
    const wchar_t* headers, const char* body, uint32 body_size, uint32 response_limit, const wchar_t* agent);
}
