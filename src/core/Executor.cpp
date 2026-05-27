#include "core/Executor.hpp"
#include "api/ConsoleInterface.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <chrono>
#include <climits>
#include <ctime>

namespace core {

uint64_t current_timestamp_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

uint64_t Executor::parse_timestamp(const std::string& ts) {
    std::tm tm = {};
    int ms = 0;
    sscanf(ts.c_str(), "%4d.%2d.%2d-%2d:%2d:%2d.%3d",
        &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
        &tm.tm_hour, &tm.tm_min, &tm.tm_sec, &ms);
    tm.tm_year -= 1900;
    tm.tm_mon -= 1;
    time_t time = mktime(&tm);
    return (uint64_t)time * 1000 + ms;
}

Value Executor::eval_expr(parser::Expression* expr, const storage::Record& rec, const parser::TableMetadata& meta) {
    if (auto lit = dynamic_cast<parser::LiteralExpr*>(expr)) {
        if (lit->value.type == parser::TokenType::NUMBER) {
            return {true, (uint32_t)std::stoi(lit->value.value), ""};
        } else {
            return {false, 0, lit->value.value};
        }
    }
    if (auto col = dynamic_cast<parser::ColumnExpr*>(expr)) {
        int idx = -1;
        for (size_t i = 0; i < meta.columns.size(); ++i) {
            if (meta.columns[i].name == col->column_name) { idx = i; break; }
        }
        if (idx == -1) throw std::runtime_error("");
        if (meta.columns[idx].type == "int") return {true, rec.values[idx], ""};
        else return {false, 0, catalog_.getStringPool().get_string(rec.values[idx])};
    }
    throw std::runtime_error("");
}

bool Executor::eval_cond(parser::Expression* expr, const storage::Record& rec, const parser::TableMetadata& meta) {
    if (!expr) return true;
    
    if (auto bin = dynamic_cast<parser::BinaryExpr*>(expr)) {
        if (bin->op.value == "and") return eval_cond(bin->left.get(), rec, meta) && eval_cond(bin->right.get(), rec, meta);
        if (bin->op.value == "or") return eval_cond(bin->left.get(), rec, meta) || eval_cond(bin->right.get(), rec, meta);
        
        Value left = eval_expr(bin->left.get(), rec, meta);
        Value right = eval_expr(bin->right.get(), rec, meta);
        
        if (bin->op.value == "like") {
            if (left.is_int || right.is_int) return false;
            std::regex r(right.str_val);
            return std::regex_match(left.str_val, r);
        }
        if (left.is_int != right.is_int) return false;
        
        if (left.is_int) {
            if (bin->op.value == "==") return left.int_val == right.int_val;
            if (bin->op.value == "!=") return left.int_val != right.int_val;
            if (bin->op.value == "<") return left.int_val < right.int_val;
            if (bin->op.value == ">") return left.int_val > right.int_val;
            if (bin->op.value == "<=") return left.int_val <= right.int_val;
            if (bin->op.value == ">=") return left.int_val >= right.int_val;
        } else {
            if (bin->op.value == "==") return left.str_val == right.str_val;
            if (bin->op.value == "!=") return left.str_val != right.str_val;
            if (bin->op.value == "<") return left.str_val < right.str_val;
            if (bin->op.value == ">") return left.str_val > right.str_val;
            if (bin->op.value == "<=") return left.str_val <= right.str_val;
            if (bin->op.value == ">=") return left.str_val >= right.str_val;
        }
    }
    
    if (auto bet = dynamic_cast<parser::BetweenExpr*>(expr)) {
        Value val = eval_expr(bet->value.get(), rec, meta);
        Value low = eval_expr(bet->lower.get(), rec, meta);
        Value high = eval_expr(bet->upper.get(), rec, meta);
        if (val.is_int) return val.int_val >= low.int_val && val.int_val < high.int_val;
        else return val.str_val >= low.str_val && val.str_val < high.str_val;
    }
    return false;
}

void Executor::execute(const parser::Statement& stmt) {
    switch (stmt.type) {
        case parser::StatementType::CREATE_DATABASE: execute_create_database(static_cast<const parser::CreateDatabaseStatement&>(stmt)); break;
        case parser::StatementType::DROP_DATABASE: execute_drop_database(static_cast<const parser::DropDatabaseStatement&>(stmt)); break;
        case parser::StatementType::USE: execute_use(static_cast<const parser::UseStatement&>(stmt)); break;
        case parser::StatementType::CREATE_TABLE: execute_create_table(static_cast<const parser::CreateTableStatement&>(stmt)); break;
        case parser::StatementType::DROP_TABLE: execute_drop_table(static_cast<const parser::DropTableStatement&>(stmt)); break;
        case parser::StatementType::INSERT: execute_insert(static_cast<const parser::InsertStatement&>(stmt)); break;
        case parser::StatementType::UPDATE: execute_update(static_cast<const parser::UpdateStatement&>(stmt)); break;
        case parser::StatementType::DELETE: execute_delete(static_cast<const parser::DeleteStatement&>(stmt)); break;
        case parser::StatementType::SELECT: execute_select(static_cast<const parser::SelectStatement&>(stmt)); break;
        case parser::StatementType::REVERT: execute_revert(static_cast<const parser::RevertStatement&>(stmt)); break;
        default: break;
    }
}

void Executor::execute_create_database(const parser::CreateDatabaseStatement& stmt) { catalog_.createDatabase(stmt.db_name); }
void Executor::execute_drop_database(const parser::DropDatabaseStatement& stmt) { catalog_.dropDatabase(stmt.db_name); }
void Executor::execute_use(const parser::UseStatement& stmt) { catalog_.setActiveDatabase(stmt.db_name); }
void Executor::execute_drop_table(const parser::DropTableStatement& stmt) { catalog_.dropTable(catalog_.getActiveDatabase(), stmt.table_name); }

void Executor::execute_create_table(const parser::CreateTableStatement& stmt) {
    parser::TableMetadata meta;
    meta.db_name = catalog_.getActiveDatabase();
    meta.table_name = stmt.table_name;
    meta.columns = stmt.columns;
    catalog_.createTable(meta);
}

void Executor::execute_insert(const parser::InsertStatement& stmt) {
    auto db = catalog_.getActiveDatabase();
    auto meta = catalog_.getTableMetadata(db, stmt.table_name);
    auto& table = catalog_.getTable(db, stmt.table_name);
    uint64_t now = current_timestamp_ms();

    for (const auto& row_vals : stmt.values) {
        storage::Record rec;
        rec.is_deleted = false;
        rec.ts_start = now;
        rec.ts_end = ULLONG_MAX;
        rec.values.resize(meta.columns.size(), 0);

        for (size_t i = 0; i < meta.columns.size(); ++i) {
            const auto& colDef = meta.columns[i];
            bool provided = false;
            std::string val_str;

            if (stmt.columns.empty()) {
                if (i < row_vals.size()) { provided = true; val_str = row_vals[i].value; }
            } else {
                for (size_t j = 0; j < stmt.columns.size(); ++j) {
                    if (stmt.columns[j] == colDef.name) { provided = true; val_str = row_vals[j].value; break; }
                }
            }

            if (!provided) {
                if (!colDef.default_value.empty()) val_str = colDef.default_value;
                else if (colDef.is_not_null) throw std::runtime_error("");
            }

            if (colDef.type == "int") rec.values[i] = val_str.empty() ? 0 : std::stoi(val_str);
            else rec.values[i] = val_str.empty() ? 0 : catalog_.getStringPool().get_or_add_string(val_str);
        }
        table.insert_record(rec);
    }
}

// MVCC Обновление (старая запись закрывается, новая открывается)
void Executor::execute_update(const parser::UpdateStatement& stmt) {
    auto db = catalog_.getActiveDatabase();
    auto meta = catalog_.getTableMetadata(db, stmt.table_name);
    auto& table = catalog_.getTable(db, stmt.table_name);
    uint64_t now = current_timestamp_ms();

    auto all_recs = table.scan_all();
    for (uint32_t row_id = 0; row_id < all_recs.size(); ++row_id) {
        auto r = all_recs[row_id];
        if (r.is_deleted) continue;
        
        if (eval_cond(stmt.where_clause.get(), r, meta)) {
            int idx_col = table.get_indexed_col();

            if (idx_col != -1) table.remove_from_index(r.values[idx_col], row_id);

            r.is_deleted = true;
            r.ts_end = now;
            table.update_record(row_id, r);

            storage::Record new_r = r;
            new_r.is_deleted = false;
            new_r.ts_start = now;
            new_r.ts_end = ULLONG_MAX;

            for (const auto& assign : stmt.assignments) {
                for (size_t i = 0; i < meta.columns.size(); ++i) {
                    if (meta.columns[i].name == assign.first) {
                        if (meta.columns[i].type == "int") new_r.values[i] = std::stoi(assign.second.value);
                        else new_r.values[i] = catalog_.getStringPool().get_or_add_string(assign.second.value);
                        break;
                    }
                }
            }
            table.insert_record(new_r);
        }
    }
}

void Executor::execute_delete(const parser::DeleteStatement& stmt) {
    auto db = catalog_.getActiveDatabase();
    auto meta = catalog_.getTableMetadata(db, stmt.table_name);
    auto& table = catalog_.getTable(db, stmt.table_name);
    uint64_t now = current_timestamp_ms();

    auto all_recs = table.scan_all();
    for (uint32_t row_id = 0; row_id < all_recs.size(); ++row_id) {
        auto r = all_recs[row_id];
        if (r.is_deleted) continue;

        if (eval_cond(stmt.where_clause.get(), r, meta)) {
            int idx_col = table.get_indexed_col();
            if (idx_col != -1) table.remove_from_index(r.values[idx_col], row_id);
            r.is_deleted = true;
            r.ts_end = now;
            table.update_record(row_id, r);
        }
    }
}

// МАШИНА ВРЕМЕНИ
void Executor::execute_revert(const parser::RevertStatement& stmt) {
    auto db = catalog_.getActiveDatabase();
    auto& table = catalog_.getTable(db, stmt.table_name);
    uint64_t target_ts = parse_timestamp(stmt.timestamp);

    auto all_recs = table.scan_all();
    for (uint32_t row_id = 0; row_id < all_recs.size(); ++row_id) {
        auto r = all_recs[row_id];
        bool changed = false;
        int idx_col = table.get_indexed_col();

        if (r.ts_start > target_ts) {
            if (!r.is_deleted && idx_col != -1) table.remove_from_index(r.values[idx_col], row_id);
            r.is_deleted = true;
            changed = true;
        }
        else if (r.ts_end > target_ts && r.ts_start <= target_ts) {
            if (r.is_deleted && idx_col != -1) table.add_to_index(r.values[idx_col], row_id);
            r.is_deleted = false;
            r.ts_end = ULLONG_MAX;
            changed = true;
        }

        if (changed) table.update_record(row_id, r);
    }
}

void Executor::execute_select(const parser::SelectStatement& stmt) {
    auto db = catalog_.getActiveDatabase();
    auto meta = catalog_.getTableMetadata(db, stmt.table_name);
    auto& table = catalog_.getTable(db, stmt.table_name);

    std::vector<storage::Record> results;
    bool used_index = false;

    if (stmt.where_clause && !stmt.select_all && stmt.columns.size() == 1 && stmt.columns[0].aggregation.empty()) {
        if (auto bin = dynamic_cast<parser::BinaryExpr*>(stmt.where_clause.get())) {
            if (bin->op.value == "==") {
                auto col = dynamic_cast<parser::ColumnExpr*>(bin->left.get());
                auto lit = dynamic_cast<parser::LiteralExpr*>(bin->right.get());
                int idx_col = table.get_indexed_col();
                
                if (idx_col != -1 && col && lit && meta.columns[idx_col].name == col->column_name) {
                    uint32_t search_val = (lit->value.type == parser::TokenType::NUMBER) ?
                        std::stoi(lit->value.value) : catalog_.getStringPool().get_or_add_string(lit->value.value);

                    storage::Record found_rec;
                    if (table.find_by_id(search_val, found_rec) && !found_rec.is_deleted) results.push_back(found_rec);
                    used_index = true;
                }
            }
        }
    }

    if (!used_index) {
        auto all_recs = table.scan_all();
        for (const auto& r : all_recs) {
            if (r.is_deleted) continue;
            if (eval_cond(stmt.where_clause.get(), r, meta)) results.push_back(r);
        }
    }

    std::vector<std::string> headers;
    if (stmt.select_all) for (const auto& c : meta.columns) headers.push_back(c.name);
    else for (const auto& c : stmt.columns) headers.push_back(c.alias.empty() ? c.name : c.alias);

    std::vector<std::vector<std::string>> rows;
    
    if (!stmt.columns.empty() && !stmt.columns[0].aggregation.empty()) {
        std::vector<std::string> row_strs;
        for (const auto& c : stmt.columns) {
            double res = 0;
            int idx = -1;
            for (size_t i = 0; i < meta.columns.size(); ++i) if (meta.columns[i].name == c.name) { idx = i; break; }
            if (c.aggregation == "sum") { for (const auto& r : results) res += r.values[idx]; } 
            else if (c.aggregation == "count") { res = (double)results.size(); } 
            else if (c.aggregation == "avg") { if (!results.empty()) { for (const auto& r : results) res += r.values[idx]; res /= results.size(); } }
            
            std::string res_str = std::to_string(res);
            res_str.erase(res_str.find_last_not_of('0') + 1, std::string::npos);
            if (res_str.back() == '.') res_str.pop_back();
            row_strs.push_back(res_str);
        }
        rows.push_back(row_strs);
    } 
    else {
        for (const auto& r : results) {
            std::vector<std::string> row_strs;
            if (stmt.select_all) {
                for (size_t i = 0; i < meta.columns.size(); ++i) {
                    if (meta.columns[i].type == "int") row_strs.push_back(std::to_string(r.values[i]));
                    else row_strs.push_back(catalog_.getStringPool().get_string(r.values[i]));
                }
            } else {
                for (const auto& c : stmt.columns) {
                    int idx = -1;
                    for (size_t i = 0; i < meta.columns.size(); ++i) if (meta.columns[i].name == c.name) { idx = i; break; }
                    if (idx == -1) continue;
                    if (meta.columns[idx].type == "int") row_strs.push_back(std::to_string(r.values[idx]));
                    else row_strs.push_back(catalog_.getStringPool().get_string(r.values[idx]));
                }
            }
            rows.push_back(row_strs);
        }
    }

    api::JsonFormatter::print(headers, rows);
}

} 