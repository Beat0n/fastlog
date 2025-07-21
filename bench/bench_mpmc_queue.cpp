#include <benchmark/benchmark.h>
#include <fastlog/concurrency/mpmc_queue.h> // Corrected namespace
#include <thread>
#include <vector>

// We will be pushing simple integers for the benchmark.
// The performance of the queue itself should not depend on the object type.
using QueueType = fastlog::concurrency::MPMCQueue<int, 65536>; // Use a larger buffer

// --- SPSC: Single Producer, Single Consumer ---
static void BM_SPSC_Queue(benchmark::State& state) {
    auto queue = std::make_unique<QueueType>();
    int dummy_val = 0;

    // The consumer thread
    std::thread consumer_thread([&]() {
        while (state.KeepRunning()) { // Keep running until the benchmark is over
            int val;
            if (queue->pop(val)) {
                // In a real scenario, do something with val
                benchmark::DoNotOptimize(val);
            }
        }
        // Drain any remaining items after the main loop
        while(queue->pop(dummy_val)) {}
    });

    // The producer (main thread)
    for (auto _ : state) {
        while (!queue->push(1)) {
            // Spin if the queue is full, which is unlikely with a large buffer
            // but good practice for a benchmark.
        }
    }

    state.SetItemsProcessed(state.iterations());

    // Signal consumer to stop and wait for it
    // state.KeepRunning() will become false after the main loop
    consumer_thread.join();
}
BENCHMARK(BM_SPSC_Queue);

// --- MPMC: Multi Producer, Multi Consumer ---
// This function can be used for MPSC, SPMC, and MPMC by varying thread counts.
static void BM_MPMC_Queue(benchmark::State& state) {
    auto queue = std::make_unique<QueueType>();
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    const int num_producers = state.range(0);
    const int num_consumers = state.range(1);

    // Create consumer threads
    for (int i = 0; i < num_consumers; ++i) {
        consumers.emplace_back([&]() {
            while (state.KeepRunning()) {
                int val;
                if (queue->pop(val)) {
                    benchmark::DoNotOptimize(val);
                } else {
                    // Yield to other threads if the queue is empty
                    std::this_thread::yield();
                }
            }
            // Drain remaining items
            int dummy;
            while (queue->pop(dummy)) {}
        });
    }

    // Create producer threads
    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back([&]() {
            while (state.KeepRunning()) {
                if (!queue->push(1)) {
                    // Yield if queue is full
                    std::this_thread::yield();
                }
            }
        });
    }

    // The main thread just waits for the benchmark duration
    for (auto _ : state) {
        // This loop controls the benchmark timing.
        // The actual work is done in the threads.
        // We calculate items_processed based on what consumers can pull.
    }

    // The benchmark framework will stop state.KeepRunning()
    // after the specified duration.

    for (auto& t : producers) t.join();
    for (auto& t : consumers) t.join();

    // NOTE: This item count is an approximation in MPMC, as it's hard to measure
    // precisely without extra synchronization. A better way for MPMC is to
    // measure throughput over time. Google Benchmark does this by default.
}

// Register the MPMC benchmark with different thread configurations
// Args({num_producers, num_consumers})
BENCHMARK(BM_MPMC_Queue)->Args({1, 4});   // SPMC
BENCHMARK(BM_MPMC_Queue)->Args({4, 1});   // MPSC
BENCHMARK(BM_MPMC_Queue)->Args({2, 2});   // MPMC (balanced)
BENCHMARK(BM_MPMC_Queue)->Args({4, 4});   // MPMC (high contention)
BENCHMARK(BM_MPMC_Queue)->Args({8, 8});   // MPMC (very high contention)

// Run the benchmark
BENCHMARK_MAIN();
