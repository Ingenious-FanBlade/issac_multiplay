#include "core/tick_pipeline.h"

namespace cs::core {

const char *TickPipelinePhaseName(TickPipelinePhase phase) noexcept {
  switch (phase) {
  case TickPipelinePhase::Idle:
    return "Idle";
  case TickPipelinePhase::Draining:
    return "Draining";
  case TickPipelinePhase::Simulating:
    return "Simulating";
  case TickPipelinePhase::AwaitPostState:
    return "AwaitPostState";
  case TickPipelinePhase::Committed:
    return "Committed";
  case TickPipelinePhase::Published:
    return "Published";
  case TickPipelinePhase::Failed:
    return "Failed";
  }
  return "UNKNOWN";
}
bool TickPipeline::Configure(const InputRuntimeConfig &config) noexcept {
  if (configured_ || phase_ != TickPipelinePhase::Idle)
    return false;
  if (ValidateInputRuntimeConfig(config) != InputConfigDecision::Accepted)
    return false;
  if (!clock_.ConfigureFutureWindow(config.future_window_ticks))
    return false;
  budget_ = config.admission_attempt_budget;
  configured_ = true;
  return true;
}
// 中止不提交 previous，不伪造 completed；原生函数是否调用仍由 wrapper
// 唯一决定。
TickDecision TickPipeline::Abort(TickDecision reason) noexcept {
  for (std::size_t i = 0; i < session_.ParticipantCount(); ++i)
    players_[i].End(false);
  clock_.Cancel(reason);
  phase_ = TickPipelinePhase::Failed;
  if (session_.AllowsBinding() || session_.AllowsCompletion())
    session_.Fail(SessionCause::RuntimeFailure);
  return reason;
}
TickDecision TickPipeline::BeginDrain(const TickBoundary &boundary,
                                      bool blocked) noexcept {
  if (phase_ == TickPipelinePhase::Failed)
    return TickDecision::Closed;
  if (phase_ != TickPipelinePhase::Idle &&
      phase_ != TickPipelinePhase::Published) {
    // 原生已返回、尚待 TopExit 时再次进入同轮入口，属于 DuplicateLoop。
    if (boundary.loop == boundary_.loop &&
        (phase_ == TickPipelinePhase::AwaitPostState ||
         phase_ == TickPipelinePhase::Committed))
      return Abort(TickDecision::DuplicateLoop);
    return Abort(TickDecision::Reentry);
  }
  const auto decision =
      clock_.CheckStart(boundary, session_.AcceptsInput(), blocked);
  if (decision == TickDecision::Inactive || decision == TickDecision::Blocked ||
      decision == TickDecision::Closed)
    return decision;
  if (decision != TickDecision::Started)
    return Abort(decision);
  if (!budget_ || budget_ > kMaximumAdmissionAttemptBudget)
    return Abort(TickDecision::DrainBudgetExceeded);
  // 只遍历当前模式的参与者；Host 的 A/B 共用检查，Solo 不触碰不存在的 B。
  for (std::size_t i = 0; i < session_.ParticipantCount(); ++i) {
    if (!players_[i].BelongsTo(session_, static_cast<std::uint32_t>(i)) ||
        players_[i].Retired() ||
        players_[i].RuntimeConfig().future_window_ticks !=
            clock_.FutureWindow())
      return Abort(TickDecision::InvalidSource);
  }
  boundary_ = boundary;
  candidate_ = clock_.Completed() + 1;
  drained_ = 0;
  receipt_ = {};
  phase_ = TickPipelinePhase::Draining;
  return TickDecision::Started;
}
bool TickPipeline::Admit(std::uint32_t ordinal) noexcept {
  if (phase_ != TickPipelinePhase::Draining ||
      !session_.HasParticipant(ordinal)) {
    Abort(TickDecision::Reentry);
    return false;
  }
  if (drained_ >= budget_) {
    Abort(TickDecision::DrainBudgetExceeded);
    return false;
  }
  ++drained_;
  return true;
}
// 任一参与者的 sequence 冲突都中止整个会话；两种 admission 重载共用处理，
// 不留下 B 已退休、A 仍推进的窗口，也不给任一 ordinal 单独的冲突旁路。
Decision TickPipeline::AfterAdmission(Decision decision) noexcept {
  if (decision == Decision::SequenceConflict)
    Abort(TickDecision::Cancelled);
  return decision;
}
Decision TickPipeline::AcceptInput(std::uint32_t ordinal,
                                   const InputCmdDraftV2 &command,
                                   std::uint64_t generation) noexcept {
  if (!Admit(ordinal))
    return Decision::Invalid;
  return AfterAdmission(players_[ordinal].AcceptInput(
      command, generation, clock_.Completed()));
}
Decision TickPipeline::AcceptInput(std::uint32_t ordinal,
                                   std::span<const std::uint8_t> bytes,
                                   std::uint64_t generation) noexcept {
  if (!Admit(ordinal))
    return Decision::Invalid;
  return AfterAdmission(players_[ordinal].AcceptInput(
      bytes, generation, clock_.Completed()));
}
// drain 完成才预留时钟 candidate 并发布 Resolve 的 current；缺输入仍是中性
// tick。
TickDecision TickPipeline::BeginSimulation() noexcept {
  if (phase_ != TickPipelinePhase::Draining)
    return Abort(TickDecision::Reentry);
  const auto decision = clock_.Begin(boundary_, session_.AcceptsInput(), false);
  if (decision != TickDecision::Started)
    return Abort(decision);
  for (std::size_t i = 0; i < session_.ParticipantCount(); ++i)
    if (!players_[i].Begin(candidate_))
      return Abort(TickDecision::Reentry);
  phase_ = TickPipelinePhase::Simulating;
  return TickDecision::Started;
}
TickDecision TickPipeline::NativeReturned(const TickBoundary &boundary,
                                          bool normal) noexcept {
  if (phase_ != TickPipelinePhase::Simulating)
    return Abort(TickDecision::Reentry);
  const auto decision =
      clock_.CheckComplete(boundary, normal, session_.AllowsCompletion());
  if (decision != TickDecision::Committed)
    return Abort(decision);
  for (std::size_t i = 0; i < session_.ParticipantCount(); ++i)
    players_[i].End(true);
  phase_ = TickPipelinePhase::AwaitPostState;
  return TickDecision::Committed;
}
// 即使没有 world_record，也必须经过此阶段提交时钟，不能直接发布 receipt。
TickDecision
TickPipeline::FinishPostState(std::uint64_t loop,
                              const TickPostState &observed) noexcept {
  if (phase_ != TickPipelinePhase::AwaitPostState)
    return Abort(TickDecision::Reentry);
  if (loop != boundary_.loop)
    return Abort(TickDecision::WrongReturn);
  if (observed.status != PostStateStatus::Disabled)
    return Abort(TickDecision::InvalidSource);
  // NativeReturned 已确认正常模拟；随后绑定失效或请求退休不撤销已完成的事实。
  const auto decision = clock_.Complete(boundary_, true, true);
  if (decision != TickDecision::Committed)
    return Abort(decision);
  receipt_.tick = clock_.Completed();
  receipt_.loop = loop;
  receipt_.kind = boundary_.kind;
  receipt_.boundary = boundary_;
  receipt_.participant_count = session_.ParticipantCount();
  for (std::size_t i = 0; i < receipt_.participant_count; ++i) {
    const auto progress = players_[i].Progress(clock_.Completed());
    receipt_.clients[i] = {progress.client,
                           progress.generation,
                           progress.last_state_generation,
                           progress.last_accepted_sequence,
                           progress.last_processed_sequence,
                           progress.last_processed_tick,
                           progress.last_processed_source_sequence,
                           players_[i].BoundValid() && session_.AcceptsInput(),
                           true,
                           progress.availability};
  }
  phase_ = TickPipelinePhase::Committed;
  return TickDecision::Committed;
}
bool TickPipeline::PublishCompleted(bool success) noexcept {
  if (phase_ != TickPipelinePhase::Committed) {
    Abort(TickDecision::Reentry);
    return false;
  }
  if (!success) {
    session_.Fail(SessionCause::PublicationFailure);
    Abort(TickDecision::Cancelled);
    return false;
  }
  receipt_.published = true;
  phase_ = TickPipelinePhase::Published;
  return true;
}
TickDecision TickPipeline::EndOuter(std::uint64_t loop) noexcept {
  (void)loop;
  if (phase_ == TickPipelinePhase::Idle ||
      phase_ == TickPipelinePhase::Published ||
      phase_ == TickPipelinePhase::Failed)
    return TickDecision::Idle;
  return Abort(TickDecision::MissingReturn);
}

} // namespace cs::core
