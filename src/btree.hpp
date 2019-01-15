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
    if (comp(keys[this->count - 1], key))
        return right_child;
    return children[::lower_bound(keys, this->count, key, comp)];
}

IMLAB_BTREE_TEMPL bool IMLAB_BTREE_CLASS::InnerNode::full() const {
    return this->count >= kCapacity;
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
IMLAB_BTREE_TEMPL constexpr IMLAB_BTREE_CLASS::LeafNode::LeafNode()
    : Node(0) {}
IMLAB_BTREE_TEMPL uint32_t IMLAB_BTREE_CLASS::LeafNode::lower_bound(const Key &key) const {
    if (comp(keys[this->count - 1], key))
        return this->count;
    return ::lower_bound(keys, this->count, key, comp);
}

IMLAB_BTREE_TEMPL const T &IMLAB_BTREE_CLASS::LeafNode::at(uint32_t idx) const {
    assert(idx < this->count);
    return values[idx];
}

IMLAB_BTREE_TEMPL T &IMLAB_BTREE_CLASS::LeafNode::at(uint32_t idx) {
    assert(idx < this->count);
    return values[idx];
}

IMLAB_BTREE_TEMPL bool IMLAB_BTREE_CLASS::LeafNode::is_equal(const Key &key, uint32_t idx) const {
    return idx < this->count && keys[idx] == key;
}

IMLAB_BTREE_TEMPL bool IMLAB_BTREE_CLASS::LeafNode::full() const {
    return this->count >= kCapacity;
}

IMLAB_BTREE_TEMPL T &IMLAB_BTREE_CLASS::LeafNode::make_space(const Key &key, uint32_t idx) {
    assert(idx < kCapacity);

    // move to the right
    for (uint32_t j = this->count; j > idx; --j) {
        keys[j] = keys[j - 1];
        values[j] = values[j - 1];
    }

    // insert
    keys[idx] = key;
    ++this->count;
    return values[idx];
}

IMLAB_BTREE_TEMPL void IMLAB_BTREE_CLASS::LeafNode::erase(uint32_t idx) {
    assert(idx < this->count);

    // move to the left
    for (; idx < this->count - 1; ++idx) {
        keys[idx] = keys[idx + 1];
        values[idx] = values[idx + 1];
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
IMLAB_BTREE_TEMPL typename IMLAB_BTREE_CLASS::iterator IMLAB_BTREE_CLASS::begin() {
    if (!root)
        return end();

    auto fix = this->fix_exclusive(*root);
    while (!fix.template as<Node>()->is_leaf()) {
        assert(fix.template as<Node>().count > 0);
        fix = this->fix_exclusive(fix.template as<InnerNode>()->children[0]);
    }

    return iterator(*this, fix, 0);
}

IMLAB_BTREE_TEMPL typename IMLAB_BTREE_CLASS::iterator IMLAB_BTREE_CLASS::end() {
    return iterator(*this, {}, 0);
}

IMLAB_BTREE_TEMPL typename IMLAB_BTREE_CLASS::iterator IMLAB_BTREE_CLASS::find(const Key &key) {
    if (!root)
        return end();

    auto fix = this->fix_exclusive(*root);
    while (!fix.template as<Node>()->is_leaf()) {
        assert(fix.template as<Node>()->count > 0);
        fix = this->fix_exclusive(fix.template as<InnerNode>()->lower_bound(key));
    }

    auto &leaf = *fix.template as<LeafNode>();
    auto i = leaf.lower_bound(key);

    return leaf.is_equal(key, i) ? iterator(*this, std::move(fix), i) : end();
}

IMLAB_BTREE_TEMPL void IMLAB_BTREE_CLASS::insert(const Key &key, const T &value) {
    T *i = insert_internal(key);
    if (i)
        *i = value;
}

IMLAB_BTREE_TEMPL void IMLAB_BTREE_CLASS::insert(const Key &key, T &&value) {
    T *i = insert_internal(key);
    if (i)
        *i = value;
}

IMLAB_BTREE_TEMPL void IMLAB_BTREE_CLASS::insert_or_assign(const Key &key, const T &value) {
    insert_or_assign_internal(key) = value;
}

IMLAB_BTREE_TEMPL void IMLAB_BTREE_CLASS::insert_or_assign(const Key &key, T &&value) {
    insert_or_assign_internal(key) = value;
}

IMLAB_BTREE_TEMPL void IMLAB_BTREE_CLASS::erase(const Key &key) {
    auto ir = exclusive_find_node(key);
    auto &leaf = *ir.fix.template as<LeafNode>();
    auto idx = leaf.lower_bound(key);
    if (leaf.is_equal(key, idx))
        leaf.erase(idx);

    // TODO join ?
}

IMLAB_BTREE_TEMPL typename BufferManager<page_size>::Fix IMLAB_BTREE_CLASS::root_fix() {
    if (root)
        return this->fix(root);
    return {};
}

IMLAB_BTREE_TEMPL typename BufferManager<page_size>::ExclusiveFix IMLAB_BTREE_CLASS::root_fix_exclusive() {
    if (root)
        return this->fix_exclusive(*root);

    root = next_page_id;
    return new_leaf();
}

IMLAB_BTREE_TEMPL typename BufferManager<page_size>::ExclusiveFix IMLAB_BTREE_CLASS::new_leaf() {
    auto fix = this->fix_exclusive(next_page_id++);
    new (fix.data()) LeafNode();
    ++count;

    return fix;
}

IMLAB_BTREE_TEMPL typename BufferManager<page_size>::ExclusiveFix IMLAB_BTREE_CLASS::new_inner(const Node &child) {
    auto fix = this->fix_exclusive(next_page_id++);
    new (fix.data()) InnerNode(child);

    return fix;
}

IMLAB_BTREE_TEMPL T *IMLAB_BTREE_CLASS::insert_internal(const Key &key) {
    // TODO early split when really full
    auto ir = exclusive_find_node(key);
    auto *leaf = ir.fix.template as<LeafNode>();

    // check for key
    auto idx = leaf->lower_bound(key);
    if (leaf->is_equal(key, idx))
        return nullptr;

    // check for space, split if unavailable
    if (leaf->count >= LeafNode::kCapacity) {
        ir = insert_full_lock_rec_split(key);
        leaf = ir.fix.template as<LeafNode>();
        idx = leaf->lower_bound(key);
    }

    return &leaf->make_space(key, idx);
}

IMLAB_BTREE_TEMPL T &IMLAB_BTREE_CLASS::insert_or_assign_internal(const Key &key) {
    // TODO try no split if really empty
    auto ir = insert_lc_early_split(key);

    auto &leaf = *ir.fix.template as<LeafNode>();
    auto idx = leaf.lower_bound(key);

    if (leaf.is_equal(key, idx))
        return leaf.at(idx);
    else
        return leaf.make_space(key, idx);
}

IMLAB_BTREE_TEMPL typename IMLAB_BTREE_CLASS::CoupledFixes IMLAB_BTREE_CLASS::split_inner(CoupledFixes cf, const Key &key) {
    // TODO

    return cf;
}

IMLAB_BTREE_TEMPL typename IMLAB_BTREE_CLASS::CoupledFixes IMLAB_BTREE_CLASS::split_leaf(CoupledFixes cf, const Key &key) {
    // TODO

    return cf;
}

IMLAB_BTREE_TEMPL typename IMLAB_BTREE_CLASS::CoupledFixes IMLAB_BTREE_CLASS::insert_lc_early_split(const Key &key) {
    CoupledFixes cf = { root_fix_exclusive() };

    while (!cf.fix.template as<Node>()->is_leaf()) {
        auto &inner = *cf.fix.template as<InnerNode>();
        if (inner.full())
            cf = split_inner(std::move(cf), key);

        cf.advance(this->fix_exclusive(cf.fix.template as<InnerNode>()->lower_bound(key)));
    }

    if (cf.fix.template as<LeafNode>()->full())
        cf = split_leaf(std::move(cf), key);

    return cf;
}

IMLAB_BTREE_TEMPL typename IMLAB_BTREE_CLASS::CoupledFixes IMLAB_BTREE_CLASS::exclusive_find_node(const Key &key) {
    CoupledFixes cf = { root_fix_exclusive() };

    while (!cf.fix.template as<Node>()->is_leaf())
        cf.advance(this->fix_exclusive(cf.fix.template as<InnerNode>()->lower_bound(key)));

    return cf;
}

IMLAB_BTREE_TEMPL typename IMLAB_BTREE_CLASS::CoupledFixes IMLAB_BTREE_CLASS::insert_full_lock_rec_split(const Key &key) {
    std::vector<typename BufferManager<page_size>::ExclusiveFix> fixes;
    fixes.push_back(root_fix_exclusive());

    while (!fixes.back().template as<Node>()->is_leaf())
        fixes.push_back(this->fix_exclusive(fixes.back().template as<InnerNode>()->lower_bound(key)));

    auto it = fixes.rbegin();
    if (it->template as<LeafNode>()->full()) {
        // TODO split leaf
        ++it;
        for (; it != fixes.rend(); ++it) {
            if (!it->template as<InnerNode>()->full())
                break;
            // TODO split inner
        }
    }

    return { std::move(fixes.back()), std::move(*++fixes.rbegin()) };
}

IMLAB_BTREE_TEMPL uint64_t IMLAB_BTREE_CLASS::size() const {
    return count;
}

IMLAB_BTREE_TEMPL uint64_t IMLAB_BTREE_CLASS::capacity() const {
    return leaf_count * LeafNode::kCapacity;
}
// ---------------------------------------------------------------------------------------------------
IMLAB_BTREE_TEMPL typename IMLAB_BTREE_CLASS::iterator &IMLAB_BTREE_CLASS::iterator::operator++() {
    auto &leaf = *fix.template as<LeafNode>();
    if (++i > leaf.count) {
        if (leaf.next)
            fix = segment.fix_exclusive(leaf.next);
        else
            fix.unfix();

        i = 0;
    }
}

IMLAB_BTREE_TEMPL bool IMLAB_BTREE_CLASS::iterator::operator==(const iterator &other) const {
    return fix.data() == other.fix.data() && i == other.i;
}

IMLAB_BTREE_TEMPL bool IMLAB_BTREE_CLASS::iterator::operator!=(const iterator &other) const {
    return !(*this == other);
}

IMLAB_BTREE_TEMPL typename IMLAB_BTREE_CLASS::reference IMLAB_BTREE_CLASS::iterator::operator*() {
    return fix.template as<LeafNode>()->at(i);
}

IMLAB_BTREE_TEMPL typename IMLAB_BTREE_CLASS::pointer IMLAB_BTREE_CLASS::iterator::operator->() {
    return &fix.template as<LeafNode>()->at(i);
}
// ---------------------------------------------------------------------------------------------------
IMLAB_BTREE_TEMPL void IMLAB_BTREE_CLASS::CoupledFixes::advance(typename BufferManager<page_size>::ExclusiveFix next) {
    prev = std::move(fix);
    fix = std::move(next);
}
// ---------------------------------------------------------------------------------------------------
}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#endif  // SRC_BTREE_HPP_
