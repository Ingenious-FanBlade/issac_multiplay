#pragma once
#include "replica/stream_model.h"
#include "world/common/live_consumer.h"
#include <array>
#include "replica/stream_metrics.h"

namespace cs::replica::stream {
// 来源与后续wire/apply共用固定外层布局；不由报文或运行中的游戏对象决定必需字段。
const Layouts &CaptureLayouts();
// 一份发布持有精确版本闭包；网络拿共享引用，不复制三个世界，也不拥有游戏对象。
struct Publication final {
  struct Validity final {Version version;Sample acquired,checked;std::uint64_t invalidation=0;};
  WorldScope world;
  Range manifest,world_dynamic,player_fast;
  std::vector<Range> ranges;
  // acquired保留原取得时刻，checked说明目标本窗按完整有界读取复核，绝不重写旧版本的Sample。
  std::vector<Validity> validity;
};
struct CaptureStatus final {
  std::uint64_t ordinary=0,bodies=0,invalidations=0,world_sequence=0,body_gaps=0;
  std::uint64_t baseline_request_local_ms=0,baseline_published_local_ms=0;
  bool baseline_requested=true,ready=false;
  std::string reason="stream_capture_off",body_reason="player_post_not_updated";
};
// 逐身体证言只含身份和实际更新事实；原生地址留在Collector/桥的同步栈内。
struct BodySample final {
  std::uint64_t epoch=0,run=0,room=0,local=0,use=0,shape=0,tick=0,outer=0,owner=0,mutation=0,binding=0;
  CaptureKind kind=CaptureKind::Ordinary;
};
// 主线程唯一Host owner。普通窗口读完全部必需来源后才原子换目标；失败不会发布半套成员/删除。
class Capture final {
public:
  void Begin(std::uint64_t authority_session) noexcept;
  void Stop() noexcept;
  void Invalidate(const char *) noexcept;
  void RequestBaseline() noexcept {if(!status_.baseline_requested)status_.baseline_request_local_ms=MetricMicros()/1000;status_.baseline_requested=true;}
  bool NeedsBaseline() const noexcept {return status_.baseline_requested;}
  void Ordinary(const world::observation::LiveBatch &) noexcept;
  void PlayerPost(const BodySample &,const std::string &payload) noexcept;
  void BodyGap(const char *) noexcept;
  void PhysicalRetire(unsigned space,std::uint64_t epoch,std::uint64_t local,std::uint64_t use,
                      std::uint64_t shape,std::uint64_t event,std::uint64_t outer) noexcept;
  const CaptureStatus &Status() const noexcept {return status_;}
  std::shared_ptr<const Publication> Latest() const noexcept {return status_.ready?latest_:nullptr;}
  std::shared_ptr<const Publication> Baseline() const noexcept {return status_.ready?baseline_:nullptr;}
  std::vector<std::shared_ptr<const Publication>> Bodies() const;
  const Metrics &Statistics() const noexcept {return metrics_;}
  const Layouts &Schemas() const noexcept {return layouts_;}
private:
  void Scope(const world::observation::ObservationContext &);
  Member Subject(const Value &,const std::string &category,bool declare);
  Range Make(RangeData,bool reuse);
  void Collect();
  std::uint64_t Next();
  std::uint64_t authority_session_=0,source_session_=0,source_epoch_=0,source_run_=0,source_room_=0;
  std::uint64_t next_run_=0,next_scene_=0,next_revision_=0,last_capture_=0,last_tick_=0,loss_=0,invalidation_=0;
  WorldScope world_{};
  CaptureStatus status_{};
  Metrics metrics_;
  bool exhausted_=false;
  Layouts layouts_;
  std::unique_ptr<Registry> registry_;
  std::unique_ptr<Store> store_;
  std::map<RangeId,Range> current_;
  std::map<SubjectKey,Member> members_;
  struct Retirement final {Member member;std::uint64_t epoch=0,event=0,outer=0;};
  std::map<SubjectKey,Retirement> retirements_;
  std::map<SubjectKey,Range> bodies_;
  std::map<SubjectKey,std::shared_ptr<const Publication>> body_publications_;
  std::shared_ptr<const Publication> latest_,baseline_;
};
} // namespace cs::replica::stream
