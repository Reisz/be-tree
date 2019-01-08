// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#ifndef INCLUDE_IMLAB_BETREE_H_
#define INCLUDE_IMLAB_BETREE_H_

#include <functional>
#include "imlab/segment.h"
// ---------------------------------------------------------------------------------------------------
namespace imlab {

#define IMLAB_BETREE_TEMPL \
    template<typename Key, typename T, size_t page_size, size_t epsilon, typename Compare>
#define IMLAB_BETREE_CLASS \
    BeTree<Key, T, page_size, epsilon, Compare>

template<typename Key, typename T, size_t page_size, size_t epsilon, typename Compare = std::less<Key>>
class BeTree : Segment<page_size> {
    struct CoupledFixes;

 public:
    using key_type = Key;
    using value_type = T;

    struct Node;
    class InnerNode;
    class LeafNode;

    struct MessageKey;
    struct Message;
    struct InsertMessage;
    struct InsertOrUpdateMessage;
    struct UpsertMessage;
    struct EraseMessage;

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
    void insert(const Key &key, T &&value);
    void insert_or_assign(const Key &key, const T &value);
    void insert_or_assign(const Key &key, T &&value);
    void erase(const Key &key);

    uint64_t size() const;
    uint64_t capacity() const;

 private:
    static constexpr Compare comp{};

    std::optional<uint64_t> root;
    uint64_t next_page_id = 0;

    uint64_t count = 0;
    uint64_t leaf_count = 0;
};

IMLAB_BETREE_TEMPL struct IMLAB_BETREE_CLASS::Node {
    constexpr explicit Node(uint16_t level);
    bool is_leaf() const;

    uint16_t level;
    uint16_t count = 0;
};

}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#include "betree.hpp"
// ---------------------------------------------------------------------------------------------------
#endif  // INCLUDE_IMLAB_BETREE_H_
