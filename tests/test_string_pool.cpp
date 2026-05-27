#include <gtest/gtest.h>
#include "storage/StringPool.hpp"

TEST(StringPoolTest, AddAndRetrieve) {
    storage::StringPool pool("test_strings.bin");
    uint32_t id = pool.get_or_add_string("Apple");
    EXPECT_EQ(pool.get_string(id), "Apple");
}

TEST(StringPoolTest, DuplicateHandling) {
    storage::StringPool pool("test_strings2.bin");
    uint32_t id1 = pool.get_or_add_string("Database");
    uint32_t id2 = pool.get_or_add_string("Database");
    EXPECT_EQ(id1, id2);
}

TEST(StringPoolTest, Persistence) {
    {
        storage::StringPool pool("test_strings_pers.bin");
        pool.get_or_add_string("Persistent");
    }
    {
        storage::StringPool pool2("test_strings_pers.bin");
        uint32_t id = pool2.get_or_add_string("Persistent");
        EXPECT_EQ(pool2.get_string(id), "Persistent");
    }
}