#pragma once
// tx-04-10 公共纯值合同；域、身份空间和来源阶段分别编号，禁止携带借用游戏对象。
#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace cs::capture {class OwnedSource;}

namespace cs::world::observation {
enum class DomainId : std::uint16_t { Entity=1, Grid, Room, Level, Exploration, RoomSave, Player, Run, Random, Presentation, Mod };
enum class IdentitySpace : std::uint16_t { Entity=1, Grid, Run, Floor, Dimension, RoomDescriptor, RoomInstance, SaveEntry, Participant, ControlGroup, RandomStream, Presentation, Resource, ModGraphNode };
enum class Presence : std::uint8_t { Known, Unknown, NotApplicable };
enum class Phase : std::uint8_t { OrdinaryPost, PlayerPost, Lifecycle, Transition, Render, ActualCall };
// 按域声明实际接线阶段：Room无Transition来源，Level无Render来源；未知请求明确拒绝。
constexpr std::uint32_t SupportedPhases(DomainId domain) noexcept {
  return domain==DomainId::Entity ? 55u : domain==DomainId::Grid ? 53u : domain==DomainId::Room ? 53u : domain==DomainId::Level ? 45u : domain==DomainId::Player ? 47u : domain>=DomainId::Exploration&&domain<=DomainId::RoomSave ? 13u : domain==DomainId::Run ? 45u : domain==DomainId::Random ? 37u : domain==DomainId::Presentation ? 53u : domain==DomainId::Mod ? 61u : 0u;
}
enum class RecordKind : std::uint8_t { State, Event, Relation, Coverage, Gap, Catalog };
enum class Outcome : std::uint8_t { AuthorityCommitted, IndependentPublished, Cancelled, Invalidated };
enum class Coverage : std::uint8_t { Complete, Partial, Failed, NotApplicable, NotScheduled, Disabled, Unsupported };
enum class ValueStatus : std::uint8_t { Value, Null, Empty, NotApplicable, Unavailable, NotCaptured, ReadFailed, Nonfinite, Truncated };
enum class Resolution : std::uint8_t { Null, KnownCurrent, KnownHistorical, Unresolved, NotCaptured, UnsupportedTarget, Retired, AmbiguousReuse, CrossEpoch };
template<class T> struct Optional final { Presence presence=Presence::Unknown; T value{}; };
// 主体仅在本会话与观测 epoch 内唯一；录制代次不参与主体身份。
struct SubjectKey final {
  std::uint64_t session=0, epoch=0, local=0;
  IdentitySpace space=IdentitySpace::Entity;
  bool operator==(const SubjectKey &) const = default;
};
struct ObservationContext final {
  std::uint64_t session=0, epoch=0, scope=0, parent_scope=0, sequence=0;
  Optional<SubjectKey> run, floor, dimension, descriptor, room;
  Optional<std::uint64_t> authority_tick;
  Phase phase=Phase::OrdinaryPost;
  std::uint64_t outer=0, owner=0, mutation_before=0, mutation_after=0, event_watermark=0;
  std::uint32_t source=0, source_version=1, plan_revision=1;
  // 仅供Service按同一实际普通采集机会调度，不是新增网络状态序号。
  std::uint64_t capture_index=0;
};
struct ObservationReference final {
  SubjectKey source{}, target{};
  IdentitySpace target_space=IdentitySpace::Entity;
  Resolution resolution=Resolution::Unresolved;
  std::uint64_t scope=0, sequence=0, target_revision=0;
  std::string relation, basis;
};
// 2026-11-03按用户要求先放宽节点容量；独立的字节/深度/对象约束继续生效。
inline constexpr std::uint32_t kNodeBudget=64u<<20;
// Entity承载完整Player及备份，和Grid同为32MiB；其它域维持16MiB最高上限。
constexpr std::uint64_t MaximumDomainBytes(DomainId d) noexcept {return (d==DomainId::Entity||d==DomainId::Grid?32ull:16ull)<<20;}
// Random同时计数每身体的完整原生流，八活动体及备份可超过Entity成员16384；仍按域分别有界。
constexpr std::uint32_t MaximumDomainObjects(DomainId d) noexcept {return d==DomainId::Random?32768u:16384u;}
struct Budget final { std::uint32_t objects=16384, nodes=kNodeBudget, depth=12; std::uint64_t bytes=16ull<<20; };
struct DomainResult final {
  Coverage coverage=Coverage::NotScheduled;
  std::string range;
  std::vector<std::string> reasons;
  std::uint64_t objects=0, nodes=0, bytes=0;
  Budget budget{};
};
struct DomainDescriptor final {
  DomainId domain;
  const char *schema;
  std::uint32_t version;
  const char *range;
  std::uint32_t phases;
  Budget budget;
};
struct DomainRequest final { DomainId domain=DomainId::Entity; std::uint32_t phases=1, interval=1; Budget budget{}; };
struct CapturePlan final { std::uint32_t revision=1; std::vector<DomainRequest> domains; };
// payload 是主线程已经复制的 JSON 值；后台只能编码 envelope，不能补调 Getter。
struct ObservationEnvelope final {
  ObservationContext context{};
  DomainId domain=DomainId::Entity;
  RecordKind kind=RecordKind::State;
  std::string schema;
  std::uint32_t version=2;
  DomainResult result{};
  std::string payload;
  // 正式实时来源在Lua编码前分域；录制/旧模型仍只使用payload，不读取此产品热路径。
  std::string stream_payload;
  // 正式来源直接持有封闭的当前窗口纯值；诊断文本可选，后台分层不再解析Lua JSON。
  std::shared_ptr<const capture::OwnedSource> owned_source;
  std::uint64_t owned_bytes=0;
};
struct RequestResult final { bool accepted=false; const char *reason="unavailable"; };
// 窄回调仅在主线程借用 const 值；需要后台使用时须复制到自有有界队列。
struct Consumer final {
  std::uint32_t id=0;
  void *owner=nullptr;
  void (*publish)(void *, const ObservationEnvelope &) noexcept=nullptr;
  CapturePlan plan{};
  // 失效先于后续值交付；正式consumer可撤销候选，不受Recorder I/O结果影响。
  void (*invalidate)(void *,const char *) noexcept=nullptr;
};
const char *CoverageName(Coverage) noexcept;
const char *PhaseName(Phase) noexcept;
const char *RecordKindName(RecordKind) noexcept;
std::string EncodeEnvelope(const ObservationEnvelope &);
} // namespace cs::world::observation
