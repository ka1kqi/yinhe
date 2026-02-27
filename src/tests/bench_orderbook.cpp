#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

#include "order.hpp"
#include "orderbook.hpp"

class OrderbookBench {
public:
  static void insert_order(Orderbook &ob, Side side, OrderID id, Price price,
                           Quantity qty,
                           orderType type = orderType::GOODTOCANCEL) {
    auto &level = (side == Side::BUY) ? ob.bids_[price] : ob.asks_[price];
    level.push_back(Order(side, id, price, qty, type));
    auto it = std::prev(level.end());
    ob.orders_.insert({id, orderEntry{it}});
  }

  static Trades call_match(Orderbook &ob) { return ob.match(); }

  static void flush_logs(Orderbook &ob) { ob.Logger.flush_log_Dir(); }

  static void print_stats(const char *name, std::vector<int64_t> &latencies) {
    if (latencies.empty())
      return;

    std::sort(latencies.begin(), latencies.end());

    size_t n = latencies.size();
    int64_t sum = 0;
    for (auto v : latencies)
      sum += v;

    double avg = static_cast<double>(sum) / static_cast<double>(n);
    int64_t p50 = latencies[n * 50 / 100];
    int64_t p95 = latencies[n * 95 / 100];
    int64_t p99 = latencies[n * 99 / 100];
    int64_t min_val = latencies.front();
    int64_t max_val = latencies.back();

    double throughput = 0.0;
    if (sum > 0)
      throughput =
          static_cast<double>(n) / (static_cast<double>(sum) / 1e9);

    std::cout << "\n=== " << name << " (" << n << " ops) ===" << std::endl;
    std::cout << "  Throughput:  " << std::fixed << std::setprecision(0)
              << throughput << " ops/sec" << std::endl;
    std::cout << "  Avg:   " << std::fixed << std::setprecision(1) << avg
              << " ns" << std::endl;
    std::cout << "  P50:   " << p50 << " ns" << std::endl;
    std::cout << "  P95:   " << p95 << " ns" << std::endl;
    std::cout << "  P99:   " << p99 << " ns" << std::endl;
    std::cout << "  Min:   " << min_val << " ns" << std::endl;
    std::cout << "  Max:   " << max_val << " ns" << std::endl;
  }

  static void bench_add_order() {
    const int N = 1'000'000;
    Orderbook ob;
    std::vector<int64_t> latencies;
    latencies.reserve(N);

    for (int i = 0; i < N; ++i) {
      Side side = (i % 2 == 0) ? Side::BUY : Side::SELL;
      Price price = 1000 + (i % 200) - 100; /* prices in [900, 1099] */
      Quantity qty = 1 + (i % 50);

      auto t0 = std::chrono::high_resolution_clock::now();
      (void)ob.add_order(side, price, qty, orderType::GOODTOCANCEL);
      auto t1 = std::chrono::high_resolution_clock::now();

      latencies.push_back(
          std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0)
              .count());
    }

    print_stats("add_order", latencies);
    flush_logs(ob);
  }

  static void bench_cancel_order() {
    const int N = 500'000;
    Orderbook ob;

    /* Insert N orders at non-crossing prices (no matches) */
    for (int i = 0; i < N; ++i) {
      Side side = (i % 2 == 0) ? Side::BUY : Side::SELL;
      /* buys at low prices, sells at high prices -> no crossing */
      Price price = (side == Side::BUY) ? (500 - (i % 200))
                                        : (1500 + (i % 200));
      insert_order(ob, side, static_cast<OrderID>(i + 1), price, 10);
    }

    std::vector<int64_t> latencies;
    latencies.reserve(N);

    for (int i = 0; i < N; ++i) {
      OrderID id = static_cast<OrderID>(i + 1);
      auto t0 = std::chrono::high_resolution_clock::now();
      ob.cancel_order(id);
      auto t1 = std::chrono::high_resolution_clock::now();

      latencies.push_back(
          std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0)
              .count());
    }

    print_stats("cancel_order", latencies);
    flush_logs(ob);
  }

  static void bench_match_heavy() {
    const int N = 500'000;
    Orderbook ob;

    /* Insert 500K bids and 500K asks at crossing prices */
    for (int i = 0; i < N; ++i) {
      Price price = 1000 + (i % 100); /* prices in [1000, 1099] */
      Quantity qty = 1 + (i % 10);
      insert_order(ob, Side::BUY, static_cast<OrderID>(i + 1), price, qty);
      insert_order(ob, Side::SELL, static_cast<OrderID>(N + i + 1), price,
                   qty);
    }

    std::cout << "\n  Book size before match: " << ob.get_size() << std::endl;

    auto t0 = std::chrono::high_resolution_clock::now();
    Trades trades = call_match(ob);
    auto t1 = std::chrono::high_resolution_clock::now();

    int64_t total_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
    double total_ms = static_cast<double>(total_ns) / 1e6;
    double per_trade_ns =
        trades.empty() ? 0.0
                       : static_cast<double>(total_ns) /
                             static_cast<double>(trades.size());
    double throughput =
        trades.empty()
            ? 0.0
            : static_cast<double>(trades.size()) /
                  (static_cast<double>(total_ns) / 1e9);

    std::cout << "\n=== match (bulk, " << trades.size() << " trades) ==="
              << std::endl;
    std::cout << "  Total time:      " << std::fixed << std::setprecision(2)
              << total_ms << " ms" << std::endl;
    std::cout << "  Per-trade avg:   " << std::fixed << std::setprecision(1)
              << per_trade_ns << " ns" << std::endl;
    std::cout << "  Throughput:      " << std::fixed << std::setprecision(0)
              << throughput << " trades/sec" << std::endl;
    std::cout << "  Book size after: " << ob.get_size() << std::endl;

    flush_logs(ob);
  }
};

int main() {
  std::cout << "===== Orderbook Benchmark =====" << std::endl;

  OrderbookBench::bench_add_order();
  OrderbookBench::bench_cancel_order();
  OrderbookBench::bench_match_heavy();

  std::cout << "\n===== Benchmark complete. =====" << std::endl;
  return 0;
}
