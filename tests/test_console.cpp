#include <gtest/gtest.h>
#include "api/ConsoleInterface.hpp"
#include <sstream>

TEST(ConsoleTest, MetaCommandExit) {
    api::REPL repl;
    
    // Проверяем, что .exit возвращает false (сигнал к выходу)
    EXPECT_FALSE(repl.process_line(".exit"));
}

TEST(ConsoleTest, MetaCommandClear) {
    api::REPL repl; 
    
    EXPECT_TRUE(repl.process_line(".clear"));
}