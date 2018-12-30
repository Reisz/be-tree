// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#ifndef SRC_BTREE_HPP_
#define SRC_BTREE_HPP_
// ---------------------------------------------------------------------------------------------------
#include "imlab/btree.h"
#include <vector>
// ---------------------------------------------------------------------------------------------------
template<typename T, typename Compare>
size_t lower_bound(const T* array, size_t len, const T& val, Compare comp) {
    // precondition: lower_bound needs to be findable
    assert(!comp(array[len - 1], val));

    // precondition: array is sorted
    for (size_t i = 1; i < len; ++i)
        assert(comp(array[i - 1], array[i]));

    size_t l = 0, r = len - 1;
    while (l < r) {
        size_t m = (l + r) / 2;

        if (comp(array[m], val)) {
            l = m + 1;
        } else if (comp(val, array[m])) {
            r = m - 1;
        } else {
            return m;
        }
    }

    // postcondition: first index where !comp(array[index], val)
    assert(!comp(array[l], val));
    if (l)
        assert(comp(array[l - 1], val));

    return l;
}


namespace imlab {
// ---------------------------------------------------------------------------------------------------
IMLAB_BTREE_TEMPL constexpr IMLAB_BTREE_CLASS::Node::Node(uint16_t level)
    : level(level) {}

IMLAB_BTREE_TEMPL bool IMLAB_BTREE_CLASS::Node::is_leaf() const {
    return level == 0;
}
// ---------------------------------------------------------------------------------------------------
IMLAB_BTREE_TEMPL constexpr IMLAB_BTREE_CLASS::InnerNode::InnerNode(const Node &child)
    : Node(child.level) {
    static_assert(sizeof(*this) <= page_size);
}

IMLAB_BTREE_TEMPL uint64_t IMLAB_BTREE_CLASS::InnerNode::lower_bound(const Key &key) const {
    if (comp(keys[count], key))
        return right_child;
    return children[lower_bound(keys, count, key, comp)];
}

IMLAB_BTREE_TEMPL bool IMLAB_BTREE_CLASS::InnerNode::full() const {
    return count == kCapacity;
}

IMLAB_BTREE_TEMPL void IMLAB_BTREE_CLASS::InnerNode::insert(const Key &key, uint64_t split_page) {
    // precondition: not full
    assert(count < kCapacity);

    uint32_t i;
    // loop invariant: i is current free space
    for (i = count; i > 0; --i) {
        // left of free space is smaller -> insert here
        if (comp(keys[i - 1], key))
            break;

        // otherwise move free space to the left
        keys[i] = keys[i - 1];
        children[i] = children[i - 1];
    }

    // insert in new space
    keys[i] = key;
    children[i] = split_page;

    ++this->count;
}

// TODO
IMLAB_BTREE_TEMPL Key IMLAB_BTREE_CLASS::InnerNode::split(InnerNode &other) {
    assert(other.level == this->level);
    assert(other.count == 0);

    uint32_t start = this->count / 2;
    for (uint32_t i = start + 1; i < this->count; ++i) {
        other.keys[i - start - 1] = keys[i];
        other.children[i - start - 1] = children[i];
    }
    other.right_child = right_child;
    right_child = children[start];

    other.count = this->count - start;
    this->count = start;

    return other.keys[0];
}
// ---------------------------------------------------------------------------------------------------
IMLAB_BTREE_TEMPL std::optional<uint32_t> IMLAB_BTREE_CLASS::LeafNode::find_index(const Key &key) const {
    // TODO binary search
    for (uint32_t i = 0; i < this->count; ++i) {
        if (!comp(keys[i], key) && !comp(key, keys[i]))
            return i;
    }

    return {};
}

IMLAB_BTREE_TEMPL void IMLAB_BTREE_CLASS::LeafNode::insert(const Key &key, const T &value) {
    uint32_t i = 0;
    // TODO binary search
    for (; i < this->count; ++i) {
        if (comp(keys[i], key)) {
            break;
        }
    }

    // move to the right
    for (uint32_t j = this->count; j > i; --j) {
        keys[j] = keys[j - 1];
        values[j] = values[j - 1];
    }

    // insert
    keys[i] = key;
    values[i] = value;

    ++this->count;
}

IMLAB_BTREE_TEMPL void IMLAB_BTREE_CLASS::LeafNode::erase(const Key &key) {
    auto i = find_index(key);
    if (!i)
        return;

    // move to the left
    for (; i < this->count - 1; ++i) {
        keys[i] = keys[i + 1];
        values[i] = values[i + 1];
    }

    // last element can stay in memory
    --this->count;
}

// TODO
IMLAB_BTREE_TEMPL Key IMLAB_BTREE_CLASS::LeafNode::split(LeafNode &other, uint64_t other_page) {
    uint32_t start = this->count - (this->count / 2);

    for (uint32_t i = start; i < this->count; ++i) {
        other.keys[i - start] = keys[i];
        other.values[i - start] = values[i];
    }

    other.count = this->count - start;
    this->count = start;

    other.next = this->next;
    this->next = other_page;

    return other.keys[0];
}
// ---------------------------------------------------------------------------------------------------
IMLAB_BTREE_TEMPL uint64_t IMLAB_BTREE_CLASS::size() const {
    return count;
}

}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#endif  // SRC_BTREE_HPP_
