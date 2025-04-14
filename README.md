# FASTLOG-C++高性能无锁日志库

FLOG 是一个高性能的 C++ 日志库，旨在提供低延迟、高吞吐量的日志记录功能，特别适用于多线程应用程序。它使用了本地线程日志（`LocalLogger`）和全局日志（`GlobalLogger`）的双层缓冲设计，结合高效的无锁队列机制，优化日志的写入速度和并发性能。通过引入 backoff 机制来减少自旋等待次数，进一步提高了系统的吞吐量和响应性。

## 特性

* **高性能** ：使用了无锁队列（`SPSCQueue` 和 `MPMCQueue`）和内存对齐优化，确保在多线程环境中进行高效的日志写入。
* **低延迟** ：通过本地线程缓存日志项，并将日志格式化延迟到刷屏时，提升了整体性能。
* **Backoff机制** ：引入了自旋退避（backoff）算法，避免过度自旋导致的 CPU 浪费，在队列满或空时降低 CPU 的占用。
* **易用的接口** ：提供简洁的宏接口，方便在代码中记录日志。
* **灵活的日志级别设置** ：支持按日志级别过滤，用户可以根据需要动态设置不同的日志级别。

## 架构

* **本地日志记录器（`LocalLogger`）** ：每个线程有一个独立的本地日志记录器，使用单生产者单消费者队列（SPSCQueue）缓存日志，并由一个对应的后台线程写入全局日志记录器（GlobalLogger）。
* **全局日志记录器（`GlobalLogger`）** ：全局日志记录器负责将本地日志批量写入文件。使用多生产者多消费者队列（MPMCQueue）确保高效的日志传输。
* **自旋退避机制** ：通过引入 `backoff` 类，减少队列满时的自旋次数，从而减少无效的 CPU 占用。

## 使用方法

### 设置日志级别

```cpp
SET_LOG_LEVEL_DEBUG;  // 设置日志级别为 DEBUG
SET_LOG_LEVEL_INFO;   // 设置日志级别为 Info
SET_LOG_LEVEL_WARN;   // 设置日志级别为 WARN
SET_LOG_LEVEL_ERROR;  // 设置日志级别为 ERROR
SET_LOG_LEVEL_FATAL;  // 设置日志级别为 FATAL
```

### 记录日志

```c++
FLOG_DEBUG("This is a debug message");
FLOG_INFO("This is an info message");
FLOG_WARN("This is a warning message");
FLOG_ERROR("This is an error message");
FLOG_FATAL("This is a fatal message");
```

## 性能测试

### 无锁队列性能测试

```
xmake build benchmark_queue && xmake run benchmark_queue
```

## TODO

1. 引入config模块进行logger初始化
2. 自定义日志格式
3. 日志定时切割、压缩
