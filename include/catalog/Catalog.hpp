#pragma once
#include "parser/Parser.hpp"
#include "storage/Table.hpp"
#include "storage/Pager.hpp"
#include "storage/StringPool.hpp"
#include "index/b_star_plus_tree.hpp" 
#include <unordered_map>
#include <string>
#include <stdexcept>
#include <memory>
#include <filesystem>

namespace catalog {

// Контекст для удержания активных компонентов таблицы в памяти
struct TableContext {
    std::unique_ptr<storage::Pager> pager;
    std::unique_ptr<db_engine::DbIndex> index;
    std::unique_ptr<storage::Table> table;
};

// Главный реестр СУБД, управляющий файлами, таблицами и глобальным пулом строк
class Catalog : public parser::ICatalog {
private:
    std::unordered_map<std::string, std::unordered_map<std::string, parser::TableMetadata>> databases_;
    std::string active_database_;
    std::unordered_map<std::string, TableContext> opened_tables_;
    storage::StringPool string_pool_; 

public:
    Catalog() : string_pool_("global_string_pool.bin") {
        databases_["default_db"] = {};
        active_database_ = "default_db";
    }

    storage::StringPool& getStringPool() { return string_pool_; }

    bool databaseExists(const std::string& db_name) const override {
        return databases_.find(db_name) != databases_.end();
    }

    bool tableExists(const std::string& db_name, const std::string& table_name) const override {
        auto db_it = databases_.find(db_name);
        if (db_it == databases_.end()) return false;
        return db_it->second.find(table_name) != db_it->second.end();
    }

    parser::TableMetadata getTableMetadata(const std::string& db_name, const std::string& table_name) const override {
        auto db_it = databases_.find(db_name);
        if (db_it == databases_.end()) throw std::invalid_argument("DB not found");
        auto tbl_it = db_it->second.find(table_name);
        if (tbl_it == db_it->second.end()) throw std::invalid_argument("Table not found");
        return tbl_it->second;
    }

    std::string getActiveDatabase() const override { return active_database_; }

    void createDatabase(const std::string& db_name) {
        if (databaseExists(db_name)) throw std::invalid_argument("DB exists");
        databases_[db_name] = {};
    }

    // Физически удаляет все таблицы внутри базы, а затем и саму базу
    void dropDatabase(const std::string& db_name) {
        if (!databaseExists(db_name)) throw std::invalid_argument("DB not found");
        
        auto tables_copy = databases_[db_name];
        for (const auto& pair : tables_copy) {
            dropTable(db_name, pair.first);
        }
        
        databases_.erase(db_name);
        if (active_database_ == db_name) active_database_ = "";
    }

    void setActiveDatabase(const std::string& db_name) {
        if (!databaseExists(db_name)) throw std::invalid_argument("DB not found");
        active_database_ = db_name;
    }

    void createTable(const parser::TableMetadata& meta) {
        if (!databaseExists(meta.db_name)) throw std::invalid_argument("DB not found");
        if (tableExists(meta.db_name, meta.table_name)) throw std::invalid_argument("Table exists");
        databases_[meta.db_name][meta.table_name] = meta;
        openTable(meta.db_name, meta.table_name);
    }

    // Удаляет таблицу из метаданных и стирает файлы .bin и .idx с диска
    void dropTable(const std::string& db_name, const std::string& table_name) {
        if (!tableExists(db_name, table_name)) throw std::invalid_argument("Table not found");
        
        std::string key = db_name + "." + table_name;
        opened_tables_.erase(key);
        databases_[db_name].erase(table_name);
        
        std::filesystem::remove(key + ".bin");
        std::filesystem::remove(key + ".idx");
    }

    storage::Table& getTable(const std::string& db_name, const std::string& table_name) {
        std::string key = db_name + "." + table_name;
        if (opened_tables_.find(key) == opened_tables_.end()) {
            if (!tableExists(db_name, table_name)) throw std::runtime_error("Table not found");
            openTable(db_name, table_name);
        }
        return *(opened_tables_[key].table);
    }

private:
    void openTable(const std::string& db_name, const std::string& table_name) {
        std::string key = db_name + "." + table_name;
        std::string filename = key + ".bin";

        auto meta = getTableMetadata(db_name, table_name);
        int indexed_col = -1;
        for (size_t i = 0; i < meta.columns.size(); ++i) {
            if (meta.columns[i].is_indexed) {
                indexed_col = i;
                break;
            }
        }

        TableContext ctx;
        ctx.pager = std::make_unique<storage::Pager>(filename);
        ctx.index = std::make_unique<db_engine::DbIndex>(5);
        ctx.table = std::make_unique<storage::Table>(
            table_name, 
            *(ctx.pager), 
            *(ctx.index), 
            meta.columns.size(), 
            indexed_col
        );

        opened_tables_[key] = std::move(ctx);
    }
};

} // namespace catalog