#include "storage/Table.hpp"
#include <stdexcept>

namespace storage {

Table::Table(const std::string& name, Pager& data_pager, db_engine::DbIndex& primary_index, uint32_t num_columns, int indexed_col)
    : name_(name), record_manager_(data_pager, num_columns), index_(primary_index), indexed_col_(indexed_col) {}

uint32_t Table::insert_record(const Record& record) {
    if (indexed_col_ != -1) {
        uint32_t key = record.values[indexed_col_];
        auto existing = index_.find(key);
        if (!existing.empty()) {
            throw std::runtime_error("");
        }
    }

    uint32_t row_id = record_manager_.append_record(record);

    if (indexed_col_ != -1) {
        index_.insert(record.values[indexed_col_], row_id);
    }

    return row_id;
}

void Table::update_record(uint32_t row_id, const Record& record) {
    record_manager_.write_record(row_id, record);
}

void Table::delete_record(uint32_t row_id) {
    Record rec = record_manager_.get_record(row_id);
    rec.is_deleted = true;
    record_manager_.write_record(row_id, rec);
}

bool Table::find_by_id(uint32_t id, Record& out_record) {
    std::vector<db_engine::RowID> row_ids = index_.find(id);
    if (row_ids.empty()) {
        return false;
    }
    out_record = record_manager_.get_record(row_ids[0]);
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

void Table::add_to_index(uint32_t key, uint32_t row_id) {
    index_.insert(key, row_id);
}

void Table::remove_from_index(uint32_t key, uint32_t row_id) {
    index_.remove(key, row_id);
}

}