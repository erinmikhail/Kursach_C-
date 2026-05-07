#pragma once

#include "parser/Lexer.hpp"
#include <vector>
#include <string>
#include <memory>
#include <stdexcept>

namespace parser {


/**
 * @brief Базовый класс для всех математических и логических выражений.
 */
struct Expression {
    virtual ~Expression() = default;
};

/**
 * @brief Значение (число или строка), например: 42, "Apple".
 */
struct LiteralExpr : public Expression {
    Token value; // Токен NUMBER, STRING или TIMESTAMP
    explicit LiteralExpr(Token val) : value(std::move(val)) {}
};

/**
 * @brief Обращение к колонке, например: id, users.name.
 */
struct ColumnExpr : public Expression {
    std::string column_name;
    explicit ColumnExpr(std::string name) : column_name(std::move(name)) {}
};

/**
 * @brief Бинарная операция (AND, OR, ==, !=, <, >, BETWEEN, LIKE).
 * Формирует ветви дерева.
 */
struct BinaryExpr : public Expression {
    std::unique_ptr<Expression> left;
    Token op; // Оператор
    std::unique_ptr<Expression> right;

    BinaryExpr(std::unique_ptr<Expression> l, Token oper, std::unique_ptr<Expression> r)
        : left(std::move(l)), op(std::move(oper)), right(std::move(r)) {}
};


/**
 * @brief Описание одной колонки при создании таблицы.
 */
struct ColumnDef {
    std::string name;
    std::string type; // "int" или "string"
    bool is_not_null = false;
    bool is_indexed = false;
    std::string default_value = ""; // Значение, если есть модификатор DEFAULT
};

/**
 * @brief Выбранная колонка в SELECT (с поддержкой функций и алиасов).
 */
struct SelectColumn {
    std::string name;
    std::string alias = ""; // Если использовалось AS
    std::string aggregation = ""; // "SUM", "COUNT", "AVG" или пусто
};


enum class StatementType {
    CREATE_DATABASE, DROP_DATABASE, USE,
    CREATE_TABLE, DROP_TABLE,
    INSERT, UPDATE, DELETE, SELECT, REVERT
};

struct Statement {
    StatementType type;
    explicit Statement(StatementType t) : type(t) {}
    virtual ~Statement() = default;
};

struct CreateDatabaseStatement : public Statement {
    std::string db_name;
    CreateDatabaseStatement() : Statement(StatementType::CREATE_DATABASE) {}
};

struct UseStatement : public Statement {
    std::string db_name;
    UseStatement() : Statement(StatementType::USE) {}
};

struct CreateTableStatement : public Statement {
    std::string table_name;
    std::vector<ColumnDef> columns;
    CreateTableStatement() : Statement(StatementType::CREATE_TABLE) {}
};

struct InsertStatement : public Statement {
    std::string table_name;
    std::vector<std::string> columns; // Может быть пустым, если вставляют все
    std::vector<Token> values;
    InsertStatement() : Statement(StatementType::INSERT) {}
};

struct UpdateStatement : public Statement {
    std::string table_name;
    std::string column_to_update;
    Token new_value;
    std::unique_ptr<Expression> where_clause; // Условие
    UpdateStatement() : Statement(StatementType::UPDATE) {}
};

struct DeleteStatement : public Statement {
    std::string table_name;
    std::unique_ptr<Expression> where_clause;
    DeleteStatement() : Statement(StatementType::DELETE) {}
};

struct SelectStatement : public Statement {
    std::string table_name;
    bool select_all = false; // Если SELECT *
    std::vector<SelectColumn> columns;
    std::unique_ptr<Expression> where_clause;
    SelectStatement() : Statement(StatementType::SELECT) {}
};

struct RevertStatement : public Statement {
    std::string table_name;
    std::string timestamp;
    RevertStatement() : Statement(StatementType::REVERT) {}
};


class Parser {
private:
    const std::vector<Token>& tokens_;
    size_t pos_ = 0;

    const Token& consume();
    const Token& peek() const;
    const Token& previous() const;
    bool is_at_end() const;
    bool match(TokenType type, const std::string& val = "");

    // Методы парсинга команд
    std::unique_ptr<Statement> parse_create();
    std::unique_ptr<Statement> parse_drop();
    std::unique_ptr<Statement> parse_use();
    std::unique_ptr<Statement> parse_insert();
    std::unique_ptr<Statement> parse_update();
    std::unique_ptr<Statement> parse_delete();
    std::unique_ptr<Statement> parse_select();
    std::unique_ptr<Statement> parse_revert();

    // Парсинг выражений (WHERE)
    std::unique_ptr<Expression> parse_expression();
    std::unique_ptr<Expression> parse_logical();
    std::unique_ptr<Expression> parse_comparison();
    std::unique_ptr<Expression> parse_primary();

public:
    explicit Parser(const std::vector<Token>& tokens);
    
    // Возвращает умный указатель на базовый класс команды
    std::unique_ptr<Statement> parse();
};

} // namespace parser