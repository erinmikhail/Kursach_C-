#include "catalog/Catalog.hpp"
#include "core/Executor.hpp"
#include "network/ClusterMetadata.hpp"
#include "parser/Lexer.hpp"
#include "parser/Parser.hpp"

#include <gtest/gtest.h>
#include <filesystem>

namespace {

std::unique_ptr<parser::Statement> parse_sql(const std::string& sql, parser::ICatalog& catalog) {
    parser::Lexer lexer(sql);
    const auto tokens = lexer.tokenize();
    parser::Parser parser(tokens, catalog);
    return parser.parse();
}

} // namespace

TEST(ClusterMetadataTest, AppliesDdlAndBuildsReplayCommands) {
    network::ClusterMetadata metadata;
    network::SessionCatalog session(metadata);

    auto create_db = parse_sql("create database app;", session);
    metadata.apply_ddl(*create_db, session.getActiveDatabase());
    session.setActiveDatabase("app");

    auto create_table = parse_sql("create table users (id int indexed, name string default \"guest\");", session);
    metadata.apply_ddl(*create_table, session.getActiveDatabase());

    EXPECT_TRUE(metadata.databaseExists("app"));
    EXPECT_TRUE(metadata.tableExists("app", "users"));

    const auto replay = metadata.replay_commands();
    ASSERT_EQ(replay.size(), 2u);
    EXPECT_EQ(replay[0].query, "create database app;");
    EXPECT_EQ(replay[1].db, "app");
    EXPECT_EQ(replay[1].query, "create table users (id int indexed, name string default \"guest\");");
}

TEST(ExecutorStructuredTest, SelectReturnsHeadersAndRows) {
    const std::string dir = "test_cluster_executor_data";
    std::filesystem::remove_all(dir);

    catalog::Catalog catalog(dir);
    core::Executor executor(catalog);

    auto create_table = parse_sql("create table users (id int indexed, name string);", catalog);
    executor.execute_structured(*create_table);
    auto insert = parse_sql("insert into users value (1, \"Ada\"), (2, \"Linus\");", catalog);
    executor.execute_structured(*insert);
    auto select = parse_sql("select * from users;", catalog);
    core::QueryResult result = executor.execute_structured(*select);

    ASSERT_TRUE(result.ok);
    EXPECT_EQ(result.headers, (std::vector<std::string>{"id", "name"}));
    ASSERT_EQ(result.rows.size(), 2u);
    EXPECT_EQ(result.rows[0], (std::vector<std::string>{"1", "Ada"}));
    EXPECT_EQ(result.rows[1], (std::vector<std::string>{"2", "Linus"}));

    std::filesystem::remove_all(dir);
}
