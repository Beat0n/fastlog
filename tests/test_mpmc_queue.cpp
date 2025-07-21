#include <algorithm>
#include <atomic>
#include <fastlog/concurrency/mpmc_queue.h> // Ensure correct path
#include <gtest/gtest.h>
#include <set>
#include <thread>
#include <vector>

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
  constexpr size_t kQueueSize =
      1024; // A smaller queue size increases contention

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
          std::this_thread::yield(); // Yield to consumers
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
          std::this_thread::yield(); // Queue is empty, yield
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
  ASSERT_EQ(consumed_items.size(), kTotalItems)
      << "The number of consumed items does not match the total number of "
         "produced items.";

  // 2. Verify uniqueness and completeness of items
  // We sort the consumed items and check if they form a perfect sequence from 0
  // to kTotalItems-1
  std::sort(consumed_items.begin(), consumed_items.end());

  for (size_t i = 0; i < kTotalItems; ++i) {
    ASSERT_EQ(consumed_items[i], i) << "Mismatch found at index " << i
                                    << ". Data corruption or loss occurred.";
  }

  // An alternative check for uniqueness using a set (slower but conceptually
  // clear)
  std::set<int> unique_items(consumed_items.begin(), consumed_items.end());
  ASSERT_EQ(unique_items.size(), kTotalItems)
      << "Duplicate items were consumed, or some items were lost.";
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