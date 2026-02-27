# yinhe

A high-performance order-matching engine written in C++17.

## Architecture

- **Orderbook** — Price-time priority matching engine using `std::map` (sorted bid/ask levels) and `std::list` (FIFO per level). Supports Good-To-Cancel (GTC), Fill-Or-Kill (FOK), and Fill-And-Kill order types.
- **Lock-free SPSC logger** — Trades are logged asynchronously via a single-producer/single-consumer ring buffer. The consumer thread spin-polls the queue, avoiding `condition_variable` syscall overhead on the hot path.
- **O(1) cancel** — Orders are indexed by ID in an `unordered_map` pointing directly into the level's linked list, so cancellation is a constant-time erase.

## Performance

Monte Carlo benchmark: 10 runs per scenario, compiled with `-O3 -march=native`. Each run uses a fresh orderbook.

| Metric | add_order (1M ops) | cancel_order (500K ops) | match (455K trades) |
|--------|-------------------|------------------------|-------------------|
| **Throughput** | 2,089,000 ops/sec | 9,255,000 ops/sec | 1,714,000 trades/sec |
| **Avg latency** | 480 ns | 108 ns | 589 ns/trade |
| **P50** | 208 ns | 83 ns | — |
| **P95** | 1,096 ns | 142 ns | — |
| **P99** | 5,312 ns | 200 ns | — |
| **Stddev (throughput)** | 107K | 480K | 157K |

*Measured on Apple M-series. Logger enabled (realistic path including SPSC push).*

## Building

```bash
# Debug
cmake -B build && cmake --build build

# Release (benchmarks)
cmake -B build-release -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS="-O3 -std=c++17 -DNDEBUG -march=native"
cmake --build build-release
```

## Running

```bash
# Tests
./build/bin/test_orderbook
./build/bin/test_orderbook_stress

# Benchmark
./build-release/bin/bench_orderbook
```

## Project Structure

```
src/
  engine/
    orderbook.{hpp,cpp}   — core matching engine
    order.{hpp,cpp}        — order value type
    levelInfo.hpp          — price level aggregation
    tradeUtils/trade.hpp   — trade result type
  common/
    orderLog.hpp           — async SPSC logger
    SPSCQueue.hpp          — lock-free ring buffer
    types.hpp, enums.hpp   — shared type aliases and enums
  tests/
    test_orderbook.cpp     — unit tests
    test_orderbook_stress.cpp — stress / edge-case tests
    bench_orderbook.cpp    — Monte Carlo performance benchmark
  main.cpp                 — demo entry point
```
