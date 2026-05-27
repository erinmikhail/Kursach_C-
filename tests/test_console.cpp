#include <gtest/gtest.h>
#include "api/ConsoleInterface.hpp"
#include "catalog/Catalog.hpp"
#include "core/Executor.hpp"

TEST(ConsoleTest, MetaCommandExit) {
    catalog::Catalog catalog;
    core::Executor executor(catalog);
    api::REPL repl(catalog, executor);
    EXPECT_FALSE(repl.process_line(".exit"));
}

TEST(ConsoleTest, MetaCommandClear) {
    catalog::Catalog catalog;
    core::Executor executor(catalog);
    api::REPL repl(catalog, executor);
    
    EXPECT_TRUE(repl.process_line(".clear"));
}