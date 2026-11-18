#pragma once
#include "game/observation/binary_production.h"
#include "game/observation/binary_display_inputs.h"
#include "game/observation/binary_intentions.h"
#include "state/production_cache.h"
#include "world/binary/entity_source.h"

namespace cs::game {
struct BinaryDisplayRead {
  void *context=nullptr;
  bool (*read)(void *,std::uint64_t,std::span<std::byte>) noexcept=nullptr;
  state::CaptureLink (*reference)(void *,std::uint32_t) noexcept=nullptr;
  std::span<const state::CaptureReferenceEntry> (*references)(void *) noexcept=nullptr;
  state::CaptureLink (*stable_reference)(void *,const state::CaptureReferenceEntry &) noexcept=nullptr;
};
// 正常生产元数据与ordinary值树分别拥有；所有动态区冷准备，逐帧不分配、不等待。
class BinaryDisplayCapture final {
public:
  explicit BinaryDisplayCapture(std::thread::id);
  ~BinaryDisplayCapture();
  BinaryDisplayCapture(const BinaryDisplayCapture &)=delete;
  BinaryDisplayCapture &operator=(const BinaryDisplayCapture &)=delete;
  bool Prepare(const state::CaptureWitness &,world::binary::EntityFacts &,world::binary::EntitySource &,
               BinaryDisplayRead,const BinaryDisplayInputs &,state::CaptureDiagnostic &) noexcept;
  bool Observe(BinaryProductionKind,std::uint32_t,std::span<const std::uint32_t>,const state::CaptureWitness &,
               world::binary::EntityFacts,world::binary::EntitySource &,BinaryDisplayRead,
               std::span<const state::CaptureReferenceEntry>,state::CaptureDiagnostic &,
               const state::CaptureReferenceEntry *event_player=nullptr) noexcept;
  bool Sfx(const state::CaptureWitness &,world::binary::EntityFacts,world::binary::EntitySource &,BinaryDisplayRead,
           int,float,int,bool,float,float,std::uint32_t,int,std::span<const state::CaptureReferenceEntry>,state::CaptureDiagnostic &) noexcept;
  bool Events(const state::StateScope &,state::CaptureValueWriter &,std::uint32_t &,state::CaptureDiagnostic &,state::CaptureAppendContext) noexcept;
  void MapFocus(std::uint32_t owner,std::uint32_t descriptor,std::uint32_t frame,std::uint32_t selected) noexcept;
  void Reset(bool new_run) noexcept;
  void BootstrapMusic(const BinaryMusicIntent &) noexcept;
  void BootstrapSfx(const BinarySfxIntent &) noexcept;
  // Start调用前预留，只给同一正常调用树内的Boss事件关联；拒绝请求允许留下序号空隙。
  std::uint64_t ReserveTransition() noexcept;
  void OverlayShown(std::uint32_t,std::int32_t) noexcept;
  void BossShown(std::uint32_t,std::uint32_t,std::uint32_t,std::int32_t,std::int32_t) noexcept;
  // 锁存导致显示源失效的第一条诊断，后续ordinary不能用Incomplete覆盖它。
  void Fault(const state::CaptureDiagnostic &) noexcept;
  state::CaptureDiagnostic FirstFailure() const noexcept;
  std::uint64_t RetainedBytes() const noexcept;
  std::uint64_t EventWatermark() const noexcept;
  // worker已让所有当前Peer可靠队列拥有该范围后，主线程才退休采集副本；这不是AppliedAck。
  void EventsTransferred(std::uint64_t watermark) noexcept;
  BinaryEventCaptureStats Statistics() const noexcept;
private:
  struct Impl;std::unique_ptr<Impl> impl_;
};
}
