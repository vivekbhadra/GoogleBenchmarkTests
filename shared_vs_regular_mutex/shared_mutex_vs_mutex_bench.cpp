/**
 * @file shared_mutex_vs_mutex_bench.cpp
 * @brief Professional-grade performance comparison between std::mutex and std::shared_mutex.
 * * This benchmark simulates a Single-Writer / Multiple-Reader (SWMR) scenario.
 * It tracks scaling from 2 to 8 threads to demonstrate the throughput
 * advantages of shared locks as the reader-to-writer ratio increases.
 */

#include <benchmark/benchmark.h>
#include <cmath>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <vector>

/**
 * @struct BenchmarkContext
 * @brief Encapsulates shared resources and ensures cache-line isolation.
 * * alignas(64) prevents "False Sharing," ensuring the mutexes do not
 * inhabit the same CPU cache line.
 */
struct BenchmarkContext
{
    std::map<int, double> data;

    /// @brief Mutex for exclusive access testing.
    alignas(64) std::mutex regular_mtx;

    /// @brief Shared mutex for concurrent read testing.
    alignas(64) std::shared_mutex shared_mtx;

    /**
     * @brief Pre-populates the data map once.
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

/// @brief Global context for the benchmark threads.
static BenchmarkContext g_ctx;

/**
 * @brief Simulates a non-trivial read operation.
 */
void DoHeavyRead()
{
    double total = 0;
    for (int i = 0; i < 50; ++i)
    {
        total += std::sin(g_ctx.data[i % 1000]);
    }
    benchmark::DoNotOptimize(total);
}

/**
 * @brief Simulates a data update operation.
 */
void DoWrite()
{
    g_ctx.data[0] += 1.1;
    benchmark::DoNotOptimize(g_ctx.data[0]);
}

/**
 * @brief Benchmark: Regular Mutex with 1 Writer and N-1 Readers.
 * @details Thread 0 is always the writer. All threads are serialized.
 */
static void BM_RegularMutex_Mixed(benchmark::State &state)
{
    g_ctx.setup();
    for (auto _ : state)
    {
        std::lock_guard<std::mutex> lock(g_ctx.regular_mtx);
        if (state.thread_index() == 0)
        {
            DoWrite();
        }
        else
        {
            DoHeavyRead();
        }
    }
}
// Automatically test 2, 4, and 8 threads
BENCHMARK(BM_RegularMutex_Mixed)->ThreadRange(2, 8)->UseRealTime();

/**
 * @brief Benchmark: Shared Mutex with 1 Writer and N-1 Readers.
 * @details Thread 0 is always the writer (exclusive). Others are readers (shared).
 */
static void BM_SharedMutex_Mixed(benchmark::State &state)
{
    g_ctx.setup();
    for (auto _ : state)
    {
        if (state.thread_index() == 0)
        {
            // Exclusive lock for the single writer
            std::unique_lock<std::shared_mutex> lock(g_ctx.shared_mtx);
            DoWrite();
        }
        else
        {
            // Shared lock for the multiple readers
            std::shared_lock<std::shared_mutex> lock(g_ctx.shared_mtx);
            DoHeavyRead();
        }
    }
}
// Automatically test 2, 4, and 8 threads
BENCHMARK(BM_SharedMutex_Mixed)->ThreadRange(2, 8)->UseRealTime();

BENCHMARK_MAIN();