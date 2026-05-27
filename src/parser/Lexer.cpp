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
        "insert", "into", "value", "select", "set", "from", "where", "update", "delete", "create", "table",
        "int", "string", "not_null", "indexed", "default", "between", "and", "or", 
        "like", "sum", "count", "avg", "as", "database", "use", "revert"
    };

    static const std::unordered_set<char> symbols = {
        '*', ',', ';', '(', ')', '=', '<', '>', '!'
    };

    while (pos_ < input_.size()) {
        skip_whitespace();

        if (pos_ >= input_.size()) {
            break;
        }

        size_t start_pos = pos_;
        char current = peek();

        if (std::isdigit(static_cast<unsigned char>(current))) {
            bool is_timestamp = false;
            
            while (pos_ < input_.size() && (
                   std::isdigit(static_cast<unsigned char>(peek())) || 
                   peek() == '.' || peek() == '-' || peek() == ':')) {
                
                char next = peek();
                if (next == '-' || next == ':') {
                    is_timestamp = true;
                }
                if (next == '.' && is_timestamp == false) {
                    for (size_t i = pos_ + 1; i < input_.size() && !std::isspace(input_[i]); ++i) {
                        if (input_[i] == '.' || input_[i] == '-' || input_[i] == ':') {
                            is_timestamp = true;
                            break;
                        }
                    }
                }
                advance();
            }
            
            std::string value = input_.substr(start_pos, pos_ - start_pos);
            if (is_timestamp) {
                tokens.push_back({TokenType::TIMESTAMP, std::move(value)});
            } else {
                tokens.push_back({TokenType::NUMBER, std::move(value)});
            }
        }

        else if (current == '"') {
            advance();
            size_t str_start = pos_;
            bool closed = false;

            while (pos_ < input_.size()) {
                if (peek() == '"') {
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
                   (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_' || peek() == '.')) {
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

        else if (symbols.count(current)) {
            std::string op(1, advance());
            char next = peek();

            if (next == '=') {
                if (current == '=' || current == '!' || current == '<' || current == '>') {
                    op += advance();
                    tokens.push_back({TokenType::OPERATOR, std::move(op)});
                    continue;
                }
            }
            
            if (current == '=' || current == '<' || current == '>') {
                tokens.push_back({TokenType::OPERATOR, std::move(op)});
            } else {
                tokens.push_back({TokenType::SYMBOL, std::move(op)});
            }
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