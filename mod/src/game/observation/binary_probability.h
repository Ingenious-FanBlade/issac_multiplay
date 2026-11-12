#pragma once
#include <array>
#include <cstdint>

namespace cs::game {
// GetAngelRoomChance本身是修正值；以下只读输入用于原生HUD同义拆分，不运行HUD重算器。
struct BinaryProbabilityInput {
  std::uint32_t game_flags=0,level_flags=0;
  std::int32_t donation=0,deals=0;
  float angel_modifier=0;
  bool eligible=false;
};
bool QueryBinaryHudProbability(void *game,void *room,std::uint8_t requested,const BinaryProbabilityInput &,std::array<float,3> &) noexcept;
}
