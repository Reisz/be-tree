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
// ---------------------------------------------------------------------------------------------------
}  // namespace
// ---------------------------------------------------------------------------------------------------
