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
    template <size_t I> using element_t = typename std::tuple_element<I, std::tuple<Ts...>>::type;

    // an inner node in the rb-tree, doubles as slot entry for values
    struct RBNode {
        Key key;
        node_pointer left = 0, right = 0, parent;
        pointer value;
        enum Color : uint8_t { Red, Black } color = Red;

        constexpr RBNode(const Key &key, pointer value, node_pointer parent)
            : key(key), value(value), parent(parent) {}
    };

    // representation of values in the tree, grow from the end of the data area
    template<size_t I> struct RBValue {
        const tag type = I;
        element_t<I> value;

        explicit constexpr RBValue(const element_t<I> &value)
            : value(value)  {}
    };

 public:
    constexpr RBTree() {
       static_assert(sizeof(*this) == page_size);
    }

    template<size_t I> bool insert(const Key &key, const element_t<I> &value);

 private:
    inline RBNode &node_at(node_pointer i) {
        return *(reinterpret_cast<RBNode*>(data) + (i - 1));
    }
    inline node_pointer reserve_node() {
        node_pointer i = header.node_count++;
        header.free_space -= sizeof(RBNode);
        return i + 1;
    }
    template<size_t I> inline RBValue<I> &value_at(pointer i) {
        return *(reinterpret_cast<RBValue<I>*>(data + i));
    }
    template<size_t I> inline pointer reserve_value() {
        pointer i = header.data_start - sizeof(RBValue<I>);
        header.free_space -= sizeof(RBValue<I>);
        return header.data_start = i;
    }

    struct Header;
    static constexpr size_t kDataSize = page_size - sizeof(Header);

    struct Header {
        node_pointer root_node = 0, node_count = 0;
        pointer data_start = kDataSize, free_space = kDataSize;
    } header;

    std::byte data[kDataSize];
};

}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#include "rbtree.tcc"
// ---------------------------------------------------------------------------------------------------
#endif  // INCLUDE_IMLAB_RBTREE_H_
