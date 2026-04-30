// tests/test_parser.cpp
#include <gtest/gtest.h>
#include "parser/Lexer.hpp"
#include "parser/Parser.hpp"


TEST(ParserTest, InsertValid) {
    parser::Lexer lexer("INSERT 10 'apple'");
    auto tokens = lexer.tokenize();
    parser::Parser parser(tokens);
    auto stmt = parser.parse();
    EXPECT_EQ(stmt.type, parser::StatementType::INSERT);
    EXPECT_EQ(stmt.insert_id, 10);
    EXPECT_EQ(stmt.insert_text, "apple");
}

TEST(ParserTest, InsertMissingString) {
    parser::Lexer lexer("INSERT 5");
    auto tokens = lexer.tokenize();
    parser::Parser parser(tokens);
    EXPECT_THROW(parser.parse(), std::invalid_argument);
}

TEST(ParserTest, SelectAllValid) {
    parser::Lexer lexer("SELECT * FROM mytable");
    auto tokens = lexer.tokenize();
    parser::Parser parser(tokens);
    auto stmt = parser.parse();
    EXPECT_EQ(stmt.type, parser::StatementType::SELECT);
    EXPECT_TRUE(stmt.select_all);
}

TEST(ParserTest, UnknownCommand) {
    parser::Lexer lexer("DROP TABLE x");
    auto tokens = lexer.tokenize();
    parser::Parser parser(tokens);
    EXPECT_THROW(parser.parse(), std::invalid_argument);
}

TEST(ParserTest, InsertCaseInsensitive) {
    parser::Lexer lexer("InSeRt 99 'Orange'");
    auto tokens = lexer.tokenize();
    parser::Parser parser(tokens);
    auto stmt = parser.parse();
    EXPECT_EQ(stmt.type, parser::StatementType::INSERT);
    EXPECT_EQ(stmt.insert_id, 99);
    EXPECT_EQ(stmt.insert_text, "Orange");
}

TEST(ParserTest, SelectAllCaseInsensitive) {
    parser::Lexer lexer("sElEcT * fRoM students");
    auto tokens = lexer.tokenize();
    parser::Parser parser(tokens);
    auto stmt = parser.parse();
    EXPECT_EQ(stmt.type, parser::StatementType::SELECT);
    EXPECT_TRUE(stmt.select_all);
}

TEST(ParserTest, SelectWithSemicolon) {
    parser::Lexer lexer("SELECT * FROM users;");
    auto tokens = lexer.tokenize();
    parser::Parser parser(tokens);
    auto stmt = parser.parse();
    EXPECT_EQ(stmt.type, parser::StatementType::SELECT);
    EXPECT_TRUE(stmt.select_all);
}

TEST(ParserTest, InsertMaxUint32) {
    parser::Lexer lexer("INSERT 4294967295 'max'");
    auto tokens = lexer.tokenize();
    parser::Parser parser(tokens);
    auto stmt = parser.parse();
    EXPECT_EQ(stmt.type, parser::StatementType::INSERT);
    EXPECT_EQ(stmt.insert_id, 4294967295u);
    EXPECT_EQ(stmt.insert_text, "max");
}

TEST(ParserTest, InsertOverflow) {
    parser::Lexer lexer("INSERT 4294967296 'overflow'");
    auto tokens = lexer.tokenize();
    parser::Parser parser(tokens);
    EXPECT_THROW(parser.parse(), std::invalid_argument);
}

TEST(ParserTest, InsertWithExtraTokens) {
    parser::Lexer lexer("INSERT 5 'data' something");
    auto tokens = lexer.tokenize();
    parser::Parser parser(tokens);
    EXPECT_THROW(parser.parse(), std::invalid_argument);
}

TEST(ParserTest, EmptyInput) {
    parser::Lexer lexer("");
    auto tokens = lexer.tokenize();
    parser::Parser parser(tokens);
    EXPECT_THROW(parser.parse(), std::invalid_argument);
}