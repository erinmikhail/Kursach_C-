#pragma once

#include <fstream>
#include <string>

namespace api {

/**
 * @class Logger
 * @brief Подсистема логирования активности СУБД (согласно ТЗ, пункт 7).
 */
class Logger {
private:
    std::ofstream log_file_;
    std::string file_path_;

public:
    /**
     * @brief Открывает файл лога для дозаписи.
     * @param path Путь к файлу (например, "access.log").
     */
    explicit Logger(const std::string& path = "access.log");
    
    ~Logger();

    /**
     * @brief Записывает информацию о запросе в файл.
     * @param request Тело запроса.
     * @param status Статус (например, "SUCCESS" или текст ошибки).
     * @param duration_ms Время выполнения в миллисекундах.
     */
    void log_request(const std::string& request, 
                     const std::string& status, 
                     double duration_ms);
};

} // namespace api

namespace utils {
using Logger = api::Logger;
} // namespace utils
