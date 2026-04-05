#pragma once

#include "storage/Pager.hpp"
#include <unordered_map>
#include <list>
#include <vector>
#include <cstdint>
#include <stdexcept>

namespace storage {

// Фрейм — это обертка над 4096 байтами, которая хранится в оперативной памяти
struct Page {
    std::vector<char> data; // Сами данные (4 КБ)
    uint32_t page_id = 0;   // Какой странице на диске это соответствует
    int pin_count = 0;      // Сколько процессов сейчас читают/пишут эту страницу
    bool is_dirty = false;  // Была ли страница изменена (нужно ли ее сохранять)

    Page() : data(PAGE_SIZE, 0) {}
};

class BufferPool {
private:
    Pager& pager_;
    size_t pool_size_; // Максимальный размер кэша (например, 10 страниц)

    std::vector<Page> pages_; // Массив памяти под страницы (кэш)
    std::list<size_t> free_frames_; // Список свободных индексов в массиве pages_

    // Таблица страниц: Page ID -> Индекс в массиве pages_ (Frame ID)
    std::unordered_map<uint32_t, size_t> page_table_;

    // Алгоритм вытеснения LRU (Least Recently Used)
    // Хранит индексы фреймов. Самый старый (кандидат на удаление) находится в front()
    std::list<size_t> lru_list_; 

    // Внутренний метод: найти страницу для вытеснения, если кэш забит
    bool evict_page(size_t* out_frame_id);

public:
    BufferPool(Pager& pager, size_t pool_size);
    ~BufferPool();

    BufferPool(const BufferPool&) = delete;
    BufferPool& operator=(const BufferPool&) = delete;

    // Запросить страницу (если ее нет в памяти - качает с диска)
    // ВАЖНО: Увеличивает pin_count!
    Page* fetch_page(uint32_t page_id);

    // Сказать пулу, что мы закончили работу со страницей
    // is_dirty = true означает, что мы изменили данные
    void unpin_page(uint32_t page_id, bool is_dirty);

    // Выделить новую страницу на диске и сразу загрузить в кэш
    Page* new_page(uint32_t& out_page_id);

    // Принудительно сбросить конкретную страницу на диск
    void flush_page(uint32_t page_id);

    // Сбросить все измененные страницы (вызывается при штатном закрытии БД)
    void flush_all_pages();
};

}