#include "parser/Parser.hpp"
#include <climits>
#include <stdexcept>
#include <unordered_set>

namespace parser {

Parser::Parser(const std::vector<Token>& tokens, const ICatalog& catalog)
    : tokens_(tokens), catalog_(catalog), pos_(0) {}

const Token& Parser::consume() {
    if (is_at_end()) {
        throw std::invalid_argument("Синтаксическая ошибка: неожиданный конец запроса");
    }
    return tokens_[pos_++];
}

const Token& Parser::peek() const {
    if (is_at_end()) {
        if (!tokens_.empty() && tokens_.back().type == TokenType::END_OF_FILE) {
            return tokens_.back();
        }
        throw std::invalid_argument("Синтаксическая ошибка: выход за пределы токенов");
    }
    return tokens_[pos_];
}

const Token& Parser::previous() const {
    if (pos_ == 0) {
        throw std::logic_error("Нет предыдущего токена");
    }
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
        if (lit->value.type == TokenType::NUMBER) {
            return "int";
        }
        if (lit->value.type == TokenType::STRING) {
            return "string";
        }
        if (lit->value.type == TokenType::TIMESTAMP) {
            return "timestamp";
        }
    }

    if (auto col = dynamic_cast<const ColumnExpr*>(expr)) {
        const ColumnDef* def = meta.getColumn(col->column_name);
        if (!def) {
            throw std::invalid_argument("Колонка '" + col->column_name + "' не найдена");
        }
        return def->type;
    }

    throw std::invalid_argument("Не удалось определить тип выражения");
}

std::pair<std::string, std::string> Parser::resolve_table_name(const std::string& full_name) {
    size_t dot_pos = full_name.find('.');
    if (dot_pos == std::string::npos) {
        std::string active_db = catalog_.getActiveDatabase();
        if (active_db.empty()) {
            throw std::invalid_argument("Семантическая ошибка: не выбрана активная БД и не указан префикс в '" + full_name + "'");
        }
        return {active_db, full_name};
    }
    return {full_name.substr(0, dot_pos), full_name.substr(dot_pos + 1)};
}

std::unique_ptr<Statement> Parser::parse() {
    if (is_at_end()) {
        throw std::invalid_argument("Пустой запрос");
    }

    const Token& first = peek();

    if (first.type != TokenType::KEYWORD) {
        throw std::invalid_argument("Запрос должен начинаться с команды (SELECT, INSERT, ...)");
    }

    std::unique_ptr<Statement> statement;

    if (first.value == "create") {
        statement = parse_create();
    } else if (first.value == "drop") {
        statement = parse_drop();
    } else if (first.value == "use") {
        statement = parse_use();
    } else if (first.value == "insert") {
        statement = parse_insert();
    } else if (first.value == "update") {
        statement = parse_update();
    } else if (first.value == "delete") {
        statement = parse_delete();
    } else if (first.value == "select") {
        statement = parse_select();
    } else if (first.value == "revert") {
        statement = parse_revert();
    } else {
        throw std::invalid_argument("Неизвестная команда: " + first.value);
    }

    // Финальная точка с запятой
    if (!match(TokenType::SYMBOL, ";")) {
        if (!is_at_end() && peek().type != TokenType::END_OF_FILE) {
             throw std::invalid_argument("Синтаксическая ошибка: ожидался ';' в конце команды");
        }
    }

    return statement;
}


