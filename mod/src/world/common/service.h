#pragma once
#include "world/common/contracts.h"
#include <array>
#include <optional>
#include <atomic>
namespace cs::world::observation {
// 单一主线程协调器；采集 epoch 和录制 generation 无关，未提交的普通后态不能发布。
class Service final {
public:
  RequestResult ActivateObservation(std::uint64_t session, std::uint64_t epoch) noexcept;
  RequestResult AttachConsumer(Consumer consumer) noexcept;
  RequestResult DetachConsumer(std::uint32_t id) noexcept;
  RequestResult UpdateCapturePlan(std::uint32_t id, CapturePlan plan) noexcept;
  const CapturePlan &Plan() const noexcept { return plan_; }
  bool Active() const noexcept { return active_; }
  bool Needed(DomainId, Phase) const noexcept;
  ObservationContext BeginScope(Phase, std::uint32_t source, std::uint64_t parent=0) noexcept;
  RequestResult PrepareScope(ObservationEnvelope envelope) noexcept;
  RequestResult CommitScope(std::uint64_t scope, Outcome outcome) noexcept;
  void Invalidate(const char *reason) noexcept;
  void RequestStop() noexcept;
  std::uint64_t Published() const noexcept { return published_.load(std::memory_order_acquire); }
  std::uint64_t LossSequence() const noexcept { return loss_.load(std::memory_order_acquire); }
  std::uint64_t Epoch() const noexcept { return epoch_; }
  // 关联空间使用同一分配 owner；Entity/Grid 的 local_id 仍由现有身份表唯一分配。
  SubjectKey AllocateRelatedSubject(IdentitySpace space) noexcept;
private:
  void MergePlan();
  void Publish(const ObservationEnvelope &) noexcept;
  std::array<Consumer,4> consumers_{};
  std::array<std::optional<ObservationEnvelope>,12> pending_{};
  CapturePlan plan_{};
  std::uint64_t session_=0, epoch_=0, scope_=0, sequence_=0;
  std::atomic<std::uint64_t> published_{0}, loss_{0};
  bool active_=false, publishing_=false;
  std::array<std::uint64_t,15> related_ids_{};
};
}
