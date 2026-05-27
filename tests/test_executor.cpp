#include <gtest/gtest.h>
#include "catalog/Catalog.hpp"
#include "core/Executor.hpp"
#include "parser/Lexer.hpp"
#include "parser/Parser.hpp"
#include <cstdio>
#include <sstream>

namespace {

class ExecutorIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override { cleanup(); }
    void TearDown() override { cleanup(); }

    static void cleanup() {
        std::remove("global_string_pool.bin");
        std::remove("default_db.people_exec.bin");
        std::remove("default_db.people_exec.idx");
        std::remove("default_db.metrics_exec.bin");
        std::remove("default_db.metrics_exec.idx");
    }

    catalog::Catalog catalog;
    core::Executor executor{catalog};

    void run_sql(const std::string& sql) {
        parser::Lexer lexer(sql);
        const auto tokens = lexer.tokenize();
        parser::Parser parser(tokens, catalog);
        auto stmt = parser.parse();
        executor.execute(*stmt);
    }

    std::string run_sql_capture_stdout(const std::string& sql) {
        std::ostringstream buffer;
        auto* old = std::cout.rdbuf(buffer.rdbuf());
        run_sql(sql);
        std::cout.rdbuf(old);
        return buffer.str();
    }

    static std::unique_ptr<parser::Expression> eq_expr(const std::string& col, parser::Token token) {
        return std::make_unique<parser::BinaryExpr>(
            std::make_unique<parser::ColumnExpr>(col),
            parser::Token{parser::TokenType::OPERATOR, "=="},
            std::make_unique<parser::LiteralExpr>(std::move(token)));
    }
};

} // namespace

TEST_F(ExecutorIntegrationTest, CreateInsertSelectUsesDefaultsAndJsonOutput) {
    run_sql(R"(create table people_exec (id int indexed, age int default 18, name string default "Anon");)");
    run_sql(R"(insert into people_exec (id, name) value (1, "Alice"), (2, "Bob");)");

    const std::string output = run_sql_capture_stdout("select * from people_exec;");

    EXPECT_NE(output.find(R"("id": 1)"), std::string::npos);
    EXPECT_NE(output.find(R"("age": 18)"), std::string::npos);
    EXPECT_NE(output.find(R"("name": "Alice")"), std::string::npos);
    EXPECT_NE(output.find(R"("name": "Bob")"), std::string::npos);
}

TEST_F(ExecutorIntegrationTest, NotNullAndIndexedColumnRejectInvalidRows) {
    run_sql(R"(create table people_exec (id int indexed, name string not_null);)");
    run_sql(R"(insert into people_exec value (1, "Alice");)");

    EXPECT_THROW(run_sql("insert into people_exec (id) value (2);"), std::runtime_error);
    EXPECT_THROW(run_sql(R"(insert into people_exec value (1, "Duplicate");)"), std::runtime_error);
}

TEST_F(ExecutorIntegrationTest, UpdateAndDeleteWithWhereMaintainTableState) {
    run_sql(R"(create table people_exec (id int indexed, age int, name string);)");
    run_sql(R"(insert into people_exec value (1, 20, "Alice"), (2, 30, "Bob"), (3, 40, "Carol");)");
    run_sql(R"(update people_exec set age = 31, name = "Bobby" where id == 2;)");
    run_sql("delete from people_exec where age > 35;");

    const std::string output = run_sql_capture_stdout("select * from people_exec;");

    EXPECT_NE(output.find(R"("name": "Alice")"), std::string::npos);
    EXPECT_NE(output.find(R"("name": "Bobby")"), std::string::npos);
    EXPECT_EQ(output.find(R"("name": "Carol")"), std::string::npos);
    EXPECT_NE(output.find(R"("age": 31)"), std::string::npos);
}

TEST_F(ExecutorIntegrationTest, AggregationsReturnExpectedValues) {
    run_sql("create table metrics_exec (id int indexed, value int);");
    run_sql("insert into metrics_exec value (1, 10), (2, 20), (3, 30);");

    const std::string output = run_sql_capture_stdout("select sum(value) as total, count(id) as amount, avg(value) as average from metrics_exec;");

    EXPECT_NE(output.find(R"("total": 60)"), std::string::npos);
    EXPECT_NE(output.find(R"("amount": 3)"), std::string::npos);
    EXPECT_NE(output.find(R"("average": 20)"), std::string::npos);
}

TEST_F(ExecutorIntegrationTest, ManuallyBuiltWhereSupportsLikeAndLogicalOr) {
    run_sql(R"(create table people_exec (id int indexed, age int, name string);)");
    run_sql(R"(insert into people_exec value (1, 20, "Alice"), (2, 30, "Bob"), (3, 40, "Carol");)");

    parser::SelectStatement select;
    select.table_name = "people_exec";
    select.select_all = true;
    auto like_alice = std::make_unique<parser::BinaryExpr>(
        std::make_unique<parser::ColumnExpr>("name"),
        parser::Token{parser::TokenType::KEYWORD, "like"},
        std::make_unique<parser::LiteralExpr>(parser::Token{parser::TokenType::STRING, "A.*"}));
    auto id_three = eq_expr("id", parser::Token{parser::TokenType::NUMBER, "3"});
    select.where_clause = std::make_unique<parser::BinaryExpr>(
        std::move(like_alice),
        parser::Token{parser::TokenType::KEYWORD, "or"},
        std::move(id_three));

    std::ostringstream buffer;
    auto* old = std::cout.rdbuf(buffer.rdbuf());
    executor.execute(select);
    std::cout.rdbuf(old);

    const std::string output = buffer.str();
    EXPECT_NE(output.find(R"("name": "Alice")"), std::string::npos);
    EXPECT_NE(output.find(R"("name": "Carol")"), std::string::npos);
    EXPECT_EQ(output.find(R"("name": "Bob")"), std::string::npos);
}

TEST_F(ExecutorIntegrationTest, ManuallyBuiltWhereSupportsBetweenExpression) {
    run_sql(R"(create table people_exec (id int indexed, age int, name string);)");
    run_sql(R"(insert into people_exec value (1, 20, "Alice"), (2, 30, "Bob"), (3, 40, "Carol");)");

    parser::SelectStatement select;
    select.table_name = "people_exec";
    select.select_all = true;
    select.where_clause = std::make_unique<parser::BetweenExpr>(
        std::make_unique<parser::ColumnExpr>("age"),
        std::make_unique<parser::LiteralExpr>(parser::Token{parser::TokenType::NUMBER, "25"}),
        std::make_unique<parser::LiteralExpr>(parser::Token{parser::TokenType::NUMBER, "40"}));

    std::ostringstream buffer;
    auto* old = std::cout.rdbuf(buffer.rdbuf());
    executor.execute(select);
    std::cout.rdbuf(old);

    const std::string output = buffer.str();
    EXPECT_EQ(output.find(R"("name": "Alice")"), std::string::npos);
    EXPECT_NE(output.find(R"("name": "Bob")"), std::string::npos);
    EXPECT_EQ(output.find(R"("name": "Carol")"), std::string::npos);
}
