// ---------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "imlab/rbtree.h"
// ---------------------------------------------------------------------------------------------------
namespace {
// ---------------------------------------------------------------------------------------------------
template<typename Key> struct MessageKey {
    uint64_t timestamp;
    Key key;

    friend bool operator<(const MessageKey &k1, const MessageKey &k2) {
        return (k1.key == k2.key && k1.key < k2.key)  || k1.key < k2.key;
    }
};
template<typename T> struct Insert { T key; };
template<typename T> struct Update { T value; };
using Delete = void;
template<size_t page_size> using RBTreeTest = imlab::RBTree<MessageKey<uint64_t>, page_size, std::less<MessageKey<uint64_t>>,
    Insert<uint64_t>, Update<uint64_t>, Delete>;

TEST(RBTree, Sizes) {
    ASSERT_EQ(sizeof(RBTreeTest<255>), 255);
    ASSERT_EQ(sizeof(RBTreeTest<(1ull << 16) - 2>), (1ull << 16) - 2);
    ASSERT_EQ(sizeof(RBTreeTest<(1ull << 32) - 4>), (1ull << 32) - 4);
    ASSERT_EQ(sizeof(RBTreeTest<1ull << 32>), 1ull << 32);
}
// ---------------------------------------------------------------------------------------------------
}  // namespace
// ---------------------------------------------------------------------------------------------------
