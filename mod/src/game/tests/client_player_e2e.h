#pragma once
#include "game/client/entity/client_objects_native.h"
namespace cs::game {
// 注入只在正常返回的具名边界停止；不模拟未知SEH，不改allocator或原版对象。
#ifdef ISAACCS_E2E
bool ClientPlayerTestFault(const char *,ClientObjectKey,std::uint64_t instance,unsigned prefix,unsigned slot=0,std::uint64_t child_generation=0) noexcept;
std::string ClientPlayerTestArm(const std::string &,const std::string &) noexcept;
std::string ClientPlayerTestStatus(const std::string &) noexcept;
struct ClientPlayerInjected final {const char *point;};
inline void ClientPlayerTestBoundary(const char *point,ClientObjectKey key,std::uint64_t instance,unsigned prefix){
  if(ClientPlayerTestFault(point,key,instance,prefix))throw ClientPlayerInjected{point};
}
#else
inline bool ClientPlayerTestFault(const char *,ClientObjectKey,std::uint64_t,unsigned,unsigned=0,std::uint64_t=0) noexcept {return false;}
inline void ClientPlayerTestBoundary(const char *,ClientObjectKey,std::uint64_t,unsigned) noexcept {}
#endif
}
