#include <cassert>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "../queue/mpmc_queue.hh"
#include "../queue/lock_queue.hh"
#include "../queue/spsc_queue.hh"
#include "../queue/two_lock_queue.hh"

std::string init_prefix() {
  std::string tmp = "thread";
  std::string str;
  for (int i = 0; i < 10; ++i) {
    str += tmp;
  }
  str += "_";
  return str;
}

int push_loops = 1000000;
int push_workers = 1;
int pop_loops = 1000000;
int pop_workers = 1;
int buffer_size = 1024;
int vec_length = push_loops * push_workers;
std::mutex mu;
std::string prefix = init_prefix();

template <typename Queue>
void test_queue_int(Queue* q, std::string q_name, int push_loops = 1000000,
                    int push_workers = 1, int pop_loops = 1000000,
                    int pop_workers = 1) {
  assert(push_loops * push_workers == pop_loops * pop_workers);
  int vec_length = push_loops * push_workers;
  std::vector<int> vec = std::vector<int>(vec_length, 0);
  std::atomic_bool start{false};
  std::vector<std::thread> producers;
  std::vector<std::thread> consumers;
  long long produce_duration = 0;
  long long consume_duration = 0;
  auto produce_task = [q, &start, &produce_duration, &push_loops](int id) {
    while (!start) {
    };
    auto begin = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < push_loops; ++i) {
      while (!q->push(std::move(push_loops * id + i))) {
      };
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - begin)
            .count();
    std::unique_lock<std::mutex> lock(mu);
    produce_duration += duration;
  };
  auto consume_task = [q, &start, &consume_duration, pop_loops, &vec]() {
    std::vector<int> tmp(pop_loops);
    while (!start) {
    };
    auto begin = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < pop_loops; ++i) {
      int num;
      //   flog::backoff b;
      while (!q->pop(num)) {
        // std::this_thread::yield();
        // b.snooze();
      };
      tmp[i] = num;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - begin)
            .count();
    std::unique_lock<std::mutex> lock(mu);
    for (size_t i = 0; i < tmp.size(); ++i) {
      vec[tmp[i]] += 1;
    }
    consume_duration += duration;
  };

  for (int i = 0; i < push_workers; ++i) {
    producers.emplace_back(produce_task, i);
  }
  for (int i = 0; i < pop_workers; ++i) {
    consumers.emplace_back(consume_task);
  }

  start = true;

  for (int i = 0; i < push_workers; ++i) {
    producers[i].join();
  }
  for (int i = 0; i < pop_workers; ++i) {
    consumers[i].join();
  }

  for (int i = 0; i < vec_length; ++i) {
    if (vec[i] != 1) {
      std::cerr << q_name << " error! "
                << "vec[" << i << "]"
                << "==" << vec[i] << std::endl;
    }
  }

  std::cout << std::format(
      "{:<15} | {:>8} push ops in {:>5}ms | {:>8} pop ops in "
      "{:>5}ms | {:>3} pushers | {:>3} poppers\n",
      q_name, push_loops * push_workers, produce_duration / push_workers,
      pop_loops * pop_workers, consume_duration / pop_workers, push_workers,
      pop_workers);
}

template <typename Queue>
void test_queue_unique_ptr(Queue* q, std::string q_name,
                           int push_loops = 1000000, int push_workers = 1,
                           int pop_loops = 1000000, int pop_workers = 1) {
  assert(push_loops * push_workers == pop_loops * pop_workers);
  int vec_length = push_loops * push_workers;
  std::vector<int> vec = std::vector<int>(vec_length, 0);
  std::atomic_bool start{false};
  std::vector<std::thread> producers;
  std::vector<std::thread> consumers;
  long long produce_duration = 0;
  long long consume_duration = 0;
  auto produce_task = [q, &start, &produce_duration, &push_loops](int id) {
    while (!start) {
    };
    auto begin = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < push_loops; ++i) {
      auto ptr = std::make_unique<int>(push_loops * id + i);
      while (!q->push(std::move(ptr))) {
      };
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - begin)
            .count();
    std::unique_lock<std::mutex> lock(mu);
    produce_duration += duration;
  };
  auto consume_task = [q, &start, &consume_duration, pop_loops, &vec]() {
    std::vector<int> tmp(pop_loops);
    while (!start) {
    };
    auto begin = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < pop_loops; ++i) {
      std::unique_ptr<int> p;
      while (!q->pop(p)) {
        //   flog::backoff b;
        // std::this_thread::yield();
        // b.snooze();
      };
      tmp[i] = *p;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - begin)
            .count();
    std::unique_lock<std::mutex> lock(mu);
    for (size_t i = 0; i < tmp.size(); ++i) {
      vec[tmp[i]] += 1;
    }
    consume_duration += duration;
  };

  for (int i = 0; i < push_workers; ++i) {
    producers.emplace_back(produce_task, i);
  }
  for (int i = 0; i < pop_workers; ++i) {
    consumers.emplace_back(consume_task);
  }

  start = true;

  for (int i = 0; i < push_workers; ++i) {
    producers[i].join();
  }
  for (int i = 0; i < pop_workers; ++i) {
    consumers[i].join();
  }

  for (int i = 0; i < vec_length; ++i) {
    if (vec[i] != 1) {
      std::cerr << q_name << " error! "
                << "vec[" << i << "]"
                << "==" << vec[i] << std::endl;
    }
  }

  std::cout << std::format(
      "{:<15} | {:>8} push ops in {:>5}ms | {:>8} pop ops in "
      "{:>5}ms | {:>3} pushers | {:>3} poppers\n",
      q_name, push_loops * push_workers, produce_duration / push_workers,
      pop_loops * pop_workers, consume_duration / pop_workers, push_workers,
      pop_workers);
}

