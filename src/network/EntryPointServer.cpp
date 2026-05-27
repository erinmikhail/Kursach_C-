#include "network/EntryPointServer.hpp"
#include "core/QueryRunner.hpp"
#include "network/SocketUtils.hpp"
#include "parser/Lexer.hpp"
#include "parser/Parser.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <netinet/in.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <utility>

namespace network {
namespace {

std::string path_in_dir(const std::string& dir, const std::string& file) {
    if (dir.empty() || dir == ".") return file;
    return (std::filesystem::path(dir) / file).string();
}

std::string trim_newline(std::string value) {
    while (!value.empty() && (value.back() == '\n' || value.back() == '\r')) value.pop_back();
    return value;
}

std::string upper_copy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return value;
}

std::vector<std::string> split_words(const std::string& value) {
    std::stringstream ss(value);
    std::vector<std::string> words;
    std::string word;
    while (ss >> word) words.push_back(word);
    return words;
}

core::QueryResult json_to_result(const nlohmann::json& json) {
    core::QueryResult result;
    result.ok = json.value("ok", false);
    result.error = json.value("error", "");
    result.text = json.value("text", "");
    result.duration_ms = json.value("duration_ms", 0.0);
    if (json.contains("headers")) result.headers = json.at("headers").get<std::vector<std::string>>();
    if (json.contains("rows")) result.rows = json.at("rows").get<std::vector<std::vector<std::string>>>();
    if (!result.ok && result.text.empty() && !result.error.empty()) {
        result.text = "Ошибка: " + result.error + "\n";
    }
    return result;
}

bool is_ddl(parser::StatementType type) {
    return type == parser::StatementType::CREATE_DATABASE ||
           type == parser::StatementType::DROP_DATABASE ||
           type == parser::StatementType::CREATE_TABLE ||
           type == parser::StatementType::DROP_TABLE;
}

bool is_aggregate_select(const parser::SelectStatement& stmt) {
    return !stmt.columns.empty() && !stmt.columns.front().aggregation.empty();
}

} // namespace

EntryPointServer::EntryPointServer(int port,
                                   size_t storage_count,
                                   int storage_base_port,
                                   std::string data_dir,
                                   std::string storage_binary)
    : port_(port),
      storage_count_(storage_count),
      storage_base_port_(storage_base_port),
      data_dir_(std::move(data_dir)),
      cluster_(std::move(storage_binary), path_in_dir(data_dir_, "storage_nodes")),
      auth_manager_(path_in_dir(data_dir_, "auth.bin")),
      logger_(path_in_dir(data_dir_, "entrypoint_access.log")) {
    std::filesystem::create_directories(data_dir_);
    cluster_.set_replay_callback([this](int node_id) { replay_schema_to_node(node_id); });
}

EntryPointServer::~EntryPointServer() {
    stop();
}

void EntryPointServer::stop() {
    running_ = false;
    if (server_fd_ != -1) {
        close(server_fd_);
        server_fd_ = -1;
    }
    cluster_.stop_all();
}

void EntryPointServer::start() {
    cluster_.start_initial(storage_count_, storage_base_port_);

    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) throw std::runtime_error("EntryPoint socket creation failed");

    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(static_cast<uint16_t>(port_));

    if (bind(server_fd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        throw std::runtime_error("EntryPoint bind failed");
    }
    if (listen(server_fd_, 64) < 0) throw std::runtime_error("EntryPoint listen failed");

    running_ = true;
    std::cout << "[EntryPoint] listening on port " << port_ << "\n";

    while (running_) {
        socklen_t addrlen = sizeof(address);
        const int client_socket = accept(server_fd_, reinterpret_cast<sockaddr*>(&address), &addrlen);
        if (client_socket < 0) continue;
        std::thread(&EntryPointServer::process_client, this, client_socket).detach();
    }
}

