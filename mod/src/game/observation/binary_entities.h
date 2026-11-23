#pragma once
#include "state/capture_exchange.h"
#include "world/binary/entity_source.h"
#include "game/observation/binary_production.h"
#include "game/observation/binary_intentions.h"
#include "state/host_publish.h"
#include <memory>

namespace cs::game {
struct WorldCapture;
// 资源 Hook 的非主线程只上报水位，不访问游戏桥或主线程缓存。
void BinaryForeignResourceWrite(bool entering) noexcept;
struct BinaryEntityStats {
  bool active=false;
  std::uint64_t ordinary=0,bodies=0,joint=0,gaps=0;
  state::CaptureDiagnostic failure,first_object_failure;
  // 单根沿用、无 Body 资格和整帧失败分开计数。
  std::uint64_t object_failures=0,body_skipped=0;
  // 统计所有在采集边界收到的 SourceChanged，包括丢弃的 Body 候选。
  std::uint64_t source_changed=0;
  std::uint32_t retained_roots=0,max_retained_frames=0;
  state::CaptureExchangeStats exchange;
  state::HostMemberIndexStats member_indices;
  BinaryEventCaptureStats events;
  std::uint64_t publication_revision=0;
  std::uint64_t mod_missing_references=0,mod_encode_failures=0;
  world::binary::CaptureReadSession::Counters reads;
  world::binary::NativeSourceStats native;
  std::uint64_t body_reused=0,member_scans=0,scene_scans=0,roots=0,cells=0,paths=0,bytes=0;
  std::uint64_t transient_entity_retirements=0;
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
  bool Prepare(bool normal,std::uint64_t completed_tick) noexcept;
  void CommitTransition() noexcept;
  void Commit(bool committed,const core::TickPostState &) noexcept;
  void Invalidate() noexcept;
  void RetireBody(std::uint32_t address) noexcept;
  void EntityChanged(std::uint32_t address) noexcept;
  void TextWritten(std::uint32_t address,std::uint64_t local,std::uint64_t use,std::uint64_t shape) noexcept;
  void ResourceChanged(std::uint32_t address,const char *operation) noexcept;
  void SharedResourceChanged(bool animation) noexcept;
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
  // 主线程授权纯值邮箱，网络线程从邮箱读取，不取得采集对象或原生地址。
  std::shared_ptr<const state::HostStatePublication> Publication() const noexcept;
  bool ModIntentWorld(net::ModIntent &) const noexcept;
  void ModSceneIdentity(std::uint64_t &run,std::uint64_t &floor) const noexcept;
  std::uint16_t ModIntentMember(const void *) const noexcept;
  void *ResolveModIntentMember(const net::ModIntent &,std::uint16_t) const noexcept;
  BinaryEntityStats Statistics() const noexcept;
  std::string WorkerFields(unsigned page) const;
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
