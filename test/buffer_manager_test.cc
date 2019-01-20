// ---------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------
#include <gtest/gtest.h>
#include <cstring>
#include "imlab/buffer_manager.h"
// ---------------------------------------------------------------------------------------------------
BUFFER_MANAGER_TEMPL const std::vector<uint64_t> imlab::BUFFER_MANAGER_CLASS::get_fifo() const {
    std::vector<uint64_t> result;

    Page *p = fifo_head;
    while (p) {
        result.push_back(p->page_id);
        p = p->next;
    }

    return result;
}

BUFFER_MANAGER_TEMPL const std::vector<uint64_t> imlab::BUFFER_MANAGER_CLASS::get_lru() const {
    std::vector<uint64_t> result;

    Page *p = lru_head;
    while (p) {
        result.push_back(p->page_id);
        p = p->next;
    }

    return result;
}
// ---------------------------------------------------------------------------------------------------
namespace {
// ---------------------------------------------------------------------------------------------------
TEST(BufferManager, FixSingle) {
    imlab::BufferManager<1024> manager{10};

    EXPECT_FALSE(manager.in_memory(1));
    EXPECT_FALSE(manager.is_dirty(1));

    std::vector<uint64_t> expected_values(1024 / sizeof(uint64_t), 123);
    {
        auto fix = manager.fix_exclusive(1);
        std::memcpy(fix.data(), expected_values.data(), 1024);
        fix.set_dirty();
        fix.unfix();
        EXPECT_EQ(std::vector<uint64_t>{1}, manager.get_fifo());
        EXPECT_TRUE(manager.get_lru().empty());
    }

    EXPECT_FALSE(manager.in_memory(0));
    EXPECT_FALSE(manager.is_dirty(0));

    EXPECT_TRUE(manager.in_memory(1));
    EXPECT_TRUE(manager.is_dirty(1));

    {
        std::vector<uint64_t> values(1024 / sizeof(uint64_t));
        auto fix = manager.fix(1);
        std::memcpy(values.data(), fix.data(), 1024);
        fix.unfix();
        EXPECT_TRUE(manager.get_fifo().empty());
        EXPECT_EQ(std::vector<uint64_t>{1}, manager.get_lru());
        ASSERT_EQ(expected_values, values);
    }
}

TEST(BufferManager, PersistentRestart) {
    {
        imlab::BufferManager<1024> manager{10};
        for (uint16_t segment = 0; segment < 3; ++segment) {
            for (uint64_t segment_page = 0; segment_page < 10; ++segment_page) {
                uint64_t page_id = (static_cast<uint64_t>(segment) << 48) | segment_page;
                auto fix = manager.fix_exclusive(page_id);
                uint64_t& value = *fix.as<uint64_t>();
                value = segment * 10 + segment_page;
                fix.set_dirty();
            }
        }
    }
    {
        imlab::BufferManager<1024> manager{10};
        for (uint16_t segment = 0; segment < 3; ++segment) {
            for (uint64_t segment_page = 0; segment_page < 10; ++segment_page) {
                uint64_t page_id = (static_cast<uint64_t>(segment) << 48) | segment_page;
                auto fix = manager.fix(page_id);
                uint64_t value = *fix.as<uint64_t>();
                fix.unfix();
                EXPECT_EQ(segment * 10 + segment_page, value);
            }
        }
    }
}

TEST(BufferManager, FIFOEvict) {
    imlab::BufferManager<1024> manager{10};
    for (uint64_t i = 1; i < 11; ++i) {
        auto fix = manager.fix(i);
    }

    EXPECT_FALSE(manager.in_memory(11));

    {
        std::vector<uint64_t> expected_fifo{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        EXPECT_EQ(expected_fifo, manager.get_fifo());
        EXPECT_TRUE(manager.get_lru().empty());
    }

    {
        auto fix = manager.fix(11);
    }

    EXPECT_FALSE(manager.in_memory(1));
    EXPECT_TRUE(manager.in_memory(11));

    {
        std::vector<uint64_t> expected_fifo{2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
        EXPECT_EQ(expected_fifo, manager.get_fifo());
        EXPECT_TRUE(manager.get_lru().empty());
    }
}

TEST(BufferManager, BufferFull) {
    imlab::BufferManager<1024> manager{10};

    std::vector<imlab::BufferManager<1024>::Fix> pages;
    pages.reserve(10);

    for (uint64_t i = 1; i < 11; ++i) {
        pages.push_back(manager.fix(i));
    }

    EXPECT_THROW(manager.fix(11), imlab::buffer_full_error);
}

TEST(BufferManager, MoveToLRU) {
    imlab::BufferManager<1024> manager{2};

    manager.fix(1);
    manager.fix(2);

    EXPECT_EQ((std::vector<uint64_t>{1, 2}), manager.get_fifo());
    EXPECT_TRUE(manager.get_lru().empty());

    manager.fix(2);

    EXPECT_EQ(std::vector<uint64_t>{1}, manager.get_fifo());
    EXPECT_EQ(std::vector<uint64_t>{2}, manager.get_lru());
}

TEST(BufferManager, LRURefresh) {
    imlab::BufferManager<1024> manager{10};

    manager.fix(1);
    manager.fix(1);
    manager.fix(2);
    manager.fix(2);

    EXPECT_TRUE(manager.get_fifo().empty());
    EXPECT_EQ((std::vector<uint64_t>{1, 2}), manager.get_lru());

    manager.fix(1);

    EXPECT_TRUE(manager.get_fifo().empty());
    EXPECT_EQ((std::vector<uint64_t>{2, 1}), manager.get_lru());
}
// ---------------------------------------------------------------------------------------------------
}  // namespace
// ---------------------------------------------------------------------------------------------------
