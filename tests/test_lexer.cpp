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

TEST(LexerTest, CreateTableKeywordsAndModifiers) {
    parser::Lexer lexer(R"(CREATE TABLE users (id int indexed, name string not_null default "guest");)");
    auto tokens = lexer.tokenize();

    EXPECT_EQ(tokens[0].type, parser::TokenType::KEYWORD);
    EXPECT_EQ(tokens[0].value, "create");
    EXPECT_EQ(tokens[1].value, "table");
    EXPECT_EQ(tokens[2].type, parser::TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[6].value, "indexed");
    EXPECT_EQ(tokens[10].value, "not_null");
    EXPECT_EQ(tokens[11].value, "default");
    EXPECT_EQ(tokens[12].type, parser::TokenType::STRING);
    EXPECT_EQ(tokens.back().type, parser::TokenType::END_OF_FILE);
}

TEST(LexerTest, QualifiedIdentifierAndTimestamp) {
    parser::Lexer lexer(R"(select users.name from users where created == 2024-05-01:12:30;)");
    auto tokens = lexer.tokenize();

    EXPECT_EQ(tokens[1].type, parser::TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[1].value, "users.name");
    EXPECT_EQ(tokens[7].type, parser::TokenType::TIMESTAMP);
    EXPECT_EQ(tokens[7].value, "2024-05-01:12:30");
}

TEST(LexerTest, ReportsUnterminatedStringAndUnexpectedCharacter) {
    EXPECT_THROW(parser::Lexer(R"("unterminated)").tokenize(), std::runtime_error);
    EXPECT_THROW(parser::Lexer("select @ from users;").tokenize(), std::runtime_error);
}
