#include "game/observation/binary_entities.h"
#include "game/observation/world_bridge.h"
#include "game/observation/binary_display_capture.h"
#include "game/authority/state.h"
#include "game/authority/tick.h"
#include "game/common/bridge.h"
#include "world/observe.h"
#include "state/budget.h"
#include "world/binary/resource_export.h"
#include <mutex>
#include <algorithm>
#include <bit>
#include <tuple>
#include <cmath>
#include <limits>
#include <windows.h>
#include <atomic>

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
  BinaryDisplayCapture display;
  // 以下状态仅由consumer访问；outbox是与主线程之间的自有值邮箱。
  HostStateCapture publisher;
  CaptureExchange::ReadTicket transferring;
  HostStateSnapshotRef transfer_source;
  std::mutex outbox;
  HostStateSnapshotRef published,ordinary_published;
  std::vector<HostStateResource> published_resources;
  std::uint64_t resource_session=0,resource_run=0;
  std::array<char,128> publish_failure{};
  std::atomic<std::uint32_t> network_publication{0};
  std::uint64_t publication_floor=0; // 仅producer访问，撤销前的邮箱值不能在新提交后复活。
  // consumer写、主线程读的SPSC回执；64位数据置于release/acquire保护的槽中，不用64位原子。
  struct EventReceipt {std::uint64_t session=0,run=0,watermark=0;};
  std::array<EventReceipt,32> event_receipts{};
  static_assert(std::atomic<std::uint32_t>::is_always_lock_free,"event receipts require lock-free Win32 u32 atomics");
  std::atomic<std::uint32_t> receipt_write{0},receipt_read{0};
  struct Produced {
    world::NativeCaptureWindow window;
    CaptureWitness witness;
    BinaryProductionKind kind=BinaryProductionKind::Score;
    std::uint32_t token=0,object=0,manager=0;
    std::int32_t sound=-1;
    float volume=0,pitch=1,pan=0;
    std::uint32_t stream=0,selected=0;
    bool loop=false,audio_world=false;
    std::int32_t ambient=-1;
    std::array<std::uint32_t,8> args{};
    std::array<char,4096> title{},subtitle{};
    bool title_seen=false,subtitle_seen=false,display_failed=false;
    std::int32_t frame_delay=0;
  };
  std::array<Produced,16> producing{};
  std::uint32_t production_depth=0,next_production_token=0;
  struct ScoreCache {StateScope scope;std::array<std::uint32_t,16> values{};std::uint64_t revision=0;};
  ScoreCache score;
  std::vector<EntityFacts> roots,ordinary_facts;
  std::vector<std::uint32_t> root_index,link_index,published_addresses;
  std::vector<CaptureReferenceEntry> links;
  std::vector<TextWitness> texts;
  std::vector<std::uint32_t> members,member_check;
  std::array<world::RawSource,10> member_sources{};
  std::array<GroupFacts,4> groups{};
  NativeScene scene;
  std::array<EntityFacts,986> scene_roots{};
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
  std::uint64_t display_run=0;
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
    display(p) {
    roots.resize(16384);ordinary_facts.resize(16384);root_index.resize(16384);links.resize(16392);link_index.resize(16392);texts.resize(16384);
    published_addresses.resize(16384);
    members.resize(world::kMembershipBudget);member_check.resize(world::kMembershipBudget);
    // 全部自有暂存与交接合计占用同一64MiB shared额度，不给每个owner重复领取。
    if(memory->Bytes()+source.RetainedBytes()+exchange.RetainedBytes()+display.RetainedBytes()>StateBudgets().shared_value_bytes)
      throw std::invalid_argument("binary_entity_shared_capacity");
  }
  bool Main() const noexcept {return std::this_thread::get_id()==producer;}
  void DrainEventReceipts() noexcept {
    const auto end=receipt_write.load(std::memory_order_acquire);auto at=receipt_read.load(std::memory_order_relaxed);
    for(;at!=end;++at){const auto &receipt=event_receipts[at%event_receipts.size()];
      if(capture.collector&&receipt.session==capture.session_key&&receipt.run==capture.collector->CurrentRunToken())
        display.EventsTransferred(receipt.watermark);}
    receipt_read.store(at,std::memory_order_release);
  }
  void DisplayRun() noexcept {if(!capture.collector)return;const auto current=capture.collector->CurrentRunToken();
    if(!current)return;if(display_run&&display_run!=current)display.Reset(true);display_run=current;}
  BinaryDisplayRead DisplayRead() noexcept {return {this,
    [](void *p,std::uint64_t address,std::span<std::byte> out) noexcept {auto &s=*static_cast<Impl *>(p);return s.Main()&&s.collecting&&s.capture.collector&&s.capture.collector->NativeWindowCurrent(s.reading_window)&&
      address&&address<=UINT32_MAX&&out.size()<=(std::uint64_t{1}<<32)-address&&s.capture.source_memory.copy(std::uint32_t(address),out.data(),out.size())&&s.capture.collector->NativeWindowCurrent(s.reading_window);},
    [](void *p,std::uint32_t address) noexcept {return static_cast<Impl *>(p)->Reference(address);},
    [](void *p) noexcept {auto &s=*static_cast<Impl *>(p);return std::span<const CaptureReferenceEntry>(s.links).first(s.link_used);},
    [](void *p,const CaptureReferenceEntry &ref) noexcept {return static_cast<Impl *>(p)->Register({ref.space,ref.local,ref.use});}};}
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
    else {
      if(!reading_window.ordinary||!floor_ready||floor_depth||w.scope.floor!=floor||f.scene!=&scene)return false;
      bool mapped=false;
      if(f.layout==239)mapped=f.address==scene.room;
      else if(f.layout==437)mapped=f.manager&&std::uint64_t(f.manager)+0x2a170==f.address;
      else if(f.layout==276||f.layout==237||f.layout==382||f.layout==383)mapped=f.address==scene.game;
      else if(f.layout==270)mapped=f.source_slot<scene.descriptors.size()&&scene.descriptors[f.source_slot]==f.address&&
        f.subject.use==floor&&f.subject.shape==descriptor_revisions[f.source_slot];
      // 448—453只接纳锚定同一Game／run的自有事件树，不扩大任意原生对象可读范围。
      else if(f.layout>=448&&f.layout<=453)mapped=f.ready&&f.address==scene.game&&
        f.subject==CaptureSubject{3,w.scope.run,1,1}&&!f.display_values.empty()&&f.display_values.front().layout==f.layout;
      if(!mapped)return false;
    }
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
  bool ProductionInto(CaptureValueWriter &,std::uint32_t &,CaptureDiagnostic &) noexcept;
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
    std::array<std::uint32_t,16> current_score{};std::array<std::uint32_t,15> components{};
    context.score_ready=score.revision && score.scope==reading_witness.scope &&
      NativeValue(std::uint64_t(scene.game)+0x25c5c,components) && NativeValue(std::uint64_t(scene.game)+0x25ca8,current_score[15]);
    std::copy(components.begin(),components.end(),current_score.begin());context.score_ready=context.score_ready&&current_score==score.values;
    BinaryDisplayInputs inputs;
    if(!NativeValue(std::uint64_t(capture.source_memory.image_base)+0x7e4a1c,context.manager) || !context.manager ||
       !source.ReadDisplayContext(reading_witness,context,diagnostic) || !QueryBinaryDisplayInputs(reinterpret_cast<void *>(context.address),reinterpret_cast<void *>(scene.room),inputs)||
       !display.Prepare(reading_witness,context,source,DisplayRead(),inputs,diagnostic)||!push(context))return false;
    context.layout=437;context.address=context.manager+0x2a170;context.subject={12,reading_window.run,1,1};
    return context.manager<=UINT32_MAX-0x2a174 && push(context);
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
bool BinaryEntities::Impl::ProductionInto(CaptureValueWriter &writer,std::uint32_t &count,CaptureDiagnostic &diagnostic) noexcept {
  // Q14不再要求旧HUD／D0／显示结果生产缓存；普通值由收缩后的GameRun计划采集。
  return display.Events(reading_witness.scope,writer,count,diagnostic,{this,
    [](void *owner,const CaptureReferenceEntry &ref) noexcept {auto &s=*static_cast<Impl *>(owner);
      return s.Register({ref.space,ref.local,ref.use});}});
}
void BinaryEntities::Intention(const BinaryMusicIntent &intent) noexcept {auto &s=*impl_;if(!s.Main()||!s.stats.active)return;s.DisplayRun();s.display.BootstrapMusic(intent);}
void BinaryEntities::Intention(const BinarySfxIntent &intent) noexcept {auto &s=*impl_;if(!s.Main()||!s.stats.active)return;s.DisplayRun();s.display.BootstrapSfx(intent);}
void BinaryEntities::MapFocus(std::uint32_t owner,std::uint32_t descriptor,std::uint32_t frame,std::uint32_t selected) noexcept {
  auto &s=*impl_;if(!s.Main()||!s.stats.active)return;s.DisplayRun();s.display.MapFocus(owner,descriptor,frame,selected);
}
void BinaryEntities::OverlayShown(std::uint32_t owner,std::int32_t controller) noexcept {
  auto &s=*impl_;if(!s.Main()||!s.stats.active)return;s.DisplayRun();s.display.OverlayShown(owner,controller);
}
void BinaryEntities::BossShown(std::uint32_t owner,std::uint32_t first,std::uint32_t second,std::int32_t room,std::int32_t dimension) noexcept {
  auto &s=*impl_;if(!s.Main()||!s.stats.active)return;s.DisplayRun();s.display.BossShown(owner,first,second,room,dimension);
}
BinaryEntities::BinaryEntities(WorldCapture &c,std::thread::id p,std::thread::id worker):impl_(std::make_unique<Impl>(c,p,worker)) {}
BinaryEntities::~BinaryEntities()=default;
bool BinaryEntities::Active() const noexcept {return impl_->stats.active;}
void BinaryEntities::Active(bool active) noexcept {auto &s=*impl_;if(!s.Main() || active==s.stats.active)return;
  if(active&&(!StreamResourceHooksInstalled()||!BinaryDisplayCaptureHooksInstalled())){s.Failure({CaptureFailure::MissingQualification});return;}
  s.ordinary={};for(auto &body:s.bodies)body={};s.fast_keys={};s.fast_addresses={};s.last_ordinary=0;s.published_count=0;s.source.Invalidate();
  s.score={};s.production_depth=0;
  s.display.Reset(true);
  s.display_run=0;
  s.publication_floor=s.revision==UINT64_MAX?UINT64_MAX:s.revision+1;
  s.invalidated=false;if(active)s.stats.active=s.exchange.Begin();else {s.exchange.Stop();s.stats.active=false;}}
