# Общий путь к папкам 
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")

include_directories(${INCLUDE_DIR})
include(GoogleTest)

# 1. Pager
add_executable(PagerTests test_pager.cpp ${SRC_DIR}/storage/Pager.cpp)
target_link_libraries(PagerTests PRIVATE GTest::gtest GTest::gtest_main pthread)
gtest_discover_tests(PagerTests)

# 2. RecordManager
add_executable(RecordManagerTests test_record_manager.cpp ${SRC_DIR}/storage/RecordManager.cpp ${SRC_DIR}/storage/Pager.cpp)
target_link_libraries(RecordManagerTests PRIVATE GTest::gtest GTest::gtest_main pthread)
gtest_discover_tests(RecordManagerTests)

# 3. BufferPool
add_executable(BufferPoolTests test_buffer_pool.cpp ${SRC_DIR}/storage/BufferPool.cpp ${SRC_DIR}/storage/Pager.cpp)
target_link_libraries(BufferPoolTests PRIVATE GTest::gtest GTest::gtest_main pthread)
gtest_discover_tests(BufferPoolTests)

# 4. StringPool
add_executable(StringPoolTests test_string_pool.cpp ${SRC_DIR}/storage/StringPool.cpp)
target_link_libraries(StringPoolTests PRIVATE GTest::gtest GTest::gtest_main pthread)
gtest_discover_tests(StringPoolTests)

# 5. Parser
add_executable(ParserTests test_parser.cpp ${SRC_DIR}/parser/Lexer.cpp ${SRC_DIR}/parser/Parser.cpp)
target_link_libraries(ParserTests PRIVATE GTest::gtest GTest::gtest_main pthread)
gtest_discover_tests(ParserTests)

# 6. Index
add_executable(IndexTests b_star_plus_tree_tests.cpp)
target_link_libraries(IndexTests PRIVATE GTest::gtest GTest::gtest_main pthread)
gtest_discover_tests(IndexTests)

# 7. Lexer
add_executable(LexerTests test_lexer.cpp ${SRC_DIR}/parser/Lexer.cpp)
target_link_libraries(LexerTests PRIVATE GTest::gtest GTest::gtest_main pthread)
gtest_discover_tests(LexerTests)

# 8. Logger
add_executable(LoggerTests test_logger.cpp ${SRC_DIR}/api/Logger.cpp)
target_link_libraries(LoggerTests PRIVATE GTest::gtest GTest::gtest_main pthread)
gtest_discover_tests(LoggerTests)

# 9. Console
add_executable(ConsoleTests test_console.cpp ${SRC_DIR}/api/ConsoleInterface.cpp ${SRC_DIR}/parser/Lexer.cpp ${SRC_DIR}/parser/Parser.cpp ${SRC_DIR}/api/Logger.cpp)
target_link_libraries(ConsoleTests PRIVATE GTest::gtest GTest::gtest_main pthread)
gtest_discover_tests(ConsoleTests)

# Регистрация всех тестов
include(GoogleTest)
gtest_discover_tests(PagerTests)
gtest_discover_tests(RecordManagerTests)
gtest_discover_tests(BufferPoolTests)
gtest_discover_tests(StringPoolTests)
gtest_discover_tests(ParserTests)
gtest_discover_tests(LexerTests)
gtest_discover_tests(LoggerTests)
gtest_discover_tests(ConsoleTests)
gtest_discover_tests(IndexTests)