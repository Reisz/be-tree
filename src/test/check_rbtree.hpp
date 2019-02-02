// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#ifndef SRC_TEST_CHECK_RBTREE_HPP_
#define SRC_TEST_CHECK_RBTREE_HPP_
// ---------------------------------------------------------------------------------------------------
#include <deque>
#include <tuple>
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
// ---------------------------------------------------------------------------------------------------
#endif  // SRC_TEST_CHECK_RBTREE_HPP_
