#include "storage/BufferPool.hpp"

#include <utility>

namespace storage {

namespace {

void remove_from_lru(std::list<size_t>& lru_list, size_t frame_id) {
    lru_list.remove(frame_id);
}

} // namespace

BufferPool::BufferPool(Pager& pager, size_t pool_size)
    : pager_(pager), pool_size_(pool_size), pages_(pool_size) {
    for (size_t frame_id = 0; frame_id < pool_size_; ++frame_id) {
        free_frames_.push_back(frame_id);
    }
}

BufferPool::~BufferPool() {
    flush_all_pages();
}

bool BufferPool::evict_page(size_t* out_frame_id) {
    if (out_frame_id == nullptr) {
        throw std::invalid_argument("out_frame_id must not be null");
    }

    if (!free_frames_.empty()) {
        *out_frame_id = free_frames_.front();
        free_frames_.pop_front();
        return true;
    }

    if (lru_list_.empty()) {
        return false;
    }

    const size_t frame_id = lru_list_.front();
    lru_list_.pop_front();

    Page& victim = pages_[frame_id];
    if (victim.is_dirty) {
        pager_.write_page(victim.page_id, victim.data);
        victim.is_dirty = false;
    }

    page_table_.erase(victim.page_id);
    victim.pin_count = 0;
    *out_frame_id = frame_id;
    return true;
}

Page* BufferPool::fetch_page(uint32_t page_id) {
    const auto page_it = page_table_.find(page_id);
    if (page_it != page_table_.end()) {
        Page& page = pages_[page_it->second];
        if (page.pin_count == 0) {
            remove_from_lru(lru_list_, page_it->second);
        }
        ++page.pin_count;
        return &page;
    }

    if (page_id >= pager_.get_num_pages()) {
        throw std::out_of_range("Page does not exist on disk");
    }

    size_t frame_id = 0;
    if (!evict_page(&frame_id)) {
        return nullptr;
    }

    Page& page = pages_[frame_id];
    page.data = pager_.read_page(page_id);
    page.page_id = page_id;
    page.pin_count = 1;
    page.is_dirty = false;
    page_table_[page_id] = frame_id;

    return &page;
}

void BufferPool::unpin_page(uint32_t page_id, bool is_dirty) {
    const auto page_it = page_table_.find(page_id);
    if (page_it == page_table_.end()) {
        throw std::out_of_range("Page is not loaded in buffer pool");
    }

    Page& page = pages_[page_it->second];
    if (page.pin_count <= 0) {
        throw std::logic_error("Cannot unpin page with zero pin count");
    }

    if (is_dirty) {
        page.is_dirty = true;
    }

    --page.pin_count;
    if (page.pin_count == 0) {
        remove_from_lru(lru_list_, page_it->second);
        lru_list_.push_back(page_it->second);
    }
}

Page* BufferPool::new_page(uint32_t& out_page_id) {
    out_page_id = pager_.allocate_page();

    size_t frame_id = 0;
    if (!evict_page(&frame_id)) {
        throw std::runtime_error("No available frame for a new page");
    }

    Page& page = pages_[frame_id];
    page.data.assign(PAGE_SIZE, 0);
    page.page_id = out_page_id;
    page.pin_count = 1;
    page.is_dirty = false;
    page_table_[out_page_id] = frame_id;

    return &page;
}

void BufferPool::flush_page(uint32_t page_id) {
    const auto page_it = page_table_.find(page_id);
    if (page_it == page_table_.end()) {
        throw std::out_of_range("Page is not loaded in buffer pool");
    }

    Page& page = pages_[page_it->second];
    pager_.write_page(page.page_id, page.data);
    page.is_dirty = false;
}

void BufferPool::flush_all_pages() {
    for (auto& [page_id, frame_id] : page_table_) {
        (void)page_id;
        Page& page = pages_[frame_id];
        if (page.is_dirty) {
            pager_.write_page(page.page_id, page.data);
            page.is_dirty = false;
        }
    }
}

} // namespace storage
