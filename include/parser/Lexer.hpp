#pragma once

#include <string>
#include <vector>

namespace parser {

enum class TokenType {
    KEYWORD,    ///< Ключевые слова SQL (INSERT, SELECT)
    IDENTIFIER, ///< Имена таблиц, колонок (например, users, id)
    NUMBER,     ///< Числовые значения (например, 42)
    STRING,     ///< Строковые значения в кавычках (например, "Apple")
    SYMBOL,     ///< Спецсимволы (звездочка *, скобки, запятая, точка с запятой ;)
    OPERATOR,   ///< Операторы сравнения (==, !=, <=, >=, <, >, =)
    TIMESTAMP,  ///< Временная метка для REVERT (yyyy.mm.dd-hh:mm:ss.msmsms)
    INVALID,    ///< Неизвестный символ (ошибка лексера)
    END_OF_FILE ///< Конец строки запроса
};

/**
 * @brief Структура, описывающая один токен (лексическую единицу).
 */
struct Token {
    TokenType type;       ///< Тип токена
    std::string value;    ///< Текстовое значение токена
};

/**
 * @class Lexer
 * @brief Лексический анализатор. 
 * Разбивает сырую строку SQL-запроса на последовательность токенов.
 */
class Lexer {
private:
    std::string input_; ///< Исходная строка запроса
    size_t pos_ = 0;    ///< Текущая позиция чтения

    char peek() const;
    char advance();
    void skip_whitespace();

public:
    explicit Lexer(const std::string& input);
    std::vector<Token> tokenize();
};

} // namespace parser