// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#ifndef SRC_BTREE_TCC_
#define SRC_BTREE_TCC_

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
        if (!Compare(keys[i], key))
            return i;
    }

    return {};
}

BTREE_TEMPL void BTREE_CLASS::InnerNode::insert(const Key &key, uint64_t split_page) {
    uint32_t i = 0;
    // TODO binary search
    for (; i < this->count; ++i) {
        if (Compare(keys[i], key)) {
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
        if (!Compare(keys[i], key) && !Compare(key, keys[i]))
            return i;
    }

    return {};
}

BTREE_TEMPL void BTREE_CLASS::LeafNode::insert(const Key &key, const T &value) {
    uint32_t i = 0;
    // TODO binary search
    for (; i < this->count; ++i) {
        if (Compare(keys[i], key)) {
            break;
        }
    }

    // move to the right
    for (uint32_t j = this->count - 1; j >= i; --j) {
        keys[j + 1] = keys[j];
        values[j + 1] = values[j];
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
BTREE_TEMPL std::optional<T&> BTREE_CLASS::find(const Key &key) {
    if (!root)
        return {};

    return {};  // TODO
}

BTREE_TEMPL void BTREE_CLASS::insert(const Key &key, const T &value) {
    if (!root) {
        root = next_page_id++;

        auto root_fix = manager.fix_exclusive(segment_page_id(root.value()));
        new (root_fix.data()) LeafNode();
        root_fix.set_dirty();
    }

    std::vector<uint64_t> path = {root};
    BufferFixExclusive prev_fix;  // keep parent page locked

    // tree traversal loop
    while (true) {
        auto fix = manager.fix_exclusive(segment_page_id(path.back()));

        if (reinterpret_cast<Node*>(fix.data()).is_leaf()) {
            LeafNode &leaf = *reinterpret_cast<LeafNode*>(fix.data());

            if (leaf->count < LeafNode::kCapacity) {
                leaf->insert(key, value);
                fix.set_dirty();
                break;
            } else {  // split node
                Key pivot;

                {   // create new leaf to split into
                    uint64_t new_leaf = next_page_id++;
                    auto other_fix = manager.fix_exclusive(segment_page_id(new_leaf));
                    LeafNode &other = *(new (other_fix.data()) LeafNode());

                    pivot = leaf->split(other);
                    if (Compare(key, pivot)) {  // insert into old node
                        leaf->insert(key, value);
                    } else {  // insert into new node
                        other->insert(key, value);
                    }

                    fix.set_dirty();
                    other_fix.set_dirty();
                }

                uint64_t prev = path.pop_back();  // remove leaf from path
                while (!path.empty()) {  // go back up until
                    // update fixes for going upward
                    fix = prev_fix;
                    if (path.size() > 1)
                        prev_fix = path[path.size() - 2];

                    InnerNode &inner = *reinterpret_cast<InnerNode*>(fix.data());
                    if (innert->count < InnerNode::kCapacity) {
                        inner.insert(pivot, prev);
                        fix.set_dirty();
                        break;
                    } else {  // split node
                        
                    }
                }
            }
        } else {  // inner node
            InnerNode *inner = reinterpret_cast<InnerNode*>(fix.data());
            auto next = inner->lower_bound(key);

            if (next) {
                path.push_back(next);
            } else {  // greater than all pivot keys
                path.push_back(inner->children[inner->count]);
            }

            prev_fix = fix;
            continue;
        }
    }
}

BTREE_TEMPL void BTREE_CLASS::erase(const Key &key) {
    // TODO
}

}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#endif  // SRC_BTREE_TCC_
