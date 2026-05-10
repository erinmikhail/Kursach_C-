#include "api/ConsoleInterface.hpp"

#include "parser/Lexer.hpp"
#include "parser/Parser.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string_view>

namespace api {

namespace {

static constexpr size_t kMaxCellWidth = 50; 

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

void REPL::execute_sql_query(const std::string& query) {
    parser::Lexer lexer(query);
    const std::vector<parser::Token> tokens = lexer.tokenize();
    parser::Parser parser(tokens);
    parser.parse();

    std::cout << "SQL запрос принят к обработке: " << query << "\n";
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

        const auto started_at = std::chrono::steady_clock::now();
        std::string status = "SUCCESS";
        bool should_exit = false;

        if (input.front() == '.') {
            try {
                const MetaCommandResult result = execute_meta_command(input);
                if (result == MetaCommandResult::UNRECOGNIZED_COMMAND) {
                    status = "ERROR (MetaCommandError)";
                } else if (result == MetaCommandResult::EXIT_REQUESTED) {
                    should_exit = true;
                }
            } catch (const std::exception& ex) {
                status = std::string("ERROR (MetaCommandError: ") + ex.what() + ")";
            }
        } else {
            try {
                execute_sql_query(input);
            } catch (const std::exception& ex) {
                status = std::string("ERROR (ParserError: ") + ex.what() + ")";
                std::cout << "Ошибка обработки SQL запроса: " << ex.what() << "\n";
            }
        }

        const auto finished_at = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration<double, std::milli>(finished_at - started_at);
        logger_.log_request(input, status, elapsed.count());

        if (should_exit) {
            break;
        }
    }
}

} // namespace api
