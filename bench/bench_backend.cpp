#include <benchmark/benchmark.h>
#include <fastlog/logger.h>
#include <fastlog/sinks/null_sink.h>
// We need to include configuration APIs if they are separate
// Assuming they are in logger.h for now.
// #include <fastlog/config.h>

#include <mutex>

// A simple flag to ensure initialization happens only once.
static std::once_flag g_logger_initialized;

// --- The Benchmark for Full Pipeline Throughput ---

static void BM_FullPipeline(benchmark::State& state) {
  // --- Setup Phase ---
  // Use std::call_once to ensure the logger is configured exactly once
  // across all threads and all iterations of this benchmark.
  std::call_once(g_logger_initialized, []() {
    // In a real test suite, we'd need a `fastlog::clear_sinks()` or similar
    // to ensure a clean state. For this standalone benchmark, it's okay.
    fastlog::details::add_sink<fastlog::sinks::NullSink>();
    fastlog::details::set_level(fastlog::LogLevel::Info);
  });

  // --- Benchmarked Code ---
  // Each thread created by the benchmark framework will run this loop.
  for (auto _ : state) {
    FASTLOG_INFO("User logged in successfully. UserID: {}, SessionID: {}", 12345, "abcdef123456");
  }

  // Track the number of items processed (number of log calls).
  state.SetItemsProcessed(state.iterations());
}

// Register the throughput benchmark with multiple threads.
BENCHMARK(BM_FullPipeline)->ThreadRange(1, 16)->UseRealTime();

// --- The Benchmark for Frontend Latency ---

static void BM_LogCallLatency(benchmark::State& state) {
  // We assume the logger is already initialized by the first benchmark.
  // This is a slight simplification. A fixture would be cleaner if we had
  // a `fastlog::reset()` function.
  std::call_once(g_logger_initialized, []() {
    fastlog::details::add_sink<fastlog::sinks::NullSink>();
    fastlog::details::set_level(fastlog::LogLevel::Info);
  });

  // --- Benchmarked Code ---
  for (auto _ : state) {
    // The benchmark framework will measure the time it takes for this call to return.
    FASTLOG_INFO("A single message for latency test.");
  }
}

// Register the latency benchmark. It typically runs on a single thread.
BENCHMARK(BM_LogCallLatency);

// If you have multiple `bench_*.cpp` files, make sure BENCHMARK_MAIN()
// only appears in one of them, or in a separate `bench_main.cpp`.
BENCHMARK_MAIN();