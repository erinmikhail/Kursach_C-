#include "network/ClusterMetadata.hpp"

#include <sstream>
#include <stdexcept>

namespace network {
namespace {

std::string escape_string(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    for (char ch : value) {
        if (ch == '"') out += "\\\"";
        else out.push_back(ch);
    }
    return out;
}

} // namespace

ClusterMetadata::ClusterMetadata() {
    databases_["default_db"] = {};
}

bool ClusterMetadata::databaseExists(const std::string& db_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return databases_.find(db_name) != databases_.end();
}

bool ClusterMetadata::tableExists(const std::string& db_name, const std::string& table_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto db_it = databases_.find(db_name);
    if (db_it == databases_.end()) return false;
    return db_it->second.find(table_name) != db_it->second.end();
}

parser::TableMetadata ClusterMetadata::getTableMetadata(const std::string& db_name, const std::string& table_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto db_it = databases_.find(db_name);
    if (db_it == databases_.end()) throw std::invalid_argument("DB not found");
    const auto table_it = db_it->second.find(table_name);
    if (table_it == db_it->second.end()) throw std::invalid_argument("Table not found");
    return table_it->second;
}

void ClusterMetadata::apply_ddl(const parser::Statement& stmt, const std::string& active_db) {
    std::lock_guard<std::mutex> lock(mutex_);
    switch (stmt.type) {
        case parser::StatementType::CREATE_DATABASE: {
            const auto& create = static_cast<const parser::CreateDatabaseStatement&>(stmt);
            if (databases_.find(create.db_name) != databases_.end()) throw std::invalid_argument("DB exists");
            databases_[create.db_name] = {};
            break;
        }
        case parser::StatementType::DROP_DATABASE: {
            const auto& drop = static_cast<const parser::DropDatabaseStatement&>(stmt);
            if (databases_.find(drop.db_name) == databases_.end()) throw std::invalid_argument("DB not found");
            databases_.erase(drop.db_name);
            break;
        }
        case parser::StatementType::CREATE_TABLE: {
            const auto& create = static_cast<const parser::CreateTableStatement&>(stmt);
            auto db_it = databases_.find(active_db);
            if (db_it == databases_.end()) throw std::invalid_argument("DB not found");
            if (db_it->second.find(create.table_name) != db_it->second.end()) throw std::invalid_argument("Table exists");
            parser::TableMetadata meta;
            meta.db_name = active_db;
            meta.table_name = create.table_name;
            meta.columns = create.columns;
            db_it->second[meta.table_name] = meta;
            break;
        }
        case parser::StatementType::DROP_TABLE: {
            const auto& drop = static_cast<const parser::DropTableStatement&>(stmt);
            auto db_it = databases_.find(active_db);
            if (db_it == databases_.end()) throw std::invalid_argument("DB not found");
            if (db_it->second.erase(drop.table_name) == 0) throw std::invalid_argument("Table not found");
            break;
        }
        default:
            break;
    }
}

std::vector<ReplayCommand> ClusterMetadata::replay_commands() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ReplayCommand> commands;
    for (const auto& db_pair : databases_) {
        if (db_pair.first != "default_db") {
            commands.push_back({"default_db", "create database " + db_pair.first + ";"});
        }
    }
    for (const auto& db_pair : databases_) {
        for (const auto& table_pair : db_pair.second) {
            commands.push_back({db_pair.first, create_table_sql(table_pair.second)});
        }
    }
    return commands;
}

std::string ClusterMetadata::create_table_sql(const parser::TableMetadata& meta) const {
    std::ostringstream ss;
    ss << "create table " << meta.table_name << " (";
    for (size_t i = 0; i < meta.columns.size(); ++i) {
        const auto& col = meta.columns[i];
        if (i > 0) ss << ", ";
        ss << col.name << " " << col.type;
        if (col.is_indexed) ss << " indexed";
        else if (col.is_not_null) ss << " not_null";
        if (!col.default_value.empty()) ss << " default " << literal_sql(col.default_value, col.type);
    }
    ss << ");";
    return ss.str();
}

std::string ClusterMetadata::literal_sql(const std::string& value, const std::string& type) const {
    if (type == "string") return "\"" + escape_string(value) + "\"";
    return value;
}

SessionCatalog::SessionCatalog(ClusterMetadata& metadata) : metadata_(metadata) {}

bool SessionCatalog::databaseExists(const std::string& db_name) const {
    return metadata_.databaseExists(db_name);
}

bool SessionCatalog::tableExists(const std::string& db_name, const std::string& table_name) const {
    return metadata_.tableExists(db_name, table_name);
}

parser::TableMetadata SessionCatalog::getTableMetadata(const std::string& db_name, const std::string& table_name) const {
    return metadata_.getTableMetadata(db_name, table_name);
}

std::string SessionCatalog::getActiveDatabase() const {
    return active_database_;
}

void SessionCatalog::setActiveDatabase(const std::string& db_name) {
    if (!metadata_.databaseExists(db_name)) throw std::invalid_argument("DB not found");
    active_database_ = db_name;
}

} // namespace network
