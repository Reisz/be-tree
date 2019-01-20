// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#ifndef SRC_BUFFER_MANAGER_HPP_
#define SRC_BUFFER_MANAGER_HPP_
// ---------------------------------------------------------------------------------------------------
#include "imlab/segment_file.h"
#include "imlab/buffer_manager.h"
// ---------------------------------------------------------------------------------------------------
namespace imlab {

BUFFER_MANAGER_TEMPL constexpr BUFFER_MANAGER_CLASS::BufferManager(size_t page_count)
: page_count(page_count) {}

BUFFER_MANAGER_TEMPL BUFFER_MANAGER_CLASS::~BufferManager() {
    for (auto &entry : pages) {
        if (entry.second.data_state == Page::Dirty)
            save_page(entry.second);
    }
}

BUFFER_MANAGER_TEMPL typename BUFFER_MANAGER_CLASS::Fix BUFFER_MANAGER_CLASS::fix(uint64_t page_id) {
    return Fix(fix(page_id, false), this);
}

BUFFER_MANAGER_TEMPL typename BUFFER_MANAGER_CLASS::ExclusiveFix BUFFER_MANAGER_CLASS::fix_exclusive(uint64_t page_id) {
    return ExclusiveFix(fix(page_id, true), this);
}

BUFFER_MANAGER_TEMPL typename BUFFER_MANAGER_CLASS::Page *BUFFER_MANAGER_CLASS::fix(uint64_t page_id, bool exclusive) {
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
                std::forward_as_tuple(page_id)), exclusive);
        }
    }

    return p;
}

BUFFER_MANAGER_TEMPL bool BUFFER_MANAGER_CLASS::in_memory(uint64_t page_id) const {
    auto it = pages.find(page_id);
    if (it != pages.end())
        return it->second.data_state != Page::Writing;
    return false;
}

BUFFER_MANAGER_TEMPL bool BUFFER_MANAGER_CLASS::is_dirty(uint64_t page_id) const {
    auto it = pages.find(page_id);
    if (it != pages.end())
        return it->second.data_state == Page::Dirty;
    return false;
}

BUFFER_MANAGER_TEMPL void BUFFER_MANAGER_CLASS::unfix(Page *page) {
    std::unique_lock<std::mutex> lock(mutex);
    page->unfix();
}

BUFFER_MANAGER_TEMPL
typename BUFFER_MANAGER_CLASS::Page *BUFFER_MANAGER_CLASS::try_fix_existing(typename BUFFER_MANAGER_CLASS::PageMap::iterator it, bool exclusive) {
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

BUFFER_MANAGER_TEMPL
typename BUFFER_MANAGER_CLASS::Page *BUFFER_MANAGER_CLASS::try_fix_new(typename BUFFER_MANAGER_CLASS::PageMap::iterator it, bool exclusive) {
    Page &p = it->second;

    if (!try_reserve_space()) {
        pages.erase(it);
        throw buffer_full_error();
    }

    p.fix(exclusive);
    add_to_fifo(&p);

    // TODO unlock
    load_page(p);
    // TODO relock
    p.data_state = Page::Clean;
    // TODO notify?

    return &p;
}

BUFFER_MANAGER_TEMPL bool BUFFER_MANAGER_CLASS::try_reserve_space() {
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
            save_page(*steal);
            // TODO relock
            // TODO notify ?
        }

        pages.erase(steal->page_id);
    }

    return true;
}

BUFFER_MANAGER_TEMPL void BUFFER_MANAGER_CLASS::add_to_fifo(Page *p) {
    remove_from_queues(p);

    if (!fifo_tail) {
        fifo_head = fifo_tail = p;
    } else {
        p->prev = fifo_tail;
        fifo_tail->next = p;
        fifo_tail = p;
    }
}

BUFFER_MANAGER_TEMPL void BUFFER_MANAGER_CLASS::add_to_lru(Page *p) {
    remove_from_queues(p);

    if (!lru_tail) {
      lru_head = lru_tail = p;
    } else {
      p->prev = lru_tail;
      lru_tail->next = p;
      lru_tail = p;
    }
}

