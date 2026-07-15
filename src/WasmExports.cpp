#include "CoreProtocol.h"

#include <cstdlib>
#include <cstring>
#include <string>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#define SRR_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define SRR_EXPORT
#endif

namespace {
char* copyResponse(const char* data, std::size_t size) noexcept {
    char* result = static_cast<char*>(std::malloc(size + 1));
    if (!result) return nullptr;
    std::memcpy(result, data, size);
    result[size] = '\0';
    return result;
}
}

extern "C" SRR_EXPORT char* srr_dispatch(const char* request_json) noexcept {
    try {
        const std::string response = srr::v1::dispatchJson(request_json ? request_json : "");
        return copyResponse(response.data(), response.size());
    } catch (...) {
        constexpr const char fallback[] =
            "{\"schemaVersion\":1,\"operation\":null,\"ok\":false,"
            "\"error\":{\"code\":\"internal_error\",\"path\":\"$\",\"params\":{}}}";
        return copyResponse(fallback, sizeof(fallback) - 1);
    }
}

extern "C" SRR_EXPORT void srr_free(char* response_json) noexcept {
    std::free(response_json);
}
