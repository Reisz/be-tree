// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#ifndef INCLUDE_IMLAB_BETREE_H_
#define INCLUDE_IMLAB_BETREE_H_

#include <functional>
#include <limits>
#include <algorithm>
#include <utility>
#include <vector>
#include "imlab/segment.h"
#include "imlab/rbtree.h"
// ---------------------------------------------------------------------------------------------------
namespace imlab {

#define IMLAB_BETREE_TEMPL \
    template<typename Key, typename T, size_t page_size, size_t epsilon, typename Compare>
#define IMLAB_BETREE_CLASS \
    BeTree<Key, T, page_size, epsilon, Compare>

template<typename Key, typename T, size_t page_size, size_t epsilon, typename Compare = std::less<Key>>
class BeTree : private Segment<page_size> {
    struct CoupledFixes;
    using Fix = typename BufferManager<page_size>::Fix;
    using ExclusiveFix = typename BufferManager<page_size>::ExclusiveFix;

 public:
    using key_type = Key;
    using value_type = T;

    struct Node;
    class InnerNode;
    class LeafNode;

    struct MessageKey;
    typedef T (*upsert_t) (T &&);
    using MessageMap = RbTree<MessageKey, epsilon, std::less<MessageKey>,
        T, T, upsert_t, void>;
    enum Message {
        Insert, InsertOrAssign, Upsert, Erase
    };
    using MessageRange = std::pair<typename MessageMap::const_iterator, typename MessageMap::const_iterator>;

    // using reference = T&;
    using const_reference = const T&;
    // using pointer = T*;
    using const_pointer = const T*;
    // class iterator;
    class const_iterator;

    BeTree(uint16_t segment_id, BufferManager<page_size> &manager)
        : Segment<page_size>(segment_id, manager) {}

    const_iterator begin() const;
    const_iterator end() const;

    const_iterator lower_bound(const Key &key) const;
    const_iterator upper_bound(const Key &key) const;
    const_iterator find(const Key &key) const;

    void insert(const Key &key, const T &value);
    // void insert(const Key &key, T &&value);
    // void insert_or_assign(const Key &key, const T &value);
    // void insert_or_assign(const Key &key, T &&value);
    void erase(const Key &key);

    uint64_t size() const;
    uint64_t size_pending() const;
    uint64_t capacity() const;

 private:
    static constexpr Compare comp{};

    std::optional<uint64_t> root;
    uint64_t next_page_id = 0;
    uint64_t next_timestamp = 1;

    uint64_t count = 0, leaf_count = 0;
    int64_t pending = 0;

    // get exclusive fix, will always return fix of valid node
    ExclusiveFix root_fix_exclusive();
    ExclusiveFix new_leaf();
    ExclusiveFix new_inner(uint16_t level);

    void split(ExclusiveFix &parent, ExclusiveFix &child, const Key &key);
    // flush starting at the root node until there is space for at least one message
    // fixes the entire path currently being flushed and splits recursively as necessary
    void flush(ExclusiveFix &root, size_t min_amount);
    // find the best child to fulshu to, also immediately flushes to dirty childs if possible
    std::pair<uint32_t, size_t> find_flush(ExclusiveFix &fix);
    void insert_leaf(ExclusiveFix &root, const Key &key, uint64_t page_id);
};

IMLAB_BETREE_TEMPL struct IMLAB_BETREE_CLASS::Node {
    constexpr explicit Node(uint16_t level);
    bool is_leaf() const;

    uint16_t level;
    uint16_t count = 0;
};

IMLAB_BETREE_TEMPL class IMLAB_BETREE_CLASS::InnerNode : public Node {
 public:
    static_assert(epsilon < page_size);
    static constexpr uint32_t kCapacity =
        (page_size - sizeof(Node) - sizeof(uint64_t) - epsilon) / (sizeof(Key) + sizeof(uint64_t));

    constexpr InnerNode(uint16_t level);

    uint64_t begin() const;
    uint64_t lower_bound(const Key &key) const;
    uint64_t upper_bound(const Key &key) const;
    bool full() const;

    uint64_t at(uint32_t idx) const;
    const Key &key(uint32_t idx) const;

    bool message_insert(const MessageKey &key, const T &value);
    bool message_insert_or_assign(const MessageKey &key, const T &value);
    bool message_erase(const MessageKey &key);
    bool message_upsert(const MessageKey &key, upsert_t value);
    bool apply(typename MessageMap::const_iterator it);

    size_t map_capacity_bytes() const;
    MessageRange map_get_range(uint32_t idx) const;
    MessageRange map_get_key_range(const Key &key) const;
    void map_erase(typename MessageMap::const_iterator it);

    void init(uint64_t left);
    void insert(const Key &key, uint64_t split_page);
    Key split(InnerNode &other);

 private:
    MessageMap msgs;
    Key keys[kCapacity];
    uint64_t children[kCapacity + 1];
};

IMLAB_BETREE_TEMPL class IMLAB_BETREE_CLASS::LeafNode : public Node {
 public:
    static constexpr uint32_t kCapacity =
        (page_size - sizeof(Node)) / (sizeof(Key) + sizeof(T));

    constexpr LeafNode();

    // returns first index where keys[i] >= key
    uint32_t lower_bound(const Key &key) const;
    uint32_t upper_bound(const Key &key) const;
    const T &at(uint32_t idx) const;
    T &at(uint32_t idx);
    bool is_equal(const Key &key, uint32_t idx) const;

    bool full() const;

    // make space for a new value, shift others to the right
    T &make_space(const Key &key, uint32_t idx);
    // erase a value, shift others to the left
    void erase(uint32_t idx);

    // transfer half of the key/value pairs to `other`
    // update traversal pointers to insert `other_page`
    // returns pivot key for new page
    Key split(LeafNode &other);

 private:
    Key keys[kCapacity];
    T values[kCapacity];
};

IMLAB_BETREE_TEMPL struct IMLAB_BETREE_CLASS::CoupledFixes {
    typename BufferManager<page_size>::ExclusiveFix fix, prev;

    void advance(typename BufferManager<page_size>::ExclusiveFix next);
};

IMLAB_BETREE_TEMPL struct IMLAB_BETREE_CLASS::MessageKey {
    static constexpr Compare comp{};

    Key key;
    uint64_t timestamp;


    friend bool operator<(const MessageKey &a, const MessageKey &b) {
        return comp(a.key, b.key) || (!comp(b.key, a.key) && a.timestamp < b.timestamp);
    }

    static constexpr MessageKey min(const Key &key) {
        return {key, 0};
    }
    static constexpr MessageKey max(const Key &key) {
        return {key, std::numeric_limits<uint64_t>::max()};
    }
};

IMLAB_BETREE_TEMPL class IMLAB_BETREE_CLASS::const_iterator {
    friend class BeTree;

 public:
    const_iterator &operator++();
    bool operator==(const const_iterator &other) const;
    bool operator!=(const const_iterator &other) const;
    const_reference operator*();
    const_pointer operator->();

 private:
    const_iterator(const Segment<page_size> &segment, std::vector<Fix> fixes, typename MessageMap::const_iterator it, uint32_t idx)
        : segment(segment), fixes(std::move(fixes)), it(it), idx(idx) {}

    typename MessageMap::const_iterator it;
    uint32_t idx;
    const Segment<page_size> &segment;
    std::vector<Fix> fixes;
};

}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#include "betree.hpp"
// ---------------------------------------------------------------------------------------------------
#endif  // INCLUDE_IMLAB_BETREE_H_
