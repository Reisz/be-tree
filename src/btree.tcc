// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#ifndef SRC_BTREE_TCC_
#define SRC_BTREE_TCC_

#include <vector>
// ---------------------------------------------------------------------------------------------------
namespace imlab {
#define BTREE_TEMPL \
    template<typename Key, typename T, size_t page_size, typename Compare>
#define BTREE_CLASS \
    BTree<Key, T, page_size, Compare>

// ---------------------------------------------------------------------------------------------------
BTREE_TEMPL std::optional<uint32_t> BTREE_CLASS::InnerNode::lower_bound(const Key &key) const {
    // if (Compare(keys[this->count - 1], key))
    //     return {};

    // TODO binary search
    for (uint32_t i = 0; i < this->count; ++i) {
        if (!comp(keys[i], key))
            return i;
    }

    return {};
}

BTREE_TEMPL void BTREE_CLASS::InnerNode::insert(const Key &key, uint64_t split_page) {
    uint32_t i = 0;
    // TODO binary search
    for (; i < this->count; ++i) {
        if (comp(keys[i], key)) {
            break;
        }
    }

    // move rightmost pointer
    children[this->count + 1] = children[this->count];
    // move to the right
    for (uint32_t j = this->count - 1; j >= i; --j) {
        keys[j + 1] = keys[j];
        children[j + 1] = children[j];
    }

    // insert
    keys[i] = key;
    children[i] = split_page;

    ++this->count;
}

BTREE_TEMPL Key BTREE_CLASS::InnerNode::split(InnerNode &other) {
    uint32_t start = this->count - (this->count / 2);

    for (uint32_t i = start; i < this->count; ++i) {
        other.keys[i - start] = keys[i];
        other.children[i - start] = children[i];
    }
    other.children[this->count - start] = children[this->count - 1];

    other.count = this->count - start;
    this->count = start;

    return other.keys[0];
}
// ---------------------------------------------------------------------------------------------------
BTREE_TEMPL std::optional<uint32_t> BTREE_CLASS::LeafNode::find_index(const Key &key) const {
    // TODO binary search
    for (uint32_t i = 0; i < this->count; ++i) {
        if (!comp(keys[i], key) && !comp(key, keys[i]))
            return i;
    }

    return {};
}

BTREE_TEMPL void BTREE_CLASS::LeafNode::insert(const Key &key, const T &value) {
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

BTREE_TEMPL void BTREE_CLASS::LeafNode::erase(const Key &key) {
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

BTREE_TEMPL Key BTREE_CLASS::LeafNode::split(LeafNode &other, uint64_t other_page) {
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
BTREE_TEMPL const T *BTREE_CLASS::find(const Key &key) const {
    if (!root)
        return nullptr;

    uint64_t current = root.value();
    BufferFix prev_fix, fix = manager.fix(segment_page_id(root.value()));
    while (!reinterpret_cast<const Node *>(fix.data())->is_leaf()) {
        const InnerNode &inner = *reinterpret_cast<const InnerNode*>(fix.data());
        auto lb = inner.lower_bound(key);
        current = lb ? inner.children[lb.value()] : inner.children[inner.count];

        prev_fix = std::move(fix);
        fix = manager.fix(segment_page_id(current));
    }

    const LeafNode &leaf = *reinterpret_cast<const LeafNode*>(fix.data());
    auto idx = leaf.find_index(key);

    if (idx)
        return &leaf.values[idx.value()];
    return nullptr;
}

BTREE_TEMPL void BTREE_CLASS::insert(const Key &key, const T &value) {
    if (!root) {
        root = next_page_id++;

        auto root_fix = manager.fix_exclusive(segment_page_id(root.value()));
        LeafNode *leaf = new (root_fix.data()) LeafNode();

        leaf->insert(key, value);

        root_fix.set_dirty();
        ++leaf_count;
    } else {
        if (should_early_split()) {
            leaf_count += insert_lc_early_split(key, value);
        } else if (!try_insert_lc_no_split(key, value)) {
            leaf_count += insert_full_lock_rec_split(key, value);
        }
    }

    ++count;
}

BTREE_TEMPL bool BTREE_CLASS::insert_lc_early_split(const Key &key, const T &value) {
    uint64_t current = root.value();
    BufferFixExclusive prev_fix;

    while (true) {
        auto fix = manager.fix_exclusive(segment_page_id(current));

        if (reinterpret_cast<Node*>(fix.data())->is_leaf()) {
            LeafNode &leaf = *reinterpret_cast<LeafNode*>(fix.data());

            if (leaf.count < LeafNode::kCapacity) {
                leaf.insert(key, value);
                fix.set_dirty();
                return false;
            } else {
                // TODO split
            }
        } else {
            InnerNode &inner = *reinterpret_cast<InnerNode*>(fix.data());

            BufferFixExclusive split;
            if (inner.count == InnerNode::kCapacity) {
                // TODO split

                fix.set_dirty();
                split.set_dirty();
            }
        }
    }
}

BTREE_TEMPL bool BTREE_CLASS::try_insert_lc_no_split(const Key &key, const T &value) {
    uint64_t current = root.value();
    BufferFixExclusive prev_fix;  // keep parent page locked

    // tree traversal loop
    while (true) {
        auto fix = manager.fix_exclusive(segment_page_id(current));

        if (reinterpret_cast<Node*>(fix.data())->is_leaf()) {
            LeafNode &leaf = *reinterpret_cast<LeafNode*>(fix.data());

            if (leaf.count < LeafNode::kCapacity) {
                leaf.insert(key, value);
                fix.set_dirty();
                return true;
            }

            return false;
        } else {  // inner node
            InnerNode &inner = *reinterpret_cast<InnerNode*>(fix.data());
            auto lb = inner.lower_bound(key);

            current = lb ? inner.children[lb.value()] : inner.children[inner.count];

            prev_fix = std::move(fix);
            continue;
        }
    }
}


BTREE_TEMPL bool BTREE_CLASS::insert_full_lock_rec_split(const Key &key, const T &value) {
    return false;  // TODO
}

BTREE_TEMPL void BTREE_CLASS::erase(const Key &key) {
    // TODO
}

}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#endif  // SRC_BTREE_TCC_
