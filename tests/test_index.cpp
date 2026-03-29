#include <gtest/gtest.h>
#include <sstream>
#include <string>

// Функция, которую будем тестировать
std::string checkNumber(int result) {
    if (result > 0) {
        return "Число больше нуля";
    } else if (result == 0) {
        return "Число равно нулю";
    } else {
        return "Число меньше нуля";
    }
}

// Тесты
TEST(NumberTest, Positive) {
    EXPECT_EQ(checkNumber(5), "Число больше нуля");
}

TEST(NumberTest, Zero) {
    EXPECT_EQ(checkNumber(0), "Число равно нулю");
}

TEST(NumberTest, Negative) {
    EXPECT_EQ(checkNumber(-7), "Число меньше нуля");
}

// Если хочешь, можно проверить ввод-вывод через stringstream
TEST(NumberTest, IOTest) {
    std::istringstream in("10");
    std::ostringstream out;

    int number;
    in >> number;
    out << checkNumber(number);

    EXPECT_EQ(out.str(), "Число больше нуля");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}