#pragma once

#include "parser/Parser.hpp"

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace network {

struct ReplayCommand {
    std::string db;
    std::string query;
};

class ClusterMetadata {
public:
    ClusterMetadata();

    bool databaseExists(const std::string& db_name) const;
    bool tableExists(const std::string& db_name, const std::string& table_name) const;
    parser::TableMetadata getTableMetadata(const std::string& db_name, const std::string& table_name) const;

    void apply_ddl(const parser::Statement& stmt, const std::string& active_db);
    std::vector<ReplayCommand> replay_commands() const;

private:
    std::string create_table_sql(const parser::TableMetadata& meta) const;
    std::string literal_sql(const std::string& value, const std::string& type) const;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::unordered_map<std::string, parser::TableMetadata>> databases_;
};

class SessionCatalog : public parser::ICatalog {
public:
    explicit SessionCatalog(ClusterMetadata& metadata);

    bool databaseExists(const std::string& db_name) const override;
    bool tableExists(const std::string& db_name, const std::string& table_name) const override;
    parser::TableMetadata getTableMetadata(const std::string& db_name, const std::string& table_name) const override;
    std::string getActiveDatabase() const override;

    void setActiveDatabase(const std::string& db_name);

private:
    ClusterMetadata& metadata_;
    std::string active_database_ = "default_db";
};

} // namespace network
