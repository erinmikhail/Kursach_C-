#include "storage/StringPool.hpp"
#include <fstream>
#include <stdexcept>

namespace storage {

StringPool::StringPool(const std::string& filepath) : filepath_(filepath) {}

StringPool::~StringPool() {
    flush(); 
}

void StringPool::load() {
    std::ifstream file(filepath_, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        return;
    }

    while (file.peek() != EOF) {
        uint32_t length = 0;
        if (!file.read(reinterpret_cast<char*>(&length), sizeof(length))) {
            break;
        }

        std::string str(length, '\0');
        if (!file.read(str.data(), length)) {
            throw std::runtime_error("Ошибка при чтении строки из файла.");
        }

        // Перемещаем строку в deque, чтобы избежать копирования
        id_to_string_.push_back(std::move(str));
        
        // Мапа "смотрит" на память строки, которая теперь лежит в deque
        string_to_id_[id_to_string_.back()] = next_id_;
        next_id_++;
    }

    unsaved_start_index_ = next_id_; 
    file.close();
}

uint32_t StringPool::get_or_add_string(std::string_view str) {
    auto it = string_to_id_.find(str);
    if (it != string_to_id_.end()) {
        return it->second;
    }

    uint32_t new_id = next_id_;
    next_id_++;

    id_to_string_.emplace_back(str);

    string_to_id_[id_to_string_.back()] = new_id;

    return new_id;
}

std::string StringPool::get_string(uint32_t id) const {
    if (id >= id_to_string_.size()) {
        throw std::out_of_range("Недопустимый идентификатор строки: " + std::to_string(id));
    }
    return id_to_string_[id];
}

void StringPool::flush() {
    if (unsaved_start_index_ == next_id_) {
        return;
    }

    std::ofstream file(filepath_, std::ios::app | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Не удалось открыть файл пула строк для записи: " + filepath_);
    }

    // Записываем только те строки, которые появились с момента последнего flush
    for (uint32_t i = unsaved_start_index_; i < next_id_; ++i) {
        const std::string& str = id_to_string_[i];
        
        uint32_t length = static_cast<uint32_t>(str.size());
        file.write(reinterpret_cast<const char*>(&length), sizeof(length));
        file.write(str.data(), length);
    }

    file.flush();
    file.close();

    unsaved_start_index_ = next_id_;
}

} 