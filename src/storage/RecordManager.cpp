#include "storage/RecordManager.hpp"
#include <cstring>
#include <stdexcept>

namespace storage {

RecordManager::RecordManager(Pager& pager) : pager_(pager) {
    if (pager_.get_num_pages() == 0) {
        pager_.allocate_page();
        total_records_ = 0;
        save_metadata();
    } else {
        load_metadata();
    }
}

void RecordManager::load_metadata() {
    std::vector<char> page0 = pager_.read_page(0);
    std::memcpy(&total_records_, &page0[META_OFFSET_TOTAL_RECORDS], sizeof(total_records_));
}

void RecordManager::save_metadata() {
    std::vector<char> page0 = pager_.read_page(0);
    std::memcpy(&page0[META_OFFSET_TOTAL_RECORDS], &total_records_, sizeof(total_records_));
    pager_.write_page(0, page0);
}

std::pair<uint32_t, size_t> RecordManager::get_location(uint32_t record_index) const {
    uint32_t page_num = 1 + (record_index / RECORDS_PER_PAGE);
    size_t offset = (record_index % RECORDS_PER_PAGE) * sizeof(Record);
    return {page_num, offset};
}

Record RecordManager::get_record(uint32_t record_index) {
    if (record_index >= total_records_) {
        throw std::out_of_range("Record index out of range");
    }
    
    auto [page_num, offset] = get_location(record_index);
    std::vector<char> page_data = pager_.read_page(page_num);
    if (offset + sizeof(Record) > PAGE_SIZE) {
        throw std::runtime_error("Offset exceeds page size");
    }

    Record rec;
    std::memcpy(&rec, &page_data[offset], sizeof(Record));
    return rec;
}

void RecordManager::write_record(uint32_t record_index, const Record& record) {
    if (record_index > total_records_) {
        throw std::out_of_range("Record index out of range");
    }
    
    auto [page_num, offset] = get_location(record_index);
    std::vector<char> page_data = pager_.read_page(page_num);
    if (offset + sizeof(Record) > PAGE_SIZE) {
        throw std::runtime_error("Offset exceeds page size");
    }

    std::memcpy(&page_data[offset], &record, sizeof(Record));
    pager_.write_page(page_num, page_data);
}

uint32_t RecordManager::append_record(const Record& record) {
    uint32_t new_index = total_records_;
    auto [target_page, _] = get_location(new_index);
    while (target_page >= pager_.get_num_pages()) {
        pager_.allocate_page();
    }
    write_record(new_index, record);
    total_records_++;
    save_metadata();
    return new_index;
}

uint32_t RecordManager::get_total_records() const {
    return total_records_;
}

} // namespace storage