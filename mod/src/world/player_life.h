#pragma once
// 原生生命状态（幽灵化／复活）调用前的同实例证言。
//
// 本头只使用 C++17 可表达的值类型，因为生成的 SDK-facing Hook wrapper
// （hooks_generated.cpp，按项目约定以 C++17 编译）会经
// game/observation/world_entity_owner.h 直接持有该值。它不依赖 SDK、游戏头、
// windows.h 或任何运行时状态。

#include <array>
#include <cstdint>

namespace cs::world {

// 进入幽灵化／复活原生调用时锁定同一 Player 实例；只在本次调用栈内使用。
// revision 是身份表内的实例代次，shape 是 EntityType 三字，seed 是 InitSeed。
struct PlayerLifeWitness final {
  std::uint64_t run=0,local=0,use=0,revision=0;
  std::uint32_t address=0,seed=0;
  std::array<std::uint32_t,3> shape{};
  bool ghost=false;
};

} // namespace cs::world