void EntryPointServer::process_client(int client_socket) {
    SessionCatalog session(metadata_);
    std::string current_user;
    char buffer[8192];

    while (running_) {
        std::memset(buffer, 0, sizeof(buffer));
        const ssize_t valread = read(client_socket, buffer, sizeof(buffer));
        if (valread <= 0) break;

        const std::string query = trim_newline(std::string(buffer, static_cast<size_t>(valread)));
        if (query.empty()) continue;

        if (query == ".exit") {
            send_all(client_socket, "Bye!\n");
            break;
        }

        if (query.rfind("LOGIN ", 0) == 0) {
            std::stringstream ss(query);
            std::string cmd, user, pass;
            ss >> cmd >> user >> pass;
            try {
                const std::string jwt = auth_manager_.login(user, pass);
                send_all(client_socket, "Token: " + jwt + "\n");
            } catch (...) {
                send_all(client_socket, "Login fail\n");
            }
            continue;
        }

        if (query.rfind("AUTH ", 0) == 0) {
            try {
                current_user = auth_manager_.validate_jwt(query.substr(5));
                send_all(client_socket, "OK\n");
            } catch (...) {
                send_all(client_socket, "Auth fail\n");
            }
            continue;
        }

        if (current_user.empty()) {
            send_all(client_socket, "Login required\n");
            continue;
        }

        const auto started = std::chrono::steady_clock::now();
        bool is_error = false;
        std::string response;

        try {
            if (upper_copy(query).rfind("CLUSTER ", 0) == 0) {
                response = handle_cluster_command(query, current_user);
            } else if (query == "TELEMETRY") {
                response = get_telemetry_report();
            } else {
                core::QueryResult result = handle_sql(query, session, current_user);
                is_error = !result.ok;
                response = core::format_query_result(result);
            }
        } catch (const std::exception& e) {
            is_error = true;
            response = std::string("Ошибка: ") + e.what() + "\n";
        }

        const auto finished = std::chrono::steady_clock::now();
        std::chrono::duration<double, std::milli> diff = finished - started;
        update_telemetry(is_error, diff.count());
        logger_.log_request(query, is_error ? "ERROR" : "SUCCESS", diff.count());
        send_all(client_socket, response);
    }
    close(client_socket);
}

core::QueryResult EntryPointServer::handle_sql(const std::string& query, SessionCatalog& session, const std::string& current_user) {
    parser::Lexer lexer(query);
    const auto tokens = lexer.tokenize();
    parser::Parser parser(tokens, session);
    auto stmt = parser.parse();

    if (stmt->type == parser::StatementType::USE) {
        const auto& use_stmt = static_cast<const parser::UseStatement&>(*stmt);
        session.setActiveDatabase(use_stmt.db_name);
        return {};
    }

    if (!auth_manager_.check_permission(current_user, session.getActiveDatabase(), stmt->type)) {
        core::QueryResult denied;
        denied.ok = false;
        denied.error = "Access Denied";
        denied.text = "Access Denied\n";
        return denied;
    }

    return route_statement(query, *stmt, session);
}

core::QueryResult EntryPointServer::route_statement(const std::string& query, const parser::Statement& stmt, SessionCatalog& session) {
    const std::string db = session.getActiveDatabase();
    if (is_ddl(stmt.type)) {
        core::QueryResult result = broadcast_exec(db, query);
        if (result.ok) metadata_.apply_ddl(stmt, db);
        return result;
    }

    switch (stmt.type) {
        case parser::StatementType::INSERT:
            return route_insert(static_cast<const parser::InsertStatement&>(stmt), db);
        case parser::StatementType::SELECT:
            return route_select(query, static_cast<const parser::SelectStatement&>(stmt), db);
        case parser::StatementType::UPDATE:
        case parser::StatementType::DELETE:
        case parser::StatementType::REVERT:
            return broadcast_exec(db, query);
        default:
            return {};
    }
}

