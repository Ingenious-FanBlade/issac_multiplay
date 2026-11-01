#pragma once
#include "game/client_native_world.h"
#include "game/client_resources.h"
#include <array>
#include <cstdint>
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
// 此值合同只表达17已实现的代表路径；逐类全字段由18—20增加，不能默认忽略必需字段。
struct ClientObjectDescription final {
  ClientObjectKey key{};
  std::uint64_t shape_revision=0;
  unsigned type=0,variant=0,subtype=0,seed=0;
  ClientPoseVector position{},velocity{},position_offset{};
  float depth=0,size=0,mass=0,collision_damage=0;
  int entity_collision=0,grid_collision=0;
  bool visible=true;
  std::array<ClientObjectReference,4> references{}; // Parent、Child、Target、Spawner。
  ClientResourcePose resource{};
  // 普通Isaac的最终基础统计；构造期不执行Pre/Post起始物品玩法。
  bool player_stats=false;
  float damage=0,move_speed=0,max_fire_delay=0,shot_speed=0,tear_range=0,luck=0;
  int hearts=0,max_hearts=0,soul_hearts=0,black_hearts=0,eternal_hearts=0,coins=0,keys=0,bombs=0;
};
enum class ClientObjectOperation : unsigned { Remove, Create, Change, Replace, References, Check };
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
};
struct ClientNativeObjects;
ClientNativeObjects *ClientNativeObjectsCreate() noexcept;
void ClientNativeObjectsDestroy(ClientNativeObjects *) noexcept;
ClientObjectStep ClientNativeObjectsExecute(ClientNativeObjects *,const ClientNativeRoots &,const ClientObjectCommand &) noexcept;
// 全场景先处理没有Room成员的已Init池use，再撤映射；已发布实例由NativeWorld单一owner清理。
ClientObjectStep ClientNativeObjectsAbandon(ClientNativeObjects *,const ClientNativeRoots &) noexcept;
// I4只在当前借用内枚举已提交的原生实例；地址不允许存入网络或跨帧队列。
using ClientObjectVisitor=bool (*)(void *,void *,const ClientObjectKey &,std::uint64_t,unsigned) noexcept;
bool ClientNativeObjectsVisit(const ClientNativeObjects *,void *,ClientObjectVisitor) noexcept;
// 仅当前已提交实例的首次真实Body/Head原链诊断；不额外绘制，不改变原生调用处置。
void ClientObjectPlayerDrawDiagnostic(void *player,const void *position,bool body,bool after) noexcept;
// 唯一Hook清单中的Player Pre包装调用此函数；只跳过本次受控Init的同一Player。
bool ClientObjectSuppressPlayerPreStats(void *player) noexcept;
bool ClientObjectSkipPlayerPreStats() noexcept;
bool ClientObjectPreStatsIsolationInstalled() noexcept;
} // namespace cs::game
