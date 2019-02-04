#ifndef INCLUDE_IMLAB_INFRA_BENCH_H_
#define INCLUDE_IMLAB_INFRA_BENCH_H_

#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>

struct Bencher {
    // timing section
    std::chrono::time_point<std::chrono::steady_clock> start;
    double total;
    size_t count;

    std::vector<double> write_times;
    std::vector<double> read_times;

    void set_count(size_t c) {
        total = 0;
        count = c;

        write_times.clear();
        read_times.clear();
    }

    inline void start_timer() {
        start = std::chrono::steady_clock::now();
    }

    inline void end_timer_read() {
        auto end = std::chrono::steady_clock::now();
        double diff = std::chrono::duration <double, std::micro>(end - start).count();
        total += diff;
        read_times.push_back(diff);
    }

    inline void end_timer_write() {
        auto end = std::chrono::steady_clock::now();
        double diff = std::chrono::duration <double, std::micro>(end - start).count();
        total += diff;
        write_times.push_back(diff);
    }

    // stats section
    uint16_t depth;
    size_t reads, writes;
};

std::ostream &operator<<(std::ostream &os, Bencher &b) {
    std::sort(b.write_times.begin(), b.write_times.end());
    std::sort(b.read_times.begin(), b.read_times.end());

    os << b.count;
    os << ',' << std::fixed << b.write_times[b.write_times.size() / 2];
    os << ',' << std::fixed << b.read_times[b.read_times.size() / 2];
    os << ',' << b.depth;
    os << ',' << b.reads;
    os << ',' << b.writes;

    return os;
}

static constexpr double kMaxBenchTime = 1e5;

static constexpr size_t kRangeStart = 1 << 7;
static constexpr size_t kRangeEnd = 1 << 14;
static constexpr size_t kRangeSteps = 10;

static constexpr size_t kRangeStep = (kRangeEnd - kRangeStart) / kRangeSteps;

#define SINGLE_BENCH(fn) do {\
    Bencher bencher;\
    for (size_t range = kRangeStart; range <= kRangeEnd; range += kRangeStep) {\
        bencher.set_count(range);\
        do {\
            fn(bencher);\
        } while (bencher.total < kMaxBenchTime);\
        std::cout << bencher << std::endl;\
    }\
} while (false)

#endif  // INCLUDE_IMLAB_INFRA_BENCH_H_
