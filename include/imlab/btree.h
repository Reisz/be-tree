// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#ifndef INCLUDE_IMLAB_BTREE_H_
#define INCLUDE_IMLAB_BTREE_H_

#include <functional>
#include <utility>
#include <vector>
#include "imlab/segment.h"
// ---------------------------------------------------------------------------------------------------
namespace imlab {

class BufferManager;

template<size_t actual, size_t expected, uint32_t count> struct check_size {
    static_assert(count > 1);
    static_assert(actual <= expected);
    static_assert(actual + 8 >= expected);
};

template<typename Key, typename T, size_t page_size, typename Compare = std::less<Key>>
class BTree : Segment {
    using key_type = Key;
    using mapped_type = T;

    struct Node {
        uint16_t level;
        uint16_t count;
        inline bool is_leaf() const {
            return level == 0;
        }
    };

    struct InnerNode : public Node {
        static constexpr uint32_t kCapacity
            = (page_size - sizeof(Node) - sizeof(uint64_t)) / (sizeof(Key) + sizeof(uint64_t));
        Key keys[kCapacity];
        uint64_t children[kCapacity + 1];

        InnerNode() : Node(0, 0) {}

        std::optional<uint32_t> lower_bound(const Key &key) const;
        void insert(const Key &key, uint64_t split_page);
        Key split(InnerNode &other);

        // testing interface, not linked in prod code
        std::vector<Key> get_key_vector() const;
        std::vector<uint64_t> get_child_vector() const;
    };
    static constexpr check_size<sizeof(InnerNode), page_size, InnerNode::kCapacity> check_inner{};

    struct LeafNode: public Node {
        std::optional<uint64_t> next;

        static constexpr uint32_t kCapacity
            = (page_size - sizeof(Node) - sizeof(next)) / (sizeof(Key) + sizeof(T));
        Key keys[kCapacity];
        T values[kCapacity];

        LeafNode() : Node(0, 0) {}

        // returns first index where keys[i] >= key
        uint32_t lower_bound(const Key &key) const;

        // insert a new value, leaf can not be full
        void insert(const Key &key, const T &value);
        void erase(const Key &key);

        // transfer half of the key/value pairs to `other`
        // update traversal pointers to insert `other_page`
        // returns pivot key for new page
        Key split(LeafNode &other, uint64_t other_page);

        // testing interface, not linked in prod code
        std::vector<Key> get_key_vector() const;
        std::vector<T> get_value_vector() const;
    };
    static constexpr check_size<sizeof(LeafNode), page_size, LeafNode::kCapacity> check_leaf{};

 public:
    BTree(uint16_t segment_id, BufferManager &manager)
        : Segment(segment_id, manager) {}

    std::optional<const T&> find(const Key &key) const;
    // TODO range_query

    void insert(const Key &key, const T &value);
    void update(const Key &key, const T &value);
    void erase(const Key &key);

    uint64_t root_id() const;
    void load(uint64_t root_id);

    uint64_t size() const { return count; }
    uint64_t capacity() const { return leaf_space; }

 private:
    std::optional<uint64_t> root;
    uint64_t next_page_id = 0;

    uint64_t count = 0;
    uint64_t leaf_space = 0;

    // insert in a lock coupled manner, prevent cascadng splits by splitting
    // full nodes on the path in all cases
    void insert_lc_early_split(const Key &key, const T &value);
    // TODO ? bool try_insert_lc_no split(const Key &key, const T &value);
    //        void insert_full_lock_rec_split(const Key &key, const T &value);
};

}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#include "btree.tcc"
// ---------------------------------------------------------------------------------------------------
#endif  // INCLUDE_IMLAB_BTREE_H_