std::unique_ptr<Statement> Parser::parse_insert() {
    consume(); // insert

    auto stmt = std::make_unique<InsertStatement>();

    expect(TokenType::KEYWORD, "into", "INSERT: ожидалось INTO");
    
    if (peek().type != TokenType::IDENTIFIER) {
        throw std::invalid_argument("INSERT: ожидалось имя таблицы");
    }
    stmt->table_name = consume().value;

    auto [db, tbl] = resolve_table_name(stmt->table_name);
    if (!catalog_.tableExists(db, tbl)) {
        throw std::invalid_argument("Семантическая ошибка: таблица '" + stmt->table_name + "' не существует");
    }

    auto meta = catalog_.getTableMetadata(db, tbl);

    if (match(TokenType::SYMBOL, "(")) {
        do {
            if (peek().type != TokenType::IDENTIFIER) {
                throw std::invalid_argument("INSERT: ожидалось имя колонки");
            }

            std::string col = consume().value;

            if (!meta.getColumn(col)) {
                throw std::invalid_argument("Семантическая ошибка: колонка '" + col + "' не найдена");
            }
            stmt->columns.push_back(col);

        } while (match(TokenType::SYMBOL, ","));

        expect(TokenType::SYMBOL, ")", "INSERT: ожидалась ')'");
    }

    expect(TokenType::KEYWORD, "value", "INSERT: ожидалось VALUE");

    // Проверка NOT_NULL колонок
    for (const auto& colDef : meta.columns) {
        bool found = false;
        if (stmt->columns.empty()) {
            // Если колонки не указаны явно, то предполагается, что заполняются все колонки по порядку.
            found = true; 
        } else {
            for (const auto& cName : stmt->columns) {
                if (cName == colDef.name) {
                    found = true;
                    break;
                }
            }
        }

        // Если колонки нет в списке, она NOT_NULL и у неё нет DEFAULT — это ошибка
        if (!found && colDef.is_not_null && colDef.default_value.empty()) {
            throw std::invalid_argument("Семантическая ошибка: колонка '" + colDef.name + "' помечена NOT_NULL и не имеет значения по умолчанию");
        }
    }

    do {
        expect(TokenType::SYMBOL, "(", "INSERT: ожидалось '(' перед списком значений");

        std::vector<Token> row;
        size_t idx = 0;

        do {
            if (peek().type != TokenType::NUMBER &&
                peek().type != TokenType::STRING &&
                peek().type != TokenType::TIMESTAMP) {

                throw std::invalid_argument("INSERT: ожидалось значение");
            }
            const Token& val = consume();

            const ColumnDef* target = stmt->columns.empty() ? (idx < meta.columns.size() ? &meta.columns[idx] : nullptr) 
                                                           : meta.getColumn(stmt->columns[idx]);
            
            if (target) {
                if (target->type == "int" && val.type != TokenType::NUMBER) {
                    throw std::invalid_argument("Тип не совпадает: колонка '" + target->name + "' ожидает число");
                }
                if (target->type == "string" && val.type != TokenType::STRING) {
                    throw std::invalid_argument("Тип не совпадает: колонка '" + target->name + "' ожидает строку");
                }
            }

            row.push_back(val);
            idx++;

        } while (match(TokenType::SYMBOL, ","));
        
        size_t expected = stmt->columns.empty() ? meta.columns.size() : stmt->columns.size();

        if (row.size() != expected) {
            throw std::invalid_argument("Количество значений не совпадает с количеством колонок");
        }
        expect(TokenType::SYMBOL, ")", "INSERT: ожидалась ')'");
        stmt->values.push_back(std::move(row));

    } while (match(TokenType::SYMBOL, ","));

    return stmt;
}

std::unique_ptr<Statement> Parser::parse_select() {
    consume(); // select
    auto stmt = std::make_unique<SelectStatement>();

    if (match(TokenType::SYMBOL, "*")) stmt->select_all = true;
    else {
        do {
            SelectColumn col;
            if (peek().type == TokenType::KEYWORD && (peek().value == "sum" || peek().value == "count" || peek().value == "avg")) {
                col.aggregation = consume().value;
                expect(TokenType::SYMBOL, "(", "Ожидалась '('");
                
                if (peek().type != TokenType::IDENTIFIER) {
                    throw std::invalid_argument("SELECT: ожидалось имя колонки");
                }
                col.name = consume().value;

                expect(TokenType::SYMBOL, ")", "Ожидалась ')'");
            } else {
                if (peek().type != TokenType::IDENTIFIER) {
                    throw std::invalid_argument("SELECT: ожидалось имя колонки");
                }
                col.name = consume().value;
            }
            if (match(TokenType::KEYWORD, "as")) {
                if (peek().type != TokenType::IDENTIFIER) {
                    throw std::invalid_argument("SELECT: ожидался alias");
                }
                col.alias = consume().value;
            }
            
            stmt->columns.push_back(col);
        } while (match(TokenType::SYMBOL, ","));
    }

    expect(TokenType::KEYWORD, "from", "SELECT: ожидалось FROM");

    if (peek().type != TokenType::IDENTIFIER) {
        throw std::invalid_argument("SELECT: ожидалось имя таблицы");
    }
    stmt->table_name = consume().value;

    auto [db, tbl] = resolve_table_name(stmt->table_name);
    if (!catalog_.tableExists(db, tbl)) {
        throw std::invalid_argument("Семантическая ошибка: таблица '" + stmt->table_name + "' не существует");
    }

    auto meta = catalog_.getTableMetadata(db, tbl);

    if (!stmt->select_all) {
        for (const auto& c : stmt->columns) {
            auto* def = meta.getColumn(c.name);
            if (!def) {
                throw std::invalid_argument("Колонка '" + c.name + "' не найдена");
            }
            if ((c.aggregation == "sum" || c.aggregation == "avg") && def->type != "int") {
                throw std::invalid_argument("Агрегат " + c.aggregation + " доступен только для числовых полей");
            }
        }
    }

    if (match(TokenType::KEYWORD, "where")) {
        stmt->where_clause = parse_expression(meta);
    }
    return stmt;
}

