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

TEST(BTree, Sizes) {
    ASSERT_GE(1024, sizeof(BTreeTest<1024>::InnerNode));
    ASSERT_GE(1024, sizeof(BTreeTest<1024>::LeafNode));
}

TEST(BTree, InsertEmptyTree) {
    imlab::BufferManager<1024> buffer_manager{10};
    BTreeTest<1024> tree(0, buffer_manager);

    tree.insert(12, 34);
    EXPECT_EQ(1, tree.size());

    auto it = tree.find(12);
    ASSERT_NE(tree.end(), it);
    ASSERT_EQ(34, *it);
}

TEST(BTree, InsertDoesNotOverwrite) {
    imlab::BufferManager<1024> buffer_manager{10};
    BTreeTest<1024> tree(0, buffer_manager);

    tree.insert(12, 34);
    EXPECT_EQ(1, tree.size());

    tree.insert(12, 45);
    EXPECT_EQ(1, tree.size());

    auto it = tree.find(12);
    ASSERT_NE(tree.end(), it);
    ASSERT_EQ(34, *it);
}

TEST(BTree, InsertOrAssign) {
    imlab::BufferManager<1024> buffer_manager{10};
    BTreeTest<1024> tree(0, buffer_manager);

    tree.insert(12, 34);
    EXPECT_EQ(1, tree.size());

    tree.insert_or_assign(12, 45);
    EXPECT_EQ(1, tree.size());

    auto it = tree.find(12);
    ASSERT_NE(tree.end(), it);
    ASSERT_EQ(45, *it);
}

TEST(BTree, LookupEmptyTree) {
    imlab::BufferManager<1024> buffer_manager{10};
    BTreeTest<1024> tree(0, buffer_manager);

    ASSERT_EQ(tree.end(), tree.begin());
    ASSERT_EQ(tree.end(), tree.find(0));
}

template<size_t size> constexpr auto insert_amount =
    BTreeTest<size>::LeafNode::kCapacity * BTreeTest<size>::InnerNode::kCapacity * 2;

TEST(BTree, MultipleInserts) {
    imlab::BufferManager<1024> buffer_manager{10};
    BTreeTest<1024> tree(0, buffer_manager);

    for (uint32_t i = 0; i < insert_amount<1024>; ++i)
        tree.insert(i, i);
    EXPECT_EQ(insert_amount<1024>, tree.size());

    ASSERT_EQ(tree.end(), tree.find(insert_amount<1024>));
    ASSERT_EQ(tree.end(), tree.lower_bound(insert_amount<1024>));
    ASSERT_EQ(tree.end(), tree.upper_bound(insert_amount<1024> - 1));

    uint32_t i = 0;
    for (auto j : tree)
        EXPECT_EQ(i++, j);
    for (uint32_t i = 0; i < insert_amount<1024>; ++i) {
        EXPECT_EQ(i, *tree.find(i));
        EXPECT_EQ(i, *tree.lower_bound(i));
        if (i > 0)
            EXPECT_EQ(i, *tree.upper_bound(i - 1));
    }
}

TEST(BTree, MultipleInsertOrAssigns) {
    imlab::BufferManager<1024> buffer_manager{10};
    BTreeTest<1024> tree(0, buffer_manager);

    for (uint32_t i = 0; i < insert_amount<1024>; ++i)
        tree.insert_or_assign(i, i);
    EXPECT_EQ(insert_amount<1024>, tree.size());

    ASSERT_EQ(tree.end(), tree.find(insert_amount<1024>));
    ASSERT_EQ(tree.end(), tree.lower_bound(insert_amount<1024>));
    ASSERT_EQ(tree.end(), tree.upper_bound(insert_amount<1024> - 1));

    uint32_t i = 0;
    for (auto j : tree)
        EXPECT_EQ(i++, j);
    for (uint32_t i = 0; i < insert_amount<1024>; ++i) {
        EXPECT_EQ(i, *tree.find(i));
        EXPECT_EQ(i, *tree.lower_bound(i));
        if (i > 0)
            EXPECT_EQ(i, *tree.upper_bound(i - 1));
    }
}

TEST(BTree, MultipleInsertReverse) {
    imlab::BufferManager<1024> buffer_manager{10};
    BTreeTest<1024> tree(0, buffer_manager);

    for (uint32_t i = insert_amount<1024>; i > 0; --i)
        tree.insert(i, i);
    ASSERT_EQ(insert_amount<1024>, tree.size());

    ASSERT_EQ(tree.end(), tree.find(0));
    ASSERT_EQ(1, *tree.lower_bound(0));
    ASSERT_EQ(1, *tree.upper_bound(0));
    ASSERT_EQ(tree.end(), tree.find(insert_amount<1024> + 1));
    ASSERT_EQ(tree.end(), tree.lower_bound(insert_amount<1024> + 1));
    ASSERT_EQ(tree.end(), tree.upper_bound(insert_amount<1024>));

    uint32_t i = 1;
    for (auto j : tree)
        EXPECT_EQ(i++, j);
    for (uint32_t i = 1; i <= insert_amount<1024>; ++i) {
        EXPECT_EQ(i, *tree.find(i));
        EXPECT_EQ(i, *tree.lower_bound(i));
        EXPECT_EQ(i, *tree.upper_bound(i - 1));
    }
}

TEST(BTree, MultipleInsertOrAssignsReverse) {
    imlab::BufferManager<1024> buffer_manager{10};
    BTreeTest<1024> tree(0, buffer_manager);

    for (uint32_t i = insert_amount<1024>; i > 0; --i)
        tree.insert_or_assign(i, i);
    ASSERT_EQ(insert_amount<1024>, tree.size());

    ASSERT_EQ(tree.end(), tree.find(0));
    ASSERT_EQ(1, *tree.lower_bound(0));
    ASSERT_EQ(1, *tree.upper_bound(0));
    ASSERT_EQ(tree.end(), tree.find(insert_amount<1024> + 1));
    ASSERT_EQ(tree.end(), tree.lower_bound(insert_amount<1024> + 1));
    ASSERT_EQ(tree.end(), tree.upper_bound(insert_amount<1024>));

    uint32_t i = 1;
    for (auto j : tree)
        EXPECT_EQ(i++, j);
    for (uint32_t i = 1; i <= insert_amount<1024>; ++i) {
        EXPECT_EQ(i, *tree.find(i));
        EXPECT_EQ(i, *tree.lower_bound(i));
        EXPECT_EQ(i, *tree.upper_bound(i - 1));
    }
}
// ---------------------------------------------------------------------------------------------------
}  // namespace
// ---------------------------------------------------------------------------------------------------
