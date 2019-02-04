#include "imlab/buffer_manager.h"
#include "imlab/infra/random.h"
#include "imlab/betree.h"
// ---------------------------------------------------------------------------
int main(int argc, char **argv) {
    imlab::BufferManager<1024> manager{100};
    imlab::BeTree<uint64_t, uint64_t, 1024, 255> tree{0, manager};

    for (uint64_t i = 0; i < 1 << 17; ++i)
        tree.insert(xorshf96(), 0);
}
// ---------------------------------------------------------------------------
