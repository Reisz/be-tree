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
// ---------------------------------------------------------------------------
}  // namespace

static constexpr double kMaxBenchTime = 1e6;

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
        std::cout << range << ',' << result / times << std::endl;\
    }\
} while (false)

#define B_TREE_BENCH(name, page_size) do {\
    std::cout << "BTree<" << page_size << '>' << std::endl;\
    void (*bench)(size_t) = name<page_size, imlab::BTree<uint64_t, uint64_t, page_size>>;\
    SINGLE_BENCH(bench);\
} while (false)

#define BE_TREE_BENCH(name, page_size, epsilon) do {\
    std::cout << "BeTree<" << page_size << ',' << epsilon << '>' << std::endl;\
    void (*bench)(size_t) = name<page_size, imlab::BeTree<uint64_t, uint64_t, page_size, epsilon>>;\
    SINGLE_BENCH(bench);\
} while (false)

#define BENCH(name) do {\
    std::cout << #name << std::endl;\
    B_TREE_BENCH(name, 256);\
    B_TREE_BENCH(name, 512);\
    B_TREE_BENCH(name, 1024);\
    B_TREE_BENCH(name, 4096);\
    BE_TREE_BENCH(name, 256, 64);\
    BE_TREE_BENCH(name, 512, 64);\
    BE_TREE_BENCH(name, 512, 128);\
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
}
// ---------------------------------------------------------------------------
