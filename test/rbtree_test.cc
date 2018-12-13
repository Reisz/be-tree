// ---------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "imlab/rbtree.h"
// ---------------------------------------------------------------------------------------------------
namespace {
// ---------------------------------------------------------------------------------------------------
template<typename T> struct Insert { T key; };
template<typename T> struct Update { T value; };
using Delete = void;
template<size_t page_size> using RBTreeTest = imlab::RBTree<uint64_t, page_size, std::less<uint64_t>,
    Insert<uint64_t>, Update<uint64_t>, Delete>;

TEST(RBTree, PointerSizes) {
    RBTreeTest<255> test8;  // static assert
    RBTreeTest<(1ull << 16) - 1> test16;  // static assert
    RBTreeTest<(1ull << 32) - 1> test32;  // static assert
    RBTreeTest<1ull << 32> test64;  // static assert
}
// ---------------------------------------------------------------------------------------------------
}  // namespace
// ---------------------------------------------------------------------------------------------------
