#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <cstdint>

namespace storage {
    constexpr size_t PAGE_SIZE = 4096;

    constexpr uint32_t META_PAGE_NUM = 0; // резерв под 0 стр пот метаданные дерева

    class Pager {
    private: 
        std::string filepath_; // путь к файлы
        std::fstream file_; // файловый птоток для чтения и записи
        uint32_t num_pages_; // кол-во стр в файле

    public: 
        explicit Pager(const std::string& filepath);


        ~Pager(); // сброс буфера и закрытие файла
        // запрет на копирование 
        Pager(const Pager&) = delete;
        Pager& operator = (const Pager&) = delete;

        std::vector<char> read_page(uint32_t page_num); // read стр с диска по индексу

        void write_page(uint32_t page_num, const std::vector<char>& page_data); // запись стр на диск по ее индексу

        uint32_t allocate_page(); // увел. размера файла на 1 стр и возврат ее индекса

        void free_page(uint32_t page_num); // пометка стр как свободн для повтор исп.

        uint32_t get_num_pages() const; //возврат стр в файле тек
    };
}