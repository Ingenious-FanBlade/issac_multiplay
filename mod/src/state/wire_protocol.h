#pragma once
#include "state/frame_events.h"

namespace cs::state {
// 与旧Range／JSON入口隔离。这里只接收重组完成的正文，连接与route取外层可信封套。
enum class StateMessageKind : std::uint8_t { ContextReady=8, AppliedAck=9, Resync=10, Frame=21, Members=22, Resources=23 };
enum class StateLane : std::uint8_t { Control=1, Baseline=2, Context=3, World=4, Body=5 };
struct StateRoute {
  std::uint64_t connection=0, route=0;
  StateScope scope;
  auto operator<=>(const StateRoute &) const = default;
};
struct StateAgreement { std::uint32_t binary_state=0; std::uint16_t layout_version=0; };
StateAgreement LocalStateAgreement() noexcept;
std::uint8_t StateBinaryEnvelopeFlag() noexcept;
bool CompatibleStateAgreement(StateAgreement) noexcept;

enum class MemberOperation : std::uint8_t { Join, Reshape, Leave };
struct StateMemberEntry {
  std::uint16_t index=kNullIndex;
  MemberOperation operation=MemberOperation::Join;
  PresentationMember identity;
  auto operator<=>(const StateMemberEntry &) const = default;
};
struct StateMemberTable {
  StateScope scope;
  std::uint32_t epoch=0, previous_epoch=0;
  std::span<const StateMemberEntry> entries;
};
// 编解码只形成不可变元数据；生命周期owner在第12步保留退休历史并决定合法回收。
StateMemberTable DecodeStateMemberTable(const StateScope &,std::span<const std::byte>,std::span<StateMemberEntry>,std::span<std::uint16_t> key_order);
std::span<const std::byte> EncodeStateMemberTableInto(const StateMemberTable &,std::span<std::byte>,std::span<std::uint16_t> key_order);
struct StateMemberDirectory {
  StateScope scope;
  std::uint32_t epoch=0, previous_epoch=0;
  std::span<const StateMemberEntry> entries, update;
};
class StateMemberTransitions {
public:
  virtual ~StateMemberTransitions() = default;
  // null表示加入／离开；还须核退休身份不复活、合法序号回收。
  // Host核全体存活Peer，Client核本端未退休的在途引用，不虚构其他Peer的本地证明。
  // 此查询只读自有生命周期账本，不删除原生对象，也不因表到达而退休已提交目标。
  virtual bool Allowed(const StateScope &,const StateMemberEntry *before,const StateMemberEntry *after) const = 0;
};
const StateMemberEntry *FindStateMember(const StateMemberDirectory &,std::uint16_t) noexcept;
StateMemberDirectory MergeStateMemberTableInto(const StateMemberDirectory &,const StateMemberTable &,
    std::span<StateMemberEntry>,const StateMemberTransitions &);

// records仅借用完整的块正文；投影事实不跨记录借用同一scratch，完整影子须重新合并校验。
struct StateRecordPayload {
  std::uint16_t member=0, layout=0;
  std::uint32_t mask=0;
  std::span<const std::byte> bytes;
};
struct StateSectionView {
  PresentationBatchContext context;
  std::span<const StateRecordPayload> records;
  StateFrameEventView presentation;
};
struct StateFrameView {
  StateScope scope;
  std::uint16_t layout_version=0;
  std::uint8_t section_count=0;
  std::array<StateSectionView,2> sections{};
  // 解码／编码完成后的精确规范正文，由owner持有；用精确字节比较重复，不计算哈希。
  std::span<const std::byte> encoded;
  bool decoded=false; // 仅完整结构解码成功才为true；定界结果不能获应用ticket。
};
class StateFrameCodecContext {
public:
  virtual ~StateFrameCodecContext() = default;
  virtual std::optional<PresentationMember> Member(std::uint32_t epoch,std::uint16_t index) const = 0;
  // member=FFFF用于结果值；其余为当前根的同候选共享投影／同epoch配置上下文。
  virtual const BodyCodecContext *BodyContext(const PresentationBatchContext &,std::uint16_t member) const = 0;
};
struct StateFrameScratch {
  std::span<StateRecordPayload> records;
  std::span<PresentationAudioEvent> events;
  std::span<std::uint16_t> member_order;
  BodyCodecScratch body;
};
// reliable仅解码内存保护；不会让普通超预算帧改走Baseline。预算由整帧统一扣除。
StateFrameView DecodeStateFrame(const StateScope &,std::span<const std::byte>,std::uint64_t installed_events,
    const StateFrameCodecContext &,StateFrameScratch &,BodyCodecLimits={});
// 先核固定头与整帧边界，使旧base可补ACK而不按当前水位误报旧事件缺口。
// 不解析typed正文、不检查资源，不可当作完整候选或申请原生提交。
StateFrameView InspectStateFrame(const StateScope &,std::span<const std::byte>,std::uint64_t installed_events,
    StateFrameScratch &,BodyCodecLimits={});
std::span<const std::byte> EncodeStateFrameInto(StateFrameView &,std::span<std::byte>,
    const StateFrameCodecContext &,StateFrameScratch &,BodyCodecLimits={});
StateLane StateFrameLane(const StateFrameView &);

struct StateContextReady { StateScope scope; std::uint32_t member_epoch=0, resource_epoch=0; };
struct StateAckEntry {
  BodyStream stream=BodyStream::World;
  std::uint32_t target=0, member_epoch=0, resource_epoch=0;
  std::uint64_t event_watermark=0;
  auto operator<=>(const StateAckEntry &) const = default;
};
struct StateAppliedAck {
  StateScope scope;
  std::uint8_t count=0;
  std::array<StateAckEntry,2> entries{};
  // count=2就是原始joint凭据，不得拆成两个单项ACK。
  bool operator==(const StateAppliedAck &) const = default;
};
enum class StateResyncReason : std::uint16_t { Base=1, Table=2, Dependency=3, Budget=4, Apply=5, Event=6, Lifecycle=7 };
struct StateResync {
  StateScope scope;
  std::uint8_t streams=0;
  StateResyncReason reason=StateResyncReason::Base;
  // reason=Apply时仅为历史成功版本，Host无条件为请求序列重建base=0。
  std::uint32_t installed_world=0, installed_body=0;
};
StateContextReady DecodeStateContextReady(const StateScope &,std::span<const std::byte>);
StateAppliedAck DecodeStateAppliedAck(const StateScope &,std::span<const std::byte>);
StateResync DecodeStateResync(const StateScope &,std::span<const std::byte>);
std::span<const std::byte> EncodeStateContextReadyInto(const StateContextReady &,std::span<std::byte>);
std::span<const std::byte> EncodeStateAppliedAckInto(const StateAppliedAck &,std::span<std::byte>);
std::span<const std::byte> EncodeStateResyncInto(const StateResync &,std::span<std::byte>);
// 从原完整目标形成ACK只提供数值转换；调用者只能在原生整批成功后发布。
StateAppliedAck StateFrameAck(const StateFrameView &);
} // namespace cs::state
