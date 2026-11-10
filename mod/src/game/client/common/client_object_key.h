#pragma once
#include <cstdint>

namespace cs::game {
// C/S稳定纯值身份独立于原生owner和旧描述；跨线程只传值，不传游戏地址。
struct ClientObjectKey final {
  std::uint64_t run=0,id=0;
  unsigned space=0;
  bool operator==(const ClientObjectKey &v) const noexcept {return run==v.run&&id==v.id&&space==v.space;}
  bool operator<(const ClientObjectKey &v) const noexcept {
    return run!=v.run?run<v.run:space!=v.space?space<v.space:id<v.id;
  }
};
} // namespace cs::game
