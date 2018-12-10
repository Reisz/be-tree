// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#ifndef INCLUDE_IMLAB_BUFFER_MANAGER_PAGE_H_
#define INCLUDE_IMLAB_BUFFER_MANAGER_PAGE_H_

#include <cstddef>
#include <memory>
// ---------------------------------------------------------------------------------------------------
namespace imlab {

struct Page {
    enum DataState { Reading, Clean, Dirty, Writing };

    Page(uint64_t page_id, size_t page_size);
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

}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#endif  // INCLUDE_IMLAB_BUFFER_MANAGER_PAGE_H_
