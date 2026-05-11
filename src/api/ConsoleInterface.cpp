#include "api/ConsoleInterface.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <fstream>
#include <cctype>

namespace api {

namespace {

static constexpr size_t kMaxCellWidth = 50;

bool is_numeric(const std::string& str) {
    if (str.empty()) return false;
    size_t start = 0;
    if (str[0] == '-' || str[0] == '+') {
        if (str.size() == 1) return false;
        start = 1;
    }
    bool has_dot = false;
    for (size_t i = start; i < str.size(); ++i) {
        if (str[i] == '.') {
            if (has_dot) return false; // Две точки — не число
            has_dot = true;
        } else if (!std::isdigit(static_cast<unsigned char>(str[i]))) {
            return false;
        }
    }
    return true;
}

std::string build_border(const std::vector<size_t>& widths) {
    std::string line;
    line.reserve(widths.size() * 3 + 1);
    line.push_back('+');
    for (size_t width : widths) {
        line.append(width + 2, '-');
        line.push_back('+');
    }
    return line;
}

void print_row(const std::vector<std::string>& row, const std::vector<size_t>& widths) {
    std::cout << "|";
    for (size_t column = 0; column < widths.size(); ++column) {
        std::string_view cell = column < row.size() ? std::string_view(row[column]) : std::string_view{};
        
        if (cell.size() > kMaxCellWidth) {
            cell = cell.substr(0, kMaxCellWidth);
        }
        
        std::cout << " " << std::left << std::setw(static_cast<int>(widths[column])) << cell << std::right << " |";
    }
    std::cout << "\n";
}

void print_help() {
    std::cout << "Системные команды:\n"
              << "  .help          - показать общую подсказку\n"
              << "  .help <topic>  - показать справку по теме\n"
              << "  .clear         - очистить экран\n"
              << "  .exit          - выйти из программы\n";
}

void print_help_for(std::string_view topic) {
    if (topic == "insert") {
        std::cout << "INSERT пока не реализован, но в будущем позволит добавлять записи в таблицу.\n";
    } else if (topic == "select") {
        std::cout << "SELECT пока не реализован. На данном этапе запросы только принимаются к обработке.\n";
    } else if (topic == "exit") {
        std::cout << "Команда .exit завершает программу независимо от аргументов, например .exit 0.\n";
    } else if (topic == "clear") {
        std::cout << "Команда .clear очищает экран терминала.\n";
    } else {
        std::cout << "Справка для темы '" << topic << "' не найдена.\n";
    }
}

}

void JsonFormatter::print(const std::vector<std::string>& headers,
                          const std::vector<std::vector<std::string>>& rows) {
    std::cout << "[\n";
    for (size_t r = 0; r < rows.size(); ++r) {
        std::cout << "  {\n";
        for (size_t c = 0; c < headers.size(); ++c) {
            std::cout << "    \"" << headers[c] << "\": ";
            
            std::string val = c < rows[r].size() ? rows[r][c] : "";
            if (is_numeric(val)) {
                std::cout << val;
            } else {
                std::cout << "\"" << val << "\"";
            }
            
            if (c + 1 < headers.size()) {
                std::cout << ",";
            }
            std::cout << "\n";
        }
        std::cout << "  }";
        if (r + 1 < rows.size()) {
            std::cout << ",";
        }
        std::cout << "\n";
    }
    std::cout << "]\n";
}

void TableFormatter::print_table(const std::vector<std::string>& headers,
                                 const std::vector<std::vector<std::string>>& rows) {
    if (headers.empty()) {
        return;
    }

    std::vector<size_t> widths(headers.size());

    for (size_t i = 0; i < headers.size(); ++i) {
        widths[i] = std::min(headers[i].size(), kMaxCellWidth);
    }

    for (auto const& row : rows) {
        for (size_t i = 0; i < headers.size(); ++i) {
            size_t cell_size = i < row.size() ? row[i].size() : 0;
            widths[i] = std::max(widths[i], std::min(cell_size, kMaxCellWidth));
        }
    }

    const std::string border = build_border(widths);
    
    std::cout << border << "\n";
    print_row(headers, widths);
    std::cout << border << "\n";
    for (auto const& row : rows) {
        print_row(row, widths);
    }
    std::cout << border << "\n";
}

void REPL::print_prompt() const {
    std::cout << "dbms > ";
    std::cout.flush();
}

void REPL::read_input(std::string& input) const {
    std::getline(std::cin, input);
}

MetaCommandResult REPL::execute_meta_command(const std::string& command) {
    if (command == ".exit" || command.rfind(".exit ", 0) == 0) {
        return MetaCommandResult::EXIT_REQUESTED;
    }
    if (command == ".clear" || command.rfind(".clear ", 0) == 0) {
        std::cout << "\033[2J\033[1;1H";
        return MetaCommandResult::SUCCESS;
    }
    if (command == ".help") {
        print_help();
        return MetaCommandResult::SUCCESS;
    }
    if (command.rfind(".help ", 0) == 0) {
        print_help_for(command.substr(6));
        return MetaCommandResult::SUCCESS;
    }

    std::cout << "Неизвестная мета-команда: " << command << "\n";
    return MetaCommandResult::UNRECOGNIZED_COMMAND;
}

void REPL::start() {
    while (true) {
        print_prompt();

        std::string input;
        read_input(input);
        if (!std::cin) {
            std::cout << "\n";
            break;
        }

        if (input.empty()) {
            continue;
        }

        if (input.front() == '.') {
            const MetaCommandResult result = execute_meta_command(input);
            if (result == MetaCommandResult::EXIT_REQUESTED) {
                break;
            }
            continue;
        }

        std::cout << "SQL запрос принят к обработке: " << input << "\n";
    }
}

} // namespace api
