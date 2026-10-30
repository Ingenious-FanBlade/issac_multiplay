#pragma once
// tx-04-12 有界纯值层级表：描述身份由楼层实例、维度和列表位置限定，地址不参与。
#include "world/common/contracts.h"
#include <array>
namespace cs::world::level {
struct Descriptor final { bool used=false; int dimension=0,list=0,grid=0; observation::SubjectKey subject{}; };
struct Hierarchy final {
  std::uint64_t run=0,room=0;
  int stage=0,type=0,dimension_index=-1;
  std::uint32_t seed=0;
  bool valid=false;
  observation::SubjectKey floor{},dimension{},descriptor{};
  std::array<observation::SubjectKey,3> dimensions{};
  std::array<Descriptor,1024> descriptors{};
  void Invalidate(bool new_floor) noexcept {
    valid=false; room=0; dimension={}; descriptor={}; dimension_index=-1;
    if(new_floor) { floor={}; dimensions={}; descriptors={}; }
  }
};
}
