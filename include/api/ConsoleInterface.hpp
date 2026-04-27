#pragma once

#include <string>
#include <vector>

namespace api {


  //Результат выполнения мета-команды.
enum class MetaCommandResult {
    SUCCESS,
    UNRECOGNIZED_COMMAND,
    EXIT_REQUESTED
};

 //Утилита для красивого форматирования вывода результатов SQL-запросов.
 //Динамически рассчитывает ширину колонок и отрисовывает псевдографическую ASCII-таблицу.
class TableFormatter {
public:
    //Отрисовывает таблицу в стандартный вывод (std::cout).
    //headers Вектор строк с названиями колонок (например, {"id", "name"}).
    //rows Двумерный вектор строк с данными (например, {{"1", "Apple"}, {"2", "Banana"}}).
    static void print_table(const std::vector<std::string>& headers, 
                            const std::vector<std::vector<std::string>>& rows);
};

    //Read-Eval-Print Loop. Главный цикл взаимодействия пользователя с СУБД.
class REPL {
private:
    //Печатает приглашение ко вводу.
    void print_prompt() const;

    //Читает строку из стандартного ввода.
    //input Ссылка на строку, куда будет записан ввод.
    void read_input(std::string& input) const;

    //Обрабатывает служебные команды (начинающиеся с точки).
    //command Текст команды (например, ".exit").
    // return MetaCommandResult Статус выполнения команды.
    MetaCommandResult execute_meta_command(const std::string& command);

public:
    REPL() = default;
    ~REPL() = default;


    //Запускает бесконечный цикл обработки команд.
    //Выход из цикла происходит только при получении команды .exit.
    void start();
};

} 