#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

#include "order.hpp"
#include "orderbook.hpp"

static constexpr int MONTE_CARLO_RUNS = 10;

struct RunStats {
  double throughput;
  double avg_ns;
  int64_t p50;
  int64_t p95;
  int64_t p99;
  int64_t min_val;
  int64_t max_val;
};

struct MatchRunStats {
  double throughput;
  double total_ms;
  double per_trade_ns;
  size_t num_trades;
};

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

  static RunStats compute_stats(std::vector<int64_t> &latencies) {
    std::sort(latencies.begin(), latencies.end());
    size_t n = latencies.size();
    int64_t sum = 0;
    for (auto v : latencies)
      sum += v;

    RunStats s{};
    s.avg_ns = static_cast<double>(sum) / static_cast<double>(n);
    s.p50 = latencies[n * 50 / 100];
    s.p95 = latencies[n * 95 / 100];
    s.p99 = latencies[n * 99 / 100];
    s.min_val = latencies.front();
    s.max_val = latencies.back();
    s.throughput = (sum > 0)
                       ? static_cast<double>(n) / (static_cast<double>(sum) / 1e9)
                       : 0.0;
    return s;
  }

  static void print_summary(const char *name, int ops_per_run,
                             const std::vector<RunStats> &runs) {
    size_t n = runs.size();
    double sum_throughput = 0, sum_avg = 0;
    double sum_p50 = 0, sum_p95 = 0, sum_p99 = 0;

    std::vector<double> throughputs;
    throughputs.reserve(n);

    for (auto &r : runs) {
      sum_throughput += r.throughput;
      sum_avg += r.avg_ns;
      sum_p50 += r.p50;
      sum_p95 += r.p95;
      sum_p99 += r.p99;
      throughputs.push_back(r.throughput);
    }

    double mean_tp = sum_throughput / n;
    double variance = 0;
    for (auto t : throughputs)
      variance += (t - mean_tp) * (t - mean_tp);
    double stddev_tp = std::sqrt(variance / n);

    std::cout << "\n=== " << name << " (" << ops_per_run << " ops x " << n
              << " runs) ===" << std::endl;
    std::cout << "  Throughput:  " << std::fixed << std::setprecision(0)
              << mean_tp << " ops/sec  (stddev: " << stddev_tp << ")"
              << std::endl;
    std::cout << "  Avg:   " << std::fixed << std::setprecision(1)
              << sum_avg / n << " ns" << std::endl;
    std::cout << "  P50:   " << std::fixed << std::setprecision(0)
              << sum_p50 / n << " ns" << std::endl;
    std::cout << "  P95:   " << std::fixed << std::setprecision(0)
              << sum_p95 / n << " ns" << std::endl;
    std::cout << "  P99:   " << std::fixed << std::setprecision(0)
              << sum_p99 / n << " ns" << std::endl;
  }

  static void print_match_summary(const std::vector<MatchRunStats> &runs) {
    size_t n = runs.size();
    double sum_tp = 0, sum_ms = 0, sum_per = 0;
    std::vector<double> throughputs;
    throughputs.reserve(n);

    for (auto &r : runs) {
      sum_tp += r.throughput;
      sum_ms += r.total_ms;
      sum_per += r.per_trade_ns;
      throughputs.push_back(r.throughput);
    }

    double mean_tp = sum_tp / n;
    double variance = 0;
    for (auto t : throughputs)
      variance += (t - mean_tp) * (t - mean_tp);
    double stddev_tp = std::sqrt(variance / n);

    std::cout << "\n=== match (bulk, " << runs[0].num_trades << " trades x "
              << n << " runs) ===" << std::endl;
    std::cout << "  Throughput:      " << std::fixed << std::setprecision(0)
              << mean_tp << " trades/sec  (stddev: " << stddev_tp << ")"
              << std::endl;
    std::cout << "  Total time:      " << std::fixed << std::setprecision(2)
              << sum_ms / n << " ms" << std::endl;
    std::cout << "  Per-trade avg:   " << std::fixed << std::setprecision(1)
              << sum_per / n << " ns" << std::endl;
  }

  static std::vector<RunStats> bench_add_order() {
    const int N = 1'000'000;
    std::vector<RunStats> runs;

    for (int run = 0; run < MONTE_CARLO_RUNS; ++run) {
      Orderbook ob;
      std::vector<int64_t> latencies;
      latencies.reserve(N);

      for (int i = 0; i < N; ++i) {
        Side side = (i % 2 == 0) ? Side::BUY : Side::SELL;
        Price price = 1000 + (i % 200) - 100;
        Quantity qty = 1 + (i % 50);

        auto t0 = std::chrono::high_resolution_clock::now();
        (void)ob.add_order(side, price, qty, orderType::GOODTOCANCEL);
        auto t1 = std::chrono::high_resolution_clock::now();

        latencies.push_back(
            std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0)
                .count());
      }

      runs.push_back(compute_stats(latencies));
      flush_logs(ob);
      std::cout << "  [add_order] run " << (run + 1) << "/" << MONTE_CARLO_RUNS
                << ": " << std::fixed << std::setprecision(0)
                << runs.back().throughput << " ops/sec" << std::endl;
    }

    print_summary("add_order", N, runs);
    return runs;
  }

  static std::vector<RunStats> bench_cancel_order() {
    const int N = 500'000;
    std::vector<RunStats> runs;

    for (int run = 0; run < MONTE_CARLO_RUNS; ++run) {
      Orderbook ob;

      for (int i = 0; i < N; ++i) {
        Side side = (i % 2 == 0) ? Side::BUY : Side::SELL;
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

      runs.push_back(compute_stats(latencies));
      flush_logs(ob);
      std::cout << "  [cancel_order] run " << (run + 1) << "/"
                << MONTE_CARLO_RUNS << ": " << std::fixed
                << std::setprecision(0) << runs.back().throughput << " ops/sec"
                << std::endl;
    }

    print_summary("cancel_order", N, runs);
    return runs;
  }

  static std::vector<MatchRunStats> bench_match_heavy() {
    const int N = 500'000;
    std::vector<MatchRunStats> runs;

    for (int run = 0; run < MONTE_CARLO_RUNS; ++run) {
      Orderbook ob;

      for (int i = 0; i < N; ++i) {
        Price price = 1000 + (i % 100);
        Quantity qty = 1 + (i % 10);
        insert_order(ob, Side::BUY, static_cast<OrderID>(i + 1), price, qty);
        insert_order(ob, Side::SELL, static_cast<OrderID>(N + i + 1), price,
                     qty);
      }

      auto t0 = std::chrono::high_resolution_clock::now();
      Trades trades = call_match(ob);
      auto t1 = std::chrono::high_resolution_clock::now();

      int64_t total_ns =
          std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0)
              .count();

      MatchRunStats s{};
      s.num_trades = trades.size();
      s.total_ms = static_cast<double>(total_ns) / 1e6;
      s.per_trade_ns = trades.empty()
                           ? 0.0
                           : static_cast<double>(total_ns) /
                                 static_cast<double>(trades.size());
      s.throughput = trades.empty()
                         ? 0.0
                         : static_cast<double>(trades.size()) /
                               (static_cast<double>(total_ns) / 1e9);

      runs.push_back(s);
      flush_logs(ob);
      std::cout << "  [match] run " << (run + 1) << "/" << MONTE_CARLO_RUNS
                << ": " << std::fixed << std::setprecision(0) << s.throughput
                << " trades/sec" << std::endl;
    }

    print_match_summary(runs);
    return runs;
  }
};

int main() {
  std::cout << "===== Orderbook Benchmark (Monte Carlo, " << MONTE_CARLO_RUNS
            << " runs) =====" << std::endl;

  auto add_runs = OrderbookBench::bench_add_order();
  auto cancel_runs = OrderbookBench::bench_cancel_order();
  auto match_runs = OrderbookBench::bench_match_heavy();

  std::cout << "\n===== Benchmark complete. =====" << std::endl;
  return 0;
}