std::unique_ptr<Statement> Parser::parse_create() {
    consume(); // create

    if (peek().type != TokenType::KEYWORD) {
        throw std::invalid_argument("CREATE: ожидалось DATABASE или TABLE");
    }
    std::string what = consume().value;

    if (what == "database") {
        auto stmt = std::make_unique<CreateDatabaseStatement>();
        if (peek().type != TokenType::IDENTIFIER) {
            throw std::invalid_argument("CREATE DATABASE: ожидалось имя базы данных");
        }

        stmt->db_name = consume().value;
        if (catalog_.databaseExists(stmt->db_name)) {
            throw std::invalid_argument("Семантическая ошибка: база данных '" + stmt->db_name + "' уже существует");
        }
        
        return stmt;

    } else if (what == "table") {
        auto stmt = std::make_unique<CreateTableStatement>();
        if (peek().type != TokenType::IDENTIFIER) {
            throw std::invalid_argument("CREATE TABLE: ожидалось имя таблицы");
        }

        stmt->table_name = consume().value;
        auto [db, tbl] = resolve_table_name(stmt->table_name);
        if (!catalog_.databaseExists(db)) {
            throw std::invalid_argument(
                "Семантическая ошибка: база данных '" + db + "' не существует"
            );
        }

        if (catalog_.tableExists(db, tbl)) {
            throw std::invalid_argument("Семантическая ошибка: таблица '" + stmt->table_name + "' уже существует");
        }

        expect(TokenType::SYMBOL, "(");
        std::unordered_set<std::string> column_names;

        do {
            ColumnDef col;
            if (peek().type != TokenType::IDENTIFIER) {
                throw std::invalid_argument("CREATE TABLE: ожидалось имя колонки");
            }

            col.name = consume().value;
            if (column_names.count(col.name)) {
                throw std::invalid_argument("Семантическая ошибка: колонка '" + col.name + "' уже существует");
            }

            column_names.insert(col.name);
            if (peek().type != TokenType::KEYWORD) {
                throw std::invalid_argument("CREATE TABLE: ожидался тип колонки");
            }

            col.type = consume().value;
            if (col.type != "int" && col.type != "string") {
                throw std::invalid_argument("CREATE TABLE: неподдерживаемый тип '" + col.type + "'");
            }

            while (!is_at_end()) {
                if (match(TokenType::KEYWORD, "not_null")) {
                    col.is_not_null = true;

                } else if (match(TokenType::KEYWORD, "indexed")) {
                    col.is_indexed = true;
                    col.is_not_null = true;

                } else if (match(TokenType::KEYWORD, "default")) {
                    if (peek().type != TokenType::NUMBER &&
                        peek().type != TokenType::STRING &&
                        peek().type != TokenType::TIMESTAMP) {
                        throw std::invalid_argument("CREATE TABLE: ожидалось значение после DEFAULT");
                    }
                    const Token& val = consume();

                    if (col.type == "int" && val.type != TokenType::NUMBER) {
                        throw std::invalid_argument("DEFAULT для '" + col.name + "' должен быть числом");
                    }

                    if (col.type == "string" && val.type != TokenType::STRING) {
                        throw std::invalid_argument("DEFAULT для '" + col.name + "' должен быть строкой");
                    }
                    col.default_value = val.value;
                } else {
                    break;
                }
            }
            stmt->columns.push_back(col);

        } while (match(TokenType::SYMBOL, ","));
        expect(TokenType::SYMBOL, ")");
        return stmt;
    }
    throw std::invalid_argument("CREATE: ожидалось DATABASE или TABLE");
}

