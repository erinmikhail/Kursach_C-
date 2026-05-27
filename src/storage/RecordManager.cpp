#include "storage/RecordManager.hpp"
#include <cstring>
#include <stdexcept>

namespace storage {

RecordManager::RecordManager(Pager& pager, uint32_t num_columns) 
    : pager_(pager), num_columns_(num_columns), total_records_(0) {
    record_size_ = sizeof(bool) + 2 * sizeof(uint64_t) + num_columns_ * sizeof(uint32_t);
    
    if (pager_.get_num_pages() == 0) {
        pager_.allocate_page();
        save_metadata();
    } else {
        load_metadata();
    }
}

void RecordManager::load_metadata() {
    std::vector<char> page0 = pager_.read_page(0);
    std::memcpy(&total_records_, &page0[0], sizeof(total_records_));
}

void RecordManager::save_metadata() {
    std::vector<char> page0 = pager_.read_page(0);
    std::memcpy(&page0[0], &total_records_, sizeof(total_records_));
    pager_.write_page(0, page0);
}

uint32_t RecordManager::records_per_page() const {
    return PAGE_SIZE / record_size_;
}

std::pair<uint32_t, size_t> RecordManager::get_location(uint32_t record_index) const {
    uint32_t page_num = 1 + (record_index / records_per_page());
    size_t offset = (record_index % records_per_page()) * record_size_;
    return {page_num, offset};
}

Record RecordManager::get_record(uint32_t record_index) {
    if (record_index >= total_records_) throw std::out_of_range("");
    
    auto loc = get_location(record_index);
    std::vector<char> page_data = pager_.read_page(loc.first);
    
    Record rec;
    std::memcpy(&rec.is_deleted, &page_data[loc.second], sizeof(bool));
    std::memcpy(&rec.ts_start, &page_data[loc.second + sizeof(bool)], sizeof(uint64_t));
    std::memcpy(&rec.ts_end, &page_data[loc.second + sizeof(bool) + sizeof(uint64_t)], sizeof(uint64_t));
    
    rec.values.resize(num_columns_);
    std::memcpy(rec.values.data(), &page_data[loc.second + sizeof(bool) + 2 * sizeof(uint64_t)], num_columns_ * sizeof(uint32_t));
    
    return rec;
}

void RecordManager::write_record(uint32_t record_index, const Record& record) {
    if (record_index >= total_records_) throw std::out_of_range("");
    
    auto loc = get_location(record_index);
    std::vector<char> page_data = pager_.read_page(loc.first);
    
    std::memcpy(&page_data[loc.second], &record.is_deleted, sizeof(bool));
    std::memcpy(&page_data[loc.second + sizeof(bool)], &record.ts_start, sizeof(uint64_t));
    std::memcpy(&page_data[loc.second + sizeof(bool) + sizeof(uint64_t)], &record.ts_end, sizeof(uint64_t));
    std::memcpy(&page_data[loc.second + sizeof(bool) + 2 * sizeof(uint64_t)], record.values.data(), num_columns_ * sizeof(uint32_t));
    
    pager_.write_page(loc.first, page_data);
}

uint32_t RecordManager::append_record(const Record& record) {
    uint32_t new_index = total_records_;
    auto loc = get_location(new_index);
    
    while (loc.first >= pager_.get_num_pages()) {
        pager_.allocate_page();
    }
    
    total_records_++;
    save_metadata();
    write_record(new_index, record);
    
    return new_index;
}

uint32_t RecordManager::get_total_records() const {
    return total_records_;
}

}