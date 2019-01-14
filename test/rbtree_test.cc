// ---------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------
#include <gtest/gtest.h>
#include <deque>
#include "imlab/rbtree.h"
// ---------------------------------------------------------------------------------------------------
template<typename Key, size_t page_size, typename Compare, typename... Ts>
void imlab::RbTree<Key, page_size, Compare, Ts...>::check_rb_invariants() const {
    auto node = ref(header.root_node);
    if (!node)
        return;

    std::deque<std::tuple<const_node_ref, uint32_t>> todo = { {node, 0} };
    std::optional<uint32_t> black_depth;
    uint32_t node_count = 0;

    while (!todo.empty()) {
        auto tuple = todo.front();
        todo.pop_front();

        node = std::get<0>(tuple);
        if (!node) {
            if (!black_depth)
                black_depth = std::get<1>(tuple);
            EXPECT_EQ(black_depth.value(), std::get<1>(tuple)) << "Count: " << size();
            continue;
        }

        ++node_count;

        auto left = ref(node->left);
        auto right = ref(node->right);

        bool is_black = node->color == Node::Black;

        todo.emplace_front(left, std::get<1>(tuple) + is_black);
        todo.emplace_front(right, std::get<1>(tuple) + is_black);

        // basic binary tree invariants
        if (left)
            EXPECT_TRUE(comp(left->key, node->key)) << "Count: " << size();
        if (right)
            EXPECT_TRUE(comp(node->key, right->key)) << "Count: " << size();

        // red nodes have two black children (empty counts as black)
        if (node->color == Node::Red) {
            if (left)
                EXPECT_EQ(left->color, Node::Black) << "Count: " << size();
            if (right)
                EXPECT_EQ(right->color, Node::Black) << "Count: " << size();
        }
    }

    EXPECT_EQ(node_count, size()) << "Count: " << size();
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
template<size_t page_size> using RbTreeTest = imlab::RbTree<MessageKey<uint64_t>, page_size, std::less<MessageKey<uint64_t>>,
    Insert<uint64_t>, Update<uint64_t>, Delete>;

// insertion order results in irregular tree for traversal testing
constexpr uint8_t traversal_test[] = {
    1, 10, 8, 3, 6, 4, 7, 11, 9, 17, 2, 5, 15, 14, 13, 16, 12
};
constexpr auto traversal_test_size = sizeof(traversal_test) / sizeof(traversal_test[0]);

TEST(RbTree, Sizes) {
    ASSERT_EQ(sizeof(RbTreeTest<255>), 255);
    ASSERT_EQ(sizeof(RbTreeTest<(1ull << 16) - 2>), (1ull << 16) - 2);
    ASSERT_EQ(sizeof(RbTreeTest<(1ull << 32) - 4>), (1ull << 32) - 4);
    ASSERT_EQ(sizeof(RbTreeTest<1ull << 32>), 1ull << 32);
}

TEST(RbTree, LinearInsertion) {
    RbTreeTest<255> tree;

    uint32_t i = 0;
    while (tree.insert<2>({i++, 0}))
        tree.check_rb_invariants();
}

TEST(RbTree, LinearReverseInsertion) {
    RbTreeTest<255> tree;

    uint32_t i = std::numeric_limits<uint32_t>::max();
    while (tree.insert<2>({i--, 0}))
        tree.check_rb_invariants();
}

TEST(RbTree, SwitchingInsertion) {
    RbTreeTest<255> tree;

    uint32_t i = 0;
    uint32_t k = 0;
    while (tree.insert<2>({i++, k ^= 1}))
        tree.check_rb_invariants();
}

TEST(RbTree, IrregularInsertion) {
    RbTreeTest<1024> tree;

    for (auto i : traversal_test) {
        tree.insert<2>({0, i});
        tree.check_rb_invariants();
    }
}

TEST(RbTree, EmptyTreeIterators) {
    RbTreeTest<255> tree;
    ASSERT_EQ(tree.end(), tree.begin());
}

TEST(RbTree, NonEmptyTreeIterators) {
    RbTreeTest<255> tree;
    tree.insert<2>({0, 0});
    ASSERT_NE(tree.end(), tree.begin());
}

TEST(RbTree, IteratorBegin) {
    RbTreeTest<255> tree;
    tree.insert<2>({0, 0});
    tree.insert<2>({0, 1});

    ASSERT_EQ(2, tree.begin()->type());
}

TEST(RbTree, ForwardIterationLinear) {
    RbTreeTest<255> tree;

    uint32_t i = 0;
    while (tree.insert<1>({i, 0}, {i}))
        ++i;

    i = 0;
    for (auto ref : tree)
        ASSERT_EQ(i++, ref.as<1>().value);
}

TEST(RbTree, ForwardIterationLinearReverse) {
    RbTreeTest<255> tree;

    uint32_t i = std::numeric_limits<uint32_t>::max();
    while (tree.insert<1>({i, 0}, {i}))
        --i;

    for (auto ref : tree)
        ASSERT_EQ(++i, ref.as<1>().value);
}

TEST(RbTree, ForwardIterationSwitching) {
    RbTreeTest<255> tree;

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

TEST(RbTree, ForwardIterationIrregular) {
    RbTreeTest<1024> tree;

    for (auto i : traversal_test)
        ASSERT_TRUE(tree.insert<1>({0, i}, {i}));

    uint8_t i = 1;
    for (auto ref : tree)
        ASSERT_EQ(i++, ref.as<1>().value);
}

TEST(RbTree, Find) {
    RbTreeTest<1024> tree;
    ASSERT_EQ(tree.end(), tree.find({0, 0}));

    for (auto i : traversal_test)
        ASSERT_TRUE(tree.insert<1>({0, i}, {i}));

    ASSERT_EQ(tree.end(), tree.find({0, 0}));
    ASSERT_EQ(tree.end(), tree.find({1, 1}));
    ASSERT_EQ(tree.end(), tree.find({0, 100}));

    for (auto i : traversal_test)
        ASSERT_EQ(i, tree.find({0, i})->as<1>().value);
}

TEST(RbTree, LowerBound) {
    RbTreeTest<1024> tree;
    ASSERT_EQ(tree.end(), tree.lower_bound({0, 0}));

    uint32_t i = 1;
    while (tree.insert<1>({i, 0}, {i}))
        i += 5;

    ASSERT_EQ(tree.end(), tree.lower_bound({i, 0}));
    for (uint32_t j = 1; j < i; j += 5) {
        EXPECT_EQ(j, tree.lower_bound({j - 1, 0})->as<1>().value);
        EXPECT_EQ(j, tree.lower_bound({j, 0})->as<1>().value);
        if (j + 5 < i)
            EXPECT_EQ(j + 5, tree.lower_bound({j + 1, 0})->as<1>().value);
    }
}

TEST(RbTree, UpperBound) {
    RbTreeTest<1024> tree;
    ASSERT_EQ(tree.end(), tree.upper_bound({0, 0}));

    uint32_t i = 1;
    while (tree.insert<1>({i, 0}, {i}))
        i += 5;

    ASSERT_EQ(tree.end(), tree.upper_bound({i - 5, 0}));
    for (uint32_t j = 1; j < i; j += 5) {
        EXPECT_EQ(j, tree.upper_bound({j - 1, 0})->as<1>().value);
        if (j + 5 < i)
            EXPECT_EQ(j + 5, tree.upper_bound({j, 0})->as<1>().value);
    }
}

TEST(RbTree, SingleDelete) {
    RbTreeTest<255> tree;

    tree.insert<1>({1, 2}, {3});
    auto it = tree.find({1, 2});
    tree.erase(it);

    EXPECT_EQ(0, tree.size());
    tree.check_rb_invariants();
    ASSERT_EQ(tree.end(), tree.begin());
}

TEST(RbTree, FillLinearEmptyFill) {
    RbTreeTest<255> tree;

    uint32_t i = 0;
    while (tree.insert<2>({i++, 0})) {}
    EXPECT_EQ(i - 1, tree.size());

    auto it = tree.begin();
    while (it != tree.end()) {
        tree.erase(it);
        tree.check_rb_invariants();
        it = tree.begin();
    }
    EXPECT_EQ(0, tree.size());

    uint32_t j = 0;
    while (tree.insert<2>({j++, 0}))
        tree.check_rb_invariants();
    ASSERT_EQ(i, j);
}

TEST(RbTree, FillLinearReverseEmptyFill) {
    RbTreeTest<255> tree;

    uint32_t i = std::numeric_limits<uint32_t>::max();
    while (tree.insert<2>({i--, 0})) {}
    EXPECT_EQ(std::numeric_limits<uint32_t>::max() - i - 1, tree.size());

    auto it = tree.begin();
    while (it != tree.end()) {
        tree.erase(it);
        tree.check_rb_invariants();
        it = tree.begin();
    }
    EXPECT_EQ(0, tree.size());

    uint32_t j = 0;
    while (tree.insert<2>({j++, 0}))
        tree.check_rb_invariants();
    ASSERT_EQ(std::numeric_limits<uint32_t>::max() - i, j);
}

TEST(RbTree, FillSwitchingEmptyFill) {
    RbTreeTest<255> tree;

    uint32_t i = 0;
    uint32_t k = 0;
    while (tree.insert<2>({i++, k ^= 1}))
        tree.check_rb_invariants();
    EXPECT_EQ(i - 1, tree.size());

    auto it = tree.begin();
    while (it != tree.end()) {
        tree.erase(it);
        tree.check_rb_invariants();
        it = tree.begin();
    }
    EXPECT_EQ(0, tree.size());

    uint32_t j = 0;
    while (tree.insert<2>({j++, 0}))
        tree.check_rb_invariants();
    ASSERT_EQ(i, j);
}

TEST(RbTree, FillIrregularEmpty) {
    RbTreeTest<1024> tree;

    for (auto i : traversal_test)
        ASSERT_TRUE(tree.insert<2>({0, i}));
    EXPECT_EQ(traversal_test_size, tree.size());

    auto it = tree.begin();
    while (it != tree.end()) {
        tree.erase(it);
        tree.check_rb_invariants();
        it = tree.begin();
    }
    EXPECT_EQ(0, tree.size());
}

TEST(RbTree, FillLinearEmptyIrregular) {
    RbTreeTest<1024> tree;

    for (uint32_t i = 1; i <= traversal_test_size; ++i)
        ASSERT_TRUE(tree.insert<2>({0, i}));
    EXPECT_EQ(traversal_test_size, tree.size());


    for (auto i : traversal_test)
        tree.erase(tree.find({0, i}));
    EXPECT_EQ(0, tree.size());
}

// ---------------------------------------------------------------------------------------------------
}  // namespace
// ---------------------------------------------------------------------------------------------------
