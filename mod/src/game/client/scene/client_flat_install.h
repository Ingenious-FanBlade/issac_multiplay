#pragma once
#include "game/client/scene/client_atomic_install.h"
#include "game/client/scene/client_atomic_bindings_native.h"

namespace cs::game {
// 此表只借用当前 Manager 的活对象。后台镜像及票据始终是纯值。
struct ClientFlatTarget {
  state::PresentationMember identity;
  void *address=nullptr;
  std::uint64_t instance=0;
};
struct ClientFlatNativeContext {
  const ClientNativeRoots &roots;
  std::span<const ClientAtomicResource> resources;
  std::uint32_t frame=0;
  void *context=nullptr;
  bool (*current)(void *) noexcept=nullptr;
  ClientFlatTarget (*target)(void *,const state::PresentationMember &) noexcept=nullptr;
  bool (*snapshot)(void *,const ClientAtomicWrite &,ClientAtomicReadRegion &) noexcept=nullptr;
};
struct ClientFlatNativeState {
  struct Edge {std::uint32_t offset=0;ClientFlatTarget target;};
  // Tear 的引用槽全部在根内。同一实例续用时先沿用上次登记的真实边。
  std::array<Edge,8> edges{};
  std::uint8_t count=0;
  bool references_uncertain=true;
  state::ClientFlatImageRef written;
  // 特殊调用异常后持有原生回退承载的最后证言，禁止当成已提交旧值使用。
  ClientBinarySpecialWrite interrupted;
  ClientBinarySpecialUndo undo;
  memory::Lease special_charge;
};
struct ClientFlatNativeResult {
  const char *reason=nullptr;
  std::uint16_t layout=0,field=state::kNullIndex;
  std::uint32_t bytes=0;
  bool written=false;
};
ClientFlatNativeResult ClientWriteFlatTear(const ClientFlatNativeContext &,void *,const state::FlatLayout &,
    const state::ClientFlatWritePlan &,bool full,ClientFlatNativeState &) noexcept;
ClientFlatNativeResult ClientBindFlatTear(const ClientFlatNativeContext &,void *,const state::FlatLayout &,
    const state::ClientFlatWritePlan &,bool full,ClientFlatNativeState &) noexcept;
ClientFlatNativeResult ClientCheckFlatTear(const ClientFlatNativeContext &,void *,const state::FlatLayout &,
    const state::ClientFlatWritePlan &,ClientFlatNativeState &,std::uint32_t sample,bool graphics,bool full) noexcept;
bool ClientClearFlatTear(void *,const state::FlatLayout &,ClientFlatNativeState &,
                         bool retiring,std::span<const ClientLiveRetirement>) noexcept;
}
