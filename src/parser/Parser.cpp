#include "parser/Parser.hpp"
#include <climits>
#include <stdexcept>

namespace parser {

Parser::Parser(const std::vector<Token>& tokens)
    : tokens_(tokens), pos_(0) {}

const Token& Parser::consume() {
    if (is_at_end()) {
        throw std::invalid_argument("Синтаксическая ошибка: неожиданный конец запроса");
    }
    return tokens_[pos_++];
}

const Token& Parser::peek() const {
    if (is_at_end()) {
        throw std::invalid_argument("Синтаксическая ошибка: выход за пределы токенов");
    }
    return tokens_[pos_];
}

const Token& Parser::previous() const {
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


std::unique_ptr<Statement> Parser::parse() {
    if (is_at_end()) {
        throw std::invalid_argument("Пустой запрос");
    }

    const Token& first = peek();

    if (first.type != TokenType::KEYWORD) {
        throw std::invalid_argument("Запрос должен начинаться с ключевого слова");
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

    // Проверка на завершающую точку с запятой по ТЗ
    if (!is_at_end()) {
        if (match(TokenType::SYMBOL, ";")) {
            // Ок, запрос корректно завершен
        } else if (!is_at_end() && peek().type != TokenType::END_OF_FILE) {
            throw std::invalid_argument("Синтаксическая ошибка: ожидался ';' или конец запроса");
        }
    }

    return statement;
}


std::unique_ptr<Statement> Parser::parse_insert() {
    consume(); // Пропускаем "insert"

    auto stmt = std::make_unique<InsertStatement>();
    
    // TODO: По СЗоП нужно парсить конструкцию: INTO [table_name] (col_1, ...) VALUE (val_1, ...);
    // Пока оставляем твою старую логику чтения id и строки как пример заполнения:

    stmt->table_name = "default_table"; // Временная заглушка

    if (is_at_end()) throw std::invalid_argument("Синтаксическая ошибка в INSERT: ожидалось число");
    
    const Token& id_token = consume();
    if (id_token.type != TokenType::NUMBER) {
        throw std::invalid_argument("Синтаксическая ошибка в INSERT: ожидалось число, получено: " + id_token.value);
    }

    try {
        unsigned long parsed = std::stoul(id_token.value);
        if (parsed > UINT32_MAX) throw std::invalid_argument("Слишком большое число: " + id_token.value);
        // Записываем токен как значение
        stmt->values.push_back(id_token); 
    } catch (...) {
        throw std::invalid_argument("Некорректное число: " + id_token.value);
    }

    if (is_at_end()) throw std::invalid_argument("Синтаксическая ошибка в INSERT: ожидалась строка");
    
    const Token& text_token = consume();
    if (text_token.type != TokenType::STRING) {
        throw std::invalid_argument("Синтаксическая ошибка в INSERT: ожидалась строка");
    }
    stmt->values.push_back(text_token);

    return stmt;
}

std::unique_ptr<Statement> Parser::parse_select() {
    consume(); // Пропускаем "select"

    auto stmt = std::make_unique<SelectStatement>();

    if (is_at_end()) throw std::invalid_argument("Синтаксическая ошибка в SELECT: ожидалась '*'");
    
    const Token& star = consume();
    if (star.type != TokenType::SYMBOL || star.value != "*") {
        // TODO: Добавить парсинг списка конкретных колонок, алиасов (AS) и агрегатных функций
        throw std::invalid_argument("Синтаксическая ошибка в SELECT: ожидалась '*', получено: " + star.value);
    }
    stmt->select_all = true;

    if (is_at_end()) throw std::invalid_argument("Синтаксическая ошибка в SELECT: ожидалось 'from'");
    
    const Token& from_token = consume();
    // Обновили проверку, так как Лексер теперь отдает from как KEYWORD
    if (from_token.type != TokenType::KEYWORD || from_token.value != "from") {
        throw std::invalid_argument("Синтаксическая ошибка в SELECT: ожидалось 'from'");
    }

    if (is_at_end()) throw std::invalid_argument("Синтаксическая ошибка в SELECT: ожидалось имя таблицы");
    
    const Token& table_token = consume();
    if (table_token.type != TokenType::IDENTIFIER) {
        throw std::invalid_argument("Синтаксическая ошибка в SELECT: некорректное имя таблицы");
    }
    stmt->table_name = table_token.value;

    // TODO: Добавить парсинг WHERE
    // if (!is_at_end() && peek().value == "where") {
    //     consume();
    //     stmt->where_clause = parse_expression();
    // }

    return stmt;
}


std::unique_ptr<Statement> Parser::parse_create() {
    // TODO: Реализовать парсинг CREATE DATABASE и CREATE TABLE
    throw std::runtime_error("Парсинг команды CREATE еще не реализован");
}

std::unique_ptr<Statement> Parser::parse_drop() {
    // TODO: Реализовать парсинг DROP DATABASE и DROP TABLE
    throw std::runtime_error("Парсинг команды DROP еще не реализован");
}

std::unique_ptr<Statement> Parser::parse_use() {
    // TODO: Реализовать парсинг USE [database_name]
    throw std::runtime_error("Парсинг команды USE еще не реализован");
}

std::unique_ptr<Statement> Parser::parse_update() {
    // TODO: Реализовать UPDATE [table_name] SET col_1 = val_1 WHERE condition
    throw std::runtime_error("Парсинг команды UPDATE еще не реализован");
}

std::unique_ptr<Statement> Parser::parse_delete() {
    // TODO: Реализовать DELETE FROM [table_name] WHERE condition
    throw std::runtime_error("Парсинг команды DELETE еще не реализован");
}

std::unique_ptr<Statement> Parser::parse_revert() {
    // TODO: Реализовать REVERT [table_name] [timestamp]
    throw std::runtime_error("Парсинг команды REVERT еще не реализован");
}


std::unique_ptr<Expression> Parser::parse_expression() {
    // TODO: Парсинг логического OR
    return parse_logical();
}

std::unique_ptr<Expression> Parser::parse_logical() {
    // TODO: Парсинг логического AND
    return parse_comparison();
}

std::unique_ptr<Expression> Parser::parse_comparison() {
    // TODO: Парсинг операторов ==, !=, <, >, <=, >=, BETWEEN, LIKE
    return parse_primary();
}

std::unique_ptr<Expression> Parser::parse_primary() {
    // TODO: Парсинг колонок (IDENTIFIER), констант (NUMBER, STRING), скобок ( ... )
    throw std::runtime_error("Парсинг выражений (Expression) еще не реализован");
}

} // namespace parser