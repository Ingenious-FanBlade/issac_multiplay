#pragma once
#include "state/wire_protocol.h"
#include "state/resource_protocol.h"

namespace cs::state {
// 描述与规范正文由同一次编解码产生。所有span借用owner的不可变存储；退休凭据前不可覆盖。
struct StateTarget {
  StateRoute route;
  std::uint16_t layout_version=0;
  std::uint8_t count=0;
  std::array<PresentationBatchContext,2> sections{};
  std::span<const std::byte> encoded;
  bool decoded=false;
};
StateTarget DescribeStateTarget(const StateRoute &,const StateFrameView &);
StateAppliedAck StateTargetAck(const StateTarget &);
std::uint8_t StateTargetStreams(const StateTarget &);
// 联合或完整恢复占用两槽；票据仍只接受报文实际携带的流。
inline std::uint8_t StateApplyOccupancy(const StateTarget &target) {
  return !target.sections[0].base_version?3:StateTargetStreams(target);
}
inline unsigned StateApplyChannel(const StateTarget &target) {
  return StateApplyOccupancy(target)==2?1:0;
}
// 检查所有仍保留的候选／在途／提交凭据，同一序列target不可重新绑定正文或joint关系。
bool StateTargetsConflict(const StateTarget &,const StateTarget &);

enum class StateReadiness : std::uint8_t {
  Ready, MissingTable, ResourcePending, SourcePending, MissingClosure, EventGap, BaseLost,
  Budget, InvalidEncoding, InvalidReference, MissingLocalResource, Lifecycle
};
enum class StateAcceptAction : std::uint8_t { Drop, Reack, Decode, Wait, Apply, Resync, Terminal };
struct StateAcceptDecision {
  StateAcceptAction action=StateAcceptAction::Drop;
  std::uint8_t streams=0;
  StateResyncReason reason=StateResyncReason::Base;
};
// 第12步唯一完整候选owner给出Ready；必须包含继承块／投影、必需结果、资源用途／闭包、
// 引用／生命周期、来源和窄拓扑。不得仅凭wire解码成功返回Ready；方法只读自有纯值。
class StateCandidateContext {
public:
  virtual ~StateCandidateContext() = default;
  virtual StateReadiness Inspect(const StateTarget &) const = 0;
};
struct StateApplyTicket { StateRoute route; std::uint64_t serial=0; };

// 此状态机没有游戏／网络调用，只交接凭据。owner按主线程完成消息串行驱动它，
// 原生结构退休仍须等待在途callback／draw借用正常排空，不能将Retire当作已回滚。
class ClientStateAcceptance {
public:
  void Open(const StateRoute &,StateAgreement);
  void Retire() noexcept;
  StateAcceptDecision Assess(const StateTarget &,const StateCandidateContext &) const;
  // Begin再次Inspect，防止候选排队期间失效；仅Apply可获唯一ticket。
  StateApplyTicket Begin(const StateTarget &,const StateCandidateContext &);
  // 只推进客机的应用版本与恢复状态。接收 ACK 由快照环生成。
  bool MarkApplied(const StateTarget &);
  void RequireRecovery(std::uint8_t streams,StateResyncReason,bool structural=false);
  std::optional<StateResync> RecoveryRequest() const;
  std::array<std::uint32_t,2> HistoricalVersions() const noexcept { return versions_; }
  std::uint8_t UnusableStreams() const noexcept { return needs_full_; }
private:
  StateRoute route_{};
  bool active_=false;
  std::uint64_t serial_=0, events_=0;
  std::array<std::uint32_t,2> versions_{};
  std::uint8_t needs_full_=3;
  bool needs_joint_=true;
  StateResyncReason reason_=StateResyncReason::Lifecycle;
};

enum class StateAckResult : std::uint8_t { Accepted, Duplicate, StaleRoute, UnknownOrConflict };
struct StateRecoveryPlan {
  std::uint8_t streams=0;
  StateResyncReason reason=StateResyncReason::Base;
  // installed只诊断，不修改Host issued／confirmed或授权跳水位。
  std::array<std::uint32_t,2> bases{};
};
// ledger只保存实际已发出的未确认目标。owner预留槽和字节租约后才能开始发送；
// 未发出的队列不能登记。满时停止新发布并按预算恢复，不能静默遗忘未确认目标。
class HostStateAcknowledgements {
public:
  HostStateAcknowledgements(const StateRoute &,std::span<StateTarget> ledger,class StateEventQueue *events=nullptr);
  bool RegisterSent(const StateTarget &);
  StateAckResult Accept(const StateRoute &,const StateAppliedAck &);
  // 同一请求重传不抬高恢复起点；恢复期间迟到的旧增量／旧baseline ACK不能解除恢复。
  StateRecoveryPlan Recover(const StateRoute &,const StateResync &,bool structural=false);
  std::array<StateAckEntry,2> Confirmed() const noexcept { return confirmed_; }
  std::array<std::uint32_t,2> Issued() const noexcept { return issued_; }
  std::uint8_t RecoveringStreams() const noexcept { return recovering_; }
  // 调度owner在发送前还须扣掉自己已预留但尚未发出的槽／租约，并保留恢复槽。
  std::size_t FreeSlots() const noexcept;
  std::uint64_t PinnedBodyBytes() const noexcept;
private:
  StateRoute route_;
  StateEventQueue *events_=nullptr;
  std::span<StateTarget> ledger_;
  std::array<std::uint32_t,2> issued_{};
  std::array<StateAckEntry,2> confirmed_{};
  std::array<std::optional<StateAppliedAck>,2> receipts_{};
  std::uint8_t recovering_=0;
  std::uint8_t recovery_seen_=0;
  bool recovery_joint_=false;
  std::array<std::uint32_t,2> recovery_floor_{}, recovery_history_{};
};
StateRecoveryPlan PlanStateRecovery(const StateRoute &,const StateRoute &,const StateResync &,bool structural);
// 仅由生命周期owner排空并退休旧复制route后调用。整数epoch可在新复制代次重新编号，
// 但完整资源身份目录逐字节保持，同run编号／内容历史绝不清空，不因换route自动调用。
ResourceDirectoryView RebaseStateResourceEpoch(const StateRoute &retired,const StateRoute &fresh,
    const ResourceDirectoryView &,const ResourceTableView &full,std::span<ResourceEntryView>,ResourceCodecScratch &);
} // namespace cs::state
