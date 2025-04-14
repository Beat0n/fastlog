
#include "../logger/logger.hh"
#include <thread>
#include <vector>

int main() {
  // std::cout << flog::level2string(flog::LogLevel::Debug) << std::endl;
  std::vector<std::thread> threads;
  for(int i=0; i<16; ++i) {
    threads.emplace_back([i] {
      for (int j = 0; j < 1; ++j) {
        FLOG_INFO(std::format("Hello, world! thread_{}: {}", i, j));
      }
    });
  }
  for (auto &thread : threads) {
    thread.join();
  }
}