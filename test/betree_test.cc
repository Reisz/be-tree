// ---------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "imlab/betree.h"
// ---------------------------------------------------------------------------------------------------
namespace {
// ---------------------------------------------------------------------------------------------------
template<size_t page_size, size_t epsilon> using BeTreeTest = imlab::BeTree<uint64_t, uint64_t, page_size, epsilon>;
// ---------------------------------------------------------------------------------------------------
TEST(BeTree, Sizes) {
    ASSERT_GE(1024, sizeof(BeTreeTest<1024, 256>::InnerNode));
    ASSERT_GE(1024, sizeof(BeTreeTest<1024, 256>::LeafNode));
}

TEST(BeTree, InsertEmptyTree) {
    imlab::BufferManager<1024> buffer_manager{10};
    BeTreeTest<1024, 256> tree(0, buffer_manager);

    tree.insert(12, 34);
    EXPECT_LE(tree.size(), tree.size_pending());
    EXPECT_EQ(1, tree.size_pending());

    // auto it = tree.find(12);
    // ASSERT_NE(tree.end(), it);
    // ASSERT_EQ(34, *it);
}

}  // namespace
// ---------------------------------------------------------------------------------------------------
