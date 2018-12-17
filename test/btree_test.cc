// ---------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "imlab/buffer_manager.h"
#include "imlab/btree.h"
// ---------------------------------------------------------------------------------------------------
namespace {
// ---------------------------------------------------------------------------------------------------
TEST(BTree, InsertEmptyTree) {
    imlab::BufferManager buffer_manager(1024, 100);
    imlab::BTree<uint64_t, uint64_t, 1024> tree(0, buffer_manager);
}
// ---------------------------------------------------------------------------------------------------
}  // namespace
// ---------------------------------------------------------------------------------------------------