std::unique_ptr<Statement> Parser::parse_drop() {
    consume(); // drop

    if (peek().type != TokenType::KEYWORD) {
        throw std::invalid_argument("DROP: ожидалось DATABASE или TABLE");
    }
    std::string what = consume().value;

    if (what == "database") {
        auto stmt = std::make_unique<DropDatabaseStatement>();
        if (peek().type != TokenType::IDENTIFIER) {
            throw std::invalid_argument("DROP DATABASE: ожидалось имя базы данных");
        }
        stmt->db_name = consume().value;

        if (!catalog_.databaseExists(stmt->db_name)) {
            throw std::invalid_argument("Семантическая ошибка: база данных '" + stmt->db_name + "' не существует");
        }

        return stmt;

    } else if (what == "table") {
        auto stmt = std::make_unique<DropTableStatement>();
        if (peek().type != TokenType::IDENTIFIER) {
            throw std::invalid_argument("DROP TABLE: ожидалось имя таблицы");
        }

        stmt->table_name = consume().value;
        auto [db, tbl] = resolve_table_name(stmt->table_name);

        if (!catalog_.tableExists(db, tbl)) {
            throw std::invalid_argument("Семантическая ошибка: таблица '" + stmt->table_name + "' не существует");
        }

        return stmt;
    }
    throw std::invalid_argument("DROP: ожидалось DATABASE или TABLE");
}

std::unique_ptr<Statement> Parser::parse_use() {
    consume(); // use
    auto stmt = std::make_unique<UseStatement>();

    if (peek().type != TokenType::IDENTIFIER) {
        throw std::invalid_argument("USE: ожидалось имя базы данных");
    }
    stmt->db_name = consume().value;

    if (!catalog_.databaseExists(stmt->db_name)) {
        throw std::invalid_argument("Семантическая ошибка: база данных '" + stmt->db_name + "' не существует");
    }
    return stmt;
}

std::unique_ptr<Statement> Parser::parse_update() {
    consume(); // update
    auto stmt = std::make_unique<UpdateStatement>();
    if (peek().type != TokenType::IDENTIFIER) {
        throw std::invalid_argument("UPDATE: ожидалось имя таблицы");
    }
    stmt->table_name = consume().value;

    // Проверка таблицы
    auto [db, tbl] = resolve_table_name(stmt->table_name);
    if (!catalog_.tableExists(db, tbl)) {
        throw std::invalid_argument("Семантическая ошибка: таблица '" + stmt->table_name + "' не существует");
    }
    auto meta = catalog_.getTableMetadata(db, tbl);

    expect(TokenType::KEYWORD, "set", "UPDATE: ожидалось SET");
    do {
        if (peek().type != TokenType::IDENTIFIER) {
            throw std::invalid_argument("UPDATE: ожидалось имя колонки");
        }
        std::string col_name = consume().value;
        
        // Проверка существования колонки
        const ColumnDef* def = meta.getColumn(col_name);
        if (!def) {
            throw std::invalid_argument("Семантическая ошибка: колонка '" + col_name + "' не найдена в '" + stmt->table_name + "'");
        }

        expect(TokenType::OPERATOR, "=", "UPDATE: ожидалось '='");
        
        if (peek().type != TokenType::NUMBER &&
            peek().type != TokenType::STRING &&
            peek().type != TokenType::TIMESTAMP) {
            throw std::invalid_argument("UPDATE: ожидалось значение");
        }
        const Token& val = consume();

        // Проверка типов
        if (def->type == "int" && val.type != TokenType::NUMBER) {
            throw std::invalid_argument("Семантическая ошибка: колонка '" + col_name + "' ожидает число");
        }

        if (def->type == "string" && val.type != TokenType::STRING) {
            throw std::invalid_argument("Семантическая ошибка: колонка '" + col_name + "' ожидает строку");
        }

        stmt->assignments.push_back({col_name, val});
    } while (match(TokenType::SYMBOL, ","));

    if (match(TokenType::KEYWORD, "where")) {
        stmt->where_clause = parse_expression(meta);
    }

    return stmt;
}

std::unique_ptr<Statement> Parser::parse_delete() {
    consume(); // delete
    auto stmt = std::make_unique<DeleteStatement>();
    expect(TokenType::KEYWORD, "from", "DELETE: ожидалось FROM");

    if (peek().type != TokenType::IDENTIFIER) {
        throw std::invalid_argument("DELETE: ожидалось имя таблицы");
    }
    stmt->table_name = consume().value;

    // Проверка таблицы
    auto [db, tbl] = resolve_table_name(stmt->table_name);
    if (!catalog_.tableExists(db, tbl)) {
        throw std::invalid_argument("Семантическая ошибка: таблица '" + stmt->table_name + "' не существует");
    }

    auto meta = catalog_.getTableMetadata(db, tbl);
    if (match(TokenType::KEYWORD, "where")) {
        stmt->where_clause = parse_expression(meta);
    }

    return stmt;
}

