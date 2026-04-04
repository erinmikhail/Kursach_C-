#include <gtest/gtest.h>
#include "storage/Pager.hpp"
#include "storage/RecordManager.hpp"
#include <cstdio>

class RecordManagerTest : public ::testing::Test {
protected:
    const std::string test_file = "test_records.bin";

    void SetUp() override {
        std::remove(test_file.c_str());
    }

    void TearDown() override {
        std::remove(test_file.c_str());
    }
};

TEST_F(RecordManagerTest, AppendAndGet) {
    storage::Pager pager(test_file);
    storage::RecordManager manager(pager);

    EXPECT_EQ(manager.get_total_records(), 0);

    storage::Record rec1 = {10, 1}; 
    storage::Record rec2 = {20, 2};

    uint32_t idx1 = manager.append_record(rec1);
    uint32_t idx2 = manager.append_record(rec2);

    EXPECT_EQ(idx1, 0);
    EXPECT_EQ(idx2, 1);
    EXPECT_EQ(manager.get_total_records(), 2);

    storage::Record read_rec1 = manager.get_record(idx1);
    EXPECT_EQ(read_rec1.id, 10);
    EXPECT_EQ(read_rec1.name_id, 1);
}

TEST_F(RecordManagerTest, PageCrossing) {
    storage::Pager pager(test_file);
    storage::RecordManager manager(pager);

    for (uint32_t i = 0; i < 600; ++i) {
        manager.append_record({i * 10, i}); 
    }

    EXPECT_EQ(manager.get_total_records(), 600);

    EXPECT_EQ(pager.get_num_pages(), 2) << "Менеджер не запросил новую страницу у Pager'а!";
    storage::Record rec_550 = manager.get_record(550);
    EXPECT_EQ(rec_550.id, 5500) << "Ошибка расчета смещения на второй странице";
    EXPECT_EQ(rec_550.name_id, 550);
}