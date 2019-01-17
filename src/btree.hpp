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
    assert(len > 0 && !comp(array[len - 1], val));

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

    assert(l >= 0 && l < len);
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
IMLAB_BTREE_TEMPL constexpr IMLAB_BTREE_CLASS::InnerNode::InnerNode(uint16_t level)
    : Node(level) {
    static_assert(sizeof(*this) <= page_size);
    assert(level > 0);
}

IMLAB_BTREE_TEMPL uint64_t IMLAB_BTREE_CLASS::InnerNode::begin() const {
    return children[0];
}

IMLAB_BTREE_TEMPL uint64_t IMLAB_BTREE_CLASS::InnerNode::lower_bound(const Key &key) const {
    assert(this->count > 0);
    if (comp(keys[this->count - 1], key))
        return children[this->count];
    return children[::lower_bound(keys, this->count, key, comp)];
}

IMLAB_BTREE_TEMPL bool IMLAB_BTREE_CLASS::InnerNode::full() const {
    return this->count >= kCapacity;
}

IMLAB_BTREE_TEMPL void IMLAB_BTREE_CLASS::InnerNode::init(uint64_t left) {
    children[0] = left;
}

IMLAB_BTREE_TEMPL void IMLAB_BTREE_CLASS::InnerNode::insert(const Key &key, uint64_t split_page) {
    // precondition: not full
    assert(this->count < kCapacity);

    uint32_t i;
    // loop invariant: i is current free space
    for (i = this->count; i > 0; --i) {
        // left of free space is smaller -> insert here
        if (comp(keys[i - 1], key))
            break;

        // otherwise move free space to the left
        keys[i] = keys[i - 1];
        children[i + 1] = children[i];
    }

    // insert in new space
    keys[i] = key;
    children[i + 1] = split_page;

    ++this->count;
}

IMLAB_BTREE_TEMPL Key IMLAB_BTREE_CLASS::InnerNode::split(InnerNode &other) {
    assert(this->count > 2);
    assert(other.level == this->level);
    assert(other.count == 0);

    // Transformation (start = 2 gets pushed up)
    //   k0  k1  k2  k3   ->   k0  k1        k3
    // c0  c1  c2  c3  c4 -> c0  c1  c2    c3  c4
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //   k0  k1  k2  k3  k4   ->   k0  k1        k3  k4
    // c0  c1  c2  c3  c4  c5 -> c0  c1  c2    c3  c4  c5

    uint32_t start = this->count / 2;
    uint32_t i;
    for (i = start + 1; i < this->count; ++i) {
        other.keys[i - start - 1] = keys[i];
        other.children[i - start - 1] = children[i];
    }
    other.children[i - start - 1] = children[i];

    other.count = this->count - start - 1;
    this->count = start - 1;

    return keys[start];
}
// ---------------------------------------------------------------------------------------------------
IMLAB_BTREE_TEMPL constexpr IMLAB_BTREE_CLASS::LeafNode::LeafNode()
    : Node(0) {}