core::QueryResult EntryPointServer::route_insert(const parser::InsertStatement& stmt, const std::string& db) {
    const auto nodes = cluster_.live_nodes();
    if (nodes.empty()) {
        core::QueryResult result;
        result.ok = false;
        result.error = "No live storage nodes";
        result.text = "Ошибка: No live storage nodes\n";
        return result;
    }

    std::map<int, std::vector<size_t>> rows_by_node;
    for (size_t row = 0; row < stmt.values.size(); ++row) {
        const auto hash = std::hash<std::string>{}(canonical_row_key(stmt, row, db));
        const auto& node = nodes[hash % nodes.size()];
        rows_by_node[node.id].push_back(row);
    }

    core::QueryResult merged;
    for (const auto& [node_id, rows] : rows_by_node) {
        auto node_it = std::find_if(nodes.begin(), nodes.end(), [node_id](const StorageNodeInfo& node) {
            return node.id == node_id;
        });
        if (node_it == nodes.end()) continue;
        core::QueryResult partial = exec_on_node(*node_it, db, build_insert_query(stmt, rows));
        if (!partial.ok) return partial;
    }
    return merged;
}

core::QueryResult EntryPointServer::route_select(const std::string& query, const parser::SelectStatement& stmt, const std::string& db) {
    if (is_aggregate_select(stmt)) return route_aggregate_select(stmt, db);

    std::vector<core::QueryResult> partials;
    core::QueryResult status = broadcast_exec(db, query, &partials);
    if (!status.ok) return status;

    core::QueryResult merged;
    merged.headers = select_headers(stmt, db);
    for (const auto& partial : partials) {
        if (!partial.headers.empty() && merged.headers.empty()) merged.headers = partial.headers;
        merged.rows.insert(merged.rows.end(), partial.rows.begin(), partial.rows.end());
    }
    return merged;
}

core::QueryResult EntryPointServer::route_aggregate_select(const parser::SelectStatement& stmt, const std::string& db) {
    std::vector<core::QueryResult> partials;
    core::QueryResult status = broadcast_exec(db, build_aggregate_query(stmt), &partials);
    if (!status.ok) return status;

    core::QueryResult merged;
    merged.headers = select_headers(stmt, db);
    std::vector<double> sums(stmt.columns.size(), 0.0);
    std::vector<double> counts(stmt.columns.size(), 0.0);

    for (const auto& partial : partials) {
        if (partial.rows.empty()) continue;
        const auto& row = partial.rows.front();
        size_t pos = 0;
        for (size_t i = 0; i < stmt.columns.size(); ++i) {
            const auto& col = stmt.columns[i];
            if (pos >= row.size()) break;
            if (col.aggregation == "avg") {
                const double partial_sum = std::stod(row[pos++]);
                const double partial_count = pos < row.size() ? std::stod(row[pos++]) : 0.0;
                sums[i] += partial_sum;
                counts[i] += partial_count;
            } else if (col.aggregation == "sum") {
                sums[i] += std::stod(row[pos++]);
            } else if (col.aggregation == "count") {
                counts[i] += std::stod(row[pos++]);
            }
        }
    }

    std::vector<std::string> row;
    for (size_t i = 0; i < stmt.columns.size(); ++i) {
        const auto& col = stmt.columns[i];
        if (col.aggregation == "avg") row.push_back(format_number(counts[i] == 0.0 ? 0.0 : sums[i] / counts[i]));
        else if (col.aggregation == "sum") row.push_back(format_number(sums[i]));
        else if (col.aggregation == "count") row.push_back(format_number(counts[i]));
    }
    merged.rows.push_back(row);
    return merged;
}

core::QueryResult EntryPointServer::broadcast_exec(const std::string& db, const std::string& query, std::vector<core::QueryResult>* partials) {
    const auto nodes = cluster_.live_nodes();
    if (nodes.empty()) {
        core::QueryResult result;
        result.ok = false;
        result.error = "No live storage nodes";
        result.text = "Ошибка: No live storage nodes\n";
        return result;
    }

    core::QueryResult merged;
    for (const auto& node : nodes) {
        core::QueryResult partial = exec_on_node(node, db, query);
        if (!partial.ok) return partial;
        if (partials) partials->push_back(partial);
        if (!partial.text.empty()) merged.text += partial.text;
    }
    return merged;
}

