// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#ifndef INCLUDE_IMLAB_BTREE_H_
#define INCLUDE_IMLAB_BTREE_H_

#include <functional>
#include <utility>
#include <vector>
// ---------------------------------------------------------------------------------------------------
namespace imlab {

class BufferManager;

template<size_t actual, size_t expected> struct check_size {
  static_assert(actual <= expected);
};

template<typename Key, typename T, size_t page_size, typename Compare = std::less<T>>
class BTree {
    using key_type = Key;
    using mapped_type = T;

    struct Node {
        uint16_t level;
        uint16_t child_count;
        inline bool is_leaf() const {
            return level == 0;
        }
    };

    struct InnerNode : public Node {
        static constexpr uint32_t kCapacity
            = (page_size - sizeof(Node)) / (sizeof(Key) + sizeof(uint64_t));
        Key keys[kCapacity];
        uint64_t children[kCapacity];

        InnerNode() : Node(0, 0) {}

        std::pair<uint32_t, bool> lower_bound(const Key &key) const;
        void insert(const Key &key, uint64_t split_page);
        Key split(std::byte* buffer);

        // testing interface, not linked in prod code
        std::vector<Key> get_key_vector() const;
        std::vector<uint64_t> get_child_vector() const;
    };
    static constexpr check_size<sizeof(InnerNode), page_size> check_inner{};

    struct LeafNode: public Node {
        static constexpr uint32_t kCapacity
            = (page_size - sizeof(Node)) / (sizeof(Key) + sizeof(T));
        Key keys[kCapacity];
        T values[kCapacity];

        LeafNode() : Node(0, 0) {}
        void insert(const Key &key, const T &value);
        void erase(const Key &key);
        Key split(std::byte* buffer);

        // testing interface, not linked in prod code
        std::vector<Key> get_key_vector();
        std::vector<T> get_value_vector();
    };
    static constexpr check_size<sizeof(LeafNode), page_size> check_leaf{};

 public:
    BTree(uint16_t segment_id, BufferManager &manager) {}  // TODO

    std::optional<T&> find(const Key &key);
    void insert(const Key &key, const T &value);
    void erase(const Key &key);
    // TODO range_query
};

}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#include "btree.tcc"
// ---------------------------------------------------------------------------------------------------
#endif  // INCLUDE_IMLAB_BTREE_H_
