// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#ifndef INCLUDE_IMLAB_SEGMENT_H_
#define INCLUDE_IMLAB_SEGMENT_H_

#include <cassert>
#include "imlab/buffer_manager.h"
// ---------------------------------------------------------------------------------------------------
namespace imlab {

template <size_t page_size> class Segment {
 public:
    constexpr Segment(uint16_t segment_id, BufferManager<page_size> &manager)
        : segment_id_mask(((uint64_t) segment_id) << 48), manager(manager) {}

    typename BufferManager<page_size>::Fix fix(uint64_t page_id) {
        return manager.fix(segment_page_id(page_id));
    }

    typename BufferManager<page_size>::ExclusiveFix fix_exclusive(uint64_t page_id) {
        return manager.fix_exclusive(segment_page_id(page_id));
    }

    uint64_t page_id(typename BufferManager<page_size>::Fix &fix) {
        return fix.page_id() & ((1ull << 48) - 1);
    }

 private:
    BufferManager<page_size>& manager;
    uint64_t segment_id_mask;

    uint64_t segment_page_id(uint64_t id) const {
        assert((id & ((1ull << 16) - 1) << 48) == 0);
        return segment_id_mask | id;
    }
};

}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#endif  // INCLUDE_IMLAB_SEGMENT_H_
