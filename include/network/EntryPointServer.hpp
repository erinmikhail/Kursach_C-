#pragma once

#include "api/Logger.hpp"
#include "core/QueryResult.hpp"
#include "network/ClusterManager.hpp"
#include "network/ClusterMetadata.hpp"
#include "security/AuthManager.hpp"

#include <atomic>
#include <chrono>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace network {

class EntryPointServer {
public:
    EntryPointServer(int port,
                     size_t storage_count,
                     int storage_base_port,
                     std::string data_dir,
                     std::string storage_binary);
    ~EntryPointServer();

    void start();
    void stop();

private:
    void process_client(int client_socket);
    core::QueryResult handle_sql(const std::string& query, SessionCatalog& session, const std::string& current_user);
    core::QueryResult route_statement(const std::string& query, const parser::Statement& stmt, SessionCatalog& session);
    core::QueryResult route_insert(const parser::InsertStatement& stmt, const std::string& db);
    core::QueryResult route_select(const std::string& query, const parser::SelectStatement& stmt, const std::string& db);
    core::QueryResult route_aggregate_select(const parser::SelectStatement& stmt, const std::string& db);
    core::QueryResult broadcast_exec(const std::string& db, const std::string& query, std::vector<core::QueryResult>* partials = nullptr);
    core::QueryResult exec_on_node(const StorageNodeInfo& node, const std::string& db, const std::string& query);

    std::string handle_cluster_command(const std::string& command, const std::string& current_user);
    void replay_schema_to_node(int node_id);

    std::string build_insert_query(const parser::InsertStatement& stmt, const std::vector<size_t>& row_indices) const;
    std::string build_aggregate_query(const parser::SelectStatement& stmt) const;
    std::string token_to_sql(const parser::Token& token) const;
    std::string expression_to_sql(const parser::Expression* expr) const;
    std::vector<std::string> select_headers(const parser::SelectStatement& stmt, const std::string& db) const;
    std::string canonical_row_key(const parser::InsertStatement& stmt, size_t row_index, const std::string& db) const;
    std::string format_number(double value) const;

    void update_telemetry(bool is_error, double duration);
    std::string get_telemetry_report();

    int port_;
    size_t storage_count_;
    int storage_base_port_;
    std::string data_dir_;
    int server_fd_ = -1;
    std::atomic<bool> running_{false};

    ClusterMetadata metadata_;
    ClusterManager cluster_;
    security::AuthManager auth_manager_;
    api::Logger logger_;

    std::mutex telemetry_mutex_;
    std::deque<std::chrono::steady_clock::time_point> request_timestamps_;
    std::deque<std::chrono::steady_clock::time_point> error_timestamps_;
    std::deque<std::pair<std::chrono::steady_clock::time_point, double>> exec_times_;
};

} // namespace network
