#include "storage/Pager.hpp"
#include <iostream>
#include <stdexcept>

namespace storage { 
    // конструктор под отр и закр и счет стр файла
    Pager::Pager(const std::string& filepath) : filepath_(filepath), num_pages(0) {
        file_.open(filepath_, std::ios::in | std::ios::out | std::ios::binary);

        if (!file_.is_open()){
            file_open(filepath_; std::ios::out | std::ios::binary);
            file_.close();

            file_.open(filepath_, std::ios::in | std::ios::out | std::ios::binary);

            if (!file_.is_open()){
                throw std::runtime_error("Не удалось открыть/создать файл: " + filepath_);
            }
        }

        file_.seekg(0, std::ios::end);
        std::streampos file_size = file_.tellg();
        num_pages = file_size / PAGE_SIZE;
    }

    Pager::~Pager() {
        if (file_is_open()) {
            file_.close();
        }
    }

    std::vector<char> Pager::read_page(uint32_t page_num){
        if (page_num >= num_pages) {
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
        uint32_t new_page_num = num_pages++;
        std::vector<char> empty_page(PAGE_SIZE, 0);
        write_page(new_page_num, empty_page);
        return new_page_num;
    }

    void Pager::free_page(uint32_t page_num) {
        TODO: // реализовать при индкексах!!!
    }

    uint32_t Pager::get_num_pages() const {
        return num_pages;
    }
}