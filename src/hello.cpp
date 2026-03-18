#include <iostream>

int main() {
    std::cout << "Введите число: ";

    int result = 0;
    std::cin >> result;
    if (result > 0) {
        std::cout << "Число больше нуля" << std::endl;
    } else if (result == 0) {
        std::cout << "Число равно нулю" << std::endl;
    } else {
        std::cout << "Число меньше нуля" << std::endl;
    }

    return 0;
}