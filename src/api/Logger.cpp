#include "api/Logger.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace api {

namespace {

std::string current_timestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t now_time = std::chrono::system_clock::to_time_t(now);

    std::tm local_time{};
#if defined(_WIN32)
    localtime_s(&local_time, &now_time);
#else
    localtime_r(&now_time, &local_time);
#endif

    std::ostringstream stream;
    stream << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S");
    return stream.str();
}

std::string escape_request(const std::string& request) {
    std::string escaped;
    escaped.reserve(request.size());

    for (char ch : request) {
        switch (ch) {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped.push_back(ch);
                break;
        }
    }

    return escaped;
}

} // namespace

Logger::Logger(const std::string& path)
    : file_path_(path) {
    log_file_.open(file_path_, std::ios::out | std::ios::app);
    if (!log_file_.is_open()) {
        throw std::runtime_error("Не удалось открыть файл лога: " + file_path_);
    }

    log_file_ << std::unitbuf;
}

Logger::~Logger() {
    if (log_file_.is_open()) {
        log_file_.flush();
    }
}

void Logger::log_request(const std::string& request,
                         const std::string& status,
                         double duration_ms) {
    if (!log_file_.is_open()) {
        return;
    }

    log_file_ << "[" << current_timestamp() << "]"
              << " | REQ: \"" << escape_request(request) << "\""
              << " | STATUS: " << status
              << " | TIME: " << std::fixed << std::setprecision(3) << duration_ms << "ms"
              << '\n';
    log_file_.flush();
}

} // namespace api
