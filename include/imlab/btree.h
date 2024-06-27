// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#ifndef INCLUDE_IMLAB_BTREE_H_
#define INCLUDE_IMLAB_BTREE_H_

#include "imlab/segment.h"

#include <functional>
#include <optional>
#include <utility>
// ---------------------------------------------------------------------------------------------------
namespace imlab {

#define IMLAB_BTREE_TEMPL \
    template<typename Key, typename T, size_t page_size, typename Compare>
#define IMLAB_BTREE_CLASS \
    BTree<Key, T, page_size, Compare>

template<typename Key, typename T, size_t page_size, typename Compare = std::less<Key>>
class BTree : private Segment<page_size> {
    struct CoupledFixes;
    using Fix = typename BufferManager<page_size>::Fix;
    using ExclusiveFix = typename BufferManager<page_size>::ExclusiveFix;

    using InsertResult = std::pair<ExclusiveFix, T&>;

 public:
    using key_type = Key;
    using value_type = T;

    struct Node;
    class InnerNode;
    class LeafNode;

    // TODO using value_type = std::pair<const Key, T> ?
    using reference = T&;
    // using const_reference = const T&;
    using pointer = T*;
    // using const_pointer = const T*;
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
    void insert(const Key &key, T &&value);
    void insert_or_assign(const Key &key, const T &value);
    void insert_or_assign(const Key &key, T &&value);
    void erase(const Key &key);

    uint64_t size() const;
    uint64_t capacity() const;
    uint16_t depth() const;

 private:
    static constexpr Compare comp{};

    std::optional<uint64_t> root;
    uint64_t next_page_id = 0;

    uint64_t count = 0;
    uint64_t leaf_count = 0;

    // get exclusive fix, will always return fix of valid node
    ExclusiveFix root_fix_exclusive();
    ExclusiveFix new_leaf();
    ExclusiveFix new_inner(uint16_t level);

    std::optional<InsertResult> insert_internal(const Key &key);
    InsertResult insert_or_assign_internal(const Key &key);

    void split(ExclusiveFix &parent, ExclusiveFix &child, const Key &key);

    // insert in a lock coupled manner, prevent cascading splits by splitting
    // full nodes on the path in all cases
    CoupledFixes insert_lc_early_split(const Key &key);
    // returns an exclusive fix to the leaf node which can contain the key
    // leaf node could be full
    CoupledFixes exclusive_find_node(const Key &key);
    // lock the entire path down the tree to be able to split as needed
    CoupledFixes insert_full_lock_rec_split(const Key &key);
};

IMLAB_BTREE_TEMPL struct IMLAB_BTREE_CLASS::Node {
    constexpr explicit Node(uint16_t level);
    bool is_leaf() const;

    uint16_t level;
    uint16_t count = 0;
};


IMLAB_BTREE_TEMPL class IMLAB_BTREE_CLASS::InnerNode : public Node {
 public:
    static constexpr uint32_t kCapacity =
        (page_size - sizeof(Node) - sizeof(uint64_t)) / (sizeof(Key) + sizeof(uint64_t));

    constexpr InnerNode(uint16_t level);

    uint64_t begin() const;
    uint64_t lower_bound(const Key &key) const;
    uint64_t upper_bound(const Key &key) const;
    bool full() const;

    void init(uint64_t left);
    void insert(const Key &key, uint64_t split_page);
    Key split(InnerNode &other);

 private:
    Key keys[kCapacity];
    uint64_t children[kCapacity + 1];
};

IMLAB_BTREE_TEMPL class IMLAB_BTREE_CLASS::LeafNode : public Node {
 public:
    using next_ptr = std::optional<uint64_t>;
    static constexpr uint32_t kCapacity =
        (page_size - sizeof(Node) - sizeof(next_ptr)) / (sizeof(Key) + sizeof(T));

    constexpr LeafNode();

    // returns first index where keys[i] >= key
    uint32_t lower_bound(const Key &key) const;
    uint32_t upper_bound(const Key &key) const;
    const T &at(uint32_t idx) const;
    T &at(uint32_t idx);
    bool is_equal(const Key &key, uint32_t idx) const;

    const next_ptr &get_next() const;

    bool full() const;

    // make space for a new value, shift others to the right
    T &make_space(const Key &key, uint32_t idx);
    // erase a value, shift others to the left
    void erase(uint32_t idx);

    // transfer half of the key/value pairs to `other`
    // update traversal pointers to insert `other_page`
    // returns pivot key for new page
    Key split(LeafNode &other, uint64_t other_page);

 private:
    Key keys[kCapacity];
    T values[kCapacity];
    next_ptr next;
};

IMLAB_BTREE_TEMPL struct IMLAB_BTREE_CLASS::CoupledFixes {
    typename BufferManager<page_size>::ExclusiveFix fix, prev;

    void advance(typename BufferManager<page_size>::ExclusiveFix next);
};

IMLAB_BTREE_TEMPL class IMLAB_BTREE_CLASS::iterator {
    friend class BTree;

 public:
    iterator &operator++();
    bool operator==(const iterator &other) const;
    bool operator!=(const iterator &other) const;
    reference operator*();
    pointer operator->();

 private:
    iterator(Segment<page_size> &segment, typename BufferManager<page_size>::ExclusiveFix fix, uint32_t i)
        : segment(segment), fix(std::move(fix)), i(i) {}

    typename BufferManager<page_size>::ExclusiveFix fix;
    Segment<page_size> &segment;
    uint32_t i;
};

}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#include "btree.hpp"
// ---------------------------------------------------------------------------------------------------
#endif  // INCLUDE_IMLAB_BTREE_H_
