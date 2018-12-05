// ---------------------------------------------------------------------------------------------------
#include "imlab/buffer_manager.h"
// ---------------------------------------------------------------------------------------------------
namespace imlab {

BufferFix::~BufferFix() {
    unfix();
}

void *BufferFix::getData() {
    if (page)
        return page->data;
    return nullptr;
}

const void *BufferFix::getData() const {
    if (page)
        return page->data;
    return nullptr;
}

void BufferFix::unfix() {
    if (page)
        manager->unfix(page, dirty);
    page = nullptr;
}

// ---------------------------------------------------------------------------------------------------

BufferManager::BufferManager(size_t page_size, size_t page_count)
: page_size(page_size), page_count(page_count) {}

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
