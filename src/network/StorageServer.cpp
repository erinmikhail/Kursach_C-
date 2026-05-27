#include "network/StorageServer.hpp"
#include "core/QueryRunner.hpp"
#include "network/SocketUtils.hpp"

#include <filesystem>
#include <iostream>
#include <netinet/in.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

namespace network {
namespace {

std::string path_in_dir(const std::string& dir, const std::string& file) {
    if (dir.empty() || dir == ".") return file;
    return (std::filesystem::path(dir) / file).string();
}

nlohmann::json result_to_json(const core::QueryResult& result) {
    return {
        {"type", "result"},
        {"ok", result.ok},
        {"error", result.error},
        {"text", result.text},
        {"headers", result.headers},
        {"rows", result.rows},
        {"duration_ms", result.duration_ms}
    };
}

} // namespace

StorageServer::StorageServer(int id, int port, std::string data_dir)
    : id_(id),
      port_(port),
      data_dir_(std::move(data_dir)),
      catalog_(data_dir_),
      executor_(catalog_),
      logger_(path_in_dir(data_dir_, "storage_access.log")) {}

StorageServer::~StorageServer() {
    stop();
}

void StorageServer::stop() {
    running_ = false;
    if (server_fd_ != -1) {
        close(server_fd_);
        server_fd_ = -1;
    }
}

void StorageServer::start() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) throw std::runtime_error("Storage socket creation failed");

    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(static_cast<uint16_t>(port_));

    if (bind(server_fd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        throw std::runtime_error("Storage bind failed");
    }
    if (listen(server_fd_, 32) < 0) throw std::runtime_error("Storage listen failed");

    running_ = true;
    std::cout << "[Storage " << id_ << "] listening on port " << port_ << "\n";

    while (running_) {
        socklen_t addrlen = sizeof(address);
        const int client_socket = accept(server_fd_, reinterpret_cast<sockaddr*>(&address), &addrlen);
        if (client_socket < 0) continue;
        std::thread(&StorageServer::process_client, this, client_socket).detach();
    }
}

void StorageServer::process_client(int client_socket) {
    std::string line;
    while (running_ && read_line(client_socket, line)) {
        nlohmann::json response;
        try {
            const auto request = nlohmann::json::parse(line);
            const std::string type = request.value("type", "");

            if (type == "ping") {
                response = {{"type", "pong"}, {"ok", true}, {"id", id_}};
            } else if (type == "shutdown") {
                response = {{"type", "shutdown"}, {"ok", true}};
                send_line(client_socket, response.dump());
                stop();
                break;
            } else if (type == "exec") {
                const std::string db = request.value("db", "default_db");
                const std::string query = request.value("query", "");
                core::QueryResult result;
                {
                    std::lock_guard<std::mutex> lock(execute_mutex_);
                    if (catalog_.databaseExists(db)) catalog_.setActiveDatabase(db);
                    result = core::run_query(query, catalog_, executor_, logger_);
                }
                response = result_to_json(result);
            } else {
                response = {{"type", "error"}, {"ok", false}, {"error", "Unknown storage request"}};
            }
        } catch (const std::exception& e) {
            response = {{"type", "error"}, {"ok", false}, {"error", e.what()}};
        }
        send_line(client_socket, response.dump());
    }
    close(client_socket);
}

} // namespace network
