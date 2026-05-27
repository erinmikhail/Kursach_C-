#include "storage/StringPool.hpp"
#include <fstream>
#include <stdexcept>

namespace storage {

StringPool::StringPool(const std::string& filename) : filename_(filename) {
    load();
}

uint32_t StringPool::get_or_add_string(const std::string& str) {
    auto it = string_to_id_.find(str);
    if (it != string_to_id_.end()) {
        return it->second;
    }
    uint32_t id = id_to_string_.size();
    string_to_id_[str] = id;
    id_to_string_.push_back(str);
    save();
    return id;
}

std::string StringPool::get_string(uint32_t id) const {
    if (id >= id_to_string_.size()) {
        throw std::out_of_range("");
    }
    return id_to_string_[id];
}

void StringPool::load() {
    string_to_id_.clear();
    id_to_string_.clear();
    std::ifstream in(filename_, std::ios::binary);
    if (!in.is_open()) return;
    uint32_t count = 0;
    if (!in.read(reinterpret_cast<char*>(&count), sizeof(count))) return;
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t len = 0;
        if (!in.read(reinterpret_cast<char*>(&len), sizeof(len))) break;
        std::string s(len, '\0');
        in.read(&s[0], len);
        string_to_id_[s] = i;
        id_to_string_.push_back(s);
    }
}

void StringPool::save() {
    std::ofstream out(filename_, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) throw std::runtime_error("");
    uint32_t count = id_to_string_.size();
    out.write(reinterpret_cast<const char*>(&count), sizeof(count));
    for (const auto& s : id_to_string_) {
        uint32_t len = s.size();
        out.write(reinterpret_cast<const char*>(&len), sizeof(len));
        out.write(s.data(), len);
    }
}

}