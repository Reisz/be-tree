// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#ifndef SRC_BETREE_HPP_
#define SRC_BETREE_HPP_

#include "imlab/betree.h"

#include <algorithm>
#include <cassert>
#include <map>
#include <utility>
#include <vector>
// ---------------------------------------------------------------------------------------------------
// #define DEBUG_FLUSH
#ifdef DEBUG_FLUSH
    #include <iostream>
    #define DEBUG(x) do { std::cerr << x; } while (0)
#else
    #define DEBUG(x) do {} while (0)
#endif
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
    assert(idx <= this->count);
    return children[idx];
}

IMLAB_BETREE_TEMPL const Key &IMLAB_BETREE_CLASS::InnerNode::key(uint32_t idx) const {
    assert(idx < this->count);
    return keys[idx];
}

IMLAB_BETREE_TEMPL bool IMLAB_BETREE_CLASS::InnerNode::message_insert(const MessageKey &key, const T &value) {
    if (!msgs.template insert<Insert>(key, value))
        return false;

    return true;
}

IMLAB_BETREE_TEMPL bool IMLAB_BETREE_CLASS::InnerNode::message_insert_or_assign(const MessageKey &key, const T &value) {
    if (!msgs.template insert<InsertOrAssign>(key, value))
        return false;

    return true;
}

IMLAB_BETREE_TEMPL bool IMLAB_BETREE_CLASS::InnerNode::message_upsert(const MessageKey &key, upsert_t value) {
    if (!msgs.template insert<Upsert>(key, value))
        return false;

    return true;
}

IMLAB_BETREE_TEMPL bool IMLAB_BETREE_CLASS::InnerNode::message_erase(const MessageKey &key) {
    if (!msgs.template insert<Erase>(key))
        return false;

    return true;
}

IMLAB_BETREE_TEMPL bool IMLAB_BETREE_CLASS::InnerNode::apply(typename MessageMap::const_iterator it) {
    switch (it->type()) {
        case Insert:
            return message_insert(it->key(), it->template as<Insert>());
        case InsertOrAssign:
            return message_insert_or_assign(it->key(), it->template as<InsertOrAssign>());
        case Upsert:
            return message_upsert(it->key(), it->template as<Upsert>());
        case Erase:
            return message_erase(it->key());
        default:
            throw;
    }
}

IMLAB_BETREE_TEMPL const typename IMLAB_BETREE_CLASS::MessageMap &IMLAB_BETREE_CLASS::InnerNode::messages() const {
    return msgs;
}

IMLAB_BETREE_TEMPL size_t IMLAB_BETREE_CLASS::InnerNode::map_capacity_bytes() const {
    return msgs.capacity_bytes();
}

IMLAB_BETREE_TEMPL uint32_t IMLAB_BETREE_CLASS::InnerNode::map_start_index() const {
    assert(this->count > 0);
    const auto &key = msgs.begin()->key().key;
    if (comp(keys[this->count - 1], key))
        return this->count;
    return std::lower_bound(keys, keys + this->count, key, comp) - keys;
}

IMLAB_BETREE_TEMPL typename IMLAB_BETREE_CLASS::MessageRange IMLAB_BETREE_CLASS::InnerNode::map_get_range(uint32_t idx) const {
    return std::make_pair(
        idx > 0 ? msgs.upper_bound(MessageKey::max(keys[idx - 1])) : msgs.begin(),
        idx < this->count ? msgs.upper_bound(MessageKey::max(keys[idx])) : msgs.end());
}

IMLAB_BETREE_TEMPL typename IMLAB_BETREE_CLASS::MessageRange IMLAB_BETREE_CLASS::InnerNode::map_get_key_range(const Key &key) const {
    return std::make_pair(msgs.lower_bound(MessageKey::min(key)), msgs.upper_bound(MessageKey::max(key)));
}

