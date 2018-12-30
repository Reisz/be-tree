// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#ifndef SRC_RBTREE_HPP_
#define SRC_RBTREE_HPP_
// ---------------------------------------------------------------------------------------------------
#include "imlab/rbtree.h"
#include <utility>
// ---------------------------------------------------------------------------------------------------
namespace imlab {
    #define RBTREE_TEMPL \
        template<typename Key, size_t page_size, typename Compare, typename... Ts>
    #define RBTREE_CLASS \
        RBTree<Key, page_size, Compare, Ts...>

    RBTREE_TEMPL template<typename T> std::optional<typename RBTREE_CLASS::pointer>
    RBTREE_CLASS::insert(const Key &key) {
        if (sizeof(T) + sizeof(RBNode) > header.free_space)
           return {};

        // find parent
        NodeRef parent = ref(header.root_node);
        typename RBNode::Child child;
        if (parent) {
            while (true) {
                if (comp(parent->key, key)) {
                    child = RBNode::Right;
                } else if (comp(key, parent->key)) {
                    child = RBNode::Left;
                } else {
                    throw;  // be-tree should never duplicate keys
                }

                // found insert spot
                if (!parent->children[child])
                    break;

                parent = ref(parent->children[child]);
            }
        }

        // add new node
        pointer i = reserve_value<T>();
        NodeRef node = emplace_node(key, i, parent);

        // https://de.wikipedia.org/wiki/Rot-Schwarz-Baum#Einf%C3%BCgen
        if (parent) {
            parent->children[child] = node;

            // fix rb invariants
            do {
                child = parent->side(node);

                // case 1: parent is black -> done
                if (parent->color == RBNode::Black)
                    break;

                NodeRef grandp = ref(parent->parent);
                NodeRef uncle = ref(grandp->children[-grandp->side(parent)]);

                // case 2: parent & uncle are red
                if (uncle && uncle->color == RBNode::Red) {
                    // make them black instead and continue with grandparent
                    parent->color = uncle->color = RBNode::Black;

                    grandp->color = RBNode::Red;
                    node = grandp;
                    continue;
                }

                // case 3: no / black uncle & zigzag from grandparent to node
                if (parent == grandp->children[-child]) {
                    // fix zigzag -> now in case 4
                    rotate(parent, -child);
                    std::swap(node, parent);
                }

                // case 4: no / black uncle & stright line from grandparent to node
                // rotate around grandparent to balance out
                rotate(grandp, -child);
                parent->color = RBNode::Black;
                grandp->color = RBNode::Red;
            } while ((parent = ref(node->parent)));
        }

        // iteration ended with new root node (or no iteration happened)
        if (!parent) {
            header.root_node = node;
            node->color = RBNode::Black;
        }

        return i;
    }

    RBTREE_TEMPL void RBTREE_CLASS::rotate(NodeRef node, typename RBNode::Child child) {
        NodeRef parent = ref(node->parent);
        NodeRef other = ref(node->children[-child]);
        NodeRef m = ref(other->children[child]);

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

        // replace other with m in node
        node->children[-child] = m;
        // replace m with node in other
        other->children[child] = node;

        // fix parent pointers
        other->parent = parent;
        node->parent = other;
        if (m)
            m->parent = node;
    }

}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#endif  // SRC_RBTREE_HPP_
