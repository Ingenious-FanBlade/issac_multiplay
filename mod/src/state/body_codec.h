#pragma once
#include "state/nested.h"
#include <array>
#include <compare>
#include <optional>

namespace cs::state {
// 正文与原生对象分开：所有输入均为已封存自有纯值，小端标量保留实际位模式。
enum class ScalarKind : std::uint8_t { None, U8, I8, U16, I16, U32, I32, U64, I64, F32, F64 };
enum class BodyStream : std::uint8_t { World, Body, Value };
inline constexpr std::size_t kCodecMaxDepth = 32;
// 路径只含生成的布局／字段／元素位置；同容器不同投影使用相同canonical field。
struct CodecPathStep {
  std::uint16_t layout=0, field=0, element=kNullIndex, choice_layout=0;
  auto operator<=>(const CodecPathStep &) const = default;
};
struct CodecLocation {
  std::span<const CodecPathStep> parents;
  std::uint16_t layout=0, field=0, canonical_field=0;
};
// 实际配置来自调用者持有的同resource_epoch纯值表；不访问配置原生指针或执行Getter。
struct CodecItemConfig { std::uint32_t type=0, id=0; std::int32_t cooldown=0; };
class BodyCodecContext {
public:
  virtual ~BodyCodecContext() = default;
  virtual std::optional<CodecItemConfig> Item(std::uint8_t, std::uint16_t) const { return {}; }
  // 增量仅带共享owner尾投影时，必须提供同一完整候选中首投影的存在性，不能猜空。
  virtual std::optional<bool> SharedPresence(const CodecLocation &) const { return {}; }
};
// 第10／11步把已采集、已换算的自有值绑定到数字路径；本接口不得读取游戏内存。
// Scalar返回恰好count个指定小端标量；仅有效分支会调用源，不读取旧可选负载。
class BodyValueSource {
public:
  virtual ~BodyValueSource() = default;
  virtual std::span<const std::byte> Scalar(const CodecLocation &, ScalarKind, std::uint32_t count) const = 0;
  virtual std::uint32_t Count(const CodecLocation &) const = 0;
  virtual bool SharedPresent(const CodecLocation &) const = 0;
  virtual std::uint32_t SlotMask(const CodecLocation &) const = 0;
  virtual std::uint16_t SlotLayout(const CodecLocation &, std::uint8_t slot) const = 0;
};
// 结构预算与实际发送额度分开；本层最多允许旧32MiB解码保护，正常调用默认64KiB。
// 字段访问次数包含嵌套元素，限制零长节点／条件分支造成的CPU放大。
struct BodyCodecLimits {
  std::uint32_t bytes=65536, field_visits=262144;
  std::uint8_t depth=32;
  // 可靠承运不改变body确认序列；ordinary body-joint不套PlayerFast专用body_limit。
  // 调用方仍须给出整帧剩余额度，并遵守首次准备／速率预算，不能靠此标志放宽产品预算。
  bool reliable=false;
};
enum class ProjectionFactKind : std::uint8_t { Count, SharedPresence };
// 投影事实只属于调用者当前根身份／目标；跨base合并前须由影子层核scope/use/shape。
// count group与slot来自生成计划；presence group使用canonical field，均不计算哈希。
struct ProjectionFact {
  std::array<CodecPathStep,kCodecMaxDepth> parents{};
  std::uint16_t layout=0, group=0, slot=0, field=0, value=0, limit=0, copies=0, slots=0;
  std::uint8_t depth=0;
  ProjectionFactKind kind=ProjectionFactKind::Count;
};
// 可选根字段视图复用生成字段索引；失效条件分支明确present=false，不伪装为空容器。
struct BodyFieldView {
  std::uint16_t field=0;
  std::uint8_t block=0;
  bool present=false;
  std::span<const std::byte> bytes;
};
// 调用者复用有界scratch；正文编解码器不分配字段树、map或逐元素临时vector。
// root_fields为空时只输出块视图；提供该区域时必须容纳本次选中根字段。
struct BodyCodecScratch {
  std::span<ProjectionFact> facts;
  std::uint32_t used=0;
  std::span<BodyFieldView> root_fields;
  std::uint32_t root_field_count=0;
};
struct BodyBlockView { std::uint8_t block=0; std::span<const std::byte> bytes; };
// 成功返回只证明正文结构及本地值规则成立。links_required和needs_base_checks必须在
// 第12步完整候选上解决；本对象不代表资源ready、生命周期合法、已安装或可回ACK。
struct BodyView {
  std::uint16_t layout=0;
  std::uint32_t block_mask=0;
  BodyStream stream=BodyStream::Value;
  std::span<const std::byte> bytes;
  std::array<BodyBlockView,32> blocks{};
  std::uint8_t block_count=0;
  std::span<const ProjectionFact> facts;
  std::span<const BodyFieldView> fields;
  bool links_required=false, needs_base_checks=false;
};
struct BodyRecordView { std::uint16_t member=0; BodyView body; std::uint32_t consumed=0; };

// 编码写入调用者区域；失败时该区域视为未发布暂存，禁止使用部分结果。
BodyView EncodeBodyInto(std::uint16_t layout, std::uint32_t block_mask, BodyStream,
    const BodyValueSource &, std::span<std::byte> storage, BodyCodecScratch &,
    const BodyCodecContext *context=nullptr, BodyCodecLimits={});
BodyView DecodeBody(std::uint16_t layout, std::uint32_t block_mask, BodyStream,
    std::span<const std::byte> encoded, BodyCodecScratch &,
    const BodyCodecContext *context=nullptr, BodyCodecLimits={});
// 嵌套／结果／公共固定头按完整值布局编码，不额外添加状态记录头或块mask。
BodyView EncodeValueInto(std::uint16_t layout, const BodyValueSource &, std::span<std::byte>,
    BodyCodecScratch &, const BodyCodecContext *context=nullptr, BodyCodecLimits={});
BodyView DecodeValue(std::uint16_t layout, std::span<const std::byte>, BodyCodecScratch &,
    const BodyCodecContext *context=nullptr, BodyCodecLimits={});
// 精确保留12B头；Decoder的expected身份由指定成员表快照提供，不按线上layout猜类型。
BodyRecordView EncodeRecordInto(std::uint16_t member, std::uint16_t layout, std::uint32_t mask,
    BodyStream, const BodyValueSource &, std::span<std::byte>, BodyCodecScratch &,
    const BodyCodecContext *context=nullptr, BodyCodecLimits={});
BodyRecordView DecodeRecordPrefix(std::uint16_t expected_member, std::uint16_t expected_layout,
    BodyStream, std::span<const std::byte>, BodyCodecScratch &,
    const BodyCodecContext *context=nullptr, BodyCodecLimits={});
// 对同目标合并后的事实排序并检查计数组／共享presence。require_complete=true时缺
// 投影也失败；false时返回是否仍需base投影。调用者必须保证不混入不同根身份的事实。
bool ValidateProjectionFacts(std::span<ProjectionFact>, bool require_complete);
} // namespace cs::state
