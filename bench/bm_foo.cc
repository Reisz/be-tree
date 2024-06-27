// ---------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------
#include "benchmark/benchmark.h"
#include "imlab/buffer_manager.h"
#include "imlab/betree.h"
// ---------------------------------------------------------------------------
namespace {
// ---------------------------------------------------------------------------
void BM_MeaningfulName(benchmark::State &state) {
    char buffer[255];
    for (auto _ : state) {

        // The following line basically tells the compiler that
        // anything could happen with the contents of the buffer which
        // prevents many compiler optimizations.
        asm volatile("" : "+m" (buffer));
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
    state.SetBytesProcessed(state.iterations() * 1);

    // Use user-defined counters if you want to track something else.
    state.counters["user_defined_counter"] = 42;
}

volatile uint16_t segment = 0;
void BM_BeTreeLinearInsert(benchmark::State &state) {
    imlab::BufferManager<1024> manager{10};
    for (auto _ : state) {
        imlab::BeTree<uint64_t, uint64_t, 1024, 255> tree{segment++, manager};

        for (uint64_t i = 0; i < state.range(0); ++i)
            tree.insert(i, i);
        benchmark::DoNotOptimize(tree);
    }
};
// ---------------------------------------------------------------------------
}  // namespace
// ---------------------------------------------------------------------------
BENCHMARK(BM_BeTreeLinearInsert)
    -> Range(1 << 8, 1 << 20);
// ---------------------------------------------------------------------------
int main(int argc, char **argv) {
    // Your could load TPCH into global vectors here

    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
}
// ---------------------------------------------------------------------------
