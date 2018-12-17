// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#ifndef INCLUDE_IMLAB_SEGMENT_H_
#define INCLUDE_IMLAB_SEGMENT_H_

#include <cassert>
#include "imlab/buffer_manager.h"
// ---------------------------------------------------------------------------------------------------
namespace imlab {

class Segment {
 public:
    constexpr Segment(uint16_t segment_id, BufferManager &manager)
        : segment_id_mask(((uint64_t) segment_id) << 48), manager(manager) {}

    uint64_t segment_page_id(uint64_t id) const {
        assert((id & ((1ull << 16) - 1) << 48) == 0);
        return segment_id_mask | id;
    }

 protected:
    BufferManager& manager;

 private:
    uint64_t segment_id_mask;
};

}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#endif  // INCLUDE_IMLAB_SEGMENT_H_
