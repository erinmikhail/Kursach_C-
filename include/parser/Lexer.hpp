#pragma once

#include <string>
#include <vector>

namespace parser {

enum class TokenType {
    KEYWORD,    ///< Ключевые слова SQL (INSERT, SELECT)
    IDENTIFIER, ///< Имена таблиц, колонок (например, users, id)
    NUMBER,     ///< Числовые значения (например, 42)
    STRING,     ///< Строковые значения в кавычках (например, 'Apple')
    SYMBOL,     ///< Спецсимволы (звездочка *, скобки, запятая, точка с запятой ;)
    INVALID,    ///< Неизвестный символ (ошибка лексера)
    END_OF_FILE ///< Конец строки запроса
};

//Структура, описывающая один токен (лексическую единицу).

struct Token {
    TokenType type;       ///< Тип токена
    std::string value;    ///< Текстовое значение токена (например, "42" или "INSERT")
};

    //Лексический анализатор. 
    //Разбивает сырую строку SQL-запроса на последовательность токенов.
class Lexer {
private:
    std::string input_; ///< Исходная строка запроса
    size_t pos_ = 0;    ///< Текущая позиция чтения

    // Вспомогательные методы (рекомендуется реализовать в .cpp)
    char peek() const;
    char advance();
    void skip_whitespace();

public:
    //Конструктор лексера.
    // input Строка с запросом пользователя.
    explicit Lexer(const std::string& input);

    //Запускает процесс токенизации.
    // return std::vector<Token> Массив извлеченных токенов.
    //throws std::runtime_error Если встречается недопустимый символ или незакрытая кавычка.
    std::vector<Token> tokenize();
};

}