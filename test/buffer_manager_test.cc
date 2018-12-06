// ---------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------
#include <gtest/gtest.h>
#include <cstring>
#include "imlab/buffer_manager.h"
// ---------------------------------------------------------------------------------------------------
const std::vector<uint64_t> imlab::BufferManager::get_fifo() const {
    std::vector<uint64_t> result;

    Page *p = fifo_head;
    while (p) {
        result.push_back(p->page_id);
        p = p->next;
    }

    return result;
}

const std::vector<uint64_t> imlab::BufferManager::get_lru() const {
    std::vector<uint64_t> result;

    Page *p = lru_head;
    while (p) {
        result.push_back(p->page_id);
        p = p->next;
    }

    return result;
}

namespace {
// ---------------------------------------------------------------------------------------------------
TEST(BufferManager, FixSingle) {
    imlab::BufferManager buffer_manager{1024, 10};
    std::vector<uint64_t> expected_values(1024 / sizeof(uint64_t), 123);
    {
        auto fix = buffer_manager.fix_exclusive(1);
        std::memcpy(fix.data(), expected_values.data(), 1024);
        fix.set_dirty();
        fix.unfix();
        EXPECT_EQ(std::vector<uint64_t>{1}, buffer_manager.get_fifo());
        EXPECT_TRUE(buffer_manager.get_lru().empty());
    }
    {
        std::vector<uint64_t> values(1024 / sizeof(uint64_t));
        auto fix = buffer_manager.fix(1);
        std::memcpy(values.data(), fix.data(), 1024);
        fix.set_dirty();
        fix.unfix();
        EXPECT_TRUE(buffer_manager.get_fifo().empty());
        EXPECT_EQ(std::vector<uint64_t>{1}, buffer_manager.get_lru());
        ASSERT_EQ(expected_values, values);
    }
}
// ---------------------------------------------------------------------------------------------------
}  // namespace
// ---------------------------------------------------------------------------------------------------
