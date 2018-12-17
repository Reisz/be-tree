// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#ifndef SRC_RBTREE_TCC_
#define SRC_RBTREE_TCC_

#include "imlab/rbtree.h"
#include <utility>
// ---------------------------------------------------------------------------------------------------
namespace imlab {
    #define RBTREE_TEMPL \
        template<typename Key, size_t page_size, typename Compare, typename... Ts>
    #define RBTREE_CLASS \
        RBTree<Key, page_size, Compare, Ts...>

    RBTREE_TEMPL template<typename T> std::optional<typename RBTREE_CLASS::pointer> RBTREE_CLASS::insert(const Key &key) {
        if (sizeof(T) + sizeof(RBNode) > header.free_space)
           return {};

        pointer i = reserve_value<T>();

        // find parent
        NodeRef parent = ref(header.root_node);
        node_pointer *child_ptr;
        if (parent) {
            do {
                if (comp(parent->key, key)) {
                    child_ptr = &parent->right;
                } else if (comp(key, parent->key)) {
                    child_ptr = &parent->left;
                } else {
                    throw;
                }

                if (*child_ptr)
                    parent = ref(*child_ptr);
            } while (*child_ptr);
        }

        // add new node
        NodeRef node = reserve_node();
        *node = RBNode(key, i, parent);

        // https://de.wikipedia.org/wiki/Rot-Schwarz-Baum#Einf%C3%BCgen
        if (parent) {
            *child_ptr = node;
            node->parent = parent;

            // fix rb invariants
            do {
                typename RBNode::Child child = parent->side(node);

                // case 1
                if (parent->color == RBNode::Black)
                    break;

                NodeRef grandp = ref(parent->parent);
                NodeRef uncle = ref(grandp->children[-child]);

                // case 2
                if (uncle && uncle->color == RBNode::Red) {
                    parent->color = RBNode::Black;
                    uncle->color = RBNode::Black;
                    grandp->color = RBNode::Red;

                    node = grandp;
                    parent = ref(node->parent);

                    continue;
                }

                // case 3
                if (parent == grandp->children[-child]) {
                    rotate(parent, -child);

                    uncle = node;
                    node = parent;
                    parent = uncle;
                }

                // case 4
                rotate(grandp, -child);
                parent->color = RBNode::Black;
                grandp->color = RBNode::Red;
                break;
            } while ((parent = ref(node->parent)));
        }

        if (!parent) {
            header.root_node = node;
            node->color = RBNode::Black;
            node->parent = 0;
        }

        return i;
    }

    RBTREE_TEMPL void RBTREE_CLASS::rotate(NodeRef node, typename RBNode::Child child) {
        NodeRef parent = ref(node->parent);
        NodeRef other = ref(node->children[-child]);

        //     P            P
        //     |            |
        //    (R)          (L)
        //    / \\   ---> // \
        //   L   b        a   R
        //  / \     <---     / \
        // a   m            m   b

        // adapt child pointers
        if (parent) {
            parent->children[parent->side(node)] = other;
        } else {
            header.root_node = other;
        }

        // replace other with child of other
        node->children[-child] = other->children[child];
        // replace child of other with node
        other->children[child] = node;

        // fix parent pointers
        other->parent = parent;
        node->parent = other;

        NodeRef m = ref(node->children[-child]);
        if (m)
            m->parent = node;
    }

}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#endif  // SRC_RBTREE_TCC_
