#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace cs::net {

// 固定容量的单生产者/单消费者队列；两个方向各用一个实例，禁止第三方读写。
// 满时保留原队列并累计丢弃数，不覆盖尚未消费的输入，也不阻塞主线程。
template <class T, std::size_t Capacity> class SpscQueue final {
  static_assert(Capacity > 0 && Capacity < UINT32_MAX / 2);
  static_assert(std::is_trivially_copyable_v<T>);
  static_assert(std::atomic<std::uint32_t>::is_always_lock_free);

public:
  bool TryPush(const T &value) noexcept {
    const auto write = write_.load(std::memory_order_relaxed);
    const auto next = Next(write);
    if (next == read_.load(std::memory_order_acquire)) {
      // 只有生产者修改此计数；达到上限后饱和，避免高包率下回绕为零。
      const auto dropped = dropped_.load(std::memory_order_relaxed);
      if (dropped != UINT32_MAX)
        dropped_.store(dropped + 1, std::memory_order_relaxed);
      return false;
    }
    values_[write] = value;
    write_.store(next, std::memory_order_release);
    return true;
  }
  bool TryPop(T &value) noexcept {
    const auto read = read_.load(std::memory_order_relaxed);
    if (read == write_.load(std::memory_order_acquire))
      return false;
    value = values_[read];
    read_.store(Next(read), std::memory_order_release);
    return true;
  }
  std::uint32_t Dropped() const noexcept {
    return dropped_.load(std::memory_order_relaxed);
  }

private:
  static constexpr std::uint32_t Next(std::uint32_t value) noexcept {
    return value == Capacity ? 0 : value + 1;
  }
  std::array<T, Capacity + 1> values_{};
  alignas(64) std::atomic<std::uint32_t> write_{0};
  alignas(64) std::atomic<std::uint32_t> read_{0};
  std::atomic<std::uint32_t> dropped_{0};
};
} // namespace cs::net
