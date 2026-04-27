#pragma once

#include "parser/Lexer.hpp"
#include <vector>
#include <string>
#include <memory>
#include <stdexcept>

namespace parser {

enum class StatementType {
    INSERT,
    SELECT,
    UNKNOWN
};


 //Базовая структура распарсенной команды.
 //В будущем можно расширить до полноценного абстрактного синтаксического дерева (AST).

struct Statement {
    StatementType type = StatementType::UNKNOWN;

    // Поля для INSERT
    uint32_t insert_id = 0;
    std::string insert_text;

    // Поля для SELECT (в текущей реализации предполагается SELECT * FROM table)
    bool select_all = false;
};

    //Синтаксический анализатор запросов.
    //Принимает вектор токенов, проверяет их порядок на соответствие грамматике СУБД
    //и формирует промежуточное представление (Statement) для выполнения движком.
class Parser {
private:
    std::vector<Token> tokens_; ///< Вектор токенов, полученный от лексера
    size_t pos_ = 0;            ///< Текущий обрабатываемый токен


    //Возвращает текущий токен и сдвигает указатель вперед.

    Token consume();

    //Возвращает текущий токен без сдвига указателя.
    Token peek() const;

    //Проверяет, не закончились ли токены.
    bool is_at_end() const;

    // Специфичные парсеры для каждой команды
    Statement parse_insert();
    Statement parse_select();

public:
    //Конструктор парсера.
    //tokens Массив токенов для анализа.
    explicit Parser(const std::vector<Token>& tokens);

    //Выполняет синтаксический анализ последовательности токенов.
    //return Statement Структура, содержащая распарсенные данные запроса.
    //@throws std::invalid_argument В случае синтаксической ошибки в запросе.

    Statement parse();
};

}