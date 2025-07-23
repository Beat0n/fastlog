#include <fastlog/concurrency/mpmc_queue.h>  // Ensure correct path
#include <fmt/core.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <set>
#include <source_location>
#include <string>
#include <thread>
#include <vector>

#include "fastlog/log_message.h"

// Test fixture for our MPMCQueue tests
class MPMCQueueTest : public ::testing::Test {
 protected:
  // You can define common setup and teardown logic here if needed.
};

// Test case for high-contention multi-producer, multi-consumer scenario
TEST_F(MPMCQueueTest, HighContentionCorrectness) {
  // Test parameters
  constexpr size_t kNumProducers = 8;
  constexpr size_t kNumConsumers = 8;
  constexpr size_t kItemsPerProducer = 1000000;
  constexpr size_t kTotalItems = kNumProducers * kItemsPerProducer;
  constexpr size_t kQueueSize = 1024;  // A smaller queue size increases contention

  // The queue under test
  fastlog::concurrency::MPMCQueue<int, kQueueSize> queue;

  // --- Producer side ---
  std::vector<std::thread> producers;
  std::atomic<size_t> producer_id_counter{0};

  for (size_t i = 0; i < kNumProducers; ++i) {
    producers.emplace_back([&]() {
      // Each producer gets a unique ID from 0 to kNumProducers-1
      const size_t producer_id = producer_id_counter.fetch_add(1);

      for (size_t j = 0; j < kItemsPerProducer; ++j) {
        // Generate a unique number for each item across all producers.
        // Producer 0 generates 0 to 9999
        // Producer 1 generates 10000 to 19999, etc.
        int unique_value = producer_id * kItemsPerProducer + j;

        // Push until successful (to handle full queue)
        while (!queue.push(unique_value)) {
          std::this_thread::yield();  // Yield to consumers
        }
      }
    });
  }

  // --- Consumer side ---
  std::vector<std::thread> consumers;
  // We need a thread-safe way to collect results from all consumers.
  // A simple mutex-protected vector is fine for a test.
  std::vector<int> consumed_items;
  std::mutex consumed_mutex;

  // A flag to signal consumers to stop when all producers are done
  // and the queue is likely empty.
  std::atomic<bool> producers_finished{false};

  for (size_t i = 0; i < kNumConsumers; ++i) {
    consumers.emplace_back([&]() {
      int item;
      while (!producers_finished.load(std::memory_order_acquire)) {
        if (queue.pop(item)) {
          std::lock_guard<std::mutex> lock(consumed_mutex);
          consumed_items.push_back(item);
        } else {
          std::this_thread::yield();  // Queue is empty, yield
        }
      }
      // After producers are done, drain any remaining items in the queue
      while (queue.pop(item)) {
        std::lock_guard<std::mutex> lock(consumed_mutex);
        consumed_items.push_back(item);
      }
    });
  }

  // Wait for all producers to finish their work
  for (auto &t : producers) {
    t.join();
  }

  // Signal consumers that production is over
  producers_finished.store(true, std::memory_order_release);

  // Wait for all consumers to finish
  for (auto &t : consumers) {
    t.join();
  }

  // --- Verification ---

  // 1. Verify total number of items consumed
  ASSERT_EQ(consumed_items.size(), kTotalItems) << "The number of consumed items does not match the total number of "
                                                   "produced items.";

  // 2. Verify uniqueness and completeness of items
  // We sort the consumed items and check if they form a perfect sequence from 0
  // to kTotalItems-1
  std::sort(consumed_items.begin(), consumed_items.end());

  for (size_t i = 0; i < kTotalItems; ++i) {
    ASSERT_EQ(consumed_items[i], i) << "Mismatch found at index " << i << ". Data corruption or loss occurred.";
  }

  // An alternative check for uniqueness using a set (slower but conceptually
  // clear)
  std::set<int> unique_items(consumed_items.begin(), consumed_items.end());
  ASSERT_EQ(unique_items.size(), kTotalItems) << "Duplicate items were consumed, or some items were lost.";
}

// You can add more tests here, e.g., for move-only types
struct MoveOnlyType {
  int value;
  std::unique_ptr<int> ptr;

  MoveOnlyType(int v) : value(v), ptr(std::make_unique<int>(v)) {}
  MoveOnlyType(const MoveOnlyType &) = delete;
  MoveOnlyType &operator=(const MoveOnlyType &) = delete;
  MoveOnlyType(MoveOnlyType &&) = default;
  MoveOnlyType &operator=(MoveOnlyType &&) = default;
};

TEST_F(MPMCQueueTest, MoveOnlyTypeSupport) {
  fastlog::concurrency::MPMCQueue<MoveOnlyType, 128> queue;

  // Test emplace
  ASSERT_TRUE(queue.emplace(10));

  // Test push with rvalue
  ASSERT_TRUE(queue.push(MoveOnlyType(20)));

  MoveOnlyType result(0);

  ASSERT_TRUE(queue.pop(result));
  EXPECT_EQ(result.value, 10);
  ASSERT_NE(result.ptr, nullptr);
  EXPECT_EQ(*result.ptr, 10);

  ASSERT_TRUE(queue.pop(result));
  EXPECT_EQ(result.value, 20);
  ASSERT_NE(result.ptr, nullptr);
  EXPECT_EQ(*result.ptr, 20);

  ASSERT_FALSE(queue.pop(result));
}

