#include <gtest/gtest.h>
#include "storage/StringPool.hpp"
#include <cstdio>

class StringPoolTest : public ::testing::Test {
protected:
    const std::string test_file = "test_pool.bin";

    void SetUp() override {
        std::remove(test_file.c_str());
    }

    void TearDown() override {
        std::remove(test_file.c_str());
    }
};

TEST_F(StringPoolTest, AddAndRetrieve) {
    storage::StringPool pool(test_file);
    
    uint32_t id_apple = pool.get_or_add_string("Apple");
    uint32_t id_banana = pool.get_or_add_string("Banana");

    EXPECT_NE(id_apple, id_banana) << "Разным строкам присвоен одинаковый ID";
    EXPECT_EQ(pool.get_string(id_apple), "Apple");
    EXPECT_EQ(pool.get_string(id_banana), "Banana");
}

TEST_F(StringPoolTest, DuplicateHandling) {
    storage::StringPool pool(test_file);
    
    uint32_t id1 = pool.get_or_add_string("Database");
    uint32_t id2 = pool.get_or_add_string("Database");

    EXPECT_EQ(id1, id2) << "Дубликату присвоен новый ID вместо существующего";
}

TEST_F(StringPoolTest, Persistence) {
    uint32_t original_id;
    {
        storage::StringPool pool(test_file);
        original_id = pool.get_or_add_string("PersistentString");
    } 

    {

        storage::StringPool pool(test_file);
        pool.load();

        EXPECT_EQ(pool.get_string(original_id), "PersistentString") << "Данные не загрузились с диска";

        uint32_t new_id = pool.get_or_add_string("PersistentString");
        EXPECT_EQ(original_id, new_id) << "После загрузки дубликат получил новый ID";
    }
}