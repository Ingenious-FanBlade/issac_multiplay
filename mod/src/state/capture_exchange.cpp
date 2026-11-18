#include "state/capture_exchange.h"
#include <chrono>
#include "state/budget.h"
#include <atomic>
#include <utility>

namespace cs::state {
namespace {
std::uint64_t CaptureClock() noexcept {
  return std::uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}
constexpr std::uint32_t Empty=UINT32_MAX;
enum SlotState : std::uint32_t {Free,Writing,Ready,Reading};
static_assert(std::atomic<std::uint32_t>::is_always_lock_free);
}
struct CaptureExchange::State {
  struct Slot {
    std::unique_ptr<CaptureBuffer> buffer;
    std::atomic<std::uint32_t> state{Free},serial{0};
    Key key;
    std::uint32_t order=0;
    std::uint64_t queued_us=0;
    CapturePacket packet;
  };
  // State租约先建最后释放；每个CaptureBuffer另外为自身实际capacity记账，引用不重复收费。
  memory::Lease memory;
  CaptureExchangeConfig config;
  std::thread::id producer,consumer;
  std::unique_ptr<Slot[]> slots;
  std::atomic<std::uint32_t> generation{0},watermark{0},request{0},failed{0},failure{0},active{0};
  std::atomic<std::uint32_t> ordinary{Empty},cold{Empty};
  std::array<std::atomic<std::uint32_t>,256> bodies;
  std::array<std::atomic<std::uint32_t>,256> body_generations{};
  std::array<CaptureSubject,256> body_keys{}; // 只有producer读写，不随consumer读取而退休身体。
  std::array<CaptureControl,257> controls{};
  std::atomic<std::uint32_t> head{0},tail{0};
  std::atomic<std::uint32_t> published{0},replaced{0},revoked{0},reading{0};
  std::uint32_t serial=0,order=0,cold_enqueued=0; // 单producer独占，停止后也不回绕复用。
  std::uint32_t next_lane=0; // 单consumer独占；ordinary与各身体轮转。
  State(CaptureExchangeConfig cfg,std::thread::id p,std::thread::id c):config(cfg),producer(p),consumer(c) {
    if(cfg.buffers<4 || cfg.buffers>260 || !cfg.body_lanes || cfg.body_lanes>256 || p==std::thread::id{} ||
       c==std::thread::id{} || p==c || (cfg.ordinary_buffers && (cfg.ordinary_buffers<3 || cfg.ordinary_buffers>=cfg.buffers)))
      throw std::invalid_argument("capture_exchange_config");
    const auto estimate=std::uint64_t(cfg.buffers-cfg.ordinary_buffers)*CaptureBuffer::EstimatedBytes(cfg.capacity)+
      std::uint64_t(cfg.ordinary_buffers)*CaptureBuffer::EstimatedBytes(cfg.ordinary_capacity)+sizeof(State)+std::uint64_t(cfg.buffers)*sizeof(Slot);
    if(estimate>StateBudgets().shared_value_bytes)
      throw std::invalid_argument("capture_exchange_shared_capacity");
    memory=memory::Reserve(memory::Pool::Values,sizeof(State)+128+std::uint64_t(cfg.buffers)*sizeof(Slot),"state_capture_exchange");
    slots=std::make_unique<Slot[]>(cfg.buffers);
    auto retained=memory->Bytes();
    for(std::uint32_t i=0;i<cfg.buffers;++i){slots[i].buffer=std::make_unique<CaptureBuffer>(i<cfg.ordinary_buffers?cfg.ordinary_capacity:cfg.capacity);retained+=slots[i].buffer->RetainedBytes();}
    if(retained>StateBudgets().shared_value_bytes)throw std::invalid_argument("capture_exchange_retained_capacity");
    for(auto &body:bodies)body.store(Empty,std::memory_order_relaxed);
  }
  void Flag(CaptureFailure reason) noexcept {
    std::uint32_t none=0;failure.compare_exchange_strong(none,std::uint32_t(reason),std::memory_order_acq_rel);
    failed.store(1,std::memory_order_release);
  }
  bool Producer() noexcept {if(std::this_thread::get_id()==producer)return true;Flag(CaptureFailure::WrongThread);return false;}
  bool Consumer() noexcept {if(std::this_thread::get_id()==consumer)return true;Flag(CaptureFailure::WrongThread);return false;}
  bool Current(const Key &key) const noexcept {
    return !failed.load(std::memory_order_acquire) && active.load(std::memory_order_acquire) &&
      key.generation==generation.load(std::memory_order_acquire) && key.watermark==watermark.load(std::memory_order_acquire) &&
      (key.lane!=CaptureLane::Body || (key.body_slot<config.body_lanes && key.body_generation==body_generations[key.body_slot].load(std::memory_order_acquire)));
  }
  bool Owned(const Key &key,SlotState expected) const noexcept {
    return key.slot<config.buffers && slots[key.slot].serial.load(std::memory_order_acquire)==key.serial &&
      slots[key.slot].state.load(std::memory_order_acquire)==expected;
  }
  // 替换只释放仍在mailbox的Ready槽；consumer取走的Reading槽绝不回收或覆盖。
  void Clear(std::atomic<std::uint32_t> &lane) noexcept {
    const auto old=lane.exchange(Empty,std::memory_order_acq_rel);
    if(old!=Empty){slots[old].state.store(Free,std::memory_order_release);revoked.fetch_add(1,std::memory_order_relaxed);}
  }
  void ClearPending() noexcept {Clear(ordinary);Clear(cold);for(std::uint32_t i=0;i<config.body_lanes;++i)Clear(bodies[i]);}
  void Terminal(CaptureFailure reason) noexcept {Flag(reason);active.store(0,std::memory_order_release);ClearPending();}
  bool Next(std::uint32_t &value) noexcept {if(value==UINT32_MAX){Terminal(CaptureFailure::SequenceExhausted);return false;}++value;return true;}
  bool AllFree() const noexcept {
    for(std::uint32_t i=0;i<config.buffers;++i)if(slots[i].state.load(std::memory_order_acquire)!=Free)return false;
    return true;
  }
  bool Push(CaptureControlKind kind,CaptureFailure reason,CaptureSubject subject) noexcept {
    const auto write=tail.load(std::memory_order_relaxed),next=write==256?0:write+1;
    if(next==head.load(std::memory_order_acquire)){Terminal(CaptureFailure::ControlOverflow);return false;}
    if(!Next(order))return false;
    controls[write]={kind,reason,generation.load(std::memory_order_relaxed),watermark.load(std::memory_order_relaxed),
                     order,request.load(std::memory_order_relaxed),subject};
    tail.store(next,std::memory_order_release);return true;
  }
  // 只有真实外层已提交、当前证言仍有效时才能转Ready；此后producer不再碰packet内容。
  bool Commit(Key key,CaptureValueWriter &writer,const CaptureWindow &window,const CaptureRoot &anchor,std::uint32_t roots) noexcept {
    if(!Producer()){writer.Fail(CaptureFailure::WrongThread);return false;}
    if(!Owned(key,Writing) || !Current(key)){writer.Fail(CaptureFailure::Revoked);return false;}
    if(key.lane==CaptureLane::Body && anchor.subject!=key.subject){writer.Fail(CaptureFailure::InvalidWindow);return false;}
    CaptureDiagnostic diagnostic;
    if(window.witness!=writer.Witness()){writer.Fail(CaptureFailure::InvalidWindow);return false;}
    if(!window.Check(anchor,diagnostic)){writer.Fail(diagnostic);return false;}
    auto &slot=slots[key.slot];
    if(!writer.Seal(roots,slot.packet))return false;
    if(!window.Check(anchor,diagnostic)){writer.Fail(diagnostic);return false;}
    if(!Current(key)){writer.Fail(CaptureFailure::Revoked);return false;}
    std::atomic<std::uint32_t> *lane=&ordinary;
    if(key.lane==CaptureLane::Body){
      const auto chosen=key.body_slot;
      if(chosen>=config.body_lanes || body_keys[chosen]!=key.subject){writer.Fail(CaptureFailure::Revoked);return false;}
      bool has_subject=false;
      for(const auto &record:slot.packet.records){
        if(record.stream==BodyStream::World){writer.Fail(CaptureFailure::InvalidValue);return false;}
        if(record.subject==key.subject)has_subject=true;
      }
      if(!has_subject){writer.Fail(CaptureFailure::InvalidValue);return false;}
      body_keys[chosen]=key.subject;lane=&bodies[chosen];key.cold_request=0;
    }else if(key.cold_request==request.load(std::memory_order_relaxed) && key.cold_request!=cold_enqueued){
      lane=&cold;cold_enqueued=key.cold_request;
    }else key.cold_request=0;
    if(!Next(order))return false;
    slot.key=key;slot.order=order;slot.queued_us=CaptureClock();slot.state.store(Ready,std::memory_order_release);
    const auto old=lane->exchange(key.slot,std::memory_order_acq_rel);
    if(old!=Empty){slots[old].state.store(Free,std::memory_order_release);replaced.fetch_add(1,std::memory_order_relaxed);}
    published.fetch_add(1,std::memory_order_relaxed);return true;
  }
};
CaptureExchange::CaptureExchange(CaptureExchangeConfig config,std::thread::id producer,std::thread::id consumer)
    :state_(std::make_shared<State>(config,producer,consumer)){}
bool CaptureExchange::Control(CaptureControlKind kind,CaptureFailure reason,CaptureSubject subject) noexcept {
  auto &s=*state_;if(!s.Producer())return false;
  if(kind==CaptureControlKind::Begin){
    if(s.failed.load(std::memory_order_acquire)){
      s.active.store(0,std::memory_order_release);s.ClearPending();
      if(!s.AllFree() || s.head.load(std::memory_order_acquire)!=s.tail.load(std::memory_order_acquire))return false;
    }
    auto gen=s.generation.load(std::memory_order_relaxed),mark=s.watermark.load(std::memory_order_relaxed),request=s.request.load(std::memory_order_relaxed);
    if(!s.Next(gen)||!s.Next(mark)||!s.Next(request))return false;
    s.active.store(0,std::memory_order_release);s.generation.store(gen,std::memory_order_release);s.watermark.store(mark,std::memory_order_release);
    s.request.store(request,std::memory_order_release);s.ClearPending();s.body_keys={};s.cold_enqueued=0;
    s.failure.store(0,std::memory_order_release);s.failed.store(0,std::memory_order_release);s.active.store(1,std::memory_order_release);
  }else{
    if(s.failed.load(std::memory_order_acquire))return false;
    if(kind==CaptureControlKind::RetireBody){
      bool found=false;
      // 单身体退休不撤销同窗ordinary事务；各lane代次也使consumer手中的旧body立即失效。
      for(std::uint32_t i=0;i<s.config.body_lanes;++i)if(s.body_keys[i]==subject){
        auto generation=s.body_generations[i].load(std::memory_order_relaxed);if(!s.Next(generation))return false;
        s.body_generations[i].store(generation,std::memory_order_release);s.Clear(s.bodies[i]);s.body_keys[i]={};found=true;}
      if(!found)return true;
    }else if(kind==CaptureControlKind::ColdRequest){auto request=s.request.load(std::memory_order_relaxed);if(!s.Next(request))return false;
      s.request.store(request,std::memory_order_release);s.Clear(s.cold);
    }else{
      auto mark=s.watermark.load(std::memory_order_relaxed);if(!s.Next(mark))return false;s.watermark.store(mark,std::memory_order_release);
      s.ClearPending();s.cold_enqueued=0;
      if(kind==CaptureControlKind::Stop){s.active.store(0,std::memory_order_release);s.body_keys={};}
    }
  }
  return s.Push(kind,reason,subject);
}
bool CaptureExchange::Begin() noexcept {return Control(CaptureControlKind::Begin,CaptureFailure::None);}
bool CaptureExchange::Stop(CaptureFailure reason) noexcept {return Control(CaptureControlKind::Stop,reason);}
bool CaptureExchange::Invalidate(CaptureFailure reason) noexcept {return Control(CaptureControlKind::Invalidate,reason);}
bool CaptureExchange::RequestCold() noexcept {return Control(CaptureControlKind::ColdRequest,CaptureFailure::None);}
bool CaptureExchange::RetireBody(CaptureSubject subject) noexcept {return Control(CaptureControlKind::RetireBody,CaptureFailure::Revoked,subject);}
CaptureExchange::WriteTicket CaptureExchange::Acquire(CaptureLane lane,CaptureWitness witness,CaptureSubject subject,CaptureFailure &failure) noexcept {
  failure=CaptureFailure::None;auto &s=*state_;
  if(!s.Producer()){failure=CaptureFailure::WrongThread;return {};}
  if(!s.active.load(std::memory_order_acquire) || s.failed.load(std::memory_order_acquire)){failure=CaptureFailure::Revoked;return {};}
  if(!ValidCaptureWitness(witness) || (lane!=CaptureLane::Ordinary && lane!=CaptureLane::Body) ||
      (lane==CaptureLane::Body && (witness.phase!=CapturePhase::PlayerPost || !witness.required_revision || !subject.space || !subject.local || !subject.use)) ||
      (lane==CaptureLane::Ordinary && witness.phase!=CapturePhase::Ordinary)){failure=CaptureFailure::InvalidWindow;return {};}
  auto body_slot=Empty;
  if(lane==CaptureLane::Body){for(std::uint32_t i=0;i<s.config.body_lanes;++i)if(s.body_keys[i]==subject){body_slot=i;break;}
    if(body_slot==Empty)for(std::uint32_t i=0;i<s.config.body_lanes;++i)if(!s.body_keys[i].local){body_slot=i;break;}
    if(body_slot==Empty){failure=CaptureFailure::QueueFull;return {};}}
  const auto first=s.config.ordinary_buffers && lane==CaptureLane::Body?s.config.ordinary_buffers:0;
  const auto end=s.config.ordinary_buffers && lane==CaptureLane::Ordinary?s.config.ordinary_buffers:s.config.buffers;
  for(std::uint32_t i=first;i<end;++i){std::uint32_t expected=Free;
    if(!s.slots[i].state.compare_exchange_strong(expected,Writing,std::memory_order_acq_rel))continue;
    if(!s.Next(s.serial)){s.slots[i].state.store(Free,std::memory_order_release);failure=CaptureFailure::SequenceExhausted;return {};}
    s.slots[i].serial.store(s.serial,std::memory_order_release);
    Key key{i,s.serial,s.generation.load(std::memory_order_acquire),s.watermark.load(std::memory_order_acquire),s.request.load(std::memory_order_acquire),lane,subject};
    if(lane==CaptureLane::Body){s.body_keys[body_slot]=subject;key.body_slot=body_slot;key.body_generation=s.body_generations[body_slot].load(std::memory_order_acquire);}
    return WriteTicket(state_,key,witness);
  }
  failure=CaptureFailure::QueueFull;return {};
}
bool CaptureExchange::PopControl(CaptureControl &out) noexcept {
  auto &s=*state_;if(!s.Consumer())return false;
  const auto read=s.head.load(std::memory_order_relaxed);if(read==s.tail.load(std::memory_order_acquire))return false;
  out=s.controls[read];s.head.store(read==256?0:read+1,std::memory_order_release);return true;
}
CaptureExchange::ReadTicket CaptureExchange::Take(std::uint64_t available_ordinary) noexcept {
  auto &s=*state_;if(!s.Consumer() || s.reading.load(std::memory_order_acquire) || s.failed.load(std::memory_order_acquire) ||
    s.head.load(std::memory_order_acquire)!=s.tail.load(std::memory_order_acquire))return {};
  // cold优先；其余轮转。未取得依赖时只延后该快槽，已可用的快槽仍能推进。
  for(std::uint32_t attempt=0;attempt<s.config.body_lanes+2;++attempt){
    const auto rotating=(s.next_lane+(attempt?attempt-1:0))%(s.config.body_lanes+1);
    std::atomic<std::uint32_t> *lane=attempt==0?&s.cold:rotating==0?&s.ordinary:&s.bodies[rotating-1];
    const auto index=lane->exchange(Empty,std::memory_order_acq_rel);if(index==Empty)continue;
    auto &slot=s.slots[index];auto expected=std::uint32_t(Ready);
    // producer 可能在前一轮普通槽检查后才发布。同一生产线程先交 ordinary，再交依赖它的 Body。
    if(attempt&&rotating&&slot.packet.witness.required_revision>available_ordinary&&
       (s.cold.load(std::memory_order_acquire)!=Empty||s.ordinary.load(std::memory_order_acquire)!=Empty)){
      auto empty=Empty;
      if(!lane->compare_exchange_strong(empty,index,std::memory_order_acq_rel)){
        slot.state.store(Free,std::memory_order_release);s.replaced.fetch_add(1,std::memory_order_relaxed);
      }
      continue;
    }
    if(!slot.state.compare_exchange_strong(expected,Reading,std::memory_order_acq_rel)){s.Flag(CaptureFailure::InvalidTransaction);return {};}
    const auto key=slot.key;
    if(!s.Current(key) || (key.cold_request && key.cold_request!=s.request.load(std::memory_order_acquire))){
      slot.state.store(Free,std::memory_order_release);s.revoked.fetch_add(1,std::memory_order_relaxed);continue;}
    if(attempt)s.next_lane=(rotating+1)%(s.config.body_lanes+1);
    s.reading.store(1,std::memory_order_release);return ReadTicket(state_,key);
  }
  return {};
}
CaptureExchangeStats CaptureExchange::Statistics() const noexcept {const auto &s=*state_;return {s.published.load(),s.replaced.load(),s.revoked.load(),s.failed.load()!=0,CaptureFailure(s.failure.load())};}
bool CaptureExchange::Drained() const noexcept {const auto &s=*state_;return s.AllFree() && s.head.load(std::memory_order_acquire)==s.tail.load(std::memory_order_acquire);}
std::uint64_t CaptureExchange::RetainedBytes() const noexcept {const auto &s=*state_;auto bytes=s.memory->Bytes();
  for(std::uint32_t i=0;i<s.config.buffers;++i)bytes+=s.slots[i].buffer->RetainedBytes();return bytes;}
CaptureExchange::WriteTicket::WriteTicket(std::shared_ptr<State> state,Key key,CaptureWitness witness) noexcept
    :state_(std::move(state)),key_(key){writer_.emplace(*state_->slots[key.slot].buffer,witness);}
CaptureExchange::WriteTicket::~WriteTicket(){Reset();}
CaptureExchange::WriteTicket::WriteTicket(WriteTicket &&other) noexcept
    :state_(std::move(other.state_)),key_(std::exchange(other.key_,{})),writer_(std::move(other.writer_)){other.writer_.reset();}
CaptureExchange::WriteTicket &CaptureExchange::WriteTicket::operator=(WriteTicket &&other) noexcept {
  if(this!=&other){Reset();state_=std::move(other.state_);key_=std::exchange(other.key_,{});if(other.writer_){writer_.emplace(std::move(*other.writer_));other.writer_.reset();}}return *this;
}
CaptureValueWriter *CaptureExchange::WriteTicket::Values() noexcept {
  if(!state_||!writer_)return nullptr;
  if(!state_->Producer() || !state_->Owned(key_,Writing) || !state_->Current(key_)){writer_->Fail(CaptureFailure::Revoked);return nullptr;}
  return &*writer_;
}
bool CaptureExchange::WriteTicket::Publish(const CaptureWindow &window,const CaptureRoot &anchor,std::uint32_t roots) noexcept {
  if(!state_||!writer_||!state_->Commit(key_,*writer_,window,anchor,roots))return false;
  writer_.reset();key_={};state_.reset();return true;
}
void CaptureExchange::WriteTicket::Reset() noexcept {
  if(state_ && state_->Owned(key_,Writing)){state_->Producer();state_->slots[key_.slot].packet={};state_->slots[key_.slot].state.store(Free,std::memory_order_release);}
  writer_.reset();key_={};state_.reset();
}
CaptureExchange::ReadTicket::ReadTicket(std::shared_ptr<State> state,Key key) noexcept:state_(std::move(state)),key_(key){}
CaptureExchange::ReadTicket::~ReadTicket(){Reset();}
CaptureExchange::ReadTicket::ReadTicket(ReadTicket &&other) noexcept:state_(std::move(other.state_)),key_(std::exchange(other.key_,{})){}
CaptureExchange::ReadTicket &CaptureExchange::ReadTicket::operator=(ReadTicket &&other) noexcept {
  if(this!=&other){Reset();state_=std::move(other.state_);key_=std::exchange(other.key_,{});}return *this;
}
bool CaptureExchange::ReadTicket::Current() const noexcept {
  return state_ && std::this_thread::get_id()==state_->consumer && state_->Owned(key_,Reading) && state_->Current(key_) &&
    (!key_.cold_request || key_.cold_request==state_->request.load(std::memory_order_acquire));
}
const CapturePacket *CaptureExchange::ReadTicket::Packet() const noexcept {return Current()?&state_->slots[key_.slot].packet:nullptr;}
std::uint64_t CaptureExchange::ReadTicket::QueuedMicros() const noexcept {
  return Current()?CaptureClock()-state_->slots[key_.slot].queued_us:0;
}
void CaptureExchange::ReadTicket::Reset() noexcept {
  if(state_ && state_->Owned(key_,Reading)){state_->Consumer();state_->slots[key_.slot].state.store(Free,std::memory_order_release);state_->reading.store(0,std::memory_order_release);}
  key_={};state_.reset();
}
}
