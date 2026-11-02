#pragma once
// SDK-facing C++17 适配，只跨边界传递已复制的值，不导出 lua_State 或 Entity*。
#include <cstdint>
#include <string>
namespace cs::game {
// SDK适配头保持C++17；与公共合同的64M容量由非SDK协调器静态核对。
inline constexpr std::uint32_t kLuaObservationNodeBudget=64u<<20;
struct LuaEntityResult final {
  std::string payload, reason;
  bool complete=false, available=false;
  std::uint32_t objects=0, nodes=0;
};
struct ObservationSubjectValue final {
  std::uint64_t session=0,epoch=0,local=0,use=0,revision=0;
  std::uint32_t status=3,origin=0,readiness=0;
  std::uint32_t ordinal=UINT32_MAX;
  std::uint64_t participant=0,group=0,control_session=0,binding_revision=0,device_revision=0;
  bool control_active=false,group_known=false,body_active=false,can_consume=false;
  std::uint32_t body_role=6;
  std::uint32_t body_slot=UINT32_MAX;
};
struct EntitySupplementValue final {
  const char *class_name="UnknownEntity";
  const char *status="NotApplicable";
  std::uint32_t counter=0,duration=0,text_size=0;
  char text[256]{};
};
EntitySupplementValue EntitySupplement(void *object) noexcept;
std::uint32_t ObservationEntities(void *owner,void (*visit)(void *,void *,std::uint32_t) noexcept) noexcept;
const char *ObservationSpriteNames(void *sprite,void *owner,
    void (*visit)(void *,bool,const char *,std::size_t) noexcept) noexcept;
std::uint32_t ObservationGrids(void *owner,void (*visit)(void *,void *,std::uint32_t) noexcept) noexcept;
// 显式E2E只读取得最近同一普通采样批的字面标识，供Source公开回调固定截图时点。
std::string ObservationSourceSnapshotToken(std::string *reason=nullptr) noexcept;
void ObservationGridMemberships(void *grid,void *owner,void (*visit)(void *,std::uint32_t,std::uint32_t) noexcept) noexcept;
bool ObservationValidGrid(void *grid) noexcept;
void EnsureObservationLuaBridge() noexcept;
LuaEntityResult CaptureLuaEntity(std::uint32_t objects=16384,std::uint32_t nodes=kLuaObservationNodeBudget,
                                 std::uint32_t depth=12,std::uint32_t bytes=16u<<20,unsigned domain=0) noexcept;
// 最终原生来源复核/Authority提交后才更新Lua纯值历史；取消窗口不得污染后继。
void FinishLuaObservation(unsigned domain,std::uint64_t scope,bool committed) noexcept;
std::string ObservationCaptureContext() noexcept;
std::uint64_t ObservationSaveEntry() noexcept;
// 楼层上下文只传递公开逻辑键与自有值；SDK侧不包含游戏对象身份状态机。
struct LocationValue final { std::uint64_t session=0,epoch=0,floor=0,dimension=0,descriptor=0,run=0,room=0; };
LocationValue ObservationLocation(int stage,int type,std::uint32_t seed,int dimension,int list,int grid,const char *construction) noexcept;
std::uint64_t ObservationDescriptor(int dimension,int list,int grid,const char *construction) noexcept;
std::uint64_t ObservationDimension(int index) noexcept;
std::uint64_t ObservationDescriptorRevision(std::uint64_t id) noexcept;
void ObservationLocationBoundary(const char *name,bool floor) noexcept;
std::uint64_t ObservationRelatedSubject(unsigned space) noexcept;
bool ObservationLocationNeeded() noexcept;
bool ObservationPublishLocation(const char *payload,std::size_t bytes,bool complete,std::uint32_t nodes,const char *reason,unsigned domain,unsigned phase) noexcept;
bool ObservationLuaNeeded(const char *phase,bool grid=false) noexcept;
ObservationSubjectValue ObservationSubject(void *object,bool register_subject,bool grid=false) noexcept;
// 复用tx-04-09已核定拓扑，备份归属不按ordinal/ControllerIndex猜测。
void ObservationPlayerMembership(void *object,ObservationSubjectValue &value) noexcept;
bool ObservationPublishLua(const char *payload,std::size_t bytes,bool complete,
                           std::uint32_t objects,std::uint32_t nodes,const char *reason,unsigned phase,bool grid=false) noexcept;
}
