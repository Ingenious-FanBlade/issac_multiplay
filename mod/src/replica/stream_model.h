#pragma once
#include "replica/model.h"
#include "replica/stream_version.h"

namespace cs::replica::stream {
// 世界身份不携带连接；同一个不可变内容由三个Peer共享，路由只属于传输封套。
struct WorldScope final {
  std::uint64_t session=0,run=0,floor=0,dimension=0,room=0,scene=0;
  auto operator<=>(const WorldScope &) const = default;
};
Scope Connected(const WorldScope &,std::uint64_t connection);
} // namespace cs::replica::stream
