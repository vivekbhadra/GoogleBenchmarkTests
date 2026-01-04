/**
 * @file shared_mutex_vs_mutex_bench.cpp
 * @brief Industry-grade performance analysis of std::mutex vs std::shared_mutex.
 * * DESIGN PRINCIPLE:
 * This benchmark measures "Throughput Scaling." By increasing threads from 2 to 8,
 * we observe how the system handles increasing reader pressure against a
 * single constant writer.
 */

#include <benchmark/benchmark.h>
#include <cmath>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <vector>

/**
 * @struct BenchmarkContext
 * @brief Isolated data container to prevent "False Sharing."
 * * False Sharing occurs when two threads modify data on the same cache line.
 * alignas(64) ensures our mutexes are on separate cache lines, providing
 * the most accurate "pure" measurement of the locking mechanism itself.
 */
struct BenchmarkContext
{
    std::map<int, double> data;

    /// @brief Mutex for exclusive access (pessimistic).
    alignas(64) std::mutex regular_mtx;

    /// @brief Mutex for shared access (optimistic).
    alignas(64) std::shared_mutex shared_mtx;

    /**
     * @brief Setup shared data once.
     */
    void setup()
    {
        if (data.empty())
        {
            for (int i = 0; i < 1000; ++i)
            {
                data[i] = std::sqrt(i);
            }
        }
    }
};

/// @brief Global context instance for the benchmark.
static BenchmarkContext g_ctx;

/**
 * @brief EXTREMELY LIGHT Read Workload.
 * Perform a single lookup instead of 50 trig calculations.
 */
void DoLightRead()
{
    double value = g_ctx.data[500]; 
    benchmark::DoNotOptimize(value);
}

/**
 * @brief Write Workload.
 * Simulates a state update (e.g., cache invalidation or value update).
 */
void DoWrite()
{
    g_ctx.data[0] += 1.1;
    benchmark::DoNotOptimize(g_ctx.data[0]);
}

/**
 * @brief Benchmark: Regular Mutex (Mixed Workload).
 * All threads are serialized. Adding threads increases wait time linearly.
 */
static void BM_RegularMutex_Mixed(benchmark::State &state)
{
    g_ctx.setup();
    for (auto _ : state)
    {
        std::lock_guard<std::mutex> lock(g_ctx.regular_mtx);
        if (state.thread_index() == 0)
        {
            DoWrite(); // 1 Writer
        }
        else
        {
            DoLightRead(); 
        }
    }
}
BENCHMARK(BM_RegularMutex_Mixed)->ThreadRange(2, 8)->UseRealTime();

static void BM_SharedMutex_Mixed(benchmark::State &state)
{
    g_ctx.setup();
    for (auto _ : state)
    {
        if (state.thread_index() == 0)
        {
            std::unique_lock<std::shared_mutex> lock(g_ctx.shared_mtx);
            DoWrite();
        }
        else
        {
            // Shared lock for all readers
            std::shared_lock<std::shared_mutex> lock(g_ctx.shared_mtx);
            DoLightRead();
        }
    }
}
// Incrementally test 2, 4, and 8 threads to show the scaling curve.
BENCHMARK(BM_SharedMutex_Mixed)->ThreadRange(2, 8)->UseRealTime();

BENCHMARK_MAIN();