template <typename Queue>
void test_queue_string(Queue* q, std::string q_name, int push_loops = 1000000,
                       int push_workers = 1, int pop_loops = 1000000,
                       int pop_workers = 1) {
  assert(push_loops * push_workers == pop_loops * pop_workers);
  int vec_length = push_loops * push_workers;
  std::vector<int> vec = std::vector<int>(vec_length, 0);
  std::atomic_bool start{false};
  std::vector<std::thread> producers;
  std::vector<std::thread> consumers;
  long long produce_duration = 0;
  long long consume_duration = 0;
  auto produce_task = [q, &start, &produce_duration, &push_loops](int id) {
    while (!start) {
    };
    auto begin = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < push_loops; ++i) {
      auto str = prefix + std::to_string(push_loops * id + i);
      while (!q->push(std::move(str))) {
      };
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - begin)
            .count();
    std::unique_lock<std::mutex> lock(mu);
    produce_duration += duration;
  };
  auto consume_task = [q, &start, &consume_duration, pop_loops, &vec]() {
    std::vector<int> tmp(pop_loops);
    while (!start) {
    };
    auto begin = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < pop_loops; ++i) {
      std::string str;
      //   flog::backoff b;
      while (!q->pop(str)) {
        // std::this_thread::yield();
        // b.snooze();
      };
      size_t pos = str.find('_');
      int num = std::stoi(str.substr(pos + 1, str.size() - 1));
      tmp[i] = num;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - begin)
            .count();
    std::unique_lock<std::mutex> lock(mu);
    for (size_t i = 0; i < tmp.size(); ++i) {
      vec[tmp[i]] += 1;
    }
    consume_duration += duration;
  };

  for (int i = 0; i < push_workers; ++i) {
    producers.emplace_back(produce_task, i);
  }
  for (int i = 0; i < pop_workers; ++i) {
    consumers.emplace_back(consume_task);
  }

  start = true;

  for (int i = 0; i < push_workers; ++i) {
    producers[i].join();
  }
  for (int i = 0; i < pop_workers; ++i) {
    consumers[i].join();
  }

  for (int i = 0; i < vec_length; ++i) {
    if (vec[i] != 1) {
      std::cerr << q_name << " error! "
                << "vec[" << i << "]"
                << "==" << vec[i] << std::endl;
    }
  }

  std::cout << std::format(
      "{:<15} | {:>8} push ops in {:>5}ms | {:>8} pop ops in "
      "{:>5}ms | {:>3} pushers | {:>3} poppers\n",
      q_name, push_loops * push_workers, produce_duration / push_workers,
      pop_loops * pop_workers, consume_duration / pop_workers, push_workers,
      pop_workers);
}

void one_test_int(int push_loops = 1000000, int push_workers = 1,
                  int pop_loops = 1000000, int pop_workers = 1) {
  std::cout << std::format(
                   "*************************** {:>4}pushers, {:>4}popers "
                   "***************************",
                   push_workers, pop_workers)
            << std::endl;

  {
    auto q = new flog::LockQueue<int>;
    test_queue_int(q, "LockQueue", push_loops, push_workers, pop_loops,
                   pop_workers);
    delete q;
  }

  {
    auto q = new flog::TwoLockQueue<int>;
    test_queue_int(q, "TwoLockQueue", push_loops, push_workers, pop_loops,
                   pop_workers);
    delete q;
  }

  {
    auto q = new flog::MPMCQueue<int>;
    test_queue_int(q, "MPMCQueue", push_loops, push_workers, pop_loops,
                   pop_workers);
    delete q;
  }

  {
    if (push_workers == 1 && pop_workers == 1) {
      auto q = new flog::SPSCQueue<int>;
      test_queue_int(q, "SPSCQueue", push_loops, push_workers, pop_loops,
                     pop_workers);
      delete q;
    }
  }

  //   q = new flog::UnMPMCQueue<int>;
  //   test_queue_int(q, "UnMPMCQueue", push_loops, push_workers,
  //       pop_loops, pop_workers);
  //   delete q;
}

