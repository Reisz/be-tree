#include <iostream>

#include "imlab/buffer_manager.h"
#include "imlab/btree.h"
#include "imlab/betree.h"
#include "imlab/infra/random.h"
#include "imlab/infra/bench.h"
// ---------------------------------------------------------------------------
namespace {
constexpr size_t find_amount = 1 << 14;
// ---------------------------------------------------------------------------
template<size_t page_size, typename T> void BM_LinearInsert(Bencher &bencher) {
    imlab::BufferManager<page_size> manager{10};
    T tree{0, manager};

    bencher.start_timer();
    for (uint64_t i = 0; i < bencher.count; ++i)
        tree.insert(i, i);
    bencher.end_timer_write();

    size_t i = 0;
    bencher.start_timer();
    for (uint64_t i = 0; i < find_amount; ++i)
        i += tree.find(xorshf96()) == tree.end();
    bencher.end_timer_read();
    asm volatile("" : "+m" (i));

    bencher.depth = tree.depth();
    bencher.reads = manager.page_reads();
    bencher.writes = manager.page_writes();
    asm volatile("" : "+m" (tree));
}

template<size_t page_size, typename T> void BM_RandomInsert(Bencher &bencher) {
    imlab::BufferManager<page_size> manager{100};
    T tree{0, manager};

    bencher.start_timer();
    for (uint64_t i = 0; i < bencher.count; ++i)
        tree.insert(xorshf96(), 0);
    bencher.end_timer_write();

    size_t i = 0;
    bencher.start_timer();
    for (uint64_t i = 0; i < find_amount; ++i)
        i += tree.find(xorshf96()) == tree.end();
    bencher.end_timer_read();
    asm volatile("" : "+m" (i));

    bencher.depth = tree.depth();
    bencher.reads = manager.page_reads();
    bencher.writes = manager.page_writes();
    asm volatile("" : "+m" (tree));
}
// ---------------------------------------------------------------------------
}  // namespace

#define B_TREE_BENCH(name, page_size) do {\
    std::cout << "#" #name "$BTree<" #page_size ">" << std::endl;\
    void (*bench)(Bencher &) = name<page_size, imlab::BTree<uint64_t, uint64_t, page_size>>;\
    SINGLE_BENCH(bench);\
} while (false)

#define BE_TREE_BENCH(name, page_size, epsilon) do {\
    std::cout << "#" #name "$BeTree<" #page_size "," #epsilon ">" << std::endl;\
    void (*bench)(Bencher &) = name<page_size, imlab::BeTree<uint64_t, uint64_t, page_size, epsilon>>;\
    SINGLE_BENCH(bench);\
} while (false)

#define BENCH(name) do {\
    B_TREE_BENCH(name, 1024);\
    B_TREE_BENCH(name, 4096);\
    BE_TREE_BENCH(name, 1024, 64);\
    BE_TREE_BENCH(name, 1024, 128);\
    BE_TREE_BENCH(name, 1024, 255);\
    BE_TREE_BENCH(name, 1024, 512);\
    BE_TREE_BENCH(name, 4096, 64);\
    BE_TREE_BENCH(name, 4096, 128);\
    BE_TREE_BENCH(name, 4096, 255);\
    BE_TREE_BENCH(name, 4096, 512);\
    BE_TREE_BENCH(name, 4096, 1024);\
    BE_TREE_BENCH(name, 4096, 2048);\
} while (false)

// ---------------------------------------------------------------------------
int main(int argc, char **argv) {
    BENCH(BM_LinearInsert);
    BENCH(BM_RandomInsert);
}
// ---------------------------------------------------------------------------
