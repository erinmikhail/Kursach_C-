#include <gtest/gtest.h>
#include "api/Logger.hpp"
#include <fstream>
#include <cstdio>

class LoggerTest : public ::testing::Test {
protected:
    const std::string log_file = "test_log.txt";
    void SetUp() override { std::remove(log_file.c_str()); }
    void TearDown() override { std::remove(log_file.c_str()); }
};

TEST_F(LoggerTest, LogsToFile) {
    std::string path = "unique_logger_test.log";
    {
        api::Logger logger(path);
        logger.log_request("SELECT * FROM users", "SUCCESS", 10.5);
    } 

    std::ifstream file(path);
    std::string line;
    std::getline(file, line);
    
    EXPECT_NE(line.find("SELECT * FROM users"), std::string::npos);
    EXPECT_NE(line.find("STATUS: SUCCESS"), std::string::npos);
    EXPECT_NE(line.find("10.5"), std::string::npos); 
}