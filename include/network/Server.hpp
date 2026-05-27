#pragma once

#include "catalog/Catalog.hpp"
#include "core/Executor.hpp"
#include "api/Logger.hpp"
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <deque>
#include <chrono>

namespace network {

class Server {
public:
    Server(int port, catalog::Catalog& catalog, core::Executor& executor, api::Logger& logger);
    ~Server();
    void start();
    void stop();

private:
    void process_client(int client_socket);
    std::string execute_query(const std::string& query, bool& is_error, double& duration);
    
    // Подсистема асинхронности (Задание 6)
    std::string generate_uuid();
    void handle_async(const std::string& query, const std::string& uuid);
    
    // Подсистема телеметрии (Задание 8)
    void update_telemetry(bool is_error, double duration);
    std::string get_telemetry_report();

    int port_;
    int server_fd_;
    std::atomic<bool> running_;
    
    catalog::Catalog& catalog_;
    core::Executor& executor_;
    api::Logger& logger_;

    // Хранилище асинхронных задач
    std::mutex async_mutex_;
    std::unordered_map<std::string, std::string> async_results_;
    std::unordered_map<std::string, bool> async_done_;

    // Хранилище метрик телеметрии
    std::mutex telemetry_mutex_;
    std::deque<std::chrono::steady_clock::time_point> request_timestamps_;
    std::deque<std::chrono::steady_clock::time_point> error_timestamps_;
    std::deque<std::pair<std::chrono::steady_clock::time_point, double>> exec_times_;
};

} 