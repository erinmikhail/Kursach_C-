#pragma once
#include "storage/Pager.hpp"
#include <cstdint>
#include <vector>
#include <utility>

namespace storage {

// Динамическая структура записи
struct Record {
    bool is_deleted;   // Маркер удаленной записи (tombstone)
    uint64_t ts_start; // Время создания
    uint64_t ts_end;   // Время удаления
    std::vector<uint32_t> values;    // Значения (числа или ID из StringPool)
};

class RecordManager {
private:
    Pager& pager_;
    uint32_t num_columns_;
    uint32_t record_size_;
    uint32_t total_records_;

    static constexpr size_t META_OFFSET_TOTAL_RECORDS = 0;

    void load_metadata();
    void save_metadata();
    uint32_t records_per_page() const;
    std::pair<uint32_t, size_t> get_location(uint32_t record_index) const;

public:
    RecordManager(Pager& pager, uint32_t num_columns);

    Record get_record(uint32_t record_index);
    void write_record(uint32_t record_index, const Record& record);
    uint32_t append_record(const Record& record);
    uint32_t get_total_records() const;
};

} 