void one_test_unique_ptr(int push_loops = 1000000, int push_workers = 1,
                         int pop_loops = 1000000, int pop_workers = 1) {
  std::cout << std::format(
                   "*************************** {:>4}pushers, {:>4}popers "
                   "***************************",
                   push_workers, pop_workers)
            << std::endl;
  {
    auto q = new flog::LockQueue<std::unique_ptr<int>>;
    test_queue_unique_ptr(q, "LockQueue", push_loops, push_workers, pop_loops,
                          pop_workers);
    delete q;
  }

  {
    auto q = new flog::TwoLockQueue<std::unique_ptr<int>>;
    test_queue_unique_ptr(q, "TwoLockQueue", push_loops, push_workers,
                          pop_loops, pop_workers);
    delete q;
  }

  {
    auto q = new flog::MPMCQueue<std::unique_ptr<int>>;
    test_queue_unique_ptr(q, "MPMCQueue", push_loops, push_workers,
                          pop_loops, pop_workers);
    delete q;
  }

  {
    if (push_workers == 1 && pop_workers == 1) {
      auto q = new flog::SPSCQueue<std::unique_ptr<int>>;
      test_queue_unique_ptr(q, "SPSCQueue", push_loops, push_workers, pop_loops,
                            pop_workers);
      delete q;
    }
  }

  //   q = new flog::UnMPMCQueue<std::unique_ptr<int>>;
  //   test_queue_unique_ptr(q, "UnMPMCQueue", push_loops, push_workers,
  //       pop_loops, pop_workers);
  //   delete q;
}

void one_test_string(int push_loops = 1000000, int push_workers = 1,
                     int pop_loops = 1000000, int pop_workers = 1) {
  std::cout << std::format(
                   "*************************** {:>4}pushers, {:>4}popers "
                   "***************************",
                   push_workers, pop_workers)
            << std::endl;
  {
    auto q = new flog::LockQueue<std::string>;
    test_queue_string(q, "LockQueue", push_loops, push_workers, pop_loops,
                      pop_workers);
    delete q;
  }

  {
    auto q = new flog::TwoLockQueue<std::string>;
    test_queue_string(q, "TwoLockQueue", push_loops, push_workers, pop_loops,
                      pop_workers);
    delete q;
  }

  {
    auto q = new flog::MPMCQueue<std::string>;
    test_queue_string(q, "MPMCQueue", push_loops, push_workers, pop_loops,
                      pop_workers);
    delete q;
  }

  {
    if (push_workers == 1 && pop_workers == 1) {
      auto q = new flog::SPSCQueue<std::string>;
      test_queue_string(q, "SPSCQueue", push_loops, push_workers, pop_loops,
                        pop_workers);
      delete q;
    }
  }

  //   q = new flog::UnMPMCQueue<std::string>;
  //   test_queue_string(q, "UnMPMCQueue", push_loops, push_workers,
  //       pop_loops, pop_workers);
  //   delete q;
}

int main() {
  std::cout << "*************************** TestInt "
               "***************************"
            << std::endl;
  one_test_int(1000000, 1, 1000000, 1);
  one_test_int(1000000, 2, 1000000, 2);
  one_test_int(1000000, 4, 1000000, 4);
  one_test_int(1000000, 8, 1000000, 8);
  std::cout << "*************************** TestString "
               "***************************"
            << std::endl;
  one_test_string(1000000, 1, 1000000, 1);
  one_test_string(1000000, 2, 1000000, 2);
  one_test_string(1000000, 4, 1000000, 4);
  one_test_string(1000000, 8, 1000000, 8);
  std::cout << "*************************** TestUniquePtr "
               "***************************"
            << std::endl;
  one_test_unique_ptr(1000000, 1, 1000000, 1);
  one_test_unique_ptr(1000000, 2, 1000000, 2);
  one_test_unique_ptr(1000000, 4, 1000000, 4);
  one_test_unique_ptr(1000000, 8, 1000000, 8);
}