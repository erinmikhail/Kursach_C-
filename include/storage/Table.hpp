#pragma once
#include "storage/RecordManager.hpp"
#include "index/b_star_plus_tree.hpp" 
#include <string>
#include <vector>

namespace storage {

class Table {
public:
    Table(const std::string& name, Pager& data_pager, db_engine::DbIndex& primary_index, uint32_t num_columns, int indexed_col);
    ~Table() = default;

    Table(const Table&) = delete;
    Table& operator=(const Table&) = delete;

    uint32_t insert_record(const Record& record);
    void update_record(uint32_t row_id, const Record& record);
    void delete_record(uint32_t row_id);

    bool find_by_id(uint32_t id, Record& out_record);
    std::vector<Record> scan_all();

    void add_to_index(uint32_t key, uint32_t row_id);
    void remove_from_index(uint32_t key, uint32_t row_id);
    int get_indexed_col() const { return indexed_col_; }
    const std::string& get_name() const { return name_; }

private:
    std::string name_;
    RecordManager record_manager_;
    db_engine::DbIndex& index_;
    int indexed_col_; // Индекс колонки, по которой построено дерево (-1 если нет)
};

}