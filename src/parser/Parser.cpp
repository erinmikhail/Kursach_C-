/**
 * @file Parser.cpp
 * @brief Реализация синтаксического анализатора (Parser) для СУБД.
 *
 * Преобразует последовательность токенов, полученную от лексера,
 * в структуру Statement, пригодную для выполнения ядром.
 */

#include "parser/Parser.hpp"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>
#include <climits>

namespace parser {

static std::string to_upper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    return result;
}

Parser::Parser(const std::vector<Token>& tokens) : tokens_(tokens), pos_(0) {}

/**
 * @brief Главный метод синтаксического анализа.
 *
 * Определяет тип запроса (INSERT или SELECT) по первому токену,
 * вызывает соответствующий частный парсер и проверяет корректность
 * завершения запроса (наличие ';' или конца файла).
 *
 * @return Statement Заполненная структура с типом операции и данными.
 * @throws std::invalid_argument При любой синтаксической ошибке.
 */
Statement Parser::parse() {
    if (is_at_end()) {
        throw std::invalid_argument("Пустой запрос");
    }

    Token first = peek();
    if (first.type != TokenType::KEYWORD) {
        throw std::invalid_argument("Запрос должен начинаться с ключевого слова (INSERT или SELECT)");
    }

    std::string keyword = to_upper(first.value);
    Statement statement;

    if (keyword == "INSERT") {
        statement = parse_insert();
    } else if (keyword == "SELECT") {
        statement = parse_select();
    } else {
        throw std::invalid_argument("Неизвестная команда: " + first.value);
    }

    // Проверка завершающего символа ';' (если он есть).
    if (!is_at_end()) {
        Token next = peek();
        if (next.type == TokenType::SYMBOL && next.value == ";") {
            consume();
        } else if (next.type != TokenType::END_OF_FILE) {
            throw std::invalid_argument(
                "Синтаксическая ошибка: ожидался ';' или конец запроса, но получено: " + next.value);
        }
    }

    return statement;
}

Token Parser::consume() {
    if (is_at_end()) {
        throw std::invalid_argument("Синтаксическая ошибка: неожиданный конец запроса");
    }
    return tokens_[pos_++];
}

Token Parser::peek() const {
    if (is_at_end()) {
        throw std::invalid_argument("Синтаксическая ошибка: попытка чтения за пределами запроса");
    }
    return tokens_[pos_];
}

bool Parser::is_at_end() const {
    return pos_ >= tokens_.size() || tokens_[pos_].type == TokenType::END_OF_FILE;
}

/**
 * @brief Разбирает команду INSERT.
 *
 * Ожидает строгую последовательность токенов:
 * 1. Ключевое слово INSERT (уже проверено в parse()).
 * 2. Число (NUMBER) — идентификатор записи.
 * 3. Строка (STRING) — текст для вставки.
 *
 * Заполняет поля insert_id и insert_text в структуре Statement.
 *
 * @return Statement Структура с типом INSERT и данными.
 * @throws std::invalid_argument При нарушении порядка или нехватке токенов.
 */
Statement Parser::parse_insert() {
    consume();

    if (is_at_end()) {
        throw std::invalid_argument("Синтаксическая ошибка в INSERT: ожидался идентификатор (число)");
    }

    Token id_token = consume();
    if (id_token.type != TokenType::NUMBER) {
        throw std::invalid_argument("Синтаксическая ошибка в INSERT: ожидалось число, получено: " + id_token.value);
    }

    uint32_t id;
    try {
        unsigned long parsed = std::stoul(id_token.value);
        if (parsed > UINT32_MAX) {
            throw std::invalid_argument(
                "Синтаксическая ошибка в INSERT: число слишком большое (максимум 4294967295): " + id_token.value);
        }
        id = static_cast<uint32_t>(parsed);
    } catch (const std::exception&) {
        throw std::invalid_argument("Синтаксическая ошибка в INSERT: некорректное число: " + id_token.value);
    }

    if (is_at_end()) {
        throw std::invalid_argument("Синтаксическая ошибка в INSERT: ожидалась строка для вставки");
    }

    Token text_token = consume();
    if (text_token.type != TokenType::STRING) {
        throw std::invalid_argument("Синтаксическая ошибка в INSERT: ожидалась строка, получено: " + text_token.value);
    }

    Statement statement;
    statement.type = StatementType::INSERT;
    statement.insert_id = id;
    statement.insert_text = text_token.value;

    return statement;
}

/**
 * @brief Разбирает команду SELECT * FROM <таблица>.
 *
 * Ожидает последовательность:
 * 1. Ключевое слово SELECT (уже проверено).
 * 2. Символ '*' (SYMBOL со значением "*").
 * 3. Ключевое слово FROM (KEYWORD).
 * 4. Идентификатор таблицы (IDENTIFIER).
 *
 * Устанавливает тип SELECT и флаг select_all = true.
 *
 * @return Statement Структура с типом SELECT.
 * @throws std::invalid_argument При любых отклонениях от грамматики.
 */
Statement Parser::parse_select() {
    consume();

    if (is_at_end()) {
        throw std::invalid_argument("Синтаксическая ошибка в SELECT: ожидалась '*'");
    }

    Token star = consume();
    if (star.type != TokenType::SYMBOL || star.value != "*") {
        throw std::invalid_argument("Синтаксическая ошибка в SELECT: ожидалась '*', получено: " + star.value);
    }

    // FROM (лексер не выделяет его как KEYWORD, ожидаем IDENTIFIER)
    if (is_at_end()) {
        throw std::invalid_argument("Синтаксическая ошибка в SELECT: ожидалось ключевое слово FROM");
    }

    Token from_token = consume();
    if (from_token.type != TokenType::IDENTIFIER || to_upper(from_token.value) != "FROM") {
        throw std::invalid_argument("Синтаксическая ошибка в SELECT: ожидалось FROM, получено: " + from_token.value);
    }

    if (is_at_end()) {
        throw std::invalid_argument("Синтаксическая ошибка в SELECT: ожидалось имя таблицы");
    }

    Token table_token = consume();
    if (table_token.type != TokenType::IDENTIFIER) {
        throw std::invalid_argument("Синтаксическая ошибка в SELECT: некорректное имя таблицы: " + table_token.value);
    }

    Statement statement;
    statement.type = StatementType::SELECT;
    statement.select_all = true;
    return statement;
}

} // namespace parser