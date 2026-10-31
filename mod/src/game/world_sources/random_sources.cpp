#include "game/world_sources/random_sources.h"
#include "game/world_bridge.h"
#include "runtime.h"
#include <atomic>
#include <sstream>
#include <unordered_map>
#include <algorithm>
namespace cs::game {
namespace obs=world::observation;
namespace {
std::atomic<bool> armed{false};
std::atomic<std::uint64_t> foreign_calls{0};
thread_local unsigned silent=0;
struct Silence final { Silence(){++silent;}~Silence(){--silent;} };
const char *Algorithm(RandomAlgorithm a) noexcept {
  return a==RandomAlgorithm::XorShift32?"xorshift32":a==RandomAlgorithm::MTRNG?"native-MT19937":
    a==RandomAlgorithm::MsvcMT19937?"MSVC-MT19937-double-buffer":a==RandomAlgorithm::Xoshiro256?"xoshiro256**":"CRT-rand";
}
std::string Quote(const std::string &s){
  std::string r="\"";for(const unsigned char c:s){if(c=='"'||c=='\\'){r+='\\';r+=char(c);}else if(c>=32)r+=char(c);else r+=' ';}return r+'"';
}
std::uintptr_t Key(void *p,RandomAlgorithm a) noexcept { return p?reinterpret_cast<std::uintptr_t>(p):std::uintptr_t(0x100u+unsigned(a)); }
bool Equal(const RandomWords &a,const RandomWords &b) noexcept {return a.readable==b.readable&&a.words==b.words;}
bool StaticData(void *p) noexcept{
  MEMORY_BASIC_INFORMATION m{};return p&&VirtualQuery(p,&m,sizeof(m))&&m.Type==MEM_IMAGE&&m.State==MEM_COMMIT&&
    (m.Protect&(PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY));
}
}
// 地址索引从不交给后台；未取得owner资格的栈/堆临时流仅在单次调用内存在。
struct RandomRegistry final {
  struct Entry final {
    void *address=nullptr;RandomAlgorithm algorithm=RandomAlgorithm::XorShift32;
    std::uint64_t subject=0,seen=0,calls=0,changes=0,revision=0;
    std::string owner="{}",purpose="unowned-native-call",lifetime="ActualCallOnly;owner-unresolved",identity;
    RandomWords state{};unsigned inflight=0;bool qualified=false,module_static=false;
  };
  std::unordered_map<std::uintptr_t,Entry> entries;
  std::string latest,summary;
  std::uint64_t serial=0,sequence=0,calls=0,changes=0,retired=0,failures=0;
};
std::shared_ptr<RandomRegistry> MakeRandomRegistry(){return std::make_shared<RandomRegistry>();}
bool RandomSourcesNeeded() noexcept {
  if(!armed.load(std::memory_order_acquire)||silent)return false;
  if(!IsMain()){++foreign_calls;return false;}
  const auto c=CurrentWorldRecording();return c&&c->observation.Needed(obs::DomainId::Random,obs::Phase::ActualCall);
}
void ArmRandomSources(bool a) noexcept {armed.store(a,std::memory_order_release);}
bool PublishRandomObservation(const char *schema,std::string payload,bool complete,std::uint32_t nodes) noexcept {
  auto c=CurrentWorldRecording();if(!IsMain()||!c||!c->observation.Needed(obs::DomainId::Random,obs::Phase::ActualCall))return false;
  try{
    Silence guard;obs::ObservationEnvelope e;e.domain=obs::DomainId::Random;e.schema=schema;e.kind=obs::RecordKind::Event;
    e.context=c->observation.BeginScope(obs::Phase::ActualCall,1);e.context.outer=c->collector->CurrentOuterToken();
    const auto run=c->collector->CurrentRunToken();if(run)e.context.run={obs::Presence::Known,{c->session_key,c->epoch,run,obs::IdentitySpace::Run}};
    e.result.coverage=complete?obs::Coverage::Complete:obs::Coverage::Partial;e.result.range="actual-native-random-source;state-change-is-not-an-invented-call";
    if(!complete)e.result.reasons.push_back("random_source_state_or_nonlocal_completion_unavailable");
    e.result.nodes=nodes;e.result.bytes=payload.size();e.payload=std::move(payload);const auto id=e.context.scope;
    return c->observation.PrepareScope(std::move(e)).accepted&&c->observation.CommitScope(id,obs::Outcome::IndependentPublished).accepted;
  }catch(...){c->observation.Invalidate("random_source_publication_budget");return false;}
}
RandomWords ReadRandomWords(void *p,RandomAlgorithm a) noexcept {
  if(!IsMain())return {};
  if(a==RandomAlgorithm::CrtRand)return CaptureRuntimeRandomWords(0);
  RandomWords s;try{
    const unsigned count=a==RandomAlgorithm::XorShift32?4:a==RandomAlgorithm::MTRNG?625:a==RandomAlgorithm::MsvcMT19937?1250:8;
    s.words.resize(count);const auto m=WorldMemoryReader();
    s.readable=p&&m.copy&&m.copy(std::uint32_t(reinterpret_cast<std::uintptr_t>(p)),s.words.data(),count*4);
    if(s.readable&&a==RandomAlgorithm::XorShift32)s.readable=s.words[1]<32&&s.words[2]<32&&s.words[3]<32;
    if(s.readable&&a==RandomAlgorithm::MTRNG)s.readable=s.words[624]<=625;
    if(s.readable&&a==RandomAlgorithm::MsvcMT19937)s.readable=s.words[0]<=1248&&s.words[1249]==0xffffffff;
    if(!s.readable)s.words.clear();
  }catch(...){s.readable=false;s.words.clear();}return s;
}
std::string EncodeRandomWords(const RandomWords &s,RandomAlgorithm a){
  std::ostringstream o;o<<"{\"algorithm\":"<<Quote(Algorithm(a))<<",\"status\":\""<<(s.readable?"Value":"NotCapturedOrReadFailed")<<"\",\"words\":[";
  for(std::size_t i=0;i<s.words.size();++i){if(i)o<<',';o<<s.words[i];}o<<"]}";return o.str();
}
namespace {
RandomRegistry::Entry &Entry(WorldCapture &c,void *p,RandomAlgorithm a){
  auto &r=*c.random_sources;const auto key=Key(p,a);auto found=r.entries.find(key);
  if(found!=r.entries.end()&&found->second.algorithm==a)return found->second;
  if(r.entries.size()>=32768)throw std::bad_alloc();
  auto &e=r.entries[key];e.address=p;e.algorithm=a;e.subject=c.observation.AllocateRelatedSubject(obs::IdentitySpace::RandomStream).local;
  e.module_static=StaticData(p);if(e.module_static){e.qualified=true;e.owner="{\"kind\":\"module-static;native-entry-witness\"}";e.lifetime="process";}
  return e;
}
std::string Metadata(const RandomRegistry::Entry &e){
  return "\"stream\":\""+std::to_string(e.subject)+"\",\"owner\":"+e.owner+",\"purpose\":"+Quote(e.purpose)+",\"lifetime\":"+Quote(e.lifetime)+",\"identity_origin\":\"FirstObserved;native-constructor-event-is-separate\"";
}
void Update(WorldCapture &c,RandomRegistry::Entry &e,RandomWords state,const char *basis,bool observed_call=false){
  const auto before=e.state;const bool changed=!Equal(before,state);
  if(changed){
    ++e.revision;
    if(before.readable&&!observed_call){
      ++e.changes;++c.random_sources->changes;
      const auto payload="{"+Metadata(e)+",\"basis\":"+Quote(basis)+",\"operation\":\"ObservedStateChange;inline-copy-restore-not-disambiguated\",\"before\":"+
        EncodeRandomWords(before,e.algorithm)+",\"after\":"+EncodeRandomWords(state,e.algorithm)+"}";
      if(!PublishRandomObservation("isaac.cs.random.native-state-change",payload,state.readable,unsigned(state.words.size()*2)))++c.random_sources->failures;
    }
  }
  e.state=std::move(state);
}
}
void BindRandomSource(const RandomBinding &b,bool observed_return) noexcept {
  if(!RandomSourcesNeeded()||(!b.address&&b.algorithm!=RandomAlgorithm::CrtRand))return;
  auto c=CurrentWorldRecording();if(!c||!c->random_sources)return;
  try{
    Silence guard;
    const auto existing=c->random_sources->entries.find(Key(b.address,b.algorithm));
    if(existing!=c->random_sources->entries.end()&&!b.identity.empty()&&!existing->second.identity.empty()&&b.identity!=existing->second.identity&&!existing->second.inflight)
      RetireRandomSource(b.address,"qualified-owner-generation-changed");
    auto &e=Entry(*c,b.address,b.algorithm);e.owner=b.owner.empty()?"{}":b.owner;e.purpose=b.purpose;e.lifetime=b.lifetime;
    if(!b.identity.empty())e.identity=b.identity;
    e.qualified=true;e.seen=c->random_sources->serial+1;Update(*c,e,ReadRandomWords(b.address,b.algorithm),"qualified-owner-current-read",observed_return);
  }catch(...){++c->random_sources->failures;c->observation.Invalidate("random_source_binding_budget");}
}
void RetireRandomSource(void *address,const char *reason) noexcept {
  if(!IsMain())return;auto c=CurrentWorldRecording();if(!c||!c->random_sources)return;
  auto &r=*c->random_sources;auto it=r.entries.find(reinterpret_cast<std::uintptr_t>(address));if(it==r.entries.end())return;
  try{PublishRandomObservation("isaac.cs.random.native-retire","{"+Metadata(it->second)+",\"reason\":"+Quote(reason?reason:"owner-retired")+"}",true,1);}catch(...){++r.failures;}
  r.entries.erase(it);++r.retired;
}
std::string RandomSourceReference(void *address) noexcept {
  if(!IsMain())return "{\"resolution\":\"WrongThread\"}";auto c=CurrentWorldRecording();if(!c||!c->random_sources)return "{\"resolution\":\"NotCaptured\"}";
  const auto it=c->random_sources->entries.find(reinterpret_cast<std::uintptr_t>(address));
  if(it==c->random_sources->entries.end())return "{\"resolution\":\"Unresolved\",\"reason\":\"owner-not-qualified\"}";
  return "{\"resolution\":\"KnownCurrent\",\"space\":11,\"local_id\":\""+std::to_string(it->second.subject)+"\"}";
}
struct NativeRandomCall::Ticket final {
  std::shared_ptr<WorldCapture> capture;RandomWords before;std::uintptr_t key=0;
  std::uint64_t call=0,stream=0;RandomAlgorithm algorithm;std::string member;
  std::uint32_t argument=0,argument2=0;bool initialize=false,done=false;
};
NativeRandomCall::NativeRandomCall(void *p,RandomAlgorithm a,const char *member,std::uint32_t arg,bool initialize,std::uint32_t arg2) noexcept {
  if(!RandomSourcesNeeded())return;auto c=CurrentWorldRecording();if(!c||!c->random_sources)return;
  try{
    Silence guard;
    if(std::strcmp(member,"RNG.constructor")==0)RetireRandomSource(p,"observed-native-constructor;storage-reuse-does-not-inherit-identity");
    ticket_=std::make_unique<Ticket>();auto &t=*ticket_;t.capture=c;t.key=Key(p,a);t.algorithm=a;t.member=member;t.argument=arg;t.argument2=arg2;t.initialize=initialize;
    auto &r=*c->random_sources;auto &e=Entry(*c,p,a);t.stream=e.subject;t.call=++r.sequence;
    ++e.inflight;
    if(!initialize){t.before=ReadRandomWords(p,a);Update(*c,e,t.before,"between-native-entry-boundaries");}
    else if(e.state.readable)t.before=e.state;
  }catch(...){++c->random_sources->failures;ticket_.reset();c->observation.Invalidate("random_native_enter_budget");}
}
NativeRandomCall::~NativeRandomCall(){
  if(!ticket_||ticket_->done)return;auto &t=*ticket_;
  try{
    PublishRandomObservation("isaac.cs.random.native-call","{\"call\":\""+std::to_string(t.call)+"\",\"member\":"+Quote(t.member)+",\"completion\":\"CppException;original-not-repeated\"}",false,0);
    auto &r=*t.capture->random_sources;auto it=r.entries.find(t.key);if(it!=r.entries.end()&&it->second.inflight){--it->second.inflight;if(!it->second.qualified&&!it->second.inflight)r.entries.erase(it);}
  }catch(...){}
}
void NativeRandomCall::Returned(std::uint32_t value,bool floating) noexcept {
  if(!ticket_)return;auto &t=*ticket_;t.done=true;auto c=CurrentWorldRecording();
  if(c!=t.capture||!c||!c->random_sources)return;
  try{
    Silence guard;auto &r=*c->random_sources;auto it=r.entries.find(t.key);if(it==r.entries.end()||it->second.subject!=t.stream)return;
    auto &e=it->second;auto after=ReadRandomWords(e.address,t.algorithm);++e.calls;++r.calls;
    std::ostringstream o;o<<'{'<<Metadata(e)<<",\"call\":\""<<t.call<<"\",\"member\":"<<Quote(t.member)<<",\"argument\":"<<t.argument<<",\"argument2\":"<<t.argument2
      <<",\"initialize\":"<<(t.initialize?"true":"false")<<",\"before\":"<<EncodeRandomWords(t.before,t.algorithm)<<",\"after\":"<<EncodeRandomWords(after,t.algorithm)
      <<",\"result_bits\":"<<value<<",\"result_kind\":\""<<(floating?"f32":"u32")<<"\",\"completion\":\"Returned\"}";
    if(!PublishRandomObservation("isaac.cs.random.native-call",o.str(),after.readable&&(t.initialize||t.before.readable),unsigned(after.words.size()+t.before.words.size())))++r.failures;
    Update(*c,e,std::move(after),"actual-native-return",true);
    // 无owner的栈/堆参数不跨调用缓存解引用，地址复用不继承旧身份。
    if(e.inflight)--e.inflight;
    if(!e.qualified&&!e.inflight)r.entries.erase(it);
  }catch(...){++c->random_sources->failures;c->observation.Invalidate("random_native_return_budget");}
}
std::string CaptureRandomSources() noexcept {
  auto c=CurrentWorldRecording();if(!IsMain()||!c||!c->random_sources)return "{\"status\":\"Disabled\"}";
  // 显式读取在窗口外只交付上次已复制的值，不用跨帧裸RNG指针临时拼造当前快照。
  if(!c->location_window)return c->random_sources->latest.empty()?"{\"complete\":false,\"status\":\"NotScheduled\"}":c->random_sources->latest;
  try{
    NativeRandomLuaAbortPending();auto &r=*c->random_sources;
    const bool diagnostic=r.serial==0&&Settings.diagnostics;
    if(diagnostic)Trace("random_capture_stage","stage=begin entries=%u",unsigned(r.entries.size()));
    const auto owner_errors=EnumerateNativeRandomOwners(nullptr,[](void *,const RandomBinding &b)noexcept{BindRandomSource(b);});CaptureRuntimeRandomOwners();++r.serial;
    if(diagnostic)Trace("random_capture_stage","stage=enumerated entries=%u",unsigned(r.entries.size()));
    bool states_complete=true;for(auto &[key,e]:r.entries){
      (void)key;if(e.module_static)Update(*c,e,ReadRandomWords(e.address,e.algorithm),"module-static-current-read");
      if((e.module_static||e.seen==r.serial)&&!e.state.readable)states_complete=false;
    }
    const auto complete=!r.failures&&!foreign_calls.load()&&!owner_errors&&states_complete&&RuntimeRandomSourcesComplete();
    if(diagnostic)Trace("random_capture_stage","stage=states-complete complete=%u",unsigned(complete));
    std::ostringstream o;o<<"{\"complete\":"<<(complete?"true":"false")<<",\"owner_errors\":"<<owner_errors<<",\"runtime_sources_complete\":"<<(RuntimeRandomSourcesComplete()?"true":"false")<<",\"calls\":\""<<r.calls<<"\",\"state_changes\":\""<<r.changes
      <<"\",\"retired\":\""<<r.retired<<"\",\"failures\":\""<<r.failures<<"\",\"foreign_calls\":\""<<foreign_calls.load()<<"\",\"streams\":[";
    bool first=true;std::vector<std::uintptr_t> retired;unsigned objects=0,nodes=0;
    for(auto &[key,e]:r.entries){
      if(!e.module_static&&e.seen!=r.serial){if(e.qualified)retired.push_back(key);continue;}
      ++objects;nodes+=unsigned(e.state.words.size())+12;
      if(!first)o<<',';first=false;o<<'{'<<Metadata(e)<<",\"revision\":\""<<e.revision<<"\",\"calls\":\""<<e.calls<<"\",\"state\":"<<EncodeRandomWords(e.state,e.algorithm)
        <<",\"freshness\":\"CurrentQualifiedRead\",\"consumption_coverage\":\"native-entry-calls-and-observed-state-changes\"}";
    }o<<"]}";
    if(diagnostic)Trace("random_capture_stage","stage=encoded");
    for(const auto key:retired)RetireRandomSource(reinterpret_cast<void *>(key),"absent-from-current-qualified-owner-membership");
    r.latest=o.str();
    r.latest.insert(1,"\"source\":1,\"source_scope\":\""+std::to_string(c->lua_scope)+"\",");
    r.summary="{\"complete\":"+std::string(complete?"true":"false")+",\"objects\":"+std::to_string(objects)+",\"nodes\":"+std::to_string(nodes)+",\"bytes\":"+std::to_string(r.latest.size())+",\"owner_errors\":"+std::to_string(owner_errors)+",\"failures\":\""+std::to_string(r.failures)+"\",\"source_scope\":\""+std::to_string(c->lua_scope)+"\"}";
    if(diagnostic)Trace("random_capture_stage","stage=ready bytes=%u objects=%u nodes=%u",unsigned(r.latest.size()),objects,nodes);
    return r.latest;
  }catch(...){++c->random_sources->failures;c->observation.Invalidate("random_snapshot_budget");c->random_sources->summary="{\"complete\":false,\"status\":\"BudgetFailure\"}";return c->random_sources->summary;}
}
std::string RandomSourcesSummary() noexcept {
  const auto c=CurrentWorldRecording();
  if(!IsMain()||!c||!c->random_sources||c->random_sources->summary.empty())return "{\"complete\":false,\"status\":\"NotScheduled\"}";
  return c->random_sources->summary;
}
void RandomSourcesCheckpoint() noexcept {
  auto c=CurrentWorldRecording();if(!IsMain()||!c||!c->random_sources)return;
  try{for(const auto &[key,e]:c->random_sources->entries){(void)key;if(e.qualified&&e.state.readable)PublishRandomObservation("isaac.cs.random.native-checkpoint","{"+Metadata(e)+",\"state\":"+EncodeRandomWords(e.state,e.algorithm)+"}",true,unsigned(e.state.words.size()));}}catch(...){++c->random_sources->failures;}
}
}
