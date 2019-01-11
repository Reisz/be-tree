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

RBTREE_TEMPL typename RBTREE_CLASS::const_iterator RBTREE_CLASS::begin() const {
    auto current = ref(header.root_node);

    auto left = current;
    while (left) {
        current = left;
        left = ref(left->left);
    }

    return const_iterator(this, current);
}

RBTREE_TEMPL typename RBTREE_CLASS::const_iterator RBTREE_CLASS::end() const {
    return const_iterator(this, 0);
}

RBTREE_TEMPL typename RBTREE_CLASS::const_iterator RBTREE_CLASS::lower_bound(const Key &key) const {
    auto current = ref(header.root_node);

    auto result = end();
    while (current) {
        if (comp(current->key, key)) {
            current = ref(current->right);
        } else {
            result = const_iterator(this, current);
            if (!comp(key, current->key))
                break;
            current = ref(current->left);
        }
    }
    return result;
}

RBTREE_TEMPL typename RBTREE_CLASS::const_iterator RBTREE_CLASS::upper_bound(const Key &key) const {
    auto current = ref(header.root_node);
    if (!current)
        return end();

    auto result = end();
    while (current) {
        if (comp(key, current->key)) {
            result = const_iterator(this, current);
            current = ref(current->left);
        } else {
            current = ref(current->right);
        }
    }

    return result;
}

RBTREE_TEMPL typename RBTREE_CLASS::const_iterator RBTREE_CLASS::find(const Key &key) const {
    auto current = ref(header.root_node);

    while (current) {
        if (comp(current->key, key))
            current = ref(current->right);
        else if (comp(key, current->key))
            current = ref(current->left);
        else
            return const_iterator(this, current);
    }

    return end();
}

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
                rotate(parent, child = -child);
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
// ---------------------------------------------------------------------------------------------------
RBTREE_TEMPL typename RBTREE_CLASS::tag RBTREE_CLASS::const_reference::type() const {
    return tree.value_at<tag>(i);
}

RBTREE_TEMPL template<size_t I> const typename RBTREE_CLASS::template element_t<I> &RBTREE_CLASS::const_reference::as() const {
    assert(type() == I);
    return tree.value_at<RBValue<I>>(i).value;
}
// ---------------------------------------------------------------------------------------------------
RBTREE_TEMPL typename RBTREE_CLASS::const_iterator &RBTREE_CLASS::const_iterator::operator++() {
    auto current = tree->ref(i);

    // right child available: take path, then go all the way left to find smallest
    if (current->right) {
        current = tree->ref(current->right);
        do {
            i = current;
        } while ((current = tree->ref(current->left)));
    } else {  // look at parent otherwise
        auto parent = tree->ref(current->parent);
        // left side of parent: parent is next, continue with parent otherwise
        while (parent && parent->side(current) == RBNode::Right) {
            current = parent;
            parent = tree->ref(current->parent);
        }
        i = parent;
    }

    return *this;
}

RBTREE_TEMPL typename RBTREE_CLASS::const_iterator RBTREE_CLASS::const_iterator::operator++(int) {
    auto result = const_iterator(tree, i);
    ++this;
    return result;
}

RBTREE_TEMPL typename RBTREE_CLASS::const_iterator &RBTREE_CLASS::const_iterator::operator--() {
    // TODO
}

RBTREE_TEMPL typename RBTREE_CLASS::const_iterator RBTREE_CLASS::const_iterator::operator--(int) {
    auto result = iterator(tree, i);
    --this;
    return result;
}

RBTREE_TEMPL bool RBTREE_CLASS::const_iterator::operator==(const const_iterator &other) const {
    return tree == other.tree && i == other.i;
}

RBTREE_TEMPL bool RBTREE_CLASS::const_iterator::operator!=(const const_iterator &other) const {
    return !(*this == other);
}

RBTREE_TEMPL typename RBTREE_CLASS::const_reference RBTREE_CLASS::const_iterator::operator*() const {
    return const_reference(*tree, tree->ref(i)->value);
}

}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#endif  // SRC_RBTREE_HPP_
