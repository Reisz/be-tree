// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#ifndef SRC_TEST_CHECK_BETREE_HPP_
#define SRC_TEST_CHECK_BETREE_HPP_
// ---------------------------------------------------------------------------------------------------
#include "imlab/betree.h"

#include <optional>
#include <vector>
// ---------------------------------------------------------------------------------------------------
template<typename Key, typename T, size_t page_size, size_t epsilon,  typename Compare>
void imlab::BeTree<Key, T, page_size, epsilon, Compare>::check_be_invariants() const {
    if (!root)
        return;

    struct BeCheck {
            uint64_t page_id;
            std::optional<Key> min, max;
    };
    std::vector<BeCheck> to_check;
    to_check.push_back({*root});

    while (to_check.size()) {
        auto check = to_check.back();
        to_check.pop_back();

        auto fix = this->fix(check.page_id);

        if (fix.template as<Node>()->is_leaf()) {
            const auto &leaf = *fix.template as<LeafNode>();

            if (check.min)
                ASSERT_TRUE(comp(*check.min, leaf.key(0)));
            if (check.max)
                ASSERT_TRUE(!comp(*check.max, leaf.key(leaf.count - 1)));

            const auto *last_key = &leaf.key(0);
            for (uint32_t i = 1; i < leaf.count; ++i) {
                ASSERT_TRUE(comp(*last_key, leaf.key(i)));
                last_key = &leaf.key(i);
            }

        } else {
            const auto &inner = *fix.template as<InnerNode>();
            inner.messages().check_rb_invariants();

            if (check.min)
                ASSERT_TRUE(comp(*check.min, inner.key(0)));
            if (check.max)
                ASSERT_TRUE(!comp(*check.max, inner.key(inner.count - 1)));

            ASSERT_EQ(inner.messages().begin(), inner.map_get_range(0).first);
            ASSERT_EQ(inner.messages().end(), inner.map_get_range(inner.count).second);
            for (uint32_t i = 0; i <= inner.count; ++i) {
                for (auto iters = inner.map_get_range(i); iters.first != iters.second; ++iters.first) {
                    if (i > 0)
                        ASSERT_TRUE(!comp(iters.first->key().key, inner.key(i - 1)));
                    if (i < inner.count)
                        ASSERT_TRUE(comp(iters.first->key().key, inner.key(i)));
                }

                if (i > 0 && i < inner.count)
                    to_check.push_back({inner.at(i), inner.key(i - 1), inner.key(i)});
                else if (i > 0)
                    to_check.push_back({inner.at(i), inner.key(i - 1), {}});
                else
                    to_check.push_back({inner.at(i), {}, inner.key(i)});
            }

            const auto *last_key = &inner.key(0);
            for (uint32_t i = 1; i < inner.count; ++i) {
                ASSERT_TRUE(comp(*last_key, inner.key(i)));
                last_key = &inner.key(i);
            }
        }
    }
}
// ---------------------------------------------------------------------------------------------------
#endif  // SRC_TEST_CHECK_BETREE_HPP_
