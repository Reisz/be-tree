// ---------------------------------------------------------------------------------------------------
#include "imlab/segment_file.h"
#include "imlab/buffer_manager.h"
// ---------------------------------------------------------------------------------------------------
namespace imlab {

namespace {

    void load_page(Page &p, size_t page_size) {
        SegmentFile f{p.page_id, page_size};
        f.read(p.data.get());
    }

    void save_page(Page &p, size_t page_size) {
        SegmentFile f{p.page_id, page_size};
        f.write(p.data.get());
    }

}  // namespace

BufferFix::~BufferFix() {
    unfix();
}

void *BufferFix::data() {
    if (page)
        return page->data.get();
    return nullptr;
}

const void *BufferFix::data() const {
    if (page)
        return page->data.get();
    return nullptr;
}

void BufferFix::set_dirty() {
    page->data_state = Page::Dirty;
}

void BufferFix::unfix() {
    if (page)
        manager->unfix(page);
    page = nullptr;
}

// ---------------------------------------------------------------------------------------------------

BufferManager::BufferManager(size_t page_size, size_t page_count)
: page_size(page_size), page_count(page_count) {}

BufferManager::~BufferManager() {
    for (auto &entry : pages) {
        if (entry.second.data_state == Page::Dirty)
            save_page(entry.second, page_size);
    }
}

Page *BufferManager::fix(uint64_t page_id, bool exclusive) {
    std::unique_lock<std::mutex> lock(mutex);

    Page *p = nullptr;
    while (!p) {
        auto it = pages.find(page_id);
        if (it != pages.end()) {
            p = try_fix_existing(it, exclusive);
        } else {
            p = try_fix_new(pages.emplace_hint(it,
                std::piecewise_construct,
                std::forward_as_tuple(page_id),
                std::forward_as_tuple(page_id, page_size)), exclusive);
        }
    }

    return p;
}

void BufferManager::unfix(Page *page) {
    std::unique_lock<std::mutex> lock(mutex);
    page->unfix();
}

Page *BufferManager::try_fix_existing(PageMap::iterator it, bool exclusive) {
    Page &p = it->second;

    // page is currently writing back & getting deleted -> wait & try again
    if (p.data_state == Page::Writing) {
      // p->cv->wait(lock);
      return nullptr;
    }

    // page is loaded or loading -> check if fix is possible
    if (!p.can_fix(exclusive)) {
      // p->cv->wait(lock);
      return nullptr;
    }

    p.fix(exclusive);
    add_to_lru(&p);

    // page was still reading -> wait
    if (p.data_state == Page::Reading) {
        // TODO p->cv->wait(lock);
    }

    return &p;
}

Page *BufferManager::try_fix_new(PageMap::iterator it, bool exclusive) {
    Page &p = it->second;

    if (!try_reserve_space()) {
        pages.erase(it);
        throw buffer_full_error();
    }

    p.fix(exclusive);
    add_to_fifo(&p);

    // TODO unlock
    load_page(p, page_size);
    // TODO relock
    p.data_state = Page::Clean;
    // TODO notify?

    return &p;
}

bool BufferManager::try_reserve_space() {
    if (loaded_page_count < page_count) {
        ++loaded_page_count;
    } else {
        Page *steal = find_unfixed();
        if (!steal)
            return false;

        remove_from_queues(steal);
        if (steal->data_state == Page::Dirty) {
            steal->data_state = Page::Writing;
            // TODO unlock
            save_page(*steal, page_size);
            // TODO relock
            // TODO notify ?
        }

        pages.erase(steal->page_id);
    }

    return true;
}

void BufferManager::add_to_fifo(Page *p) {
    remove_from_queues(p);

    if (!fifo_tail) {
        fifo_head = fifo_tail = p;
    } else {
        p->prev = fifo_tail;
        fifo_tail->next = p;
        fifo_tail = p;
    }
}

void BufferManager::add_to_lru(Page *p) {
    remove_from_queues(p);

    if (!lru_tail) {
      lru_head = lru_tail = p;
    } else {
      p->prev = lru_tail;
      lru_tail->next = p;
      lru_tail = p;
    }
}

void BufferManager::remove_from_queues(Page *p) {
    // update heads & tails
    if (p == lru_head) {
      lru_head = p->next;
    }
    if (p == lru_tail) {
      lru_tail = p->prev;
    }
    if (p == fifo_head) {
      fifo_head = p->next;
    }
    if (p == fifo_tail) {
      fifo_tail = p->prev;
    }

    // update neighbors
    if (p->next) {
      p->next->prev = p->prev;
    }
    if (p->prev) {
      p->prev->next = p->next;
    }

    // invalidate
    p->prev = p->next = nullptr;
}

Page *BufferManager::find_unfixed() {
    Page *p = fifo_head;

    if (p) {
        do {
            if (p->fix_count == 0)
                break;
        } while ((p = p->next));
    }

    if (!p && (p = lru_head)) {
        do {
            if (p->fix_count == 0)
                break;
        } while ((p = p->next));
    }

    return p;
}

}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
