#include "network/Server.hpp"
#include "parser/Lexer.hpp"
#include "parser/Parser.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <cstring>
#include <random>
#include <iomanip>
#include <thread>

namespace network {

Server::Server(int port, catalog::Catalog& catalog, core::Executor& executor, api::Logger& logger)
    : auth_manager_("auth.bin"),
      port_(port), 
      server_fd_(-1), 
      running_(false), 
      catalog_(catalog), 
      executor_(executor), 
      logger_(logger) {}

Server::~Server() {
    stop();
}

void Server::stop() {
    running_ = false;
    if (server_fd_ != -1) {
        close(server_fd_);
        server_fd_ = -1;
    }
}

std::string Server::generate_uuid() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static std::uniform_int_distribution<> dis2(8, 11);

    std::stringstream ss;
    ss << std::hex;
    for (int i = 0; i < 8; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 4; i++) ss << dis(gen);
    ss << "-4";
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    ss << dis2(gen);
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 12; i++) ss << dis(gen);
    return ss.str();
}

std::string Server::execute_query(const std::string& query, bool& is_error, double& duration) {
    if (query.empty()) return "";

    auto start_time = std::chrono::steady_clock::now();
    std::string status = "SUCCESS";
    is_error = false;
    
    std::stringstream buffer;
    std::streambuf* old_cout = std::cout.rdbuf(buffer.rdbuf());

    try {
        parser::Lexer lexer(query);
        auto tokens = lexer.tokenize();
        parser::Parser parser(tokens, catalog_);
        auto stmt = parser.parse();
        executor_.execute(*stmt);
    } catch (const std::exception& e) {
        status = std::string("ERROR: ") + e.what();
        buffer << "Ошибка: " << e.what() << "\n";
        is_error = true;
    }

    std::cout.rdbuf(old_cout);

    auto end_time = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> diff = end_time - start_time;
    duration = diff.count();
    logger_.log_request(query, status, duration);

    return buffer.str();
}

void Server::handle_async(const std::string& query, const std::string& uuid, const std::string& db_name) {
    catalog_.setActiveDatabase(db_name);
    
    bool is_error = false;
    double duration = 0.0;
    std::string result = execute_query(query, is_error, duration);
    
    {
        std::lock_guard<std::mutex> lock(async_mutex_);
        async_results_[uuid] = result;
        async_done_[uuid] = true;
    }
    update_telemetry(is_error, duration);
}

void Server::update_telemetry(bool is_error, double duration) {
    auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(telemetry_mutex_);
    
    request_timestamps_.push_back(now);
    if (is_error) error_timestamps_.push_back(now);
    exec_times_.push_back({now, duration});

    auto ten_mins_ago = now - std::chrono::minutes(10);
    while (!request_timestamps_.empty() && request_timestamps_.front() < ten_mins_ago) request_timestamps_.pop_front();
    while (!error_timestamps_.empty() && error_timestamps_.front() < ten_mins_ago) error_timestamps_.pop_front();
    
    auto ten_secs_ago = now - std::chrono::seconds(10);
    while (!exec_times_.empty() && exec_times_.front().first < ten_secs_ago) exec_times_.pop_front();
}

std::string Server::get_telemetry_report() {
    auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(telemetry_mutex_);
    
    auto one_sec_ago = now - std::chrono::seconds(1);
    int current_rps = 0;
    for (auto it = request_timestamps_.rbegin(); it != request_timestamps_.rend(); ++it) {
        if (*it >= one_sec_ago) current_rps++;
        else break;
    }

    double avg_rps_10m = request_timestamps_.size() / 600.0;

    auto one_min_ago = now - std::chrono::minutes(1);
    int errors_1m = 0;
    for (auto it = error_timestamps_.rbegin(); it != error_timestamps_.rend(); ++it) {
        if (*it >= one_min_ago) errors_1m++;
        else break;
    }

    double sum_time = 0;
    for (const auto& pair : exec_times_) sum_time += pair.second;
    double avg_time_10s = exec_times_.empty() ? 0 : sum_time / exec_times_.size();

    std::stringstream ss;
    ss << "ТЕЛЕМЕТРИЯ СЕРВЕРА\n"
       << "Текущий RPS (за 1 сек): " << current_rps << "\n"
       << "Средний RPS (за 10 мин): " << std::fixed << std::setprecision(2) << avg_rps_10m << "\n"
       << "Среднее время (за 10 сек): " << avg_time_10s << " ms\n"
       << "Количество ошибок (за 1 мин): " << errors_1m << "\n"
       << "\n";
    return ss.str();
}

void Server::start() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ == 0) throw std::runtime_error("Socket creation failed");

    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) throw std::runtime_error("Bind failed");
    if (listen(server_fd_, 3) < 0) throw std::runtime_error("Listen failed");

    running_ = true;
    std::cout << "[Server] БД запущена на порту " << port_ << "...\n";

    while (running_) {
        int addrlen = sizeof(address);
        int client_socket = accept(server_fd_, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (client_socket < 0) continue;
        process_client(client_socket);
    }
}

void Server::process_client(int client_socket) {
    char buffer[4096];
    std::string current_user = "";

    while (running_) {
        memset(buffer, 0, 4096);
        int valread = read(client_socket, buffer, 4096);
        if (valread <= 0) break;

        std::string query(buffer);
        if (!query.empty() && query.back() == '\n') query.pop_back();

        if (query == ".exit") {
            send(client_socket, "Bye!\n", 5, 0);
            break;
        }

        if (query.rfind("LOGIN ", 0) == 0) {
            std::stringstream ss(query);
            std::string cmd, u, p; ss >> cmd >> u >> p;
            try {
                std::string jwt = auth_manager_.login(u, p);
                std::string resp = "Token: " + jwt + "\n";
                send(client_socket, resp.c_str(), resp.length(), 0);
            } catch (...) { send(client_socket, "Login fail\n", 11, 0); }
            continue;
        }

        if (query.rfind("AUTH ", 0) == 0) {
            try {
                current_user = auth_manager_.validate_jwt(query.substr(5));
                send(client_socket, "OK\n", 3, 0);
            } catch (...) { send(client_socket, "Auth fail\n", 10, 0); }
            continue;
        }

        if (current_user.empty()) {
            send(client_socket, "Login required\n", 15, 0);
            continue;
        }

        try {
            parser::Lexer lexer(query);
            auto stmt = parser::Parser(lexer.tokenize(), catalog_).parse();
            if (!auth_manager_.check_permission(current_user, catalog_.getActiveDatabase(), stmt->type)) {
                send(client_socket, "Access Denied\n", 14, 0);
                continue;
            }
        } catch (...) {}

        if (query.rfind("ASYNC ", 0) == 0) {
            std::string uuid = generate_uuid();
            { std::lock_guard<std::mutex> lock(async_mutex_); async_done_[uuid] = false; }
            std::thread(&Server::handle_async, this, query.substr(6), uuid, catalog_.getActiveDatabase()).detach();
            std::string resp = "GUID: " + uuid + "\n";
            send(client_socket, resp.c_str(), resp.length(), 0);
            continue;
        }

        if (query == "TELEMETRY") {
            std::string resp = get_telemetry_report();
            send(client_socket, resp.c_str(), resp.length(), 0);
            continue;
        }

        bool err; double dur;
        std::string resp = execute_query(query, err, dur);
        update_telemetry(err, dur);
        send(client_socket, resp.empty() ? "OK\n" : resp.c_str(), resp.empty() ? 3 : resp.length(), 0);
    }
    close(client_socket);
}

} 