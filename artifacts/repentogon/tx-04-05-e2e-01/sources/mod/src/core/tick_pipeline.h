#pragma once

#include "core/participant.h"
#include "core/session.h"
#include "core/tick_clock.h"

namespace cs::core {

// 统计所有 admission 尝试，包括重复和非法命令，不能用无效输入绕过有界 drain。
inline constexpr std::uint32_t kTickDrainAttemptBudget =
    2 + 4 * input::kDrainBudgetPerTick;
inline constexpr std::uint32_t kMaximumAdmissionAttemptBudget =
    input::kRuntimeAdmissionAttemptMaximum;

enum class TickPipelinePhase : std::uint32_t {
  Idle,
  Draining,
  Simulating,
  AwaitPostState,
  Committed,
  Published,
  Failed
};
// 本轮只保留收束阶段，不包含世界采集、gap、capture 或 post-state 发布路径。
enum class PostStateStatus : std::uint32_t { Disabled };
struct TickPostState {
  PostStateStatus status = PostStateStatus::Disabled;
};
// 只记录存在的参与者；Solo 的 clients[1] 全零且 present=false，不表示缺输入的
// B。
struct TickClientReceipt {
  std::uint64_t client = 0, generation = 0, processed_generation = 0;
  std::uint64_t accepted = 0, processed = 0, processed_tick = 0, source = 0;
  bool current_binding = false, present = false;
  ClientInputAvailability availability{};
};
struct TickReceipt {
  std::uint64_t tick = 0, loop = 0;
  TickKind kind = TickKind::StandardRoom;
  TickBoundary boundary{};
  TickPostState post{};
  std::array<TickClientReceipt, 2> clients{};
  std::size_t participant_count = 0;
  bool published = false;
};
struct TickPipelineStatus {
  TickPipelinePhase phase;
  std::uint64_t candidate;
  std::uint32_t drained, budget;
};
const char *TickPipelinePhaseName(TickPipelinePhase phase) noexcept;

// 主线程协调 Session/Clock/Participant，不调用原生函数、不读取游戏对象。
// 原生正常返回时 End 提交 previous；TopExit 的 FinishPostState 提交 completed，
// PublishCompleted 只发布回执。停止接收输入不影响已经开始的窗口正常收尾。
class TickPipeline final {
public:
  TickPipeline(Session &session, TickClock &clock,
               std::array<Participant, 2> &players,
               std::uint32_t budget = kTickDrainAttemptBudget) noexcept
      : session_(session), clock_(clock), players_(players), budget_(budget) {}

  // 首个 drain 前配置一次；参与者配置由 owner 在 Initialize
  // 前使用相同配置设置。
  bool Configure(const InputRuntimeConfig &config) noexcept;
  TickDecision BeginDrain(const TickBoundary &boundary, bool blocked) noexcept;
  Decision AcceptInput(std::uint32_t ordinal, const InputCmdDraftV2 &command,
                       std::uint64_t generation) noexcept;
  Decision AcceptInput(std::uint32_t ordinal,
                       std::span<const std::uint8_t> bytes,
                       std::uint64_t generation) noexcept;
  TickDecision BeginSimulation() noexcept;
  TickDecision NativeReturned(const TickBoundary &boundary,
                              bool normal) noexcept;
  TickDecision FinishPostState(std::uint64_t loop,
                               const TickPostState &observed = {}) noexcept;
  bool PublishCompleted(bool success) noexcept;
  TickDecision EndOuter(std::uint64_t loop) noexcept;
  TickDecision Abort(TickDecision reason) noexcept;
  std::uint64_t SimulationTick() const noexcept {
    return phase_ == TickPipelinePhase::Simulating ? clock_.Current() : 0;
  }
  TickPipelineStatus Status() const noexcept {
    return {phase_, candidate_, drained_, budget_};
  }
  std::uint32_t RemainingAdmissionAttempts() const noexcept {
    return phase_ == TickPipelinePhase::Draining && drained_ < budget_
               ? budget_ - drained_
               : 0;
  }
  const TickReceipt &Receipt() const noexcept { return receipt_; }

private:
  bool Admit(std::uint32_t ordinal) noexcept;
  Decision AfterAdmission(std::uint32_t ordinal, Decision decision) noexcept;
  Session &session_;
  TickClock &clock_;
  std::array<Participant, 2> &players_;
  std::uint32_t budget_, drained_ = 0;
  std::uint64_t candidate_ = 0;
  bool configured_ = false;
  TickBoundary boundary_{};
  TickPipelinePhase phase_ = TickPipelinePhase::Idle;
  TickReceipt receipt_{};
};

} // namespace cs::core
