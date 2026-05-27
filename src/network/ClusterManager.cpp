#include "network/ClusterManager.hpp"
#include "network/SocketUtils.hpp"

#include <chrono>
#include <csignal>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <utility>

namespace network {
namespace {

constexpr int kHeartbeatIntervalMs = 1000;
constexpr int kHeartbeatMissLimit = 3;

std::string node_dir(const std::string& root, int id) {
    return (std::filesystem::path(root) / ("storage_" + std::to_string(id))).string();
}

} // namespace

ClusterManager::ClusterManager(std::string storage_binary, std::string data_dir)
    : storage_binary_(std::move(storage_binary)), data_dir_(std::move(data_dir)) {
    std::filesystem::create_directories(data_dir_);
}

ClusterManager::~ClusterManager() {
    stop_all();
}

void ClusterManager::set_replay_callback(std::function<void(int)> callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    replay_callback_ = std::move(callback);
}

void ClusterManager::start_initial(size_t count, int base_port) {
    next_port_ = base_port;
    for (size_t i = 0; i < count; ++i) add_storage(0);
    heartbeat_running_ = true;
    heartbeat_thread_ = std::thread(&ClusterManager::heartbeat_loop, this);
}

StorageNodeInfo ClusterManager::add_storage(int requested_port) {
    StorageNodeInfo node;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        node.id = next_id_++;
        node.port = requested_port > 0 ? requested_port : next_available_port();
        node.data_dir = node_dir(data_dir_, node.id);
        nodes_.push_back(node);
    }

    start_process(node.id);

    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& stored : nodes_) {
        if (stored.id == node.id) return stored;
    }
    return node;
}

bool ClusterManager::remove_storage(int id, std::string& error) {
    StorageNodeInfo node;
    bool found = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& candidate : nodes_) {
            if (candidate.id == id && !candidate.removed) {
                node = candidate;
                candidate.removed = true;
                candidate.live = false;
                found = true;
                break;
            }
        }
    }
    if (!found) {
        error = "Storage node not found";
        return false;
    }

    nlohmann::json ignored;
    std::string ignored_error;
    request(node, {{"type", "shutdown"}}, ignored, ignored_error);
    terminate_pid(node.pid);
    return true;
}

std::vector<StorageNodeInfo> ClusterManager::live_nodes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<StorageNodeInfo> result;
    for (const auto& node : nodes_) {
        if (!node.removed && node.live) result.push_back(node);
    }
    return result;
}

std::vector<StorageNodeInfo> ClusterManager::snapshot(bool include_removed) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<StorageNodeInfo> result;
    for (const auto& node : nodes_) {
        if (include_removed || !node.removed) result.push_back(node);
    }
    return result;
}

std::string ClusterManager::status_report() const {
    std::ostringstream ss;
    ss << "CLUSTER STATUS\n";
    for (const auto& node : snapshot(true)) {
        ss << "Storage " << node.id
           << " port=" << node.port
           << " pid=" << static_cast<long>(node.pid)
           << " state=" << (node.removed ? "removed" : (node.live ? "live" : "down"))
           << " missed=" << node.failed_heartbeats
           << " data_dir=" << node.data_dir << "\n";
    }
    return ss.str();
}

bool ClusterManager::request(int node_id, const nlohmann::json& request_json, nlohmann::json& response, std::string& error) const {
    StorageNodeInfo node;
    bool found = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& candidate : nodes_) {
            if (candidate.id == node_id && !candidate.removed) {
                node = candidate;
                found = true;
                break;
            }
        }
    }
    if (!found) {
        error = "Storage node not found";
        return false;
    }
    return request(node, request_json, response, error);
}

bool ClusterManager::request(const StorageNodeInfo& node, const nlohmann::json& request_json, nlohmann::json& response, std::string& error) const {
    if (node.removed) {
        error = "Storage node removed";
        return false;
    }
    const int fd = connect_tcp("127.0.0.1", node.port, 1000);
    if (fd < 0) {
        error = "Storage connect failed";
        return false;
    }

    bool ok = send_line(fd, request_json.dump());
    std::string line;
    if (ok) ok = read_line(fd, line);
    close(fd);

    if (!ok) {
        error = "Storage request failed";
        return false;
    }

    try {
        response = nlohmann::json::parse(line);
    } catch (const std::exception& e) {
        error = e.what();
        return false;
    }
    return true;
}

