// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#ifndef INCLUDE_IMLAB_BTREE_H_
#define INCLUDE_IMLAB_BTREE_H_

#include <functional>
#include <utility>
#include <vector>
#include <string>
#include "imlab/segment.h"
// ---------------------------------------------------------------------------------------------------
namespace imlab {

#define IMLAB_BTREE_TEMPL \
    template<typename Key, typename T, size_t page_size, typename Compare>
#define IMLAB_BTREE_CLASS \
    BTree<Key, T, page_size, Compare>

template<typename Key, typename T, size_t page_size, typename Compare = std::less<Key>>
class BTree : Segment<page_size> {
 public:
    using key_type = Key;
    using value_type = T;

    class Node;
    class InnerNode;
    class LeafNode;

    using reference = T&;
    // using const_reference = const T&;
    class iterator;
    // class const_iterator;

    BTree(uint16_t segment_id, BufferManager<page_size> &manager)
        : Segment<page_size>(segment_id, manager) {}

    iterator begin();
    iterator end();

    iterator lower_bound(const Key &key);
    iterator upper_bound(const Key &key);
    iterator find(const Key &key);

    void insert(const Key &key, const T &value);
    void insert_or_assign(const Key &key, const T &value);
    void erase(const Key &key);

    std::string serialize() const;
    BTree deserialize(const std::string &s);

    uint64_t size() const;
    uint64_t capacity() const;

 private:
    static constexpr Compare comp{};

    std::optional<uint64_t> root;
    uint64_t next_page_id = 0;

    uint64_t count = 0;
    uint64_t leaf_count = 0;

    bool should_early_split() { return true; }  // TODO
    struct InsertResult;
    // insert in a lock coupled manner, prevent cascading splits by splitting
    // full nodes on the path in all cases
    InsertResult insert_lc_early_split(const Key &key);
    // try inserting in the tree without splitting, returns true on success
    InsertResult try_insert_lc_no_split(const Key &key);
    // lock the entire path down the tree to be able to split as needed
    InsertResult insert_full_lock_rec_split(const Key &key, const T &value);
};

IMLAB_BTREE_TEMPL class IMLAB_BTREE_CLASS::Node {
 public:
    constexpr explicit Node(uint16_t level);
    bool is_leaf() const;

 protected:
    uint16_t level;
    uint16_t count = 0;
};


IMLAB_BTREE_TEMPL class IMLAB_BTREE_CLASS::InnerNode : public Node {
    static constexpr uint32_t kCapacity =
        (page_size - sizeof(Node) - sizeof(uint64_t)) / (sizeof(Key) + sizeof(uint64_t));

 public:
    constexpr InnerNode(const Node &child);

    uint64_t lower_bound(const Key &key) const;
    bool full() const;

    void insert(const Key &key, uint64_t split_page);
    Key split(InnerNode &other);

 private:
    Key keys[kCapacity];
    uint64_t children[kCapacity];
    uint64_t right_child;
};

IMLAB_BTREE_TEMPL class IMLAB_BTREE_CLASS::LeafNode : public Node {
    using next_ptr = std::optional<uint64_t>;
    static constexpr uint32_t kCapacity =
        (page_size - sizeof(Node) - sizeof(next_ptr)) / (sizeof(Key) + sizeof(T));

 public:
    constexpr LeafNode();

    // returns first index where keys[i] >= key
    std::optional<uint32_t> find_index(const Key &key) const;
    // insert a new value, leaf can not be full
    void insert(const Key &key, const T &value);
    void erase(const Key &key);

    // transfer half of the key/value pairs to `other`
    // update traversal pointers to insert `other_page`
    // returns pivot key for new page
    Key split(LeafNode &other, uint64_t other_page);

 private:
    Key keys[kCapacity];
    T values[kCapacity];
    next_ptr next;
};

}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#include "btree.hpp"
// ---------------------------------------------------------------------------------------------------
#endif  // INCLUDE_IMLAB_BTREE_H_
