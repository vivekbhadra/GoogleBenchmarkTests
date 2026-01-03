// comparison_shared_vs_regular_mutex_googlebechmark.cpp
// This code compares the performance of std::mutex and std::shared_mutex
// for read-heavy workloads using Google Benchmark.

#include <benchmark/benchmark.h> // Main header for Google Benchmark framework
#include <cmath>                 // For mathematical operations (sin, sqrt)
#include <map>                   // For our shared data structure
#include <mutex>                 // For std::mutex and std::lock_guard
#include <shared_mutex>          // For std::shared_mutex and std::shared_lock
#include <string>
#include <vector>

// --- GLOBAL SHARED DATA ---
// These resources are shared across all benchmark threads.
std::map<int, double> global_data;
std::mutex m_mtx;        // Standard mutual exclusion: only 1 thread at a time.
std::shared_mutex s_mtx; // Read-Write lock: many readers OR 1 writer.

// --- SETUP FUNCTION ---
// Pre-fills the map with data so that we don't measure the cost of
// map insertion during the actual timing loops.
void SetupData()
{
    // Check if empty to ensure we only populate the map once
    if (global_data.empty())
    {
        for (int i = 0; i < 1000; ++i)
        {
            global_data[i] = std::sqrt(i);
        }
    }
}

// --- SIMULATED WORKLOAD ---
// We simulate "real work" (math calculations). Without this, the reading
// happens so fast that the benchmark only measures lock overhead.
void DoHeavyRead(benchmark::State &state)
{
    double total = 0;
    // Loop 50 times to create a "heavy" read operation
    for (int i = 0; i < 50; ++i)
    {
        total += std::sin(global_data[i % 1000]);
    }
    // benchmark::DoNotOptimize prevents the compiler from seeing that 'total'
    // is unused and deleting the entire loop during optimization (-O3).
    benchmark::DoNotOptimize(total);
}

// --- BENCHMARK 1: REGULAR MUTEX ---
// This uses the "exclusive" lock. Even if threads only want to read,
// they must wait for the thread ahead of them to finish.
static void BM_RegularMutex(benchmark::State &state)
{
    SetupData();
    // The 'state' loop is managed by Google Benchmark. It runs until it has
    // collected enough data for a statistically significant result.
    for (auto _ : state)
    {
        // lock_guard locks the mutex on creation and unlocks when it goes out of scope.
        std::lock_guard<std::mutex> lock(m_mtx);
        DoHeavyRead(state);
    }
}
// Registers the benchmark. ThreadRange(1, 8) tells it to run this test
// with 1, 2, 4, and 8 concurrent threads. UseRealTime() shows wall-clock time.
BENCHMARK(BM_RegularMutex)->ThreadRange(1, 8)->UseRealTime();

// --- BENCHMARK 2: SHARED MUTEX ---
// This uses the "shared" lock. Multiple threads can enter this section
// simultaneously as long as no writer is holding the lock.
static void BM_SharedMutex(benchmark::State &state)
{
    SetupData();
    for (auto _ : state)
    {
        // shared_lock allows other threads using shared_lock to access
        // this section at the same time.
        std::shared_lock<std::shared_mutex> lock(s_mtx);
        DoHeavyRead(state);
    }
}
BENCHMARK(BM_SharedMutex)->ThreadRange(1, 8)->UseRealTime();

// This macro creates the main() function and handles all command-line arguments.
BENCHMARK_MAIN();