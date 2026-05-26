#pragma once
#include "parser/Parser.hpp"
#include "storage/Table.hpp"
#include "storage/Pager.hpp"
#include "index/b_star_plus_tree.hpp"
#include <unordered_map>
#include <string>
#include <stdexcept>
#include <memory>

namespace catalog {

struct TableContext {
    std::unique_ptr<storage::Pager> pager;
    std::unique_ptr<db_engine::DbIndex> index;
    std::unique_ptr<storage::Table> table;
};

class Catalog : public parser::ICatalog {
private:
    std::unordered_map<std::string, std::unordered_map<std::string, parser::TableMetadata>> databases_;
    std::string active_database_;

    std::unordered_map<std::string, TableContext> opened_tables_;

public:
    Catalog() {
        databases_["default_db"] = {};
        active_database_ = "default_db";
    }

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
        if (db_it == databases_.end()) {
            throw std::invalid_argument("База данных не найдена: " + db_name);
        }
        auto tbl_it = db_it->second.find(table_name);
        if (tbl_it == db_it->second.end()) {
            throw std::invalid_argument("Таблица не найдена: " + table_name);
        }
        return tbl_it->second;
    }

    std::string getActiveDatabase() const override {
        return active_database_;
    }

    void createTable(const parser::TableMetadata& meta) {
        if (!databaseExists(meta.db_name)) {
            throw std::invalid_argument("База данных '" + meta.db_name + "' не существует");
        }
        databases_[meta.db_name][meta.table_name] = meta;

        openTable(meta.db_name, meta.table_name);
    }

    storage::Table& getTable(const std::string& db_name, const std::string& table_name) {
        std::string key = db_name + "." + table_name;

        if (opened_tables_.find(key) == opened_tables_.end()) {
            if (!tableExists(db_name, table_name)) {
                throw std::runtime_error("Таблица не существует в каталоге: " + table_name);
            }
            openTable(db_name, table_name);
        }
        return *(opened_tables_[key].table);
    }

private:
    void openTable(const std::string& db_name, const std::string& table_name) {
        std::string key = db_name + "." + table_name;
        std::string filename = key + ".bin"; 

        TableContext ctx;
        ctx.pager = std::make_unique<storage::Pager>(filename);

        ctx.index = std::make_unique<db_engine::DbIndex>(5);

        ctx.table = std::make_unique<storage::Table>(
            table_name, 
            *(ctx.pager), 
            *(ctx.index)
        );

        opened_tables_[key] = std::move(ctx);
    }
};

}