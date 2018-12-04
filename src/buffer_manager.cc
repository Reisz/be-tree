// ---------------------------------------------------------------------------------------------------
#include "imlab/buffer_manager.h"
// ---------------------------------------------------------------------------------------------------
namespace imlab {

struct Page {};

BufferFix::~BufferFix() {
    unfix();
}

void *BufferFix::getData() {
    return nullptr;  // TODO redirect page access
}

const void *BufferFix::getData() const {
    return nullptr;  // TODO redirect page access
}

void BufferFix::unfix() {
    if (page)
        manager->unfix(page, dirty);
    page = nullptr;
}

// ---------------------------------------------------------------------------------------------------

BufferManager::BufferManager(size_t page_size, size_t page_count) {
    pages.resize(page_count);
}

BufferManager::~BufferManager() {
    // TODO unfix pages
}

const BufferFix BufferManager::fix(uint64_t page_id) {
    return BufferFix();
}

BufferFix BufferManager::fix_exclusive(uint64_t page_id) {
    return BufferFix();
}

void BufferManager::unfix(Page *page, bool dirty) {
    // TODO
}

}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
