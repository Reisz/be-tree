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

}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
