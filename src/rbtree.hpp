// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#ifndef SRC_RBTREE_HPP_
#define SRC_RBTREE_HPP_
// ---------------------------------------------------------------------------------------------------
#include "imlab/rbtree.h"
#include <utility>
#include <cstring>
// ---------------------------------------------------------------------------------------------------
namespace imlab {
// ---------------------------------------------------------------------------------------------------
RBTREE_TEMPL constexpr typename RBTREE_CLASS::Node::Child RBTREE_CLASS::Node::side(node_pointer i) const {
    assert(left == i || right == i);
    return left == i ? Left : Right;
}
// ---------------------------------------------------------------------------------------------------
RBTREE_TEMPL constexpr RBTREE_CLASS::RbTree() {
  static_assert(sizeof(*this) == page_size);
}

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

RBTREE_TEMPL typename RBTREE_CLASS::const_reference RBTREE_CLASS::front() const {
    auto current = ref(header.root_node);

    auto left = current;
    while (left) {
        current = left;
        left = ref(left->left);
    }

    return const_reference(this, current);
}

RBTREE_TEMPL typename RBTREE_CLASS::const_reference RBTREE_CLASS::back() const {
    auto current = ref(header.root_node);

    auto right = current;
    while (right) {
        current = right;
        right = ref(right->right);
    }

    return const_reference(this, current);
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

RBTREE_TEMPL size_t RBTREE_CLASS::size() const {
    return header.node_count - header.deleted;
}

RBTREE_TEMPL size_t RBTREE_CLASS::capacity_bytes() const {
    return header.free_space;
}

RBTREE_TEMPL template<size_t I> constexpr size_t RBTREE_CLASS::size_bytes() {
    return sizeof(Node) + sizeof(Value<I>);
}

RBTREE_TEMPL void RBTREE_CLASS::erase(const_iterator it) {
    assert(it.tree == this);
    auto node = ref(it.ref.i);

    // inner node with 2 children
    if (node->left && node->right) {
        // find largest node in left subtree
        auto next = ref(node->left);
        while (next->right)
            next = ref(next->right);

        // copy key & data from leaf
        node->key = next->key;
        node->value = next->value;

        // continue by deleting leaf
        node = next;
    }
    typename Node::Color color = node->color;
    mark_for_deletion(node);

    // should now have one or zero children
    assert(!node->left || !node->right);
    auto parent = ref(node->parent);
    auto problem = ref(node->children[-node->side(0)]);

    typename Node::Child child;

    // add single child or 0 to parent / as root
    if (parent) {
        child = parent->side(node);
        parent->children[child] = problem;
    } else {
        header.root_node = problem;
    }

    // update parent
    if (problem)
        problem->parent = parent;

    // node was red: done
    if (color == Node::Red)
        return;

    // replacement is red and original was black:
    // make replacement black -> done
    if (problem && problem->color == Node::Red) {
        problem->color = Node::Black;
        return;
    }

    // black leaf -> need to fix black depth
    node_ref sibling, cls_neph;
    if (parent) {
        sibling = ref(parent->children[-child]);

        if (sibling->color == Node::Red)
            goto case2;
        if (sibling->children[-child])
            goto case5;
        if (sibling->children[child])
            goto case4;
        if (parent->color == Node::Red)
            goto case3;

        sibling->color = Node::Red;
        problem = parent;

        while ((parent = ref(problem->parent))) {
            child = parent->side(problem);
            sibling = ref(parent->children[-child]);

            if (sibling->color == Node::Red)
                goto case2;
            if (ref(sibling->children[-child])->color == Node::Red)
                goto case5;
            if (ref(sibling->children[child])->color == Node::Red)
                goto case5;
            if (parent->color == Node::Red)
                goto case3;

            sibling->color = Node::Red;
            problem = parent;
        }
    }

    // case1:  // no parent
    header.root_node = problem;
    return;

    case2:  // red sibling, black parent and nephews
    sibling->color = Node::Black;
    parent->color = Node::Red;

    cls_neph = ref(sibling->children[child]);
    rotate(parent, child);
    if (!parent->parent)
        header.root_node = sibling;
    sibling = cls_neph;

    if (sibling->children[-child] && ref(sibling->children[-child])->color == Node::Red)
        goto case5;
    if (sibling->children[child] && ref(sibling->children[child])->color == Node::Red)
        goto case4;
    // continue with case3

    case3:  // red parent, black sibling and nephews
    sibling->color = Node::Red;
    parent->color = Node::Black;
    return;

    case4:  // black sibling, red close nephew and problem node
    cls_neph = ref(sibling->children[child]);
    cls_neph->color = Node::Black;

    rotate(sibling, -child);
    sibling->color = Node::Red;
    sibling = cls_neph;
    // continue with case5

    case5:  // black sibling, red far nephew and problem node
    sibling->color = parent->color;
    parent->color = Node::Black;
    ref(sibling->children[-child])->color = Node::Black;

    rotate(parent, child);
    if (!parent->parent)
        header.root_node = sibling;
}

RBTREE_TEMPL template<size_t I> std::enable_if_t<std::is_same_v<void, typename RBTREE_CLASS::template element_t<I>>, bool>
RBTREE_CLASS::insert(const Key &key) {
    auto result = insert<Value<I>>(key);
    if (result)
        new (&value_at<Value<I>>(result.value())) Value<I>();
    return result.has_value();
}

RBTREE_TEMPL template<size_t I> bool RBTREE_CLASS::insert(const Key &key, const element_t<I> &value) {
    auto result = insert<Value<I>>(key);
    if (result)
        new (&value_at<Value<I>>(result.value())) Value<I>(value);
    return result.has_value();
}

RBTREE_TEMPL template<size_t I> bool RBTREE_CLASS::insert(const Key &key, element_t<I> &&value) {
    auto result = insert<Value<I>>(key);
    if (result)
        new (&value_at<Value<I>>(result.value())) Value<I>(std::forward<element_t<I>>(value));
    return result.has_value();
}

RBTREE_TEMPL template<typename T> std::optional<typename RBTREE_CLASS::pointer>
RBTREE_CLASS::insert(const Key &key) {
    if (sizeof(T) + sizeof(Node) > inner_space()) {
        if (header.deleted)
            compress();
        if (sizeof(T) + sizeof(Node) > inner_space())
            return {};
    }

    // find parent
    auto parent = ref(header.root_node);
    typename Node::Child child;
    if (parent) {
        while (true) {
            if (comp(parent->key, key)) {
                child = Node::Right;
            } else if (comp(key, parent->key)) {
                child = Node::Left;
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
    auto node = emplace_node(key, i, parent);

    // https://de.wikipedia.org/wiki/Rot-Schwarz-Baum#Einf%C3%BCgen
    if (parent) {
        parent->children[child] = node;

        // fix rb invariants
        do {
            child = parent->side(node);

            // case 1: parent is black -> done
            if (parent->color == Node::Black)
                break;

            auto grandp = ref(parent->parent);
            auto uncle = ref(grandp->children[-grandp->side(parent)]);

            // case 2: parent & uncle are red
            if (uncle && uncle->color == Node::Red) {
                // make them black instead and continue with grandparent
                parent->color = uncle->color = Node::Black;

                grandp->color = Node::Red;
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
            parent->color = Node::Black;
            grandp->color = Node::Red;
        } while ((parent = ref(node->parent)));
    }

    // iteration ended with new root node (or no iteration happened)
    if (!parent) {
        header.root_node = node;
        node->color = Node::Black;
    }

    return i;
}

RBTREE_TEMPL void RBTREE_CLASS::rotate(node_ref node, typename Node::Child child) {
    auto parent = ref(node->parent);
    auto other = ref(node->children[-child]);
    auto m = ref(other->children[child]);

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

RBTREE_TEMPL inline size_t RBTREE_CLASS::inner_space() const {
    return header.data_start - header.node_count * sizeof(Node);
}

RBTREE_TEMPL inline typename RBTREE_CLASS::node_ref RBTREE_CLASS::ref(node_pointer i) {
    return { i , i ? reinterpret_cast<Node*>(data) + (i - 1) : nullptr };
}

RBTREE_TEMPL inline typename RBTREE_CLASS::const_node_ref RBTREE_CLASS::ref(node_pointer i) const {
    return { i , i ? reinterpret_cast<const Node*>(data) + (i - 1) : nullptr };
}

RBTREE_TEMPL template<typename... Args> typename RBTREE_CLASS::node_ref RBTREE_CLASS::emplace_node(Args &&...a) {
    node_pointer i = header.node_count++;
    header.free_space -= sizeof(Node);

    auto result = ref(i + 1);
    new (result.node) Node(a...);

    return ref(i + 1);
}

RBTREE_TEMPL void RBTREE_CLASS::mark_for_deletion(node_ref node) {
    assert(node->color != Node::Deleted);
    auto size = sizeof_value(node->value);
    header.free_space += size + sizeof(Node);

    if (node->value == header.data_start) {
        header.data_start += size;

        assert(node == header.node_count);
        --header.node_count;
        return;
    }

    node->color = Node::Deleted;
    ++header.deleted;
}

RBTREE_TEMPL void RBTREE_CLASS::compress() {
    assert(header.deleted);

    pointer data_move = 0;
    node_pointer node_move = 0;
    for (node_pointer i = 1; i <= header.node_count; ++i) {
        auto node = ref(i);

        if (node->color == Node::Deleted) {
            size_t size = sizeof_value(node->value);

            ++node_move;
            data_move += size;
        } else if (node_move) {
            node_pointer target = i - node_move;
            pointer val_target = node->value + data_move;
            node->value = val_target;

            auto parent = ref(node->parent), left = ref(node->left), right = ref(node->right);
            if (parent)
                parent->children[parent->side(node)] = target;
            else
                header.root_node = target;

            if (left)
                left->parent = target;
            if (right)
                right->parent = target;

            std::memmove(data + val_target, data + node->value, sizeof_value(node->value));
            std::memmove(ref(target).node, ref(i).node, sizeof(Node));
        }
    }

    header.node_count -= header.deleted;
    header.data_start = ref(header.node_count)->value;
    header.deleted = 0;

    assert(inner_space() == header.free_space);
}

RBTREE_TEMPL template<typename T> inline T &RBTREE_CLASS::value_at(pointer i) {
    return *(reinterpret_cast<T*>(data + i));
}

RBTREE_TEMPL template<typename T> inline const T &RBTREE_CLASS::value_at(pointer i) const {
    return *(reinterpret_cast<const T*>(data + i));
}

RBTREE_TEMPL template<typename T> typename RBTREE_CLASS::pointer RBTREE_CLASS::reserve_value() {
    pointer i = header.data_start - sizeof(T);
    header.free_space -= sizeof(T);
    return header.data_start = i;
}

RBTREE_TEMPL size_t RBTREE_CLASS::sizeof_value(pointer i) const {
    return sizes[value_at<Tag>(i).type];
}
// ---------------------------------------------------------------------------------------------------
RBTREE_TEMPL const Key &RBTREE_CLASS::const_reference::key() const {
    return tree->ref(i)->key;
}

RBTREE_TEMPL typename RBTREE_CLASS::tag RBTREE_CLASS::const_reference::type() const {
    return tree->value_at<Tag>(tree->ref(i)->value).type;
}

RBTREE_TEMPL template<size_t I> const typename RBTREE_CLASS::template element_t<I> &RBTREE_CLASS::const_reference::as() const {
    assert(type() == I);
    return tree->value_at<Value<I>>(tree->ref(i)->value).value;
}

RBTREE_TEMPL size_t RBTREE_CLASS::const_reference::size_bytes() const {
    return sizeof(Node) + sizes[type()];
}
// ---------------------------------------------------------------------------------------------------
RBTREE_TEMPL typename RBTREE_CLASS::const_iterator &RBTREE_CLASS::const_iterator::operator++() {
    auto current = tree->ref(ref.i);

    // right child available: take path, then go all the way left to find smallest
    if (current->right) {
        current = tree->ref(current->right);
        do {
            ref.i = current;
        } while ((current = tree->ref(current->left)));
    } else {  // look at parent otherwise
        auto parent = tree->ref(current->parent);
        // left side of parent: parent is next, continue with parent otherwise
        while (parent && parent->side(current) == Node::Right) {
            current = parent;
            parent = tree->ref(current->parent);
        }
        ref.i = parent;
    }

    return *this;
}

RBTREE_TEMPL typename RBTREE_CLASS::const_iterator RBTREE_CLASS::const_iterator::operator++(int) {
    auto result = const_iterator(tree, ref.i);
    ++(*this);
    return result;
}

RBTREE_TEMPL bool RBTREE_CLASS::const_iterator::operator==(const const_iterator &other) const {
    return tree == other.tree && ref.i == other.ref.i;
}

RBTREE_TEMPL bool RBTREE_CLASS::const_iterator::operator!=(const const_iterator &other) const {
    return !(*this == other);
}

RBTREE_TEMPL typename RBTREE_CLASS::const_reference RBTREE_CLASS::const_iterator::operator*() const {
    return ref;
}

RBTREE_TEMPL const typename RBTREE_CLASS::const_reference *RBTREE_CLASS::const_iterator::operator->() const {
    return &ref;
}

}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#endif  // SRC_RBTREE_HPP_
