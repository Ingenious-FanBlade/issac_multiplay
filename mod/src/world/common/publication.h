#pragma once
#include "world/model.h"
#include <array>
#include <string>
namespace cs::world::observation {
// 单主线程生产、单后台分派消费的有界环；发布不加锁，满时独立计损失。
class PublicationQueue final {
public:
  explicit PublicationQueue(BudgetLedger &ledger) noexcept : ledger_(ledger) {}
  ~PublicationQueue();
  bool Push(std::string value) noexcept;
  bool Pop(std::string &value) noexcept;
  std::uint64_t LossSequence() const noexcept { return loss_.load(); }
  void NoteLoss() noexcept { ++loss_; }
  bool Empty() noexcept;
private:
  BudgetLedger &ledger_;
  std::array<std::string,4096> queue_{};
  std::array<std::uint64_t,4096> sizes_{};
  std::atomic<std::uint32_t> head_{0},tail_{0};
  std::atomic<std::uint64_t> bytes_{0};
  std::atomic<std::uint64_t> loss_{0};
};
}
