#pragma once
// SDK-facing C++17 适配，只跨边界传递已复制的值，不导出 lua_State 或 Entity*。
#include <cstdint>
#include <string>
namespace cs::game {
struct LuaEntityResult final {
  std::string payload, reason;
  bool complete=false, available=false;
  std::uint32_t objects=0, nodes=0;
};
struct ObservationSubjectValue final {
  std::uint64_t session=0,epoch=0,local=0,use=0,revision=0;
  std::uint32_t status=3,origin=0,readiness=0;
  std::uint32_t ordinal=UINT32_MAX;
  std::uint64_t participant=0,group=0,control_session=0,binding_revision=0;
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
void ObservationGridMemberships(void *grid,void *owner,void (*visit)(void *,std::uint32_t,std::uint32_t) noexcept) noexcept;
bool ObservationValidGrid(void *grid) noexcept;
void EnsureObservationLuaBridge() noexcept;
LuaEntityResult CaptureLuaEntity(std::uint32_t objects=16384,std::uint32_t nodes=262144,
                                 std::uint32_t depth=12,std::uint32_t bytes=16u<<20,unsigned domain=0) noexcept;
// 楼层上下文只传递公开逻辑键与自有值；SDK侧不包含游戏对象身份状态机。
struct LocationValue final { std::uint64_t session=0,epoch=0,floor=0,dimension=0,descriptor=0,run=0,room=0; };
LocationValue ObservationLocation(int stage,int type,std::uint32_t seed,int dimension,int list,int grid,const char *construction) noexcept;
std::uint64_t ObservationDescriptor(int dimension,int list,int grid,const char *construction) noexcept;
std::uint64_t ObservationDimension(int index) noexcept;
std::uint64_t ObservationDescriptorRevision(std::uint64_t id) noexcept;
void ObservationLocationBoundary(const char *name,bool floor) noexcept;
bool ObservationLocationNeeded() noexcept;
bool ObservationPublishLocation(const char *payload,std::size_t bytes,bool complete,std::uint32_t nodes,const char *reason,unsigned domain,unsigned phase) noexcept;
bool ObservationLuaNeeded(const char *phase,bool grid=false) noexcept;
ObservationSubjectValue ObservationSubject(void *object,bool register_subject,bool grid=false) noexcept;
bool ObservationPublishLua(const char *payload,std::size_t bytes,bool complete,
                           std::uint32_t objects,std::uint32_t nodes,const char *reason,unsigned phase,bool grid=false) noexcept;
}
