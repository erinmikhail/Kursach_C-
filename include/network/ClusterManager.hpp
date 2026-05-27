#pragma once

#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <sys/types.h>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>

namespace network {

struct StorageNodeInfo {
    int id = 0;
    int port = 0;
    pid_t pid = -1;
    std::string data_dir;
    bool live = false;
    bool removed = false;
    int failed_heartbeats = 0;
};

class ClusterManager {
public:
    ClusterManager(std::string storage_binary, std::string data_dir);
    ~ClusterManager();

    void set_replay_callback(std::function<void(int)> callback);
    void start_initial(size_t count, int base_port);
    StorageNodeInfo add_storage(int requested_port = 0);
    bool remove_storage(int id, std::string& error);

    std::vector<StorageNodeInfo> live_nodes() const;
    std::vector<StorageNodeInfo> snapshot(bool include_removed = true) const;
    std::string status_report() const;

    bool request(int node_id, const nlohmann::json& request_json, nlohmann::json& response, std::string& error) const;
    bool request(const StorageNodeInfo& node, const nlohmann::json& request_json, nlohmann::json& response, std::string& error) const;

    void stop_all();

private:
    void heartbeat_loop();
    void start_process(int id);
    bool wait_until_ready(int port) const;
    bool ping_port(int port) const;
    void terminate_pid(pid_t pid) const;
    int next_available_port();

    std::string storage_binary_;
    std::string data_dir_;
    mutable std::mutex mutex_;
    std::vector<StorageNodeInfo> nodes_;
    int next_id_ = 1;
    int next_port_ = 9001;

    std::function<void(int)> replay_callback_;
    std::atomic<bool> heartbeat_running_{false};
    std::thread heartbeat_thread_;
};

} // namespace network
