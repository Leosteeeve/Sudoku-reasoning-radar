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

extern "C" SRR_EXPORT char* srr_dispatch(const char* request_json) {
    const std::string response = srr::v1::dispatchJson(request_json ? request_json : "");
    char* result = static_cast<char*>(std::malloc(response.size() + 1));
    if (!result) return nullptr;
    std::memcpy(result, response.c_str(), response.size() + 1);
    return result;
}

extern "C" SRR_EXPORT void srr_free(char* response_json) {
    std::free(response_json);
}
