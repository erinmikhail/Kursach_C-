#include "parser/Lexer.hpp"
#include <stdexcept>
#include <cctype>
#include <algorithm>
#include <unordered_set>

namespace parser {

Lexer::Lexer(const std::string& input) : input_(input), pos_(0) {}

char Lexer::peek() const {
    if (pos_ < input_.size()) {
        return input_[pos_];
    }
    return '\0';
}

char Lexer::advance() {
    if (pos_ < input_.size()) {
        return input_[pos_++];
    }
    return '\0';
}

void Lexer::skip_whitespace() {
    while (pos_ < input_.size() && std::isspace(static_cast<unsigned char>(peek()))) {
        advance();
    }
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    static const std::unordered_set<std::string> keywords = {
        "insert", "select"
    };

    static const std::unordered_set<char> symbols = {
        '*', ',', ';', '(', ')'
    };

    while (pos_ < input_.size()) {
        skip_whitespace();

        if (pos_ >= input_.size()) {
            break;
        }

        char current = peek();

        if (std::isdigit(static_cast<unsigned char>(current))) {
            std::string number;
            while (pos_ < input_.size() && std::isdigit(static_cast<unsigned char>(peek()))) {
                number += advance();
            }
            tokens.push_back({TokenType::NUMBER, number});
        }

        else if (current == '\'') {
            advance();
            std::string str;
            bool closed = false;

            while (pos_ < input_.size()) {
                char ch = peek();
                if (ch == '\'') {
                    advance();
                    closed = true;
                    break;
                }
                str += advance();
            }

            if (!closed) {
                throw std::runtime_error("Unterminated string literal: missing closing quote");
            }

            tokens.push_back({TokenType::STRING, str});
        }

        else if (std::isalpha(static_cast<unsigned char>(current)) || current == '_') {
            std::string word;
            while (pos_ < input_.size() && 
                   (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) {
                word += advance();
            }

            std::string lower_word = word;
            std::transform(lower_word.begin(), lower_word.end(), lower_word.begin(),
                           [](unsigned char c) { return std::tolower(c); });

            if (keywords.find(lower_word) != keywords.end()) {
                tokens.push_back({TokenType::KEYWORD, word}); 
            } else {
                tokens.push_back({TokenType::IDENTIFIER, word});
            }
        }

        else if (symbols.find(current) != symbols.end()) {
            std::string sym(1, advance());
            tokens.push_back({TokenType::SYMBOL, sym});
        }

        else {
            throw std::runtime_error(
                std::string("Unexpected character: '") + current + "' at position " + 
                std::to_string(pos_)
            );
        }
    }

    tokens.push_back({TokenType::END_OF_FILE, ""});

    return tokens;
}

}