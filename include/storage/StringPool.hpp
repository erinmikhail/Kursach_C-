#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <deque>
#include <cstdint>

namespace storage {

class StringPool {
private:
    std::string filepath_;
    
    // Храним легковесные "вьюшки", а не копии строк
    std::unordered_map<std::string_view, uint32_t> string_to_id_;
    
    // Используем deque, чтобы адреса строк в памяти никогда не менялись
    std::deque<std::string> id_to_string_;
    
    uint32_t next_id_ = 0;
    
    // Маркер: с какого ID начинаются строки, которые еще не сброшены на диск
    uint32_t unsaved_start_index_ = 0; 

public:
    explicit StringPool(const std::string& filepath);
    
    // Деструктор гарантирует, что мы не потеряем данные при выходе
    ~StringPool();

    void load();

    // Принимаем string_view, чтобы не создавать временных копий при вызове
    uint32_t get_or_add_string(std::string_view str);

    std::string get_string(uint32_t id) const;

    // Пакетный сброс новых данных на диск
    void flush();
};

} // namespace storage