#include <gtest/gtest.h>
#include "storage/Pager.hpp"
#include <cstdio>
#include <vector>
#include <stdexcept>

class PagerTest : public ::testing::Test {
protected:
    const std::string test_file = "isolated_pager_suite.bin"; 
    void SetUp() override { std::remove(test_file.c_str()); }
    void TearDown() override { std::remove(test_file.c_str()); }
};

TEST_F(PagerTest, InitialState) {
    storage::Pager pager(test_file);
    EXPECT_EQ(pager.get_num_pages(), 0);
}

TEST_F(PagerTest, AllocateWriteRead) {
    storage::Pager pager(test_file);
    uint32_t page_id = pager.allocate_page();
    EXPECT_EQ(page_id, 0);
    
    std::vector<char> write_data(storage::PAGE_SIZE, 'X');
    pager.write_page(page_id, write_data);
    std::vector<char> read_data = pager.read_page(page_id);
    EXPECT_EQ(write_data, read_data);
}

TEST_F(PagerTest, Persistence) {
    {
        storage::Pager pager(test_file);
        pager.allocate_page();
        std::vector<char> data(storage::PAGE_SIZE, 'A');
        pager.write_page(0, data);
    } 
    {
        storage::Pager pager(test_file);
        EXPECT_EQ(pager.get_num_pages(), 1);
        EXPECT_EQ(pager.read_page(0)[0], 'A');
    }
}

TEST_F(PagerTest, ExceptionsHandling) {
    storage::Pager pager(test_file);
    EXPECT_THROW(pager.read_page(0), std::out_of_range);
}

TEST_F(PagerTest, ReusePages) {
    storage::Pager pager(test_file);
    uint32_t p1 = pager.allocate_page(); // id 0
    uint32_t p2 = pager.allocate_page(); // id 1

    EXPECT_EQ(p2, 1); 

    pager.free_page(p1); 
    
    uint32_t p3 = pager.allocate_page(); 
    EXPECT_EQ(p3, p1); 
    EXPECT_EQ(pager.get_num_pages(), 2);
}