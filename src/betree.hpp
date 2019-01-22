// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#ifndef SRC_BETREE_HPP_
#define SRC_BETREE_HPP_

#include <algorithm>
#include <utility>
#include <vector>
#include "imlab/betree.h"
// ---------------------------------------------------------------------------------------------------
namespace imlab {
IMLAB_BETREE_TEMPL constexpr IMLAB_BETREE_CLASS::Node::Node(uint16_t level)
    : level(level) {}

IMLAB_BETREE_TEMPL bool IMLAB_BETREE_CLASS::Node::is_leaf() const {
    return level == 0;
}
// ---------------------------------------------------------------------------------------------------
IMLAB_BETREE_TEMPL constexpr IMLAB_BETREE_CLASS::InnerNode::InnerNode(uint16_t level)
    : Node(level) {
    static_assert(sizeof(*this) <= page_size);
    assert(level > 0);
}

IMLAB_BETREE_TEMPL uint64_t IMLAB_BETREE_CLASS::InnerNode::begin() const {
    return children[0];
}

IMLAB_BETREE_TEMPL uint64_t IMLAB_BETREE_CLASS::InnerNode::lower_bound(const Key &key) const {
    assert(this->count > 0);
    if (comp(keys[this->count - 1], key))
        return children[this->count];
    return children[std::lower_bound(keys, keys + this->count, key, comp) - keys];
}

IMLAB_BETREE_TEMPL uint64_t IMLAB_BETREE_CLASS::InnerNode::upper_bound(const Key &key) const {
    assert(this->count > 0);
    if (!comp(key, keys[this->count - 1]))
        return children[this->count];
    return children[std::upper_bound(keys, keys + this->count, key, comp) - keys];
}

IMLAB_BETREE_TEMPL uint64_t IMLAB_BETREE_CLASS::InnerNode::at(uint32_t idx) const {
    return children[idx];
}

IMLAB_BETREE_TEMPL const Key &IMLAB_BETREE_CLASS::InnerNode::key(uint32_t idx) const {
    return keys[idx];
}

IMLAB_BETREE_TEMPL bool IMLAB_BETREE_CLASS::InnerNode::message_insert(const MessageKey &key, const T &value) {
    if (!msgs.template insert<Insert>(key, value))
        return false;

    if (comp(key.key, keys[0]))
        keys[0] = key.key;
    return true;
}

IMLAB_BETREE_TEMPL bool IMLAB_BETREE_CLASS::InnerNode::message_insert_or_assign(const MessageKey &key, const T &value) {
    if (!msgs.template insert<InsertOrAssign>(key, value))
        return false;

    if (comp(key.key, keys[0]))
        keys[0] = key.key;
    return true;
}

IMLAB_BETREE_TEMPL bool IMLAB_BETREE_CLASS::InnerNode::message_upsert(const MessageKey &key, upsert_t value) {
    if (!msgs.template insert<Upsert>(key, value))
        return false;

    if (comp(key.key, keys[0]))
        keys[0] = key.key;
    return true;
}

IMLAB_BETREE_TEMPL bool IMLAB_BETREE_CLASS::InnerNode::message_erase(const MessageKey &key) {
    if (!msgs.template insert<Erase>(key))
        return false;

    if (comp(key.key, keys[0]))
        keys[0] = key.key;
    return true;
}

IMLAB_BETREE_TEMPL const typename IMLAB_BETREE_CLASS::MessageMap &IMLAB_BETREE_CLASS::InnerNode::messages() const {
    return msgs;
}

IMLAB_BETREE_TEMPL void IMLAB_BETREE_CLASS::InnerNode::erase_map(typename MessageMap::const_iterator it) {
    msgs.erase(it);
}

IMLAB_BETREE_TEMPL bool IMLAB_BETREE_CLASS::InnerNode::full() const {
    return this->count >= kCapacity;
}

IMLAB_BETREE_TEMPL void IMLAB_BETREE_CLASS::InnerNode::init(uint64_t left) {
    children[0] = left;
}

IMLAB_BETREE_TEMPL void IMLAB_BETREE_CLASS::InnerNode::insert(const Key &key, uint64_t split_page) {
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

IMLAB_BETREE_TEMPL Key IMLAB_BETREE_CLASS::InnerNode::split(InnerNode &other) {
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
    this->count = start;

    // split messages
    for (auto it = msgs.upper_bound(MessageKey::min(keys[start])); it != msgs.end();) {
        switch (it->type()) {
            case Insert:
                other.message_insert(it->key(), it->template as<Insert>());
                break;
            case InsertOrAssign:
                other.msgs.template insert<InsertOrAssign>(it->key(), it->template as<InsertOrAssign>());
                break;
            case Upsert:
                other.msgs.template insert<Upsert>(it->key(), it->template as<Upsert>());
                break;
            case Erase:
                other.msgs.template insert<Erase>(it->key());
                break;
            default:
                assert(false);
        }

        auto eit = it;
        ++it;
        msgs.erase(eit);
    }

    return keys[start];
}
// ---------------------------------------------------------------------------------------------------
IMLAB_BETREE_TEMPL constexpr IMLAB_BETREE_CLASS::LeafNode::LeafNode()
    : Node(0) {}

IMLAB_BETREE_TEMPL uint32_t IMLAB_BETREE_CLASS::LeafNode::lower_bound(const Key &key) const {
    assert(this->count > 0);
    if (comp(keys[this->count - 1], key))
        return this->count;
    return std::lower_bound(keys, keys + this->count, key, comp) - keys;
}

IMLAB_BETREE_TEMPL uint32_t IMLAB_BETREE_CLASS::LeafNode::upper_bound(const Key &key) const {
    assert(this->count > 0);
    if (!comp(key, keys[this->count - 1]))
        return this->count;
    return std::upper_bound(keys, keys + this->count, key, comp) - keys;
}

IMLAB_BETREE_TEMPL const T &IMLAB_BETREE_CLASS::LeafNode::at(uint32_t idx) const {
    assert(idx < this->count);
    return values[idx];
}

IMLAB_BETREE_TEMPL T &IMLAB_BETREE_CLASS::LeafNode::at(uint32_t idx) {
    assert(idx < this->count);
    return values[idx];
}

IMLAB_BETREE_TEMPL bool IMLAB_BETREE_CLASS::LeafNode::is_equal(const Key &key, uint32_t idx) const {
    return idx < this->count && keys[idx] == key;
}

IMLAB_BETREE_TEMPL bool IMLAB_BETREE_CLASS::LeafNode::full() const {
    return this->count >= kCapacity;
}

IMLAB_BETREE_TEMPL T &IMLAB_BETREE_CLASS::LeafNode::make_space(const Key &key, uint32_t idx) {
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

IMLAB_BETREE_TEMPL void IMLAB_BETREE_CLASS::LeafNode::erase(uint32_t idx) {
    assert(idx < this->count);

    // move to the left
    for (; idx < this->count - 1; ++idx) {
        keys[idx] = keys[idx + 1];
        values[idx] = values[idx + 1];
    }

    // last element can stay in memory
    --this->count;
}

IMLAB_BETREE_TEMPL Key IMLAB_BETREE_CLASS::LeafNode::split(LeafNode &other, uint64_t other_page) {
    assert(this->count > 1);
    uint32_t start = this->count - (this->count / 2);

    for (uint32_t i = start; i < this->count; ++i) {
        other.keys[i - start] = keys[i];
        other.values[i - start] = values[i];
    }

    other.count = this->count - start;
    this->count = start;

    return keys[this->count - 1];  // NOTE maybe use inbetween key
}
// ---------------------------------------------------------o------------------------------------------
IMLAB_BETREE_TEMPL void IMLAB_BETREE_CLASS::insert(const Key &key, const T &value) {
    auto root = root_fix_exclusive();

    if (root.template as<Node>()->is_leaf()) {
        auto &leaf = *root.template as<LeafNode>();

        if (leaf.count == 0) {
            ++count;
            leaf.make_space(key, 0) = value;
            root.set_dirty();
            return;
        } else if (!leaf.full()) {
            auto idx = leaf.lower_bound(key);
            if (leaf.is_equal(key, idx))
                return;

            ++count;
            leaf.make_space(key, idx) = value;
            root.set_dirty();
            return;
        }

        ExclusiveFix old_root = std::move(root);
        split(root, old_root, key);
    }

    if (!root.template as<InnerNode>()->message_insert({key, next_timestamp}, value)) {
        root = flush(std::move(root));
        if (!root.template as<InnerNode>()->message_insert({key, next_timestamp}, value))
            assert(false);
    }
    ++next_timestamp;
    ++pending;
    root.set_dirty();
}

IMLAB_BETREE_TEMPL void IMLAB_BETREE_CLASS::erase(const Key &key) {
    // TODO
}

IMLAB_BETREE_TEMPL uint64_t IMLAB_BETREE_CLASS::size() const {
    return count;
}

IMLAB_BETREE_TEMPL uint64_t IMLAB_BETREE_CLASS::size_pending() const {
    assert(pending >= 0 || count >= -pending);
    return count + pending;
}

IMLAB_BETREE_TEMPL uint64_t IMLAB_BETREE_CLASS::capacity() const {
    return leaf_count * LeafNode::kCapacity;
}

IMLAB_BETREE_TEMPL typename IMLAB_BETREE_CLASS::ExclusiveFix IMLAB_BETREE_CLASS::root_fix_exclusive() {
    if (root)
        return this->fix_exclusive(*root);

    root = next_page_id;
    return new_leaf();
}

IMLAB_BETREE_TEMPL typename IMLAB_BETREE_CLASS::ExclusiveFix IMLAB_BETREE_CLASS::new_leaf() {
    auto fix = this->fix_exclusive(next_page_id++);
    new (fix.data()) LeafNode();
    fix.set_dirty();
    ++leaf_count;

    return fix;
}

IMLAB_BETREE_TEMPL typename IMLAB_BETREE_CLASS::ExclusiveFix IMLAB_BETREE_CLASS::new_inner(uint16_t level) {
    auto fix = this->fix_exclusive(next_page_id++);
    new (fix.data()) InnerNode(level);
    fix.set_dirty();

    return fix;
}

IMLAB_BETREE_TEMPL void IMLAB_BETREE_CLASS::split(ExclusiveFix &parent, ExclusiveFix &child, const Key &key) {
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

// TODO
IMLAB_BETREE_TEMPL typename IMLAB_BETREE_CLASS::ExclusiveFix
IMLAB_BETREE_CLASS::flush(ExclusiveFix root) {
    std::vector<ExclusiveFix> fixes;
    fixes.push_back(std::move(root));

    bool can_flush = false;
    uint32_t prev_flush_amount = 1;
    std::vector<std::pair<typename BeTree::MessageKey, typename BeTree::MessageKey>> flushes;

    auto get_rb_iters = [](InnerNode &inner, auto i) {
        return std::make_pair(
            i > 0 ? inner.messages().lower_bound(MessageKey::min(inner.key(i - 1))) : inner.messages().begin(),
            i < inner.count ? inner.messages().lower_bound(MessageKey::min(inner.key(i))) : inner.messages().end());
    };

    // get a vector of required flushes
    while (!can_flush) {
        auto &inner = *fixes.back().template as<InnerNode>();

        // find flush candidate
        uint32_t candidate;
        uint32_t max_flush_amount = 0;
        for (uint32_t i = 0; i <= inner.count; ++i) {
            auto iters = get_rb_iters(inner, i);
            uint32_t flush_amount = 0;
            while (iters.first != iters.second)
                ++iters.first, ++flush_amount;

            if (flush_amount > max_flush_amount || (flush_amount == max_flush_amount && this->is_dirty(inner.at(i)))) {
                candidate = i;
                max_flush_amount = flush_amount;
            }
        }
        assert(max_flush_amount > 0);
        auto iters = get_rb_iters(inner, candidate);
        flushes.emplace_back(iters.first->key(), iters.second != inner.messages().end() ? iters.second->key() : inner.messages().back().key());

        // check flushability
        auto new_fix = this->fix_exclusive(inner.at(candidate));
        if (new_fix.template as<Node>()->is_leaf()) {
            can_flush = true;
        } else {
            can_flush = new_fix.template as<InnerNode>()->messages().capacity() >= prev_flush_amount;
            prev_flush_amount = max_flush_amount;
        }

        // flush target should not be in vector
        if (!can_flush)
            fixes.push_back(std::move(new_fix));
    }

    // do the flushes
    auto flit = flushes.rbegin();
    for (auto it = fixes.rbegin(); it != fixes.rend(); ++flit) {  // it increment at the end
        auto *source = it->template as<InnerNode>();

        ExclusiveFix target;
        auto fl_begin = source->messages().lower_bound(flit->first);
        auto fl_end = source->messages().lower_bound(flit->second);
        while (fl_begin != fl_end) {  // flit->first increment at the end
            const Key &key = fl_begin->key().key;

            uint64_t current_target = source->lower_bound(key);
            if (!target.data() || this->page_id(target) != current_target)
                target = this->fix_exclusive(current_target);

            if (target.template as<Node>()->is_leaf()) {
                auto *leaf = target.template as<LeafNode>();
                uint32_t idx = leaf->lower_bound(key);

                auto rec_split = [&]() {
                    assert(fixes.size() > 0);

                    size_t it;
                    for (it = fixes.size() - 1; it > 0; --it) {
                        if (!fixes[it].template as<InnerNode>()->full())
                            break;
                    }

                    if (it == 0 && fixes.front().template as<InnerNode>()->full()) {
                        ExclusiveFix dummy;
                        split(dummy, fixes.front(), key);
                    }
                    for (; it + 1 < fixes.size(); ++it)
                        split(fixes[it], fixes[it + 1], key);
                    split(fixes.back(), target, key);

                    source = fixes.back().template as<InnerNode>();
                    leaf = target.template as<LeafNode>();
                    idx = leaf->lower_bound(key);

                    fl_begin = source->messages().lower_bound(flit->first);
                    fl_end = source->messages().lower_bound(flit->second);
                };

                switch (fl_begin->type()) {
                    case Insert:
                        if (!leaf->is_equal(key, idx)) {
                            if (leaf->full())
                                rec_split();

                            leaf->make_space(key, idx) = fl_begin->template as<Insert>();
                            it->set_dirty();
                            ++count;
                        }
                        --pending;
                        break;
                    case InsertOrAssign:
                        if (leaf->is_equal(key, idx)) {
                            if (leaf->full())
                                rec_split();

                            leaf->at(idx) = fl_begin->template as<InsertOrAssign>();
                            it->set_dirty();
                        } else {
                            if (leaf->full())
                                rec_split();

                            leaf->make_space(key, idx) = fl_begin->template as<InsertOrAssign>();
                            it->set_dirty();
                            ++count;
                        }
                        --pending;
                        break;
                    case Upsert:
                        if (leaf->is_equal(key, idx)) {
                            leaf->at(idx) = fl_begin->template as<Upsert>()(std::move(leaf->at(idx)));
                            it->set_dirty();
                        }
                        break;
                    case Erase:
                        if (leaf->is_equal(key, idx)) {
                            leaf->erase(idx);
                            it->set_dirty();
                            --count;
                        }
                        ++pending;
                        break;
                    default:
                        assert(false);
                }
            } else {  // target is inner node
                auto &inner = *target.template as<InnerNode>();
                switch (fl_begin->type()) {
                    case Insert:
                        inner.message_insert(fl_begin->key(), fl_begin->template as<Insert>());
                        break;
                    case InsertOrAssign:
                        inner.message_insert_or_assign(fl_begin->key(), fl_begin->template as<InsertOrAssign>());
                        break;
                    case Upsert:
                        inner.message_upsert(fl_begin->key(), fl_begin->template as<Upsert>());
                        break;
                    case Erase:
                        inner.message_erase(fl_begin->key());
                        break;
                    default:
                        assert(false);
                }
            }

            auto eit = fl_begin;
            ++fl_begin;
            source->erase_map(eit);
        }
        it->set_dirty();
        auto eit = it.base() - 1;
        ++it;
        if (fixes.size() > 1)
            fixes.erase(eit);
    }

    return std::move(fixes.front());
}
// ---------------------------------------------------------------------------------------------------
}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#endif  // SRC_BETREE_HPP_
