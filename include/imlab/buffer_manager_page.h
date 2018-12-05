// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#ifndef INCLUDE_IMLAB_BUFFER_MANAGER_PAGE_H_
#define INCLUDE_IMLAB_BUFFER_MANAGER_PAGE_H_

#include <stdint.h>
// ---------------------------------------------------------------------------------------------------
namespace imlab {

struct Page {
    enum DataState { Reading, Clean, Dirty, Writing };

    constexpr explicit Page(uint64_t page_id)
        : page_id(page_id) {}
    ~Page() = default;

    bool can_fix(bool exclusive);
    void fix(bool exclusive);

    uint64_t page_id;
    int32_t fix_count = 0;

    DataState data_state = Reading;
    void *data = nullptr;

    Page *prev = nullptr, *next = nullptr;
};

}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#endif  // INCLUDE_IMLAB_BUFFER_MANAGER_PAGE_H_
