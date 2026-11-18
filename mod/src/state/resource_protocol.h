#pragma once
#include "state/body_codec.h"
#include "state/wire_scope.h"
#include "state/resource_local.h"

namespace cs::state {
// table与kind是不同编号空间。FFFF仅用于状态字段Null，不构成真实资源表项。
struct ResourceKey {
  std::uint8_t table=255;
  std::uint16_t index=kNullIndex;
  auto operator<=>(const ResourceKey &) const = default;
};
struct ResourceEntryView {
  ResourceKey key;
  std::uint8_t kind=255;
  std::span<const std::byte> payload;
  std::uint32_t consumed=0, field_visits=0;
};
struct ResourceTableView {
  StateScope scope;
  std::uint16_t layout_version=0;
  std::uint32_t epoch=0, previous_epoch=0;
  // 按(table,index)严格升序；仅借用调用者的不可变输入和表项scratch。
  std::span<const ResourceEntryView> entries;
};
struct ResourceCodecLimits {
  // 仅可靠Context的解码内存保护；完整Frame／首次准备／实际发送预算仍由外层累计。
  BodyCodecLimits body{32u<<20,262144,32,true};
};
struct ResourceCodecScratch { BodyCodecScratch body; };
// Client表owner复用资源唯一源的容量，不按旧目录或独立常量猜测图／内容上限。
struct ResourceProtocolBounds {std::uint32_t entries=0,edges=0,directory_bytes=0,dependency_depth=0;};
ResourceProtocolBounds StateResourceBounds() noexcept;
// 后台在资源工作入队前解出最终本地准备输入；返回文本由调用者的原快照延寿。
ResourceLocalDescription DescribeResourceLocal(const ResourceEntryView &,ResourceCodecScratch &);
// 编码输入和输出不得重叠；失败区域不可发布。没有媒体blob或任意函数分派入口。
ResourceEntryView DecodeResourceEntryPrefix(std::span<const std::byte>,ResourceCodecScratch &,ResourceCodecLimits={});
std::span<const std::byte> EncodeResourceEntryInto(const ResourceEntryView &,std::span<std::byte>,ResourceCodecScratch &,ResourceCodecLimits={});
ResourceTableView DecodeResourceTable(const StateScope &expected,std::span<const std::byte>,
    std::span<ResourceEntryView> entries,ResourceCodecScratch &,ResourceCodecLimits={});
std::span<const std::byte> EncodeResourceTableInto(const ResourceTableView &,std::span<std::byte>,ResourceCodecScratch &,ResourceCodecLimits={});

// 目录是自有纯值快照，不安装GPU／音频／游戏对象。owner必须延长全部借用字节的寿命。
// update保留本epoch原始更新，以便判断同epoch精确重传；旧内容不回收、不覆盖。
struct ResourceDirectoryView {
  std::uint64_t session=0, run=0;
  std::uint32_t epoch=0, previous_epoch=0;
  std::span<const ResourceEntryView> entries, update;
};
const ResourceEntryView *FindResource(const ResourceDirectoryView &,ResourceKey) noexcept;
// 供内嵌显示定义沿用正式资源目录的VFS路径合同。
bool StateImagePathValid(std::span<const std::byte> encoded) noexcept;
ResourceDirectoryView MergeResourceTableInto(const ResourceDirectoryView &,const ResourceTableView &,
    std::span<ResourceEntryView> storage,ResourceCodecScratch &,ResourceCodecLimits={});

struct ResourceEdge { ResourceKey target; };
struct ResourceGraphScratch {
  std::span<ResourceEdge> edges;
  std::span<std::uint32_t> offsets, order;
  std::span<std::uint8_t> state;
  std::span<std::uint8_t> heights;
};
class ResourceReadyContext {
public:
  virtual ~ResourceReadyContext() = default;
  // 只能在本地加载／生成完成、资源身份及动画／配置／几何符合描述后返回true。
  // 不在此查询里执行Lua／随机生产器／文件下载；失败和未完成都不能提前ACK。
  virtual bool Materialized(const ResourceDirectoryView &,const ResourceEntryView &) const = 0;
};
struct ResourceGraphView {
  bool closed=false, materialized=false;
  std::uint32_t edge_count=0, missing_count=0;
  std::span<const std::uint32_t> materialization_order;
};
// 目录epoch变化时构建一次，后续目标闭包只遍历这些数字边；不反复解码全目录payload。
// entries与边／offsets都借用自有不可变存储，覆盖scratch或退休目录后此计划失效。
struct ResourceGraphPlan {
  std::uint64_t session=0, run=0;
  std::uint32_t epoch=0;
  std::span<const ResourceEntryView> entries;
  std::span<const ResourceEdge> edges;
  std::span<const std::uint32_t> offsets;
};
ResourceGraphPlan BuildResourceGraph(const ResourceDirectoryView &,ResourceGraphScratch &,
    ResourceCodecScratch &,ResourceCodecLimits={});
ResourceGraphView PrepareResourceClosure(const ResourceDirectoryView &,const ResourceGraphPlan &,
    ResourceGraphScratch &,const ResourceReadyContext *ready=nullptr,std::span<const ResourceKey> roots={});
// 缺声明保留为未闭合，环／深度／非法用途则抛Gap；未闭合时不返回可执行物化顺序。
// 只有闭包与所有本地物化同时成立才materialized；metadata收齐不等于ready。
// roots非空时只准备当前目标的资源闭包；空时审阅整个目录，不让无关资源扩大body等待。
ResourceGraphView ValidateResourceGraph(const ResourceDirectoryView &,ResourceGraphScratch &,
    ResourceCodecScratch &,const ResourceReadyContext *ready=nullptr,ResourceCodecLimits={},
    std::span<const ResourceKey> roots={});

// 固定图形合同：profile不包含GL整数／地址；只查询同源数字规则，不执行图形操作。
std::span<const std::uint32_t> ResourceBlendProfile(std::uint8_t profile);
std::span<const std::uint32_t> ResourceSamplerProfile(std::uint8_t profile);
// Host纯值投影只匹配精确五元组，未知返回空；sampler按已核零／非零语义规范化。
std::optional<std::uint8_t> FindResourceBlendProfile(std::span<const std::uint32_t> values);
std::uint8_t MakeResourceSamplerProfile(std::uint32_t min_filter,std::uint32_t mag_filter,
    std::uint32_t wrap_s,std::uint32_t wrap_t) noexcept;
bool ResourceShaderAllowed(std::uint8_t use,std::uint8_t shader_type) noexcept;
// 对完整值或选中块检查实际资源引用及固定图形用途；仍不代替完整候选／原生提交。
void ValidateResourceBindings(std::uint16_t layout,std::uint32_t block_mask,BodyStream,
    std::span<const std::byte>,const ResourceDirectoryView &,const ResourceReadyContext &,
    BodyCodecScratch &,const BodyCodecContext *context=nullptr,BodyCodecLimits={});
// 已由正文 codec 定界的节点可复用同一资源用途与就绪检查。
void ValidateResourceBindingNode(const BodyNodeView &,const ResourceDirectoryView &,const ResourceReadyContext &);
// 为704等正文条件提供同resource_epoch的真实(type,id,cooldown)，不访问原生配置指针。
class ResourceBodyContext final : public BodyCodecContext {
public:
  explicit ResourceBodyContext(const ResourceDirectoryView &directory,const BodyCodecContext *shared=nullptr)
      :directory_(directory),shared_(shared) {}
  std::optional<CodecItemConfig> Item(std::uint8_t,std::uint16_t) const override;
  std::optional<bool> SharedPresence(const CodecLocation &location) const override {
    return shared_?shared_->SharedPresence(location):std::nullopt;
  }
private:
  ResourceDirectoryView directory_;
  const BodyCodecContext *shared_;
};
} // namespace cs::state
