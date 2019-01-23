#include <iostream>

#include "imlab/buffer_manager.h"
#include "imlab/btree.h"
#include "imlab/betree.h"
// ---------------------------------------------------------------------------
namespace {
// ---------------------------------------------------------------------------
template<size_t page_size, typename T> void BM_LinearInsert(size_t max) {
    imlab::BufferManager<page_size> manager{10};
    T tree{0, manager};
    for (uint64_t i = 0; i < max; ++i)
        tree.insert(i, i);
    asm volatile("" : "+m" (tree));
}

// https://stackoverflow.com/a/1640399
uint64_t xorshf96() {
    static uint64_t x = 123456789, y = 362436069, z = 521288629;

    x ^= x << 16;
    x ^= x >> 5;
    x ^= x << 1;

    uint64_t t = x;
    x = y;
    y = z;
    z = t ^ x ^ y;

    return z;
}
template<size_t page_size, typename T> void BM_RandomInsert(size_t max) {
    imlab::BufferManager<page_size> manager{10};
    T tree{0, manager};
    for (uint64_t i = 0; i < max; ++i)
        tree.insert(xorshf96(), 0);
    asm volatile("" : "+m" (tree));
}
// ---------------------------------------------------------------------------
}  // namespace

static constexpr double kMaxBenchTime = 1e5;

static constexpr size_t kRangeStart = 1 << 8;
static constexpr size_t kRangeEnd = 1 << 20;
static constexpr size_t kRangeSteps = 20;

static constexpr size_t kRangeStep = (kRangeEnd - kRangeStart) / kRangeSteps;
#define SINGLE_BENCH(fn) do {\
    size_t range = 1 << 20;\
    for (size_t range = kRangeStart; range <= kRangeEnd; range += kRangeStep) {\
        std::chrono::time_point<std::chrono::steady_clock> start, end;\
        uint64_t times = 0;\
        double result;\
        start = std::chrono::steady_clock::now();\
        do {\
            fn(range);\
            end = std::chrono::steady_clock::now();\
            ++times;\
        } while ((result = std::chrono::duration <double, std::micro>(end - start).count()) < kMaxBenchTime);\
        std::cout << range << ',' << std::fixed << result / times << std::endl;\
    }\
} while (false)

#define B_TREE_BENCH(name, page_size) do {\
    std::cout << "#" #name "$BTree<" #page_size ">" << std::endl;\
    void (*bench)(size_t) = name<page_size, imlab::BTree<uint64_t, uint64_t, page_size>>;\
    SINGLE_BENCH(bench);\
} while (false)

#define BE_TREE_BENCH(name, page_size, epsilon) do {\
    std::cout << "#" #name "$BeTree<" #page_size "," #epsilon ">" << std::endl;\
    void (*bench)(size_t) = name<page_size, imlab::BeTree<uint64_t, uint64_t, page_size, epsilon>>;\
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
