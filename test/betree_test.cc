// ---------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "imlab/betree.h"
#include "imlab/infra/random.h"
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
namespace {
// ---------------------------------------------------------------------------------------------------
template<size_t page_size, size_t epsilon> using BeTreeTest = imlab::BeTree<uint64_t, uint64_t, page_size, epsilon>;
// ---------------------------------------------------------------------------------------------------
TEST(BeTree, Sizes) {
    ASSERT_GE(1024, sizeof(BeTreeTest<1024, 256>::InnerNode));
    ASSERT_GE(1024, sizeof(BeTreeTest<1024, 256>::LeafNode));
}

TEST(BeTree, InsertEmptyTree) {
    imlab::BufferManager<1024> buffer_manager{10};
    BeTreeTest<1024, 256> tree(0, buffer_manager);

    tree.insert(12, 34);
    EXPECT_LE(tree.size(), tree.size_pending());
    EXPECT_EQ(1, tree.size_pending());

    auto it = tree.find(12);
    ASSERT_NE(tree.end(), it);
    ASSERT_EQ(34, *it);
}

TEST(BeTree, InsertSplitRootLeaf) {
    imlab::BufferManager<1024> buffer_manager{10};
    BeTreeTest<1024, 256> tree(0, buffer_manager);

    size_t i = 0;
    for (; i <= BeTreeTest<1024, 256>::LeafNode::kCapacity; ++i)
        tree.insert(i, i);
    EXPECT_LE(tree.size(), tree.size_pending());
    EXPECT_EQ(i, tree.size_pending());

    for (i = 0; i <= BeTreeTest<1024, 256>::LeafNode::kCapacity; ++i)
        ASSERT_EQ(i, *tree.find(i));
}

template<size_t size, size_t e> constexpr auto insert_amount =
    BeTreeTest<size, e>::LeafNode::kCapacity * BeTreeTest<size, e>::InnerNode::kCapacity * 2;

TEST(BeTree, MultipleInserts) {
    imlab::BufferManager<1024> buffer_manager{10};
    BeTreeTest<1024, 256> tree(0, buffer_manager);
    constexpr auto ia = insert_amount<1024, 256>;

    for (uint32_t i = 0; i < ia; ++i)
        tree.insert(i, i);
    EXPECT_LE(tree.size(), tree.size_pending());
    EXPECT_EQ(ia, tree.size_pending());

    ASSERT_EQ(tree.end(), tree.find(insert_amount<1024, 256>));
    // ASSERT_EQ(tree.end(), tree.lower_bound(insert_amount<1024, 256>));
    // ASSERT_EQ(tree.end(), tree.upper_bound(insert_amount<1024, 256> - 1));

    // uint32_t i = 0;
    // for (auto j : tree)
    //     EXPECT_EQ(i++, j);
    // for (uint32_t i = 0; i < insert_amount<1024, 256>; ++i) {
    //     EXPECT_EQ(i, *tree.find(i));
    //     EXPECT_EQ(i, *tree.lower_bound(i));
    //     if (i > 0)
    //         EXPECT_EQ(i, *tree.upper_bound(i - 1));
    // }
}

TEST(BeTree, MultipleInsertsReverse) {
    imlab::BufferManager<1024> buffer_manager{10};
    BeTreeTest<1024, 256> tree(0, buffer_manager);
    constexpr auto ia = insert_amount<1024, 256>;

    for (uint32_t i = ia; i > 0; --i)
        tree.insert(i, i);
    EXPECT_LE(tree.size(), tree.size_pending());
    EXPECT_EQ(ia, tree.size_pending());

    // ASSERT_EQ(tree.end(), tree.find(insert_amount<1024, 256>));
    // ASSERT_EQ(tree.end(), tree.lower_bound(insert_amount<1024, 256>));
    // ASSERT_EQ(tree.end(), tree.upper_bound(insert_amount<1024, 256> - 1));
    //
    // uint32_t i = 0;
    // for (auto j : tree)
    //     EXPECT_EQ(i++, j);
    // for (uint32_t i = 0; i < insert_amount<1024, 256>; ++i) {
    //     EXPECT_EQ(i, *tree.find(i));
    //     EXPECT_EQ(i, *tree.lower_bound(i));
    //     if (i > 0)
    //         EXPECT_EQ(i, *tree.upper_bound(i - 1));
    // }
}

TEST(BeTree, MultipleInsertsRandom) {
    imlab::BufferManager<1024> buffer_manager{10};
    BeTreeTest<1024, 256> tree(0, buffer_manager);
    constexpr auto ia = insert_amount<1024, 256>;

    for (uint32_t i = 0; i < ia; ++i)
        tree.insert(xorshf96(), 0);
    EXPECT_LE(tree.size(), tree.size_pending());
    EXPECT_EQ(ia, tree.size_pending());

    // ASSERT_EQ(tree.end(), tree.find(insert_amount<1024, 256>));
    // ASSERT_EQ(tree.end(), tree.lower_bound(insert_amount<1024, 256>));
    // ASSERT_EQ(tree.end(), tree.upper_bound(insert_amount<1024, 256> - 1));
    //
    // uint32_t i = 0;
    // for (auto j : tree)
    //     EXPECT_EQ(i++, j);
    // for (uint32_t i = 0; i < insert_amount<1024, 256>; ++i) {
    //     EXPECT_EQ(i, *tree.find(i));
    //     EXPECT_EQ(i, *tree.lower_bound(i));
    //     if (i > 0)
    //         EXPECT_EQ(i, *tree.upper_bound(i - 1));
    // }
}

}  // namespace
// ---------------------------------------------------------------------------------------------------
