#pragma once

#include "parser/Lexer.hpp"
#include <vector>
#include <string>
#include <memory>
#include <stdexcept>

namespace parser {

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
 * @brief Метаданные таблицы для семантической проверки во время парсинга.
 */
struct TableMetadata {
    std::string db_name;
    std::string table_name;
    std::vector<ColumnDef> columns;

    const ColumnDef* getColumn(const std::string& col_name) const {
        for (const auto& col : columns) {
            if (col.name == col_name) return &col;
        }
        return nullptr;
    }
};

/**
 * @brief Интерфейс каталога. Реализуется основным движком БД.
 */
class ICatalog {
public:
    virtual ~ICatalog() = default;
    virtual bool databaseExists(const std::string& db_name) const = 0;
    virtual bool tableExists(const std::string& db_name, const std::string& table_name) const = 0;
    virtual TableMetadata getTableMetadata(const std::string& db_name, const std::string& table_name) const = 0;
    virtual std::string getActiveDatabase() const = 0;
};

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
    Token op;
    std::unique_ptr<Expression> right;

    BinaryExpr(std::unique_ptr<Expression> l, Token oper, std::unique_ptr<Expression> r)
        : left(std::move(l)), op(std::move(oper)), right(std::move(r)) {}
};

// Добавлено: структура для Between
struct BetweenExpr : public Expression {
    std::unique_ptr<Expression> value;
    std::unique_ptr<Expression> lower;
    std::unique_ptr<Expression> upper;

    BetweenExpr(std::unique_ptr<Expression> v, std::unique_ptr<Expression> l, std::unique_ptr<Expression> u)
        : value(std::move(v)), lower(std::move(l)), upper(std::move(u)) {}
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

struct DropDatabaseStatement : public Statement {
    std::string db_name;
    DropDatabaseStatement() : Statement(StatementType::DROP_DATABASE) {}
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

struct DropTableStatement : public Statement {
    std::string table_name;
    DropTableStatement() : Statement(StatementType::DROP_TABLE) {}
};

struct InsertStatement : public Statement {
    std::string table_name;
    std::vector<std::string> columns; 
    std::vector<std::vector<Token>> values; // [[v1, v2], [v3, v4]]
    InsertStatement() : Statement(StatementType::INSERT) {}
};

struct UpdateStatement : public Statement {
    std::string table_name;
    std::vector<std::pair<std::string, Token>> assignments;
    std::unique_ptr<Expression> where_clause;
    UpdateStatement() : Statement(StatementType::UPDATE) {}
};

struct DeleteStatement : public Statement {
    std::string table_name;
    std::unique_ptr<Expression> where_clause;
    DeleteStatement() : Statement(StatementType::DELETE) {}
};

/**
 * @brief Выбранная колонка в SELECT (с поддержкой функций и алиасов).
 */
struct SelectColumn {
    std::string name;
    std::string alias = "";
    std::string aggregation = "";
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
    const ICatalog& catalog_;
    size_t pos_ = 0;

    const Token& consume();
    const Token& peek() const;
    const Token& previous() const;
    bool is_at_end() const;
    bool match(TokenType type, const std::string& val = "");
    void expect(TokenType type, const std::string& val = "", const std::string& error_msg = "");

    // Работа с именами таблиц
    std::pair<std::string, std::string> resolve_table_name(const std::string& full_name);

    std::string resolve_expression_type(const Expression* expr, const TableMetadata& meta) const;

    // Методы парсинга команд
    std::unique_ptr<Statement> parse_create();
    std::unique_ptr<Statement> parse_drop();
    std::unique_ptr<Statement> parse_use();
    std::unique_ptr<Statement> parse_insert();
    std::unique_ptr<Statement> parse_update();
    std::unique_ptr<Statement> parse_delete();
    std::unique_ptr<Statement> parse_select();
    std::unique_ptr<Statement> parse_revert();

    // Парсинг выражений (WHERE), принимает метаданные для проверки колонок
    std::unique_ptr<Expression> parse_expression(const TableMetadata& meta);   // OR
    std::unique_ptr<Expression> parse_logical(const TableMetadata& meta);      // AND
    std::unique_ptr<Expression> parse_comparison(const TableMetadata& meta);   // ==, !=, <, >, BETWEEN, LIKE
    std::unique_ptr<Expression> parse_primary(const TableMetadata& meta);      // Литерал, колонка, скобки

public:
    Parser(const std::vector<Token>& tokens, const ICatalog& catalog);
    
    // Возвращает умный указатель на базовый класс команды
    std::unique_ptr<Statement> parse();
};

} // namespace parser
