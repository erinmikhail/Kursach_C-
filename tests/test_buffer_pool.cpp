#include <gtest/gtest.h>

#include "storage/BufferPool.hpp"

#include <algorithm>
#include <cstdio>
#include <vector>

class BufferPoolTest : public ::testing::Test {
protected:
    const std::string test_file = "test_buffer_pool.bin";

    void SetUp() override {
        std::remove(test_file.c_str());
    }

    void TearDown() override {
        std::remove(test_file.c_str());
    }
};

TEST_F(BufferPoolTest, FetchExistingPagePinsAndUnpins) {
    storage::Pager pager(test_file);
    const uint32_t page_id = pager.allocate_page();

    std::vector<char> data(storage::PAGE_SIZE, 0);
    data[0] = 'A';
    pager.write_page(page_id, data);

    storage::BufferPool pool(pager, 1);
    storage::Page* page = pool.fetch_page(page_id);

    ASSERT_NE(page, nullptr);
    EXPECT_EQ(page->page_id, page_id);
    EXPECT_EQ(page->pin_count, 1);
    EXPECT_EQ(page->data[0], 'A');

    pool.unpin_page(page_id, false);
    EXPECT_EQ(page->pin_count, 0);
}

TEST_F(BufferPoolTest, FlushesDirtyPageAfterUnpinAndEviction) {
    storage::Pager pager(test_file);
    const uint32_t first_page = pager.allocate_page();
    const uint32_t second_page = pager.allocate_page();

    storage::BufferPool pool(pager, 1);

    storage::Page* page = pool.fetch_page(first_page);
    ASSERT_NE(page, nullptr);
    page->data[0] = 'Z';
    pool.unpin_page(first_page, true);

    storage::Page* next = pool.fetch_page(second_page);
    ASSERT_NE(next, nullptr);
    pool.unpin_page(second_page, false);

    std::vector<char> disk_page = pager.read_page(first_page);
    EXPECT_EQ(disk_page[0], 'Z');
}

TEST_F(BufferPoolTest, ReturnsNullWhenAllFramesArePinned) {
    storage::Pager pager(test_file);
    const uint32_t first_page = pager.allocate_page();
    const uint32_t second_page = pager.allocate_page();

    storage::BufferPool pool(pager, 1);

    storage::Page* page = pool.fetch_page(first_page);
    ASSERT_NE(page, nullptr);

    EXPECT_EQ(pool.fetch_page(second_page), nullptr);
}

TEST_F(BufferPoolTest, NewPageCreatesZeroFilledPageAndPersistsAfterFlush) {
    storage::Pager pager(test_file);
    storage::BufferPool pool(pager, 2);

    uint32_t page_id = 0;
    storage::Page* page = pool.new_page(page_id);

    ASSERT_NE(page, nullptr);
    EXPECT_EQ(page_id, 0);
    EXPECT_EQ(page->pin_count, 1);
    EXPECT_TRUE(std::all_of(page->data.begin(), page->data.end(),
                            [](char ch) { return ch == 0; }));

    page->data[10] = 'Q';
    pool.unpin_page(page_id, true);
    pool.flush_page(page_id);

    std::vector<char> disk_page = pager.read_page(page_id);
    EXPECT_EQ(disk_page[10], 'Q');
}
