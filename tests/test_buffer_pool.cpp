#include <gtest/gtest.h>
#include "storage/BufferPool.hpp"
#include "storage/Pager.hpp"
#include <cstdio>

class BufferPoolTest : public ::testing::Test {
protected:
    const std::string test_file = "test_bpm.bin";
    
    void SetUp() override { std::remove(test_file.c_str()); }
    void TearDown() override { std::remove(test_file.c_str()); }
};

TEST_F(BufferPoolTest, NewPageAndFetch) {
    storage::Pager pager(test_file);
    storage::BufferPool bpm(pager, 5); 

    uint32_t page_id;
    storage::Page* page = bpm.new_page(page_id);
    
    ASSERT_NE(page, nullptr);
    EXPECT_EQ(page_id, 0);

    page->data[0] = 'B';
    page->data[1] = 'P';

    bpm.unpin_page(page_id, true); 

    storage::Page* fetched_page = bpm.fetch_page(0);
    EXPECT_EQ(fetched_page->data[0], 'B');
    EXPECT_EQ(fetched_page->data[1], 'P');
    
    bpm.unpin_page(0, false);
}

TEST_F(BufferPoolTest, LRUReplacement) {
    storage::Pager pager(test_file);
    storage::BufferPool bpm(pager, 3);

    for (int i = 0; i < 3; ++i) {
        uint32_t pid;
        storage::Page* p = bpm.new_page(pid);
        p->data[0] = 'A' + i; 
        bpm.unpin_page(pid, true);
    }

    uint32_t pid_4;
    storage::Page* p_4 = bpm.new_page(pid_4);
    p_4->data[0] = 'D';
    bpm.unpin_page(pid_4, true);

    storage::Page* p_0 = bpm.fetch_page(0);
    EXPECT_EQ(p_0->data[0], 'A') << "Алгоритм LRU стер старую страницу, не сохранив ее на диск!";
    bpm.unpin_page(0, false);
}