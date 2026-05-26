#include "storage/Table.hpp"
#include <stdexcept>
#include <iostream>

namespace storage {

Table::Table(const std::string& name, Pager& data_pager, db_engine::DbIndex& primary_index)
    : name_(name), record_manager_(data_pager), index_(primary_index) {}

uint32_t Table::insert_record(const Record& record) {
    std::vector<db_engine::RowID> existing = index_.find(record.id);

    if (!existing.empty()) {
        throw std::runtime_error("Ошибка уникальности: Запись с ID " + std::to_string(record.id) + " уже существует.");
    }

    uint32_t row_id = record_manager_.append_record(record);

    index_.insert(record.id, row_id);

    return row_id;
}

bool Table::find_by_id(uint32_t id, Record& out_record) {
    std::vector<db_engine::RowID> row_ids = index_.find(id);
    
    if (row_ids.empty()) {
        return false; 
    }

    uint32_t target_row_id = row_ids[0];

    out_record = record_manager_.get_record(target_row_id);
    return true;
}

std::vector<Record> Table::scan_all() {
    std::vector<Record> result;
    uint32_t total = record_manager_.get_total_records();

    for (uint32_t i = 0; i < total; ++i) {
        result.push_back(record_manager_.get_record(i));
    }
    return result;
}

} 