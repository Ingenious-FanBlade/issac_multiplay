#pragma once
#include "state/client_tables.h"

namespace cs::state {
struct ClientShadowStorage;
struct ClientShadowStateData;
struct ClientShadowRecordData;
struct ClientShadowPacket;
// MetadataOnly只用于本地正式owner登记的纯身份锚点；不能由报文删减必需记录。
struct ClientShadowLayoutBinding {
  std::uint32_t space=0;
  std::uint16_t layout=0;
  bool metadata_only=false;
};
class ClientShadowError final : public std::runtime_error {
public:
  ClientShadowError(StateReadiness readiness,const char *reason,std::uint16_t member=kNullIndex,
      std::uint16_t layout=0,std::uint16_t field=kNullIndex)
      :std::runtime_error(reason),readiness_(readiness),member_(member),layout_(layout),field_(field) {}
  StateReadiness Readiness() const noexcept {return readiness_;}
  std::uint16_t Member() const noexcept {return member_;}
  std::uint16_t LayoutId() const noexcept {return layout_;}
  std::uint16_t FieldId() const noexcept {return field_;}
private:
  StateReadiness readiness_;
  std::uint16_t member_,layout_,field_;
};
// 变化版本／帧／来源属于最后实际携带该块的目标；继承及重新绑定表不伪造新变化。
struct ClientShadowBlockView {
  std::uint8_t block=0;
  std::uint32_t version=0,game_frame=0,source_member_epoch=0,source_resource_epoch=0;
  std::uint64_t source_revision=0;
  std::span<const std::byte> bytes;
};
// 此视图只借用持有它的State；返回字段片段指向独立不可变块，不指向解码scratch。
class ClientShadowRecordView final {
public:
  std::uint16_t Member() const noexcept;
  const PresentationMember &Identity() const noexcept;
  std::uint32_t Mask() const noexcept;
  std::optional<ClientShadowBlockView> Block(std::uint8_t) const noexcept;
  std::span<const BodyFieldView> Fields() const noexcept;
  const BodyFieldView *Field(std::uint16_t) const noexcept;
  std::span<const ProjectionFact> Projections() const noexcept;
private:
  friend class ClientShadowState;
  explicit ClientShadowRecordView(const ClientShadowRecordData *data):data_(data) {}
  const ClientShadowRecordData *data_;
};
// 每个State是某一序列的完整目标；块内容可以共享，不能沿父候选链无限延寿。
class ClientShadowState final {
public:
  explicit ClientShadowState(const std::shared_ptr<ClientStateMemory> &);
  const PresentationBatchContext &Context() const noexcept;
  std::size_t RecordCount() const noexcept;
  ClientShadowRecordView Record(std::size_t) const;
  std::optional<ClientShadowRecordView> Find(const PresentationMember &) const noexcept;
  const ClientMemberSnapshotRef &Members() const noexcept;
  const ClientResourceSnapshotRef &Resources() const noexcept;
private:
  friend struct ClientShadowStorage;
  std::shared_ptr<ClientShadowStateData> data_;
};
using ClientShadowStateRef=std::shared_ptr<const ClientShadowState>;
enum class ClientShadowChangeKind : std::uint8_t { Join, Reshape, Leave, Remap };
struct ClientShadowMemberChange {
  ClientShadowChangeKind kind=ClientShadowChangeKind::Join;
  StateMemberEntry before,after;
};
// 结构封存不授Ready：域、来源、资源、显示与实际原生资格仍由后续validator核定。
class ClientStateCandidate final {
public:
  explicit ClientStateCandidate(const std::shared_ptr<ClientStateMemory> &);
  const StateTarget &Target() const noexcept;
  const StateFrameView &Frame() const noexcept;
  ClientShadowStateRef State(BodyStream) const;
  std::span<const ClientShadowMemberChange> Changes() const noexcept;
private:
  friend struct ClientShadowStorage;
  friend class ClientStateShadow;
  std::shared_ptr<const ClientShadowPacket> packet_;
  std::array<ClientShadowStateRef,2> states_{};
  ClientStateVector<ClientShadowMemberChange> changes_;
};
using ClientStateCandidateRef=std::shared_ptr<const ClientStateCandidate>;
class ClientShadowValidation {
public:
  virtual ~ClientShadowValidation()=default;
  // 必须包含第4／5项的全部域／来源／必要结果／资源／窄拓扑检查；没有默认Ready实现。
  virtual StateReadiness Inspect(const ClientStateCandidate &) const=0;
};
class ClientShadowCommit {
public:
  virtual ~ClientShadowCommit()=default;
  // 第5项提供已预留的结果／事件缓存提升；仅纯值无分配提交，不再执行原生操作。
  // false表示无法保证完整提交，停止确认并共同恢复；没有默认成功实现。
  virtual bool Promote(const ClientStateCandidate &,const StateAppliedAck &) noexcept=0;
};
struct ClientShadowBuildResult {
  // decision是先定界所得的接受决策。candidate非空表示结构已封存，仍须Assess／Begin。
  StateAcceptDecision decision;
  ClientStateCandidateRef candidate;
};
// 后台唯一owner：构建只读候选、核成功base并驱动接受凭据；不创建线程或调用游戏。
// registry固定外部借用直到后台Collect。生命周期owner须Close并收回全部借用后销毁。
class ClientStateShadow final {
public:
  ClientStateShadow(std::shared_ptr<ClientStateMemory>,std::thread::id owner,std::span<const ClientShadowLayoutBinding>);
  ~ClientStateShadow();
  ClientStateShadow(const ClientStateShadow &)=delete;
  ClientStateShadow &operator=(const ClientStateShadow &)=delete;
  void Open(const StateRoute &,StateAgreement);
  void Close();
  void Collect();
  bool Drained() const;
  ClientShadowBuildResult Build(const StateRoute &,StateLane,std::span<const std::byte>,ClientStateTables &,
      BodyCodecLimits={32u<<20,262144,32,true});
  StateAcceptDecision Assess(const ClientStateCandidateRef &,const ClientShadowValidation &) const;
  StateApplyTicket Begin(const ClientStateCandidateRef &,const ClientShadowValidation &);
  // 只有真正的主线程整批完成才能调用；成功提升不分配，失败不发布候选或半个ACK。
  std::optional<StateAppliedAck> Complete(StateApplyTicket,StateApplyOutcome,ClientShadowCommit &,std::uint8_t affected_streams=0);
  // 可能写入／取消后必须收到真实清理排空再释放物理busy；逻辑inflight清零不能替代。
  bool CompleteDrain(StateApplyTicket);
  void RequireRecovery(std::uint8_t streams,StateResyncReason,bool structural=false);
  ClientShadowStateRef Installed(BodyStream) const;
  std::array<std::uint32_t,2> HistoricalVersions() const;
  std::uint64_t InstalledEvents() const;
  std::optional<StateResync> RecoveryRequest() const;
  std::array<std::optional<StateAppliedAck>,2> Reacknowledgements() const;
private:
  std::shared_ptr<ClientStateMemory> memory_;
  std::shared_ptr<ClientShadowStorage> storage_;
};
} // namespace cs::state