core::QueryResult EntryPointServer::exec_on_node(const StorageNodeInfo& node, const std::string& db, const std::string& query) {
    nlohmann::json response;
    std::string error;
    const nlohmann::json request = {{"type", "exec"}, {"db", db}, {"query", query}};
    if (!cluster_.request(node, request, response, error)) {
        core::QueryResult result;
        result.ok = false;
        result.error = "Storage " + std::to_string(node.id) + ": " + error;
        result.text = "Ошибка: " + result.error + "\n";
        return result;
    }
    return json_to_result(response);
}

std::string EntryPointServer::handle_cluster_command(const std::string& command, const std::string& current_user) {
    if (current_user != "admin") return "Access Denied\n";

    const auto words = split_words(command);
    if (words.size() < 2 || upper_copy(words[0]) != "CLUSTER") return "Ошибка: Unknown cluster command\n";
    const std::string action = upper_copy(words[1]);

    if (action == "STATUS") return cluster_.status_report();
    if (action == "ADD_STORAGE") {
        int port = 0;
        if (words.size() >= 3) port = std::stoi(words[2]);
        const StorageNodeInfo node = cluster_.add_storage(port);
        return "Storage " + std::to_string(node.id) + " added on port " + std::to_string(node.port) + "\n";
    }
    if (action == "REMOVE_STORAGE") {
        if (words.size() < 3) return "Ошибка: Storage id required\n";
        std::string error;
        if (!cluster_.remove_storage(std::stoi(words[2]), error)) return "Ошибка: " + error + "\n";
        return "Storage " + words[2] + " removed\n";
    }
    return "Ошибка: Unknown cluster command\n";
}

void EntryPointServer::replay_schema_to_node(int node_id) {
    for (const auto& command : metadata_.replay_commands()) {
        nlohmann::json response;
        std::string error;
        const nlohmann::json request = {{"type", "exec"}, {"db", command.db}, {"query", command.query}};
        if (!cluster_.request(node_id, request, response, error)) {
            std::cerr << "[EntryPoint] schema replay failed for storage " << node_id << ": " << error << "\n";
            return;
        }
        const core::QueryResult result = json_to_result(response);
        if (!result.ok) {
            std::cerr << "[EntryPoint] schema replay query failed for storage " << node_id << ": " << result.error << "\n";
            return;
        }
    }
}

std::string EntryPointServer::build_insert_query(const parser::InsertStatement& stmt, const std::vector<size_t>& row_indices) const {
    std::ostringstream ss;
    ss << "insert into " << stmt.table_name;
    if (!stmt.columns.empty()) {
        ss << " (";
        for (size_t i = 0; i < stmt.columns.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << stmt.columns[i];
        }
        ss << ")";
    }
    ss << " value ";
    for (size_t i = 0; i < row_indices.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << "(";
        const auto& values = stmt.values[row_indices[i]];
        for (size_t j = 0; j < values.size(); ++j) {
            if (j > 0) ss << ", ";
            ss << token_to_sql(values[j]);
        }
        ss << ")";
    }
    ss << ";";
    return ss.str();
}

std::string EntryPointServer::build_aggregate_query(const parser::SelectStatement& stmt) const {
    std::ostringstream ss;
    ss << "select ";
    bool first = true;
    for (const auto& col : stmt.columns) {
        if (col.aggregation == "avg") {
            if (!first) ss << ", ";
            ss << "sum(" << col.name << "), count(" << col.name << ")";
            first = false;
        } else {
            if (!first) ss << ", ";
            ss << col.aggregation << "(" << col.name << ")";
            first = false;
        }
    }
    ss << " from " << stmt.table_name;
    if (stmt.where_clause) ss << " where " << expression_to_sql(stmt.where_clause.get());
    ss << ";";
    return ss.str();
}

std::string EntryPointServer::token_to_sql(const parser::Token& token) const {
    if (token.type == parser::TokenType::STRING) return "\"" + token.value + "\"";
    return token.value;
}

