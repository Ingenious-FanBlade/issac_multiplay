#pragma once
// 已核定 native 来源的窄读取能力。域只能复制值及报告原因，不持有协调器可写状态。
#include "world/identity.h"
#include "world/storage.h"
namespace cs::world {
struct NativeSource final {
  void *owner=nullptr;
  bool (*copy)(void *,std::uint64_t,void *,std::size_t) noexcept=nullptr;
  void (*incomplete)(void *,Reason) noexcept=nullptr;
  std::uint32_t image_base=0,image_size=0,game=0,room=0,game_frame=0,grid_width=0;
  bool polluted=false;
  FaultSwitches *faults=nullptr;
  bool Read(std::uint64_t address,void *out,std::size_t size) const noexcept { return copy&&copy(owner,address,out,size); }
  template<class T> bool Value(std::uint64_t address,T &out) const noexcept { return Read(address,&out,sizeof(out)); }
  void Fail(Reason reason) const noexcept { if(incomplete) incomplete(owner,reason); }
};
Class ClassifyNative(const NativeSource &,ObjectDomain,std::uint32_t,std::uint32_t &,bool &) noexcept;
} // namespace cs::world
