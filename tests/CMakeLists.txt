# Общий путь к заголовкам
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

# 1. Тесты для Pager 
add_executable(PagerTests test_pager.cpp ../src/storage/Pager.cpp)
target_include_directories(PagerTests PRIVATE ${INCLUDE_DIR})
target_link_libraries(PagerTests PRIVATE GTest::gtest GTest::gtest_main pthread)

# 2. Тесты для RecordManager
add_executable(RecordManagerTests 
    test_record_manager.cpp 
    ../src/storage/RecordManager.cpp 
    ../src/storage/Pager.cpp
)
target_include_directories(RecordManagerTests PRIVATE ${INCLUDE_DIR})
target_link_libraries(RecordManagerTests PRIVATE GTest::gtest GTest::gtest_main pthread)

# 3. Тесты для BufferPool
add_executable(BufferPoolTests 
    test_buffer_pool.cpp 
    ../src/storage/BufferPool.cpp
    ../src/storage/Pager.cpp
)
target_include_directories(BufferPoolTests PRIVATE ${INCLUDE_DIR})
target_link_libraries(BufferPoolTests PRIVATE GTest::gtest GTest::gtest_main pthread)

# 4. Тесты для StringPool 
add_executable(StringPoolTests 
    test_string_pool.cpp 
    ../src/storage/StringPool.cpp
)
target_include_directories(StringPoolTests PRIVATE ${INCLUDE_DIR})
target_link_libraries(StringPoolTests PRIVATE GTest::gtest GTest::gtest_main pthread)

# Регистрация всех тестов
include(GoogleTest)
gtest_discover_tests(PagerTests)
gtest_discover_tests(RecordManagerTests)
gtest_discover_tests(BufferPoolTests)
gtest_discover_tests(StringPoolTests)