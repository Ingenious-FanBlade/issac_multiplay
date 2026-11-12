#pragma once
#include "state/body_codec.h"
#include "state/wire_scope.h"

namespace cs::state {
// 此上下文来自公共头／分段头；不在线上再复制一份，也不增加Presentation确认序列。
using PresentationScope=StateScope;
struct PresentationBatchContext {
  PresentationScope scope;
  std::uint16_t layout_version=0;
  BodyStream stream=BodyStream::World;
  std::uint32_t member_epoch=0, resource_epoch=0, base_version=0, target_version=0;
  std::uint64_t source_revision=0, presentation_batch=0;
  std::uint32_t game_frame=0;
  // 仅由外层实际共同目标提供；0表示没有配对，不能从来源修订推算Peer目标。
  std::array<std::uint32_t,2> paired_targets{};
  std::uint64_t installed_event_watermark=0, event_watermark=0;
};
enum class ResultOperation : std::uint8_t { Replace, Reference, Retire };
enum class ResultDependencyKind : std::uint8_t { Member, Resource, PlayerTopology, ResultGroup, PairedTarget };
// 每个未使用字段保持规范空值。序号仅在精确epoch下解释，不充当持久身份。
struct ResultDependency {
  ResultDependencyKind kind=ResultDependencyKind::Member;
  std::uint8_t table=255;
  std::uint16_t member=kNullIndex, purpose=kNullIndex, index=kNullIndex;
  std::uint64_t revision=0, instance=0, generation=0;
  auto operator<=>(const ResultDependency &) const = default;
};
struct ResultContribution {
  std::uint32_t id=0, parent=UINT32_MAX;
  std::uint16_t pass=0, field=kNullIndex, element=kNullIndex, flags=0;
};
struct ResultGroupHeader {
  ResultOperation operation=ResultOperation::Replace;
  std::uint8_t flags=0;
  std::uint16_t owner=kNullIndex, subject=kNullIndex, purpose=0, layout=0;
  std::uint64_t instance=0, generation=0, produced_revision=0;
};
// payload仅为现有具名结果的typed值。Reference／Retire没有typed值；不套12B成员头。
struct ResultGroupView {
  ResultGroupHeader header;
  std::span<const ResultDependency> dependencies;
  std::span<const ResultContribution> contributions;
  std::span<const std::byte> payload;
  std::uint32_t consumed=0, field_visits=0;
};
struct PresentationAudioEvent {
  std::uint64_t sequence=0;
  std::uint32_t logical_frame=0;
  std::uint16_t owner=kNullIndex, purpose=382;
  std::uint64_t voice=0, generation=0;
  std::uint8_t clear_scope=0, flags=0;
  std::uint16_t layout=383;
  std::span<const std::byte> payload;
  std::uint32_t consumed=0, field_visits=0;
};
// 贡献暂存使用真实字段路径，最多16层父链。只存数字和匹配结果，不生成字段树。
struct ContributionTarget {
  std::array<CodecPathStep,kCodecMaxDepth> path{};
  std::uint16_t layout=0;
  std::uint8_t path_size=0, parent_depth=0;
  bool matched=false;
};
struct PresentationEnvelopeScratch {
  std::span<ResultDependency> dependencies;
  std::span<ResultContribution> contributions;
  std::span<ContributionTarget> targets;
  std::span<std::uint16_t> dependency_order, contribution_order;
  BodyCodecScratch body;
  // 解码同一批前由owner置零；逐组只追加，返回视图有效至owner复用这些区域。
  std::uint32_t dependencies_used=0, contributions_used=0;
};
struct PresentationEnvelopeLimits {
  BodyCodecLimits body;
};

// scope／来源／用途及规范依赖属于编解码规则；真正ready与来源资格由下一阶段解析。
// 所有失败抛Gap；编码输入与输出不得重叠。输出只属暂存，不能发布部分字节或修改缓存／事件水位。
ResultGroupView DecodeResultGroupPrefix(const PresentationBatchContext &, std::span<const std::byte>,
    PresentationEnvelopeScratch &, const BodyCodecContext *context=nullptr, PresentationEnvelopeLimits={});
std::span<const std::byte> EncodeResultGroupInto(const PresentationBatchContext &, const ResultGroupView &,
    std::span<std::byte>, PresentationEnvelopeScratch &, const BodyCodecContext *context=nullptr,
    PresentationEnvelopeLimits={});
PresentationAudioEvent DecodePresentationAudioEventPrefix(const PresentationBatchContext &,
    std::span<const std::byte>, BodyCodecScratch &, PresentationEnvelopeLimits={});
std::span<const std::byte> EncodePresentationAudioEventInto(const PresentationBatchContext &,
    const PresentationAudioEvent &, std::span<std::byte>, BodyCodecScratch &, PresentationEnvelopeLimits={});
struct PresentationBatchView {
  std::span<const ResultGroupView> groups;
  std::span<const PresentationAudioEvent> events;
  std::uint32_t consumed=0;
};
// 连续编码分段的groups随后events区域；计数来自唯一Section头，不添加第二层count。
// Decoder必须恰好消费调用者切出的整个显示区域；prefix接口供完整Frame顺序解析使用。
PresentationBatchView DecodePresentationBatch(const PresentationBatchContext &,std::uint32_t group_count,
    std::uint32_t event_count,std::span<const std::byte>,std::span<ResultGroupView>,
    std::span<PresentationAudioEvent>,PresentationEnvelopeScratch &,
    const BodyCodecContext *context=nullptr,PresentationEnvelopeLimits={});
std::span<const std::byte> EncodePresentationBatchInto(const PresentationBatchContext &,
    std::span<const ResultGroupView>,std::span<const PresentationAudioEvent>,std::span<std::byte>,
    PresentationEnvelopeScratch &,const BodyCodecContext *context=nullptr,PresentationEnvelopeLimits={},
    std::uint32_t *field_visits=nullptr);

// 完整组键解析到真实Key/use/shape；member号不保留为跨epoch身份，原生地址不进入键。
struct PresentationMember {
  std::uint32_t space=0;
  std::uint16_t layout=0;
  std::uint64_t id=0, use=0, shape=0;
  auto operator<=>(const PresentationMember &) const = default;
};
struct ResultGroupKey {
  PresentationScope scope;
  PresentationMember owner, subject;
  std::uint16_t purpose=0;
  std::uint64_t instance=0, generation=0;
  auto operator<=>(const ResultGroupKey &) const = default;
};
struct CommittedResult {
  ResultGroupKey key;
  std::uint16_t layout=0;
  // 缓存字节中的成员／资源序号属于原快照。epoch变化时使用Replace重新映射，
  // produced_revision保持真实原值，不能仅以组键仍相同便按新表重解释旧字节。
  std::uint32_t member_epoch=0, resource_epoch=0;
  std::uint64_t produced_revision=0;
  std::span<const ResultContribution> contributions;
  std::span<const std::byte> payload;
};
// 第12／13步提供精确快照与已提交不可变缓存。每个方法只读纯值；缺失返回失败，
// 不能调用Lua／游戏生产器补值。本接口不安装缓存，不创建原生对象，不产生ACK。
// Context实例只服务当前connection/route；退休后不能返回旧代次的组／事件缓存，
// 即使新旧scope／epoch数字相同。跨复制代次以完整目标／Replace重新建立资格。
class PresentationEnvelopeContext : public BodyCodecContext {
public:
  virtual std::optional<PresentationMember> Member(std::uint32_t epoch,std::uint16_t index) const = 0;
  virtual bool ResourceReady(std::uint32_t epoch,std::uint8_t table,std::uint16_t index) const = 0;
  virtual std::optional<std::uint32_t> PlayerTopology(std::uint32_t epoch,std::uint16_t member) const = 0;
  virtual std::optional<CommittedResult> Cached(const ResultGroupKey &,std::uint64_t produced_revision) const = 0;
  // ResultGroup依赖不重复subject；此查询按owner／用途／实例代次解析到完整已提交键。
  virtual std::optional<CommittedResult> Result(const PresentationScope &,const PresentationMember &owner,
      std::uint16_t purpose,std::uint64_t instance,std::uint64_t generation,std::uint64_t revision) const = 0;
  // owner保留已接受但尚未随world提交／ACK的事件正文，拒绝同序号内容冲突。
  // 精确重传幂等；没有既有事件时返回空，本层不会自行登记或推进水位。
  virtual std::optional<PresentationAudioEvent> KnownEvent(const PresentationScope &,std::uint64_t sequence) const = 0;
  // 包括正常来源资格、失效、实例不可改绑subject，以及具名合同的窄状态依赖。
  // 不得用内容相等或旧strong仍存活替代这些事实；Retire必须有明确退休事实。
  virtual bool SourceQualified(const ResultGroupKey &,std::uint64_t produced_revision,
      const PresentationBatchContext &,ResultOperation) const = 0;
};
// 单个分段的全部组／事件形成一次校验；同批组依赖可前向引用，环／缺口失败。
// group_keys与dependency_state为调用者scratch。成功只证明本批显式输入及其依赖ready；
// 完整目标必需组覆盖／base继承由第12步检查，本层不提交／不ACK。
void ValidatePresentationBatch(const PresentationBatchContext &, std::span<const ResultGroupView>,
    std::span<const PresentationAudioEvent>, const PresentationEnvelopeContext &,
    std::span<ResultGroupKey> group_keys, std::span<std::uint8_t> dependency_state,
    PresentationEnvelopeScratch &, PresentationEnvelopeLimits={});
} // namespace cs::state