IMLAB_BTREE_TEMPL uint32_t IMLAB_BTREE_CLASS::LeafNode::lower_bound(const Key &key) const {
    assert(this->count > 0);
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

IMLAB_BTREE_TEMPL const typename IMLAB_BTREE_CLASS::LeafNode::next_ptr &IMLAB_BTREE_CLASS::LeafNode::get_next() const {
    return next;
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

IMLAB_BTREE_TEMPL Key IMLAB_BTREE_CLASS::LeafNode::split(LeafNode &other, uint64_t other_page) {
    assert(this->count > 1);
    uint32_t start = this->count - (this->count / 2);

    for (uint32_t i = start; i < this->count; ++i) {
        other.keys[i - start] = keys[i];
        other.values[i - start] = values[i];
    }

    other.count = this->count - start;
    this->count = start;

    other.next = this->next;
    this->next = other_page;

    return other.keys[0];  // NOTE maybe use inbetween key
}
// ---------------------------------------------------------------------------------------------------
IMLAB_BTREE_TEMPL typename IMLAB_BTREE_CLASS::iterator IMLAB_BTREE_CLASS::begin() {
    if (!root)
        return end();

    auto fix = this->fix_exclusive(*root);
    while (!fix.template as<Node>()->is_leaf()) {
        assert(fix.template as<Node>()->count > 0);
        fix = this->fix_exclusive(fix.template as<InnerNode>()->begin());
    }
    assert(fix.template as<Node>()->count > 0);

    return iterator(*this, std::move(fix), 0);
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
    assert(fix.template as<Node>()->count > 0);

    auto &leaf = *fix.template as<LeafNode>();
    auto i = leaf.lower_bound(key);

    return leaf.is_equal(key, i) ? iterator(*this, std::move(fix), i) : end();
}

IMLAB_BTREE_TEMPL void IMLAB_BTREE_CLASS::insert(const Key &key, const T &value) {
    auto ir = insert_internal(key);
    if (ir)
        ir->second = value;
}

IMLAB_BTREE_TEMPL void IMLAB_BTREE_CLASS::insert(const Key &key, T &&value) {
    auto ir = insert_internal(key);
    if (ir)
        ir->second = value;
}

IMLAB_BTREE_TEMPL void IMLAB_BTREE_CLASS::insert_or_assign(const Key &key, const T &value) {
    insert_or_assign_internal(key).second = value;
}

IMLAB_BTREE_TEMPL void IMLAB_BTREE_CLASS::insert_or_assign(const Key &key, T &&value) {
    insert_or_assign_internal(key).second = value;
}

IMLAB_BTREE_TEMPL void IMLAB_BTREE_CLASS::erase(const Key &key) {
    auto ir = exclusive_find_node(key);
    auto &leaf = *ir.fix.template as<LeafNode>();
    auto idx = leaf.lower_bound(key);
    if (leaf.is_equal(key, idx)) {
        leaf.erase(idx);
        ir.fix.set_dirty();
    }

    // TODO join ?
}

IMLAB_BTREE_TEMPL typename IMLAB_BTREE_CLASS::Fix IMLAB_BTREE_CLASS::root_fix() {
    if (root)
        return this->fix(root);
    return {};
}

IMLAB_BTREE_TEMPL typename IMLAB_BTREE_CLASS::ExclusiveFix IMLAB_BTREE_CLASS::root_fix_exclusive() {
    if (root)
        return this->fix_exclusive(*root);

    root = next_page_id;
    return new_leaf();
}

IMLAB_BTREE_TEMPL typename IMLAB_BTREE_CLASS::ExclusiveFix IMLAB_BTREE_CLASS::new_leaf() {
    auto fix = this->fix_exclusive(next_page_id++);
    new (fix.data()) LeafNode();
    fix.set_dirty();
    ++leaf_count;

    return fix;
}

IMLAB_BTREE_TEMPL typename IMLAB_BTREE_CLASS::ExclusiveFix IMLAB_BTREE_CLASS::new_inner(uint16_t level) {
    auto fix = this->fix_exclusive(next_page_id++);
    new (fix.data()) InnerNode(level);
    fix.set_dirty();

    return fix;
}

IMLAB_BTREE_TEMPL std::optional<typename IMLAB_BTREE_CLASS::InsertResult> IMLAB_BTREE_CLASS::insert_internal(const Key &key) {
    // TODO early split when really full
    auto ir = exclusive_find_node(key);
    auto *leaf = ir.fix.template as<LeafNode>();

    if (leaf->count == 0) {
        ++count;
        ir.fix.set_dirty();
        return {{std::move(ir.fix), leaf->make_space(key, 0)}};
    }

    // check for key
    auto idx = leaf->lower_bound(key);
    if (leaf->is_equal(key, idx))
        return {};

    // check for space, split if unavailable
    if (leaf->count >= LeafNode::kCapacity) {
        ir = {};
        ir = insert_full_lock_rec_split(key);
        leaf = ir.fix.template as<LeafNode>();
        idx = leaf->lower_bound(key);
    }

    assert(leaf->count < LeafNode::kCapacity);
    ++count;
    ir.fix.set_dirty();
    return {{std::move(ir.fix), leaf->make_space(key, idx)}};
}

IMLAB_BTREE_TEMPL typename IMLAB_BTREE_CLASS::InsertResult IMLAB_BTREE_CLASS::insert_or_assign_internal(const Key &key) {
    // TODO try no split if really empty
    auto ir = insert_lc_early_split(key);

    auto &leaf = *ir.fix.template as<LeafNode>();

    if (leaf.count == 0) {
        ++count;
        ir.fix.set_dirty();
        return {std::move(ir.fix), leaf.make_space(key, 0)};
    }

    auto idx = leaf.lower_bound(key);
    ir.fix.set_dirty();
    if (leaf.is_equal(key, idx)) {
        return {std::move(ir.fix), leaf.at(idx)};
    } else {
        ++count;
        return {std::move(ir.fix), leaf.make_space(key, idx)};
    }
}

IMLAB_BTREE_TEMPL void IMLAB_BTREE_CLASS::split(ExclusiveFix &parent, ExclusiveFix &child, const Key &key) {
    if (!parent.data()) {
        root = next_page_id;
        parent = new_inner(child.template as<Node>()->level + 1);
    }
    assert(!parent.template as<Node>()->is_leaf());

    auto &pnode = *parent.template as<InnerNode>();

    Key split_key;
    ExclusiveFix split;
    uint64_t split_page = next_page_id;
    if (child.template as<Node>()->is_leaf()) {
        auto &cnode = *child.template as<LeafNode>();
        split = new_leaf();
        split_key = cnode.split(*split.template as<LeafNode>(), split_page);
    } else {
        auto &cnode = *child.template as<InnerNode>();
        split = new_inner(cnode.level);
        split_key = cnode.split(*split.template as<InnerNode>());
    }

    parent.set_dirty();
    child.set_dirty();
    split.set_dirty();

    if (pnode.count == 0)
        pnode.init(this->page_id(child));
    pnode.insert(split_key, split_page);
    if (!comp(key, split_key))
        child = std::move(split);
}

IMLAB_BTREE_TEMPL typename IMLAB_BTREE_CLASS::CoupledFixes IMLAB_BTREE_CLASS::insert_lc_early_split(const Key &key) {
    CoupledFixes cf = { root_fix_exclusive() };

    while (!cf.fix.template as<Node>()->is_leaf()) {
        auto &inner = *cf.fix.template as<InnerNode>();
        if (inner.full())
            split(cf.prev, cf.fix, key);

        cf.advance(this->fix_exclusive(cf.fix.template as<InnerNode>()->lower_bound(key)));
    }

    if (cf.fix.template as<LeafNode>()->full())
        split(cf.prev, cf.fix, key);

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
    auto split_it = [this, &fixes, &key, &it] () {
        ExclusiveFix dummy = {};
        it + 1 == fixes.rend() ? split(dummy, *it, key) : split(*(it + 1), *it, key);
    };

    if (it->template as<LeafNode>()->full()) {
        split_it();
        for (++it; it != fixes.rend(); ++it) {
            if (!it->template as<InnerNode>()->full())
                break;
            split_it();
        }
    }

    if (fixes.size() > 1)
        return { std::move(fixes.back()), std::move(*++fixes.rbegin()) };
    return { std::move(fixes.back()) };
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
    if (++i >= leaf.count) {
        if (leaf.get_next())
            fix = segment.fix_exclusive(*leaf.get_next());
        else
            fix.unfix();

        i = 0;
    }

    return *this;
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
