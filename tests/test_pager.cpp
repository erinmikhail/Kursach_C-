#include <gtest/gtest.h>
#include "storage/Pager.hpp"
#include <cstdio>
#include <vector>
#include <stdexcept>

class PagerTest : public ::testing::Test {
protected:
    const std::string test_file = "test_pager_db.bin";

    void SetUp() override {
        std::remove(test_file.c_str());
    }

    void TearDown() override {
        std::remove(test_file.c_str());
    }
};

TEST_F(PagerTest, InitialState) {
    storage::Pager pager(test_file);
    EXPECT_EQ(pager.get_num_pages(), 0) << "Новый файл должен содержать 0 страниц";
}

TEST_F(PagerTest, AllocateWriteRead) {
    storage::Pager pager(test_file);

    uint32_t page_id = pager.allocate_page();
    EXPECT_EQ(page_id, 0);
    EXPECT_EQ(pager.get_num_pages(), 1);

    std::vector<char> write_data(storage::PAGE_SIZE, 'X');
    write_data[0] = 'H'; 
    write_data[1] = 'I';
    pager.write_page(page_id, write_data);
    std::vector<char> read_data = pager.read_page(page_id);

    EXPECT_EQ(write_data, read_data) << "Прочитанные данные не совпадают с записанными";
}

TEST_F(PagerTest, Persistence) {
    {
        storage::Pager pager(test_file);
        pager.allocate_page();
        std::vector<char> write_data(storage::PAGE_SIZE, 'A');
        pager.write_page(0, write_data);
    } 

    {

        storage::Pager pager(test_file);
        EXPECT_EQ(pager.get_num_pages(), 1) << "Pager не увидел созданную ранее страницу";
        
        std::vector<char> read_data = pager.read_page(0);
        EXPECT_EQ(read_data[0], 'A') << "Данные не сохранились на диске";
    }
}

TEST_F(PagerTest, ExceptionsHandling) {
    storage::Pager pager(test_file);

    EXPECT_THROW(pager.read_page(0), std::out_of_range) 
        << "Должно выбрасываться исключение при чтении несуществующей страницы";

    std::vector<char> bad_data(100, 'Z'); 
    EXPECT_THROW(pager.write_page(0, bad_data), std::invalid_argument)
        << "Должно выбрасываться исключение при неверном размере страницы";
}