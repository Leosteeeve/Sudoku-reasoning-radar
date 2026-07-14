#pragma once

#include <string>

namespace srr::v1 {

constexpr int SchemaVersion = 1;

std::string dispatchJson(const std::string& requestJson);

}  // namespace srr::v1

extern "C" {
char* srr_dispatch(const char* request_json);
void srr_free(char* response_json);
}