BUFFER_MANAGER_TEMPL void BUFFER_MANAGER_CLASS::remove_from_queues(Page *p) {
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

BUFFER_MANAGER_TEMPL typename BUFFER_MANAGER_CLASS::Page *BUFFER_MANAGER_CLASS::find_unfixed() {
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

BUFFER_MANAGER_TEMPL void BUFFER_MANAGER_CLASS::load_page(Page &p) {
    SegmentFile f{p.page_id, page_size};
    f.read(p.data.get());
}

BUFFER_MANAGER_TEMPL void BUFFER_MANAGER_CLASS::save_page(const Page &p) {
    SegmentFile f{p.page_id, page_size};
    f.write(p.data.get());
}

// ---------------------------------------------------------------------------------------------------

BUFFER_MANAGER_TEMPL constexpr BUFFER_MANAGER_CLASS::Page::Page(uint64_t page_id)
    : page_id(page_id), data(new std::byte[page_size]) {}

BUFFER_MANAGER_TEMPL bool BUFFER_MANAGER_CLASS::Page::can_fix(bool exclusive) {
    if (exclusive)
        return fix_count == 0;
    return fix_count >= 0;
}

BUFFER_MANAGER_TEMPL void BUFFER_MANAGER_CLASS::Page::fix(bool exclusive) {
    if (exclusive)
        fix_count = -1;
    else
        fix_count++;
}

BUFFER_MANAGER_TEMPL void BUFFER_MANAGER_CLASS::Page::unfix() {
    if (fix_count > 0)
        --fix_count;
    else
        fix_count = 0;
}

// ---------------------------------------------------------------------------------------------------

BUFFER_MANAGER_TEMPL constexpr BUFFER_MANAGER_CLASS::Fix::Fix(Page *page, BufferManager *manager) noexcept
    : page(page), manager(manager) {}

BUFFER_MANAGER_TEMPL constexpr BUFFER_MANAGER_CLASS::Fix::Fix(Fix &&o) noexcept {
    *this = std::move(o);
}

BUFFER_MANAGER_TEMPL constexpr typename BUFFER_MANAGER_CLASS::Fix &BUFFER_MANAGER_CLASS::Fix::operator=(Fix &&o) noexcept {
    if (this != &o) {
        unfix();

        this->manager = o.manager;
        this->page = o.page;

        o.page = nullptr;
    }

    return *this;
}

BUFFER_MANAGER_TEMPL constexpr BUFFER_MANAGER_CLASS::ExclusiveFix::ExclusiveFix(Page *page, BufferManager *manager) noexcept
        : Fix(page, manager) {}

BUFFER_MANAGER_TEMPL BUFFER_MANAGER_CLASS::Fix::~Fix() {
    unfix();
}

BUFFER_MANAGER_TEMPL void BUFFER_MANAGER_CLASS::Fix::unfix() {
    if (page)
    manager->unfix(page);
    page = nullptr;
}

BUFFER_MANAGER_TEMPL uint64_t BUFFER_MANAGER_CLASS::Fix::page_id() const {
    return page->page_id;
}

BUFFER_MANAGER_TEMPL const std::byte *BUFFER_MANAGER_CLASS::Fix::data() const {
    if (page)
        return page->data.get();
    return nullptr;
}

BUFFER_MANAGER_TEMPL std::byte *BUFFER_MANAGER_CLASS::ExclusiveFix::data() {
    if (this->page)
        return this->page->data.get();
    return nullptr;
}

BUFFER_MANAGER_TEMPL template<typename T> const T *BUFFER_MANAGER_CLASS::Fix::as() const {
    static_assert(sizeof(T) <= page_size);
    return reinterpret_cast<const T*>(data());
}

BUFFER_MANAGER_TEMPL template<typename T> T *BUFFER_MANAGER_CLASS::ExclusiveFix::as() {
    static_assert(sizeof(T) <= page_size);
    return reinterpret_cast<T*>(data());
}

BUFFER_MANAGER_TEMPL void BUFFER_MANAGER_CLASS::ExclusiveFix::set_dirty() {
    this->page->data_state = Page::Dirty;
}

}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#endif  // SRC_BUFFER_MANAGER_HPP_
