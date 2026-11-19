#pragma once
#include <chrono>
#include "state/client_tables.h"
#include "state/client_flat.h"

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
// 版本／帧／来源描述块内容及引用语义的来源。等价重传沿用已安装块的元数据。
struct ClientShadowBlockView {
  std::uint8_t block=0;
  std::uint32_t version=0,game_frame=0,source_member_epoch=0,source_resource_epoch=0;
  std::uint64_t source_revision=0;
  std::span<const std::byte> bytes;
};
// 反向边按被引用身份／资源建立，消费者精确到成员与根块。
struct ClientShadowDependency {
  PresentationMember member,consumer;
  ResourceKey resource;
  std::uint8_t block=0;
  bool is_resource=false;
  auto operator<=>(const ClientShadowDependency &) const = default;
};
struct ClientShadowVisitCursor {unsigned block=0;std::size_t node=0;};
// 此视图只借用持有它的State；返回字段片段指向独立不可变块，不指向解码scratch。
class ClientShadowRecordView final {
public:
  std::uint16_t Member() const noexcept;
  const PresentationMember &Identity() const noexcept;
  std::uint32_t Mask() const noexcept;
  const ClientFlatImageRef &Flat() const noexcept;
  std::span<const std::byte> FlatValue(std::string_view name) const noexcept;
  std::optional<ClientShadowBlockView> Block(std::uint8_t) const noexcept;
  // 块相同须同时满足内容和解析出的引用语义，不能只比较 wire 序号或地址。
  bool SameBlock(ClientShadowRecordView,std::uint8_t) const noexcept;
  bool SameContent(ClientShadowRecordView,std::uint8_t) const noexcept;
  // 比较指定根字段及其实际引用，允许同块别的字段更新。
  bool SameField(ClientShadowRecordView,std::uint16_t) const noexcept;
  bool SameStructure(ClientShadowRecordView,std::uint8_t) const noexcept;
  bool SameDependencies(ClientShadowRecordView,std::uint8_t) const noexcept;
  std::uint32_t FieldVisits() const noexcept;
  // 回放已定界的数字节点，所有 span 都借自此记录的不可变块。
  void Visit(std::uint32_t mask,BodyNodeObserver &) const;
  bool VisitSome(std::uint32_t mask,BodyNodeObserver &,ClientShadowVisitCursor &,unsigned max_nodes) const;
  // 按块重放时由调用方在记录末尾统一登记根节点，块本身只产生字段和数字节点。
  bool VisitBlockSome(std::uint8_t,BodyNodeObserver &,std::size_t &node,unsigned &remaining) const;
  // 导入已验证的计划片段只需字段／值的原交错顺序，每次最多取 64 个节点。
  std::uint64_t BlockFieldNodes(std::uint8_t,std::size_t first,unsigned count) const;
  void AppendResourceKeys(std::uint8_t block,ClientStateVector<ResourceKey> &)const;
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
  std::optional<std::uint16_t> RecordPosition(const PresentationMember &) const noexcept;
  const ClientMemberSnapshotRef &Members() const noexcept;
  const ClientResourceSnapshotRef &Resources() const noexcept;
  std::span<const ClientFlatFault> FlatFaults() const noexcept;
  const ClientFlatFault *FlatFault(const PresentationMember &) const noexcept;
#ifdef ISAACCS_E2E
  bool DeferredResource(std::uint16_t member) const noexcept;
#endif
  // 比较实际根和引用编号；资源目录同 run 只追加，旧引用必须在目标表存在。
  bool MemberClosureCompatible(const ClientMemberSnapshotRef &) const noexcept;
  bool ResourceClosureCompatible(const ClientResourceSnapshotRef &) const noexcept;
  std::span<const ClientShadowDependency> MemberConsumers(const PresentationMember &) const noexcept;
  std::span<const ClientShadowDependency> ResourceConsumers(ResourceKey) const noexcept;
private:
  friend struct ClientShadowStorage;
  friend class ClientStateShadow;
  std::shared_ptr<ClientShadowStateData> data_;
};
using ClientShadowStateRef=std::shared_ptr<const ClientShadowState>;
// 一次发布的完整两流引用。它只持有纯值，未变化的流、成员和块连续共享。
struct ClientStateBlockTable final {
  StateRoute route;
  std::array<ClientShadowStateRef,2> streams;
};
using ClientStateBlockTableRef=std::shared_ptr<const ClientStateBlockTable>;
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
  friend class ClientStateEvents;
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
struct ClientShadowBuildResult {
  // decision是先定界所得的接受决策。candidate非空表示结构已封存，仍须Assess／Begin。
  StateAcceptDecision decision;
  ClientStateCandidateRef candidate;
};
struct ClientShadowBuildWork;
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
  // 显式诊断请求才遍历注册表，并按实际正文 owner 去重；采样失败不改变接纳。
  void SampleBlockMemory(std::span<const ClientStateCandidateRef> waiting,
      std::span<const std::shared_ptr<ClientShadowBuildWork>> decoding) noexcept;
  bool Drained() const;
  ClientShadowBuildResult Build(const StateRoute &,StateLane,std::span<const std::byte>,ClientStateTables &,
      BodyCodecLimits={32u<<20,262144,32,true});
  // 普通 Body 同步消费，World／joint 从原等待位续做；一个成员和一条 wire 记录是最小工作单元。
  std::optional<ClientShadowBuildResult> BuildStep(const StateRoute &,StateLane,std::span<const std::byte>,ClientStateTables &,
      std::shared_ptr<ClientShadowBuildWork> &,std::chrono::steady_clock::time_point,
      BodyCodecLimits={32u<<20,262144,32,true});
  StateAcceptDecision Assess(const ClientStateCandidateRef &,const ClientShadowValidation &) const;
  // 等待位只使用定界决策筛掉冲突／迟到报文，不把它当作完整候选Ready。
  StateAcceptDecision AssessEnvelope(const StateTarget &,const ClientStateTables &) const;
  StateApplyTicket Begin(const ClientStateCandidateRef &,const ClientShadowValidation &);
  // 发布回执只推进应用影子。排空回执用于撤销仍占用原生承载的票据。
  bool CompleteApplication(StateApplyTicket,bool applied,bool drained,class ClientStateEvents &);
  // 可能写入或取消后须收到实际排空回执，才释放用量。
  bool CompleteDrain(StateApplyTicket);
  void RequireRecovery(std::uint8_t streams,StateResyncReason,bool structural=false);
  ClientShadowStateRef Installed(BodyStream) const;
  std::array<std::uint32_t,2> HistoricalVersions() const;
  // 完全解码的纯值根进入接收环后才允许发送 ACK。
  StateAppliedAck RetainReceived(const ClientStateCandidateRef &);
  ClientStateCandidateRef ForWorldApplication(const ClientStateCandidateRef &);
  ClientStateCandidateRef SelectKnownBodyObjects(const ClientStateCandidateRef &);
  ClientStateCandidateRef DeferUnreadyObjects(const ClientStateCandidateRef &,const ClientStateTables &);
  std::array<std::uint32_t,2> ReceivedVersions() const;
  std::uint64_t ReceivedEvents() const;
  std::optional<StateResync> RecoveryRequest() const;
  std::array<std::optional<StateAppliedAck>,2> Reacknowledgements() const;
private:
  std::shared_ptr<ClientStateMemory> memory_;
  std::shared_ptr<ClientShadowStorage> storage_;
};
} // namespace cs::state
