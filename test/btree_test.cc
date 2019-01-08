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
    ASSERT_GE(1024, sizeof(BTreeTest<1024>::InnerNode));
    ASSERT_GE(1024, sizeof(BTreeTest<1024>::LeafNode));
}

TEST(BTree, InsertEmptyTree) {
    imlab::BufferManager<1024> buffer_manager{100};
    BTreeTest<1024> tree(0, buffer_manager);

    tree.insert(12, 34);
    EXPECT_EQ(1, tree.size());

    // auto *v = tree.find(12);
    // ASSERT_TRUE(v);
    // ASSERT_EQ(34, *tree.find(12));
}

TEST(BTree, InsertDoesNotOverwrite) {
    imlab::BufferManager<1024> buffer_manager{100};
    BTreeTest<1024> tree(0, buffer_manager);

    tree.insert(12, 34);
    EXPECT_EQ(1, tree.size());

    tree.insert(12, 45);
    EXPECT_EQ(1, tree.size());

    // auto *v = tree.find(12);
    // ASSERT_TRUE(v);
    // ASSERT_EQ(34, *tree.find(12));
}

TEST(BTree, InsertOrAssign) {
    imlab::BufferManager<1024> buffer_manager{100};
    BTreeTest<1024> tree(0, buffer_manager);

    tree.insert(12, 34);
    EXPECT_EQ(1, tree.size());

    tree.insert_or_assign(12, 45);
    EXPECT_EQ(1, tree.size());

    // auto *v = tree.find(12);
    // ASSERT_TRUE(v);
    // ASSERT_EQ(34, *tree.find(12));
}
// ---------------------------------------------------------------------------------------------------
}  // namespace
// ---------------------------------------------------------------------------------------------------
