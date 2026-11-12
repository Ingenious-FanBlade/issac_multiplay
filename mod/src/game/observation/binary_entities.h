#pragma once
#include "state/capture_exchange.h"
#include "world/binary/entity_source.h"
#include "game/observation/binary_production.h"
#include <memory>

namespace cs::game {
struct WorldCapture;
struct BinaryEntityStats {
  bool active=false;
  std::uint64_t ordinary=0,bodies=0,joint=0,gaps=0;
  state::CaptureDiagnostic failure;
  state::CaptureExchangeStats exchange;
};
// 身份目录只保留物理身份，不可变；shape/binding/topology取同一packet的记录，不覆盖旧handle。
struct BinaryEntityCatalog {
  std::span<const state::CaptureReferenceEntry> references;
  world::binary::NativeResourceView resources;
};
// 主线程只生产纯值，现有world工作线程只收控制／退休排空；第11步再接编码与发送。
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
  void SceneBuild(std::uint32_t,bool entering,bool normal,bool replace) noexcept;
  void SceneGraphicsBoundary() noexcept;
  void SceneClear() noexcept;
  void SceneImageBefore(std::uint32_t,std::uint32_t,std::uint32_t,const std::array<std::uint32_t,4> &) noexcept;
  void SceneImageAfter(std::uint32_t) noexcept;
  std::uint32_t ProductionBegin(BinaryProductionKind,std::uint32_t object,std::uint32_t manager) noexcept;
  void ProductionEnd(std::uint32_t token,bool normal) noexcept;
  void ProductionDrop(std::uint32_t token,std::uint32_t player,bool held) noexcept;
  void ProductionMantle(std::uint32_t token,std::uint32_t effects) noexcept;
  void ProductionProgress(std::uint32_t token,bool seed,bool blocked) noexcept;
  void ProductionAudio(std::uint32_t token,int sound,float volume,float pitch,float pan,bool loop,bool world) noexcept;
  void ProductionAudioPan(std::uint32_t token,std::uint32_t stream) noexcept;
  void ProductionRoomEffect(std::uint32_t token,std::uint32_t effects,bool enabled) noexcept;
  void ProductionStat(std::uint32_t token,int index,std::uint32_t flags) noexcept;
  void ProductionPaused(std::uint32_t token,std::uint32_t game,bool paused) noexcept;
  void Background(bool stopping) noexcept;
  BinaryEntityStats Statistics() const noexcept;
  // 后续worker可绑定同一个真实consumer领取票据；不得借用游戏对象或主线程scratch。
  state::CaptureExchange::ReadTicket Take() noexcept;
  BinaryEntityCatalog Catalog(const state::CaptureExchange::ReadTicket &) const noexcept;
private:
  std::unique_ptr<Impl> impl_;
};
}
