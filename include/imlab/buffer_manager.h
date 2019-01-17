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
// ---------------------------------------------------------------------------------------------------
namespace imlab {

#define BUFFER_MANAGER_TEMPL \
    template<size_t page_size>
#define BUFFER_MANAGER_CLASS \
    BufferManager<page_size>

BUFFER_MANAGER_TEMPL class BufferManager {
    struct Page;
 public:
     // owned representation of a fix on a page
    class Fix;
    class ExclusiveFix;

    explicit constexpr BufferManager(size_t page_count);
    ~BufferManager();

    // fix interface
    Fix fix(uint64_t page_id);
    ExclusiveFix fix_exclusive(uint64_t page_id);

    // testing interface, not linked in prod code
    const std::vector<uint64_t> get_fifo() const;
    const std::vector<uint64_t> get_lru() const;

 private:
    // fix management
    Page *fix(uint64_t page_id, bool exclusive);
    void unfix(Page *page);

    using PageMap = std::unordered_map<uint64_t, Page>;
    PageMap pages;

    Page *try_fix_existing(typename PageMap::iterator it, bool exclusive);
    Page *try_fix_new(typename PageMap::iterator it, bool exclusive);
    bool try_reserve_space();

    const size_t page_count;
    size_t loaded_page_count = 0;

    // queue management
    void add_to_fifo(Page *p);
    void add_to_lru(Page *p);
    void remove_from_queues(Page *p);
    Page *find_unfixed();
    Page *fifo_head = nullptr, *fifo_tail = nullptr, *lru_head = nullptr, *lru_tail = nullptr;

    // backstore
    void load_page(Page &p);
    void save_page(const Page &p);

    // global mutex
    // TODO finer lock granularity
    std::mutex mutex;
};

BUFFER_MANAGER_TEMPL struct BUFFER_MANAGER_CLASS::Page {
    enum DataState { Reading, Clean, Dirty, Writing };

    constexpr Page(uint64_t page_id);
    ~Page() = default;

    bool can_fix(bool exclusive);
    void fix(bool exclusive);
    void unfix();

    uint64_t page_id;
    int32_t fix_count = 0;

    DataState data_state = Reading;
    std::unique_ptr<std::byte[]> data;

    Page *prev = nullptr, *next = nullptr;
};

BUFFER_MANAGER_TEMPL class BufferManager<page_size>::Fix {
    friend class BufferManager;
 public:
    Fix() = default;

    Fix(const Fix &) = delete;
    Fix &operator=(const Fix &) = delete;

    constexpr Fix(Fix &&o) noexcept;
    constexpr Fix &operator=(Fix &&o) noexcept;

    // unfix page automatically
    ~Fix();
    void unfix();

    // observers
    uint64_t page_id() const;

    // get pointer for non exclusive fix
    const std::byte *data() const;
    template<typename T> const T *as() const;

 protected:
    constexpr Fix(Page *page, BufferManager *manager) noexcept;
    Page *page = nullptr;

 private:
    BufferManager *manager;
};

BUFFER_MANAGER_TEMPL class BUFFER_MANAGER_CLASS::ExclusiveFix : public Fix {
    friend class BufferManager;
 public:
    ExclusiveFix() = default;

    // get pointer for exclusive fix
    using Fix::data;
    std::byte *data();
    template<typename T> T *as();

    // mark page for writeback
    void set_dirty();
 private:
    constexpr ExclusiveFix(Page *page, BufferManager *manager) noexcept;
};

class buffer_full_error : public std::exception {
 public:
    const char* what() const noexcept override {
        return "buffer is full";
    }
};

}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#include "buffer_manager.hpp"
// ---------------------------------------------------------------------------------------------------
#endif  // INCLUDE_IMLAB_BUFFER_MANAGER_H_
