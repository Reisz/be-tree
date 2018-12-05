// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#ifndef INCLUDE_IMLAB_BUFFER_MANAGER_PAGE_H_
#define INCLUDE_IMLAB_BUFFER_MANAGER_PAGE_H_

#include <stdint.h>
// ---------------------------------------------------------------------------------------------------
namespace imlab {

struct Page {
    enum DataState { Invalid, Reading, Clean, Dirty, Writing };

    explicit Page() = default;
    ~Page() = default;

    uint64_t page_id;
    int32_t fix_count;

    DataState data_state = Invalid;
    void *data = nullptr;

    Page *prev = nullptr, *next = nullptr;
};

}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#endif  // INCLUDE_IMLAB_BUFFER_MANAGER_PAGE_H_
