#include "replica/stream_capture_worker.h"
#include <condition_variable>
#include <cstring>
#include <thread>
#include <algorithm>

namespace cs::replica::stream {
struct CaptureWorker::Output final {
  memory::Lease memory;
  std::uint64_t generation=0,watermark=0,baseline_request=0;
  CaptureStatus status;Metrics metrics;
  std::shared_ptr<const Publication> latest,baseline;
  std::vector<std::shared_ptr<const Publication>> bodies;
};
struct CaptureWorker::State final {
  struct ControlItem {unsigned kind=0;std::uint64_t order=0,generation=0,watermark=0;std::array<std::uint64_t,7> values{};std::array<char,128> reason{};};
  struct Job {std::uint64_t order=0,generation=0,watermark=0,baseline_request=0;std::shared_ptr<const world::observation::LiveBatch> ordinary;BodySample body;capture::Source source;};
  // 租约先构造、最后析构，固定控制槽和队列释放前不提前退还预算。
  memory::Lease memory=memory::Reserve(memory::Pool::Values,sizeof(State)+128,"capture_worker_slots");
  // 一个在建、一个最新普通候选，另有不被合并覆盖的冷候选及固定八身体槽。
  std::mutex inbox,outbox;std::condition_variable wake;
  // 控制为单主线程生产/单worker消费的独立环；主线程不因普通槽短锁竞争丢失退休事实。
  std::array<ControlItem,256> controls{};std::atomic<unsigned> control_head{0},control_tail{0};
  Job ordinary,cold;std::array<Job,8> bodies{};
  std::shared_ptr<const Output> output;
  std::atomic<bool> quit{false},failed{false};
  std::atomic<std::uint64_t> generation{0},watermark{0},requests{0},dropped{0},body_gaps{0};
  void Run() noexcept {
    Capture capture;std::uint64_t current=0,requested=0;
    for(;;){Job job;ControlItem control;bool has_control=false;
      {std::unique_lock lock(inbox);wake.wait_for(lock,std::chrono::milliseconds(10),[&]{return quit||control_head.load()!=control_tail.load()||ordinary.order||cold.order||std::any_of(bodies.begin(),bodies.end(),[](const auto &v){return v.order!=0;});});
        if(quit)break;
        if(failed){if(ordinary.order)++dropped;if(cold.order)++dropped;for(const auto &body:bodies)if(body.order)++body_gaps;ordinary={};cold={};bodies={};}
        Job *next=nullptr;for(auto *candidate:{&cold,&ordinary})if(candidate->order&&(!next||candidate->order<next->order))next=candidate;
        for(auto &body:bodies)if(body.order&&(!next||body.order<next->order))next=&body;
        const auto head=control_head.load(std::memory_order_relaxed),tail=control_tail.load(std::memory_order_acquire);
        if(head!=tail&&(!next||controls[head%controls.size()].order<next->order)){control=controls[head%controls.size()];control_head.store(head+1,std::memory_order_release);has_control=true;}
        else if(next){job=std::move(*next);*next={};}
      }
      if(failed)capture.Stop();
      if(has_control){
        if(control.kind==0){current=control.generation;requested=0;capture.Begin(control.values[0]);}
        else if(control.kind==1){capture.Stop();current=control.generation;}
        else if(control.generation==current){if(control.kind==2)capture.Invalidate(control.reason.data());
          else if(control.kind==3){const auto &v=control.values;capture.PhysicalRetire(unsigned(v[0]),v[1],v[2],v[3],v[4],v[5],v[6]);}}
      }
      if(job.order){
        if(job.generation!=current||job.generation!=generation.load()||job.watermark!=watermark.load()){if(job.source)++body_gaps;else ++dropped;continue;}
        if(job.ordinary){
          // 请求只能由请求之后取得的完整ordinary满足；正在构建的旧候选不能冒充新冷来源。
          if(job.baseline_request!=requested){capture.RequestBaseline();requested=job.baseline_request;}
          capture.Ordinary(*job.ordinary);
        }
        else if(job.source)capture.PlayerPost(job.body,job.source);
        // 构建中发生退休/失效时丢弃整个结果；不允许旧代次在Begin/Stop之后重新发布。
        if(job.generation!=generation.load()||job.watermark!=watermark.load()){capture.Invalidate("stream_build_invalidated");if(job.source)++body_gaps;else ++dropped;continue;}
      }
      if(!has_control&&!job.order)continue;
      try {
        auto output_memory=memory::Reserve(memory::Pool::Values,sizeof(Output)+2048,"capture_worker_output");
        auto out=memory::DeferredValue<Output>();out->memory=std::move(output_memory);
        out->generation=current;out->watermark=has_control?control.watermark:job.watermark;out->baseline_request=requested;
        out->status=capture.Status();out->metrics=capture.Statistics();
        out->latest=capture.Latest();out->baseline=capture.Baseline();out->bodies=capture.Bodies();
        if(out->generation!=generation.load()||out->watermark!=watermark.load())continue;
        std::shared_ptr<const Output> retired;{std::lock_guard lock(outbox);retired=std::move(output);output=std::move(out);}
      }catch(...){if(current==generation.load()){failed=true;++watermark;}capture.Invalidate("stream_output_budget");}
    }
    // 本线程清空全部纯值；析构不join主线程，DLL沿既定常驻生命周期保留。
    capture.Stop();ordinary={};cold={};bodies={};output.reset();
  }
};
CaptureWorker::CaptureWorker():state_(std::make_shared<State>()){auto state=state_;std::thread([state]{state->Run();}).detach();}
CaptureWorker::~CaptureWorker(){state_->quit=true;++state_->watermark;state_->wake.notify_one();}
// 控制项不能随普通候选合并；环溢出立即撤资格，直到新的显式Begin。
void CaptureWorker::Control(unsigned kind,const char *reason,std::array<std::uint64_t,7> values) noexcept {
  auto &s=*state_;const auto watermark=++s.watermark;
  const auto tail=s.control_tail.load(std::memory_order_relaxed),head=s.control_head.load(std::memory_order_acquire);
  if(tail-head>=s.controls.size()||order_==UINT64_MAX||!watermark){s.failed=true;s.wake.notify_one();return;}
  auto &item=s.controls[tail%s.controls.size()];item={};item.kind=kind;item.order=++order_;item.generation=generation_;item.watermark=watermark;item.values=values;
  if(reason)std::strncpy(item.reason.data(),reason,item.reason.size()-1);
  if(kind==0)s.failed=false;s.control_tail.store(tail+1,std::memory_order_release);s.wake.notify_one();
}
void CaptureWorker::Begin(std::uint64_t session) noexcept {
  if(generation_==UINT64_MAX||state_->requests.load()==UINT64_MAX){state_->failed=true;state_->wake.notify_one();return;}
  ++generation_;state_->generation=generation_;required_capture_=0;++state_->requests;last_body_gap_={};Control(0,"stream_begin",{session});
}
void CaptureWorker::Stop() noexcept {
  if(generation_==UINT64_MAX){state_->failed=true;state_->wake.notify_one();return;}
  ++generation_;state_->generation=generation_;required_capture_=0;Control(1,"stream_capture_off");
}
void CaptureWorker::Invalidate(const char *reason) noexcept {Control(2,reason);}
void CaptureWorker::PhysicalRetire(unsigned space,std::uint64_t epoch,std::uint64_t local,std::uint64_t use,std::uint64_t shape,std::uint64_t event,std::uint64_t outer) noexcept {Control(3,"stream_physical_retirement",{space,epoch,local,use,shape,event,outer});}
void CaptureWorker::RequestBaseline() noexcept {if(state_->requests.load()==UINT64_MAX)state_->failed=true;else ++state_->requests;state_->wake.notify_one();}
std::shared_ptr<const CaptureWorker::Output> CaptureWorker::Read() const noexcept {
  {std::unique_lock lock(state_->outbox,std::try_to_lock);if(lock)view_=state_->output;}
  if(state_->failed||!view_||view_->generation!=state_->generation.load()||view_->watermark!=state_->watermark.load())return {};return view_;
}
bool CaptureWorker::NeedsBaseline() const noexcept {
  Read();
  // dirty水位只撤新目标资格，不把已完成的冷请求改回未完成；否则每次Grid更新
  // 都会触发Random等全域冷读取。新的会话/显式请求仍必须取得新的完整冷来源。
  return state_->failed||!view_||view_->generation!=state_->generation.load()||
    view_->baseline_request!=state_->requests.load()||view_->status.baseline_requested;
}
void CaptureWorker::Ordinary(std::shared_ptr<const world::observation::LiveBatch> values) noexcept {
  if(!values)return;required_capture_=values->capture;auto &s=*state_;std::unique_lock lock(s.inbox,std::try_to_lock);
  if(order_==UINT64_MAX){s.failed=true;s.wake.notify_one();return;}
  if(!lock||s.failed){++s.dropped;return;}State::Job job;job.order=++order_;job.generation=generation_;job.watermark=s.watermark.load();job.baseline_request=s.requests.load();job.ordinary=std::move(values);
  // 冷请求保留第一份完整候选；其它ordinary仅替换尚未开始的普通槽。
  if(NeedsBaseline()&&!s.cold.order)s.cold=std::move(job);
  else {if(s.ordinary.order)++s.dropped;s.ordinary=std::move(job);}s.wake.notify_one();
}
void CaptureWorker::PlayerPost(BodySample body,capture::Source source) noexcept {
  auto &s=*state_;body.required_capture=required_capture_;std::unique_lock lock(s.inbox,std::try_to_lock);
  if(order_==UINT64_MAX){s.failed=true;s.wake.notify_one();return;}
  if(!lock||s.failed||!body.required_capture){++s.body_gaps;return;}
  State::Job *slot=nullptr;for(auto &candidate:s.bodies){if(candidate.order&&candidate.body.local==body.local&&candidate.body.use==body.use){slot=&candidate;break;}if(!candidate.order&&!slot)slot=&candidate;}
  if(!slot){++s.body_gaps;return;}if(slot->order)++s.body_gaps;
  *slot={++order_,generation_,s.watermark.load(),s.requests.load(),{},body,std::move(source)};s.wake.notify_one();
}
void CaptureWorker::BodyGap(const char *reason) noexcept {++state_->body_gaps;last_body_gap_={};if(reason)std::strncpy(last_body_gap_.data(),reason,last_body_gap_.size()-1);}
CaptureStatus CaptureWorker::Status() const noexcept {try{CaptureStatus status;if(const auto out=Read())status=out->status;else status.reason=state_->failed?"stream_control_or_output_overflow":"stream_build_pending_or_revoked";status.body_gaps+=state_->body_gaps.load();if(last_body_gap_[0])status.body_reason=last_body_gap_.data();return status;}catch(...){return {};}}
Metrics CaptureWorker::Statistics() const noexcept {Metrics metrics;if(const auto out=Read())metrics=out->metrics;metrics.replacements+=state_->dropped.load();return metrics;}
std::shared_ptr<const Publication> CaptureWorker::Latest() const noexcept {const auto out=Read();return out&&out->status.ready?out->latest:nullptr;}
std::shared_ptr<const Publication> CaptureWorker::Baseline() const noexcept {const auto out=Read();return out&&out->status.ready&&!out->status.baseline_requested&&out->baseline_request==state_->requests.load()?out->baseline:nullptr;}
std::vector<std::shared_ptr<const Publication>> CaptureWorker::Bodies() const {const auto out=Read();return out&&out->status.ready?out->bodies:std::vector<std::shared_ptr<const Publication>>{};}
}
