#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace storage {

class StringPool {
private:
    std::unordered_map<std::string, uint32_t> string_to_id_;
    std::vector<std::string> id_to_string_;
    std::string filename_;

    // Внутренний метод сохранения состояния пула на диск
    void save();

public:
    // Конструктор принимает имя файла для персистентности (по умолчанию strings.bin)
    explicit StringPool(const std::string& filename = "strings.bin");

    // Получает ID строки, если ее нет - добавляет, присваивает ID и сохраняет на диск
    uint32_t get_or_add_string(const std::string& str);

    // Возвращает строку по ее ID. Бросает исключение, если ID не существует.
    std::string get_string(uint32_t id) const;

    // Загружает пул из файла при запуске СУБД
    void load();
};

} 