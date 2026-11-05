#pragma once
#include "world/common/service.h"
#include <map>
#include <memory>
#include <atomic>

namespace cs::world::observation {
inline constexpr std::uint32_t kLatestConsumer=1,kRecorderConsumer=2,kLiveConsumer=3;
// 自有值的账本随最后一个后台引用释放；不把主线程owner指针捕获到共享值析构中。
struct LiveLedger final {std::atomic<std::uint64_t> bytes{0};std::uint64_t limit=256ull<<20;};
struct LiveBatch final {
  std::uint64_t session=0,epoch=0,capture=0,tick=0,outer=0,owner=0,mutation=0,loss=0,invalidation=0,event_watermark=0;
  std::uint32_t plan_revision=0;
  std::vector<std::shared_ptr<const ObservationEnvelope>> domains;
};
struct LiveStatus final {
  bool attached=false;
  std::uint64_t delivered=0,invalidations=0,gaps=0,bytes=0;
  char reason[128]="live_off";
};
// consumer 3只借用回调参数至返回；单份普通值按明确外层提交收束，多Peer取得同一个const共享批次。
class LiveConsumer final {
public:
  LiveConsumer():ledger_(std::make_shared<LiveLedger>()){}
  Consumer Bind(CapturePlan plan);
  void PlanAccepted(CapturePlan plan) noexcept {plan_=std::move(plan);Invalidate("live_plan_changed");}
  void Attached(bool) noexcept;
  void Finish(bool committed,bool stable,std::uint64_t loss,std::uint64_t watermark=0) noexcept;
  void Invalidate(const char *) noexcept;
  std::shared_ptr<const LiveBatch> Latest() const noexcept {return latest_;}
  LiveStatus Status() const noexcept;
private:
  static void Publish(void *,const ObservationEnvelope &) noexcept;
  static void Invalidated(void *,const char *) noexcept;
  void Receive(const ObservationEnvelope &) noexcept;
  std::shared_ptr<const ObservationEnvelope> Own(const ObservationEnvelope &);
  std::shared_ptr<LiveLedger> ledger_;
  std::map<DomainId,std::shared_ptr<const ObservationEnvelope>> pending_;
  std::shared_ptr<const LiveBatch> latest_;
  CapturePlan plan_;
  LiveStatus status_;
};
} // namespace cs::world::observation
