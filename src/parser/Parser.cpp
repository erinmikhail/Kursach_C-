#include "parser/Parser.hpp"
#include <climits>

namespace parser {

Parser::Parser(const std::vector<Token>& tokens)
    : tokens_(tokens), pos_(0) {}

Statement Parser::parse() {
    if (is_at_end()) {
        throw std::invalid_argument("Пустой запрос");
    }

    const Token& first = peek();

    if (first.type != TokenType::KEYWORD) {
        throw std::invalid_argument("Запрос должен начинаться с ключевого слова");
    }

    Statement statement;

    if (first.value == "insert") {
        statement = parse_insert();
    } else if (first.value == "select") {
        statement = parse_select();
    } else {
        throw std::invalid_argument("Неизвестная команда: " + first.value);
    }

    if (!is_at_end()) {
        const Token& next = peek();

        if (next.type == TokenType::SYMBOL && next.value == ";") {
            consume();
        } else if (next.type != TokenType::END_OF_FILE) {
            throw std::invalid_argument(
                "Синтаксическая ошибка: ожидался ';' или конец запроса, получено: " + next.value);
        }
    }

    return statement;
}

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

bool Parser::is_at_end() const {
    return pos_ >= tokens_.size() ||
           tokens_[pos_].type == TokenType::END_OF_FILE;
}

Statement Parser::parse_insert() {
    consume();

    if (is_at_end()) {
        throw std::invalid_argument("Синтаксическая ошибка в INSERT: ожидалось число");
    }

    const Token& id_token = consume();

    if (id_token.type != TokenType::NUMBER) {
        throw std::invalid_argument(
            "Синтаксическая ошибка в INSERT: ожидалось число, получено: " + id_token.value);
    }

    uint32_t id = 0;
    try {
        unsigned long parsed = std::stoul(id_token.value);

        if (parsed > UINT32_MAX) {
            throw std::invalid_argument("Слишком большое число: " + id_token.value);
        }

        id = static_cast<uint32_t>(parsed);

    } catch (const std::invalid_argument&) {
        throw std::invalid_argument("Некорректное число: " + id_token.value);

    } catch (const std::out_of_range&) {
        throw std::invalid_argument("Слишком большое число: " + id_token.value);
    }

    if (is_at_end()) {
        throw std::invalid_argument("Синтаксическая ошибка в INSERT: ожидалась строка");
    }

    const Token& text_token = consume();

    if (text_token.type != TokenType::STRING) {
        throw std::invalid_argument(
            "Синтаксическая ошибка в INSERT: ожидалась строка, получено: " + text_token.value);
    }

    Statement statement;
    statement.type = StatementType::INSERT;
    statement.insert_id = id;
    statement.insert_text = text_token.value;

    return statement;
}

Statement Parser::parse_select() {
    consume();

    if (is_at_end()) {
        throw std::invalid_argument("Синтаксическая ошибка в SELECT: ожидалась '*'");
    }

    const Token& star = consume();

    if (star.type != TokenType::SYMBOL || star.value != "*") {
        throw std::invalid_argument(
            "Синтаксическая ошибка в SELECT: ожидалась '*', получено: " + star.value);
    }

    if (is_at_end()) {
        throw std::invalid_argument("Синтаксическая ошибка в SELECT: ожидалось 'from'");
    }

    const Token& from_token = consume();

    if (from_token.type != TokenType::IDENTIFIER || from_token.value != "from") {
        throw std::invalid_argument(
            "Синтаксическая ошибка в SELECT: ожидалось 'from', получено: " + from_token.value);
    }

    if (is_at_end()) {
        throw std::invalid_argument("Синтаксическая ошибка в SELECT: ожидалось имя таблицы");
    }

    const Token& table_token = consume();

    if (table_token.type != TokenType::IDENTIFIER) {
        throw std::invalid_argument(
            "Синтаксическая ошибка в SELECT: некорректное имя таблицы: " + table_token.value);
    }

    Statement statement;
    statement.type = StatementType::SELECT;
    statement.select_all = true;

    return statement;
}

} // namespace parser