void BinaryEntities::FrameBegin() noexcept {auto &s=*impl_;if(!s.Main())return;s.ordinary={};s.ordinary_count=0;s.link_used=0;for(auto &body:s.bodies)body={};s.collecting=false;
  s.DrainEventReceipts();
  if(s.stats.active && s.capture.collector && ((s.run && s.run!=s.capture.collector->CurrentRunToken()) ||
      (s.room && s.room!=s.capture.collector->CurrentRoomToken()))){
    for(std::uint32_t i=0;i<s.scene_root_count;++i)s.source.Retire(s.scene_roots[i].subject);
    s.scene_root_count=0;s.score={};
    if(s.run && s.run!=s.capture.collector->CurrentRunToken()){if(s.display_run!=s.capture.collector->CurrentRunToken())s.display.Reset(true);}
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
  s.publication_floor=s.revision==UINT64_MAX?UINT64_MAX:s.revision+1;
  if(s.resource_mutation==UINT64_MAX){s.stats.active=false;s.exchange.Stop(CaptureFailure::SequenceExhausted);return;}++s.resource_mutation;s.source.Invalidate();
  for(auto &key:s.fast_keys)if(key.local){s.exchange.RetireBody(key);key={};}s.fast_addresses={};s.exchange.Invalidate(CaptureFailure::Revoked);}
void BinaryEntities::RetireBody(std::uint32_t address) noexcept {auto &s=*impl_;if(!s.Main())return;
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
}
// Init进入即退休旧floor候选；多层嵌套合并一个episode，异常不恢复读取资格。
void BinaryEntities::FloorBoundary(bool entering,bool normal) noexcept {auto &s=*impl_;if(!s.Main())return;
  if(entering){if(!s.floor_depth){Invalidate();s.floor_ready=true;if(s.next_floor>=UINT64_MAX/3-1)s.floor_ready=false;
      else {s.floor=++s.next_floor;s.descriptor_shapes={};s.descriptor_revisions={};}}
    if(s.floor_depth==UINT32_MAX){s.floor_ready=false;return;}++s.floor_depth;
  }else {if(!s.floor_depth){s.floor_ready=false;return;}s.floor_ready=s.floor_ready&&normal;--s.floor_depth;}}
void BinaryEntities::GridChanged(std::uint32_t address) noexcept {auto &s=*impl_;if(!s.Main() || !address)return;
  if(std::find(s.scene.grids.begin(),s.scene.grids.end(),address)!=s.scene.grids.end())Invalidate();
  if(const auto *id=s.capture.identities.Find(world::ObjectDomain::Grid,address);id&&id->Retired())s.source.Retire({2,id->local_id,id->use});}
BinaryEntityStats BinaryEntities::Statistics() const noexcept {auto &s=*impl_;auto out=s.stats;out.exchange=s.exchange.Statistics();out.events=s.display.Statistics();
  std::unique_lock lock(s.outbox,std::try_to_lock);if(lock){out.publication_revision=s.published?s.published->witness.revision:0;out.publication_failure=s.publish_failure;}return out;}
// 先建立真实生产窗口与有界调用栈；暂停显示允许tick=0，不能借用上一次Authority tick。
std::uint32_t BinaryEntities::ProductionBegin(BinaryProductionKind kind,std::uint32_t object,std::uint32_t manager) noexcept {
  auto &s=*impl_;if(!s.Main() || !s.stats.active || !s.capture.collector || !object)return 0;
  s.DisplayRun();
  if(s.production_depth==s.producing.size() || s.next_production_token==UINT32_MAX){s.Failure({CaptureFailure::TraversalBudget});return 0;}
  Impl::Produced p;p.kind=kind;p.object=object;p.manager=manager;p.token=++s.next_production_token;
  if(!s.capture.collector->NativeProduction(p.window,p.token))return 0;
  if(kind>=BinaryProductionKind::SfxPlay && kind<=BinaryProductionKind::SfxAmbient &&
     (!manager||std::uint64_t(manager)+0x2a160!=object))return 0;
  p.witness=s.Witness(p.window,CapturePhase::Production);if(!ValidCaptureWitness(p.witness))return 0;
  s.producing[s.production_depth++]=p;return p.token;
}
void BinaryEntities::ProductionAudio(std::uint32_t token,int sound,float volume,float pitch,float pan,bool loop,bool world) noexcept {auto &s=*impl_;if(!s.Main())return;
  for(unsigned i=s.production_depth;i;--i){auto &p=s.producing[i-1];if(p.token!=token)continue;
    p.sound=sound;p.volume=volume;p.pitch=pitch;p.pan=pan;p.loop=loop;p.audio_world=world;
    if(p.kind==BinaryProductionKind::SfxPlay)for(unsigned parent=i-1;parent;--parent){const auto &outer=s.producing[parent-1];
      if(outer.kind==BinaryProductionKind::SfxAmbient){p.ambient=outer.sound;p.audio_world=outer.audio_world;break;}}
    return;}}
void BinaryEntities::ProductionRequest(std::uint32_t token,const std::array<std::uint32_t,5> &args) noexcept {
  auto &s=*impl_;if(!s.Main())return;
  for(unsigned i=s.production_depth;i;--i){auto &p=s.producing[i-1];if(p.token!=token)continue;std::copy(args.begin(),args.end(),p.args.begin());
    s.ReadProduction(p);
    if(p.kind==BinaryProductionKind::SfxPlay)p.frame_delay=std::bit_cast<std::int32_t>(args[0]);
    if(p.kind>=BinaryProductionKind::MusicPlay&&p.kind<=BinaryProductionKind::MusicLayerOff)
      p.display_failed=!s.NativeValue(std::uint64_t(p.object)+0x30c,p.args[5]);
    if(p.kind==BinaryProductionKind::Fortune){p.args[1]=0;p.args[2]=UINT32_MAX;
      for(unsigned parent=i-1;parent;--parent){const auto &outer=s.producing[parent-1];if(outer.kind==BinaryProductionKind::FortuneSource){p.args[1]=outer.args[0];p.args[2]=outer.args[2];break;}}}
    if(p.kind==BinaryProductionKind::FortuneSource)p.args[2]=UINT32_MAX;
    if(p.kind==BinaryProductionKind::TransitionStart){const auto serial=s.display.ReserveTransition();
      p.args[6]=std::uint32_t(serial);p.args[7]=std::uint32_t(serial>>32);
      p.display_failed=!serial||!s.NativeValue(std::uint64_t(p.window.game)+0x1ba78,p.args[5]);}
    if(p.kind==BinaryProductionKind::BossIntro)for(unsigned parent=i-1;parent;--parent){const auto &outer=s.producing[parent-1];
      if(outer.kind==BinaryProductionKind::TransitionStart){p.args[6]=outer.args[6];p.args[7]=outer.args[7];break;}}
    if(p.kind==BinaryProductionKind::MapCache)p.display_failed=!s.NativeValue(std::uint64_t(p.window.room_address)+4,p.args[0]);
    s.collecting=false;return;}
}
void BinaryEntities::ProductionRandom(std::uint32_t token,std::uint32_t value) noexcept {
  auto &s=*impl_;if(!s.Main())return;
  for(unsigned i=s.production_depth;i;--i){auto &p=s.producing[i-1];if(p.token!=token)continue;
    if(p.kind==BinaryProductionKind::FortuneSource){p.args[2]=value;return;}
    if(p.kind==BinaryProductionKind::MapCache){s.ReadProduction(p);std::uint32_t count=0,focus=0;
      if(s.NativeValue(std::uint64_t(p.window.game)+0x182cc,count)&&value<count&&count<=527&&
         s.NativeValue(std::uint64_t(p.window.game)+36+std::uint64_t(value)*184+4,focus))
        s.display.MapFocus(p.object,p.args[0],p.window.game_frame,focus);
      else p.display_failed=true;s.collecting=false;return;}}
}
// 捕获正常LoadText实际消费的最终UTF16，再有界转UTF8；不额外本地化或重跑文字随机。
void BinaryEntities::ProductionBannerText(std::uint32_t token,std::uint32_t text,bool subtitle) noexcept {
  auto &s=*impl_;if(!s.Main())return;
  for(unsigned i=s.production_depth;i;--i){auto &p=s.producing[i-1];if(p.token!=token||p.kind!=BinaryProductionKind::ItemBanner)continue;
    s.ReadProduction(p);std::array<wchar_t,4096> value{};unsigned count=0;
    if(text)for(;count<value.size()-1;++count){if(!s.NativeValue(std::uint64_t(text)+count*2,value[count])){p.display_failed=true;break;}if(!value[count])break;}
    if(count==value.size()-1)p.display_failed=true;
    auto &out=subtitle?p.subtitle:p.title;auto &seen=subtitle?p.subtitle_seen:p.title_seen;
    if(seen)p.display_failed=true;
    const int size=count?WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value.data(),int(count),out.data(),int(out.size()-1),nullptr,nullptr):0;
    if(count&&!size)p.display_failed=true;else {out[std::size_t(size)]=0;seen=true;}
    s.collecting=false;return;}
}
// 9F7970是正常选样本后的共同尾叶：即使本地source为空也会到达，不能以Play后端成功分配世界实例。
void BinaryEntities::ProductionAudioPan(std::uint32_t token,std::uint32_t stream) noexcept {auto &s=*impl_;if(!s.Main())return;
  for(unsigned i=s.production_depth;i;--i){auto &p=s.producing[i-1];if(p.token!=token)continue;if(p.kind!=BinaryProductionKind::SfxPlay)return;
    s.ReadProduction(p);std::array<std::uint32_t,3> table{};std::uint32_t samples=0;
    bool valid=p.sound>=0 && stream>=24 && s.NativeValue(p.object,table) && table[0]<=table[1] && table[1]<=table[2] &&
      (table[1]-table[0])%440==0 && std::uint32_t(p.sound)<(table[1]-table[0])/440;
    const auto effect=std::uint64_t(table[0])+std::uint64_t(std::uint32_t(p.sound))*440;
    valid=valid && s.NativeValue(effect+400,samples) && samples>0 && samples<=7 && stream>=effect+32 &&
      (stream-effect-32)%56==0 && (stream-effect-32)/56<samples;
    if(valid){p.stream=stream;++p.selected;}else p.display_failed=true;
    s.collecting=false;return;}}
