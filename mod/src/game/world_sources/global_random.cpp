#include "game/world_sources/global_random.h"
#include "game/world_bridge.h"
#include "runtime.h"
#include <atomic>
#include <sstream>
namespace cs::game {
namespace {
namespace obs=world::observation;
std::atomic<bool> armed{false};
std::atomic<std::uint64_t> foreign_faults{0};
std::uint64_t sequence=0,parent_call=0;
// J273静态核定：Random RVA2EB4B0；seed入口ECX为整数种子，不是可解引用对象。
constexpr std::uint32_t kWords=0x7e8720,kIndex=0x7ab814,kCrt=0x7e72ff;
GlobalRandomState Read() noexcept {
  GlobalRandomState s;
  const auto m=WorldMemoryReader();
  if(!IsMain()||!m.copy||m.image_size<kWords+sizeof(s.words)) return s;
  s.readable=m.copy(m.image_base+kWords,s.words.data(),sizeof(s.words))&&
    m.copy(m.image_base+kIndex,&s.index,sizeof(s.index))&&m.copy(m.image_base+kCrt,&s.crt,1);
  return s;
}
bool Valid(const GlobalRandomState &s) noexcept { return s.readable&&(s.crt?CaptureRuntimeRandomWords(0).readable:s.index<=625); }
// 独立ActualCall scope不借用上个Authority tick；不触及Lua或原版Online状态。
bool Publish(WorldCapture &c,const char *schema,std::string payload,bool complete,std::uint32_t nodes) {
  obs::ObservationEnvelope e;e.domain=obs::DomainId::Random;e.schema=schema;e.kind=obs::RecordKind::State;
  e.context=c.observation.BeginScope(obs::Phase::ActualCall,1);
  e.context.outer=c.collector->CurrentOuterToken();
  const auto run=c.collector->CurrentRunToken();
  if(run)e.context.run={obs::Presence::Known,{c.session_key,c.epoch,run,obs::IdentitySpace::Run}};
  e.result.coverage=complete?obs::Coverage::Complete:obs::Coverage::Partial;
  e.result.range="native-global-Random;MT19937-624;original-exactly-once";
  e.result.nodes=nodes;e.result.bytes=payload.size();e.payload=std::move(payload);
  if(!complete)e.result.reasons.push_back("native_global_rng_unreadable_crt_or_thread_fault");
  const auto id=e.context.scope;
  return c.observation.PrepareScope(std::move(e)).accepted&&c.observation.CommitScope(id,obs::Outcome::IndependentPublished).accepted;
}
void Words(std::ostringstream &o,const GlobalRandomState &s) {
  o<<'[';for(unsigned i=0;i<624;++i){if(i)o<<',';o<<s.words[i];}o<<']';
}
void State(std::ostringstream &o,const GlobalRandomState &s,std::uint64_t block) {
  o<<"{\"block\":\""<<block<<"\",\"index\":"<<s.index<<",\"mode\":\""<<(s.crt?"CRT-rand":"MT19937")
    <<"\",\"status\":\""<<(!s.readable?"ReadFailed":s.crt?"CRTStateInSeparateNativeSource":s.index==625?"Uninitialized":s.index<=624?"Value":"InvalidIndex")<<"\"}";
}
// 624字只在实际内容变化时发布；每次取值仍记录真实前后索引、结果和直接原生调用者RVA。
std::uint64_t Block(WorldCapture &c,const GlobalRandomState &s,bool checkpoint=false) {
  auto &m=c.global_random;
  if(!m.subject)m.subject=c.observation.AllocateRelatedSubject(obs::IdentitySpace::RandomStream).local;
  const bool changed=!m.block||m.state.readable!=s.readable||m.state.words!=s.words;
  if(changed||checkpoint) {
    if(changed)++m.block;
    std::ostringstream o;o<<"{\"stream\":\""<<m.subject<<"\",\"block\":\""<<m.block<<"\",\"words\":";Words(o,s);o<<'}';
    if(!Publish(c,"isaac.cs.random.native-global-block",o.str(),s.readable,624)){++m.failures;c.observation.Invalidate("native_global_block_delivery");}
  }
  m.state=s;return m.block;
}
}
void ArmGlobalRandom(bool value) noexcept { armed.store(value,std::memory_order_release); }
// 新Recorder从当前完整块开始；不能依赖上一次录制或消费者已见过的历史块。
void GlobalRandomCheckpoint() noexcept {
  if(!IsMain())return;auto c=CurrentWorldRecording();
  if(!c||!c->observation.Needed(obs::DomainId::Random,obs::Phase::ActualCall))return;
  try{Block(*c,Read(),true);}catch(...){++c->global_random.failures;c->observation.Invalidate("native_global_checkpoint_budget");}
}
GlobalRandomCall::GlobalRandomCall(bool seed,std::uint32_t value,void *return_slot) noexcept:seed_value_(value),seed_(seed) {
  if(!armed.load(std::memory_order_acquire))return;
  if(!IsMain()){++foreign_faults;return;}
  auto c=CurrentWorldRecording();if(!c||!c->observation.Needed(obs::DomainId::Random,obs::Phase::ActualCall))return;
  try {
    fault_=foreign_faults.load();before_=Read();block_=Block(*c,before_);
    if(before_.crt)crt_before_=EncodeRandomWords(CaptureRuntimeRandomWords(0),RandomAlgorithm::CrtRand);
    epoch_=c->epoch;session_=c->session_key;sequence_=++sequence;parent_=parent_call;parent_call=sequence_;active_=true;
    // ZHL 1.1.3无参非void adapter：保存EBP及5个寄存器后call wrapper。
    // wrapper返回槽+28才是原生调用者；OS栈回溯在无展开元数据的生成adapter处终止。
    // 仅接纳游戏映像内、且前一条E8的目标等于本入口的返回地址，不扫描栈猜调用者。
    const auto memory=WorldMemoryReader();std::uint32_t caller=0;unsigned char call[5]{};
    const auto slot=std::uint32_t(reinterpret_cast<uintptr_t>(return_slot));
    if(slot&&memory.copy&&memory.copy(slot+28,&caller,4)&&caller>=memory.image_base+5&&caller-memory.image_base<memory.image_size&&
       memory.copy(caller-5,call,5)&&call[0]==0xe8){
      std::int32_t delta=0;std::memcpy(&delta,call+1,4);
      if(caller+delta==memory.image_base+(seed_?0x2eb470u:0x2eb4b0u))callers_[caller_count_++]=caller-memory.image_base;
    }
  }catch(...){++c->global_random.failures;c->observation.Invalidate("native_global_enter_budget");}
}
GlobalRandomCall::~GlobalRandomCall() {
  if(!active_)return;
  parent_call=parent_;
  if(!returned_){auto c=CurrentWorldRecording();if(c&&c->epoch==epoch_&&c->session_key==session_){++c->global_random.failures;c->observation.Invalidate("native_global_original_exception");}}
}
void GlobalRandomCall::Returned(std::uint32_t result) noexcept {
  returned_=true;if(!active_)return;
  auto c=CurrentWorldRecording();if(!c||c->epoch!=epoch_||c->session_key!=session_)return;
  try {
    const auto after=Read();const auto block=Block(*c,after);auto &m=c->global_random;
    if(seed_)++m.seeds;else ++m.calls;
    const bool complete=Valid(before_)&&Valid(after)&&fault_==foreign_faults.load();
    std::ostringstream o;o<<"{\"member\":\""<<(seed_?"::SeedGlobalRandom":"::Random")<<"\",\"stream\":\""<<m.subject
      <<"\",\"call\":\""<<sequence_<<"\",\"parent_call\":\""<<parent_<<"\",\"before\":";State(o,before_,block_);
    o<<",\"after\":";State(o,after,block);o<<",\"result\":"<<result<<",\"seed_argument\":";
    if(seed_)o<<seed_value_;else o<<"null";
    o<<",\"game_callers_rva\":[";for(unsigned i=0;i<caller_count_;++i){if(i)o<<',';o<<callers_[i];}
    o<<"],\"caller_basis\":\""<<(caller_count_?"ZHL-noarg-return-slot;direct-call-target-checked":"ExternalOrAdapter;not-inferred")<<"\",\"foreign_faults\":\""<<foreign_faults.load()<<"\"";
    if(before_.crt||after.crt)o<<",\"crt_before\":"<<(crt_before_.empty()?"null":crt_before_)<<",\"crt_after\":"<<EncodeRandomWords(CaptureRuntimeRandomWords(0),RandomAlgorithm::CrtRand);
    o<<'}';
    if(!Publish(*c,"isaac.cs.random.native-global-call",o.str(),complete,32)){++m.failures;c->observation.Invalidate("native_global_call_delivery");}
  }catch(...){++c->global_random.failures;c->observation.Invalidate("native_global_return_budget");}
}
std::string ObservationGlobalRandomState(bool test_disabled) noexcept {
  if(!IsMain())return "{\"status\":\"WrongThread\"}";
  auto c=CurrentWorldRecording();
  if(!c||!c->observation.Needed(obs::DomainId::Random,obs::Phase::ActualCall)){
    if(!test_disabled)return "{\"status\":\"Disabled\"}";
    try{const auto s=Read();std::ostringstream o;o<<"{\"enabled\":false,\"state\":";State(o,s,0);o<<",\"words\":";Words(o,s);o<<'}';return o.str();}catch(...){return "{}";}
  }
  try {
    const auto s=Read();const auto block=Block(*c,s);const auto &m=c->global_random;
    std::ostringstream o;o<<"{\"enabled\":true,\"algorithm\":\"MT19937\",\"lifetime\":\"process;first-observed-not-birth\",\"stream\":\""<<m.subject<<"\",\"state\":";
    State(o,s,block);o<<",\"words\":";Words(o,s);
    if(s.crt)o<<",\"crt_state\":"<<EncodeRandomWords(CaptureRuntimeRandomWords(0),RandomAlgorithm::CrtRand);
    o<<",\"calls\":\""<<m.calls<<"\",\"seeds\":\""<<m.seeds<<"\",\"failures\":\""<<m.failures
      <<"\",\"foreign_faults\":\""<<foreign_faults.load()<<"\",\"complete\":"<<(Valid(s)&&!m.failures&&!foreign_faults.load()?"true":"false")<<'}';return o.str();
  }catch(...){c->observation.Invalidate("native_global_snapshot_budget");return "{\"status\":\"BudgetFailure\",\"complete\":false}";}
}
}
