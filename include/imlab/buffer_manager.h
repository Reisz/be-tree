// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#ifndef INCLUDE_IMLAB_BUFFER_MANAGER_H_
#define INCLUDE_IMLAB_BUFFER_MANAGER_H_

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

struct Page;

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
    const void *getData() const;
    // get pointer for exclusive fix
    void *getData();

    // mark page for writeback
    inline void setDirty() { dirty = true; }
    // declare this fix unecessary
    void unfix();

 private:
    // data management
    explicit BufferFix(Page *page, BufferManager *manager)
        : page(page), manager(manager) {}
    BufferManager *manager;
    Page *page = nullptr;
    bool dirty = false;
};

class BufferManager {
    friend class BufferFix;
 public:
    explicit BufferManager(size_t page_size, size_t page_count);
    ~BufferManager();

    const BufferFix fix(uint64_t page_id);
    BufferFix fix_exclusive(uint64_t page_id);
    void unfix(BufferFix &&page);

 private:
    void unfix(Page *page, bool dirty);
    void prepeare_fix();

    size_t page_size, page_count;
    std::atomic<size_t> loaded_page_count = 0;
    Page *fifo_head, *fifo_tail, *lru_head, *lru_tail;

    std::mutex mutex;
    std::unordered_map<uint64_t, Page> pages;
};

}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#endif  // INCLUDE_IMLAB_BUFFER_MANAGER_H_