// 正常返回才封存；所有失败阻断新完整显示目标，不改变Authority输入或原生调用次数。
void BinaryEntities::ProductionEnd(std::uint32_t token,bool normal) noexcept {
  auto &s=*impl_;if(!s.Main() || !s.production_depth)return;auto p=s.producing[s.production_depth-1];
  if(p.token!=token){s.Failure({CaptureFailure::Reentrant});s.score={};s.production_depth=0;return;}
  --s.production_depth;
  if(normal&&p.kind==BinaryProductionKind::TransitionFinish){
    // ChangeRoom的正常完成会退休旧room；重新取得真实新room生产窗，不沿用旧凭据。
    if(s.capture.collector->NativeProduction(p.window,p.token))p.witness=s.Witness(p.window,CapturePhase::Production);else normal=false;
  }
  if(!normal || !s.capture.collector->NativeWindowCurrent(p.window)){
    if(p.kind==BinaryProductionKind::Score)s.score.revision=0;
    s.Failure({CaptureFailure::SourceChanged});return;}
  if(s.revision==UINT64_MAX){s.Failure({CaptureFailure::SequenceExhausted});return;}p.witness.revision=++s.revision;
  s.ReadProduction(p);
  if(p.kind>=BinaryProductionKind::MusicPlay||p.kind>=BinaryProductionKind::SfxPlay&&p.kind<=BinaryProductionKind::SfxAmbient){
    bool nested_music=false;
    if(p.kind>=BinaryProductionKind::MusicPlay&&p.kind<=BinaryProductionKind::MusicLayerOff)
      for(unsigned i=0;i<s.production_depth;++i)nested_music|=s.producing[i].kind>=BinaryProductionKind::MusicPlay&&s.producing[i].kind<=BinaryProductionKind::MusicLayerOff;
    if(nested_music){s.collecting=false;return;}
    EntityFacts facts;facts.subject={3,p.window.run,1,1};facts.address=p.window.game;facts.ready=true;facts.manager=p.manager;
    CaptureDiagnostic diagnostic;bool ok=!p.display_failed;
    if(p.kind==BinaryProductionKind::SfxPlay){if(p.audio_world&&p.selected){ok=ok&&p.selected==1;
      if(ok)BinaryNativeSfxFallback(p.kind,reinterpret_cast<void *>(p.object),p.sound,p.volume,p.frame_delay,p.loop,p.pitch,p.pan,reinterpret_cast<void *>(p.stream),p.ambient);} }
    else if(p.kind>=BinaryProductionKind::SfxVolume&&p.kind<=BinaryProductionKind::SfxAmbient){if(p.audio_world){p.args[0]=std::uint32_t(p.sound);p.args[1]=std::bit_cast<std::uint32_t>(p.volume);p.args[2]=std::bit_cast<std::uint32_t>(p.pitch);
        if(ok)BinaryNativeSfxFallback(p.kind,reinterpret_cast<void *>(p.object),p.sound,p.volume,0,false,p.pitch,0,nullptr,-1);}}
    else {if(p.kind==BinaryProductionKind::ItemBanner){p.args[6]=p.title_seen?world::PackWord(p.title.data()):0;p.args[7]=p.subtitle_seen?world::PackWord(p.subtitle.data()):0;}
      ok=ok&&s.display.Observe(p.kind,p.object,p.args,p.witness,facts,s.source,s.DisplayRead(),std::span(s.links).first(s.link_used),diagnostic);}
    if(!ok){s.display.Fault();s.Failure(diagnostic.reason==CaptureFailure::None?CaptureDiagnostic{CaptureFailure::MissingQualification}:diagnostic);}
    s.collecting=false;return;
  }
  if(p.kind==BinaryProductionKind::Score){
    std::array<std::uint32_t,15> values{};std::uint32_t time=0;
    if(p.object==std::uint64_t(p.window.game)+0x25c5c && s.NativeValue(p.object,values) && s.NativeValue(std::uint64_t(p.object)+0x4c,time)){
      s.score.scope=p.witness.scope;std::copy(values.begin(),values.end(),s.score.values.begin());s.score.values[15]=time;s.score.revision=p.witness.revision;
    }else s.Failure({CaptureFailure::MissingQualification,{},234});
  }
  s.collecting=false;
}
CaptureExchange::ReadTicket BinaryEntities::Take() noexcept {return impl_->exchange.Take();}
BinaryEntityCatalog BinaryEntities::Catalog(const CaptureExchange::ReadTicket &ticket) const noexcept {const auto &s=*impl_;
  if(std::this_thread::get_id()!=s.consumer)return {};const auto *packet=ticket.Packet();if(!packet)return {};
  return {packet->references,s.source.PublishedResources(packet->catalog)};}
