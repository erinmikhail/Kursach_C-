#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace storage {

class StringPool {
private:
    std::string filepath_;
    // Быстрый поиск ID по строке (для INSERT)
    std::unordered_map<std::string, uint32_t> string_to_id_;
    // Быстрый поиск строки по ID (для SELECT)
    std::vector<std::string> id_to_string_;
    
    uint32_t next_id_ = 0;

public:
    // Конструктор: принимает путь к файлу string_pool.bin
    explicit StringPool(const std::string& filepath);

    // Загружает существующий пул строк с диска в память при старте СУБД
    void load();

    // Основной метод: если строка новая — сохраняет её на диск и дает новый ID.
    // Если строка уже была — просто возвращает старый ID.
    uint32_t get_or_add_string(const std::string& str);

    // Возвращает текст по его числовому идентификатору
    std::string get_string(uint32_t id) const;

    // Вспомогательный метод: сохраняет весь пул на диск (синхронизация)
    void flush();
};

}