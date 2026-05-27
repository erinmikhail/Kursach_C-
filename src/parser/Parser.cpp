#include "parser/Parser.hpp"
#include <climits>
#include <stdexcept>
#include <unordered_set>

namespace parser {

Parser::Parser(const std::vector<Token>& tokens, const ICatalog& catalog)
    : tokens_(tokens), catalog_(catalog), pos_(0) {}

const Token& Parser::consume() {
    if (is_at_end()) throw std::invalid_argument("Синтаксическая ошибка: неожиданный конец запроса");
    return tokens_[pos_++];
}

const Token& Parser::peek() const {
    if (is_at_end()) {
        if (!tokens_.empty() && tokens_.back().type == TokenType::END_OF_FILE) return tokens_.back();
        throw std::invalid_argument("Синтаксическая ошибка: выход за пределы токенов");
    }
    return tokens_[pos_];
}

const Token& Parser::previous() const {
    if (pos_ == 0) throw std::logic_error("Нет предыдущего токена");
    return tokens_[pos_ - 1];
}

bool Parser::is_at_end() const {
    return pos_ >= tokens_.size() || tokens_[pos_].type == TokenType::END_OF_FILE;
}

bool Parser::match(TokenType type, const std::string& val) {
    if (is_at_end()) return false;
    const Token& curr = peek();
    if (curr.type == type && (val.empty() || curr.value == val)) {
        consume();
        return true;
    }
    return false;
}

void Parser::expect(TokenType type, const std::string& val, const std::string& error_msg) {
    if (!match(type, val)) {
        throw std::invalid_argument(error_msg.empty() ? ("Ожидался '" + val + "'") : error_msg);
    }
}

std::string Parser::resolve_expression_type(const Expression* expr, const TableMetadata& meta) const {
    if (auto lit = dynamic_cast<const LiteralExpr*>(expr)) {
        if (lit->value.type == TokenType::NUMBER) return "int";
        if (lit->value.type == TokenType::STRING) return "string";
    }
    if (auto col = dynamic_cast<const ColumnExpr*>(expr)) {
        const ColumnDef* def = meta.getColumn(col->column_name);
        if (!def) throw std::invalid_argument("Колонка '" + col->column_name + "' не найдена");
        return def->type;
    }
    throw std::invalid_argument("Не удалось определить тип выражения");
}

std::pair<std::string, std::string> Parser::resolve_table_name(const std::string& full_name) {
    size_t dot_pos = full_name.find('.');
    if (dot_pos == std::string::npos) {
        std::string active_db = catalog_.getActiveDatabase();
        if (active_db.empty()) throw std::invalid_argument("Не выбрана активная БД");
        return {active_db, full_name};
    }
    return {full_name.substr(0, dot_pos), full_name.substr(dot_pos + 1)};
}

std::unique_ptr<Statement> Parser::parse() {
    if (is_at_end()) throw std::invalid_argument("Пустой запрос");
    const Token& first = peek();
    std::unique_ptr<Statement> statement;
    if (first.value == "create") statement = parse_create();
    else if (first.value == "drop") statement = parse_drop();
    else if (first.value == "use") statement = parse_use();
    else if (first.value == "insert") statement = parse_insert();
    else if (first.value == "update") statement = parse_update();
    else if (first.value == "delete") statement = parse_delete();
    else if (first.value == "select") statement = parse_select();
    else throw std::invalid_argument("Неизвестная команда: " + first.value);
    
    if (!match(TokenType::SYMBOL, ";")) {
         throw std::invalid_argument("Синтаксическая ошибка: ожидался ';' в конце команды");
    }
    return statement;
}

std::unique_ptr<Statement> Parser::parse_create() {
    consume();
    std::string what = consume().value;
    if (what == "database") {
        auto stmt = std::make_unique<CreateDatabaseStatement>();
        stmt->db_name = consume().value;
        return stmt;
    } else if (what == "table") {
        auto stmt = std::make_unique<CreateTableStatement>();
        stmt->table_name = consume().value;
        expect(TokenType::SYMBOL, "(");
        do {
            ColumnDef col;
            col.name = consume().value;
            col.type = consume().value;
            while (!match(TokenType::SYMBOL, ",") && !match(TokenType::SYMBOL, ")")) {
                std::string mod = consume().value;
                if (mod == "indexed") { col.is_indexed = true; col.is_not_null = true; }
                else if (mod == "not_null") { col.is_not_null = true; }
                else if (mod == "default") { col.default_value = consume().value; }
            }
            stmt->columns.push_back(col);
            if (previous().value == ")") break;
        } while (true);
        return stmt;
    }
    throw std::invalid_argument("Ожидалось DATABASE или TABLE");
}

std::unique_ptr<Statement> Parser::parse_insert() {
    consume(); expect(TokenType::KEYWORD, "into");
    auto stmt = std::make_unique<InsertStatement>();
    stmt->table_name = consume().value;

    if (!catalog_.tableExists(catalog_.getActiveDatabase(), stmt->table_name)) {
        throw std::invalid_argument("Table not found");
    }
    
    if (match(TokenType::SYMBOL, "(")) {
        do { stmt->columns.push_back(consume().value); } while (match(TokenType::SYMBOL, ","));
        expect(TokenType::SYMBOL, ")");
    }
    expect(TokenType::KEYWORD, "value");
    do {
        expect(TokenType::SYMBOL, "(");
        std::vector<Token> row;
        do { row.push_back(consume()); } while (match(TokenType::SYMBOL, ","));
        expect(TokenType::SYMBOL, ")");
        stmt->values.push_back(row);
    } while (match(TokenType::SYMBOL, ","));
    return stmt;
}

std::unique_ptr<Statement> Parser::parse_select() {
    consume(); auto stmt = std::make_unique<SelectStatement>();
    if (match(TokenType::SYMBOL, "*")) stmt->select_all = true;
    else {
        do {
            SelectColumn col;
            std::string val = consume().value;
            if (val == "sum" || val == "count" || val == "avg") {
                col.aggregation = val;
                expect(TokenType::SYMBOL, "(");
                col.name = consume().value;
                expect(TokenType::SYMBOL, ")");
            } else {
                col.name = val;
            }
            if (match(TokenType::KEYWORD, "as")) col.alias = consume().value;
            stmt->columns.push_back(col);
        } while (match(TokenType::SYMBOL, ","));
    }
    expect(TokenType::KEYWORD, "from");
    stmt->table_name = consume().value;
    if (match(TokenType::KEYWORD, "where")) stmt->where_clause = parse_expression(TableMetadata());
    return stmt;
}

std::unique_ptr<Statement> Parser::parse_update() {
    consume(); auto stmt = std::make_unique<UpdateStatement>();
    stmt->table_name = consume().value;
    expect(TokenType::KEYWORD, "set");
    do {
        std::string col = consume().value;
        expect(TokenType::OPERATOR, "=");
        stmt->assignments.push_back({col, consume()});
    } while (match(TokenType::SYMBOL, ","));
    if (match(TokenType::KEYWORD, "where")) stmt->where_clause = parse_expression(TableMetadata());
    return stmt;
}

std::unique_ptr<Statement> Parser::parse_delete() {
    consume(); expect(TokenType::KEYWORD, "from");
    auto stmt = std::make_unique<DeleteStatement>();
    stmt->table_name = consume().value;
    if (match(TokenType::KEYWORD, "where")) stmt->where_clause = parse_expression(TableMetadata());
    return stmt;
}

std::unique_ptr<Statement> Parser::parse_drop() {
    consume(); consume(); // drop table/db
    auto stmt = std::make_unique<DropTableStatement>();
    stmt->table_name = consume().value;
    return stmt;
}

std::unique_ptr<Statement> Parser::parse_use() {
    consume(); auto stmt = std::make_unique<UseStatement>();
    stmt->db_name = consume().value;
    return stmt;
}

std::unique_ptr<Expression> Parser::parse_expression(const TableMetadata& meta) {
    auto left = parse_logical(meta);
    while (match(TokenType::KEYWORD, "or")) {
        Token op = previous();
        auto right = parse_logical(meta);
        left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    }
    return left;
}

std::unique_ptr<Expression> Parser::parse_logical(const TableMetadata& meta) {
    auto left = parse_comparison(meta);
    while (match(TokenType::KEYWORD, "and")) {
        Token op = previous();
        auto right = parse_comparison(meta);
        left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    }
    return left;
}

std::unique_ptr<Expression> Parser::parse_comparison(const TableMetadata& meta) {
    auto left = parse_primary(meta);
    if (peek().type == TokenType::OPERATOR) {
        auto op = consume();
        auto right = parse_primary(meta);
        return std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    }
    return left;
}

std::unique_ptr<Expression> Parser::parse_primary(const TableMetadata& meta) {
    if (match(TokenType::SYMBOL, "(")) {
        auto expr = parse_expression(meta);
        expect(TokenType::SYMBOL, ")");
        return expr;
    }
    const Token& t = consume();
    if (t.type == TokenType::NUMBER || t.type == TokenType::STRING) return std::make_unique<LiteralExpr>(t);
    return std::make_unique<ColumnExpr>(t.value);
}

std::unique_ptr<Statement> Parser::parse_revert() { return nullptr; }

}