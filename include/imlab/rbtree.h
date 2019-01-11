// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#ifndef INCLUDE_IMLAB_RBTREE_H_
#define INCLUDE_IMLAB_RBTREE_H_

#include <functional>
#include <limits>
#include <tuple>
// ---------------------------------------------------------------------------------------------------
namespace imlab {

#define RBTREE_TEMPL \
    template<typename Key, size_t page_size, typename Compare, typename... Ts>
#define RBTREE_CLASS \
    RBTree<Key, page_size, Compare, Ts...>

template<uint64_t max> struct uint_fit {
    using type =
        std::conditional_t<std::numeric_limits<uint8_t >::max() >= max, uint8_t,
        std::conditional_t<std::numeric_limits<uint16_t>::max() >= max, uint16_t,
        std::conditional_t<std::numeric_limits<uint32_t>::max() >= max, uint32_t, uint64_t>>>;
};
template<uint64_t max> using uint_fit_t = typename uint_fit<max>::type;

// In-place RB-Tree with variadic
template<typename Key, size_t page_size, typename Compare = std::less<Key>, typename... Ts>
class RBTree {
    // use the smallest uint that can address the whole data array
    using pointer = uint_fit_t<page_size>;
    using node_pointer = pointer;  // TODO maybe make smaller
    // use the smallest uint that can distinguish between all value types and nodes
    using tag = uint_fit_t<sizeof...(Ts)>;
    // get element type by index
    template<size_t I> using element_t = typename std::tuple_element<I, std::tuple<Ts...>>::type;
    // bookkeeping header for the tree, subratct from final data size
    struct Header;
    static constexpr size_t kDataSize = page_size - sizeof(Header);


    // an inner node in the rb-tree, doubles as slot entry for values
    struct RBNode {
        Key key;
        pointer value;
        node_pointer parent;

        union {
            struct {
                node_pointer left = 0, right = 0;
            };
            node_pointer children[2];
        };
        enum Child : uint8_t { Left, Right };
        // assuming i->parent == *this
        constexpr Child side(node_pointer i) const {
            return left == i ? Left : Right;
        }

        friend constexpr Child operator-(const Child &c) {
            return typename RBNode::Child(1 - c);
        }

        enum Color : uint8_t { Red, Black } color = Red;

        constexpr RBNode(const Key &key, pointer value, node_pointer parent)
            : key(key), value(value), parent(parent) {}
    };

    // representation of values in the tree, grow from the end of the data area
    template<size_t I> struct RBTag {
        const tag type = I;
    };

    template<size_t I> struct RBValue : RBTag<I> {
        element_t<I> value;

        explicit constexpr RBValue(const element_t<I> &value)
            : value(value)  {}
        explicit constexpr RBValue(element_t<I> &&value)
            : value(value)  {}
    };

 public:
    class const_reference;
    class const_iterator;

    constexpr RBTree() {
       static_assert(sizeof(*this) == page_size);
    }

    const_iterator begin() const;
    const_iterator end() const;

    const_iterator lower_bound(const Key &key) const;
    const_iterator upper_bound(const Key &key) const;
    const_iterator find(const Key &key) const;

    void erase(const_iterator pos);
    void erase(const_iterator start, const_iterator end);

    // three insert variants (void, copy, move)
    template<size_t I> std::enable_if_t<std::is_same_v<void, element_t<I>>, bool> insert(const Key &key) {
        auto result = insert<RBTag<I>>(key);
        if (result)
            new (&value_at<RBTag<I>>(result.value())) RBTag<I>();
        return result.has_value();
    }
    template<size_t I> bool insert(const Key &key, const element_t<I> &value) {
        auto result = insert<RBValue<I>>(key);
        if (result)
            new (&value_at<RBValue<I>>(result.value())) RBValue<I>(value);
        return result.has_value();
    }
    template<size_t I> bool insert(const Key &key, element_t<I> &&value) {
        auto result = insert<RBValue<I>>(key);
        if (result)
            new (&value_at<RBValue<I>>(result.value())) RBValue<I>(std::forward<element_t<I>>(value));
        return result.has_value();
    }


    // testing interface, not linked in prod code
    void check_rb_invariants();

 private:
    // For use in functions working with nodes
    struct NodeRef {
        node_pointer i;
        RBNode *node;

        operator node_pointer() { return i; }
        RBNode *operator->() { return node; }
        RBNode &operator*() { return *node; }
    };
    struct const_noderef {
        node_pointer i;
        const RBNode *node;

        operator node_pointer() { return i; }
        const RBNode *operator->() { return node; }
        const RBNode &operator*() { return *node; }
    };
    inline NodeRef ref(node_pointer i) {
        return { i , i ? reinterpret_cast<RBNode*>(data) + (i - 1) : nullptr };
    }
    inline const_noderef ref(node_pointer i) const {
        return { i , i ? reinterpret_cast<const RBNode*>(data) + (i - 1) : nullptr };
    }
    template<typename... Args> inline NodeRef emplace_node(Args &&...a) {
        node_pointer i = header.node_count++;
        header.free_space -= sizeof(RBNode);

        NodeRef result = ref(i + 1);
        new (result.node) RBNode(a...);

        return ref(i + 1);
    }

    template<typename T> inline T &value_at(pointer i) {
        return *(reinterpret_cast<T*>(data + i));
    }
    template<typename T> inline const T &value_at(pointer i) const {
        return *(reinterpret_cast<const T*>(data + i));
    }
    template<typename Val> inline pointer reserve_value() {
        pointer i = header.data_start - sizeof(Val);
        header.free_space -= sizeof(Val);
        return header.data_start = i;
    }

    // helper functions
    template<typename T> std::optional<pointer> insert(const Key &key);
    void rotate(NodeRef node, typename RBNode::Child child);

    static constexpr Compare comp{};

    // member variables
    struct Header {
        node_pointer root_node = 0, node_count = 0;
        pointer data_start = kDataSize, free_space = kDataSize;
    } header;
    std::byte data[kDataSize];
};

RBTREE_TEMPL class RBTREE_CLASS::const_iterator {
    friend class RBTree;
 public:
    const_iterator &operator++();
    const_iterator operator++(int);
    const_iterator &operator--();
    const_iterator operator--(int);
    bool operator==(const const_iterator &other) const;
    bool operator!=(const const_iterator &other) const;
    const_reference operator*() const;

 private:
    const_iterator(const RBTree &tree, node_pointer i)
        : tree(tree), i(i) {}

    const RBTree &tree;
    node_pointer i;
};

RBTREE_TEMPL class RBTREE_CLASS::const_reference {
    friend class const_iterator;
 public:
    tag type() const;
    template<size_t I> const element_t<I> &as() const;

 private:
    const_reference(const RBTree &tree, pointer i)
        : tree(tree), i(i) {}

    const RBTree &tree;
    pointer i;
};

}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#include "rbtree.hpp"
// ---------------------------------------------------------------------------------------------------
#endif  // INCLUDE_IMLAB_RBTREE_H_
