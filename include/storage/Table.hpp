#pragma once

#include "storage/RecordManager.hpp"
#include "index/b_star_plus_tree.hpp"
#include <string>
#include <vector>

namespace storage {

class Table {
public:
    Table(const std::string& name, Pager& data_pager, db_engine::DbIndex& primary_index);

    ~Table() = default;

    Table(const Table&) = delete;
    Table& operator=(const Table&) = delete;

    uint32_t insert_record(const Record& record);

    bool find_by_id(uint32_t id, Record& out_record);

    std::vector<Record> scan_all();

    const std::string& get_name() const { return name_; }

private:
    std::string name_;
    RecordManager record_manager_;
    db_engine::DbIndex& index_; 
};

} 