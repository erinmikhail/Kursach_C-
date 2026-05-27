#include <gtest/gtest.h>
#include "storage/Table.hpp"
#include "storage/Pager.hpp"
#include "index/b_star_plus_tree.hpp"
#include <cstdio>

namespace {

class StorageTableTest : public ::testing::Test {
protected:
    const std::string test_file = "isolated_table_suite.bin";

    void SetUp() override { std::remove(test_file.c_str()); }
    void TearDown() override { std::remove(test_file.c_str()); }

    static storage::Record record(std::initializer_list<uint32_t> values, bool is_deleted = false) {
        storage::Record rec;
        rec.is_deleted = is_deleted;
        rec.values.assign(values.begin(), values.end());
        return rec;
    }
};

} // namespace

TEST_F(StorageTableTest, InsertScanAndFindByIndexedColumn) {
    storage::Pager pager(test_file);
    db_engine::DbIndex index(3);
    storage::Table table("users", pager, index, 2, 0);

    const uint32_t first = table.insert_record(record({10, 100}));
    const uint32_t second = table.insert_record(record({20, 200}));

    EXPECT_EQ(first, 0);
    EXPECT_EQ(second, 1);

    auto rows = table.scan_all();
    ASSERT_EQ(rows.size(), 2);
    EXPECT_EQ(rows[0].values[0], 10);
    EXPECT_EQ(rows[1].values[1], 200);

    storage::Record found;
    ASSERT_TRUE(table.find_by_id(20, found));
    EXPECT_EQ(found.values[1], 200);
    EXPECT_FALSE(table.find_by_id(99, found));
}

TEST_F(StorageTableTest, UpdateAndDeleteRecordAreVisibleInScanAndIndex) {
    storage::Pager pager(test_file);
    db_engine::DbIndex index(3);
    storage::Table table("users", pager, index, 2, 0);

    const uint32_t row_id = table.insert_record(record({10, 100}));
    table.remove_from_index(10, row_id);
    table.update_record(row_id, record({30, 300}));
    table.add_to_index(30, row_id);

    storage::Record found;
    ASSERT_TRUE(table.find_by_id(30, found));
    EXPECT_EQ(found.values[1], 300);
    EXPECT_FALSE(table.find_by_id(10, found));

    table.delete_record(row_id);
    auto rows = table.scan_all();
    ASSERT_EQ(rows.size(), 1);
    EXPECT_TRUE(rows[0].is_deleted);
}

TEST_F(StorageTableTest, IndexedTableRejectsDuplicateKey) {
    storage::Pager pager(test_file);
    db_engine::DbIndex index(3);
    storage::Table table("users", pager, index, 2, 0);

    table.insert_record(record({10, 100}));
    EXPECT_THROW(table.insert_record(record({10, 200})), std::runtime_error);
}

TEST_F(StorageTableTest, TableWithoutIndexAllowsDuplicateValuesAndCannotFindById) {
    storage::Pager pager(test_file);
    db_engine::DbIndex index(3);
    storage::Table table("logs", pager, index, 2, -1);

    table.insert_record(record({10, 100}));
    table.insert_record(record({10, 200}));

    auto rows = table.scan_all();
    ASSERT_EQ(rows.size(), 2);
    EXPECT_EQ(rows[0].values[0], 10);
    EXPECT_EQ(rows[1].values[0], 10);

    storage::Record found;
    EXPECT_FALSE(table.find_by_id(10, found));
}
