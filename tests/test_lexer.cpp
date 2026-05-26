#include <gtest/gtest.h>
#include "parser/Lexer.hpp"

TEST(LexerTest, BasicTokens) {
    parser::Lexer lexer("insert into users value (10)");
    auto tokens = lexer.tokenize();
    
    EXPECT_EQ(tokens[0].value, "insert");
    EXPECT_EQ(tokens[0].type, parser::TokenType::KEYWORD);
    EXPECT_EQ(tokens[1].value, "into");
    EXPECT_EQ(tokens[4].value, "(");
    EXPECT_EQ(tokens[5].value, "10");
    EXPECT_EQ(tokens[5].type, parser::TokenType::NUMBER);
}

TEST(LexerTest, StringLiteral) {
    parser::Lexer lexer(R"("test string")");
    auto tokens = lexer.tokenize();
    EXPECT_EQ(tokens[0].value, "test string");
    EXPECT_EQ(tokens[0].type, parser::TokenType::STRING);
}

TEST(LexerTest, Operators) {
    parser::Lexer lexer("== != < > <= >=");
    auto tokens = lexer.tokenize();
    EXPECT_EQ(tokens[0].value, "==");
    EXPECT_EQ(tokens[1].value, "!=");
    EXPECT_EQ(tokens[2].value, "<");
    EXPECT_EQ(tokens[5].value, ">=");
}