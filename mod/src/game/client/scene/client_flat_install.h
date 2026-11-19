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
  state::BodyStream stream=state::BodyStream::World;
  bool fresh=false;
  const state::ClientFlatImage *player_world=nullptr; // Body 借用同一拓扑的 World 服装映射。
  void *context=nullptr;
  bool (*current)(void *) noexcept=nullptr;
  ClientFlatTarget (*target)(void *,const state::PresentationMember &) noexcept=nullptr;
  bool (*snapshot)(void *,const ClientAtomicWrite &,ClientAtomicReadRegion &) noexcept=nullptr;
};
struct ClientFlatNativeState {
  explicit ClientFlatNativeState(const std::shared_ptr<state::ClientStateMemory> &memory)
    :edges(state::ClientStateAllocator<ClientFlatTarget>(memory)) {}
  // 退休时从活 owner 重新定位；这里不保存可能随着 resize 失效的槽地址。
  state::ClientStateVector<ClientFlatTarget> edges;
  bool references_uncertain=true;
  state::BodyStream stream=state::BodyStream::World;
  std::uint32_t costume_topology=0;
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
ClientFlatNativeResult ClientWriteFlat(const ClientFlatNativeContext &,void *,const state::FlatLayout &,
    const state::ClientFlatWritePlan &,bool full,ClientFlatNativeState &) noexcept;
ClientFlatNativeResult ClientBindFlat(const ClientFlatNativeContext &,void *,const state::FlatLayout &,
    const state::ClientFlatWritePlan &,bool full,ClientFlatNativeState &) noexcept;
ClientFlatNativeResult ClientCheckFlat(const ClientFlatNativeContext &,void *,const state::FlatLayout &,
    const state::ClientFlatWritePlan &,ClientFlatNativeState &,std::uint32_t sample,bool graphics,bool full) noexcept;
bool ClientClearFlat(void *,const state::FlatLayout &,ClientFlatNativeState &,
                         bool retiring,std::span<const ClientLiveRetirement>) noexcept;
}