std::string EntryPointServer::expression_to_sql(const parser::Expression* expr) const {
    if (!expr) return "";
    if (const auto* literal = dynamic_cast<const parser::LiteralExpr*>(expr)) return token_to_sql(literal->value);
    if (const auto* column = dynamic_cast<const parser::ColumnExpr*>(expr)) return column->column_name;
    if (const auto* binary = dynamic_cast<const parser::BinaryExpr*>(expr)) {
        return "(" + expression_to_sql(binary->left.get()) + " " + binary->op.value + " " + expression_to_sql(binary->right.get()) + ")";
    }
    if (const auto* between = dynamic_cast<const parser::BetweenExpr*>(expr)) {
        return "(" + expression_to_sql(between->value.get()) + " between " + expression_to_sql(between->lower.get()) +
               " and " + expression_to_sql(between->upper.get()) + ")";
    }
    return "";
}

std::vector<std::string> EntryPointServer::select_headers(const parser::SelectStatement& stmt, const std::string& db) const {
    std::vector<std::string> headers;
    if (stmt.select_all) {
        const auto meta = metadata_.getTableMetadata(db, stmt.table_name);
        for (const auto& col : meta.columns) headers.push_back(col.name);
    } else {
        for (const auto& col : stmt.columns) headers.push_back(col.alias.empty() ? col.name : col.alias);
    }
    return headers;
}

std::string EntryPointServer::canonical_row_key(const parser::InsertStatement& stmt, size_t row_index, const std::string& db) const {
    std::ostringstream ss;
    ss << db << "." << stmt.table_name << "|";
    for (const auto& col : stmt.columns) ss << col << ",";
    ss << "|";
    for (const auto& token : stmt.values[row_index]) ss << static_cast<int>(token.type) << ":" << token.value << ";";
    return ss.str();
}

std::string EntryPointServer::format_number(double value) const {
    if (std::fabs(value - std::round(value)) < 0.0000001) return std::to_string(static_cast<long long>(std::llround(value)));
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(6) << value;
    std::string out = ss.str();
    while (!out.empty() && out.back() == '0') out.pop_back();
    if (!out.empty() && out.back() == '.') out.pop_back();
    return out.empty() ? "0" : out;
}

void EntryPointServer::update_telemetry(bool is_error, double duration) {
    const auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(telemetry_mutex_);
    request_timestamps_.push_back(now);
    if (is_error) error_timestamps_.push_back(now);
    exec_times_.push_back({now, duration});

    const auto ten_mins_ago = now - std::chrono::minutes(10);
    while (!request_timestamps_.empty() && request_timestamps_.front() < ten_mins_ago) request_timestamps_.pop_front();
    while (!error_timestamps_.empty() && error_timestamps_.front() < ten_mins_ago) error_timestamps_.pop_front();

    const auto ten_secs_ago = now - std::chrono::seconds(10);
    while (!exec_times_.empty() && exec_times_.front().first < ten_secs_ago) exec_times_.pop_front();
}

std::string EntryPointServer::get_telemetry_report() {
    const auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(telemetry_mutex_);

    const auto one_sec_ago = now - std::chrono::seconds(1);
    int current_rps = 0;
    for (auto it = request_timestamps_.rbegin(); it != request_timestamps_.rend(); ++it) {
        if (*it >= one_sec_ago) current_rps++;
        else break;
    }

    const double avg_rps_10m = request_timestamps_.size() / 600.0;
    const auto one_min_ago = now - std::chrono::minutes(1);
    int errors_1m = 0;
    for (auto it = error_timestamps_.rbegin(); it != error_timestamps_.rend(); ++it) {
        if (*it >= one_min_ago) errors_1m++;
        else break;
    }

    double sum_time = 0.0;
    for (const auto& pair : exec_times_) sum_time += pair.second;
    const double avg_time_10s = exec_times_.empty() ? 0.0 : sum_time / exec_times_.size();

    std::ostringstream ss;
    ss << "ТЕЛЕМЕТРИЯ ENTRYPOINT\n"
       << "Текущий RPS (за 1 сек): " << current_rps << "\n"
       << "Средний RPS (за 10 мин): " << std::fixed << std::setprecision(2) << avg_rps_10m << "\n"
       << "Среднее время (за 10 сек): " << avg_time_10s << " ms\n"
       << "Количество ошибок (за 1 мин): " << errors_1m << "\n\n";
    return ss.str();
}

} // namespace network
