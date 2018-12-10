// ---------------------------------------------------------------------------------------------------
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdlib>
#include <system_error>
#include <string>
#include "imlab/segment_file.h"
// ---------------------------------------------------------------------------------------------------
namespace imlab {

namespace {

    constexpr uint16_t segment_id(uint64_t page_id) {
        return page_id >> 48;
    }
    constexpr uint64_t segment_page_id(uint64_t page_id) {
        return page_id & ((1ull << 48) - 1);
    }

    const char *save_directory() {
        static const char *save_directory = nullptr;
        if (!save_directory) {
            save_directory = std::getenv("SEGMENT_DIRECTORY");
            if (!save_directory)
                save_directory = "/tmp/";
        }

        return save_directory;
    }

    [[noreturn]] static void throw_errno() {
        throw std::system_error{errno, std::system_category()};
    }

}  // namespace

SegmentFile::SegmentFile(uint64_t page_id, size_t page_size) : page_size(page_size) {
    std::string filename = save_directory();
    filename += std::to_string(segment_id(page_id));

    fd = open(filename.c_str(), O_RDWR | O_CREAT | O_SYNC, 0666);
    if (fd < 0)
        throw_errno();

    pos = page_size * segment_page_id(page_id);

    struct stat file_stat;
    if (fstat(fd, &file_stat) < 0) {
        throw_errno();
    }

    if (file_stat.st_size < pos + page_size) {
        if (ftruncate(fd, pos + page_size) < 0)
            throw_errno();
    }
}

SegmentFile::~SegmentFile() {
    close(fd);
}


void SegmentFile::read(std::byte *data) {
    prw_loop(pread, data);
}

void SegmentFile::write(std::byte *data) {
    prw_loop(pwrite, data);
}


template<typename Op> void SegmentFile::prw_loop(Op op, std::byte *data) {
    size_t total_bytes = 0;
    while (total_bytes < page_size) {
        ssize_t bytes = op(fd, data + total_bytes, page_size - total_bytes, pos + total_bytes);

        if (bytes == 0) {
            // This should probably never happen. Return here to prevent
            // an infinite loop.
            return;
        } else if (bytes < 0) {
            throw_errno();
        }

        total_bytes += static_cast<size_t>(bytes);
    }
}

}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
