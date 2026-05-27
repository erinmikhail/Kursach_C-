#pragma once
#include "parser/Parser.hpp"
#include "catalog/Catalog.hpp"

namespace core {

// Структура для вычисления выражений WHERE (AST)
struct Value {
    bool is_int;
    uint32_t int_val;
    std::string str_val;
};

class Executor {
private:
    catalog::Catalog& catalog_;

    Value eval_expr(parser::Expression* expr, const storage::Record& rec, const parser::TableMetadata& meta);
    bool eval_cond(parser::Expression* expr, const storage::Record& rec, const parser::TableMetadata& meta);

    void execute_revert(const parser::RevertStatement& stmt);
    uint64_t parse_timestamp(const std::string& ts);

public:
    explicit Executor(catalog::Catalog& catalog) : catalog_(catalog) {}
    void execute(const parser::Statement& stmt);

private:
    void execute_create_database(const parser::CreateDatabaseStatement& stmt);
    void execute_drop_database(const parser::DropDatabaseStatement& stmt);
    void execute_use(const parser::UseStatement& stmt);
    void execute_create_table(const parser::CreateTableStatement& stmt);
    void execute_drop_table(const parser::DropTableStatement& stmt);
    void execute_insert(const parser::InsertStatement& stmt);
    void execute_update(const parser::UpdateStatement& stmt);
    void execute_delete(const parser::DeleteStatement& stmt);
    void execute_select(const parser::SelectStatement& stmt);
};

}