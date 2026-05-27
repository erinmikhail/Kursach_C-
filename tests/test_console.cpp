#include <gtest/gtest.h>
#include "api/ConsoleInterface.hpp"
#include "catalog/Catalog.hpp"
#include "core/Executor.hpp"
#include <cstdio>
#include <sstream>

namespace {

class CaptureStdout {
public:
    CaptureStdout() : old_(std::cout.rdbuf(buffer_.rdbuf())) {}
    ~CaptureStdout() { std::cout.rdbuf(old_); }
    std::string str() const { return buffer_.str(); }

private:
    std::ostringstream buffer_;
    std::streambuf* old_;
};

class CaptureStderr {
public:
    CaptureStderr() : old_(std::cerr.rdbuf(buffer_.rdbuf())) {}
    ~CaptureStderr() { std::cerr.rdbuf(old_); }
    std::string str() const { return buffer_.str(); }

private:
    std::ostringstream buffer_;
    std::streambuf* old_;
};

} // namespace

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

TEST(ConsoleTest, JsonFormatterPrintsNumbersAndStrings) {
    CaptureStdout capture;
    api::JsonFormatter::print({"id", "name"}, {{"1", "Alice"}});

    const std::string output = capture.str();
    EXPECT_NE(output.find(R"("id": 1)"), std::string::npos);
    EXPECT_NE(output.find(R"("name": "Alice")"), std::string::npos);
}

TEST(ConsoleTest, TableFormatterHandlesLongAndMissingCells) {
    CaptureStdout capture;
    api::TableFormatter::print_table({"name", "comment"}, {{"Alice", std::string(80, 'x')}, {"Bob"}});

    const std::string output = capture.str();
    EXPECT_NE(output.find("Alice"), std::string::npos);
    EXPECT_NE(output.find("Bob"), std::string::npos);
    EXPECT_NE(output.find("+"), std::string::npos);
}

TEST(ConsoleTest, MetaCommandHelpAndUnknownCommand) {
    std::remove("access.log");
    catalog::Catalog catalog;
    core::Executor executor(catalog);
    api::REPL repl(catalog, executor);

    CaptureStdout capture;
    EXPECT_TRUE(repl.process_line(".help"));
    EXPECT_TRUE(repl.process_line(".unknown"));

    const std::string output = capture.str();
    EXPECT_NE(output.find("Системные команды"), std::string::npos);
    EXPECT_NE(output.find("Неизвестная мета-команда"), std::string::npos);
}

TEST(ConsoleTest, ProcessLineExecutesSimpleSql) {
    std::remove("access.log");
    std::remove("global_string_pool.bin");
    std::remove("default_db.console_users.bin");
    std::remove("default_db.console_users.idx");

    catalog::Catalog catalog;
    core::Executor executor(catalog);
    api::REPL repl(catalog, executor);

    CaptureStdout capture_out;
    CaptureStderr capture_err;
    EXPECT_TRUE(repl.process_line("create table console_users (id int indexed, name string default \"Anon\");"));
    EXPECT_TRUE(repl.process_line(R"(insert into console_users (id) value (1);)"));
    EXPECT_TRUE(repl.process_line("select * from console_users;"));

    const std::string output = capture_out.str();
    EXPECT_NE(output.find(R"("id": 1)"), std::string::npos);
    EXPECT_NE(output.find(R"("name": "Anon")"), std::string::npos);
    EXPECT_TRUE(capture_err.str().empty());

    std::remove("default_db.console_users.bin");
    std::remove("default_db.console_users.idx");
    std::remove("global_string_pool.bin");
}
