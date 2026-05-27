#include <gtest/gtest.h>
#include "storage/RecordManager.hpp"
#include "storage/Pager.hpp"

TEST(RecordManagerTest, AppendAndGet) {
    storage::Pager pager("test_rm.bin");
    storage::RecordManager manager(pager, 2);
    
    storage::Record rec;
    rec.is_deleted = false;
    rec.values = {10, 1};
    
    manager.append_record(rec);
    EXPECT_EQ(manager.get_record(0).values[0], 10);
}

TEST(RecordManagerTest, PageCrossing) {
    storage::Pager pager("test_rm_cross.bin");
    storage::RecordManager manager(pager, 2);
    
    for (int i = 0; i < 600; ++i) {
        storage::Record rec;
        rec.is_deleted = false;
        rec.values = {(uint32_t)(i * 10), (uint32_t)i};
        manager.append_record(rec);
    }
    
    EXPECT_EQ(manager.get_record(550).values[0], 5500);
}