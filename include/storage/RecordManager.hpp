#pragma once

#include "storage/Pager.hpp"
#include <cstdint>
#include <vector>
#include <utility>

namespace storage {

// Структура нашей фиксированной записи
struct Record {
    uint32_t id;          // Числовой ID пользователя
    uint32_t name_id;     // ID имени из StringPool
};

class RecordManager {
private:
    Pager& pager_; // Ссылка на наш менеджер страниц
    
    // Сколько записей помещается в одну страницу 4096 байт?
    // 4096 / sizeof(Record) = 512 записей.
    static constexpr size_t RECORDS_PER_PAGE = PAGE_SIZE / sizeof(Record);
    
    uint32_t total_records_; // Общее количество записей в таблице

    std::pair<uint32_t, size_t> get_location(uint32_t record_index) const;

    static constexpr size_t META_OFFSET_TOTAL_RECORDS = 0; // смещение в странице 0
    // Загружает total_records_ из страницы 0
    void load_metadata();
    // Сохраняет текущее total_records_ в страницу 0
    void save_metadata();

public:
    // Конструктор: принимает ссылку на уже созданный Pager
    explicit RecordManager(Pager& pager);

    // Читает запись по её абсолютному порядковому номеру (0, 1, 2...)
    Record get_record(uint32_t record_index);

    // Записывает (или обновляет) запись по указанному индексу
    void write_record(uint32_t record_index, const Record& record);

    // Добавляет новую запись в самый конец таблицы
    // Возвращает индекс новой записи
    uint32_t append_record(const Record& record);

    // Возвращает общее количество записей в таблице
    uint32_t get_total_records() const;
};

} // namespace storage