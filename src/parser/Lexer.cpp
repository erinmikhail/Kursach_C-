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
    tokens.reserve(input_.size() / 5); 

    static const std::unordered_set<std::string> keywords = {
        "insert", "select", "from", "where", "update", "delete", "create", "table"
    };

    static const std::unordered_set<char> symbols = {
        '*', ',', ';', '(', ')', '=', '<', '>'
    };

    while (pos_ < input_.size()) {
        skip_whitespace();

        if (pos_ >= input_.size()) {
            break;
        }

        size_t start_pos = pos_;
        char current = peek();

        if (std::isdigit(static_cast<unsigned char>(current)) || 
           (current == '-' && std::isdigit(static_cast<unsigned char>(input_[pos_ + 1])))) {
            
            if (current == '-') advance();
            
            while (std::isdigit(static_cast<unsigned char>(peek()))) {
                advance();
            }
            
            if (peek() == '.') {
                advance();
                while (std::isdigit(static_cast<unsigned char>(peek()))) {
                    advance();
                }
            }
            
            tokens.push_back({TokenType::NUMBER, input_.substr(start_pos, pos_ - start_pos)});
        }

        else if (current == '\'') {
            advance();
            size_t str_start = pos_;
            bool closed = false;

            while (pos_ < input_.size()) {
                if (peek() == '\'') {
                    closed = true;
                    break;
                }
                advance();
            }

            if (!closed) {
                throw std::runtime_error("Unterminated string literal at position " + std::to_string(start_pos));
            }

            std::string str_val = input_.substr(str_start, pos_ - str_start);
            advance();
            
            tokens.push_back({TokenType::STRING, std::move(str_val)});
        }

        else if (std::isalpha(static_cast<unsigned char>(current)) || current == '_') {
            while (pos_ < input_.size() && 
                   (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) {
                advance();
            }

            std::string word = input_.substr(start_pos, pos_ - start_pos);
            
            std::string lower_word = word;
            std::transform(lower_word.begin(), lower_word.end(), lower_word.begin(),
                           [](unsigned char c) { return std::tolower(c); });

            if (keywords.find(lower_word) != keywords.end()) {
                tokens.push_back({TokenType::KEYWORD, std::move(lower_word)});
            } else {
                tokens.push_back({TokenType::IDENTIFIER, std::move(word)});
            }
        }

        else if (symbols.find(current) != symbols.end()) {
            tokens.push_back({TokenType::SYMBOL, std::string(1, advance())});
        }

        else {
            throw std::runtime_error(
                "Unexpected character: '" + std::string(1, current) + "' at position " + std::to_string(pos_)
            );
        }
    }

    tokens.push_back({TokenType::END_OF_FILE, ""});
    return tokens;
}

}