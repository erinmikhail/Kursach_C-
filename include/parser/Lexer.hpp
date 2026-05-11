#pragma once

#include <string>
#include <vector>

namespace parser {

enum class TokenType {
    KEYWORD,
    IDENTIFIER,
    NUMBER,
    STRING,
    SYMBOL,
    OPERATOR,
    TIMESTAMP,
    INVALID,
    END_OF_FILE
};

struct Token {
    TokenType type;
    std::string value;
};

class Lexer {
private:
    std::string input_;
    size_t pos_ = 0;

    char peek() const;
    char advance();
    void skip_whitespace();

public:
    explicit Lexer(const std::string& input);
    std::vector<Token> tokenize();
};

}