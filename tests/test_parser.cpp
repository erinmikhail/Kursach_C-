// tests/test_parser.cpp
#include <gtest/gtest.h>
#include "parser/Lexer.hpp"
#include "parser/Parser.hpp"


TEST(ParserTest, InsertValid) {
    parser::Lexer lexer("insert 10 'apple'");
    auto tokens = lexer.tokenize();
    parser::Parser parser(tokens);
    auto stmt = parser.parse();
    EXPECT_EQ(stmt.type, parser::StatementType::INSERT);
    EXPECT_EQ(stmt.insert_id, 10);
    EXPECT_EQ(stmt.insert_text, "apple");
}

TEST(ParserTest, InsertWithSemicolon) {
    parser::Lexer lexer("insert 1 'text';");
    auto tokens = lexer.tokenize();
    parser::Parser parser(tokens);
    auto stmt = parser.parse();
    EXPECT_EQ(stmt.type, parser::StatementType::INSERT);
    EXPECT_EQ(stmt.insert_id, 1);
    EXPECT_EQ(stmt.insert_text, "text");
}

TEST(ParserTest, InsertMissingNumber) {
    parser::Lexer lexer("insert 'oops'");
    auto tokens = lexer.tokenize();
    parser::Parser parser(tokens);
    EXPECT_THROW(parser.parse(), std::invalid_argument);
}

TEST(ParserTest, InsertMissingString) {
    parser::Lexer lexer("insert 5");
    auto tokens = lexer.tokenize();
    parser::Parser parser(tokens);
    EXPECT_THROW(parser.parse(), std::invalid_argument);
}

TEST(ParserTest, InsertInvalidNumber) {
    parser::Lexer lexer("insert abc 'text'");
    auto tokens = lexer.tokenize();
    parser::Parser parser(tokens);
    EXPECT_THROW(parser.parse(), std::invalid_argument);
}

TEST(ParserTest, InsertMaxUint32) {
    parser::Lexer lexer("insert 4294967295 'max'");
    auto tokens = lexer.tokenize();
    parser::Parser parser(tokens);
    auto stmt = parser.parse();
    EXPECT_EQ(stmt.insert_id, 4294967295u);
}

TEST(ParserTest, InsertOverflow) {
    parser::Lexer lexer("insert 4294967296 'overflow'");
    auto tokens = lexer.tokenize();
    parser::Parser parser(tokens);
    EXPECT_THROW(parser.parse(), std::invalid_argument);
}

TEST(ParserTest, InsertExtraTokens) {
    parser::Lexer lexer("insert 5 'data' extra");
    auto tokens = lexer.tokenize();
    parser::Parser parser(tokens);
    EXPECT_THROW(parser.parse(), std::invalid_argument);
}

TEST(ParserTest, SelectValid) {
    parser::Lexer lexer("select * from users");
    auto tokens = lexer.tokenize();
    parser::Parser parser(tokens);
    auto stmt = parser.parse();
    EXPECT_EQ(stmt.type, parser::StatementType::SELECT);
    EXPECT_TRUE(stmt.select_all);
}

TEST(ParserTest, SelectWithSemicolon) {
    parser::Lexer lexer("select * from users;");
    auto tokens = lexer.tokenize();
    parser::Parser parser(tokens);
    auto stmt = parser.parse();
    EXPECT_EQ(stmt.type, parser::StatementType::SELECT);
}

TEST(ParserTest, SelectMissingStar) {
    parser::Lexer lexer("select id from users");
    auto tokens = lexer.tokenize();
    parser::Parser parser(tokens);
    EXPECT_THROW(parser.parse(), std::invalid_argument);
}

TEST(ParserTest, SelectMissingFrom) {
    parser::Lexer lexer("select * users");
    auto tokens = lexer.tokenize();
    parser::Parser parser(tokens);
    EXPECT_THROW(parser.parse(), std::invalid_argument);
}

TEST(ParserTest, SelectMissingTable) {
    parser::Lexer lexer("select * from");
    auto tokens = lexer.tokenize();
    parser::Parser parser(tokens);
    EXPECT_THROW(parser.parse(), std::invalid_argument);
}

TEST(ParserTest, UnknownCommand) {
    parser::Lexer lexer("drop table x");
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
