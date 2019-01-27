// ---------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "imlab/betree.h"
#include "imlab/infra/random.h"
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

    // TODO
    // auto it = tree.find(12);
    // ASSERT_NE(tree.end(), it);
    // ASSERT_EQ(34, *it);
}

TEST(BeTree, InsertSplitRootLeaf) {
    imlab::BufferManager<1024> buffer_manager{10};
    BeTreeTest<1024, 256> tree(0, buffer_manager);

    size_t i = 0;
    for (; i <= BeTreeTest<1024, 256>::LeafNode::kCapacity; ++i)
        tree.insert(i, i);
    EXPECT_LE(tree.size(), tree.size_pending());
    EXPECT_EQ(i, tree.size_pending());

    // TODO check iterators
}

template<size_t size, size_t e> constexpr auto insert_amount =
    BeTreeTest<size, e>::LeafNode::kCapacity * BeTreeTest<size, e>::InnerNode::kCapacity * 2;

TEST(BeTree, MultipleInserts) {
    imlab::BufferManager<1024> buffer_manager{10};
    BeTreeTest<1024, 256> tree(0, buffer_manager);
    constexpr auto ia = insert_amount<1024, 256>;

    for (uint32_t i = 0; i < ia; ++i)
        tree.insert(i, i);
    EXPECT_LE(tree.size(), tree.size_pending());
    EXPECT_EQ(ia, tree.size_pending());

    // ASSERT_EQ(tree.end(), tree.find(insert_amount<1024, 256>));
    // ASSERT_EQ(tree.end(), tree.lower_bound(insert_amount<1024, 256>));
    // ASSERT_EQ(tree.end(), tree.upper_bound(insert_amount<1024, 256> - 1));
    //
    // uint32_t i = 0;
    // for (auto j : tree)
    //     EXPECT_EQ(i++, j);
    // for (uint32_t i = 0; i < insert_amount<1024, 256>; ++i) {
    //     EXPECT_EQ(i, *tree.find(i));
    //     EXPECT_EQ(i, *tree.lower_bound(i));
    //     if (i > 0)
    //         EXPECT_EQ(i, *tree.upper_bound(i - 1));
    // }
}

TEST(BeTree, MultipleInsertsReverse) {
    imlab::BufferManager<1024> buffer_manager{10};
    BeTreeTest<1024, 256> tree(0, buffer_manager);
    constexpr auto ia = insert_amount<1024, 256>;

    for (uint32_t i = ia; i > 0; --i)
        tree.insert(i, i);
    EXPECT_LE(tree.size(), tree.size_pending());
    EXPECT_EQ(ia, tree.size_pending());

    // ASSERT_EQ(tree.end(), tree.find(insert_amount<1024, 256>));
    // ASSERT_EQ(tree.end(), tree.lower_bound(insert_amount<1024, 256>));
    // ASSERT_EQ(tree.end(), tree.upper_bound(insert_amount<1024, 256> - 1));
    //
    // uint32_t i = 0;
    // for (auto j : tree)
    //     EXPECT_EQ(i++, j);
    // for (uint32_t i = 0; i < insert_amount<1024, 256>; ++i) {
    //     EXPECT_EQ(i, *tree.find(i));
    //     EXPECT_EQ(i, *tree.lower_bound(i));
    //     if (i > 0)
    //         EXPECT_EQ(i, *tree.upper_bound(i - 1));
    // }
}

TEST(BeTree, MultipleRandomInsertsRandom) {
    imlab::BufferManager<1024> buffer_manager{10};
    BeTreeTest<1024, 256> tree(0, buffer_manager);
    constexpr auto ia = insert_amount<1024, 256>;

    for (uint32_t i = 0; i < ia; ++i)
        tree.insert(xorshf96(), 0);
    EXPECT_LE(tree.size(), tree.size_pending());
    EXPECT_EQ(ia, tree.size_pending());

    // ASSERT_EQ(tree.end(), tree.find(insert_amount<1024, 256>));
    // ASSERT_EQ(tree.end(), tree.lower_bound(insert_amount<1024, 256>));
    // ASSERT_EQ(tree.end(), tree.upper_bound(insert_amount<1024, 256> - 1));
    //
    // uint32_t i = 0;
    // for (auto j : tree)
    //     EXPECT_EQ(i++, j);
    // for (uint32_t i = 0; i < insert_amount<1024, 256>; ++i) {
    //     EXPECT_EQ(i, *tree.find(i));
    //     EXPECT_EQ(i, *tree.lower_bound(i));
    //     if (i > 0)
    //         EXPECT_EQ(i, *tree.upper_bound(i - 1));
    // }
}

}  // namespace
// ---------------------------------------------------------------------------------------------------
