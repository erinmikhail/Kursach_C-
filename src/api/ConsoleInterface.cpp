#include "api/ConsoleInterface.hpp"

#include <algorithm>
#include <iostream>

namespace api {

namespace {

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

void print_row(const std::vector<std::string_view>& row, const std::vector<size_t>& widths) {
    std::cout << "|";
    for (size_t column = 0; column < widths.size(); ++column) {
        const std::string_view cell = column < row.size() ? row[column] : std::string_view{};
        std::cout << " " << cell;
        if (cell.size() < widths[column]) {
            std::cout << std::string(widths[column] - cell.size(), ' ');
        }
        std::cout << " |";
    }
    std::cout << "\n";
}

} // namespace

void TableFormatter::print_table(const std::vector<std::string>& headers,
                                 const std::vector<std::vector<std::string>>& rows) {
    if (headers.empty()) {
        return;
    }

    std::vector<size_t> widths(headers.size());
    std::vector<std::string_view> header_views;
    header_views.reserve(headers.size());

    for (size_t i = 0; i < headers.size(); ++i) {
        std::string_view header(headers[i]);
        widths[i] = header.size();
        header_views.push_back(header);
    }

    std::vector<std::vector<std::string_view>> view_rows;
    view_rows.reserve(rows.size());

    for (auto const& row : rows) {
        std::vector<std::string_view> view_row;
        view_row.reserve(headers.size());

        for (size_t i = 0; i < headers.size(); ++i) {
            std::string_view cell = i < row.size() ? std::string_view(row[i]) : std::string_view{};
            widths[i] = std::max(widths[i], cell.size());
            view_row.push_back(cell);
        }

        view_rows.emplace_back(std::move(view_row));
    }

    const std::string border = build_border(widths);
    std::cout << border << "\n";
    print_row(header_views, widths);
    std::cout << border << "\n";
    for (auto const& row : view_rows) {
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
    if (command == ".exit") {
        return MetaCommandResult::EXIT_REQUESTED;
    }
    if (command == ".help") {
        std::cout << "Системные команды:\n"
                  << "  .help  - показать эту подсказку\n"
                  << "  .exit  - выйти из программы\n";
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
