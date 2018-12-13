// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#ifndef INCLUDE_IMLAB_RBTREE_H_
#define INCLUDE_IMLAB_RBTREE_H_

#include <functional>
// ---------------------------------------------------------------------------------------------------
namespace imlab {

namespace {
    template<uint64_t max> struct uint_fit {
        using type =
            std::conditional_t<std::numeric_limits<uint8_t >::max() >= max, uint8_t,
            std::conditional_t<std::numeric_limits<uint16_t>::max() >= max, uint16_t,
            std::conditional_t<std::numeric_limits<uint32_t>::max() >= max, uint32_t, uint64_t>>>;
    };
    template<uint64_t max> using uint_fit_t = typename uint_fit<max>::type;
}  // namespace

// In-place RB-Tree with variadic
template<typename Key, size_t page_size, typename Compare = std::less<Key>, typename... Ts>
class RBTree {
    // use the smallest uint that can address the whole data array
    using pointer = uint_fit_t<page_size>;
    // use the smallest uint that can distinguish between all value types and nodes
    using tag = uint_fit_t<sizeof...(Ts)>;

    struct RBNode {
       pointer left = 0, right = 0; // free slot
       enum Color : uint8_t { Black = 0, Red = 1 } color = Black;
       // TODO encode child status
    };

 public:
    constexpr RBTree() {
       static_assert(sizeof(*this) == page_size);
    }

 private:
    struct Header {
        pointer node_count = 0, first_free_node = 0,
            data_start = page_size - sizeof(Header), free_space = page_size - sizeof(Header);
    } header;
    std::byte data[page_size - sizeof(Header)];
};

}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#include "rbtree.tcc"
// ---------------------------------------------------------------------------------------------------
#endif  // INCLUDE_IMLAB_RBTREE_H_
