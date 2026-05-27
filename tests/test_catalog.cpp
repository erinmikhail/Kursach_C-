#include <gtest/gtest.h>
#include "catalog/Catalog.hpp"
#include <cstdio>
#include <fstream>

namespace {

class CatalogTest : public ::testing::Test {
protected:
    void SetUp() override { cleanup(); }
    void TearDown() override { cleanup(); }

    static void cleanup() {
        std::remove("global_string_pool.bin");
        std::remove("default_db.users_catalog.bin");
        std::remove("default_db.users_catalog.idx");
        std::remove("analytics.events.bin");
        std::remove("analytics.events.idx");
    }

    static parser::TableMetadata users_meta(const std::string& db = "default_db") {
        parser::TableMetadata meta;
        meta.db_name = db;
        meta.table_name = "users_catalog";
        meta.columns.push_back({"id", "int", false, true, ""});
        meta.columns.push_back({"name", "string", false, false, ""});
        return meta;
    }
};

} // namespace

TEST_F(CatalogTest, DefaultDatabaseExistsAndCanSwitchDatabases) {
    catalog::Catalog catalog;

    EXPECT_TRUE(catalog.databaseExists("default_db"));
    EXPECT_EQ(catalog.getActiveDatabase(), "default_db");

    catalog.createDatabase("analytics");
    EXPECT_TRUE(catalog.databaseExists("analytics"));
    catalog.setActiveDatabase("analytics");
    EXPECT_EQ(catalog.getActiveDatabase(), "analytics");

    EXPECT_THROW(catalog.createDatabase("analytics"), std::invalid_argument);
    EXPECT_THROW(catalog.setActiveDatabase("missing"), std::invalid_argument);
}

TEST_F(CatalogTest, CreateTableStoresMetadataAndOpensTable) {
    catalog::Catalog catalog;
    auto meta = users_meta();

    catalog.createTable(meta);

    EXPECT_TRUE(catalog.tableExists("default_db", "users_catalog"));
    const auto stored = catalog.getTableMetadata("default_db", "users_catalog");
    ASSERT_EQ(stored.columns.size(), 2);
    EXPECT_EQ(stored.columns[0].name, "id");
    EXPECT_TRUE(stored.columns[0].is_indexed);
    EXPECT_EQ(catalog.getTable("default_db", "users_catalog").get_name(), "users_catalog");

    EXPECT_THROW(catalog.createTable(meta), std::invalid_argument);
    EXPECT_THROW(catalog.getTableMetadata("default_db", "missing"), std::invalid_argument);
}

TEST_F(CatalogTest, DropTableRemovesMetadataAndFiles) {
    catalog::Catalog catalog;
    auto meta = users_meta();
    catalog.createTable(meta);
    {
        std::ofstream bin("default_db.users_catalog.bin");
        std::ofstream idx("default_db.users_catalog.idx");
    }

    catalog.dropTable("default_db", "users_catalog");

    EXPECT_FALSE(catalog.tableExists("default_db", "users_catalog"));
    EXPECT_FALSE(std::ifstream("default_db.users_catalog.bin").good());
    EXPECT_FALSE(std::ifstream("default_db.users_catalog.idx").good());
    EXPECT_THROW(catalog.dropTable("default_db", "users_catalog"), std::invalid_argument);
}

TEST_F(CatalogTest, DropDatabaseDropsContainedTablesAndClearsActiveWhenNeeded) {
    catalog::Catalog catalog;
    catalog.createDatabase("analytics");

    auto meta = users_meta("analytics");
    meta.table_name = "events";
    catalog.createTable(meta);
    catalog.setActiveDatabase("analytics");

    catalog.dropDatabase("analytics");

    EXPECT_FALSE(catalog.databaseExists("analytics"));
    EXPECT_EQ(catalog.getActiveDatabase(), "");
    EXPECT_THROW(catalog.dropDatabase("analytics"), std::invalid_argument);
}
