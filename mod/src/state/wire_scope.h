#pragma once
#include <compare>
#include <cstdint>
namespace cs::state {
// 公共50B头的六项稳定身份；资源目录仅按session/run复用，消息仍核完整scope。
struct StateScope {
  std::uint64_t session=0, run=0, floor=0, dimension=0, room=0, scene=0;
  auto operator<=>(const StateScope &) const = default;
};
}