IMLAB_BETREE_TEMPL void IMLAB_BETREE_CLASS::InnerNode::map_erase(typename MessageMap::const_iterator it) {
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
    DEBUG("Splitting inner node level " << this->level << std::endl);
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
    for (auto it = msgs.upper_bound(MessageKey::max(keys[start])); it != msgs.end();) {
        other.apply(it);
        msgs.erase(it++);
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

IMLAB_BETREE_TEMPL const Key &IMLAB_BETREE_CLASS::LeafNode::key(uint32_t idx) const {
    assert(idx < this->count);
    return keys[idx];
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

IMLAB_BETREE_TEMPL Key IMLAB_BETREE_CLASS::LeafNode::split(LeafNode &other) {
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
IMLAB_BETREE_TEMPL typename IMLAB_BETREE_CLASS::const_iterator IMLAB_BETREE_CLASS::end() const {
    return const_iterator(*this, {}, {}, 0);
}

IMLAB_BETREE_TEMPL typename IMLAB_BETREE_CLASS::const_iterator IMLAB_BETREE_CLASS::find(const Key &key) const {
    if (!root)
        return end();

    std::vector<Fix> fixes;
    fixes.push_back(this->fix(*root));

    typename MessageMap::const_iterator earliest_insert;
    uint32_t earliest_insert_stack_size = 0;
    while (!fixes.back().template as<Node>()->is_leaf()) {
        assert(fixes.back().template as<Node>()->count > 0);
        auto &inner = *fixes.back().template as<InnerNode>();

        for (auto iters = inner.map_get_key_range(key); iters.first != iters.second; ++iters.first) {
            switch (iters.first->type()) {
                case Insert:
                    earliest_insert = iters.first;
                    earliest_insert_stack_size = fixes.size();
                    break;
                case InsertOrAssign:
                    return const_iterator(*this, std::move(fixes), iters.first, 0);
                case Erase:
                    return end();
                case Upsert:
                    break;  // TODO
                default:
                    throw;
            }
        }

        fixes.push_back(this->fix(inner.lower_bound(key)));
    }
    assert(fixes.back().template as<Node>()->count > 0);

    auto &leaf = *fixes.back().template as<LeafNode>();
    auto i = leaf.lower_bound(key);

    if (leaf.is_equal(key, i))
        return const_iterator(*this, std::move(fixes), {}, i);

    if (earliest_insert_stack_size) {
        while (fixes.size() > earliest_insert_stack_size)
            fixes.pop_back();
        return const_iterator(*this, std::move(fixes), earliest_insert, 0);
    }

    return end();
}

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
        flush(root, MessageMap::template size_bytes<Insert>());
        assert(this->page_id(root) == *this->root);
        if (!root.template as<InnerNode>()->message_insert({key, next_timestamp}, value))
            throw;
    }
    DEBUG("Inserted " << key << std::endl);
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

IMLAB_BETREE_TEMPL uint16_t IMLAB_BETREE_CLASS::depth() const {
    if (root)
        return this->fix(*root).template as<Node>()->level;
    return 0;
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
        split_key = cnode.split(*split.template as<LeafNode>());
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

IMLAB_BETREE_TEMPL void IMLAB_BETREE_CLASS::flush(ExclusiveFix &root, size_t min_amount) {
    DEBUG("Flushing root level for " << min_amount << " bytes" << std::endl);

    struct FlushRequest { ExclusiveFix fix; uint32_t index; size_t bytes; };
    std::vector<FlushRequest> flushes; {
        auto flush = find_flush(root);
        flushes.push_back({std::move(root), flush.first, flush.second});
    }

    // === Begin Leaf Storage ===
    struct LeafStorage { ExclusiveFix fix; bool in_tree; };
    ExclusiveFix left_leaf;
    std::map<Key, LeafStorage, Compare> leaves;
    auto insert_existing_leaf = [&](auto &target) {
        const Key *leaf_key = nullptr;
        for (auto it = flushes.rbegin(); it != flushes.rend(); ++it) {
            if (it->index > 0) {
                leaf_key = &it->fix.template as<InnerNode>()->key(it->index - 1);
                break;
            }
        }

        if (leaf_key)
            leaves.insert(std::make_pair(*leaf_key, LeafStorage {std::move(target), true}));
        else
            left_leaf = std::move(target);
    };
    auto find_target_leaf = [&](const Key &key) -> auto * {
        auto it = leaves.lower_bound(key);
        if (it == leaves.end()) {
            if (leaves.size())
                return &leaves.rbegin()->second.fix;

            assert(left_leaf.data());
            return &left_leaf;
        }
        if (it == leaves.begin()) {
            assert(left_leaf.data());
            return &left_leaf;
        }
        return &(--it)->second.fix;
    };
    // === End Leaf Storage ===

    while (flushes.front().fix.template as<InnerNode>()->map_capacity_bytes() < min_amount) {
        auto &source = *flushes.back().fix.template as<InnerNode>();
        auto target = this->fix_exclusive(source.at(flushes.back().index));

        // 3 options:
        // - Node
        //   - Not enough space -> find next flush, add to vector
        //   - Enough space -> perform flush, pop vector
        // - Leaf -> perform flush, pop vector
        if (!target.template as<Node>()->is_leaf()) {
            auto &inner = *target.template as<InnerNode>();

            if (inner.map_capacity_bytes() < flushes.back().bytes) {
                auto flush = find_flush(target);

                // find_flush could have already flushed to dirty children
                if (inner.map_capacity_bytes() < flushes.back().bytes) {
                    assert(flush.second > 0);
                    flushes.push_back({std::move(target), flush.first, flush.second});
                    continue;
                }
                DEBUG("\tEarly flush was sufficient" << std::endl);
            } else {
                DEBUG("\tFlushing to inner " << flushes.back().index << std::endl);
                for (auto iters = source.map_get_range(flushes.back().index); iters.first != iters.second; source.map_erase(iters.first++)) {
                    DEBUG("\t\tFlushing " << iters.first->key().key << std::endl);
                    assert(flushes.back().index > 0 ? !comp(iters.first->key().key, source.key(flushes.back().index - 1))
                        : comp(iters.first->key().key, source.key(0)));
                    if (!inner.apply(iters.first))
                        throw;
                }
                target.set_dirty();
            }
        } else {
            insert_existing_leaf(target);

            for (auto iters = source.map_get_range(flushes.back().index); iters.first != iters.second; source.map_erase(iters.first++)) {
                const Key &key = iters.first->key().key;

                // === Begin Split Management ===
                ExclusiveFix *leaf_fix;
                LeafNode *leaf;
                uint32_t idx;
                auto determine_leaf = [&]() {
                    leaf_fix = find_target_leaf(key);
                    leaf = leaf_fix->template as<LeafNode>();
                    idx = leaf->lower_bound(key);
                };
                auto split_leaf = [&]() {
                    auto new_leaf_fix = new_leaf();
                    const auto &insert_key = leaf->split(*new_leaf_fix.template as<LeafNode>());
                    leaves.insert(std::make_pair(insert_key, LeafStorage {std::move(new_leaf_fix), false}));
                    leaf_fix->set_dirty();
                    determine_leaf();
                };
                // === End Split Management ===

                determine_leaf();
                switch (iters.first->type()) {
                    case Insert:
                        if (!leaf->is_equal(key, idx)) {
                            if (leaf->full())
                                split_leaf();

                            leaf->make_space(key, idx) = iters.first->template as<Insert>();
                            leaf_fix->set_dirty();
                            ++count;
                        }
                        --pending;
                        break;
                    case InsertOrAssign:
                        if (leaf->is_equal(key, idx)) {
                            if (leaf->full())
                                split_leaf();

                            leaf->at(idx) = iters.first->template as<InsertOrAssign>();
                            leaf_fix->set_dirty();
                        } else {
                            if (leaf->full())
                                split_leaf();

                            leaf->make_space(key, idx) = iters.first->template as<InsertOrAssign>();
                            leaf_fix->set_dirty();
                            ++count;
                        }
                        --pending;
                        break;
                    case Upsert:
                        if (leaf->is_equal(key, idx)) {
                            leaf->at(idx) = iters.first->template as<Upsert>()(std::move(leaf->at(idx)));
                            leaf_fix->set_dirty();
                        }
                        break;
                    case Erase:
                        if (leaf->is_equal(key, idx)) {
                            leaf->erase(idx);
                            leaf_fix->set_dirty();
                            --count;
                        }
                        ++pending;
                        break;
                    default:
                        throw;
                }
            }
        }
        flushes.back().fix.set_dirty();

        // don't pop root
        if (flushes.size() > 1)
            flushes.pop_back();
    }

    root = std::move(flushes.front().fix);
    for (auto &leaf_fix : leaves) {
        if (!leaf_fix.second.in_tree)
            insert_leaf(root, leaf_fix.first, this->page_id(leaf_fix.second.fix));
    }
}

IMLAB_BETREE_TEMPL std::pair<uint32_t, size_t> IMLAB_BETREE_CLASS::find_flush(ExclusiveFix &fix) {
    DEBUG("\tFinding flush" << std::endl);

    auto &inner = *fix.template as<InnerNode>();

    uint32_t result;
    size_t max_flush_amount_bytes = 0;

    assert(inner.messages().begin() != inner.messages().end());
    for (uint32_t i = inner.map_start_index(); i <= inner.count; ++i) {
        DEBUG("\t\tSearching index " << i << std::endl);
        if (i > 0)
            DEBUG("\t\tLeft key: " << inner.key(i - 1) << std::endl);
        if (i < inner.count)
            DEBUG("\t\tRight key: " << inner.key(i) << std::endl);

        auto iters = inner.map_get_range(i);
        uint32_t flush_amount_bytes = 0;

        if (iters.first == inner.messages().end())
            break;

        // try to immediately flush
        if (inner.level > 1 && this->is_dirty(inner.at(i))) {
            DEBUG("\t\t\tChild is dirty, trying to flush." << std::endl);
            auto child_fix = this->fix_exclusive(inner.at(i));
            auto &child = *child_fix.template as<InnerNode>();

            while (iters.first != iters.second) {
                assert(i > 0 ? !comp(iters.first->key().key, inner.key(i - 1)) : comp(iters.first->key().key, inner.key(0)));
                if (!child.apply(iters.first))
                    break;
                DEBUG("\t\t\t\tFlushed " << iters.first->key().key << std::endl);
                inner.map_erase(iters.first++);
                fix.set_dirty();
            }

            DEBUG(std::endl);
        }

        // count remaining message bytes
        for (; iters.first != iters.second; ++iters.first) {
            DEBUG("\t\t\t\tCounting " << iters.first->key().key << std::endl);
            flush_amount_bytes += iters.first->size_bytes();
        }

        if (flush_amount_bytes > max_flush_amount_bytes) {
            max_flush_amount_bytes = flush_amount_bytes;
            result = i;
        }
    }

    DEBUG("\t\tSelected flush: " << result << std::endl);
    return std::make_pair(result, max_flush_amount_bytes);
}

IMLAB_BETREE_TEMPL void IMLAB_BETREE_CLASS::insert_leaf(ExclusiveFix &root, const Key &key, uint64_t page_id) {
    ExclusiveFix new_root;
    std::vector<ExclusiveFix> fixes;
    fixes.push_back(std::move(root));

    while (fixes.back().template as<Node>()->level > 1)
        fixes.push_back(this->fix_exclusive(fixes.back().template as<InnerNode>()->lower_bound(key)));

    auto it = fixes.rbegin();
    auto split_it = [&] () {
        if (it + 1 == fixes.rend()) {
            split(new_root, *it, key);
        } else {
            split(*(it + 1), *it, key);
        }
    };

    if (it->template as<InnerNode>()->full()) {
        for (++it; it != fixes.rend(); ++it) {
            if (!it->template as<InnerNode>()->full())
                break;
        }

        for (--it; it != fixes.rbegin(); --it)
            split_it();
        split_it();
    }

    assert(fixes.back().template as<Node>()->level == 1 && !fixes.back().template as<InnerNode>()->full());
    fixes.back().template as<InnerNode>()->insert(key, page_id);
    fixes.back().set_dirty();

    if (new_root.data())
        root = std::move(new_root);
    else
        root = std::move(fixes.front());
}
// ---------------------------------------------------------------------------------------------------
IMLAB_BETREE_TEMPL bool IMLAB_BETREE_CLASS::const_iterator::operator==(const const_iterator &other) const {
    if (fixes.size() != other.fixes.size())
        return false;
    if (fixes.size() == 0)
        return true;
    return  fixes.back().data() == other.fixes.back().data() && (it == other.it || idx == other.idx);
}

IMLAB_BETREE_TEMPL bool IMLAB_BETREE_CLASS::const_iterator::operator!=(const const_iterator &other) const {
    return !(*this == other);
}

IMLAB_BETREE_TEMPL typename IMLAB_BETREE_CLASS::const_reference IMLAB_BETREE_CLASS::const_iterator::operator*() {
    if (fixes.back().template as<Node>()->is_leaf()) {
        return fixes.back().template as<LeafNode>()->at(idx);
    } else {
        switch (it->type()) {
            case Insert:
                return it->template as<Insert>();
            case InsertOrAssign:
                return it->template as<InsertOrAssign>();
            default:
                throw;
        }
    }
}

IMLAB_BETREE_TEMPL typename IMLAB_BETREE_CLASS::const_pointer IMLAB_BETREE_CLASS::const_iterator::operator->() {
    return &**this;
}
// ---------------------------------------------------------------------------------------------------
}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#endif  // SRC_BETREE_HPP_