std::unique_ptr<Statement> Parser::parse_revert() {
    consume(); // revert
    auto stmt = std::make_unique<RevertStatement>();

    if (peek().type != TokenType::IDENTIFIER) {
        throw std::invalid_argument("REVERT: ожидалось имя таблицы");
    }
    stmt->table_name = consume().value;

    // Проверка таблицы
    auto [db, tbl] = resolve_table_name(stmt->table_name);
    if (!catalog_.tableExists(db, tbl)) {
        throw std::invalid_argument("Семантическая ошибка: таблица '" + stmt->table_name + "' не существует");
    }

    if (peek().type != TokenType::TIMESTAMP) {
        throw std::invalid_argument("REVERT: ожидался timestamp");
    }

    stmt->timestamp = consume().value;
    return stmt;
}

std::unique_ptr<Expression> Parser::parse_expression(const TableMetadata& meta) {
    auto expr = parse_logical(meta);
    while (match(TokenType::KEYWORD, "or")) {
        Token op = previous();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, parse_logical(meta));
    }
    return expr;
}

std::unique_ptr<Expression> Parser::parse_logical(const TableMetadata& meta) {
    auto expr = parse_comparison(meta);
    while (match(TokenType::KEYWORD, "and")) {
        Token op = previous();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, parse_comparison(meta));
    }
    return expr;
}

std::unique_ptr<Expression> Parser::parse_comparison(const TableMetadata& meta) {
    auto left = parse_primary(meta);

    if (match(TokenType::KEYWORD, "between")) {
        auto low = parse_primary(meta);
        expect(TokenType::KEYWORD, "and");
        auto high = parse_primary(meta);

        std::string left_type = resolve_expression_type(left.get(), meta);
        std::string low_type = resolve_expression_type(low.get(), meta);
        std::string high_type = resolve_expression_type(high.get(), meta);

        if (left_type != low_type || left_type != high_type) {
            throw std::invalid_argument("BETWEEN: типы операндов должны совпадать");
        }

        return std::make_unique<BetweenExpr>(std::move(left), std::move(low),std::move(high));
    }

    if (match(TokenType::KEYWORD, "like")) {
        Token op = previous();

        auto right = parse_primary(meta);

        std::string left_type = resolve_expression_type(left.get(), meta);
        std::string right_type = resolve_expression_type(right.get(), meta);

        if (left_type != "string" || right_type != "string") {
            throw std::invalid_argument("LIKE работает только со строками");
        }
        return std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    }

    if (peek().type == TokenType::OPERATOR) {
        const Token& op = peek();

        if (op.value != "==" &&
            op.value != "!=" &&
            op.value != "<"  &&
            op.value != ">"  &&
            op.value != "<=" &&
            op.value != ">=") {
            throw std::invalid_argument("Недопустимый оператор сравнения: " + op.value);
        }

        auto oper = consume();
        auto right = parse_primary(meta);

        std::string left_type = resolve_expression_type(left.get(), meta);
        std::string right_type = resolve_expression_type(right.get(), meta);

        if (left_type != right_type) {
            throw std::invalid_argument("Семантическая ошибка: сравнение значений разных типов");
        }

        return std::make_unique<BinaryExpr>(std::move(left), oper, std::move(right));
    }

    return left;
}

std::unique_ptr<Expression> Parser::parse_primary(const TableMetadata& meta) {
    if (is_at_end() || peek().type == TokenType::END_OF_FILE) {
        throw std::invalid_argument("Ожидалось выражение");
    }
    const Token& t = consume();

    if (t.type == TokenType::NUMBER || t.type == TokenType::STRING || t.type == TokenType::TIMESTAMP) {
        return std::make_unique<LiteralExpr>(t);
    }

    if (t.type == TokenType::IDENTIFIER) {
        if (!meta.getColumn(t.value)) {
            throw std::invalid_argument("Семантическая ошибка: колонка '" + t.value + "' не найдена в таблице");
        }
        return std::make_unique<ColumnExpr>(t.value);
    }

    if (t.type == TokenType::SYMBOL && t.value == "(") {
        auto expr = parse_expression(meta);
        expect(TokenType::SYMBOL, ")");
        return expr;
    }
    
    throw std::invalid_argument("Ожидалось значение или колонка, встречено: " + t.value);
}

} // namespace parser
