#pragma once

#include <string>
#include <vector>

namespace core {

struct QueryResult {
    bool ok = true;
    std::string error;
    std::string text;
    std::vector<std::string> headers;
    std::vector<std::vector<std::string>> rows;
    double duration_ms = 0.0;
};

} // namespace core
