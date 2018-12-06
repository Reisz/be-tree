// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#ifndef INCLUDE_IMLAB_BUFFER_MANAGER_H_
#define INCLUDE_IMLAB_BUFFER_MANAGER_H_

#include <stdint.h>
#include <stddef.h>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <memory>
#include "imlab/buffer_manager_page.h"
// ---------------------------------------------------------------------------------------------------
namespace imlab {

class BufferManager;

// owned representation of a fix on a page
class BufferFix {
    friend class BufferManager;
 public:
    BufferFix() = default;
    BufferFix(BufferFix &) = delete;
    BufferFix(BufferFix &&) = default;

    // unfix page automatically
    ~BufferFix();

    // get pointer for non exclusive fix
    const void *data() const;
    // get pointer for exclusive fix
    void *data();

    // mark page for writeback
    void set_dirty();
    // unfix page, automatically called on destruction
    void unfix();

 private:
    // data management
    explicit BufferFix(Page *page, BufferManager *manager)
        : page(page), manager(manager) {}
    BufferManager *manager;
    Page *page = nullptr;
};

class buffer_full_error : public std::exception {
 public:
    const char* what() const noexcept override {
        return "buffer is full";
    }
};

class BufferManager {
    friend class BufferFix;
 public:
    explicit BufferManager(size_t page_size, size_t page_count);
    ~BufferManager();

    // fix interface
    inline const BufferFix fix(uint64_t page_id) {
        return BufferFix(fix(page_id, false), this);
    }
    BufferFix fix_exclusive(uint64_t page_id) {
        return BufferFix(fix(page_id, true), this);
    }

    // testing interface, not linked in prod code
    const std::vector<uint64_t> get_fifo() const;
    const std::vector<uint64_t> get_lru() const;

 private:
    using PageMap = std::unordered_map<uint64_t, Page>;

    // fix management
    Page *fix(uint64_t page_id, bool exclusive);
    void unfix(Page *page);

    Page *try_fix_existing(PageMap::iterator it, bool exclusive);
    Page *try_fix_new(PageMap::iterator it, bool exclusive);
    bool try_reserve_space();

    size_t page_size, page_count, loaded_page_count = 0;
    PageMap pages;

    // queue management
    void add_to_fifo(Page *p);
    void add_to_lru(Page *p);
    void remove_from_queues(Page *p);
    Page *find_unfixed();
    Page *fifo_head = nullptr, *fifo_tail = nullptr, *lru_head = nullptr, *lru_tail = nullptr;

    // global mutex
    // TODO finer lock granularity
    std::mutex mutex;
};

}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#endif  // INCLUDE_IMLAB_BUFFER_MANAGER_H_
