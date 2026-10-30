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
void EnsureObservationLuaBridge() noexcept;
LuaEntityResult CaptureLuaEntity(std::uint32_t objects=16384,std::uint32_t nodes=262144,
                                 std::uint32_t depth=12,std::uint32_t bytes=16u<<20) noexcept;
bool ObservationLuaNeeded(const char *phase) noexcept;
ObservationSubjectValue ObservationSubject(void *object,bool register_subject) noexcept;
bool ObservationPublishLua(const char *payload,std::size_t bytes,bool complete,
                           std::uint32_t objects,std::uint32_t nodes,const char *reason,unsigned phase) noexcept;
}
