#include "game/observation/binary_entities.h"
#include "game/observation/world_bridge.h"
#include "game/observation/binary_probability.h"
#include "game/authority/state.h"
#include "game/authority/tick.h"
#include "game/common/bridge.h"
#include "world/observe.h"
#include "state/budget.h"
#include "state/production_cache.h"
#include <algorithm>
#include <tuple>
#include <cmath>
#include <limits>
#include <windows.h>

namespace cs::game {
namespace {
using namespace state;
using world::binary::EntityFacts;
using world::binary::NativeScene;
struct TextWitness {std::uint32_t address=0;std::uint64_t local=0,use=0,shape=0;};
// handle在所有引用空间中唯一，避免Entity local=1与控制组local=1相互混淆。
struct GroupFacts {
  std::uint64_t session=0,participant=0,group=0,participant_link=0,group_link=0,binding=0;
  std::array<CaptureLink,64> anchors{};
  std::uint32_t count=0;
  bool present=false;
};
CaptureExchangeConfig Buffers() {
  CaptureExchangeConfig cfg;cfg.buffers=20;cfg.body_lanes=8;
  cfg.capacity={1,4096,16384,512u<<10,16};cfg.ordinary_buffers=4;cfg.ordinary_capacity={2048,32768,131072,2u<<20,16392};return cfg;
}
// 只绑定已安装官方1.1.3模块的真实helper／ctor与静态表关系，不执行目标helper或校验哈希。
world::binary::EntitySourceCapacity SourceCapacity(world::MemoryReader memory) noexcept {
  world::binary::EntitySourceCapacity out;const auto module=GetModuleHandleW(L"zhlREPENTOGON.dll");
  const auto base=reinterpret_cast<std::uintptr_t>(module);if(!base || base>UINT32_MAX || !memory.copy)return out;
  const auto read=[&](std::uint32_t offset,void *p,std::size_t bytes){return base+offset<=UINT32_MAX && memory.copy(std::uint32_t(base)+offset,p,bytes);};
  std::array<std::byte,32> helper{};std::array<std::byte,6> ctor{};std::uint32_t table=0,vtable=0;
  if(!read(0x261c90,helper.data(),helper.size()) || !read(0x261c00,ctor.data(),ctor.size()))return out;
  const std::array<std::byte,13> index={std::byte{0x8b},std::byte{0x40},std::byte{0x24},std::byte{0x3d},std::byte{0xbf},std::byte{1},std::byte{},std::byte{},std::byte{0x77},std::byte{0x0c},std::byte{0x8d},std::byte{4},std::byte{0x40}};
  if(!std::equal(index.begin(),index.end(),helper.begin()+10) || helper[23]!=std::byte{0x8d} || helper[24]!=std::byte{4} || helper[25]!=std::byte{0x85} ||
     ctor[0]!=std::byte{0xc7} || ctor[1]!=std::byte{1})return out;
  std::memcpy(&table,helper.data()+26,4);std::memcpy(&vtable,ctor.data()+2,4);
  if(table!=base+0x62e650 || vtable!=base+0x5112c4)return out;
  out.grid_plus=table;out.grid_plus_vtable=vtable;return out;
}
}

// 主线程状态与后台交接分开；后台只接触exchange的无锁纯值／控制，不读取这些原生关联。
struct BinaryEntities::Impl {
  WorldCapture &capture;
  std::thread::id producer,consumer;
  memory::Lease memory;
  world::binary::EntitySource source;
  CaptureExchange exchange;
  ProductionCache production;
  CaptureBuffer production_scratch;
  // 每个身体保留独立HUD实例及冻结P8修订；两个Stat/History原生槽不构成系统人数上限。
  struct HudState {
    CaptureSubject subject;
    StateScope scope;
    std::uint32_t address=0;
    std::uint64_t instance=0,mantle=0;
    HudLocalClock clock;
    std::optional<HudClockCapture> tracker;
    bool complete=false,stats=false,history=false;
    std::uint32_t body=0;
    std::array<world::binary::NativeStatRow,10> rows{};
    bool rows_initialized=false;
  };
  std::array<HudState,256> hud_states;
  struct Produced {
    world::NativeCaptureWindow window;
    CaptureWitness witness;
    BinaryProductionKind kind=BinaryProductionKind::PlayerHud;
    std::uint32_t token=0,object=0,manager=0,body=0,hud_index=UINT32_MAX,drop_reads=0,parent=UINT32_MAX;
    bool held=false,seed_seen=false,seed_blocked=false,manager_seen=false,manager_blocked=false;
    std::int32_t sound=-1;
    float volume=0,pitch=1,pan=0;
    std::uint32_t stream=0,selected=0;
    bool loop=false,audio_world=false;
    std::int32_t ambient=-1;
    float retro=0;
    bool room_effect_known=false,glitch=false;
    std::int32_t stat_index=-1;
    std::uint32_t stat_flags=0;
    world::binary::NativeDrawClock draw_clock;
    bool draw_known=false,draw_conflict=false;
  };
  std::array<Produced,16> producing{};
  std::uint32_t production_depth=0,next_production_token=0;
  std::uint64_t next_presentation_instance=0;
  struct ScoreCache {StateScope scope;std::array<std::uint32_t,16> values{};std::uint64_t revision=0;};
  ScoreCache score;
  StateScope progress_scope;
  std::uint64_t progress_revision=0;
  bool progress_blocked=false,planetarium_known=false,planetarium_qualified=false;
  // 只保留实际Recompute已经写出的公共概率值，位图区别真实生产与构造零值。
  struct ProbabilityCache {StateScope scope;std::array<float,3> values{};std::array<std::uint32_t,3> frames{};std::uint8_t mask=0;};
  ProbabilityCache probability;
  struct AudioVoice {
    world::binary::NativeAudioValue value;
    CaptureWitness production;
    std::uint64_t instance=0,generation=0;
    std::uint32_t stream=0,sound=0;
    std::int32_t ambient=-1;
    bool active=false;
  };
  struct AudioEvent {world::binary::NativeAudioValue value;CaptureWitness production;std::uint64_t instance=0,generation=0,sequence=0;};
  std::array<AudioVoice,512> voices{};
  std::array<AudioEvent,1024> audio_events{};
  std::uint32_t audio_event_count=0;
  std::uint64_t next_audio_voice=0,next_audio_event=0;
  bool audio_gap=false;
  std::vector<EntityFacts> roots,ordinary_facts;
  std::vector<std::uint32_t> root_index,link_index,published_addresses;
  std::vector<CaptureReferenceEntry> links;
  std::vector<TextWitness> texts;
  std::vector<std::uint32_t> members,member_check;
  std::array<world::RawSource,10> member_sources{};
  std::array<GroupFacts,4> groups{};
  NativeScene scene;
  std::array<EntityFacts,985> scene_roots{};
  std::array<std::array<std::uint32_t,15>,527> descriptor_shapes{};
  std::array<std::uint64_t,527> descriptor_revisions{};
  std::uint32_t scene_root_count=0,floor_depth=0;
  std::uint64_t floor=0,next_floor=0,floor_run=0;
  bool floor_ready=true;
  struct Pending {
    CaptureExchange::WriteTicket ticket;
    CaptureWitness witness;
    world::NativeCaptureWindow window;
    CaptureRoot anchor;
    EntityFacts facts;
    std::uint64_t resource_change=0;
    std::uint32_t roots=0;
  } ordinary;
  std::array<Pending,8> bodies;
  std::array<CaptureSubject,8> fast_keys{};
  std::array<std::uint32_t,8> fast_addresses{};
  world::NativeCaptureWindow reading_window;
  CaptureWitness reading_witness;
  std::uint32_t root_used=0,ordinary_count=0,published_count=0,link_used=0,text_used=0;
  std::uint64_t revision=0,last_ordinary=0,next_related=0,resource_mutation=0,reading_resource_mutation=0,run=0,room=0;
  BinaryEntityStats stats;
  bool collecting=false,invalidated=false;
  Impl(WorldCapture &c,std::thread::id p,std::thread::id worker):capture(c),producer(p),consumer(worker),
    memory(memory::Reserve(memory::Pool::Values,sizeof(*this)+16384u*(2*sizeof(EntityFacts)+2*sizeof(std::uint32_t)+sizeof(TextWitness))+16392u*(sizeof(CaptureReferenceEntry)+sizeof(std::uint32_t))+2u*world::kMembershipBudget*4,"binary_entity_bridge")),
    source(c.source_memory,p,{this,
      [](void *context,const CaptureWitness &w,const EntityFacts &facts) noexcept {return static_cast<Impl *>(context)->Current(w,facts);},
      [](void *context,std::uint32_t address) noexcept {return static_cast<Impl *>(context)->Reference(address);},
      [](void *context,std::uint32_t address) noexcept {auto &owner=*static_cast<Impl *>(context);const auto index=owner.RootIndex(address);
        if(owner.reading_window.production){const auto *id=owner.capture.identities.Find(world::ObjectDomain::Entity,address);
          return id&&!id->Retired()&&!id->candidate&&id->readiness>=2&&owner.capture.collector->NativeCurrent(owner.reading_window,address,id->local_id,id->use,id->revision);}
        return index!=UINT32_MAX && owner.roots[index].player_ready && owner.Current(owner.reading_witness,owner.roots[index]);},
      [](void *context,std::uint32_t address) noexcept {auto &owner=*static_cast<Impl *>(context);const auto index=owner.RootIndex(address);
      if(owner.reading_window.production){const auto *id=owner.capture.identities.Find(world::ObjectDomain::Entity,address);
        return id&&!id->Retired()&&!id->candidate&&id->readiness>=2&&owner.capture.collector->NativeCurrent(owner.reading_window,address,id->local_id,id->use,id->revision);}
      return index!=UINT32_MAX && owner.roots[index].ready && owner.Current(owner.reading_witness,owner.roots[index]);}},SourceCapacity(c.source_memory)),exchange(Buffers(),p,worker),
    production(p),production_scratch({8,16384,32768,256u<<10,16392}) {
    roots.resize(16384);ordinary_facts.resize(16384);root_index.resize(16384);links.resize(16392);link_index.resize(16392);texts.resize(16384);
    published_addresses.resize(16384);
    members.resize(world::kMembershipBudget);member_check.resize(world::kMembershipBudget);
    for(auto &hud:hud_states)hud.tracker.emplace(p);
    // 全部自有暂存与交接合计占用同一64MiB shared额度，不给每个owner重复领取。
    if(memory->Bytes()+source.RetainedBytes()+exchange.RetainedBytes()+production.RetainedBytes()+production_scratch.RetainedBytes()>StateBudgets().shared_value_bytes)
      throw std::invalid_argument("binary_entity_shared_capacity");
  }
  bool Main() const noexcept {return std::this_thread::get_id()==producer;}
  // 结构读取与字段执行器共用当前窗口，地址加法保持u64直到检查完成，错误不形成环回读取。
  template<class T> bool NativeValue(std::uint64_t address,T &value) noexcept {
    return Main() && collecting && capture.collector && capture.collector->NativeWindowCurrent(reading_window) && address && address<=UINT32_MAX &&
      sizeof(T)<=(std::uint64_t{1}<<32)-address && capture.source_memory.copy(std::uint32_t(address),&value,sizeof(T));
  }
  bool Current(const CaptureWitness &w,const EntityFacts &f) noexcept {
    if(!Main() || !stats.active || !collecting || reading_witness!=w || reading_resource_mutation!=resource_mutation || !capture.collector ||
       !capture.collector->NativeWindowCurrent(reading_window))return false;
    if(f.subject.space==1 || f.subject.space==2){if(!capture.collector->NativeCurrent(reading_window,f.identity_address?f.identity_address:f.address,f.subject.local,f.subject.use,f.subject.shape,
        f.subject.space==2?world::ObjectDomain::Grid:world::ObjectDomain::Entity))return false;}
    else if(reading_window.production){if(w.scope.run!=reading_window.run || w.scope.room!=reading_window.room || !f.ready)return false;}
    else if(!reading_window.ordinary || !floor_ready || floor_depth || w.scope.floor!=floor || f.scene!=&scene ||
      (f.layout==239?f.address!=scene.room:(f.layout==276||f.layout==237||f.layout==382||f.layout==383)?f.address!=scene.game:
        f.layout!=270 || f.source_slot>=scene.descriptors.size() || scene.descriptors[f.source_slot]!=f.address ||
        f.subject.use!=floor || f.subject.shape!=descriptor_revisions[f.source_slot]))return false;
    if(f.ordinal<4){const auto *runtime=Runtime();if(!runtime || runtime->finished || runtime->retiring || runtime->failure ||
        !runtime->session.HasParticipant(f.ordinal) || SessionGeneration(*runtime)!=f.control_session ||
        runtime->players[f.ordinal].Generation()!=f.binding_revision)return false;}
    return true;
  }
  CaptureLink Register(CaptureSubject subject,bool pending=false) noexcept {
    if(!subject.space || !subject.local || !subject.use)return {CapturedReferenceKind::Invalid,0};
    const auto key=[](CaptureSubject s){return std::tuple(s.space,s.local,s.use);};
    const auto end=link_index.begin()+link_used;
    auto position=std::lower_bound(link_index.begin(),end,subject,[&](std::uint32_t i,CaptureSubject s){const auto &r=links[i];return std::tuple(r.space,r.local,r.use)<key(s);});
    if(position!=end && std::tuple(links[*position].space,links[*position].local,links[*position].use)==key(subject)){
      return {pending?CapturedReferenceKind::Pending:CapturedReferenceKind::Identity,std::uint64_t(*position)+1};}
    if(link_used==links.size())return {CapturedReferenceKind::Invalid,0};
    links[link_used]={subject.space,subject.local,subject.use};std::move_backward(position,end,end+1);*position=link_used++;
    return {pending?CapturedReferenceKind::Pending:CapturedReferenceKind::Identity,link_used};
  }
  std::uint32_t RootIndex(std::uint32_t address) const noexcept {
    const auto end=root_index.begin()+root_used;auto at=std::lower_bound(root_index.begin(),end,address,[&](std::uint32_t i,std::uint32_t p){return roots[i].address<p;});
    return at!=end && roots[*at].address==address?*at:UINT32_MAX;
  }
  // 新候选失败不能丢掉上一已发布闭包的失效覆盖；这里只保留主线程地址索引。
  bool Known(std::uint32_t address) const noexcept {return RootIndex(address)!=UINT32_MAX ||
    std::binary_search(published_addresses.begin(),published_addresses.begin()+published_count,address);}
  bool TextReady(std::uint32_t address,const world::Identity &id) const noexcept {
    const auto end=texts.begin()+text_used;const auto at=std::lower_bound(texts.begin(),end,address,[](const auto &a,std::uint32_t p){return a.address<p;});
    return at!=end && at->address==address && at->local==id.local_id && at->use==id.use && at->shape==id.revision;
  }
  // C/S实际拓扑只读投影；未知与明确未分配分开，组名／设备地址均不入状态。
  void Control(EntityFacts &f) noexcept {
    auto *runtime=Runtime();f.group_known=true;
    if(!runtime || runtime->finished || runtime->retiring || runtime->failure)return;
    unsigned found=4,body_index=0;bool unknown=false;
    for(unsigned ordinal=0;ordinal<runtime->session.ParticipantLimit();++ordinal){if(!runtime->session.HasParticipant(ordinal))continue;
      const auto &topology=runtime->topology[ordinal];
      if(!topology.known || !topology.unique || topology.fault!=TopologyFault::None || topology.room!=runtime->topology[0].room){unknown=true;continue;}
      for(unsigned i=0;i<topology.body_count;++i)if(world::PackWord(topology.bodies[i].player)==f.address){
        if(found!=4){f.group_known=false;return;}found=ordinal;body_index=i;}
    }
    if(found==4){f.group_known=!unknown;return;}
    const auto &body=runtime->topology[found].bodies[body_index];const auto &group=groups[found];
    if(!group.present){f.group_known=false;return;}
    f.ordinal=std::uint8_t(found);f.role=std::uint8_t(body.role);f.control_flags=(body.active?1u:0u)|
      (runtime->players[found].BoundValid()&&body.CanConsume()?2u:0u);
    f.group=group.group_link;f.participant=group.participant_link;f.control_session=group.session;f.binding_revision=group.binding;
    f.subject.binding=group.binding;
  }
  bool Groups() noexcept {
    auto *runtime=Runtime();for(auto &group:groups){group.count=0;group.present=false;}
    if(!runtime || runtime->finished || runtime->retiring || runtime->failure)return true;
    for(unsigned ordinal=0;ordinal<runtime->session.ParticipantLimit();++ordinal){if(!runtime->session.HasParticipant(ordinal))continue;
      const auto &topology=runtime->topology[ordinal];auto &group=groups[ordinal];
      if(!topology.known || !topology.unique || topology.fault!=TopologyFault::None || topology.room!=runtime->topology[0].room)return false;
      const auto session=SessionGeneration(*runtime);if(!session)return false;
      if(group.session!=session){if(next_related>UINT64_MAX-2)return false;group.session=session;group.participant=++next_related;group.group=++next_related;}
      group.binding=runtime->players[ordinal].Generation();if(!group.binding)return false;
      const auto participant=Register({9,group.participant,1}),control=Register({10,group.group,1});
      if(participant.kind==CapturedReferenceKind::Invalid || control.kind==CapturedReferenceKind::Invalid)return false;
      group.participant_link=participant.handle;group.group_link=control.handle;group.present=true;
    }
    return true;
  }
  EntityFacts *Add(std::uint32_t address,std::uint32_t membership=0,std::uint16_t roster=kNullIndex) noexcept {
    if(!address)return nullptr;const auto prior=RootIndex(address);
    if(prior!=UINT32_MAX){auto &f=roots[prior];f.memberships|=membership;if(roster!=kNullIndex)f.roster=roster;return &f;}
    if(root_used==roots.size())return nullptr;const auto *id=capture.identities.Find(world::ObjectDomain::Entity,address);
    if(!id || id->Retired())return nullptr;
    EntityFacts f;f.address=address;f.subject={1,id->local_id,id->use,id->revision};f.memberships=membership;f.roster=roster;
    f.ready=id->readiness>=2;f.player_ready=f.ready;f.text_ready=TextReady(address,*id);
    // 激活资格独立于输入绑定／valid：只认已观察完整初始化／当前Player证言。
    f.activation_qualified=f.ready;Control(f);f.subject.topology=source.Topology(f.subject);
    auto end=root_index.begin()+root_used;auto where=std::lower_bound(root_index.begin(),end,address,[&](std::uint32_t i,std::uint32_t p){return roots[i].address<p;});
    roots[root_used]=f;std::move_backward(where,end,end+1);*where=root_used++;
    return &roots[root_used-1];
  }
  CaptureLink Reference(std::uint32_t address) noexcept {
    if(!address)return {CapturedReferenceKind::Null,0};const bool pending=RootIndex(address)==UINT32_MAX;
    if(reading_window.production){const auto *id=capture.identities.Find(world::ObjectDomain::Entity,address);
      return id&&!id->Retired()&&!id->candidate&&id->readiness>=2?Register({1,id->local_id,id->use,id->revision}):CaptureLink{CapturedReferenceKind::Invalid,0};}
    if(reading_window.ordinary){auto *f=Add(address);return f?Register(f->subject,pending):CaptureLink{CapturedReferenceKind::Invalid,0};}
    const auto *id=capture.identities.Find(world::ObjectDomain::Entity,address);
    return id&&!id->Retired()?Register({1,id->local_id,id->use,id->revision},true):CaptureLink{CapturedReferenceKind::Invalid,0};
  }
  CaptureWitness Witness(const world::NativeCaptureWindow &w,CapturePhase phase) noexcept {
    if(revision==UINT64_MAX)return {};CaptureWitness out;
    out.scope={capture.session_key,w.run,floor,floor?floor*3+scene.dimension+1:0,w.room,w.room};out.generation=capture.epoch;out.revision=++revision;out.batch=w.attempt;
    out.tick=w.tick;out.outer=w.outer;out.owner=w.owner;out.mutation=w.mutation;out.game_frame=w.game_frame;out.phase=phase;
    if(phase==CapturePhase::Production)out.batch=w.owner;
    if(phase==CapturePhase::PlayerPost)out.required_revision=last_ordinary;return out;
  }
  void ReadWindow(const Pending &p) noexcept {reading_window=p.window;reading_witness=p.witness;reading_resource_mutation=p.resource_change;collecting=true;}
  void ReadProduction(const Produced &p) noexcept {reading_window=p.window;reading_witness=p.witness;reading_resource_mutation=resource_mutation;collecting=true;}
  EntityFacts ProductionBody(std::uint32_t,const Produced &) noexcept;
  bool SaveProduction(Produced &,EntityFacts,std::uint16_t,std::uint64_t) noexcept;
  bool ProductionInto(CaptureValueWriter &,std::uint32_t &,CaptureDiagnostic &) noexcept;
  void ClearProduction() noexcept;
  void ProduceOwnedStats(Produced &) noexcept;
  bool AudioInto(CaptureValueWriter &,std::uint32_t &,CaptureDiagnostic &) noexcept;
  bool AudioEventFor(AudioVoice &,const Produced &,std::uint8_t kind) noexcept;
  void AudioControl(Produced &) noexcept;
  void Failure(CaptureDiagnostic d) noexcept {++stats.gaps;stats.failure=d;}
  bool GroupsInto(CaptureValueWriter &writer,std::uint32_t &count) noexcept {
    const auto scalar=[&](std::uint16_t field,ScalarKind kind,std::uint64_t number,unsigned bytes){std::array<std::byte,8> data{};
      for(unsigned i=0;i<bytes;++i)data[i]=std::byte(number>>(8*i));return writer.PutScalar({{},196,field,field},kind,1,std::span(data).first(bytes));};
    for(unsigned ordinal=0;ordinal<4;++ordinal){auto &g=groups[ordinal];if(!g.present)continue;
      if(!writer.BeginRoot({9,g.participant,1},195,BodyStream::Value,0) || !writer.FinishRoot())return false;++count;
      const CaptureLink participant{CapturedReferenceKind::Identity,g.participant_link};
      if(!writer.BeginRoot({10,g.group,1},196,BodyStream::Value,1) ||
         !writer.PutLinks({{},196,0,0},false,255,std::span(&participant,1)) ||
         !scalar(1,ScalarKind::U8,ordinal,1) || !scalar(2,ScalarKind::U64,g.session,8) || !scalar(3,ScalarKind::U64,g.binding,8) ||
         !writer.PutCount({{},196,4,4},g.count) || !writer.PutLinks({{},196,4,4},false,255,std::span(g.anchors).first(g.count)) ||
         !writer.FinishRoot())return false;
      ++count;
    }
    return true;
  }
  // 独立枚举已核原生列表，前后两遍保存每个指针；不借Recorder batch槽或Lua枚举。
  bool Members(bool check) noexcept {
    struct Context {Impl &source;unsigned id=0;bool failed=false;} context{*this};
    world::NativeSource reader;reader.owner=&context;reader.game=reading_window.game;reader.room=reading_window.room_address;
    reader.copy=[](void *p,std::uint64_t address,void *out,std::size_t bytes) noexcept {auto &c=*static_cast<Context *>(p);auto &s=c.source;
      return s.collecting && s.Main() && s.capture.collector->NativeWindowCurrent(s.reading_window) && address && address<=UINT32_MAX &&
        bytes<=(std::uint64_t{1}<<32)-address && s.capture.source_memory.copy(std::uint32_t(address),out,bytes);};
    reader.incomplete=[](void *p,world::Reason) noexcept {static_cast<Context *>(p)->failed=true;};
    std::array<std::uint32_t,4> fixed{};std::uint32_t used=0;
    auto output=check?std::span(member_check):std::span(members);
    for(unsigned id=0;id<10;++id){if(id==6)continue;context.id=id;world::RawSource actual;
      world::EnumerateSource(reader,id,actual,output.subspan(used),fixed,&context,
        check?+[](void *,std::uint32_t,std::uint32_t) noexcept {}:
        +[](void *p,std::uint32_t address,std::uint32_t index) noexcept {auto &c=*static_cast<Context *>(p);const auto id=c.id;
          const auto flags=id<=5 || id==7?1u|(id==3?2u:0u)|(id==2?4u:0u):0u;
          if((id==8 && index>=kNullIndex) || !c.source.Add(address,flags,id==8?std::uint16_t(index):kNullIndex))c.failed=true;});
      if(context.failed || actual.status || actual.count>output.size()-used)return false;
      if(check){const auto &prior=member_sources[id];
        if(prior.count!=actual.count || prior.descriptor!=actual.descriptor ||
           !std::equal(output.begin()+used,output.begin()+used+actual.count,members.begin()+used))return false;
      }else member_sources[id]=actual;
      used+=actual.count;
    }
    return true;
  }
  // 模板池不产生活动成员；C/S拓扑另补齐全部有独立初始化依据的存储／非活动身体。
  bool Seed() noexcept {
    if(!Members(false))return false;
    if(auto *runtime=Runtime();runtime&&!runtime->finished&&!runtime->retiring&&!runtime->failure)
      for(unsigned ordinal=0;ordinal<runtime->session.ParticipantLimit();++ordinal)if(groups[ordinal].present)
        for(unsigned b=0;b<runtime->topology[ordinal].body_count;++b)
          if(!Add(world::PackWord(runtime->topology[ordinal].bodies[b].player)))return false;
    // anchors保留原注册顺序，不按地址或角色重排，不把inactive存储体变成活动锚点。
    for(unsigned slot=0;slot<world::kControlAnchorCapacity;++slot){const auto anchor=capture.collector->CurrentAnchor(slot);if(!anchor.valid)continue;
      auto *f=Add(anchor.player);const auto ordinal=slot/world::kControlBodiesPerParticipant;
      if(!f || !f->group_known || f->ordinal!=ordinal || !groups[ordinal].present || !(f->control_flags&1))return false;
      auto &g=groups[ordinal];if(g.count==g.anchors.size())return false;
      auto ref=Register(f->subject);if(ref.kind==CapturedReferenceKind::Invalid)return false;
      for(std::uint32_t i=0;i<g.count;++i)if(g.anchors[i].handle==ref.handle)return false;
      g.anchors[g.count++]=ref;
    }
    return true;
  }
  // 正式结构owner独立于Lua层级缓存；每次ordinary前后及Clock提交前核相同完整来源。
  bool SceneRead(bool check) noexcept {
    if(!reading_window.ordinary || floor_depth || !floor_ready)return false;
    if(floor_run!=reading_window.run){if(check)return false;if(next_floor>=UINT64_MAX/3-1)return false;
      floor_run=reading_window.run;floor=++next_floor;descriptor_shapes={};descriptor_revisions={};}
    NativeScene current;current.game=reading_window.game;current.room=reading_window.room_address;
    if(current.game>UINT32_MAX-0x25f44 || current.room>UINT32_MAX-30872)return false;
    const auto read=[&](std::uint64_t address,auto &value){return NativeValue(address,value);};
    std::uint8_t room_initialized=0,level_initializing=0;std::array<std::uint32_t,2> dimensions{};
    std::uint32_t dimension_pointer=0;std::array<std::int32_t,507> occupancy{};
    if(!read(current.room,room_initialized) || room_initialized!=1 || !read(std::uint64_t(current.game)+0x183a0,level_initializing) || level_initializing ||
       !read(std::uint64_t(current.room)+4,current.current_descriptor) || !current.current_descriptor ||
       !read(std::uint64_t(current.room)+12,dimensions) || !dimensions[0] || !dimensions[1] || std::uint64_t(dimensions[0])*dimensions[1]>448 ||
       !read(std::uint64_t(current.game)+0x1830c,current.dimension) || current.dimension>2 ||
       !read(std::uint64_t(current.game)+0x182cc,current.ordinary_count) || current.ordinary_count>507 ||
       !read(std::uint64_t(current.game)+0x182c8,dimension_pointer) || dimension_pointer!=current.game+0x17adc+current.dimension*0x2a4 ||
       !read(std::uint64_t(current.game)+0x17adc,occupancy) || !read(std::uint64_t(current.room)+36,current.grids))return false;
    current.extent=dimensions[0]*dimensions[1];
    bool found_current=false;
    for(unsigned slot=0;slot<527;++slot){if(slot==525)continue;const auto address64=std::uint64_t(current.game)+20+slot*184u;
      if(address64+184>UINT32_MAX)return false;const auto address=std::uint32_t(address64);
      bool present=slot<current.ordinary_count;
      if(slot>=507){std::array<std::uint32_t,2> configs{};if(!read(address64+16,configs))return false;present=configs[0]||configs[1];}
      if(address==current.current_descriptor){found_current=true;if(!present)return false;}
      if(!present)continue;
      std::array<std::uint32_t,15> shape{};if(!read(address64,shape))return false;
      if(shape[3]>2 || (!shape[4]&&!shape[5]))return false;
      if(check){if(!descriptor_revisions[slot] || shape!=descriptor_shapes[slot])return false;}
      else if(!descriptor_revisions[slot] || shape!=descriptor_shapes[slot]){
        if(descriptor_revisions[slot]==UINT64_MAX)return false;++descriptor_revisions[slot];descriptor_shapes[slot]=shape;}
      current.descriptors[slot]=address;current.descriptor_order[current.descriptor_count++]=std::uint16_t(slot);
    }
    if(!found_current)return false;
    for(unsigned i=0;i<507;++i){const auto slot=occupancy[i];if(slot== -1)current.occupancy[i]=kNullIndex;
      else if(slot<0 || slot>=527 || slot==525 || !current.descriptors[std::size_t(slot)])return false;
      else current.occupancy[i]=std::uint16_t(slot);}
    if(check)return current.game==scene.game && current.room==scene.room && current.current_descriptor==scene.current_descriptor && current.extent==scene.extent &&
      current.dimension==scene.dimension && current.ordinary_count==scene.ordinary_count && current.descriptor_count==scene.descriptor_count &&
      current.grids==scene.grids && current.descriptors==scene.descriptors && current.occupancy==scene.occupancy && current.descriptor_order==scene.descriptor_order;
    scene=current;return true;
  }
  bool SceneSeed(CaptureDiagnostic &diagnostic) noexcept {
    scene_root_count=0;
    const auto push=[&](EntityFacts facts){if(scene_root_count==scene_roots.size())return false;
      facts.scene=&scene;facts.ready=true;const auto link=Register(facts.subject);if(link.kind==CapturedReferenceKind::Invalid)return false;
      links[std::size_t(link.handle-1)].source_slot=facts.source_slot;
      scene_roots[scene_root_count++]=facts;return true;};
    EntityFacts context;context.address=scene.game;context.subject={4,floor,1,1};context.layout=276;context.scene=&scene;context.ready=true;
    if(!source.ReadSceneContext(reading_witness,context,scene,diagnostic))return false;
    // 全448槽和八门都扫描；同一对象只捕获一次，八槽引用各自保留真实别名。
    for(unsigned slot=0;slot<456;++slot){const auto address=scene.grids[slot];if(!address)continue;
      std::uint32_t index=0;std::uint8_t initialized=0;
      if(!NativeValue(std::uint64_t(address)+0x24,index) || !NativeValue(std::uint64_t(address)+0x1c,initialized))return FailCapture(diagnostic,CaptureFailure::ReadFailed,{{2},address,0,199},1);
      if(initialized!=1 || index>=448)return FailCapture(diagnostic,CaptureFailure::MissingQualification,{{2},address,0,199},1);
      std::int32_t actual=std::int32_t(slot);
      if(slot>=448 && !NativeValue(std::uint64_t(scene.room)+1860+(slot-448)*4,actual))return FailCapture(diagnostic,CaptureFailure::ReadFailed,{{2},address,0,199},1);
      if(actual<0 || std::uint32_t(actual)!=index)return FailCapture(diagnostic,CaptureFailure::SourceChanged,{{2},address,0,199},1);
      const auto *id=capture.identities.Find(world::ObjectDomain::Grid,address);
      if(!id || id->Retired() || id->candidate || !id->readiness)return FailCapture(diagnostic,CaptureFailure::MissingQualification,{{2},address,0,199});
      const CaptureSubject subject{2,id->local_id,id->use,id->revision};auto link=Register(subject);if(link.kind==CapturedReferenceKind::Invalid)return false;
      scene.grid_handles[slot]=link.handle;
      bool prior=false;for(std::uint32_t i=0;i<scene_root_count;++i)if(scene_roots[i].subject==subject){prior=true;break;}
      if(prior)continue;EntityFacts facts;facts.subject=subject;facts.address=address;facts.grid_index=index;facts.source_slot=std::uint16_t(index);
      if(!push(facts))return false;
    }
    for(unsigned i=0;i<scene.descriptor_count;++i){const auto slot=scene.descriptor_order[i];EntityFacts facts;
      facts.subject={6,std::uint64_t(slot)+1,floor,descriptor_revisions[slot]};facts.address=scene.descriptors[slot];facts.layout=270;facts.source_slot=slot;
      if(!push(facts))return false;const auto link=Register(facts.subject);links[std::size_t(link.handle-1)].order=std::uint16_t(i);}
    if(!push(context))return false;context.address=scene.room;context.subject={7,reading_window.room,1,1};context.layout=239;
    if(!push(context))return false;
    context.address=scene.game;context.subject={3,reading_window.run,1,1};context.layout=237;
    context.progress_ready=progress_revision && progress_scope==reading_witness.scope;context.progress_blocked=progress_blocked;
    std::array<std::uint32_t,16> current_score{};std::array<std::uint32_t,15> components{};
    context.score_ready=score.revision && score.scope==reading_witness.scope &&
      NativeValue(std::uint64_t(scene.game)+0x25c5c,components) && NativeValue(std::uint64_t(scene.game)+0x25ca8,current_score[15]);
    std::copy(components.begin(),components.end(),current_score.begin());context.score_ready=context.score_ready&&current_score==score.values;
    return push(context);
  }
  bool SceneInto(CaptureValueWriter &writer,std::uint32_t &count,CaptureDiagnostic &diagnostic) noexcept {
    for(std::uint32_t i=0;i<scene_root_count;++i)if(!source.Capture(reading_witness,scene_roots[i],BodyStream::World,writer,diagnostic,true))return false;else ++count;
    return true;
  }
  void DropFast(unsigned i) noexcept {if(fast_keys[i].local)exchange.RetireBody(fast_keys[i]);fast_keys[i]={};fast_addresses[i]=0;}
  // 发布只借用当前主线程窗口；Source的字段读回调在同步Capture结束后不再有效。
  bool Publish(Pending &p) noexcept {
    ReadWindow(p);
    const auto resources=source.Resources();
    if(!p.ticket.Values()->Catalog({0,std::uint32_t(resources.entries.size()),std::uint32_t(resources.bytes.size()),std::uint32_t(resources.layers.size())},
        std::span(links).first(link_used))){collecting=false;return false;}
    struct Publication {Impl &source;Pending &pending;} publication{*this,p};
    CaptureWindow window{producer,p.witness,&publication,
      [](void *context,const CaptureWitness &w,const CaptureRoot &root) noexcept {auto &publication=*static_cast<Publication *>(context);
        return root.subject==publication.pending.facts.subject && root.address==publication.pending.facts.address && publication.source.Current(w,publication.pending.facts);},nullptr};
    const bool result=p.ticket.Publish(window,p.anchor,p.roots);collecting=false;return result;
  }
};
// 正常生产只借用本调用的原生窗口；身份仍由Collector持有，显示对象地址不成为网络身份。
EntityFacts BinaryEntities::Impl::ProductionBody(std::uint32_t body,const Produced &p) noexcept {
  EntityFacts out;const auto *id=capture.identities.Find(world::ObjectDomain::Entity,body);
  if(!id || id->Retired() || id->candidate || id->readiness<2)return out;
  out.address=out.identity_address=body;out.subject={1,id->local_id,id->use,id->revision};out.ready=out.player_ready=true;
  Control(out);out.subject.topology=source.Topology(out.subject);out.manager=p.manager;return out;
}
bool BinaryEntities::Impl::SaveProduction(Produced &p,EntityFacts facts,std::uint16_t layout,std::uint64_t instance) noexcept {
  ReadProduction(p);CaptureDiagnostic diagnostic;facts.layout=layout;
  CaptureValueWriter writer(production_scratch,p.witness);
  const auto *description=FindLayout(layout);bool body=false;
  if(description)for(unsigned i=0;i<description->block_count;++i)body|=Blocks()[description->first_block+i].body;
  bool success=source.Capture(p.witness,facts,body?BodyStream::Body:BodyStream::World,writer,diagnostic,true);
  CaptureRecord origin;origin.production=p.witness;origin.owner=facts.subject;origin.instance=instance;origin.generation=capture.epoch;
  origin.purpose=layout;origin.call=p.token;origin.parent=p.parent;
  if(success)success=writer.Production(origin);
  const auto resources=source.Resources();CapturePacket packet;
  if(success)success=writer.Catalog({0,std::uint32_t(resources.entries.size()),std::uint32_t(resources.bytes.size()),std::uint32_t(resources.layers.size())},std::span(links).first(link_used))&&writer.Seal(1,packet);
  if(success)success=production.Store(packet,diagnostic);
  collecting=false;
  if(!success){production.RetirePurpose(facts.subject,layout);Failure(diagnostic.reason==CaptureFailure::None?writer.Diagnostic():diagnostic);}return success;
}
void BinaryEntities::Impl::ClearProduction() noexcept {
  production.Clear();score={};probability={};progress_revision=0;progress_scope={};planetarium_known=false;
  for(auto &hud:hud_states){hud.subject={};hud.scope={};hud.address=hud.body=0;hud.instance=hud.mantle=0;hud.clock={};hud.complete=hud.stats=hud.history=false;
    hud.rows={};hud.rows_initialized=false;hud.tracker->Retire();}
}
// 自有纯显示生产器：以真实Stat正常tick为时钟，所有身体各自维护差值与相位。
// 六个输入是Host已核最终属性，仅在这里换成原生显示单位；Client仍只消费最终三值。
// 概率优先复用本帧正常结果，按用户明确例外复用C++查询补缺；不运行HUD Update／Recompute或Lua API。
void BinaryEntities::Impl::ProduceOwnedStats(Produced &p) noexcept {
  ReadProduction(p);if(!Groups()){collecting=false;return;}
  std::uint8_t unlocked=0,override_flag=0;std::uint32_t mode=0,override_count=0;
  const bool qualification=NativeValue(std::uint64_t(p.manager)+0x1e2,unlocked)&&NativeValue(std::uint64_t(p.manager)+8,mode)&&
    NativeValue(std::uint64_t(p.window.game)+0x25ba8,override_count)&&NativeValue(std::uint64_t(p.window.game)+0x25b01,override_flag);
  planetarium_known=qualification;planetarium_qualified=qualification&&(unlocked || (mode==2&&(override_count||override_flag)));
  // 当前原生HUD最终值优先；只补当前scope／帧缺失的概率位，不为每个参与者再算一遍。
  std::uint8_t missing=0;std::array<float,3> probability_values{};
  for(unsigned row=0;row<3;++row){if(row==2&&!planetarium_qualified)continue;
    if(probability.scope==p.witness.scope&&(probability.mask&(1u<<row))&&probability.frames[row]==p.witness.game_frame)probability_values[row]=probability.values[row];
    else missing|=std::uint8_t(1u<<row);}
  if(missing){EntityFacts context;context.subject={3,p.window.run,1,1};context.address=p.window.game;context.manager=p.manager;context.ready=true;
    CaptureDiagnostic diagnostic;BinaryProbabilityInput input;
    const auto read_inputs=[&](BinaryProbabilityInput &value){ReadProduction(p);return
      NativeValue(std::uint64_t(p.window.game)+0x25ac0,value.game_flags)&&NativeValue(std::uint64_t(p.window.game)+0x1839c,value.level_flags)&&
      NativeValue(std::uint64_t(p.window.game)+0x25ae8,value.donation)&&NativeValue(std::uint64_t(p.window.game)+0x25ae0,value.deals)&&
      NativeValue(std::uint64_t(p.window.game)+0x182f4,value.angel_modifier);};
    bool valid=!(missing&3)||(source.HudDealEligibility(p.witness,context,input.eligible,diagnostic)&&read_inputs(input));
    if(valid)valid=QueryBinaryHudProbability(reinterpret_cast<void *>(p.window.game),reinterpret_cast<void *>(p.window.room_address),missing,input,probability_values);
    ReadProduction(p);valid=valid&&capture.collector->NativeWindowCurrent(p.window);
    if(valid&&(missing&3)){BinaryProbabilityInput after;valid=read_inputs(after)&&input.game_flags==after.game_flags&&input.level_flags==after.level_flags&&
      input.donation==after.donation&&input.deals==after.deals&&input.angel_modifier==after.angel_modifier;}
    if(valid){
      if(probability.scope!=p.witness.scope){probability={};probability.scope=p.witness.scope;}
      for(unsigned row=0;row<3;++row)if(missing&(1u<<row)){probability.values[row]=probability_values[row];probability.frames[row]=p.witness.game_frame;probability.mask|=std::uint8_t(1u<<row);}
    }else Failure(diagnostic.reason==CaptureFailure::None?CaptureDiagnostic{CaptureFailure::SourceChanged,{},317}:diagnostic);}
  std::array<float,10> thresholds{};bool template_valid=true;
  for(unsigned row=0;row<thresholds.size();++row)template_valid=template_valid&&
    NativeValue(std::uint64_t(p.object)+292+row*20,thresholds[row])&&std::isfinite(thresholds[row])&&thresholds[row]>=0;
  for(auto &hud:hud_states){if(!hud.body || hud.scope!=p.witness.scope)continue;
    ReadProduction(p);auto facts=ProductionBody(hud.body,p);
    if(facts.subject!=hud.subject || facts.ordinal>=4 || !(facts.control_flags&1))continue;
    hud.stats=false;std::array<float,6> values{};std::uint32_t birth=0;
    constexpr std::uint32_t offsets[]={0x155c,0x145c,0x146c,0x147c,0x1460,0x1560};
    bool valid=template_valid&&qualification&&NativeValue(std::uint64_t(hud.body)+808,birth);
    for(unsigned row=0;row<values.size();++row)valid=valid&&NativeValue(std::uint64_t(hud.body)+offsets[row],values[row])&&std::isfinite(values[row]);
    if(valid){values[1]=float(30.0f/float(values[1]+1.0f));values[3]=float(values[3]/40.0f);
      valid=std::isfinite(values[1])&&std::isfinite(values[3]);}
    if(!valid){production.RetirePurpose(hud.subject,317);Failure({CaptureFailure::MissingQualification,hud.subject,317});continue;}
    if(!hud.rows_initialized){hud.rows={};for(unsigned row=0;row<hud.rows.size();++row)hud.rows[row].threshold=thresholds[row];hud.rows_initialized=true;}
    const bool animate=std::bit_cast<std::int32_t>(std::uint32_t(p.witness.game_frame-birth))>1;
    const auto update=[&](unsigned index,float value){auto &row=hud.rows[index];const auto old=row.value;
      if(!animate){row.previous=old;row.delta=0;row.countdown=0;}
      else if(value!=old){if(row.countdown>30){row.delta=float(value-row.previous);row.countdown=std::max(row.countdown,135);}
        else {row.previous=old;row.delta=float(value-old);if(std::fabs(row.delta)>=row.threshold)row.countdown=150;}}
      row.value=value;};
    for(unsigned row=0;row<values.size();++row)update(row,values[row]);
    for(unsigned row=0;row<3;++row){const bool needed=row<2||planetarium_qualified;
      if(needed && (probability.scope!=p.witness.scope || !(probability.mask&(1u<<row)) || probability.frames[row]!=p.witness.game_frame)){valid=false;continue;}
      if(needed)update(row+6,probability.values[row]);}
    for(auto &row:hud.rows)if(row.countdown>0)--row.countdown;
    if(!valid){production.RetirePurpose(hud.subject,317);Failure({CaptureFailure::MissingQualification,hud.subject,317});continue;}
    facts.stat_rows=&hud.rows;facts.planetarium_known=true;facts.planetarium_qualified=planetarium_qualified;
    hud.stats=SaveProduction(p,facts,317,hud.instance);
  }
  collecting=false;
}
// 必要身体来源不因原生只有两槽而默认缺席；合法尚未产生的结果保持明确缺口。
bool BinaryEntities::Impl::ProductionInto(CaptureValueWriter &writer,std::uint32_t &count,CaptureDiagnostic &diagnostic) noexcept {
  for(std::uint32_t i=0;i<root_used;++i){const auto &f=roots[i];if(f.ordinal>=4 || !(f.control_flags&1))continue;
    const auto at=std::find_if(hud_states.begin(),hud_states.end(),[&](const auto &hud){return hud.subject==f.subject && hud.scope==reading_witness.scope && hud.complete;});
    if(at==hud_states.end())return FailCapture(diagnostic,CaptureFailure::MissingQualification,{f.subject,0,0,315});
    if(!at->stats)return FailCapture(diagnostic,CaptureFailure::MissingQualification,{f.subject,0,0,317});
    if(!at->history)return FailCapture(diagnostic,CaptureFailure::MissingQualification,{f.subject,0,0,321});
  }
  // 已登记但尚未取得正常来源的必需world用途不能被空列表掩盖。
  // 原版分支“不显示”同样要由该owner明确生产present=0，不能仅因本端没有调用就推导缺席。
  constexpr std::uint16_t required[]={323,325,356,367,368,369,370,371,373,374,375,376,377,378,379,380};
  for(const auto purpose:required)if(!production.Contains(reading_witness.scope,purpose))
    return FailCapture(diagnostic,CaptureFailure::MissingQualification,{{3,reading_witness.scope.run,1,1},0,0,purpose});
  if(!production.Append(reading_witness.scope,writer,count,diagnostic,{this,
    [](void *owner,const CaptureReferenceEntry &ref) noexcept {auto &s=*static_cast<Impl *>(owner);
      bool present=false;
      if(ref.space==1)for(std::uint32_t i=0;i<s.root_used;++i){const auto &f=s.roots[i];if(f.subject.local==ref.local&&f.subject.use==ref.use){present=true;break;}}
      else present=ref.space==3 || ref.space==9 || ref.space==10;
      return present?s.Register({ref.space,ref.local,ref.use}):CaptureLink{CapturedReferenceKind::Invalid,0};}}))return false;
  return AudioInto(writer,count,diagnostic);
}
// voice与可靠事件分开保留；ordinary只复制，不因latest替换丢事件或重启同一声部。
bool BinaryEntities::Impl::AudioInto(CaptureValueWriter &writer,std::uint32_t &count,CaptureDiagnostic &d) noexcept {
  if(audio_gap)return FailCapture(d,CaptureFailure::Incomplete,{{3,reading_witness.scope.run,1,1},0,0,383});
  const auto append=[&](const world::binary::NativeAudioValue &value,const CaptureWitness &origin,std::uint64_t instance,std::uint64_t generation,std::uint64_t sequence,std::uint16_t layout){
    EntityFacts f;f.subject={3,reading_witness.scope.run,1,1};f.address=scene.game;f.layout=layout;f.ready=true;f.scene=&scene;f.audio=&value;
    if(!source.Capture(reading_witness,f,BodyStream::World,writer,d,true))return false;
    CaptureRecord record;record.production=origin;record.owner=f.subject;record.instance=instance;record.generation=generation;record.purpose=layout;
    record.event_sequence=sequence;if(!writer.Production(record))return false;++count;return true;};
  for(const auto &voice:voices)if(voice.active && voice.value.loop && voice.production.scope.run==reading_witness.scope.run)
    if(!append(voice.value,voice.production,voice.instance,voice.generation,0,382))return false;
  for(unsigned i=0;i<audio_event_count;++i){const auto &event=audio_events[i];if(event.production.scope.run!=reading_witness.scope.run)continue;
    if(!append(event.value,event.production,event.instance,event.generation,event.sequence,383))return false;}
  return true;
}
// 可靠事件不因latest状态替换丢失；停止只退休所指实例，容量不足明确撤销音频完整性。
bool BinaryEntities::Impl::AudioEventFor(AudioVoice &voice,const Produced &p,std::uint8_t kind) noexcept {
  if(audio_event_count==audio_events.size() || next_audio_event==UINT64_MAX){audio_gap=true;Failure({CaptureFailure::ValueBudget,{},383});return false;}
  auto &event=audio_events[audio_event_count++];event={voice.value,p.witness,voice.instance,voice.generation,++next_audio_event};event.value.kind=kind;
  voice.production=p.witness;if(kind==2)voice.active=false;return true;
}
// 正常控制返回后更新自有逻辑声部，不再次查询后端IsPlaying，也不将设备丢失广播为Stop。
// 这里控制的集合来自已接受的world实例；本地Options单独刷新不进入此world生产范围。
void BinaryEntities::Impl::AudioControl(Produced &p) noexcept {
  if(!p.audio_world)return;ReadProduction(p);std::array<std::uint32_t,3> table{};float modifier=0;
  if(!NativeValue(p.object,table)||table[0]>table[1]||table[1]>table[2]||(table[1]-table[0])%440 ||
     !NativeValue(std::uint64_t(p.object)+16,modifier)||!std::isfinite(modifier)){audio_gap=true;Failure({CaptureFailure::SourceChanged,{},383});return;}
  const auto count=(table[1]-table[0])/440;
  for(auto &voice:voices){if(!voice.active || voice.production.scope.run!=p.witness.scope.run)continue;
    const bool all=p.kind==BinaryProductionKind::SfxRemix||p.kind==BinaryProductionKind::SfxStopLoops;
    const bool ambient=p.kind==BinaryProductionKind::SfxAmbient;
    if(ambient?(voice.ambient!=p.sound||p.volume>0.009999999776482582f):(!all&&voice.sound!=std::uint32_t(p.sound)))continue;
    if(voice.sound>=count){audio_gap=true;Failure({CaptureFailure::SourceChanged,{},383});continue;}
    if(p.kind==BinaryProductionKind::SfxStop || (p.kind==BinaryProductionKind::SfxStopLoops&&voice.value.loop) || ambient){AudioEventFor(voice,p,2);continue;}
    if(p.kind==BinaryProductionKind::SfxStopLoops)continue;
    auto value=voice.value;
    if(p.kind==BinaryProductionKind::SfxPitch)value.pitch=p.pitch;
    else {float volume=p.volume;if(p.kind==BinaryProductionKind::SfxRemix &&
        !NativeValue(std::uint64_t(table[0])+std::uint64_t(voice.sound)*440+404,volume)){audio_gap=true;Failure({CaptureFailure::ReadFailed,{},381});continue;}
      value.gain=float(std::max(modifier,0.0f)*volume);value.mix_rule=p.kind==BinaryProductionKind::SfxRemix?1:0;}
    if(!std::isfinite(value.gain)||!std::isfinite(value.pitch)){audio_gap=true;Failure({CaptureFailure::InvalidValue,{},381});continue;}
    if(value.gain!=voice.value.gain||value.pitch!=voice.value.pitch||value.mix_rule!=voice.value.mix_rule){voice.value=value;AudioEventFor(voice,p,4);}
  }
  collecting=false;
}
BinaryEntities::BinaryEntities(WorldCapture &c,std::thread::id p,std::thread::id worker):impl_(std::make_unique<Impl>(c,p,worker)) {}
BinaryEntities::~BinaryEntities()=default;
bool BinaryEntities::Active() const noexcept {return impl_->stats.active;}
void BinaryEntities::Active(bool active) noexcept {auto &s=*impl_;if(!s.Main() || active==s.stats.active)return;
  if(active&&!StreamResourceHooksInstalled()){s.Failure({CaptureFailure::MissingQualification});return;}
  s.ordinary={};for(auto &body:s.bodies)body={};s.fast_keys={};s.fast_addresses={};s.last_ordinary=0;s.published_count=0;s.source.Invalidate();
  s.ClearProduction();s.voices={};s.audio_events={};s.audio_event_count=0;s.audio_gap=false;s.production_depth=0;
  s.invalidated=false;if(active)s.stats.active=s.exchange.Begin();else {s.exchange.Stop();s.stats.active=false;}}
void BinaryEntities::FrameBegin() noexcept {auto &s=*impl_;if(!s.Main())return;s.ordinary={};s.ordinary_count=0;s.link_used=0;for(auto &body:s.bodies)body={};s.collecting=false;
  if(s.stats.active && s.capture.collector && ((s.run && s.run!=s.capture.collector->CurrentRunToken()) ||
      (s.room && s.room!=s.capture.collector->CurrentRoomToken()))){
    for(std::uint32_t i=0;i<s.scene_root_count;++i)s.source.Retire(s.scene_roots[i].subject);
    s.scene_root_count=0;s.ClearProduction();
    if(s.run && s.run!=s.capture.collector->CurrentRunToken()){s.voices={};s.audio_events={};s.audio_event_count=0;s.audio_gap=false;}
    Invalidate();}}
void BinaryEntities::FrameEnd() noexcept {auto &s=*impl_;if(!s.Main())return;
  bool pending=s.ordinary.ticket.Values()!=nullptr;s.ordinary={};for(auto &body:s.bodies){pending|=body.ticket.Values()!=nullptr;body={};}
  s.collecting=false;if(pending)s.Failure({CaptureFailure::Incomplete});}
void BinaryEntities::Prepare(bool normal) noexcept {
  auto &s=*impl_;if(!s.Main() || !s.stats.active || !normal || !s.capture.collector)return;
  s.root_used=0;
  if(!s.Groups()){s.Failure({CaptureFailure::MissingQualification});return;}
  world::NativeCaptureWindow window;
  if(s.capture.collector->NativeOrdinary(window)){
    if((s.run && s.run!=window.run) || (s.room && s.room!=window.room))Invalidate();s.run=window.run;s.room=window.room;
    auto &p=s.ordinary;p.window=window;p.resource_change=s.resource_mutation;p.witness=s.Witness(window,CapturePhase::Ordinary);
    s.ReadWindow(p);CaptureFailure reason;
    CaptureDiagnostic diagnostic;
    bool success=s.SceneRead(false);
    if(success){p.witness.scope.floor=s.floor;p.witness.scope.dimension=s.floor*3+s.scene.dimension+1;s.ReadWindow(p);success=s.Seed()&&s.SceneSeed(diagnostic);}
    if(!success)s.Failure(diagnostic.reason==CaptureFailure::None?CaptureDiagnostic{CaptureFailure::MissingQualification,{},276}:diagnostic);
    if(success){p.ticket=s.exchange.Acquire(CaptureLane::Ordinary,p.witness,{},reason);success=p.ticket.Values()!=nullptr;
      if(!success)s.Failure({reason});}
    if(success){auto &writer=*p.ticket.Values();
      // references可把尚未枚举的已初始化目标追加到队尾，任何目标失败都回卷整包。
      for(std::uint32_t i=0;i<s.root_used && success;++i){auto &facts=s.roots[i];
        if(s.Register(facts.subject).kind==CapturedReferenceKind::Invalid){s.Failure({CaptureFailure::ReferencePending,facts.subject});success=false;break;}
        success=s.source.Capture(p.witness,facts,BodyStream::World,writer,diagnostic,true);
        if(!success)break;facts.subject=writer.Root().subject;++p.roots;
        if(!p.anchor.address){p.anchor={facts.subject,facts.address,0,writer.Root().layout};p.facts=facts;}
        if(writer.Root().layout==197){success=s.source.Capture(p.witness,facts,BodyStream::Body,writer,diagnostic,true);if(success){facts.subject=writer.Root().subject;++p.roots;}}
      }
      if(success)success=s.SceneInto(writer,p.roots,diagnostic);
      if(success)success=s.GroupsInto(writer,p.roots);
      if(success)success=s.ProductionInto(writer,p.roots,diagnostic);
      if(success)success=s.Members(true);
      if(success)success=s.SceneRead(true);
      if(!success && diagnostic.reason==CaptureFailure::None && writer.Diagnostic().reason==CaptureFailure::None)diagnostic.reason=CaptureFailure::SourceChanged;
      if(!success)s.Failure(diagnostic.reason==CaptureFailure::None?writer.Diagnostic():diagnostic);
    }
    if(!success || !p.anchor.address)p={};else {s.ordinary_count=s.root_used;std::copy_n(s.roots.begin(),s.ordinary_count,s.ordinary_facts.begin());}
    s.collecting=false;
  }
  // 快速槽仅服务真实当前PlayerPost。其余身体已经通过ordinary联合根保留，绝不截断成员。
  for(unsigned slot=0;slot<world::kControlAnchorCapacity;++slot){world::PlayerPostWitness body;
    if(!s.capture.collector->NativeBody(slot,window,body))continue;
    if(window.run!=s.run || window.room!=s.room){Invalidate();continue;}
    auto *f=s.Add(body.anchor.player);if(!f || !f->ready || !f->group_known){s.Failure({CaptureFailure::MissingQualification});continue;}
    f->subject.binding=body.anchor.generation;f->subject.topology=s.source.Topology(f->subject);
    unsigned lane=unsigned(s.fast_keys.size());
    for(unsigned i=0;i<s.fast_keys.size();++i)if(s.fast_addresses[i]==f->address){lane=i;if(s.fast_keys[i]!=f->subject)s.DropFast(i);break;}
    if(lane==s.fast_keys.size())for(unsigned i=0;i<s.fast_keys.size();++i)if(!s.fast_keys[i].local){lane=i;break;}
    if(lane==s.fast_keys.size()){++s.stats.joint;continue;}
    if(!s.last_ordinary || !f->subject.topology){++s.stats.joint;continue;}
    auto &p=s.bodies[lane];p.window=window;p.witness=s.Witness(window,CapturePhase::PlayerPost);p.resource_change=s.resource_mutation;p.facts=*f;
    CaptureFailure reason;p.ticket=s.exchange.Acquire(CaptureLane::Body,p.witness,f->subject,reason);
    if(!p.ticket.Values()){if(reason==CaptureFailure::NeedsOrdinaryJoint)++s.stats.joint;else s.Failure({reason,f->subject});p={};continue;}
    s.ReadWindow(p);CaptureDiagnostic diagnostic;const bool success=s.source.Capture(p.witness,*f,BodyStream::Body,*p.ticket.Values(),diagnostic);
    s.collecting=false;
    if(!success){s.exchange.RetireBody(f->subject);s.DropFast(lane);
      if(diagnostic.reason==CaptureFailure::NeedsOrdinaryJoint)++s.stats.joint;else s.Failure(diagnostic);p={};continue;}
    f->subject=p.ticket.Values()->Root().subject;p.facts=*f;p.anchor={f->subject,f->address,0,197};p.roots=1;
    s.fast_keys[lane]=f->subject;s.fast_addresses[lane]=f->address;
  }
}
void BinaryEntities::Commit(bool committed,const core::TickPostState &post) noexcept {
  auto &s=*impl_;if(!s.Main() || !s.stats.active)return;
  if(!s.capture.collector || !s.capture.collector->Recording()){Invalidate();return;}
  const bool match=committed && s.capture.collector && s.capture.collector->NativePostMatches(post);
  auto &ordinary=s.ordinary;
  if(ordinary.ticket.Values()){
    bool valid=match;s.ReadWindow(ordinary);
    world::NativeCaptureWindow current;
    if(valid)valid=s.capture.collector->NativeOrdinary(current) && current.run==ordinary.window.run && current.room==ordinary.window.room &&
      current.outer==ordinary.window.outer && current.mutation==ordinary.window.mutation && current.game_frame==ordinary.window.game_frame && s.Members(true) && s.SceneRead(true);
    for(std::uint32_t i=0;i<s.ordinary_count && valid;++i)valid=s.Current(ordinary.witness,s.ordinary_facts[i]);
    for(std::uint32_t i=0;i<s.scene_root_count && valid;++i)valid=s.Current(ordinary.witness,s.scene_roots[i]);
    s.collecting=false;
    if(valid && s.Publish(ordinary)){
      s.invalidated=false;s.last_ordinary=ordinary.witness.revision;++s.stats.ordinary;
      s.published_count=s.ordinary_count;for(std::uint32_t i=0;i<s.published_count;++i)s.published_addresses[i]=s.ordinary_facts[i].address;
      std::sort(s.published_addresses.begin(),s.published_addresses.begin()+s.published_count);
      for(std::uint32_t i=0;i<s.ordinary_count;++i)s.source.OrdinaryCommitted(s.ordinary_facts[i].subject);
    }else s.Failure({CaptureFailure::SourceChanged,ordinary.anchor.subject});
    ordinary={};
  }
  for(auto &p:s.bodies)if(p.ticket.Values()){
    if(match && s.Publish(p))++s.stats.bodies;else s.Failure({CaptureFailure::SourceChanged,p.anchor.subject});p={};
  }
  s.collecting=false;
}
void BinaryEntities::Invalidate() noexcept {auto &s=*impl_;if(!s.Main() || !s.stats.active)return;
  s.ordinary={};for(auto &body:s.bodies)body={};s.last_ordinary=0;s.published_count=0;
  if(s.invalidated)return;s.invalidated=true;
  if(s.resource_mutation==UINT64_MAX){s.stats.active=false;s.exchange.Stop(CaptureFailure::SequenceExhausted);return;}++s.resource_mutation;s.source.Invalidate();
  for(auto &key:s.fast_keys)if(key.local){s.exchange.RetireBody(key);key={};}s.fast_addresses={};s.exchange.Invalidate(CaptureFailure::Revoked);}
void BinaryEntities::RetireBody(std::uint32_t address) noexcept {auto &s=*impl_;if(!s.Main())return;
  for(auto &hud:s.hud_states)if(hud.address && hud.subject.local){const auto *id=s.capture.identities.Find(world::ObjectDomain::Entity,address);
    if(id&&hud.subject.local==id->local_id){s.production.Retire(hud.subject);hud.complete=hud.stats=hud.history=false;hud.mantle=0;hud.tracker->Retire();
      if(id->Retired()){hud.subject={};hud.address=hud.body=0;hud.instance=0;hud.rows={};hud.rows_initialized=false;}}}
  if(address&&s.Known(address))Invalidate();
  for(unsigned i=0;i<s.fast_keys.size();++i)if(s.fast_addresses[i]==address && s.fast_keys[i].local)s.DropFast(i);
  if(const auto *id=s.capture.identities.Find(world::ObjectDomain::Entity,address);id&&id->Retired()){
    s.source.Retire({1,id->local_id,id->use});const auto end=s.texts.begin()+s.text_used;
    const auto text=std::lower_bound(s.texts.begin(),end,address,[](const auto &entry,std::uint32_t p){return entry.address<p;});
    if(text!=end&&text->address==address){std::move(text+1,end,text);--s.text_used;}}
}
void BinaryEntities::EntityChanged(std::uint32_t address) noexcept {auto &s=*impl_;if(!s.Main())return;
  // 未进入既有闭包的新对象不撤销旧源；已发布对象的失效不能只删除fast body而留下旧ordinary。
  if(s.Known(address))Invalidate();RetireBody(address);}
void BinaryEntities::TextWritten(std::uint32_t address,std::uint64_t local,std::uint64_t use,std::uint64_t shape) noexcept {auto &s=*impl_;if(!s.Main())return;
  auto end=s.texts.begin()+s.text_used;auto where=std::lower_bound(s.texts.begin(),end,address,[](auto a,std::uint32_t p){return a.address<p;});
  if(where==end || where->address!=address){if(s.text_used==s.texts.size()){s.Failure({CaptureFailure::ValueBudget,{1,local,use,shape}});return;}std::move_backward(where,end,end+1);++s.text_used;}
  *where={address,local,use,shape};}
void BinaryEntities::ResourceChanged(std::uint32_t address) noexcept {auto &s=*impl_;if(!s.Main()||!s.source.UsesSprite(address))return;Invalidate();
  // 共享HUD Sprite可能供多个身体使用；加载／替换进入即撤销所有相关显示副本，历史P8与时钟不伪造重置。
  for(auto &hud:s.hud_states)if(hud.subject.local){s.production.RetirePurpose(hud.subject,315);s.production.RetirePurpose(hud.subject,321);hud.complete=hud.history=false;}
  for(const auto purpose:{323,325,373,374,375,376,377,378,379,380}){
    s.production.RetirePurpose({3,s.run,1,1},std::uint16_t(purpose));s.production.RetirePurpose({7,s.room,1,1},std::uint16_t(purpose));}}
// Init进入即退休旧floor候选；多层嵌套合并一个episode，异常不恢复读取资格。
void BinaryEntities::FloorBoundary(bool entering,bool normal) noexcept {auto &s=*impl_;if(!s.Main())return;
  if(entering){if(!s.floor_depth){Invalidate();s.floor_ready=true;if(s.next_floor>=UINT64_MAX/3-1)s.floor_ready=false;
      else {s.floor=++s.next_floor;s.descriptor_shapes={};s.descriptor_revisions={};}}
    if(s.floor_depth==UINT32_MAX){s.floor_ready=false;return;}++s.floor_depth;
  }else {if(!s.floor_depth){s.floor_ready=false;return;}s.floor_ready=s.floor_ready&&normal;--s.floor_depth;}}
void BinaryEntities::GridChanged(std::uint32_t address) noexcept {auto &s=*impl_;if(!s.Main() || !address)return;
  if(std::find(s.scene.grids.begin(),s.scene.grids.end(),address)!=s.scene.grids.end())Invalidate();
  if(const auto *id=s.capture.identities.Find(world::ObjectDomain::Grid,address);id&&id->Retired())s.source.Retire({2,id->local_id,id->use});}
BinaryEntityStats BinaryEntities::Statistics() const noexcept {auto out=impl_->stats;out.exchange=impl_->exchange.Statistics();return out;}
// 先建立真实生产窗口与有界调用栈；暂停显示允许tick=0，不能借用上一次Authority tick。
std::uint32_t BinaryEntities::ProductionBegin(BinaryProductionKind kind,std::uint32_t object,std::uint32_t manager) noexcept {
  auto &s=*impl_;if(!s.Main() || !s.stats.active || !s.capture.collector || !object)return 0;
  if(s.production_depth==s.producing.size() || s.next_production_token==UINT32_MAX){s.Failure({CaptureFailure::TraversalBudget});return 0;}
  Impl::Produced p;p.kind=kind;p.object=object;p.manager=manager;p.token=++s.next_production_token;
  p.parent=s.production_depth?s.producing[s.production_depth-1].token:UINT32_MAX;
  if(!s.capture.collector->NativeProduction(p.window,p.token))return 0;
  if((kind==BinaryProductionKind::Screen && object!=p.window.game) || (kind==BinaryProductionKind::RoomRender && object!=p.window.room_address))return 0;
  if(kind>=BinaryProductionKind::SfxPlay && kind<=BinaryProductionKind::SfxAmbient &&
     (!manager||std::uint64_t(manager)+0x2a160!=object))return 0;
  p.witness=s.Witness(p.window,CapturePhase::Production);if(!ValidCaptureWitness(p.witness))return 0;
  if(kind==BinaryProductionKind::PlayerHud){
    s.ReadProduction(p);if(!s.NativeValue(object,p.body)||!p.body||!s.Groups()){s.collecting=false;return 0;}
    auto facts=s.ProductionBody(p.body,p);s.collecting=false;
    if(!facts.ready || !facts.group_known || facts.ordinal>=4 || !facts.subject.binding){s.Failure({CaptureFailure::MissingQualification,facts.subject,315});return 0;}
    unsigned index=unsigned(s.hud_states.size()),free=index;
    for(unsigned i=0;i<s.hud_states.size();++i){auto &hud=s.hud_states[i];if(!hud.subject.local){if(free==s.hud_states.size())free=i;continue;}
      if(hud.subject.space==facts.subject.space&&hud.subject.local==facts.subject.local&&hud.subject.use==facts.subject.use){index=i;break;}}
    if(index==s.hud_states.size())index=free;if(index==s.hud_states.size()){s.Failure({CaptureFailure::RecordBudget,facts.subject,315});return 0;}
    auto &hud=s.hud_states[index];
    if(hud.subject!=facts.subject || hud.scope!=p.witness.scope || hud.address!=object){
      if(s.next_presentation_instance==UINT64_MAX){s.Failure({CaptureFailure::SequenceExhausted,facts.subject,315});return 0;}
      if(hud.subject.local)s.production.Retire(hud.subject);hud.tracker->Retire();hud.subject=facts.subject;hud.scope=p.witness.scope;hud.address=object;
      hud.instance=++s.next_presentation_instance;hud.mantle=0;hud.clock={};hud.complete=hud.stats=hud.history=false;hud.rows={};hud.rows_initialized=false;
    }
    hud.body=p.body;p.hud_index=index;hud.complete=false;
  }
  s.producing[s.production_depth++]=p;return p.token;
}
void BinaryEntities::ProductionDrop(std::uint32_t token,std::uint32_t body,bool held) noexcept {auto &s=*impl_;if(!s.Main())return;
  for(unsigned i=s.production_depth;i;--i){auto &p=s.producing[i-1];if(p.token!=token)continue;
    if(p.kind!=BinaryProductionKind::PlayerHud || p.body!=body || p.drop_reads==UINT32_MAX){s.Failure({CaptureFailure::SourceChanged,{},435});return;}
    ++p.drop_reads;p.held=held;return;}}
// 90A490真实查询已经发生后，保留那一次完整有序P8；冻结分支不重采，也不发护盾bool。
void BinaryEntities::ProductionMantle(std::uint32_t token,std::uint32_t effects) noexcept {auto &s=*impl_;if(!s.Main())return;
  for(unsigned i=s.production_depth;i;--i){auto &p=s.producing[i-1];if(p.token!=token)continue;
    if(p.kind!=BinaryProductionKind::PlayerHud || p.hud_index>=s.hud_states.size() || std::uint64_t(p.body)+0x1500!=effects){s.Failure({CaptureFailure::SourceChanged,{},436});return;}
    auto &hud=s.hud_states[p.hud_index];auto facts=s.ProductionBody(p.body,p);facts.address=effects;
    auto historical=p;if(s.revision==UINT64_MAX){s.Failure({CaptureFailure::SequenceExhausted,facts.subject,436});return;}
    historical.witness.revision=++s.revision;
    if(s.SaveProduction(historical,facts,436,hud.instance))hud.mantle=s.production.Revision(facts.subject,436,hud.instance);else hud.mantle=0;return;}}
void BinaryEntities::ProductionProgress(std::uint32_t token,bool seed,bool blocked) noexcept {auto &s=*impl_;if(!s.Main())return;
  for(unsigned i=s.production_depth;i;--i){auto &p=s.producing[i-1];if(p.token!=token)continue;
    if(seed){p.seed_seen=true;p.seed_blocked=blocked;}else {p.manager_seen=true;p.manager_blocked=blocked;}return;}}
void BinaryEntities::ProductionAudio(std::uint32_t token,int sound,float volume,float pitch,float pan,bool loop,bool world) noexcept {auto &s=*impl_;if(!s.Main())return;
  for(unsigned i=s.production_depth;i;--i){auto &p=s.producing[i-1];if(p.token!=token)continue;
    p.sound=sound;p.volume=volume;p.pitch=pitch;p.pan=pan;p.loop=loop;p.audio_world=world;
    if(p.kind==BinaryProductionKind::SfxPlay)for(unsigned parent=i-1;parent;--parent){const auto &outer=s.producing[parent-1];
      if(outer.kind==BinaryProductionKind::SfxAmbient){p.ambient=outer.sound;p.audio_world=outer.audio_world;break;}}
    return;}}
void BinaryEntities::ProductionStat(std::uint32_t token,int index,std::uint32_t flags) noexcept {auto &s=*impl_;if(!s.Main())return;
  for(unsigned i=s.production_depth;i;--i){auto &p=s.producing[i-1];if(p.token!=token)continue;
    if(p.kind==BinaryProductionKind::StatRecompute){p.stat_index=index;p.stat_flags=flags;}return;}}
// 只观察原链真实暂停查询；Manager相位在同一个返回点复制，不另行调用IsPaused。
void BinaryEntities::ProductionPaused(std::uint32_t token,std::uint32_t game,bool paused) noexcept {auto &s=*impl_;if(!s.Main())return;
  for(unsigned i=s.production_depth;i;--i){auto &p=s.producing[i-1];if(p.token!=token)continue;if(game!=p.window.game)return;
    s.ReadProduction(p);std::uint32_t frame=0;if(!s.NativeValue(std::uint64_t(p.manager)+0x4a8b8,frame)){p.draw_conflict=true;s.collecting=false;return;}
    if(p.draw_known&&(p.draw_clock.manager_frame!=frame||p.draw_clock.paused!=paused))p.draw_conflict=true;
    p.draw_clock={frame,paused};p.draw_known=true;s.collecting=false;return;}}
// 9F7970是正常选样本后的共同尾叶：即使本地source为空也会到达，不能以Play后端成功分配世界实例。
void BinaryEntities::ProductionAudioPan(std::uint32_t token,std::uint32_t stream) noexcept {auto &s=*impl_;if(!s.Main())return;
  for(unsigned i=s.production_depth;i;--i){auto &p=s.producing[i-1];if(p.token!=token)continue;if(p.kind!=BinaryProductionKind::SfxPlay)return;
    s.ReadProduction(p);std::array<std::uint32_t,3> table{};std::uint32_t samples=0;
    bool valid=p.sound>=0 && stream>=24 && s.NativeValue(p.object,table) && table[0]<=table[1] && table[1]<=table[2] &&
      (table[1]-table[0])%440==0 && std::uint32_t(p.sound)<(table[1]-table[0])/440;
    const auto effect=std::uint64_t(table[0])+std::uint64_t(std::uint32_t(p.sound))*440;
    valid=valid && s.NativeValue(effect+400,samples) && samples>0 && samples<=7 && stream>=effect+32 &&
      (stream-effect-32)%56==0 && (stream-effect-32)/56<samples;
    if(valid){p.stream=stream;++p.selected;}else {s.audio_gap|=p.audio_world;s.Failure({CaptureFailure::SourceChanged,{},381});}
    s.collecting=false;return;}}
// 此正常查询紧接8045EE写BE7704；在Room Render尾部清零和任何后续pass之前复制实际临时值。
void BinaryEntities::ProductionRoomEffect(std::uint32_t token,std::uint32_t effects,bool enabled) noexcept {auto &s=*impl_;if(!s.Main())return;
  for(unsigned i=s.production_depth;i;--i){auto &p=s.producing[i-1];if(p.token!=token)continue;
    if(p.kind!=BinaryProductionKind::RoomRender || p.object!=p.window.room_address || std::uint64_t(p.object)+0x7310!=effects)return;
    s.ReadProduction(p);if(s.NativeValue(std::uint64_t(s.capture.source_memory.image_base)+0x7e7704,p.retro)&&std::isfinite(p.retro)){
      p.room_effect_known=true;p.glitch=enabled;
    }else s.Failure({CaptureFailure::ReadFailed,{},369});s.collecting=false;return;}}
// 正常返回才封存；所有失败阻断新完整显示目标，不改变Authority输入或原生调用次数。
void BinaryEntities::ProductionEnd(std::uint32_t token,bool normal) noexcept {
  auto &s=*impl_;if(!s.Main() || !s.production_depth)return;auto p=s.producing[s.production_depth-1];
  if(p.token!=token){s.Failure({CaptureFailure::Reentrant});s.ClearProduction();s.production_depth=0;return;}
  --s.production_depth;if(!normal || !s.capture.collector->NativeWindowCurrent(p.window)){
    if(p.hud_index<s.hud_states.size()){auto &hud=s.hud_states[p.hud_index];s.production.Retire(hud.subject);hud.complete=false;hud.mantle=0;hud.tracker->Retire();}
    if(p.kind==BinaryProductionKind::Score)s.score.revision=0;
    if(p.kind==BinaryProductionKind::StatTick)for(auto &hud:s.hud_states)if(hud.subject.local){s.production.RetirePurpose(hud.subject,317);hud.stats=false;}
    if(p.audio_world)s.audio_gap=true;
    if(p.kind==BinaryProductionKind::Screen)for(const auto purpose:{367,368,370})s.production.RetirePurpose({3,p.window.run,1,1},std::uint16_t(purpose));
    if(p.kind==BinaryProductionKind::RoomRender)for(const auto purpose:{369,371})s.production.RetirePurpose({7,p.window.room,1,1},std::uint16_t(purpose));
    s.Failure({CaptureFailure::SourceChanged});return;}
  if(s.revision==UINT64_MAX){s.Failure({CaptureFailure::SequenceExhausted});return;}p.witness.revision=++s.revision;
  s.ReadProduction(p);
  if(p.kind==BinaryProductionKind::Score){
    std::array<std::uint32_t,15> values{};std::uint32_t time=0;
    if(p.object==std::uint64_t(p.window.game)+0x25c5c && s.NativeValue(p.object,values) && s.NativeValue(std::uint64_t(p.object)+0x4c,time)){
      s.score.scope=p.witness.scope;std::copy(values.begin(),values.end(),s.score.values.begin());s.score.values[15]=time;s.score.revision=p.witness.revision;
    }else s.Failure({CaptureFailure::MissingQualification,{},234});
  }else if(p.kind==BinaryProductionKind::HudRender){
    std::uint8_t custom=0,rerun=0;std::uint32_t challenge=0;
    if(s.NativeValue(std::uint64_t(p.window.game)+0x1bb84,custom) && s.NativeValue(std::uint64_t(p.window.game)+0x25b00,rerun) &&
       s.NativeValue(std::uint64_t(p.window.game)+0x25afc,challenge) && (custom || (p.seed_seen&&(p.seed_blocked||p.manager_seen)))){
      s.progress_blocked=custom||p.seed_blocked||p.manager_blocked||rerun||challenge;s.progress_scope=p.witness.scope;s.progress_revision=p.witness.revision;
    }
  }else if(p.kind==BinaryProductionKind::PlayerHud){
    auto &hud=s.hud_states[p.hud_index];auto facts=s.ProductionBody(p.body,p);std::array<std::int32_t,4> health{};float scale=0;
    std::uint32_t actual_body=0,hud_owner=0;
    bool valid=p.drop_reads==1 && facts.subject==hud.subject && s.NativeValue(p.object,actual_body) && actual_body==p.body &&
      s.NativeValue(std::uint64_t(p.object)+4,hud_owner) && hud_owner==p.window.game+0x1d9ac && hud.mantle &&
      s.NativeValue(std::uint64_t(p.body)+0x1344,health[0]) && s.NativeValue(std::uint64_t(p.body)+0x1348,health[1]) &&
      s.NativeValue(std::uint64_t(p.body)+0x134c,health[2]) && s.NativeValue(std::uint64_t(p.body)+0x1d74,health[3]) &&
      s.NativeValue(std::uint64_t(p.object)+1724,scale) && std::isfinite(scale) && scale>=0 && scale<=1 && float(unsigned(scale*4))*0.25f==scale;
    if(valid){HudNormalUpdate value;value.identity.scope=p.witness.scope;
      value.identity.body={1,197,facts.subject.local,facts.subject.use,facts.subject.shape};
      const auto &g=s.groups[facts.ordinal];value.identity.group={10,196,g.group,1,0};
      value.identity.binding=facts.binding_revision;value.identity.instance=hud.instance;value.identity.generation=s.capture.epoch;value.identity.topology=std::uint32_t(facts.subject.topology);
      value.source_revision=p.witness.revision;value.mantle_revision=hud.mantle;value.game_frame=p.witness.game_frame;
      value.critical_health=HudCriticalHealth(health[0],health[1],health[2],health[3]);value.drop_held=p.held;value.completed_drop_phase=std::uint8_t(scale*4);
      try{hud.clock=hud.tracker->Observe(value);}catch(...){valid=false;s.Failure({CaptureFailure::InvalidValue,facts.subject,435});}
      if(valid){facts.address=facts.hud=p.object;facts.hud_resources=hud_owner;facts.hud_clock=&hud.clock;
        valid=s.SaveProduction(p,facts,315,hud.instance);if(valid)valid=s.SaveProduction(p,facts,435,hud.instance);}
    }
    hud.complete=valid;if(!valid)s.Failure({CaptureFailure::MissingQualification,hud.subject,435});
    // 获得史按此身体自己的已选身份生产；不受History原生两槽、Host展开偏好或当前视口限制。
    facts.address=p.body;facts.history_from_player=true;hud.history=s.SaveProduction(p,facts,321,hud.instance);
  }else if(p.kind==BinaryProductionKind::RoomRender && p.room_effect_known){
    EntityFacts facts;facts.subject={7,p.window.room,1,1};facts.address=p.object;facts.ready=true;
    world::binary::NativeScreenValue value;value.amount=p.retro;value.present=p.retro>0;value.enabled=p.glitch;facts.screen=&value;
    if(s.SaveProduction(p,facts,369,p.window.room))s.SaveProduction(p,facts,371,p.window.room);
  }else if(p.kind==BinaryProductionKind::RoomRender){
    s.production.RetirePurpose({7,p.window.room,1,1},369);s.production.RetirePurpose({7,p.window.room,1,1},371);s.Failure({CaptureFailure::MissingQualification,{},369});
  }else if(p.kind==BinaryProductionKind::Screen){
    EntityFacts facts;facts.subject={3,p.window.run,1,1};facts.address=p.window.game;facts.ready=true;
    if(p.draw_known&&!p.draw_conflict){facts.draw_clock=&p.draw_clock;s.SaveProduction(p,facts,356,p.window.run);}
    else {s.production.RetirePurpose(facts.subject,356);s.Failure({CaptureFailure::MissingQualification,facts.subject,356});}
    world::binary::NativeScreenValue value;facts.screen=&value;
    // 41CC80只消费当前BossOverlay；不调用41CB40进度／音乐／切房生产链。
    s.ReadProduction(p);std::uint32_t overlay_active=0;
    if(s.NativeValue(std::uint64_t(p.window.game)+0x1d294,overlay_active)){
      value={};value.present=overlay_active!=0;auto overlay=facts;overlay.address=p.window.game+0x1d294;
      s.SaveProduction(p,overlay,379,p.window.run);
    }else {s.production.RetirePurpose(facts.subject,379);s.Failure({CaptureFailure::ReadFailed,facts.subject,379});}
    const auto save=[&](std::uint16_t layout){if(!std::isfinite(value.amount)){s.production.RetirePurpose(facts.subject,layout);s.Failure({CaptureFailure::InvalidValue,facts.subject,layout});return false;}
      return s.SaveProduction(p,facts,layout,p.window.run);};
    std::int32_t remaining=0,total=0;float strength=0;
    s.ReadProduction(p);
    if(s.NativeValue(std::uint64_t(p.window.game)+0x25aa0,remaining)){
      value={};if(remaining>0){value.present=true;
        if(!s.NativeValue(std::uint64_t(p.window.game)+0x25aa4,total) || total<=0 || !s.NativeValue(std::uint64_t(p.window.game)+0x25aa8,strength))value.amount=std::numeric_limits<float>::quiet_NaN();
        else {const float half=float(float(total)*0.5f),distance=float(std::fabs(float(float(remaining)-half))),ratio=float(distance/half);
          const float angle=float(float(float(1.0f-ratio)*3.1415927410125732f)*0.5f);value.amount=float(std::sin(angle)*strength);}}
      save(367);
    }else {s.production.RetirePurpose(facts.subject,367);s.Failure({CaptureFailure::ReadFailed,facts.subject,367});}
    s.ReadProduction(p);
    if(s.NativeValue(std::uint64_t(p.window.game)+0x25ab0,remaining)){
      value={};if(remaining>0){value.present=true;
        if(!s.NativeValue(std::uint64_t(p.window.game)+0x25ab4,total) || total<=0)value.amount=std::numeric_limits<float>::quiet_NaN();
        else if(remaining>std::int64_t(total)-10)value.amount=float(float(std::int64_t(total)-remaining)/10.0f);
        else value.amount=remaining<10?float(float(remaining)/10.0f):1.0f;}
      save(370);
    }else {s.production.RetirePurpose(facts.subject,370);s.Failure({CaptureFailure::ReadFailed,facts.subject,370});}
    s.ReadProduction(p);std::array<std::uint32_t,5> stage{};std::array<std::uint32_t,3> room_transition{};std::array<std::int32_t,2> level{};std::uint32_t style=0,upper_flags=0;
    bool inputs=s.NativeValue(std::uint64_t(p.window.game)+0x1ba78,stage[0])&&s.NativeValue(std::uint64_t(p.window.game)+0x1b83c,room_transition[0]);
    if(inputs&&(stage[0]==2||stage[0]==3))inputs=s.NativeValue(std::uint64_t(p.window.game)+0x1ba7c,stage[1])&&
      s.NativeValue(std::uint64_t(p.window.game)+0x1ba80,stage[2])&&s.NativeValue(std::uint64_t(p.window.game)+0x1ba88,stage[4]);
    if(inputs&&(room_transition[0]==2||room_transition[0]==3))inputs=s.NativeValue(std::uint64_t(p.window.game)+0x1b840,room_transition[1])&&
      s.NativeValue(std::uint64_t(p.window.game)+0x1b844,room_transition[2])&&s.NativeValue(std::uint64_t(p.window.game)+0x1b858,style);
    if(inputs && s.NativeValue(p.window.game,level) && s.NativeValue(std::uint64_t(p.window.game)+0x25ac4,upper_flags)){
      const auto progress=[](const auto &v){if(v[0]!=2&&v[0]!=3)return -1.0f;if(!v[2])return std::numeric_limits<float>::quiet_NaN();
        const auto amount=float(float(v[1])/float(v[2]));return v[0]==3?float(1.0f-amount):amount;};
      const auto stage_progress=progress(stage),room_progress=progress(room_transition);
      const auto adjusted_stage=std::int64_t(level[0])+((level[1]==4||level[1]==5)?1:0);
      const bool stage_enabled=stage[4]!=5&&(level[0]<1||level[0]>6||!(upper_flags&0x10000)||(stage[0]==2&&adjusted_stage==1));
      value={};value.present=(stage_progress>=0&&stage_enabled)||(room_progress>=0&&style==2);
      if(!std::isfinite(stage_progress)||!std::isfinite(room_progress))value.amount=std::numeric_limits<float>::quiet_NaN();
      else if(value.present)value.amount=std::max(float(std::max(stage_progress,room_progress)*0.30000001192092896f),0.00009999999747378752f);
      save(368);
    }else {s.production.RetirePurpose(facts.subject,368);s.Failure({CaptureFailure::ReadFailed,facts.subject,368});}
  }else if(p.kind==BinaryProductionKind::SfxPlay && p.audio_world && p.selected){
    s.ReadProduction(p);float modifier=0,pan=0;std::uint8_t loop=0;CaptureLink asset;CaptureDiagnostic d;
    EntityFacts facts;facts.subject={3,p.window.run,1,1};facts.address=p.object;facts.ready=true;
    bool valid=p.selected==1 && std::isfinite(p.volume) && std::isfinite(p.pitch) && std::isfinite(p.pan) &&
      s.NativeValue(std::uint64_t(p.object)+16,modifier) && std::isfinite(modifier) &&
      s.NativeValue(std::uint64_t(p.stream)+8,loop) && loop<=1 && loop==p.loop &&
      s.NativeValue(std::uint64_t(p.stream)+16,pan) && std::isfinite(pan) && pan==p.pan;
    // 非零pan尚须精确资源通道与实际生效来源，不能将非mono的未生效请求当成最终结果。
    if(p.pan!=0){valid=false;d={CaptureFailure::MissingQualification,facts.subject,381,4};}
    if(valid)valid=s.source.AudioResource(p.witness,facts,p.stream-24,asset,d);
    const auto gain=float(std::max(modifier,0.0f)*p.volume);valid=valid&&std::isfinite(gain);
    unsigned ambient=unsigned(s.voices.size());
    if(p.ambient>=0)for(unsigned i=0;i<s.voices.size();++i)if(s.voices[i].active&&s.voices[i].ambient==p.ambient&&s.voices[i].production.scope.run==p.witness.scope.run){ambient=i;break;}
    if(valid&&ambient<s.voices.size()){
      // 后端重试保持已有逻辑实例；未知改号／资源替换不能伪装成一次新的world Start。
      auto &voice=s.voices[ambient];if(voice.value.asset.kind!=asset.kind||voice.value.asset.handle!=asset.handle){s.audio_gap=true;s.Failure({CaptureFailure::SourceChanged,facts.subject,381});}
      else {const bool changed=voice.value.gain!=gain||voice.value.pitch!=p.pitch||voice.value.pan!=pan;
        voice.stream=p.stream;voice.sound=std::uint32_t(p.sound);voice.value.gain=gain;voice.value.pitch=p.pitch;voice.value.pan=pan;
        if(changed)s.AudioEventFor(voice,p,4);}
      s.collecting=false;return;
    }
    unsigned available=unsigned(s.voices.size()),previous=available;
    for(unsigned i=0;i<s.voices.size();++i){if(s.voices[i].active&&s.voices[i].stream==p.stream)previous=i;else if(!s.voices[i].active&&available==s.voices.size())available=i;}
    if(previous<s.voices.size())available=previous;
    const auto events=previous<s.voices.size()?2u:1u;
    valid=valid && available<s.voices.size() && events<=s.audio_events.size()-s.audio_event_count &&
      s.next_audio_voice!=UINT64_MAX && s.next_audio_event<=UINT64_MAX-events;
    if(valid){
      if(previous<s.voices.size()){auto &old=s.voices[previous];auto &event=s.audio_events[s.audio_event_count++];
        event={old.value,p.witness,old.instance,old.generation,++s.next_audio_event};event.value.kind=2;old.active=false;}
      auto &voice=s.voices[available];voice={};voice.value.asset=asset;voice.value.gain=gain;voice.value.pitch=p.pitch;voice.value.pan=pan;
      voice.value.loop=p.loop;voice.value.start_frame=p.witness.game_frame;voice.production=p.witness;voice.instance=++s.next_audio_voice;voice.generation=s.capture.epoch;
      voice.stream=p.stream;voice.sound=std::uint32_t(p.sound);voice.ambient=p.ambient;voice.active=true;
      s.audio_events[s.audio_event_count++]={voice.value,p.witness,voice.instance,voice.generation,++s.next_audio_event};
    }else {s.audio_gap=true;s.Failure(d.reason==CaptureFailure::None?CaptureDiagnostic{CaptureFailure::ValueBudget,facts.subject,383}:d);}
  }else if(p.kind>=BinaryProductionKind::SfxVolume && p.kind<=BinaryProductionKind::SfxAmbient){s.AudioControl(p);
  }else if(p.kind==BinaryProductionKind::StatRecompute && p.stat_index>=0 && p.stat_index<2){
    // 只在本次flags证明真实写过时采最终value，绝不把未更新行／构造值当成正常生产。
    const auto address=std::uint64_t(p.object)+280+std::uint64_t(p.stat_index)*204;std::uint32_t actual_body=0;
    if(!s.NativeValue(address-4,actual_body)||!actual_body){s.collecting=false;return;}
    const auto facts=s.ProductionBody(actual_body,p);if(!facts.ready){s.collecting=false;return;}
    if(s.probability.scope!=p.witness.scope){s.probability={};s.probability.scope=p.witness.scope;}
    for(unsigned row=0;row<3;++row){if(!(p.stat_flags&(row<2?0xc0u:0x100u)))continue;float value=0;
      if(s.NativeValue(address+std::uint64_t(row+6)*20,value)&&std::isfinite(value)){
        s.probability.values[row]=value;s.probability.frames[row]=p.witness.game_frame;s.probability.mask|=std::uint8_t(1u<<row);
      }else {s.probability.mask&=std::uint8_t(~(1u<<row));s.Failure({CaptureFailure::ReadFailed,{},317});}}
  }else if(p.kind==BinaryProductionKind::StatTick){s.ProduceOwnedStats(p);
  }
  s.collecting=false;
}
void BinaryEntities::SceneBuild(std::uint32_t owner,bool entering,bool normal,bool replace) noexcept {
  if(!impl_->Main())return;if(entering)Invalidate();impl_->source.SceneBuild(owner,entering,normal,replace);}
void BinaryEntities::SceneGraphicsBoundary() noexcept {impl_->source.SceneGraphicsBoundary();}
void BinaryEntities::SceneClear() noexcept {impl_->source.SceneClear();}
void BinaryEntities::SceneImageBefore(std::uint32_t image,std::uint32_t source,std::uint32_t destination,const std::array<std::uint32_t,4> &colors) noexcept {
  impl_->source.SceneImageBefore(image,source,destination,colors);}
void BinaryEntities::SceneImageAfter(std::uint32_t vertices) noexcept {impl_->source.SceneImageAfter(vertices);}
CaptureExchange::ReadTicket BinaryEntities::Take() noexcept {return impl_->exchange.Take();}
BinaryEntityCatalog BinaryEntities::Catalog(const CaptureExchange::ReadTicket &ticket) const noexcept {const auto &s=*impl_;
  if(std::this_thread::get_id()!=s.consumer)return {};const auto *packet=ticket.Packet();if(!packet)return {};
  return {packet->references,s.source.PublishedResources(packet->catalog)};}
// 当前步骤只让工作线程排控制；数据继续保留cold／latest，不冒充第11步编码／发送已接通。
void BinaryEntities::Background(bool stopping) noexcept {auto &s=*impl_;CaptureControl control;while(s.exchange.PopControl(control)){}
  if(stopping)for(;;){auto ticket=s.exchange.Take();if(!ticket.Packet())break;ticket.Reset();}}
}
