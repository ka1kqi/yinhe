#ifndef YINHE_SRC_COMMON_SPSCQUEUE_H
#define YINHE_SRC_COMMON_SPSCQUEUE_H

#include <atomic>
#include <cstddef>
#include <new>
#include <type_traits>

#ifdef __cpp_lib_hardware_interference_size
using std::hardware_destructive_interference_size;
#else
constexpr std::size_t hardware_destructive_interference_size = 64;
#endif

template <typename T, std::size_t Capacity = 8192>
class SPSCQueue {
  static_assert((Capacity & (Capacity - 1)) == 0,
                "Capacity must be a power of two");
  static_assert(std::is_trivially_copyable<T>::value,
                "T must be trivially copyable");

public:
  SPSCQueue() : write_pos_(0), read_pos_(0) {}

  bool try_push(const T &item) {
    const auto w = write_pos_.load(std::memory_order_relaxed);
    const auto next = (w + 1) & kMask;
    if (next == read_pos_.load(std::memory_order_acquire))
      return false; // full
    buffer_[w] = item;
    write_pos_.store(next, std::memory_order_release);
    return true;
  }

  bool try_pop(T &item) {
    const auto r = read_pos_.load(std::memory_order_relaxed);
    if (r == write_pos_.load(std::memory_order_acquire))
      return false; // empty
    item = buffer_[r];
    read_pos_.store((r + 1) & kMask, std::memory_order_release);
    return true;
  }

private:
  static constexpr std::size_t kMask = Capacity - 1;

  T buffer_[Capacity];

  alignas(hardware_destructive_interference_size)
      std::atomic<std::size_t> write_pos_;
  alignas(hardware_destructive_interference_size)
      std::atomic<std::size_t> read_pos_;
};

#endif
