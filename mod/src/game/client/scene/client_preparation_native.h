#pragma once
#include "state/resource_local.h"
#include "game/client/scene/client_native_roots.h"
#include <array>
#include <cstddef>

namespace cs::game {
// 仅主线程 owner 持有这些不透明承载；网络队列／完成反馈绝不携带它们。
struct ClientPreparedResource;
struct ClientStagedObject;
enum class ClientPreparationStatus:unsigned {Waiting,Ready,Missing,Failed};
struct ClientPreparationResult {
  ClientPreparationStatus status=ClientPreparationStatus::Waiting;
  const char *reason="binary_preparation_waiting";
  // 字段暂存走完当前配额后才置位；原生调用因 CPU 借用等待时不续做。
};
struct ClientPreparationAllocator {
  void *context=nullptr;
  void *(*allocate)(void *,std::size_t,std::size_t)=nullptr;
  void (*release)(void *,void *,std::size_t,std::size_t) noexcept=nullptr;
  void (*charge_native)(void *,std::size_t)=nullptr;
  void (*uncharge_native)(void *,std::size_t) noexcept=nullptr;
};
// 每次调用重新核 Manager、原生根、VM 与图形上下文；队列非空可等待，不抢清提交队列。
bool ClientPreparationWindow(const ClientNativeRoots &,void *vm) noexcept;
ClientNativeRoots ClientPreparationRoots() noexcept;
ClientPreparedResource *ClientPreparedResourceCreate(ClientPreparationAllocator) noexcept;
ClientPreparationResult ClientPreparedResourceLoad(ClientPreparedResource *,const state::ResourceLocalDescription &,
    ClientPreparedResource *first=nullptr,ClientPreparedResource *second=nullptr) noexcept;
bool ClientPreparedResourceCurrent(const ClientPreparedResource *) noexcept;
bool ClientPreparedResourceRelease(ClientPreparedResource *&) noexcept;
// 只在当前主线程用途栈借用资源；资源 owner 必须比最终原生使用者晚退休。
void *ClientPreparedResourceBorrow(ClientPreparedResource *) noexcept;
struct ClientPreparedNativeNeed;
struct ClientNativeNeedInput {
  unsigned kind=0;
  std::int64_t id=0,second_id=0;
  const std::int32_t *players=nullptr;std::size_t player_count=0;
};
ClientPreparedNativeNeed *ClientPreparedNativeNeedCreate(ClientPreparationAllocator) noexcept;
ClientPreparationResult ClientPreparedNativeNeedLoad(ClientPreparedNativeNeed *,const ClientNativeNeedInput &) noexcept;
bool ClientPreparedNativeNeedCurrent(const ClientPreparedNativeNeed *) noexcept;
bool ClientPreparedNativeNeedRelease(ClientPreparedNativeNeed *&) noexcept;

enum class ClientStagedKind:unsigned {Logical,Game,Level,Room,Sfx,Descriptor,Grid,Player,Entity};
struct ClientStagedObjectInput {
  ClientStagedKind kind=ClientStagedKind::Logical;
  std::uint16_t layout=0;
  std::uint32_t native_size=0,type=0,variant=0,subtype=0,seed=0,index=0,category=0;
  void *borrowed=nullptr; // 仅主线程真实 owner 解析的现有承载，不进入网络／纯值安装计划。
};
// 预先核对将使用的池类别与官方通知入口。容量在实际退休后逐对象分配。
ClientPreparationResult ClientStagedPoolNotificationsCurrent(const ClientNativeRoots &,const ClientStagedObjectInput *,std::size_t) noexcept;
ClientStagedObject *ClientStagedObjectCreate(ClientPreparationAllocator) noexcept;
// 新对象仅取得构造承载；不执行玩法 Init，不写 roster／EntityList／Grid 数组。
ClientPreparationResult ClientStagedObjectConstruct(ClientStagedObject *,const ClientNativeRoots &,const ClientStagedObjectInput &) noexcept;
bool ClientStagedObjectCurrent(const ClientStagedObject *,const ClientNativeRoots &) noexcept;
bool ClientStagedObjectRelease(ClientStagedObject *&) noexcept;
void *ClientStagedObjectBorrow(ClientStagedObject *) noexcept;
// 安装后的租约仍持有同一构造承载；不再用“未Init”的exists/valid条件判定其地址。
// 活对象的写入见证包含池中的有效位和完整原生形态。World 根允许读取或写入仍由本租约持有的不可见实例。
void *ClientStagedObjectLiveAddress(const ClientStagedObject *,bool allow_absent_entity=false) noexcept;
void *ClientStagedObjectOwnedAddress(const ClientStagedObject *) noexcept;
// Morph 已核对地址和形态；只更新实际池槽 owner 的活对象见证。
bool ClientStagedObjectAdoptShape(ClientStagedObject *,void *,unsigned type,unsigned variant,
    unsigned subtype,unsigned seed) noexcept;
} // namespace cs::game
