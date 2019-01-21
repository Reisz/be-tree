// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#ifndef INCLUDE_IMLAB_BETREE_H_
#define INCLUDE_IMLAB_BETREE_H_

#include <functional>
#include <limits>
#include <algorithm>
#include "imlab/segment.h"
#include "imlab/rbtree.h"
// ---------------------------------------------------------------------------------------------------
namespace imlab {

#define IMLAB_BETREE_TEMPL \
    template<typename Key, typename T, size_t page_size, size_t epsilon, typename Compare>
#define IMLAB_BETREE_CLASS \
    BeTree<Key, T, page_size, epsilon, Compare>

template<typename Key, typename T, size_t page_size, size_t epsilon, typename Compare = std::less<Key>>
class BeTree : Segment<page_size> {
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

    using reference = T&;
    // using const_reference = const T&;
    class iterator;
    // class const_iterator;

    BeTree(uint16_t segment_id, BufferManager<page_size> &manager)
        : Segment<page_size>(segment_id, manager) {}

    iterator begin();
    iterator end();

    iterator lower_bound(const Key &key);
    iterator upper_bound(const Key &key);
    iterator find(const Key &key);

    void insert(const Key &key, const T &value);
    // void insert(const Key &key, T &&value);
    // void insert_or_assign(const Key &key, const T &value);
    // void insert_or_assign(const Key &key, T &&value);
    void erase(const Key &key);

    uint64_t size() const;
    uint64_t capacity() const;

 private:
    static constexpr Compare comp{};

    std::optional<uint64_t> root;
    uint64_t next_page_id = 0;
    uint64_t next_timestamp = 1;

    uint64_t count = 0;
    uint64_t leaf_count = 0;

    // get exclusive fix, will always return fix of valid node
    ExclusiveFix root_fix_exclusive();
    ExclusiveFix new_leaf();
    ExclusiveFix new_inner(uint16_t level);

    void split(ExclusiveFix &parent, ExclusiveFix &child, const Key &key);
    void flush();
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

    const MessageMap &messages() const;
    MessageMap &messages();

    void init(uint64_t left);
    void insert(const Key &key, uint64_t split_page);
    Key split(InnerNode &other);

 private:
    MessageMap msgs;
    Key keys[kCapacity];
    uint64_t children[kCapacity];
    uint64_t right_child;
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
    Key split(LeafNode &other, uint64_t other_page);

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
        return comp(a.key, b.key) || (!comp(b.key, b.key) && a.timestamp < b.timestamp);
    }
    friend bool operator<(const Key &key, const MessageKey &mk) {
        return comp(key, mk.key);
    }
    friend bool operator<(const MessageKey &mk, const Key &key) {
        return comp(mk.key, key);
    }

    static constexpr MessageKey min(const Key &key) {
        return {key, 0};
    }
    static constexpr MessageKey max(const Key &key) {
        return {key, std::numeric_limits<uint64_t>::max()};
    }
};

}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#include "betree.hpp"
// ---------------------------------------------------------------------------------------------------
#endif  // INCLUDE_IMLAB_BETREE_H_
