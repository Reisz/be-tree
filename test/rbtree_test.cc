// ---------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------
#include <gtest/gtest.h>
#include <deque>
#include "imlab/rbtree.h"
// ---------------------------------------------------------------------------------------------------
template<typename Key, size_t page_size, typename Compare, typename... Ts>
void imlab::RBTree<Key, page_size, Compare, Ts...>::check_rb_invariants() {
    NodeRef node = ref(header.root_node);
    if (!node)
        return;

    std::deque<std::tuple<NodeRef, uint32_t>> todo = { {node, 0} };
    std::optional<uint32_t> black_depth;
    uint32_t node_count = 0;

    while (!todo.empty()) {
        auto tuple = todo.front();
        todo.pop_front();

        node = std::get<0>(tuple);
        if (!node) {
            if (!black_depth)
                black_depth = std::get<1>(tuple);
            EXPECT_EQ(black_depth.value(), std::get<1>(tuple)) << "Count: " << (uint64_t) header.node_count;
            continue;
        }

        ++node_count;

        NodeRef left = ref(node->left);
        NodeRef right = ref(node->right);

        bool is_black = node->color == RBNode::Black;

        todo.emplace_front(left, std::get<1>(tuple) + is_black);
        todo.emplace_front(right, std::get<1>(tuple) + is_black);

        // basic binary tree invariants
        if (left)
            EXPECT_TRUE(comp(left->key, node->key)) << "Count: " << (uint64_t) header.node_count;
        if (right)
            EXPECT_TRUE(comp(node->key, right->key)) << "Count: " << (uint64_t) header.node_count;

        // red nodes have two black children (empty counts as black)
        if (node->color == RBNode::Red) {
            if (left)
                EXPECT_EQ(left->color, RBNode::Black) << "Count: " << (uint64_t) header.node_count;
            if (right)
                EXPECT_EQ(right->color, RBNode::Black) << "Count: " << (uint64_t) header.node_count;
        }
    }

    EXPECT_EQ(node_count, header.node_count) << "Count: " << (uint64_t) header.node_count;
}

