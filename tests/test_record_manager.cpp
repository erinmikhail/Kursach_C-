#include <gtest/gtest.h>
#include "storage/Pager.hpp"
#include "storage/RecordManager.hpp"
#include <cstdio>

class RecordManagerTest : public ::testing::Test {
protected:
    const std::string test_file = "isolated_records_suite.bin";
    void SetUp() override { std::remove(test_file.c_str()); }
    void TearDown() override { std::remove(test_file.c_str()); }
};

TEST_F(RecordManagerTest, AppendAndGet) {
    storage::Pager pager(test_file);
    storage::RecordManager manager(pager);

    storage::Record rec1 = {10, 1}; 
    uint32_t idx = manager.append_record(rec1);

    EXPECT_EQ(idx, 0);
    EXPECT_EQ(manager.get_total_records(), 1);
    EXPECT_EQ(manager.get_record(0).id, 10);
}

TEST_F(RecordManagerTest, PageCrossing) {
    storage::Pager pager(test_file);
    storage::RecordManager manager(pager);

    const uint32_t count = 600;
    for (uint32_t i = 0; i < count; ++i) {
        manager.append_record({i * 10, i}); 
    }

    EXPECT_EQ(manager.get_total_records(), count);
    storage::Record rec = manager.get_record(550);
    EXPECT_EQ(rec.id, 5500);
}