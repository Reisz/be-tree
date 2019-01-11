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
    RbTree<Key, page_size, Compare, Ts...>

template<uint64_t max> struct uint_fit {
    using type =
        std::conditional_t<std::numeric_limits<uint8_t >::max() >= max, uint8_t,
        std::conditional_t<std::numeric_limits<uint16_t>::max() >= max, uint16_t,
        std::conditional_t<std::numeric_limits<uint32_t>::max() >= max, uint32_t, uint64_t>>>;
};
template<uint64_t max> using uint_fit_t = typename uint_fit<max>::type;

// In-place RB-Tree with variadic
template<typename Key, size_t page_size, typename Compare = std::less<Key>, typename... Ts>
class RbTree {
    // bookkeeping header for the tree, subratct from final data size
    struct Header;
    // an inner node in the rb-tree, doubles as slot entry for values
    struct Node;
    // representation of values in the tree, grow from the end of the data area
    template<size_t I> struct Tag;
    template<size_t I> struct Value;

    // use the smallest uint that can address the whole data array
    using pointer = uint_fit_t<page_size>;
    using node_pointer = pointer;  // TODO maybe make smaller
    // use the smallest uint that can distinguish between all value types and nodes
    using tag = uint_fit_t<sizeof...(Ts)>;
    // get element type by index
    template<size_t I> using element_t = typename std::tuple_element<I, std::tuple<Ts...>>::type;
    static constexpr size_t kDataSize = page_size - sizeof(Header);

 public:
    class const_reference;
    class const_iterator;

    constexpr RbTree();

    const_iterator begin() const;
    const_iterator end() const;

    const_iterator lower_bound(const Key &key) const;
    const_iterator upper_bound(const Key &key) const;
    const_iterator find(const Key &key) const;

    void erase(const_iterator pos);
    void erase(const_iterator start, const_iterator end);

    // three insert variants (void, copy, move)
    template<size_t I> std::enable_if_t<std::is_same_v<void, element_t<I>>, bool> insert(const Key &key);
    template<size_t I> bool insert(const Key &key, const element_t<I> &value);
    template<size_t I> bool insert(const Key &key, element_t<I> &&value);

    // testing interface, not linked in prod code
    void check_rb_invariants() const;

 private:
    // For use in functions working with nodes
    struct node_ref;
    struct const_node_ref;

    inline node_ref ref(node_pointer i);
    inline const_node_ref ref(node_pointer i) const;
    template<typename... Args> node_ref emplace_node(Args &&...a);

    template<typename T> inline T &value_at(pointer i);
    template<typename T> inline const T &value_at(pointer i) const;
    template<typename T> pointer reserve_value();

    // helper functions
    template<typename T> std::optional<pointer> insert(const Key &key);
    void rotate(node_ref node, typename Node::Child child);

    static constexpr Compare comp{};

    // member variables
    struct Header {
        node_pointer root_node = 0, node_count = 0;
        pointer data_start = kDataSize, free_space = kDataSize;
    } header;
    std::byte data[kDataSize];
};

RBTREE_TEMPL struct RBTREE_CLASS::Node {
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
    constexpr Child side(node_pointer i) const;
    friend constexpr Child operator-(const Child &c) {
        return typename RBTREE_CLASS::Node::Child(1 - c);
    }

    enum Color : uint8_t { Red, Black } color = Red;

    constexpr Node(const Key &key, pointer value, node_pointer parent)
        : key(key), value(value), parent(parent) {}
};


RBTREE_TEMPL template<size_t I> struct RBTREE_CLASS::Tag {
    const tag type = I;
};

RBTREE_TEMPL template<size_t I> struct RBTREE_CLASS::Value : Tag<I> {
    element_t<I> value;

    explicit constexpr Value(const element_t<I> &value)
        : value(value)  {}
    explicit constexpr Value(element_t<I> &&value)
        : value(value)  {}
};

RBTREE_TEMPL class RBTREE_CLASS::const_iterator {
    friend class RbTree;
 public:
    const_iterator &operator++();
    const_iterator operator++(int);
    bool operator==(const const_iterator &other) const;
    bool operator!=(const const_iterator &other) const;
    const_reference operator*() const;
    const const_reference *operator->() const;

 private:
    const_iterator(const RbTree *tree, node_pointer i)
        : tree(tree), ref(tree, i) {}

    const RbTree *tree;
    const_reference ref;
};

RBTREE_TEMPL class RBTREE_CLASS::const_reference {
    friend class const_iterator;
 public:
    const Key &key() const;

    tag type() const;
    template<size_t I> const element_t<I> &as() const;

 private:
    const_reference(const RbTree *tree, node_pointer i)
        : tree(tree), i(i) {}

    const RbTree *tree;
    node_pointer i;
};

RBTREE_TEMPL struct RBTREE_CLASS::node_ref {
    node_pointer i;
    Node *node;

    operator node_pointer() { return i; }
    Node *operator->() { return node; }
    Node &operator*() { return *node; }
};

RBTREE_TEMPL struct RBTREE_CLASS::const_node_ref {
    node_pointer i;
    const Node *node;

    operator node_pointer() { return i; }
    const Node *operator->() { return node; }
    const Node &operator*() { return *node; }
};

}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#include "rbtree.hpp"
// ---------------------------------------------------------------------------------------------------
#endif  // INCLUDE_IMLAB_RBTREE_H_
