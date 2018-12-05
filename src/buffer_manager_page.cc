// ---------------------------------------------------------------------------------------------------
#include "imlab/buffer_manager_page.h"
// ---------------------------------------------------------------------------------------------------
namespace imlab {

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
