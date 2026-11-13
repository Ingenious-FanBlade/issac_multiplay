#pragma once
#include "replica/model.h"
#include "replica/subject_key.h"
#include "replica/room_configuration.h"
#include "replica/room_grid_budget.h"
#include <array>
#include <memory>
#include <compare>

namespace cs::replica {
// 普通完整域保留各自的采样scope和序号，不借用独立Render/回调的时间。
struct DomainCapture final {
  std::uint64_t version=0,scope=0,sequence=0,tick=0;
};
// source_slot只对离图有效；普通storage_index与ListIndex一致，入口象限独立保留。
struct DescriptorLocation final {
  int grid_index=0,safe_grid_index=0,list_index=0,dimension=0,storage_index=-1,source_slot=0;
  // Source原始GridIndex保留在grid_index；native原点独立按已证地图占用表达，不回写异常来源。
  int native_grid_origin=0;
  bool raw_grid_index_adopted=true;
  std::vector<int> occupied;
  bool operator==(const DescriptorLocation &) const = default;
};
// descriptor实体使用space=6；位置只是当前代次的本地存储映射。
struct RoomDescriptorTarget final {
  SubjectKey key{};
  std::uint64_t construction_revision=0;
  DescriptorLocation location{};
  RoomConfiguration data{},override_data{};
  int allowed_doors=0,flags=0,display_flags=0,visited=0,clear_count=0;
  std::uint32_t spawn_seed=0,decoration_seed=0,award_seed=0;
  std::array<int,8> door_targets{};
  // 0=current_descriptor，1=level.descriptors，2=off_grid.entries；定位完整配置/专用字段。
  unsigned source_kind=0,source_index=0;
};
// 不把Null、缺席、Unavailable混为一谈；非Null引用必须由同目标或明确域依赖解决。
struct DomainReference final { bool is_null=false; SubjectKey key{}; };
// 路由哨兵与真实房间索引分开，Mirror/Mineshaft不能送入descriptor数组。
enum class DoorRoute : unsigned { None, Map, OffGrid, Mirror, Mineshaft };
// 一份Grid门主体由actual引用；shape允许、descriptor位和候选目标各自独立。
struct DoorBinding final {
  unsigned slot=0;
  bool shape_allowed=false,descriptor_enabled=false;
  int shape_target=0,actual_target=0;
  DomainReference actual{};
  DomainReference target_descriptor{true,{}};
  bool target_uninitialized=false;
  DoorRoute route=DoorRoute::None;
};
// 保存描述值与活动Grid状态分开，不用Init/Restore产生Authority未提供的结果。
struct GridDescriptorTarget final {
  int type=0,variant=0,state=0,var_data=0;
  std::uint32_t spawn_seed=0,variable_seed=0;
  int spawn_count=0;
  bool initialized=false;
};
// 搬起描述只属于 Effect136 owner；公开来源不提供 source_grid，保持 Unresolved。
// Entity19 通过此纯值接口安装其内嵌描述，不再创建一份活动 Grid，也不猜原槽。
struct LiftedGridTarget final {
  SubjectKey owner{};
  GridDescriptorTarget descriptor{};
  bool descriptor_null=false;
};
// source沿用原生来源11=格数组、12=门数组；alias不分配第二份主体。
struct GridMembership final {
  unsigned source=0,index=0;
  bool operator==(const GridMembership &) const = default;
};
// source_index定位自有DOM内的完整派生字段/资源，实例修订与稳定Key分开。
struct GridTarget final {
  SubjectKey key{};
  std::uint64_t instance_revision=0;
  int index=0,type=0,variant=0,state=0,collision=0,var_data=0;
  GridDescriptorTarget descriptor{};
  std::vector<GridMembership> memberships;
  std::vector<SubjectKey> references;
  unsigned source_index=0;
};
// capacity是存储上界，不表示每个格都在房形有效区域内；完整几何适配归I08。
struct RoomLayout final {
  int shape=0,width=0,height=0,capacity=448,grid_width=0,grid_height=0;
  bool operator==(const RoomLayout &) const = default;
};
// 环境与资源保持完整纯值子树，其字段的写回责任由逐成员目录声明；解析不等于原生支持。
struct EnvironmentTarget final {unsigned backdrop=0;float water=0;};
// 活动Room拥有当前描述、实际门绑定及已采的相关描述，不为缺失拓扑创建伪对象。
struct RoomTarget final {
  SubjectKey key{},floor{},descriptor{};
  int current_index=0,dimension=0;
  RoomLayout layout{};
  EnvironmentTarget environment{};
  std::vector<RoomDescriptorTarget> descriptors;
  std::array<DoorBinding,8> doors{};
};
// 调用方只能收紧预算；依赖边也有上限，避免有界步骤配出无界邻接表。
struct RoomGridLimits final {unsigned descriptors=526,grids=456,steps=8192,references=4096,dependency_edges=1048576;};
// 持续流局部值只声明四域，不把局部DTO伪装成完整十一域Baseline。
enum class RoomGridModelMode { CompleteWorld,StreamValues };
// 只通过shared_ptr<const>发布；Source是独立自有DOM，保留全部原coverage及未消费字段。
// 后续adapter按source_index取得专用字段，不持有Lua/SDK对象或外部DOM借用。
struct RoomGridTarget final {
  RoomGridBudgetUsage budget{};
  Scope scope{};
  std::uint64_t revision=0;
  DomainCapture room_capture{},grid_capture{},level_capture{};
  RoomTarget room{};
  std::vector<GridTarget> grids;
  std::vector<LiftedGridTarget> lifted;
  Json source;
  const Value &GridState(const GridTarget &) const;
  const Value &DescriptorState(const RoomDescriptorTarget &) const;
private:
  RoomGridTarget()=default;
  friend std::shared_ptr<const RoomGridTarget> ParseRoomGridTarget(const Value &,RoomGridLimits,RoomGridModelMode);
};
std::shared_ptr<const RoomGridTarget> ParseRoomGridTarget(const Value &,RoomGridLimits={},RoomGridModelMode=RoomGridModelMode::CompleteWorld);
// 只声明解析/规划的合同和预算；完整原生applier必须在I25以后单独声明。
Json RoomGridCapabilities();
const Json &RoomGridApplicationCatalog();
} // namespace cs::replica
