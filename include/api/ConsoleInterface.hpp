#pragma once

#include "api/Logger.hpp"
#include "catalog/Catalog.hpp"
#include "core/Executor.hpp"
#include <string>
#include <vector>

namespace api {

// Результат выполнения мета-команды.
enum class MetaCommandResult {
    SUCCESS,
    UNRECOGNIZED_COMMAND,
    EXIT_REQUESTED
};

// Утилита для форматирования вывода в формате JSON (согласно ТЗ).
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

// Read-Eval-Print Loop. Главный цикл взаимодействия пользователя с СУБД.
class REPL {
public:
    // ОБНОВЛЕННЫЙ КОНСТРУКТОР: принимает Каталог и Исполнитель
    REPL(catalog::Catalog& catalog, core::Executor& executor);
    ~REPL() = default;

    // Запускает интерактивный бесконечный цикл обработки команд.
    void start();

    // Запускает пакетный режим (чтение команд из файла).
    void run_batch(const std::string& filename);
    
    // Вспомогательный метод для обработки одной строки (замер времени и логирование)
    bool process_line(const std::string& input);

private:
    catalog::Catalog& catalog_; // Ссылка на общий каталог базы данных
    core::Executor& executor_;  // Ссылка на исполнитель запросов
    Logger logger_;             // Объект логгера для записи активности

    void print_prompt() const;
    void read_input(std::string& input) const;
    
    void execute_sql_query(const std::string& query);
    MetaCommandResult execute_meta_command(const std::string& command);
};

} 