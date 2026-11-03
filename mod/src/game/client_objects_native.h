#pragma once
#include "game/client_native_world.h"
#include "game/client_resources.h"
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace cs::game {
// 稳定身份与地址分开；本地实例号只在本场景单调增加，不进入网络模型。
struct ClientObjectKey final {
  std::uint64_t run=0,id=0;
  unsigned space=0;
  bool operator==(const ClientObjectKey &v) const noexcept { return run==v.run&&id==v.id&&space==v.space; }
  bool operator<(const ClientObjectKey &v) const noexcept {
    return run!=v.run?run<v.run:space!=v.space?space<v.space:id<v.id;
  }
};
struct ClientObjectReference final { bool present=false,is_null=false;ClientObjectKey target{}; };
// C++17 SDK边界只接收自有值；字段状态由纯值层解释，不向原生传JSON地址。
enum class ClientEntityValueKind : unsigned { Integer, Bits, Real, Boolean, Vector, Color, Bits128 };
struct ClientEntityField final {
  std::string member;
  ClientEntityValueKind kind=ClientEntityValueKind::Integer;
  std::int64_t integer=0;
  std::uint64_t bits=0;
  std::uint64_t high_bits=0;
  float real=0;
  bool boolean=false;
  ClientPoseVector vector{};
  ClientPoseColor color{};
};
struct ClientEntityRelation final {
  std::string member,path,contract;
  ClientObjectKey target{};
  unsigned kind=0,expected=13;
  bool is_null=false,before_init=false,value_only=false;
};
// 17代表值和19公共值显式区分；派生完整值交给具名类别owner，不默认忽略必需字段。
struct ClientEntityClassState;
struct ClientObjectDescription final {
  ClientObjectKey key{};
  std::uint64_t shape_revision=0;
  unsigned type=0,variant=0,subtype=0,seed=0;
  unsigned authority_index=0; // 仅为相同Z的绘制次序，不覆盖本地Index/身份。
  ClientPoseVector position{},velocity{},position_offset{};
  float depth=0,size=0,mass=0,collision_damage=0;
  int entity_collision=0,grid_collision=0;
  bool visible=true;
  std::array<ClientObjectReference,4> references{}; // Parent、Child、Target、Spawner。
  ClientResourcePose resource{};
  // 正式19描述与17代表描述明确区分；派生值交给具名类别owner，不丢弃未应用字段。
  bool full_entity=false,exists=true,dead=false;
  unsigned category=13;
  std::vector<ClientEntityField> common_fields;
  std::vector<ClientEntityRelation> relations;
  std::string class_values,common_values,special_values;
  // 组件内graph_ref必须在整component解析；各对象共享不可变文本，不复制N份完整图。
  std::shared_ptr<const std::string> component_values;
  std::shared_ptr<const ClientEntityClassState> derived;
  // 普通Isaac的最终基础统计；构造期不执行Pre/Post起始物品玩法。
  bool player_stats=false;
  float damage=0,move_speed=0,max_fire_delay=0,shot_speed=0,tear_range=0,luck=0;
  int hearts=0,max_hearts=0,soul_hearts=0,black_hearts=0,eternal_hearts=0,coins=0,keys=0,bombs=0;
};
enum class ClientObjectOperation : unsigned { Remove, Create, Change, Replace, References, Check,
  Unlink, Acquire, Initialize, Fields, Resources, Publish };
struct ClientObjectCommand final {
  ClientObjectOperation operation=ClientObjectOperation::Check;
  ClientObjectDescription value{};
  bool fields=true,resource=true;
};
struct ClientObjectStep final {
  bool done=false,written=false,failed=false;
  unsigned executed_prefix=0,pending_owners=0;
  const char *reason="objects_waiting";
  ClientObjectKey key{};
  std::uint64_t instance=0;
  unsigned object_prefix=0;
  std::uint64_t pool_use=0,shape_revision=0;
};
struct ClientNativeObjects;
// 外部Player/跨域owner按稳定Key每次重新借用；不缓存Authority指针，也不回退到roster[0]。
struct ClientEntityBorrow final {void *object=nullptr;std::uint64_t instance=0,owner_epoch=0;unsigned category=13;};
using ClientEntityResolve=bool (*)(void *,const ClientNativeRoots &,const ClientObjectKey &,unsigned,ClientEntityBorrow &,const char *&) noexcept;
struct ClientEntityOwners final {
  void *context=nullptr;ClientEntityResolve resolve=nullptr;
  // 描述值/Weapon等不能假冒EntityPtr；依赖owner核对同scope的完整来源与寿命。
  bool (*dependency)(void *,const ClientNativeRoots &,const ClientEntityRelation &,const char *&) noexcept=nullptr;
  bool (*player_ready)(void *,const ClientNativeRoots &,const ClientObjectDescription &,const ClientEntityBorrow &,const char *&) noexcept=nullptr;
  // 外部owner退休先使token失效；回调context在所有消费端释放前保持存活。
  std::shared_ptr<void> lifetime;
};
// 类别适配负责自己的派生Init前置、资源、关系及清理；公共执行器统一掌握前缀和成员发布。
// 回调地址与context属于产品模块，绝不从网络注册；任何回调失败都保留已进入前缀。
struct ClientEntityClassAdapter final {
  unsigned category=13;
  void *context=nullptr;
  bool (*prepare)(void *,const ClientObjectDescription &,const char *&) noexcept=nullptr;
  bool (*before_init)(void *,void *,const ClientObjectDescription &,const ClientEntityOwners &,const ClientNativeRoots &,const char *&)=nullptr;
  // 可选窄Init作用域；按类别采用动态Init或已核base Init/官方Plus链，不允许另造或提前发布实例。
  bool (*initialize)(void *,void *,const ClientObjectDescription &,const char *&)=nullptr;
  bool (*fields)(void *,void *,const ClientObjectDescription &,bool,const char *&)=nullptr;
  bool (*resources)(void *,void *,const ClientObjectDescription &,bool,const char *&)=nullptr;
  bool (*relations)(void *,void *,const ClientObjectDescription &,const ClientEntityOwners &,const ClientNativeRoots &,bool,bool,const char *&)=nullptr;
  bool (*retire)(void *,void *,const ClientObjectDescription &,const char *&)=nullptr;
  // Dummy等不走通用Factory的类须提供具名取得路径，不能用默认NPC池冒充。
  void *(*acquire)(void *,const ClientObjectDescription &,const ClientNativeRoots &,const char *&)=nullptr;
  std::shared_ptr<void> lifetime;
};
bool ClientNativeObjectsConfigure(ClientNativeObjects *,const ClientEntityOwners &,const std::vector<ClientEntityClassAdapter> &) noexcept;
ClientEntityOwners ClientNativeObjectsPlayerOwner(ClientNativeObjects *) noexcept;
bool ClientNativeObjectsPreflight(ClientNativeObjects *,const ClientNativeRoots &,const std::vector<ClientObjectDescription> &,
    const std::vector<ClientObjectKey> &,const char *&) noexcept;
// 公共字段的安装/后态共用具名存储表；不调用Damage/Kill/Update推进玩法。
bool ClientEntityCommonFields(void *,const std::vector<ClientEntityField> &,bool check,const char *&) noexcept;
ClientNativeObjects *ClientNativeObjectsCreate() noexcept;
void ClientNativeObjectsDestroy(ClientNativeObjects *) noexcept;
ClientObjectStep ClientNativeObjectsExecute(ClientNativeObjects *,const ClientNativeRoots &,const ClientObjectCommand &) noexcept;
// 全场景先处理没有Room成员的已Init池use，再撤映射；已发布实例由NativeWorld单一owner清理。
ClientObjectStep ClientNativeObjectsAbandon(ClientNativeObjects *,const ClientNativeRoots &) noexcept;
// I4只在当前借用内枚举已提交的原生实例；地址不允许存入网络或跨帧队列。
using ClientObjectVisitor=bool (*)(void *,void *,const ClientObjectKey &,std::uint64_t,unsigned) noexcept;
bool ClientNativeObjectsVisit(const ClientNativeObjects *,void *,ClientObjectVisitor) noexcept;
bool ClientNativeObjectsCheckLifted(const ClientNativeObjects *,const ClientObjectKey &,const char *&) noexcept;
// 仅本次主线程draw栈借用已核实例描述；换T/退休使当前资格失效。
const ClientObjectDescription *ClientNativeObjectDrawDescription(void *) noexcept;
void *ClientNativeObjectDrawAddress(const ClientObjectKey &) noexcept;
// 仅当前已提交实例的首次真实Body/Head原链诊断；不额外绘制，不改变原生调用处置。
void ClientObjectPlayerDrawDiagnostic(void *player,const void *position,bool body,bool after) noexcept;
// 唯一Hook清单中的Player Pre包装调用此函数；只跳过本次受控Init的同一Player。
bool ClientObjectSuppressPlayerPreStats(void *player) noexcept;
bool ClientObjectSkipPlayerPreStats() noexcept;
bool ClientObjectPreStatsIsolationInstalled() noexcept;
// 主动类别Init中的临时随机输入不推进本地全局RNG；Off/Host和其它调用原样透传。
bool ClientEntityInitializationRandom(unsigned &value) noexcept;
bool ClientEntityInitIsolationInstalled() noexcept;
bool ClientEntitySuppressRetireSound() noexcept;
} // namespace cs::game
