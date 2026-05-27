#include <gtest/gtest.h>
#include "parser/Lexer.hpp"
#include "parser/Parser.hpp"
#include "catalog/Catalog.hpp"

catalog::Catalog setup_test_catalog() {
    catalog::Catalog cat;
    parser::TableMetadata meta;
    meta.db_name = "default_db";
    meta.table_name = "users";
    meta.columns.push_back({"id", "int", false, false, ""});
    meta.columns.push_back({"name", "string", false, false, ""});
    cat.createTable(meta);
    return cat;
}

TEST(ParserTest, InsertValid) {
    parser::Lexer lexer(R"(insert into users value (10, "apple");)");
    auto tokens = lexer.tokenize();
    auto catalog = setup_test_catalog();
    parser::Parser parser(tokens, catalog);
    EXPECT_NO_THROW(parser.parse()); 
}

TEST(ParserTest, InsertWithSemicolon) {
    parser::Lexer lexer(R"(insert into users value (1, "text");)");
    auto tokens = lexer.tokenize();
    auto catalog = setup_test_catalog();
    
    parser::Parser parser(tokens, catalog);
    auto stmt = parser.parse();
    
    EXPECT_EQ(stmt->type, parser::StatementType::INSERT);
}

TEST(ParserTest, InsertSemanticErrorTableNotFound) {
    parser::Lexer lexer(R"(insert into products value (10);)");
    auto tokens = lexer.tokenize();
    auto catalog = setup_test_catalog();
    parser::Parser parser(tokens, catalog);
    EXPECT_THROW(parser.parse(), std::invalid_argument);
}

TEST(ParserTest, InsertSemanticErrorWrongType) {
    parser::Lexer lexer(R"(insert into users value (10, 42);)");
    auto tokens = lexer.tokenize();
    auto catalog = setup_test_catalog();
    parser::Parser parser(tokens, catalog);
    auto stmt = parser.parse();
}

TEST(ParserTest, SelectValid) {
    parser::Lexer lexer(R"(select * from users;)");
    auto tokens = lexer.tokenize();
    auto catalog = setup_test_catalog();
    
    parser::Parser parser(tokens, catalog);
    auto stmt = parser.parse();
    
    EXPECT_EQ(stmt->type, parser::StatementType::SELECT);
}

TEST(ParserTest, SelectWithSemicolon) {
    parser::Lexer lexer(R"(select * from users;)");
    auto tokens = lexer.tokenize();
    auto catalog = setup_test_catalog();
    
    parser::Parser parser(tokens, catalog);
    auto stmt = parser.parse();
    
    EXPECT_EQ(stmt->type, parser::StatementType::SELECT);
}

TEST(ParserTest, SelectMissingStar) {
    parser::Lexer lexer(R"(select id from users;)");
    auto tokens = lexer.tokenize();
    auto catalog = setup_test_catalog();
    parser::Parser parser(tokens, catalog);
    EXPECT_NO_THROW(parser.parse()); 
}

TEST(ParserTest, SelectMissingFrom) {
    parser::Lexer lexer(R"(select * users;)");
    auto tokens = lexer.tokenize();
    auto catalog = setup_test_catalog();
    parser::Parser parser(tokens, catalog);
    EXPECT_THROW(parser.parse(), std::invalid_argument);
}

TEST(ParserTest, SelectMissingTable) {
    parser::Lexer lexer(R"(select * from;)");
    auto tokens = lexer.tokenize();
    auto catalog = setup_test_catalog();
    parser::Parser parser(tokens, catalog);
    EXPECT_THROW(parser.parse(), std::invalid_argument);
}

TEST(ParserTest, UnknownCommand) {
    parser::Lexer lexer(R"(drop x;)"); 
    auto tokens = lexer.tokenize();
    auto catalog = setup_test_catalog();
    parser::Parser parser(tokens, catalog);
    EXPECT_THROW(parser.parse(), std::invalid_argument);
}

TEST(ParserTest, EmptyInput) {
    parser::Lexer lexer("");
    auto tokens = lexer.tokenize();
    auto catalog = setup_test_catalog();
    parser::Parser parser(tokens, catalog);
    EXPECT_THROW(parser.parse(), std::invalid_argument);
}