// Test case for concurrent push/pop of std::string
// This test is designed to stress the move semantics and SSO (Small String Optimization)
// of std::string within our lock-free queue.
TEST_F(MPMCQueueTest, StringStressTest) {
  constexpr size_t kNumProducers = 4;
  constexpr size_t kNumConsumers = 4;
  constexpr size_t kItemsPerProducer = 5000;
  constexpr size_t kTotalItems = kNumProducers * kItemsPerProducer;
  constexpr size_t kQueueSize = 1024;

  fastlog::concurrency::MPMCQueue<std::string, kQueueSize> queue;

  std::atomic<bool> producers_finished{false};
  std::atomic<size_t> items_produced{0};

  // --- Producers ---
  std::vector<std::thread> producers;
  for (size_t i = 0; i < kNumProducers; ++i) {
    producers.emplace_back([&, producer_id = i]() {
      for (size_t j = 0; j < kItemsPerProducer; ++j) {
        // Generate a mix of short (SSO) and long strings
        std::string value;
        if (j % 2 == 0) {
          // Short string, likely to use SSO
          value = fmt::format("P{}-{}", producer_id, j);
        } else {
          // Long string, guaranteed to be on the heap
          value = fmt::format("Producer {} - Item {} - This is a long string to prevent small string optimization.", producer_id, j);
        }

        while (!queue.push(std::move(value))) {
          std::this_thread::yield();
        }
      }
      // Atomically signal that this producer has finished.
      items_produced.fetch_add(kItemsPerProducer, std::memory_order_release);
    });
  }

  // --- Consumers ---
  std::vector<std::thread> consumers;
  std::vector<std::string> consumed_items;
  std::mutex consumed_mutex;

  for (size_t i = 0; i < kNumConsumers; ++i) {
    consumers.emplace_back([&]() {
      std::string item;
      // Keep consuming as long as producers are running OR there are still items to be produced.
      // This is a robust way to ensure we drain the queue completely.
      while (items_produced.load(std::memory_order_acquire) < kTotalItems || !queue.is_empty()) {  // Assuming is_empty() exists
        if (queue.pop(item)) {
          std::lock_guard<std::mutex> lock(consumed_mutex);
          consumed_items.push_back(std::move(item));
        } else {
          // If producers are done and queue is empty, we might still be in a race.
          // A small yield helps prevent busy-spinning.
          std::this_thread::yield();
        }
      }
    });
  }

  for (auto &t : producers) {
    t.join();
  }
  // No need to set `producers_finished` flag anymore, the atomic counter is better.

  for (auto &t : consumers) {
    t.join();
  }

  // --- Verification ---

  // 1. Verify total count
  ASSERT_EQ(consumed_items.size(), kTotalItems);

  // 2. Verify content
  // We sort the strings to make verification deterministic.
  std::sort(consumed_items.begin(), consumed_items.end());

  // Generate the expected list of strings
  std::vector<std::string> expected_items;
  for (size_t i = 0; i < kNumProducers; ++i) {
    for (size_t j = 0; j < kItemsPerProducer; ++j) {
      if (j % 2 == 0) {
        expected_items.push_back(fmt::format("P{}-{}", i, j));
      } else {
        expected_items.push_back(fmt::format("Producer {} - Item {} - This is a long string to prevent small string optimization.", i, j));
      }
    }
  }
  std::sort(expected_items.begin(), expected_items.end());

  ASSERT_EQ(consumed_items, expected_items);
}

// Test case for concurrent push/pop of the actual LogMessage struct.
// This is a direct test of the core data transport mechanism of our logger.
TEST_F(MPMCQueueTest, LogMessageStressTest) {
  constexpr size_t kNumProducers = 4;
  constexpr size_t kItemsPerProducer = 5000;
  constexpr size_t kTotalItems = kNumProducers * kItemsPerProducer;
  constexpr size_t kQueueSize = 1024;

  fastlog::concurrency::MPMCQueue<fastlog::LogMessage, kQueueSize> queue;

  std::atomic<size_t> items_consumed{0};
  std::atomic<bool> test_failed{false};

  // --- Consumer Thread ---
  // We start the consumer first.
  std::thread consumer_thread([&]() {
    size_t consumed_count = 0;
    while (consumed_count < kTotalItems) {
      fastlog::LogMessage msg;
      if (queue.pop(msg)) {
        // --- Verification inside the consumer ---
        // Check if the payload string contains the expected prefix.
        // This will crash if `msg` or `msg.payload` is corrupted.
        if (msg.payload.find("Message from P") == std::string::npos) {
          test_failed.store(true);
        }
        // Check if level is within a valid range
        if (msg.level > fastlog::LogLevel::Fatal) {
          test_failed.store(true);
        }

        consumed_count++;
      } else {
        std::this_thread::yield();
      }
    }
    items_consumed.store(consumed_count);
  });

  // --- Producer Threads ---
  std::vector<std::thread> producers;
  for (size_t i = 0; i < kNumProducers; ++i) {
    producers.emplace_back([&, producer_id = i]() {
      for (size_t j = 0; j < kItemsPerProducer; ++j) {
        std::source_location loc;
        fastlog::LogMessage msg{.timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()),
                                .thread_id = std::this_thread::get_id(),
                                .location = loc,
                                .level = static_cast<fastlog::LogLevel>(j % 6),
                                .payload = fmt::format("Message from P{}-{}", producer_id, j)};

        while (!queue.push(std::move(msg))) {
          std::this_thread::yield();
        }
      }
    });
  }

  for (auto &t : producers) {
    t.join();
  }

  consumer_thread.join();

  // --- Final Verification ---
  ASSERT_FALSE(test_failed.load()) << "Data corruption detected in consumer thread.";
  ASSERT_EQ(items_consumed.load(), kTotalItems) << "Mismatch in the number of consumed items.";
}