namespace {
// ---------------------------------------------------------------------------------------------------
template<typename Key> struct MessageKey {
    uint64_t timestamp;
    Key key;

    friend bool operator<(const MessageKey &k1, const MessageKey &k2) {
        return (k1.key == k2.key && k1.timestamp < k2.timestamp)  || k1.key < k2.key;
    }
};
template<typename T> struct Insert { T value; };
template<typename T> struct Update { T value; };
using Delete = void;
template<size_t page_size> using RBTreeTest = imlab::RBTree<MessageKey<uint64_t>, page_size, std::less<MessageKey<uint64_t>>,
    Insert<uint64_t>, Update<uint64_t>, Delete>;

// insertion order results in irregular tree for traversal testing
constexpr uint8_t traversal_test[] = {
    1, 10, 8, 3, 6, 4, 7, 11, 9, 17, 2, 5, 15, 14, 13, 16, 12
};

TEST(RBTree, Sizes) {
    ASSERT_EQ(sizeof(RBTreeTest<255>), 255);
    ASSERT_EQ(sizeof(RBTreeTest<(1ull << 16) - 2>), (1ull << 16) - 2);
    ASSERT_EQ(sizeof(RBTreeTest<(1ull << 32) - 4>), (1ull << 32) - 4);
    ASSERT_EQ(sizeof(RBTreeTest<1ull << 32>), 1ull << 32);
}

TEST(RBTree, LinearInsertion) {
    RBTreeTest<255> tree;

    uint32_t i = 0;
    while (tree.insert<2>({i++, 0}))
        tree.check_rb_invariants();
}

TEST(RBTree, LinearReverseInsertion) {
    RBTreeTest<255> tree;

    uint32_t i = std::numeric_limits<uint32_t>::max();
    while (tree.insert<2>({i--, 0}))
        tree.check_rb_invariants();
}

TEST(RBTree, SwitchingInsertion) {
    RBTreeTest<255> tree;

    uint32_t i = 0;
    uint32_t k = 0;
    while (tree.insert<2>({i++, k ^= 1}))
        tree.check_rb_invariants();
}

TEST(RBTree, IrregularInsertion) {
    RBTreeTest<1024> tree;

    for (auto i : traversal_test) {
        tree.insert<2>({0, i});
        tree.check_rb_invariants();
    }
}

TEST(RBTree, EmptyTreeIterators) {
    RBTreeTest<255> tree;
    ASSERT_EQ(tree.end(), tree.begin());
}

TEST(RBTree, NonEmptyTreeIterators) {
    RBTreeTest<255> tree;
    tree.insert<2>({0, 0});
    ASSERT_NE(tree.end(), tree.begin());
}

TEST(RBTree, IteratorBegin) {
    RBTreeTest<255> tree;
    tree.insert<2>({0, 0});
    tree.insert<2>({0, 1});

    ASSERT_EQ(2, (*tree.begin()).type());
}

TEST(RBTree, ForwardIterationLinear) {
    RBTreeTest<255> tree;

    uint32_t i = 0;
    while (tree.insert<1>({i, 0}, {i}))
        ++i;

    i = 0;
    for (auto ref : tree)
        ASSERT_EQ(i++, ref.as<1>().value);
}

TEST(RBTree, ForwardIterationLinearReverse) {
    RBTreeTest<255> tree;

    uint32_t i = std::numeric_limits<uint32_t>::max();
    while (tree.insert<1>({i, 0}, {i}))
        --i;

    for (auto ref : tree)
        ASSERT_EQ(++i, ref.as<1>().value);
}

TEST(RBTree, ForwardIterationSwitching) {
    RBTreeTest<255> tree;

    uint32_t i = 0;
    uint32_t k = 1;
    while (tree.insert<1>({i, k ^= 1}, {i}))
        ++i;

    i = 0;
    for (auto ref : tree) {
        if (ref.as<1>().value == 1)
            i = 1;
        ASSERT_EQ(i, ref.as<1>().value);
        i += 2;
    }
}

TEST(RBTree, ForwardIterationIrregular) {
    RBTreeTest<1024> tree;

    for (auto i : traversal_test)
        ASSERT_TRUE(tree.insert<1>({0, i}, {i}));

    uint8_t i = 1;
    for (auto ref : tree)
        ASSERT_EQ(i++, ref.as<1>().value);
}

TEST(RBTree, Find) {
    RBTreeTest<1024> tree;
    ASSERT_EQ(tree.end(), tree.find({0, 0}));

    for (auto i : traversal_test)
        ASSERT_TRUE(tree.insert<1>({0, i}, {i}));

    ASSERT_EQ(tree.end(), tree.find({0, 0}));
    ASSERT_EQ(tree.end(), tree.find({1, 1}));
    ASSERT_EQ(tree.end(), tree.find({0, 100}));

    for (auto i : traversal_test)
        ASSERT_EQ(i, (*tree.find({0, i})).as<1>().value);
}

TEST(RBTree, LowerBound) {
    RBTreeTest<1024> tree;
    ASSERT_EQ(tree.end(), tree.lower_bound({0, 0}));

    uint32_t i = 1;
    while (tree.insert<1>({i, 0}, {i}))
        i += 5;

    ASSERT_EQ(tree.end(), tree.lower_bound({i, 0}));
    for (uint32_t j = 1; j < i; j += 5) {
        EXPECT_EQ(j, (*tree.lower_bound({j - 1, 0})).as<1>().value);
        EXPECT_EQ(j, (*tree.lower_bound({j, 0})).as<1>().value);
        if (j + 5 < i)
            EXPECT_EQ(j + 5, (*tree.lower_bound({j + 1, 0})).as<1>().value);
    }
}

TEST(RBTree, UpperBound) {
    RBTreeTest<1024> tree;
    ASSERT_EQ(tree.end(), tree.upper_bound({0, 0}));

    uint32_t i = 1;
    while (tree.insert<1>({i, 0}, {i}))
        i += 5;

    ASSERT_EQ(tree.end(), tree.upper_bound({i - 5, 0}));
    for (uint32_t j = 1; j < i; j += 5) {
        EXPECT_EQ(j, (*tree.upper_bound({j - 1, 0})).as<1>().value);
        if (j + 5 < i)
            EXPECT_EQ(j + 5, (*tree.upper_bound({j, 0})).as<1>().value);
    }
}

// ---------------------------------------------------------------------------------------------------
}  // namespace
// ---------------------------------------------------------------------------------------------------
