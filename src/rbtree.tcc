// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#ifndef SRC_RBTREE_TCC_
#define SRC_RBTREE_TCC_

// ---------------------------------------------------------------------------------------------------
namespace imlab {
    #define RBTREE_TEMPL \
        template<typename Key, size_t page_size, typename Compare, typename... Ts>
    #define RBTREE_CLASS \
        RBTree<Key, page_size, Compare, Ts...>

    RBTREE_TEMPL template<size_t I> bool RBTREE_CLASS::insert(const Key &key, const element_t<I> &value) {
        if (sizeof(RBValue<I>) + sizeof(RBNode) > header.free_space)
           return false;

        pointer i = reserve_value<I>();
        value_at<I>(i) = RBValue(value);

        if (header.node_count == 0) {
            RBNode &node = node_at(header.root_node = reserve_node()) = RBNode(key, i, 0);
            node.color = RBNode::Black;
        } else {
            node_pointer parent = header.root_node;
            node_pointer *child_ptr;

            // find correct place to add new node
            do {
                RBNode &node = node_at(parent);

                if (Compare(node.key, key)) {
                    child_ptr = &node.right;
                } else if (Compare(key, node.key)) {
                    child_ptr = &node.left;
                } else {
                    throw;  // keys should never duplicate in the be-tree use-case
                }

                if (*child_ptr)
                    parent = *child_ptr;
            } while (*child_ptr);

            // add new node
            node_pointer node = *child_ptr = reserve_node();
            node_at(node) = RBNode(key, i, parent);

            // fix rb invariants
            do {
                // TODO
            } while ((parent = node_at(node).parent));
        }

        return true;
    }
}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#endif  // SRC_RBTREE_TCC_
