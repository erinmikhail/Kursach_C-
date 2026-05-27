#include "core/QueryRunner.hpp"
#include "api/ConsoleInterface.hpp"
#include "parser/Lexer.hpp"
#include "parser/Parser.hpp"

#include <chrono>
#include <cctype>
#include <iomanip>
#include <sstream>

namespace core {
namespace {

bool is_numeric(const std::string& str) {
    if (str.empty()) return false;
    size_t start = 0;
    if (str[0] == '-' || str[0] == '+') {
        if (str.size() == 1) return false;
        start = 1;
    }
    bool has_dot = false;
    for (size_t i = start; i < str.size(); ++i) {
        if (str[i] == '.') {
            if (has_dot) return false;
            has_dot = true;
        } else if (!std::isdigit(static_cast<unsigned char>(str[i]))) {
            return false;
        }
    }
    return true;
}

std::string json_escape(const std::string& value) {
    std::string out;
    out.reserve(value.size() + 8);
    for (char ch : value) {
        switch (ch) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out.push_back(ch); break;
        }
    }
    return out;
}

} // namespace

QueryResult run_query(const std::string& query,
                      catalog::Catalog& catalog,
                      Executor& executor,
                      api::Logger& logger) {
    QueryResult result;
    const auto start = std::chrono::steady_clock::now();
    std::string status = "SUCCESS";

    try {
        parser::Lexer lexer(query);
        const auto tokens = lexer.tokenize();
        parser::Parser parser(tokens, catalog);
        auto stmt = parser.parse();
        result = executor.execute_structured(*stmt);
    } catch (const std::exception& e) {
        result.ok = false;
        result.error = e.what();
        result.text = std::string("Ошибка: ") + e.what() + "\n";
        status = std::string("ERROR: ") + e.what();
    }

    const auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> diff = end - start;
    result.duration_ms = diff.count();
    logger.log_request(query, status, result.duration_ms);
    return result;
}

std::string format_query_result(const QueryResult& result) {
    if (!result.ok) return result.text.empty() ? ("Ошибка: " + result.error + "\n") : result.text;
    if (!result.headers.empty()) {
        std::ostringstream ss;
        ss << "[\n";
        for (size_t r = 0; r < result.rows.size(); ++r) {
            ss << "  {\n";
            for (size_t c = 0; c < result.headers.size(); ++c) {
                ss << "    \"" << json_escape(result.headers[c]) << "\": ";
                const std::string val = c < result.rows[r].size() ? result.rows[r][c] : "";
                if (is_numeric(val)) ss << val;
                else ss << "\"" << json_escape(val) << "\"";
                if (c + 1 < result.headers.size()) ss << ",";
                ss << "\n";
            }
            ss << "  }";
            if (r + 1 < result.rows.size()) ss << ",";
            ss << "\n";
        }
        ss << "]\n";
        return ss.str();
    }
    if (!result.text.empty()) return result.text;
    return "OK\n";
}

} // namespace core
