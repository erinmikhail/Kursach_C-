#include "storage/Pager.hpp"
#include <iostream>
#include <stdexcept>

namespace storage { 
    // конструктор под отр и закр и счет стр файла
    Pager::Pager(const std::string& filepath) : filepath_(filepath), num_pages_(0) {
        file_.open(filepath_, std::ios::in | std::ios::out | std::ios::binary);

        if (!file_.is_open()){
            file_.open(filepath_, std::ios::out | std::ios::binary);
            file_.close();

            file_.open(filepath_, std::ios::in | std::ios::out | std::ios::binary);

            if (!file_.is_open()){
                throw std::runtime_error("Не удалось открыть/создать файл: " + filepath_);
            }
        }

        file_.seekg(0, std::ios::end);
        std::streampos file_size = file_.tellg();
        num_pages_ = file_size / PAGE_SIZE;
    }

    Pager::~Pager() {
        if (file_.is_open()) {
            file_.close();
        }
    }

    std::vector<char> Pager::read_page(uint32_t page_num){
        if (page_num >= num_pages_) {
            throw std::out_of_range("Номер страницы выходит за пределы файла.");
        }

        std::vector<char> page_data(PAGE_SIZE);
        file_.seekg(page_num * PAGE_SIZE, std::ios::beg);
        file_.read(page_data.data(), PAGE_SIZE);

        return page_data;
    }

    void Pager::write_page(uint32_t page_num, const std::vector<char>& page_data) {
        if (page_data.size() != PAGE_SIZE) {
            throw std::invalid_argument("Размер данных должен быть равен размеру страницы.");
        }

        file_.seekp(page_num * PAGE_SIZE, std::ios::beg);
        file_.write(page_data.data(), PAGE_SIZE);

        file_.flush(); // принудительно выталкнуть данные на диск
    }

    uint32_t Pager::allocate_page() {
        if (!free_pages_.empty()) {
        uint32_t reused_id = free_pages_.back();
        free_pages_.pop_back();

        std::vector<char> zero_data(PAGE_SIZE, 0);
        write_page(reused_id, zero_data);
        
        return reused_id;
    }

    uint32_t new_page_id = num_pages_;
    num_pages_++;

    std::vector<char> data(PAGE_SIZE, 0);
    write_page(new_page_id, data);

    return new_page_id;
    }

    void Pager::free_page(uint32_t page_num) {
        if (page_num >= num_pages_) {
        throw std::out_of_range("Попытка освободить несуществующую страницу." + std::to_string(page_num));
    }
    free_pages_.push_back(page_num);
    }

    uint32_t Pager::get_num_pages() const {
        return num_pages_;
    }
}