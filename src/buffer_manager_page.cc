// ---------------------------------------------------------------------------------------------------
#include "imlab/buffer_manager_page.h"
// ---------------------------------------------------------------------------------------------------
namespace imlab {

Page::Page(uint64_t page_id, size_t page_size)
    : page_id(page_id), data(new uint8_t[page_size]) {}

bool Page::can_fix(bool exclusive) {
    if (exclusive)
        return fix_count == 0;
    return fix_count >= 0;
}

void Page::fix(bool exclusive) {
    if (exclusive)
        fix_count = -1;
    else
        fix_count++;
}

void Page::unfix() {
    if (fix_count > 0)
        --fix_count;
    else
        fix_count = 0;
}

}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
