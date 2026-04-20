set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

add_executable(PagerTests 
    test_pager.cpp 
    ../src/storage/Pager.cpp
)
target_include_directories(PagerTests PRIVATE ${INCLUDE_DIR})
target_link_libraries(PagerTests PRIVATE GTest::gtest GTest::gtest_main pthread)

#Тест закомментировал, нужно тупо убрать решетки где тесты, а то CMake не может найти эти файлы и ругается. Когда нужно будет протестировать эти классы, просто раскомментируешь и все заработает.


# add_executable(StringPoolTests 
#   test_string_pool.cpp 
#   ../src/storage/StringPool.cpp
#)
#target_include_directories(StringPoolTests PRIVATE ${INCLUDE_DIR})
#target_link_libraries(StringPoolTests PRIVATE GTest::gtest GTest::gtest_main pthread)

add_executable(RecordManagerTests 
   test_record_manager.cpp 
   ../src/storage/RecordManager.cpp 
   ../src/storage/Pager.cpp
)
target_include_directories(RecordManagerTests PRIVATE ${INCLUDE_DIR})
target_link_libraries(RecordManagerTests PRIVATE GTest::gtest GTest::gtest_main pthread)

#add_executable(BufferPoolTests 
#    test_buffer_pool.cpp 
#    ../src/storage/BufferPool.cpp
#    ../src/storage/Pager.cpp
#)
#target_include_directories(BufferPoolTests PRIVATE ${INCLUDE_DIR})
#target_link_libraries(BufferPoolTests PRIVATE GTest::gtest GTest::gtest_main pthread)

# Регистрируем тесты для CTest
include(GoogleTest)
gtest_discover_tests(PagerTests)
# gtest_discover_tests(StringPoolTests)
gtest_discover_tests(RecordManagerTests)
# gtest_discover_tests(NumberTests)