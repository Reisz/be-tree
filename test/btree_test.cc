// ---------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "imlab/buffer_manager.h"
#include "imlab/btree.h"
// ---------------------------------------------------------------------------------------------------
namespace {
// ---------------------------------------------------------------------------------------------------
template<size_t page_size> using BTreeTest = imlab::BTree<uint64_t, uint64_t, page_size>;

TEST(RBTree, Sizes) {
    ASSERT_EQ(1024, sizeof(BTreeTest<1024>::InnerNode));
    ASSERT_EQ(1024, sizeof(BTreeTest<1024>::LeafNode));
}

TEST(BTree, InsertEmptyTree) {
    imlab::BufferManager buffer_manager(1024, 100);
    BTreeTest<1024> tree(0, buffer_manager);

    tree.insert(12, 34);

    EXPECT_EQ(1, tree.size());

    auto *v = tree.find(12);
    ASSERT_TRUE(v);
    ASSERT_EQ(34, *tree.find(12));
}
// ---------------------------------------------------------------------------------------------------
}  // namespace
// ---------------------------------------------------------------------------------------------------
