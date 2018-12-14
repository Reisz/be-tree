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
        new (&value_at<I>(i)) RBValue<I>(value);

        if (header.node_count == 0) {
            RBNode &node = node_at(header.root_node = reserve_node()) = RBNode(key, i, 0);
            node.color = RBNode::Black;
        } else {
            node_pointer parent = header.root_node;
            node_pointer *child_ptr;

            // find correct place to add new node
            do {
                RBNode &node = node_at(parent);

                if (comp(node.key, key)) {
                    child_ptr = &node.right;
                } else if (comp(key, node.key)) {
                    child_ptr = &node.left;
                } else {
                    ;//throw;  // keys should never duplicate in the be-tree use-case
                }

                if (*child_ptr)
                    parent = *child_ptr;
            } while (*child_ptr);

            // add new node
            node_pointer node = *child_ptr = reserve_node();
            node_at(node) = RBNode(key, i, parent);

            node_pointer sibling, uncle, grandparent;
            typename RBNode::Child child;

            // fix rb invariants
            // https://de.wikipedia.org/wiki/Rot-Schwarz-Baum#Einf%C3%BCgen
            do {
                if (node == node_at(parent).left) {
                    child = RBNode::Left;
                    sibling = node_at(parent).right;
                } else {
                    child = RBNode::Right;
                    sibling = node_at(parent).left;
                }

                // case 1
                if (node_at(parent).color == RBNode::Black)
                    break;

                grandparent = node_at(parent).parent;
                uncle = node_at(grandparent).children[-child];

                // case 2
                if (uncle && node_at(uncle).color == RBNode::Red) {
                    node_at(parent).color = RBNode::Black;
                    node_at(uncle).color = RBNode::Black;
                    node_at(grandparent).color = RBNode::Red;

                    node = grandparent;
                    parent = node_at(node).parent;

                    continue;
                }

                // case 3
                if (parent == node_at(grandparent).children[-child]) {
                    rotate(parent, -child);

                    uncle = node;
                    node = parent;
                    parent = uncle;
                }

                // case 4
                rotate(grandparent, -child);
                node_at(parent).color = RBNode::Black;
                node_at(grandparent).color = RBNode::Red;
                break;
            } while ((parent = node_at(node).parent));

            if (!parent) {
                header.root_node = node;
                node_at(node).color = RBNode::Black;
            }
        }

        return true;
    }

    RBTREE_TEMPL void RBTREE_CLASS::rotate(node_pointer node, typename RBNode::Child child) {
        node_pointer parent = node_at(node).parent;
        node_pointer other = node_at(node).children[-child];

        //     P            P
        //     |            |
        //    (R)          (L)
        //    /(\)   ---> (/)\
        //   L   b        a   R
        //  / \     <---     / \
        // a   m            m   b

        // adapt child pointers
        if (parent) {
            node_pointer *parent_side = &(node == node_at(parent).left ? node_at(parent).left : node_at(parent).right);
            *parent_side = other;
        } else {
            header.root_node = other;
        }
        std::swap(node_at(other).children[child], node_at(node).children[-child]);

        // fix parent pointers
        node_at(other).parent = parent;
        node_at(node).parent = other;

        node_pointer m = node_at(node).children[-child];
        if (m)
            node_at(m).parent = node;
    }

}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#endif  // SRC_RBTREE_TCC_
