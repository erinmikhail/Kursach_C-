#pragma once

#include "api/Logger.hpp"
#include <string>
#include <vector>

namespace api {

// Результат выполнения мета-команды.
enum class MetaCommandResult {
    SUCCESS,
    UNRECOGNIZED_COMMAND,
    EXIT_REQUESTED
};

// Утилита для форматирования вывода в формате JSON (согласно ТЗ)[cite: 24].
class JsonFormatter {
public:
    static void print(const std::vector<std::string>& headers, 
                      const std::vector<std::vector<std::string>>& rows);
};

// Утилита для псевдографического вывода (для отладки).
class TableFormatter {
public:
    static void print_table(const std::vector<std::string>& headers, 
                            const std::vector<std::vector<std::string>>& rows);
};

// Read-Eval-Print Loop. Главный цикл взаимодействия пользователя с СУБД[cite: 13].
class REPL {
private:
    Logger logger_; // Объект логгера для записи активности [cite: 67]

    void print_prompt() const;
    void read_input(std::string& input) const;
    
    // Вспомогательный метод для обработки одной строки (замер времени и логирование)
    bool process_line(const std::string& input);
    
    void execute_sql_query(const std::string& query);
    MetaCommandResult execute_meta_command(const std::string& command);

public:
    REPL() = default;
    ~REPL() = default;

    // Запускает интерактивный бесконечный цикл обработки команд[cite: 14].
    void start();

    // Запускает пакетный режим (чтение команд из файла)[cite: 15].
    void run_batch(const std::string& filename);
};

} // namespace api