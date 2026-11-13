#pragma once
#include "state/capture_exchange.h"
#include "world/binary/entity_source.h"
#include "game/observation/binary_production.h"
#include "game/observation/binary_intentions.h"
#include "state/host_publish.h"
#include <memory>

namespace cs::game {
struct WorldCapture;
struct BinaryEntityStats {
  bool active=false;
  std::uint64_t ordinary=0,bodies=0,joint=0,gaps=0;
  state::CaptureDiagnostic failure;
  state::CaptureExchangeStats exchange;
  BinaryEventCaptureStats events;
  std::uint64_t publication_revision=0;
  std::array<char,128> publication_failure{};
};
// 身份目录只保留物理身份，不可变；shape/binding/topology取同一packet的记录，不覆盖旧handle。
struct BinaryEntityCatalog {
  std::span<const state::CaptureReferenceEntry> references;
  world::binary::NativeResourceView resources;
};
// 主线程只生产纯值，world工作线程封存规范块，网络Peer独立组帧／确认与回收。
class BinaryEntities final {
public:
  struct Impl;
  BinaryEntities(WorldCapture &,std::thread::id producer,std::thread::id consumer);
  ~BinaryEntities();
  void Active(bool) noexcept;
  bool Active() const noexcept;
  void FrameBegin() noexcept;
  void FrameEnd() noexcept;
  void Prepare(bool normal) noexcept;
  void Commit(bool committed,const core::TickPostState &) noexcept;
  void Invalidate() noexcept;
  void RetireBody(std::uint32_t address) noexcept;
  void EntityChanged(std::uint32_t address) noexcept;
  void TextWritten(std::uint32_t address,std::uint64_t local,std::uint64_t use,std::uint64_t shape) noexcept;
  void ResourceChanged(std::uint32_t address) noexcept;
  void FloorBoundary(bool entering,bool normal) noexcept;
  void GridChanged(std::uint32_t address) noexcept;
  std::uint32_t ProductionBegin(BinaryProductionKind,std::uint32_t object,std::uint32_t manager) noexcept;
  void ProductionEnd(std::uint32_t token,bool normal) noexcept;
  void ProductionAudio(std::uint32_t token,int sound,float volume,float pitch,float pan,bool loop,bool world) noexcept;
  void ProductionAudioPan(std::uint32_t token,std::uint32_t stream) noexcept;
  void ProductionRequest(std::uint32_t token,const std::array<std::uint32_t,5> &) noexcept;
  void ProductionRandom(std::uint32_t token,std::uint32_t value) noexcept;
  void ProductionBannerText(std::uint32_t token,std::uint32_t text,bool subtitle) noexcept;
  void Intention(const BinaryMusicIntent &) noexcept;
  void Intention(const BinarySfxIntent &) noexcept;
  void MapFocus(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) noexcept;
  void OverlayShown(std::uint32_t,std::int32_t) noexcept;
  void BossShown(std::uint32_t,std::uint32_t,std::uint32_t,std::int32_t,std::int32_t) noexcept;
  void Background(bool stopping) noexcept;
  void NetworkPublication(bool enabled) noexcept;
  // 主线程只复制共享自有快照；取不到短锁时保留上次值，不借用游戏对象。
  state::HostStateSnapshotRef Publication() const noexcept;
  state::HostStateSnapshotRef OrdinaryPublication() const noexcept;
  BinaryEntityStats Statistics() const noexcept;
  // 唯一worker consumer领取票据；不得借用游戏对象或主线程scratch。
  state::CaptureExchange::ReadTicket Take() noexcept;
  BinaryEntityCatalog Catalog(const state::CaptureExchange::ReadTicket &) const noexcept;
  // 调用方先把该ordinary范围复制到所有当前Peer可靠队列；失败／停用不能提前回收。
  // 返回false表示票据已撤销或回执FIFO已满，worker保留票据后重试，不等待主线程。
  bool EventsTransferred(const state::CaptureExchange::ReadTicket &) noexcept;
private:
  std::unique_ptr<Impl> impl_;
};
}
