// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#ifndef INCLUDE_IMLAB_BUFFER_MANAGER_H_
#define INCLUDE_IMLAB_BUFFER_MANAGER_H_

#include <cstddef>
#include <vector>
#include <unordered_map>
#include <mutex>
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
    BufferFix(const BufferFix &) = delete;
    BufferFix &operator=(const BufferFix &) = delete;
    inline BufferFix(BufferFix &&o) noexcept {
        *this = std::move(o);
    }
    constexpr BufferFix &operator=(BufferFix &&o) noexcept {
        if (this != &o) {
            unfix();

            this->manager = o.manager;
            this->page = o.page;

            o.page = nullptr;
        }

        return *this;
    }

    // unfix page automatically
    ~BufferFix();
    void unfix();

    // get pointer for non exclusive fix
    const std::byte *data() const;

 protected:
    constexpr BufferFix(Page *page, BufferManager *manager)
        : page(page), manager(manager) {}
    Page *page = nullptr;

 private:
    BufferManager *manager;
};

class BufferFixExclusive : public BufferFix {
    friend class BufferManager;
 public:
    BufferFixExclusive() = default;

    // get pointer for exclusive fix
    std::byte *data();
    // mark page for writeback
    void set_dirty();
 private:
    constexpr BufferFixExclusive(Page *page, BufferManager *manager)
        : BufferFix(page, manager) {}
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
    inline BufferFix fix(uint64_t page_id) {
        return BufferFix(fix(page_id, false), this);
    }
    inline BufferFixExclusive fix_exclusive(uint64_t page_id) {
        return BufferFixExclusive(fix(page_id, true), this);
    }

    // testing interface, not linked in prod code
    const std::vector<uint64_t> get_fifo() const;
    const std::vector<uint64_t> get_lru() const;

 private:
    // fix management
    Page *fix(uint64_t page_id, bool exclusive);
    void unfix(Page *page);

    using PageMap = std::unordered_map<uint64_t, Page>;
    PageMap pages;

    Page *try_fix_existing(PageMap::iterator it, bool exclusive);
    Page *try_fix_new(PageMap::iterator it, bool exclusive);
    bool try_reserve_space();

    const size_t page_size, page_count;
    size_t loaded_page_count = 0;

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
