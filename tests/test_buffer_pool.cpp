#include <gtest/gtest.h>
#include "storage/BufferPool.hpp"
#include <cstdio>
#include <vector>

class BufferPoolTest : public ::testing::Test {
protected:
    const std::string test_file = "isolated_pool_suite.bin";
    void SetUp() override { std::remove(test_file.c_str()); }
    void TearDown() override { std::remove(test_file.c_str()); }
};

TEST_F(BufferPoolTest, FetchExistingPagePinsAndUnpins) {
    storage::Pager pager(test_file);
    const uint32_t id = pager.allocate_page();

    storage::BufferPool pool(pager, 1);
    storage::Page* page = pool.fetch_page(id);

    ASSERT_NE(page, nullptr);
    EXPECT_EQ(page->pin_count, 1);
    pool.unpin_page(id, false);
    EXPECT_EQ(page->pin_count, 0);
}

TEST_F(BufferPoolTest, NewPageCreatesZeroFilledPageAndPersistsAfterFlush) {
    storage::Pager pager(test_file);
    storage::BufferPool pool(pager, 2);

    uint32_t page_id = 0;
    storage::Page* page = pool.new_page(page_id);

    ASSERT_NE(page, nullptr);
    page->data[10] = 'Q';
    pool.unpin_page(page_id, true);
    pool.flush_page(page_id);

    EXPECT_EQ(pager.read_page(page_id)[10], 'Q');
}