void ClusterManager::stop_all() {
    heartbeat_running_ = false;
    if (heartbeat_thread_.joinable()) heartbeat_thread_.join();

    const auto nodes = snapshot(true);
    for (const auto& node : nodes) {
        if (node.pid > 0 && !node.removed) {
            nlohmann::json ignored;
            std::string ignored_error;
            request(node, {{"type", "shutdown"}}, ignored, ignored_error);
            terminate_pid(node.pid);
        }
    }
}

void ClusterManager::heartbeat_loop() {
    while (heartbeat_running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(kHeartbeatIntervalMs));
        const auto nodes = snapshot(false);
        for (const auto& node : nodes) {
            const bool ok = ping_port(node.port);
            bool should_restart = false;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                for (auto& stored : nodes_) {
                    if (stored.id != node.id || stored.removed) continue;
                    if (ok) {
                        stored.live = true;
                        stored.failed_heartbeats = 0;
                    } else {
                        stored.live = false;
                        stored.failed_heartbeats++;
                        should_restart = stored.failed_heartbeats >= kHeartbeatMissLimit;
                    }
                    break;
                }
            }
            if (should_restart) start_process(node.id);
        }
    }
}

void ClusterManager::start_process(int id) {
    StorageNodeInfo node;
    bool found = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& candidate : nodes_) {
            if (candidate.id == id && !candidate.removed) {
                node = candidate;
                found = true;
                break;
            }
        }
    }
    if (!found) return;

    if (node.pid > 0) terminate_pid(node.pid);
    std::filesystem::create_directories(node.data_dir);

    const pid_t pid = fork();
    if (pid == 0) {
        const std::string id_arg = std::to_string(node.id);
        const std::string port_arg = std::to_string(node.port);
        execl(storage_binary_.c_str(), storage_binary_.c_str(),
              "--id", id_arg.c_str(),
              "--port", port_arg.c_str(),
              "--data-dir", node.data_dir.c_str(),
              static_cast<char*>(nullptr));
        _exit(127);
    }
    if (pid < 0) throw std::runtime_error("Failed to fork storage process");

    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& stored : nodes_) {
            if (stored.id == id && !stored.removed) {
                stored.pid = pid;
                stored.live = false;
                break;
            }
        }
    }

    const bool ready = wait_until_ready(node.port);
    std::function<void(int)> callback;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& stored : nodes_) {
            if (stored.id == id && !stored.removed) {
                stored.live = ready;
                stored.failed_heartbeats = ready ? 0 : stored.failed_heartbeats + 1;
                break;
            }
        }
        callback = replay_callback_;
    }
    if (ready && callback) callback(id);
}

bool ClusterManager::wait_until_ready(int port) const {
    for (int i = 0; i < 30; ++i) {
        if (ping_port(port)) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return false;
}

bool ClusterManager::ping_port(int port) const {
    const int fd = connect_tcp("127.0.0.1", port, 500);
    if (fd < 0) return false;
    bool ok = send_line(fd, nlohmann::json{{"type", "ping"}}.dump());
    std::string line;
    if (ok) ok = read_line(fd, line);
    close(fd);
    if (!ok) return false;
    try {
        const auto response = nlohmann::json::parse(line);
        return response.value("ok", false) && response.value("type", "") == "pong";
    } catch (...) {
        return false;
    }
}

void ClusterManager::terminate_pid(pid_t pid) const {
    if (pid <= 0) return;
    if (waitpid(pid, nullptr, WNOHANG) == pid) return;
    kill(pid, SIGTERM);
    for (int i = 0; i < 10; ++i) {
        if (waitpid(pid, nullptr, WNOHANG) == pid) return;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    kill(pid, SIGKILL);
    waitpid(pid, nullptr, 0);
}

int ClusterManager::next_available_port() {
    int port = next_port_;
    bool used = true;
    while (used) {
        used = false;
        for (const auto& node : nodes_) {
            if (node.port == port && !node.removed) {
                used = true;
                ++port;
                break;
            }
        }
    }
    next_port_ = port + 1;
    return port;
}

} // namespace network
