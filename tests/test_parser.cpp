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

TEST(ParserTest, CreateDatabaseValid) {
    parser::Lexer lexer("create database analytics;");
    auto tokens = lexer.tokenize();
    auto catalog = setup_test_catalog();
    parser::Parser parser(tokens, catalog);

    auto stmt = parser.parse();
    ASSERT_EQ(stmt->type, parser::StatementType::CREATE_DATABASE);
    auto* create_db = dynamic_cast<parser::CreateDatabaseStatement*>(stmt.get());
    ASSERT_NE(create_db, nullptr);
    EXPECT_EQ(create_db->db_name, "analytics");
}

TEST(ParserTest, CreateTableWithModifiers) {
    parser::Lexer lexer(R"(create table users2 (id int indexed, name string not_null default "guest");)");
    auto tokens = lexer.tokenize();
    auto catalog = setup_test_catalog();
    parser::Parser parser(tokens, catalog);

    auto stmt = parser.parse();
    ASSERT_EQ(stmt->type, parser::StatementType::CREATE_TABLE);
    auto* create_table = dynamic_cast<parser::CreateTableStatement*>(stmt.get());
    ASSERT_NE(create_table, nullptr);
    ASSERT_EQ(create_table->columns.size(), 2);
    EXPECT_EQ(create_table->table_name, "users2");
    EXPECT_TRUE(create_table->columns[0].is_indexed);
    EXPECT_TRUE(create_table->columns[0].is_not_null);
    EXPECT_TRUE(create_table->columns[1].is_not_null);
    EXPECT_EQ(create_table->columns[1].default_value, "guest");
}

TEST(ParserTest, UseDropUpdateDeleteAndAggregatedSelect) {
    auto catalog = setup_test_catalog();

    {
        parser::Lexer lexer("use default_db;");
        auto tokens = lexer.tokenize();
        parser::Parser parser(tokens, catalog);
        auto stmt = parser.parse();
        EXPECT_EQ(stmt->type, parser::StatementType::USE);
    }
    {
        parser::Lexer lexer(R"(update users set name = "Bob" where id == 10;)");
        auto tokens = lexer.tokenize();
        parser::Parser parser(tokens, catalog);
        auto stmt = parser.parse();
        ASSERT_EQ(stmt->type, parser::StatementType::UPDATE);
        auto* update = dynamic_cast<parser::UpdateStatement*>(stmt.get());
        ASSERT_NE(update, nullptr);
        ASSERT_EQ(update->assignments.size(), 1);
        EXPECT_NE(update->where_clause, nullptr);
    }
    {
        parser::Lexer lexer("delete from users where id >= 10;");
        auto tokens = lexer.tokenize();
        parser::Parser parser(tokens, catalog);
        auto stmt = parser.parse();
        ASSERT_EQ(stmt->type, parser::StatementType::DELETE);
        auto* delete_stmt = dynamic_cast<parser::DeleteStatement*>(stmt.get());
        ASSERT_NE(delete_stmt, nullptr);
        EXPECT_NE(delete_stmt->where_clause, nullptr);
    }
    {
        parser::Lexer lexer("select sum(id) as total, count(id) as amount from users;");
        auto tokens = lexer.tokenize();
        parser::Parser parser(tokens, catalog);
        auto stmt = parser.parse();
        ASSERT_EQ(stmt->type, parser::StatementType::SELECT);
        auto* select = dynamic_cast<parser::SelectStatement*>(stmt.get());
        ASSERT_NE(select, nullptr);
        ASSERT_EQ(select->columns.size(), 2);
        EXPECT_EQ(select->columns[0].aggregation, "sum");
        EXPECT_EQ(select->columns[0].alias, "total");
        EXPECT_EQ(select->columns[1].aggregation, "count");
        EXPECT_EQ(select->columns[1].alias, "amount");
    }
}

TEST(ParserTest, MissingSemicolonAndNoActiveDatabaseAreErrors) {
    {
        parser::Lexer lexer("select * from users");
        auto tokens = lexer.tokenize();
        auto catalog = setup_test_catalog();
        parser::Parser parser(tokens, catalog);
        EXPECT_THROW(parser.parse(), std::invalid_argument);
    }
    {
        catalog::Catalog catalog;
        catalog.dropDatabase("default_db");
        parser::Lexer lexer(R"(insert into users value (1, "name");)");
        auto tokens = lexer.tokenize();
        parser::Parser parser(tokens, catalog);
        EXPECT_THROW(parser.parse(), std::invalid_argument);
    }
}
