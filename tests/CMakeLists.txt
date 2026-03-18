# Все тестовые файлы
set(TEST_SOURCES
    test.cpp
)

add_executable(NumberTests ${TEST_SOURCES})

target_link_libraries(NumberTests PRIVATE GTest::gtest GTest::gtest_main pthread)

# Регистрируем тесты для CTest
include(GoogleTest)
gtest_discover_tests(NumberTests)