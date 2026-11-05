#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <memory>
#include <mutex>
#include <deque>
#include <optional>

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
  std::uint32_t Dropped() const noexcept { return dropped_.load(std::memory_order_relaxed); }

private:
  static constexpr std::uint32_t Next(std::uint32_t value) noexcept {
    return value == Capacity ? 0 : value + 1;
  }
  std::array<T, Capacity + 1> values_{};
  alignas(64) std::atomic<std::uint32_t> write_{0};
  alignas(64) std::atomic<std::uint32_t> read_{0};
  std::atomic<std::uint32_t> dropped_{0};
};
// 自有不可变值的单生产者/单消费者队列；pop清空槽位，及时归还共享租约。
template <class T, std::size_t Capacity> class OwnedQueue final {
public:
  bool TryPush(T value) noexcept {
    const auto write = write_.load(std::memory_order_relaxed), next = (write + 1) % (Capacity + 1);
    if (next == read_.load(std::memory_order_acquire))
      return false;
    values_[write] = std::move(value);
    write_.store(next, std::memory_order_release);
    return true;
  }
  bool TryPop(T &value) noexcept {
    const auto read = read_.load(std::memory_order_relaxed);
    if (read == write_.load(std::memory_order_acquire))
      return false;
    value = std::move(*values_[read]);
    values_[read].reset();
    read_.store((read + 1) % (Capacity + 1), std::memory_order_release);
    return true;
  }

  // 仅用于聚合诊断的近似数量，不作为同步接纳条件。
  std::size_t Size() const noexcept {const auto w=write_.load(std::memory_order_acquire),r=read_.load(std::memory_order_acquire);return (w+Capacity+1-r)%(Capacity+1);}

private:
  std::array<std::optional<T>, Capacity + 1> values_{};
  alignas(64) std::atomic<std::size_t> write_{0}, read_{0};
};
// 仅保留尚未取出的最新完整值；consumer取走后独占正在处理项，生产者不能覆盖它。
template <class T> class Latest final {
public:
  void Put(std::shared_ptr<const T> value) noexcept {
    value_.store(std::move(value), std::memory_order_release);
  }
  std::shared_ptr<const T> Take() noexcept {
    return value_.exchange({}, std::memory_order_acq_rel);
  }

private:
  std::atomic<std::shared_ptr<const T>> value_;
};
} // namespace cs::net