bool BinaryEntities::EventsTransferred(const CaptureExchange::ReadTicket &ticket) noexcept {
  auto &s=*impl_;if(std::this_thread::get_id()!=s.consumer)return false;const auto *packet=ticket.Packet();
  if(!packet||packet->witness.phase!=CapturePhase::Ordinary||!packet->event_first_sequence||packet->event_watermark==UINT64_MAX)return false;
  const auto write=s.receipt_write.load(std::memory_order_relaxed),read=s.receipt_read.load(std::memory_order_acquire);
  if(std::uint32_t(write-read)>=s.event_receipts.size())return false;
  s.event_receipts[write%s.event_receipts.size()]={packet->witness.scope.session,packet->witness.scope.run,packet->event_watermark};
  s.receipt_write.store(write+1,std::memory_order_release);return true;
}
// 控制先于数据；普通票据一直保留到网络worker接管，再经原有SPSC回执回收主线程事件副本。
void BinaryEntities::Background(bool stopping) noexcept {auto &s=*impl_;if(std::this_thread::get_id()!=s.consumer)return;
  const auto withdraw=[&]{s.transferring.Reset();s.transfer_source.reset();s.publisher.Reset();std::lock_guard lock(s.outbox);s.published.reset();s.ordinary_published.reset();};
  CaptureControl control;while(s.exchange.PopControl(control)){
    if(control.kind==CaptureControlKind::Begin||control.kind==CaptureControlKind::Stop){withdraw();}
    else if(control.kind==CaptureControlKind::Invalidate||control.kind==CaptureControlKind::RetireBody){
      s.transferring.Reset();s.transfer_source.reset();std::lock_guard lock(s.outbox);s.published.reset();s.ordinary_published.reset();
    }
    // RetireBody撤去后续快候选；下一份完整ordinary负责真实成员离开，不能合成缺读。
  }
  if(stopping){withdraw();for(;;){auto ticket=s.exchange.Take();if(!ticket.Packet())break;}return;}
  if(s.transferring.Packet()){
    // 独立观测／Recorder没有网络订阅者；空Peer集合的接管在本consumer完成，避免卡住快槽。
    if(!s.network_publication.load(std::memory_order_acquire))s.transfer_source->events_transferred.store(1,std::memory_order_release);
    if(!s.transferring.Current()){s.transferring.Reset();s.transfer_source.reset();}
    else if(!s.transfer_source->events_transferred.load(std::memory_order_acquire)||!EventsTransferred(s.transferring))return;
    else {s.transferring.Reset();s.transfer_source.reset();}
  }
  for(unsigned n=0;n<9;++n){auto ticket=s.exchange.Take();const auto *packet=ticket.Packet();if(!packet)break;
    try {const auto catalog=Catalog(ticket);const auto &scope=packet->witness.scope;
      if(scope.session!=s.resource_session||scope.run!=s.resource_run){s.published_resources.clear();s.resource_session=scope.session;s.resource_run=scope.run;}
      auto resources=world::binary::ExportStateResources(catalog.resources,s.published_resources);
      auto value=s.publisher.Capture(*packet,resources,&ticket,[](void *p) noexcept {return static_cast<CaptureExchange::ReadTicket *>(p)->Current();});if(!ticket.Current())continue;
      s.published_resources=std::move(resources);
      {std::lock_guard lock(s.outbox);s.published=value;if(packet->witness.phase==CapturePhase::Ordinary)s.ordinary_published=value;s.publish_failure={};}
      if(packet->witness.phase==CapturePhase::Ordinary){s.transfer_source=std::move(value);s.transferring=std::move(ticket);break;}
    }catch(const std::exception &error){std::lock_guard lock(s.outbox);strncpy_s(s.publish_failure.data(),s.publish_failure.size(),error.what(),_TRUNCATE);}
    catch(...){std::lock_guard lock(s.outbox);strncpy_s(s.publish_failure.data(),s.publish_failure.size(),"binary_publish_exception",_TRUNCATE);}
  }
}
void BinaryEntities::NetworkPublication(bool enabled) noexcept {auto &s=*impl_;if(!s.Main())return;
  const auto before=s.network_publication.exchange(enabled?1:0,std::memory_order_acq_rel);if(enabled&&!before&&s.stats.active)Invalidate();}
HostStateSnapshotRef BinaryEntities::Publication() const noexcept {auto &s=*impl_;if(!s.Main()||!s.stats.active||s.invalidated)return {};std::unique_lock lock(s.outbox,std::try_to_lock);
  return lock&&s.published&&s.published->ordinary.revision>=s.publication_floor?s.published:nullptr;}
HostStateSnapshotRef BinaryEntities::OrdinaryPublication() const noexcept {auto &s=*impl_;if(!s.Main()||!s.stats.active||s.invalidated)return {};std::unique_lock lock(s.outbox,std::try_to_lock);
  return lock&&s.ordinary_published&&s.ordinary_published->ordinary.revision>=s.publication_floor?s.ordinary_published:nullptr;}
}
