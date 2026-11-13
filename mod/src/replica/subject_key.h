#pragma once
#include <compare>
#include <cstdint>

namespace cs::replica {
// 沿用Registry分配的Key，不生成第二套身份；原生地址/槽位/seed不进入Key。
struct SubjectKey final {
  std::uint64_t run=0,id=0;
  unsigned space=0;
  auto operator<=>(const SubjectKey &) const = default;
};
}
