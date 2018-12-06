// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#ifndef INCLUDE_IMLAB_SEGMENT_FILE_H_
#define INCLUDE_IMLAB_SEGMENT_FILE_H_

#include <stdint.h>
#include <stddef.h>
// ---------------------------------------------------------------------------------------------------
namespace imlab {

class SegmentFile {
 public:
    SegmentFile(uint64_t page_id, size_t page_size);
    ~SegmentFile();

    void read(uint8_t *data);
    void write(uint8_t *data);

 private:
    template<typename Op> void prw_loop(Op op, uint8_t *data);

    int fd;
    uint64_t pos;
    size_t page_size;
};

}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#endif  // INCLUDE_IMLAB_SEGMENT_FILE_H_
