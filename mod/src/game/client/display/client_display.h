#pragma once
#include "state/client_shadow.h"
#include "game/client/room/client_room_description.h"
#include "game/client/scene/client_native_roots.h"
#include "game/client/display/client_display_bridge.h"
#include "game/client/display/client_display_native.h"
#include <memory>

namespace cs::game {
// 选择器来自同一候选的正式身份解析；报文中的数字槽位不能直接当本地地址。
struct ClientDisplaySelection final {
  state::PresentationMember game,level,room,sfx,descriptor;
};
struct ClientDisplayStorage;
// 完整world的显示安装片段，持有原始不可变影子及地图描述；不持有游戏指针。
class ClientDisplayWorld final {
public:
  // 仅由后台描述工厂交入自有不可变存储，允许Peer分配器同时计包装对象与控制块。
  explicit ClientDisplayWorld(std::shared_ptr<const ClientDisplayStorage>);
  const state::PresentationBatchContext &Context() const noexcept;
  const ClientDisplaySelection &Selection() const noexcept;
  const ClientRoomLevelDescription &Rooms() const noexcept;
  std::span<const ClientDisplayCopy> Copies() const noexcept;
  std::uint32_t TimeBase() const noexcept;
  bool Paused() const noexcept;
  std::uint8_t Branch() const noexcept;
  float SfxModifier() const noexcept;
  const ClientDisplayMetadata &Metadata() const noexcept;
  const ClientDisplayInjections &Inputs() const noexcept;
  std::span<const std::uint64_t> MapFacts() const noexcept;
  long StorageReferences()const noexcept;
  ClientDisplayMemoryRef Memory() const noexcept;
private:
  friend std::shared_ptr<const ClientDisplayWorld> ClientDescribeDisplayWorld(
    state::ClientShadowStateRef,const ClientDisplaySelection &,ClientRoomLevelDescription,std::shared_ptr<state::ClientStateMemory>);
  std::shared_ptr<const ClientDisplayStorage> storage_;
};
using ClientDisplayWorldRef=std::shared_ptr<const ClientDisplayWorld>;
// 第12步完成来源／拓扑／资源检查后准备；失败不触碰原生世界，也不提交ACK。
ClientDisplayWorldRef ClientDescribeDisplayWorld(state::ClientShadowStateRef,
    const ClientDisplaySelection &,ClientRoomLevelDescription,std::shared_ptr<state::ClientStateMemory> memory={});

} // namespace cs::game
