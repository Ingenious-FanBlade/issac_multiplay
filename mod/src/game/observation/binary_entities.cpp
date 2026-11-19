#include "game/observation/binary_entities.h"
#include "game/observation/world_bridge.h"
#include "game/observation/binary_display_capture.h"
#include "game/authority/state.h"
#include "game/authority/tick.h"
#include "game/common/bridge.h"
#include "game/common/binary_effect_e2e.h"
#ifdef ISAACCS_E2E
#include "game/tests/client_entity_e2e.h"
#include "game/tests/e09_compare.h"
#endif
#include "world/observe.h"
#include "state/budget.h"
#include "core/capture_cost.h"
#include "runtime.h"
#include "replica/stream_metrics.h"
#include "world/binary/resource_export.h"
#include "state_host_fields.inc"
#include <mutex>
#include <algorithm>
#include <bit>
#include <tuple>
#include <cmath>
#include <cstdio>
#include <limits>
#include <windows.h>
#include <atomic>

namespace cs::game {
namespace {
using namespace state;
using world::binary::EntityFacts;
using world::binary::NativeScene;
std::atomic<std::uint32_t> foreign_resource_writes{0},foreign_resource_active{0};
bool ForeignResourceCurrent(std::uint32_t watermark) noexcept {
  return !foreign_resource_active.load(std::memory_order_acquire)&&
    watermark==foreign_resource_writes.load(std::memory_order_acquire);
}
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
  cfg.capacity={1,4096,16384,512u<<10,16};
  // 密集战斗会先耗尽普通快照的值条目。只在冷准备时调整整房槽，仍由交换池计入共享上限。
  cfg.ordinary_buffers=4;cfg.ordinary_capacity={2048,kOrdinaryCaptureCells,131072,2u<<20,16392,true};return cfg;
}
// 只绑定已安装官方1.1.3模块的真实helper／ctor与静态表关系，不执行目标helper或校验哈希。
world::binary::EntitySourceCapacity SourceCapacity(world::MemoryReader memory) noexcept {
  world::binary::EntitySourceCapacity out;const auto module=GetModuleHandleW(L"zhlREPENTOGON.dll");
  const auto copy_image=memory.copy_image?memory.copy_image:memory.copy;
  const auto base=reinterpret_cast<std::uintptr_t>(module);if(!base || base>UINT32_MAX || !copy_image)return out;
  const auto read=[&](std::uint32_t offset,void *p,std::size_t bytes){return base+offset<=UINT32_MAX && copy_image(std::uint32_t(base)+offset,p,bytes);};
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

void BinaryForeignResourceWrite(bool entering) noexcept {
  if(entering)foreign_resource_active.fetch_add(1,std::memory_order_acq_rel);
  auto value=foreign_resource_writes.load(std::memory_order_relaxed);
  while(value!=UINT32_MAX&&!foreign_resource_writes.compare_exchange_weak(value,value+1,std::memory_order_release,std::memory_order_relaxed)){}
  if(!entering)foreign_resource_active.fetch_sub(1,std::memory_order_release);
}

// 主线程状态与后台交接分开；后台只接触exchange的无锁纯值／控制，不读取这些原生关联。
struct BinaryEntities::Impl {
  WorldCapture &capture;
  std::thread::id producer,consumer;
  memory::Lease memory;
  world::binary::CaptureReadSession reads;
  world::binary::EntitySource source;
  CaptureExchange exchange;
  BinaryDisplayCapture display;
  // 编码后台独占发布器；纯值邮箱可由网络线程直接读取。
  HostStateCapture publisher;
  CaptureExchange::ReadTicket transferring;
  HostStateSnapshotRef transfer_source;
  replica::stream::MetricsMailbox worker_metrics;
  replica::stream::Metrics worker_samples;
  std::optional<replica::stream::Measure> worker_sample;
  std::shared_ptr<HostStatePublication> publication=std::make_shared<HostStatePublication>();
  std::vector<HostStateResource> published_resources;
  std::uint64_t resource_session=0,resource_run=0;
  std::array<char,128> publish_failure{};
  std::mutex publish_status;
  std::atomic<std::uint32_t> network_publication{0};
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
    // 显示事件只保留玩家身份，正常返回时核对同一 use；不借此授予字段读取资格。
    CaptureReferenceEntry event_player;
    std::uint32_t player_entry_flags=0;
    std::array<char,4096> title{},subtitle{};
    bool title_seen=false,subtitle_seen=false,display_failed=false;
    std::int32_t frame_delay=0;
  };
  std::array<Produced,16> producing{};
  std::uint32_t production_depth=0,next_production_token=0;
  std::vector<EntityFacts> roots;
  std::vector<std::uint32_t> root_index,link_index,published_addresses;
#ifdef ISAACCS_E2E
  BinaryBloodEffects committed_blood_effects;
  CaptureSubject last_unready_entity;
#endif
  struct GridKey {std::uint32_t address=0,index=0;std::uint64_t handle=0;};
  std::array<GridKey,456> grid_keys{};
  std::vector<CaptureReferenceEntry> links;
  std::vector<TextWitness> texts;
  std::array<GroupFacts,4> groups{};
  // 一份结构投影保留原始成员顺序、描述槽和 Grid 别名，尾部复核另用固定暂存。
  struct PreparedScene : NativeScene {
    std::vector<std::uint32_t> members,member_check;
    std::array<world::RawSource,10> sources{};
  } scene;
  std::array<EntityFacts,986> scene_roots{};
  std::array<std::array<std::uint32_t,15>,527> descriptor_shapes{};
  std::array<std::uint64_t,527> descriptor_revisions{};
  std::uint32_t scene_root_count=0,floor_depth=0;
  std::uint64_t floor=0,next_floor=0,floor_run=0;
  bool floor_ready=true;
  // 封存已完成根尾复核的纯值段；提交核实际窗口与全局失效水位。
  struct CandidateSeal {
    std::uint64_t read_serial=0,resource_mutation=0;
    std::uint32_t roots=0,foreign_resource=0,references=0;
  };
  struct Pending {
    CaptureExchange::WriteTicket ticket;
    CaptureWitness witness;
    world::NativeCaptureWindow window;
    CaptureRoot anchor;
    EntityFacts facts;
    world::PlayerPostWitness player_post;
    std::uint64_t resource_change=0;
    std::uint32_t roots=0;
    CandidateSeal seal;
  } ordinary;
  std::array<Pending,8> bodies;
  std::array<CaptureSubject,8> fast_keys{};
  std::array<std::uint32_t,8> fast_addresses{};
  world::NativeCaptureWindow reading_window;
  CaptureWitness reading_witness;
  std::uint32_t reading_foreign_resource=0;
  std::uint32_t root_used=0,ordinary_count=0,published_count=0,link_used=0,text_used=0;
  std::uint64_t revision=0,last_ordinary=0,next_related=0,resource_mutation=0,reading_resource_mutation=0,run=0,room=0;
  std::uint64_t display_run=0;
  std::uint32_t ordinary_failures=0;
  std::uint64_t worker_ordinary=0,worker_body=0,worker_failures=0;
  BinaryEntityStats stats;
  // 撤销来源保留有界聚合及最近身份纯值，只在显式状态查询时格式化。
  std::uint64_t entity_invalidations=0,sprite_invalidations=0;
  std::uint64_t sprite_history_preserved=0;
  // 资源Hook的具名操作只复制到固定诊断槽，不在热路径格式化日志。
  std::array<char,64> last_sprite_operation{};
  std::array<std::uint64_t,10> last_sprite_owner{};
  std::array<std::uint64_t,8> last_entity_invalidation{};
  // 仅失败分支填写，记录来源窗口拒绝的具体层次；每个读取段开始时清空。
  std::array<char,128> current_failure{};
  bool collecting=false,invalidated=false,preparing=false,prepare_reading=false,prepare_revoked=false;
  Impl(WorldCapture &c,std::thread::id p,std::thread::id worker):capture(c),producer(p),consumer(worker),
    memory(memory::Reserve(memory::Pool::Values,sizeof(*this)+16384u*(sizeof(EntityFacts)+2*sizeof(std::uint32_t)+sizeof(TextWitness))+16392u*(sizeof(CaptureReferenceEntry)+sizeof(std::uint32_t))+2u*world::kMembershipBudget*4,"binary_entity_bridge")),
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
        return index!=UINT32_MAX && owner.roots[index].ready && owner.Current(owner.reading_witness,owner.roots[index]);},&reads,
      [](void *context,const CaptureWitness &w,const EntityFacts &facts) noexcept {return static_cast<Impl *>(context)->Current(w,facts,true);},
      [](void *context,std::uint32_t address,CaptureSubject &subject) noexcept {
        const auto &s=*static_cast<Impl *>(context);
        const auto n=s.RootIndex(address);
        if(n==UINT32_MAX)return false;
        const auto &root=s.roots[n];
        const auto *current=s.capture.identities.Find(world::ObjectDomain::Entity,address);
        if(!root.ready||root.capture_failure!=CaptureFailure::None||!current||current->Retired()||
           current->local_id!=root.subject.local||current->use!=root.subject.use||current->revision!=root.subject.shape)return false;
        // 引用只携带物理身份及形态，Player 姿态 revision 和绑定不参与引用判等。
        subject={root.subject.space,root.subject.local,root.subject.use,root.subject.shape};
        return true;
      }},SourceCapacity(c.source_memory)),exchange(Buffers(),p,worker),
    display(p) {
    roots.resize(16384);root_index.resize(16384);links.resize(16392);link_index.resize(16392);texts.resize(16384);
    published_addresses.resize(16384);
    scene.members.resize(world::kMembershipBudget);scene.member_check.resize(world::kMembershipBudget);
    memory->Resize(sizeof(*this)+roots.capacity()*sizeof(EntityFacts)+texts.capacity()*sizeof(TextWitness)+
      links.capacity()*sizeof(CaptureReferenceEntry)+(root_index.capacity()+published_addresses.capacity()+link_index.capacity()+
      scene.members.capacity()+scene.member_check.capacity())*sizeof(std::uint32_t));
    // 所有采集暂存和交接缓冲合计使用同一共享额度。失败时保留实际计费用量。
    const auto retained=memory->Bytes()+source.RetainedBytes()+exchange.RetainedBytes()+display.RetainedBytes()+
      TopologyReadWorkspaceBytes();
    if(retained>StateBudgets().shared_value_bytes){
      Trace("capture_prepare_error","retained_bytes=%llu shared_limit_bytes=%llu",
        static_cast<unsigned long long>(retained),static_cast<unsigned long long>(StateBudgets().shared_value_bytes));
      throw std::invalid_argument("binary_entity_shared_capacity");
    }
    source.ReserveReuse(StateBudgets().shared_value_bytes-retained);
  }
  bool Main() const noexcept {return std::this_thread::get_id()==producer;}
  // 保留已发布普通结构的根集合；新候选准备期间仍可识别其生命周期写入。
  void RememberPublished() noexcept {
    published_count=ordinary_count;
    for(std::uint32_t i=0;i<published_count;++i)published_addresses[i]=roots[root_index[i]].address;
  }
  bool ReadWindowCurrent() const noexcept {
    return Main()&&collecting&&capture.collector&&reading_resource_mutation==resource_mutation&&
      capture.collector->NativeWindowCurrent(reading_window,false);
  }
  bool ContinueObjectReads() noexcept {
    return !prepare_revoked&&reads.ContinueAfterObjectFailure()&&ReadWindowCurrent();
  }
  bool Copy(std::uint64_t address,void *out,std::size_t size) noexcept {
    return ReadWindowCurrent()&&reads.Copy(address,out,size);
  }
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
    [](void *p,std::uint64_t address,std::span<std::byte> out) noexcept {return static_cast<Impl *>(p)->Copy(address,out.data(),out.size());},
    [](void *p,std::uint32_t address) noexcept {return static_cast<Impl *>(p)->Reference(address);},
    [](void *p) noexcept {auto &s=*static_cast<Impl *>(p);return std::span<const CaptureReferenceEntry>(s.links).first(s.link_used);},
    [](void *p,const CaptureReferenceEntry &ref) noexcept {return static_cast<Impl *>(p)->Register({ref.space,ref.local,ref.use});}};}
  // 结构读取与字段执行器共用当前窗口，地址加法保持u64直到检查完成，错误不形成环回读取。
  template<class T> bool NativeValue(std::uint64_t address,T &value) noexcept {
    return Copy(address,&value,sizeof(T));
  }
  bool Current(const CaptureWitness &w,const EntityFacts &f,bool grid_already_qualified=false) noexcept {
    const auto reject=[&](const char *stage){
      const auto *id=(f.subject.space==1||f.subject.space==2)?capture.identities.Find(
        f.subject.space==2?world::ObjectDomain::Grid:world::ObjectDomain::Entity,f.identity_address?f.identity_address:f.address):nullptr;
      if(id&&f.subject.space==2){
        // Grid诊断使用同一有效读取段，只补已在Grid分类／资格检查中读取的vtable与initialized。
        std::uint32_t vtable=0;std::uint8_t initialized=0;
        const bool readable=NativeValue(id->address,vtable)&&NativeValue(std::uint64_t(id->address)+0x1c,initialized);
        const auto base=capture.source_memory.image_base;
        std::snprintf(current_failure.data(),current_failure.size(),
          "current=%s grid=%llu/%llu ready=%u candidate=%u owner=%llu birth_kind=%u vtable=%x init=%u read=%u",
          stage,static_cast<unsigned long long>(id->local_id),static_cast<unsigned long long>(id->use),id->readiness,unsigned(id->candidate),
          static_cast<unsigned long long>(id->owner_scope),id->birth_kind,vtable>=base?vtable-base:0,unsigned(initialized),unsigned(readable));
        return false;
      }
      std::snprintf(current_failure.data(),current_failure.size(),
        "current=%s subject=%u/%llu/%llu shape=%llu actual=%llu ready=%u retired=%u ordinal=%u binding=%llu",
        stage,unsigned(f.subject.space),static_cast<unsigned long long>(f.subject.local),static_cast<unsigned long long>(f.subject.use),
        static_cast<unsigned long long>(f.subject.shape),static_cast<unsigned long long>(id?id->revision:0),
        id?id->readiness:0,unsigned(id&&id->Retired()),unsigned(f.ordinal),static_cast<unsigned long long>(f.binding_revision));
      return false;
    };
    if(!Main()||!stats.active||!collecting||reading_witness!=w||reading_resource_mutation!=resource_mutation||!capture.collector)
      return reject("window");
    if(f.subject.space==1||f.subject.space==2){
      const bool grid=f.subject.space==2;
      if(grid&&!grid_already_qualified&&!reads.Good())return reject("window");
      world::NativeSource reader;reader.owner=this;
      reader.copy=[](void *p,std::uint64_t address,void *out,std::size_t size) noexcept {return static_cast<Impl *>(p)->Copy(address,out,size);};
      // NativeCurrent 已核窗口。Grid 首次资格检查始终借用当段的受保护读取。
      if(!capture.collector->NativeCurrent(reading_window,f.identity_address?f.identity_address:f.address,f.subject.local,f.subject.use,f.subject.shape,
         grid?world::ObjectDomain::Grid:world::ObjectDomain::Entity,grid?&reader:nullptr,grid&&grid_already_qualified,f.roster!=kNullIndex))
        return reject(capture.collector->NativeWindowCurrent(reading_window)?"identity":"window");}
    else if(!capture.collector->NativeWindowCurrent(reading_window))return reject("window");
    else if(reading_window.production){if(w.scope.run!=reading_window.run || w.scope.room!=reading_window.room || !f.ready)return reject("production");}
    else {
      if(!reading_window.ordinary||!floor_ready||floor_depth||w.scope.floor!=floor||f.scene!=&scene)return reject("scene");
      bool mapped=false;
      if(f.layout==239)mapped=f.address==scene.room;
      else if(f.layout==437)mapped=f.manager&&std::uint64_t(f.manager)+0x2a170==f.address;
      else if(f.layout==276||f.layout==237||f.layout==382||f.layout==383)mapped=f.address==scene.game;
      else if(f.layout==270)mapped=f.source_slot<scene.descriptors.size()&&scene.descriptors[f.source_slot]==f.address&&
        f.subject.use==floor&&f.subject.shape==descriptor_revisions[f.source_slot];
      // 448—453只接纳锚定同一Game／run的自有事件树，不扩大任意原生对象可读范围。
      else if(f.layout>=448&&f.layout<=453)mapped=f.ready&&f.address==scene.game&&
        f.subject==CaptureSubject{3,w.scope.run,1,1}&&!f.display_values.empty()&&f.display_values.front().layout==f.layout;
      if(!mapped)return reject("mapping");
    }
    if(f.ordinal<4){const auto *runtime=Runtime();if(!runtime || runtime->finished || runtime->retiring || runtime->failure ||
        !runtime->session.HasParticipant(f.ordinal) || SessionGeneration(*runtime)!=f.control_session ||
        runtime->players[f.ordinal].Generation()!=f.binding_revision)return reject("binding");}
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
    // 预测插值 5.7：tick 与相位对所有身体相同。Host 开插值时偶帧 StandardRoom、奇帧 OddPlayer 各走一个半步；
    // 关插值时 StandardRoom 一个 tick 内走两个半步，样本落在奇半帧之后。
    {const bool odd=runtime->boundary.kind==core::TickKind::OddPlayer;const bool interpolating=NativeInterpolationState()!=0;
      f.input_tick=std::uint32_t(runtime->clock.Current());
      f.input_phase=std::uint8_t((odd||!interpolating?1u:0u)|((interpolating?1u:2u)<<1));}
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
    f.input_sequence=std::uint32_t(runtime->players[found].LastProcessedSequence());
    f.subject.binding=group.binding;
  }
  bool Groups(CaptureDiagnostic &diagnostic) noexcept {
    auto *runtime=Runtime();for(auto &group:groups){group.count=0;group.present=false;}
    if(!runtime || runtime->finished || runtime->retiring || runtime->failure)return true;
    for(unsigned ordinal=0;ordinal<runtime->session.ParticipantLimit();++ordinal){if(!runtime->session.HasParticipant(ordinal))continue;
      const auto &topology=runtime->topology[ordinal];auto &group=groups[ordinal];
      if(!topology.known || !topology.unique || topology.fault!=TopologyFault::None || topology.room!=runtime->topology[0].room){
        // 控制组未取到同房唯一身份时记录具体分支，避免全局采集停住后只剩无来源的错误码。
        FailCapture(diagnostic,CaptureFailure::MissingQualification,{});
        std::snprintf(diagnostic.detail.data(),diagnostic.detail.size(),
          "group=%u known=%u unique=%u fault=%s room_match=%u reason=%s",ordinal,unsigned(topology.known),
          unsigned(topology.unique),TopologyFaultName(topology.fault),unsigned(topology.room==runtime->topology[0].room),topology.reason);
        return false;
      }
      const auto session=SessionGeneration(*runtime);if(!session)return FailCapture(diagnostic,CaptureFailure::MissingQualification,{});
      if(group.session!=session){if(next_related>UINT64_MAX-2)return FailCapture(diagnostic,CaptureFailure::SequenceExhausted,{});group.session=session;group.participant=++next_related;group.group=++next_related;}
      group.binding=runtime->players[ordinal].Generation();if(!group.binding)return FailCapture(diagnostic,CaptureFailure::MissingQualification,{});
      const auto participant=Register({9,group.participant,1}),control=Register({10,group.group,1});
      if(participant.kind==CapturedReferenceKind::Invalid || control.kind==CapturedReferenceKind::Invalid)return FailCapture(diagnostic,CaptureFailure::ReferencePending,{});
      group.participant_link=participant.handle;group.group_link=control.handle;group.present=true;
    }
    return true;
  }
  // 原位改变 Type、Variant 或 SubType 都须由普通采集封存到新形态，
  // 让客机用现有结构事务安装完整目标；Morph／Init 回退到新形态的首次取样。
  CaptureFailure ObserveEntityShape(world::Identity &id) noexcept {
    if(!collecting||!reading_window.ordinary||id.readiness<2)return CaptureFailure::None;
    std::array<std::uint32_t,3> shape{};
    if(!NativeValue(std::uint64_t(id.address)+0x28,shape))return CaptureFailure::ReadFailed;
    if(id.native_shape_known&&id.native_shape!=shape){
      if(id.revision>=UINT64_MAX-1)return CaptureFailure::SequenceExhausted;
      ++id.revision;
    }
    id.native_shape=shape;id.native_shape_known=true;
    return CaptureFailure::None;
  }
  // Body 只采普通采集已确认的形态。形态变化由后续 World 发布。
  bool BodyShapeCurrent(const EntityFacts &facts,CaptureDiagnostic &diagnostic) noexcept {
    const auto *id=capture.identities.Find(world::ObjectDomain::Entity,facts.address);
    const CaptureRoot root{facts.subject,facts.address,0,197};
    if(!id||!id->native_shape_known)return FailCapture(diagnostic,CaptureFailure::MissingQualification,root);
    std::array<std::uint32_t,3> shape{};
    if(!NativeValue(std::uint64_t(facts.address)+0x28,shape))return FailCapture(diagnostic,CaptureFailure::ReadFailed,root);
    return shape==id->native_shape||FailCapture(diagnostic,CaptureFailure::SourceChanged,root);
  }
  EntityFacts *Add(std::uint32_t address,std::uint32_t membership=0,std::uint16_t roster=kNullIndex) noexcept {
    if(!address)return nullptr;const auto prior=RootIndex(address);
    if(prior!=UINT32_MAX){auto &f=roots[prior];f.memberships|=membership;if(roster!=kNullIndex)f.roster=roster;return &f;}
    if(root_used==roots.size())return nullptr;auto *id=capture.identities.Find(world::ObjectDomain::Entity,address);
    if(!id||id->Retired())return nullptr;
    const auto failure=ObserveEntityShape(*id);
    if(failure!=CaptureFailure::None&&!ContinueObjectReads())return nullptr;
    EntityFacts f;f.capture_failure=failure;f.address=address;f.subject={1,id->local_id,id->use,id->revision};f.memberships=membership;f.roster=roster;
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
  // candidate只说明出生记录未确认。正常Player更新已建立当前资格且唯一锚定时，
  // 可把其同一local/use用于道具横幅和转场事件归属；字段读取资格仍独立检查。
  bool DisplayPlayerIdentity(const world::Identity &id) const noexcept {
    if(id.Retired())return false;
    if(!id.candidate)return true;
    if(!id.current_public_ready||id.readiness<2||!capture.collector)return false;
    unsigned matches=0;
    for(unsigned slot=0;slot<world::kControlAnchorCapacity;++slot){
      const auto anchor=capture.collector->CurrentAnchor(slot);
      if(anchor.valid&&anchor.player==id.address&&anchor.session&&anchor.generation)++matches;
    }
    return matches==1;
  }
  // 横幅与转场从各自已核的原生参数取得 Player*。
  static std::uint32_t DisplayPlayerAddress(const Produced &p) noexcept {
    if(p.kind==BinaryProductionKind::ItemBanner||p.kind==BinaryProductionKind::CustomBanner)return p.args[0];
    if(p.kind==BinaryProductionKind::TransitionStart)return p.args[3];
    return 0;
  }
  CaptureWitness Witness(const world::NativeCaptureWindow &w,CapturePhase phase) noexcept {
    if(revision==UINT64_MAX)return {};CaptureWitness out;
    out.scope={capture.session_key,w.run,floor,floor?floor*3+scene.dimension+1:0,w.room,w.room};out.generation=capture.epoch;out.revision=++revision;out.batch=w.attempt;
    out.tick=w.tick;out.outer=w.outer;out.owner=w.owner;out.mutation=w.mutation;out.game_frame=w.game_frame;out.phase=phase;
    if(phase==CapturePhase::Production)out.batch=w.owner;
    if(phase==CapturePhase::PlayerPost)out.required_revision=last_ordinary;return out;
  }
  bool BeginRead(const world::NativeCaptureWindow &w,const CaptureWitness &v,std::uint64_t resource) noexcept {
    current_failure={};
    const auto foreign=foreign_resource_writes.load(std::memory_order_acquire);
    if(foreign==UINT32_MAX||foreign_resource_active.load(std::memory_order_acquire)){
      reads.RevokeForResourceChange();return false;
    }
    if(!reads.Begin([](void *context) noexcept {
         return ForeignResourceCurrent(static_cast<Impl *>(context)->reading_foreign_resource);},this))return false;
    // 上一段的复制结果可能在根尾水位复核前暂存到了资源缓存；新水位必须丢弃这些证明。
    if(foreign!=reading_foreign_resource){source.InvalidateReuse();source.ClearResourceCaches();}
    reading_foreign_resource=foreign;
    reading_window=w;reading_witness=v;reading_resource_mutation=resource;collecting=true;
    return true;
  }
  void EndRead() noexcept {reads.End();collecting=false;}
  bool ReadWindow(const Pending &p) noexcept {
    // 保留 getter 返回后的禁止重入边界。各次读段使用独立证言，页权限另由 Manager 帧共享。
    if(!preparing||prepare_revoked)return false;
    prepare_reading=true;
    return BeginRead(p.window,p.witness,p.resource_change);
  }
  bool ReadProduction(const Produced &p) noexcept {
    // 首个 Prepare 读段开始后，意外生产回调撤销原候选。
    if(prepare_reading){reads.Revoke();prepare_revoked=true;return false;}
    return BeginRead(p.window,p.witness,resource_mutation);
  }
  bool Seal(Pending &p) noexcept {
    if(prepare_revoked||!p.roots||!ReadWindowCurrent())return false;
    const auto serial=reads.Serial();
    if(!serial)return false;
    p.seal={serial,resource_mutation,p.roots,reading_foreign_resource,link_used};return true;
  }
  // ordinary 的引用目录在当前 outer 内只追加；快槽借用封存前缀并按物理身份登记到自身目录。
  bool OrdinaryBodySourceCurrent(const Pending &p) const noexcept {
    return !prepare_revoked&&p.seal.read_serial&&p.seal.resource_mutation==resource_mutation&&p.seal.roots==p.roots&&
      p.seal.references<=link_used&&ForeignResourceCurrent(p.seal.foreign_resource);
  }
  bool ProductionInto(CaptureValueWriter &,std::uint32_t &,CaptureDiagnostic &) noexcept;
  void Failure(CaptureDiagnostic d) noexcept {
    if(d.reason==CaptureFailure::InvalidWindow&&!d.detail[0]){
      if(current_failure[0])d.detail=current_failure;
      else std::snprintf(d.detail.data(),d.detail.size(),"current=executor reads=%u collecting=%u phase=%u subject=%u/%llu",
        unsigned(reads.Good()),unsigned(collecting),unsigned(reading_witness.phase),unsigned(d.subject.space),static_cast<unsigned long long>(d.subject.local));
    }
    if(d.reason==CaptureFailure::SourceChanged)++stats.source_changed;
    ++stats.gaps;stats.failure=d;
  }
  void ObjectFailure(EntityFacts &root,CaptureDiagnostic diagnostic) noexcept {
    root.capture_failure=diagnostic.reason;
    diagnostic.subject=root.subject;
    if(!diagnostic.layout)diagnostic.layout=root.layout;
    if(diagnostic.reason==CaptureFailure::InvalidWindow&&!diagnostic.detail[0])diagnostic.detail=current_failure;
    if(!stats.object_failures)stats.first_object_failure=diagnostic;
    if(diagnostic.reason==CaptureFailure::SourceChanged)++stats.source_changed;
    ++stats.object_failures;
  }
  // 实体／格子的 World 和 Body 共享回卷点。成员存在性由独立来源复核。
  bool RetainFailedRoot(CaptureValueWriter &writer,CaptureValueWriter::RootCheckpoint checkpoint,
      EntityFacts &root,CaptureDiagnostic &diagnostic) noexcept {
    if(!ContinueObjectReads())return false;
    auto failure=diagnostic.reason==CaptureFailure::None?writer.Diagnostic():diagnostic;
    if(failure.reason==CaptureFailure::None)
      failure.reason=root.capture_failure==CaptureFailure::None?CaptureFailure::Incomplete:root.capture_failure;
    if(!writer.RetainRoot(checkpoint,root.subject,root.layout,failure.reason))return false;
    ObjectFailure(root,failure);
    diagnostic={};
    return true;
  }
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
  // 正式采集按原生列表顺序登记成员；显式诊断有完整值时可共享其首遍，段尾总会独立复核。
  bool Member(unsigned source,std::uint32_t address,std::uint32_t index) noexcept {
    // _tempEL 只保留原生暂存成员的见证。已离开活动列表的炸弹等对象不能靠暂存条目再次发布。
    if(source==5)return true;
    const auto flags=source<=4||source==7?1u|(source==3?2u:0u)|(source==2?4u:0u):0u;
    return !(source==8&&index>=kNullIndex)&&Add(address,flags,source==8?std::uint16_t(index):kNullIndex);
  }
  bool Members(bool check) noexcept {
    if(!check&&capture.collector->DiagnosticMembers(reading_window,scene.sources,scene.members)){
      std::uint32_t first=0;
      for(unsigned source=0;source<scene.sources.size();++source){const auto count=scene.sources[source].count;
        for(std::uint32_t i=0;i<count;++i){const auto address=scene.members[first+i];if(address&&!Member(source,address,i))return false;}
        first+=count;}
      return true;
    }
    ++stats.member_scans;
    struct Context {Impl &source;unsigned id=0;bool failed=false;} context{*this};
    world::NativeSource reader;reader.owner=&context;reader.game=reading_window.game;reader.room=reading_window.room_address;
    reader.copy=[](void *p,std::uint64_t address,void *out,std::size_t bytes) noexcept {auto &c=*static_cast<Context *>(p);auto &s=c.source;
      return s.Copy(address,out,bytes);};
    reader.incomplete=[](void *p,world::Reason) noexcept {static_cast<Context *>(p)->failed=true;};
    std::array<std::uint32_t,4> fixed{};std::uint32_t used=0;
    auto output=check?std::span(scene.member_check):std::span(scene.members);
    for(unsigned id=0;id<10;++id){if(id==6)continue;context.id=id;world::RawSource actual;
      world::EnumerateSource(reader,id,actual,output.subspan(used),fixed,&context,
        check?+[](void *,std::uint32_t,std::uint32_t) noexcept {}:
        +[](void *p,std::uint32_t address,std::uint32_t index) noexcept {auto &c=*static_cast<Context *>(p);
          if(!c.source.Member(c.id,address,index))c.failed=true;},true);
      if(context.failed || actual.status || actual.count>output.size()-used)return false;
      if(check){const auto &prior=scene.sources[id];
        if(prior.count!=actual.count || prior.descriptor!=actual.descriptor ||
           !std::equal(output.begin()+used,output.begin()+used+actual.count,scene.members.begin()+used))return false;
      }else scene.sources[id]=actual;
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
  // 枚举结构后在同段末尾复核；Authority 提交使用相同窗口和失效水位。
  bool SceneRead(bool check) noexcept {
    ++stats.scene_scans;
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
       !read(std::uint64_t(current.game)+0x17adc,occupancy) || !read(std::uint64_t(current.room)+36,current.grids)||
       !read(std::uint64_t(current.room)+1860,current.door_indices))return false;
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
      current.grids==scene.grids && current.door_indices==scene.door_indices&&current.descriptors==scene.descriptors && current.occupancy==scene.occupancy && current.descriptor_order==scene.descriptor_order;
    static_cast<NativeScene &>(scene)=current;return true;
  }
  bool SceneSeed(const BinaryDisplayInputs &inputs,CaptureDiagnostic &diagnostic) noexcept {
    scene_root_count=0;
    const auto push=[&](EntityFacts facts){if(scene_root_count==scene_roots.size())return false;
      facts.scene=&scene;facts.ready=true;const auto link=Register(facts.subject);if(link.kind==CapturedReferenceKind::Invalid)return false;
      links[std::size_t(link.handle-1)].source_slot=facts.source_slot;
      scene_roots[scene_root_count++]=facts;return true;};
    EntityFacts context;context.address=scene.game;context.subject={4,floor,1,1};context.layout=276;context.scene=&scene;context.ready=true;
    if(!source.ReadSceneContext(reading_witness,context,scene,diagnostic))return false;
    // 全448槽和八门都扫描；同一对象只捕获一次，八槽引用各自保留真实别名。
    for(unsigned slot=0;slot<456;++slot){const auto address=scene.grids[slot];if(!address)continue;
      const auto end=grid_keys.begin()+scene_root_count;
      const auto prior=std::lower_bound(grid_keys.begin(),end,address,[](const auto &entry,auto p){return entry.address<p;});
      if(prior!=end&&prior->address==address){
        // 已出现的 Grid 根复用物理槽与 handle；门别名仍核映射索引。
        const auto actual=slot<448?std::int32_t(slot):scene.door_indices[slot-448];
        if(actual<0||std::uint32_t(actual)!=prior->index)return FailCapture(diagnostic,CaptureFailure::SourceChanged,{{2},address,0,199},1);
        scene.grid_handles[slot]=prior->handle;continue;
      }
      const auto actual=slot<448?std::int32_t(slot):scene.door_indices[slot-448];
      const auto *id=capture.identities.Find(world::ObjectDomain::Grid,address);
      if(!id||id->Retired())return FailCapture(diagnostic,CaptureFailure::MissingQualification,{{2},address,0,199});
      if(actual<0||actual>=448)return FailCapture(diagnostic,CaptureFailure::SourceChanged,{{2},address,0,199},1);
      const auto index=std::uint32_t(actual);
      std::uint32_t native_index=0;std::uint8_t initialized=0;
      CaptureFailure failure=CaptureFailure::None;
      if(!NativeValue(std::uint64_t(address)+0x24,native_index)||!NativeValue(std::uint64_t(address)+0x1c,initialized))
        failure=CaptureFailure::ReadFailed;
      else if(initialized!=1)failure=CaptureFailure::MissingQualification;
      else if(native_index!=index)failure=CaptureFailure::SourceChanged;
      const CaptureSubject subject{2,id->local_id,id->use,id->revision};
      if(failure!=CaptureFailure::None&&!ContinueObjectReads())return FailCapture(diagnostic,failure,{subject,address,0,199},1);
      auto link=Register(subject);if(link.kind==CapturedReferenceKind::Invalid)return false;
      scene.grid_handles[slot]=link.handle;
      std::move_backward(prior,end,end+1);*prior={address,index,link.handle};
      EntityFacts facts;facts.subject=subject;facts.address=address;facts.grid_index=index;facts.source_slot=std::uint16_t(index);
      facts.capture_failure=failure;
      if(!push(facts))return false;
    }
    for(unsigned i=0;i<scene.descriptor_count;++i){const auto slot=scene.descriptor_order[i];EntityFacts facts;
      facts.subject={6,std::uint64_t(slot)+1,floor,descriptor_revisions[slot]};facts.address=scene.descriptors[slot];facts.layout=270;facts.source_slot=slot;
      if(!push(facts))return false;const auto link=Register(facts.subject);links[std::size_t(link.handle-1)].order=std::uint16_t(i);}
    if(!push(context))return false;context.address=scene.room;context.subject={7,reading_window.room,1,1};context.layout=239;
    if(!push(context))return false;
    context.address=scene.game;context.subject={3,reading_window.run,1,1};context.layout=237;
    if(!NativeValue(std::uint64_t(capture.source_memory.image_base)+0x7e4a1c,context.manager) || !context.manager ||
       !source.ReadDisplayContext(reading_witness,context,diagnostic) ||
       !display.Prepare(reading_witness,context,source,DisplayRead(),inputs,diagnostic)||!push(context))return false;
    context.layout=437;context.address=context.manager+0x2a170;context.subject={12,reading_window.run,1,1};
    return context.manager<=UINT32_MAX-0x2a174 && push(context);
  }
  bool SceneInto(CaptureValueWriter &writer,std::uint32_t &count,CaptureDiagnostic &diagnostic) noexcept {
    capture::Cost cost(capture::CostPart::SceneValues);
    for(std::uint32_t i=0;i<scene_root_count;++i){
      auto &root=scene_roots[i];
      const auto checkpoint=writer.Checkpoint();
      std::uint16_t layout=root.layout;
      if(root.capture_failure!=CaptureFailure::None||!source.Capture(reading_witness,root,BodyStream::World,writer,diagnostic,true,0,&layout)){
        root.layout=layout;
        if(root.subject.space!=2||!RetainFailedRoot(writer,checkpoint,root,diagnostic))return false;
      }else root.layout=writer.Root().layout;
      ++count;
    }
    return true;
  }
  void DropFast(unsigned i) noexcept {if(fast_keys[i].local)exchange.RetireBody(fast_keys[i]);fast_keys[i]={};fast_addresses[i]=0;}
  void CancelCandidates() noexcept {
    if(ordinary.seal.read_serial)capture::Candidate(true,false);
    ordinary={};
    for(auto &body:bodies){if(body.seal.read_serial)capture::Candidate(false,false);body={};}
  }
  // 发布只借用当前主线程窗口；Source的字段读回调在同步Capture结束后不再有效。
  bool CandidateCurrent(const Pending &p) noexcept {
    return !prepare_revoked&&p.seal.read_serial&&p.seal.resource_mutation==resource_mutation&&p.seal.roots==p.roots&&
      ForeignResourceCurrent(p.seal.foreign_resource)&&capture.collector->NativeWindowCurrent(p.window);
  }
  bool Publish(Pending &p,CaptureDiagnostic &diagnostic) noexcept {
    if(!CandidateCurrent(p))return false;
    auto *writer=p.ticket.Values();if(!writer)return false;
    reading_window=p.window;reading_witness=p.witness;reading_resource_mutation=p.resource_change;collecting=true;
    const auto resources=source.Resources();
    if(!writer->Catalog({0,std::uint32_t(resources.entries.size()),std::uint32_t(resources.bytes.size()),std::uint32_t(resources.layers.size())},
        std::span(links).first(link_used))){diagnostic=writer->Diagnostic();collecting=false;return false;}
    struct Publication {Impl &source;Pending &pending;} publication{*this,p};
    CaptureWindow window{producer,p.witness,&publication,
      [](void *context,const CaptureWitness &w,const CaptureRoot &root) noexcept {auto &publication=*static_cast<Publication *>(context);
        return !publication.source.prepare_revoked&&ForeignResourceCurrent(publication.pending.seal.foreign_resource)&&
          root.subject==publication.pending.facts.subject&&root.address==publication.pending.facts.address&&publication.source.Current(w,publication.pending.facts);},nullptr};
    const auto packet=writer->Completed({});
    const bool result=p.ticket.Publish(window,p.anchor,p.roots);collecting=false;
    if(result){stats.roots+=packet.records.size();stats.cells+=packet.cells.size();stats.paths+=packet.paths.size();stats.bytes+=packet.bytes.size();}
    else diagnostic=writer->Diagnostic();
    return result;
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
  s.CancelCandidates();s.EndRead();s.fast_keys={};s.fast_addresses={};s.last_ordinary=0;s.published_count=0;s.source.Invalidate();
  s.source.ClearResourceCaches();
  s.source.ConfigureSharedResourceHooks(active?StreamSharedResourceHooks():0u);
  s.production_depth=0;
  s.display.Reset(true);
  s.display_run=0;
  s.publication->InvalidateBefore(s.revision==UINT64_MAX?UINT64_MAX:s.revision+1);
  s.invalidated=false;if(active)s.stats.active=s.exchange.Begin();else {s.exchange.Stop();s.stats.active=false;}}
void BinaryEntities::FrameBegin() noexcept {auto &s=*impl_;if(!s.Main())return;s.CancelCandidates();s.EndRead();s.ordinary_count=0;s.link_used=0;
  s.DrainEventReceipts();
  if(s.stats.active && s.capture.collector && ((s.run && s.run!=s.capture.collector->CurrentRunToken()) ||
      (s.room && s.room!=s.capture.collector->CurrentRoomToken()))){
    for(std::uint32_t i=0;i<s.scene_root_count;++i)s.source.Retire(s.scene_roots[i].subject);
    s.scene_root_count=0;
    if(s.run && s.run!=s.capture.collector->CurrentRunToken()){if(s.display_run!=s.capture.collector->CurrentRunToken())s.display.Reset(true);}
    Invalidate();}}
void BinaryEntities::FrameEnd() noexcept {auto &s=*impl_;if(!s.Main())return;
  const bool pending=s.ordinary.seal.read_serial||std::any_of(s.bodies.begin(),s.bodies.end(),[](const auto &body){return body.seal.read_serial!=0;});
  s.CancelCandidates();s.EndRead();if(pending)s.Failure({CaptureFailure::Incomplete});}
bool BinaryEntities::Prepare(bool normal,std::uint64_t completed_tick) noexcept {
  auto &s=*impl_;if(!s.Main() || !s.stats.active || !normal || !s.capture.collector)return false;
  if(s.preparing||s.collecting){s.prepare_revoked=true;s.reads.Revoke();s.Failure({CaptureFailure::Reentrant});return false;}
  s.preparing=true;s.prepare_revoked=false;
  struct EndPrepare {Impl &owner;~EndPrepare(){owner.EndRead();owner.prepare_reading=false;owner.preparing=false;}} end_prepare{s};
  s.root_used=0;
  CaptureDiagnostic group_failure;
  if(!s.Groups(group_failure)){world::NativeCaptureWindow unavailable;if(s.capture.collector->NativeOrdinary(unavailable))capture::Candidate(true,false);
    if(s.ordinary_failures<StateBudgets().capture_failure_limit)++s.ordinary_failures;
    s.Failure(group_failure);return false;}
  // 只有真实 PlayerPost 在 getter 边界之后且同一 ordinary 只读段内成立，才允许快槽克隆该身体的值。
  struct OrdinaryBody {world::NativeCaptureWindow window;world::PlayerPostWitness post;std::uint32_t record=UINT32_MAX;};
  std::array<OrdinaryBody,world::kControlAnchorCapacity> ordinary_bodies{};
  world::NativeCaptureWindow window;
  if(s.capture.collector->NativeOrdinary(window,completed_tick)){
    if((s.run && s.run!=window.run) || (s.room && s.room!=window.room))Invalidate();s.run=window.run;s.room=window.room;
    auto &p=s.ordinary;p.window=window;p.resource_change=s.resource_mutation;p.witness=s.Witness(window,CapturePhase::Ordinary);
    CaptureFailure reason;
    CaptureDiagnostic diagnostic;
    bool success=false;
    {
      capture::Cost cost(capture::CostPart::ScenePrepare);
      // 概率仍走原有具名 native owner。getter 可能重入，先调用再重新核真实根与水位。
      BinaryDisplayInputs inputs;world::NativeCaptureWindow after;
      success=QueryBinaryDisplayInputs(reinterpret_cast<void *>(window.game),reinterpret_cast<void *>(window.room_address),inputs)&&!s.prepare_revoked&&
        s.capture.collector->NativeOrdinary(after,completed_tick)&&after==window&&p.window==window&&p.resource_change==s.resource_mutation&&s.ReadWindow(p)&&s.SceneRead(false);
      if(success){
        p.witness.scope.floor=s.floor;p.witness.scope.dimension=s.floor*3+s.scene.dimension+1;s.reading_witness=p.witness;
        for(unsigned slot=0;slot<ordinary_bodies.size();++slot){auto &shared=ordinary_bodies[slot];
          s.capture.collector->NativeBodyInOrdinary(slot,p.window,shared.window,shared.post);}
        success=s.Seed()&&s.SceneSeed(inputs,diagnostic);
      }
    }
    if(!success)s.Failure(diagnostic.reason==CaptureFailure::None?CaptureDiagnostic{CaptureFailure::MissingQualification,{},276}:diagnostic);
    if(success){p.ticket=s.exchange.Acquire(CaptureLane::Ordinary,p.witness,{},reason);success=p.ticket.Values()!=nullptr;
      if(!success)s.Failure({reason});}
    if(success){auto &writer=*p.ticket.Values();
      // 后台可能已跳过中间普通槽。只复用它已实际发布的最后一份值。
      auto prior=s.publication->Read().ordinary;
      if(prior&&(prior->ordinary.revision!=s.last_ordinary||prior->ordinary.scope!=p.witness.scope||
                 prior->ordinary.generation!=p.witness.generation))prior.reset();
      {
        capture::Cost cost(capture::CostPart::EntityValues);
        // 采集实体失败只回卷当前根的两个流，成员集合仍由同一窗口确认。
        for(std::uint32_t i=0;i<s.root_used&&success;++i){
          auto &facts=s.roots[i];
          // 转场不产生新的 PlayerPost。只允许当前名册内已见证、同一 use/shape 的 Player 继续提供显示。
          if(window.room_transition&&facts.roster!=kNullIndex&&!facts.ready)
            if(const auto *id=s.capture.identities.Find(world::ObjectDomain::Entity,facts.address);
               id&&s.capture.collector->NativeTransitionPlayer(window,*id)){
              // 原生名册成员仍是已初始化的显示载体。输入资格仍由当前绑定单独计算。
              facts.ready=facts.player_ready=facts.activation_qualified=true;
            }
          if(s.Register(facts.subject).kind==CapturedReferenceKind::Invalid){
            success=FailCapture(diagnostic,CaptureFailure::ReferencePending,{facts.subject});break;
          }
          const auto first=writer.WrittenRoots();
          const auto checkpoint=writer.Checkpoint();
          std::uint16_t layout=facts.layout;
          std::uint64_t reuse=0;
          if(prior){
            const PresentationMember key{facts.subject.space,host_field::EntityTear::kLayout,facts.subject.local,
              facts.subject.use,facts.subject.shape+1};
            if(std::binary_search(prior->reusable_roots.begin(),prior->reusable_roots.end(),key))reuse=prior->ordinary.revision;
          }
          bool captured=facts.capture_failure==CaptureFailure::None&&
            s.source.CaptureOrdinary(p.witness,facts,writer,diagnostic,layout,reuse);
#ifdef ISAACCS_E2E
          // 让单颗已存在泪弹走真实回卷与上一值保留路径，其他根继续采集。
          if(captured&&layout==host_field::EntityTear::kLayout&&HostTearReadTestFault())
            captured=FailCapture(diagnostic,CaptureFailure::ReadFailed,{facts.subject,facts.address,0,layout});
#endif
          if(!captured){
            facts.layout=layout;
            // 新房的首次 World 不能沿用上一房的 Player 记录补齐名册。
            if(window.room_transition&&facts.roster!=kNullIndex){success=false;break;}
            if(!s.RetainFailedRoot(writer,checkpoint,facts,diagnostic)){success=false;break;}
            ++p.roots;continue;
          }
          facts.subject=writer.Root().subject;facts.layout=writer.Root().layout;p.roots+=writer.WrittenRoots()-first;
          if(facts.layout!=197)continue;
          for(auto &body:ordinary_bodies)if(body.post.anchor.player==facts.address)body.record=first+1;
        }
      }
      if(success)success=s.SceneInto(writer,p.roots,diagnostic);
      if(success){
        // 整包发布以 Room 为证言，独立于常规实体根的失败。
        const auto room=std::find_if(s.scene_roots.begin(),s.scene_roots.begin()+s.scene_root_count,
          [](const auto &root){return root.subject.space==7;});
        if(room==s.scene_roots.begin()+s.scene_root_count)success=false;
        else {p.facts=*room;p.anchor={room->subject,room->address,0,room->layout};}
      }
      capture::Cost review(capture::CostPart::Review);
      if(success)success=s.GroupsInto(writer,p.roots);
      if(success)success=s.ProductionInto(writer,p.roots,diagnostic);
      if(success)success=s.Members(true);
      if(success)success=s.SceneRead(true);
      if(success)success=s.Seal(p);
      if(!success && diagnostic.reason==CaptureFailure::None && writer.Diagnostic().reason==CaptureFailure::None)diagnostic.reason=CaptureFailure::SourceChanged;
      if(!success){
        auto failure=diagnostic.reason==CaptureFailure::None?writer.Diagnostic():diagnostic;
        if(failure.subject.local&&!failure.layout){
          const auto end=s.roots.begin()+s.root_used;
          const auto root=std::find_if(s.roots.begin(),end,[&](const auto &facts){return facts.subject.local==failure.subject.local&&facts.subject.space==failure.subject.space;});
          if(root!=end){
            failure.layout=root->layout;
#ifdef ISAACCS_E2E
            // 短时实体可在恢复前消失。记录首个被拒的身份及当前已知原生字段，便于与同帧绘制对照。
            if(failure.reason==CaptureFailure::MissingQualification){
              const auto *id=s.capture.identities.Find(world::ObjectDomain::Entity,root->address);
              std::array<std::uint32_t,3> shape{};std::uint32_t seed=0;
              if(id&&!id->Retired()&&id->local_id==root->subject.local&&id->use==root->subject.use&&
                 s.NativeValue(std::uint64_t(root->address)+0x28,shape)&&s.NativeValue(std::uint64_t(root->address)+0x3ec,seed)){
                std::snprintf(failure.detail.data(),failure.detail.size(),"entity=%u/%u/%u seed=%u ready=%u candidate=%u birth_kind=%u",
                  shape[0],shape[1],shape[2],seed,id->readiness,unsigned(id->candidate),id->birth_kind);
                if(s.last_unready_entity!=root->subject){
                  s.last_unready_entity=root->subject;
                  Trace("binary_entity_readiness_e2e_fault_probe","frame=%u %s owner=%llu published=%u life=%u",
                    p.witness.game_frame,failure.detail.data(),static_cast<unsigned long long>(id->owner_scope),unsigned(id->published),id->lifecycle);
                }
              }
            }
#endif
          }
        }
        s.Failure(failure);
      }
    }
    if(!success||!p.anchor.address){
      if(s.ordinary_failures<StateBudgets().capture_failure_limit)++s.ordinary_failures;
      p={};capture::Candidate(true,false);
    }else s.ordinary_count=s.root_used;
    s.EndRead();
  }
  // 快速槽仅服务真实当前PlayerPost。其余身体已经通过 ordinary 同根保留，绝不截断成员。
  capture::Cost fast(capture::CostPart::FastBody);
  for(unsigned slot=0;slot<world::kControlAnchorCapacity;++slot){world::PlayerPostWitness body;
    if(!s.capture.collector->NativeBody(slot,window,body))continue;
    using Reuse=capture::BodyReuse;
    if(window.run!=s.run||window.room!=s.room){capture::CountBodyReuse(Reuse::Window);capture::Candidate(false,false);Invalidate();continue;}
    const auto *root=s.Add(body.anchor.player);
    if(!root||!root->ready||!root->group_known||root->capture_failure!=CaptureFailure::None){
      capture::CountBodyReuse(Reuse::Unqualified);++s.stats.body_skipped;capture::Candidate(false,false);continue;
    }
    // 快槽补充 PlayerPost 的绑定见证，不改动 ordinary 给提交段留下的根事实。
    auto f=*root;
    f.subject.binding=body.anchor.generation;f.subject.topology=s.source.Topology(f.subject);
    unsigned lane=unsigned(s.fast_keys.size());
    for(unsigned i=0;i<s.fast_keys.size();++i)if(s.fast_addresses[i]==f.address){lane=i;if(s.fast_keys[i]!=f.subject)s.DropFast(i);break;}
    if(lane==s.fast_keys.size())for(unsigned i=0;i<s.fast_keys.size();++i)if(!s.fast_keys[i].local){lane=i;break;}
    if(lane==s.fast_keys.size()){capture::CountBodyReuse(Reuse::NoLane);capture::Candidate(false,false);continue;}
    const auto &shared=ordinary_bodies[slot];const auto &ordinary=s.ordinary;auto *ordinary_writer=s.ordinary.ticket.Values();
    const auto packet=ordinary_writer?ordinary_writer->Completed({}):CapturePacket{};
    const auto *record=shared.record<packet.records.size()?&packet.records[shared.record]:nullptr;
    const bool same_topology=ordinary.seal.read_serial&&record&&record->subject==f.subject;
    Reuse reuse=Reuse::Match;
    if(!ordinary.seal.read_serial)reuse=Reuse::NoOrdinary;
    else if(!record)reuse=Reuse::NoSharedBody;
    else if(!same_topology)reuse=Reuse::Subject;
    else if(shared.window!=window||shared.post!=body||window.game_frame!=ordinary.window.game_frame)reuse=Reuse::Window;
    else if(!record->fast_body&&!record->flat_bytes)reuse=Reuse::TooLarge;
    else if(!s.OrdinaryBodySourceCurrent(ordinary))reuse=Reuse::Stale;
    capture::CountBodyReuse(reuse);
    // 同页表并不延长某一身体的 World 结构资格。超出快槽预算时直接省略本拍。
    if(reuse==Reuse::TooLarge||reuse==Reuse::Stale||(!same_topology&&!s.last_ordinary)||!f.subject.topology){
      ++s.stats.body_skipped;capture::Candidate(false,false);continue;
    }
    auto &p=s.bodies[lane];p.window=window;p.player_post=body;p.witness=s.Witness(window,CapturePhase::PlayerPost);p.resource_change=s.resource_mutation;p.facts=f;
    if(same_topology){p.witness.scope=s.ordinary.witness.scope;p.witness.required_revision=s.ordinary.witness.revision;}
    CaptureFailure reason;p.ticket=s.exchange.Acquire(CaptureLane::Body,p.witness,f.subject,reason);
    auto *writer=p.ticket.Values();
    if(!writer){s.Failure({reason,f.subject});capture::Candidate(false,false);p={};continue;}
    // Acquire 已登记快槽身份，即使本拍无正文也保留该槽供同一身体续用。
    s.fast_keys[lane]=f.subject;s.fast_addresses[lane]=f.address;
    CaptureDiagnostic diagnostic;bool success=false;
    // 平铺 Body 按本次 PlayerPost 证言采集，目录句柄属于当前窗口。
    if(reuse==Reuse::Match&&!record->flat_bytes){
      capture::Cost clone(capture::CostPart::BodyClone);
      capture::BodyCloneCounts work;
      if(!s.OrdinaryBodySourceCurrent(ordinary)||s.ordinary.ticket.Values()!=ordinary_writer)
        FailCapture(diagnostic,CaptureFailure::Revoked,{f.subject});
      else success=writer->CloneOrdinaryBody(*ordinary_writer,shared.record,f.subject,197,
        std::span(s.links).first(ordinary.seal.references),{&s,[](void *owner,const CaptureReferenceEntry &ref) noexcept {
          return static_cast<Impl *>(owner)->Register({ref.space,ref.local,ref.use});}},work);
      capture::CountBodyClone(success,work);
      if(success){p.seal=ordinary.seal;p.seal.roots=1;++s.stats.body_reused;}
    }else if(s.ReadWindow(p)){
      p.roots=1;
      success=s.BodyShapeCurrent(f,diagnostic)&&
        s.source.Capture(p.witness,f,BodyStream::Body,*writer,diagnostic,false,same_topology?f.subject.topology:0)&&s.Seal(p);
      s.EndRead();
    }
    if(!success){
      if(diagnostic.reason==CaptureFailure::None)diagnostic=writer->Diagnostic();
      if(diagnostic.reason==CaptureFailure::SourceChanged||diagnostic.reason==CaptureFailure::ValueBudget){
        if(diagnostic.reason==CaptureFailure::SourceChanged)++s.stats.source_changed;
        ++s.stats.body_skipped;
      }else s.Failure(diagnostic.reason==CaptureFailure::None?CaptureDiagnostic{CaptureFailure::SourceChanged,f.subject}:diagnostic);
      capture::Candidate(false,false);p={};continue;}
    f.subject=p.ticket.Values()->Root().subject;p.facts=f;p.anchor={f.subject,f.address,0,197};p.roots=1;
    s.fast_keys[lane]=f.subject;s.fast_addresses[lane]=f.address;
  }
  return s.ordinary.seal.read_serial!=0;
}
void BinaryEntities::CommitTransition() noexcept {
  // Manager 已正常返回，且这一窗口没有待提交的玩法 tick 或 PlayerPost。
  if(impl_->ordinary.window.room_transition)Commit(false,{});
}
void BinaryEntities::Commit(bool committed,const core::TickPostState &post) noexcept {
  auto &s=*impl_;if(!s.Main() || !s.stats.active)return;
  capture::Cost cost(capture::CostPart::Commit);
  if(!s.capture.collector || !s.capture.collector->Recording()){Invalidate();return;}
  const bool match=committed && s.capture.collector && s.capture.collector->NativePostMatches(post);
  auto &ordinary=s.ordinary;
  const auto ordinary_revision=ordinary.witness.revision;
  if(ordinary.seal.read_serial){
    // Manager 尾部 Prepare 到时钟提交之间只处理纯值。实体、Grid 与场景根已在写完时复核；
    // 真实 Game／Room、Authority 窗口和资源水位仍需与封存值一致。
    world::NativeCaptureWindow current;
    const bool transition=ordinary.window.room_transition;
    bool valid=(match||transition)&&s.capture.collector->NativeOrdinary(current,transition?ordinary.window.tick:0)&&current==ordinary.window;
    if(valid&&s.ordinary_failures>=StateBudgets().capture_failure_limit){
      if(auto *writer=ordinary.ticket.Values())writer->RequestBaseline();
      else valid=false;
    }
    CaptureDiagnostic diagnostic;
    const bool published=valid&&s.Publish(ordinary,diagnostic);capture::Candidate(true,published);
    if(published){
      s.invalidated=false;s.last_ordinary=ordinary.witness.revision;s.ordinary_failures=0;++s.stats.ordinary;
      s.capture.collector->CommitNativeMemberships(ordinary.window,s.scene.sources,s.scene.members,s.scene.grids);
#ifdef ISAACCS_E2E
      if(replica::stream::E2ETraceEnabled()){
      BinaryBloodEffects effects;
      for(std::uint32_t i=0;i<s.ordinary_count;++i){
        if(const auto *id=s.capture.identities.Find(world::ObjectDomain::Entity,s.roots[i].address);id&&id->native_shape_known)
          effects.Observe(id->native_shape[0],id->native_shape[1]);
      }
      effects.TraceChange("host",ordinary.witness.game_frame,s.committed_blood_effects);
      s.committed_blood_effects=effects;
      }
#endif
      s.RememberPublished();
      for(std::uint32_t i=0;i<s.ordinary_count;++i){
        const auto &root=s.roots[i];
        if(root.layout==197&&root.capture_failure==CaptureFailure::None)s.source.OrdinaryCommitted(root.subject);
      }
    }else {
      if(s.ordinary_failures<StateBudgets().capture_failure_limit)++s.ordinary_failures;
      s.Failure(diagnostic.reason==CaptureFailure::None?CaptureDiagnostic{CaptureFailure::SourceChanged,ordinary.anchor.subject}:diagnostic);
    }
    ordinary={};
  }
  for(auto &p:s.bodies)if(p.seal.read_serial){
    const auto root=s.RootIndex(p.anchor.address);
    if(ordinary_revision&&p.witness.required_revision==ordinary_revision&&ordinary_revision==s.last_ordinary&&
       root<s.ordinary_count&&s.roots[root].capture_failure!=CaptureFailure::None){
      ++s.stats.body_skipped;capture::Candidate(false,false);p={};continue;
    }
    world::NativeCaptureWindow current;world::PlayerPostWitness body;CaptureDiagnostic diagnostic;
    const bool published=match&&p.witness.required_revision==s.last_ordinary&&
      s.capture.collector->NativeBody(p.window.body_slot,current,body)&&current==p.window&&body==p.player_post&&s.Publish(p,diagnostic);
    capture::Candidate(false,published);
    if(published)++s.stats.bodies;
    else s.Failure(diagnostic.reason==CaptureFailure::None?CaptureDiagnostic{CaptureFailure::SourceChanged,p.anchor.subject}:diagnostic);
    p={};
  }
  s.EndRead();
}
void BinaryEntities::Invalidate() noexcept {auto &s=*impl_;if(!s.Main() || !s.stats.active)return;
  s.reads.Revoke();
  s.CancelCandidates();s.ordinary_failures=0;s.last_ordinary=0;s.published_count=0;
  if(s.resource_mutation==UINT64_MAX){s.stats.active=false;s.exchange.Stop(CaptureFailure::SequenceExhausted);return;}
  ++s.resource_mutation;s.source.Invalidate();
  if(s.invalidated)return;s.invalidated=true;
  s.publication->InvalidateBefore(s.revision==UINT64_MAX?UINT64_MAX:s.revision+1);
  for(auto &key:s.fast_keys)if(key.local){s.exchange.RetireBody(key);key={};}s.fast_addresses={};s.exchange.Invalidate(CaptureFailure::Revoked);}
void BinaryEntities::RetireBody(std::uint32_t address) noexcept {auto &s=*impl_;if(!s.Main())return;
  if(address&&s.Known(address)){
    ++s.entity_invalidations;
    const auto *id=s.capture.identities.Find(world::ObjectDomain::Entity,address);
    s.last_entity_invalidation={address,id?id->local_id:0,id?id->use:0,id?id->lifecycle:0,
      id?id->readiness:0,std::uint64_t(id&&id->native_shape_known),id?id->native_shape[0]:0,id?id->birth_type:0};
    Invalidate();
  }
  for(unsigned i=0;i<s.fast_keys.size();++i)if(s.fast_addresses[i]==address && s.fast_keys[i].local)s.DropFast(i);
  if(const auto *id=s.capture.identities.Find(world::ObjectDomain::Entity,address);id&&id->Retired()){
    s.source.Retire({1,id->local_id,id->use});const auto end=s.texts.begin()+s.text_used;
    const auto text=std::lower_bound(s.texts.begin(),end,address,[](const auto &entry,std::uint32_t p){return entry.address<p;});
    if(text!=end&&text->address==address){std::move(text+1,end,text);--s.text_used;}}
}
void BinaryEntities::EntityChanged(std::uint32_t address) noexcept {auto &s=*impl_;if(!s.Main())return;
  const auto *id=s.capture.identities.Find(world::ObjectDomain::Entity,address);
  if(id&&id->Retired()&&id->native_shape_known&&
     std::find(s.fast_addresses.begin(),s.fast_addresses.end(),address)==s.fast_addresses.end()){
    const auto type=id->native_shape[0];
    if(type==2||type==9||type==1000){
      // Tear、Projectile、Effect 的正常退休只影响当前原生读取。上一帧已封存的值仍是合法历史，
      // 下一份普通快照提交真实移除。保留切房、Player、Grid 和资源切换原有撤销边界。
      if(s.collecting||s.prepare_reading||s.ordinary.seal.read_serial){s.reads.Revoke();s.prepare_revoked=true;}
      s.source.Retire({1,id->local_id,id->use});
      ++s.stats.transient_entity_retirements;
      return;
    }
  }
  RetireBody(address);}
void BinaryEntities::TextWritten(std::uint32_t address,std::uint64_t local,std::uint64_t use,std::uint64_t shape) noexcept {auto &s=*impl_;if(!s.Main())return;
  auto end=s.texts.begin()+s.text_used;auto where=std::lower_bound(s.texts.begin(),end,address,[](auto a,std::uint32_t p){return a.address<p;});
  if(where==end || where->address!=address){if(s.text_used==s.texts.size()){s.Failure({CaptureFailure::ValueBudget,{1,local,use,shape}});return;}std::move_backward(where,end,end+1);++s.text_used;}
  *where={address,local,use,shape};}
void BinaryEntities::ResourceChanged(std::uint32_t address,const char *operation) noexcept {auto &s=*impl_;if(!s.Main())return;
  if(s.collecting||s.prepare_reading||s.ordinary.seal.read_serial){s.reads.Revoke();s.prepare_revoked=true;}
  std::uint32_t owner=0;CaptureSubject subject;
  if(s.source.UsesSprite(address,owner,subject)){++s.sprite_invalidations;
    s.source.ForgetSpriteResources(subject);
    const auto *id=subject.space==1?s.capture.identities.Find(world::ObjectDomain::Entity,owner):nullptr;
    const bool obsolete=subject.space==1&&id&&(id->local_id!=subject.local||id->use!=subject.use);
    const bool transient=subject.space==1&&id&&id->local_id==subject.local&&id->use==subject.use&&
      id->native_shape_known&&(id->native_shape[0]==2||id->native_shape[0]==9||id->native_shape[0]==1000);
    if(obsolete||transient){
      // 泪弹/弹幕/特效会在存活时Reset，也会复用ANM地址；不能用活对象变化撤销所有历史帧。
      // 撤销本outer未提交的候选并移除旧依赖；下一次采集重读资源，后台只消费自有值及只追加目录。
      s.reads.Revoke();s.CancelCandidates();s.source.Retire(subject);
      if(s.resource_mutation==UINT64_MAX){s.stats.active=false;s.exchange.Stop(CaptureFailure::SequenceExhausted);return;}
      ++s.resource_mutation;++s.sprite_history_preserved;return;
    }
    s.last_sprite_owner={owner,subject.local,subject.use,id?id->local_id:0,id?id->use:0,id?id->lifecycle:0,
      id?id->readiness:0,std::uint64_t(id&&id->native_shape_known),id?id->native_shape[0]:0,id?id->birth_type:0};
    if(!operation)operation="";
    const auto length=std::min(std::strlen(operation),s.last_sprite_operation.size()-1);
    std::copy_n(operation,length,s.last_sprite_operation.begin());s.last_sprite_operation[length]=0;
    Invalidate();}
}
// 清扫只撤销受影响的共享层。正在采集的候选仍服从原读段撤销规则。
void BinaryEntities::SharedResourceChanged(bool animation) noexcept {
  auto &s=*impl_;if(!s.Main()||!s.stats.active)return;
  s.source.SharedResourceChanged(animation);
  const bool body_pending=std::any_of(s.bodies.begin(),s.bodies.end(),[](const auto &p){return p.seal.read_serial!=0;});
  if(s.collecting||s.prepare_reading||s.ordinary.seal.read_serial||body_pending){
    s.reads.Revoke();s.prepare_revoked=true;s.CancelCandidates();
  }
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
BinaryEntityStats BinaryEntities::Statistics() const noexcept {auto &s=*impl_;auto out=s.stats;out.exchange=s.exchange.Statistics();out.events=s.display.Statistics();out.reads=s.reads.Statistics();out.native=s.source.Statistics();
  out.member_indices=s.publisher.MemberIndexStats();
  // 显示生产首因会持续阻断普通快照，不能被后到的独立 Body 拒绝覆盖。
  if(const auto first=s.display.FirstFailure();first.reason!=CaptureFailure::None)out.failure=first;
  if(const auto published=s.publication->Read();published.latest){
    out.publication_revision=published.latest->witness.revision;
    out.retained_roots=std::uint32_t(published.latest->retained_members.size());
    for(const auto &root:published.latest->retained_members)
      out.max_retained_frames=(std::max)(out.max_retained_frames,root.count);
  }
  std::unique_lock lock(s.publish_status,std::try_to_lock);if(lock)out.publication_failure=s.publish_failure;return out;}
std::string BinaryEntities::WorkerFields(unsigned page) const {
  const auto &s=*impl_;auto out=s.worker_metrics.Read().Fields("stream.capture_worker.",page);
  out+="stream.capture_worker.flat_padding_failures="+std::to_string(s.publisher.FlatPaddingFailures())+"\n";
  out+="stream.source.entity_invalidations="+std::to_string(s.entity_invalidations)+"\n";
  out+="stream.source.sprite_invalidations="+std::to_string(s.sprite_invalidations)+"\n";
  out+="stream.source.sprite_history_preserved="+std::to_string(s.sprite_history_preserved)+"\n";
  out+="stream.source.last_sprite_operation="+std::string(s.last_sprite_operation.data())+"\n";
  constexpr const char *sprite_names[]={"address","local","use","current_local","current_use","lifecycle","readiness","shape_known","type","birth_type"};
  for(unsigned i=0;i<s.last_sprite_owner.size();++i)
    out+="stream.source.last_sprite_owner."+std::string(sprite_names[i])+"="+std::to_string(s.last_sprite_owner[i])+"\n";
  constexpr const char *names[]={"address","local","use","lifecycle","readiness","shape_known","type","birth_type"};
  for(unsigned i=0;i<s.last_entity_invalidation.size();++i)
    out+="stream.source.last_entity_invalidation."+std::string(names[i])+"="+std::to_string(s.last_entity_invalidation[i])+"\n";
  return out;
}
// 先建立真实生产窗口与有界调用栈；暂停显示允许tick=0，不能借用上一次Authority tick。
std::uint32_t BinaryEntities::ProductionBegin(BinaryProductionKind kind,std::uint32_t object,std::uint32_t manager) noexcept {
  auto &s=*impl_;if(!s.Main() || !s.stats.active || !s.capture.collector || !object)return 0;
  // 每日挑战分数缓存已按用户要求退出同步；常驻 Hook 的原生调用和 Client 隔离保持。
  if(kind==BinaryProductionKind::Score)return 0;
  if(s.collecting||s.prepare_reading){s.reads.Revoke();s.prepare_revoked=true;s.Failure({CaptureFailure::Reentrant});return 0;}
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
    if(!s.ReadProduction(p)){s.Failure({CaptureFailure::Reentrant});return;}
    if(p.kind==BinaryProductionKind::SfxPlay)p.frame_delay=std::bit_cast<std::int32_t>(args[0]);
    if(p.kind>=BinaryProductionKind::MusicPlay&&p.kind<=BinaryProductionKind::MusicLayerOff)
      p.display_failed=!s.NativeValue(std::uint64_t(p.object)+0x30c,p.args[5]);
    if(p.kind==BinaryProductionKind::Fortune){p.args[1]=0;p.args[2]=UINT32_MAX;
      for(unsigned parent=i-1;parent;--parent){const auto &outer=s.producing[parent-1];if(outer.kind==BinaryProductionKind::FortuneSource){p.args[1]=outer.args[0];p.args[2]=outer.args[2];break;}}}
    if(p.kind==BinaryProductionKind::FortuneSource)p.args[2]=UINT32_MAX;
    if(p.kind==BinaryProductionKind::TransitionStart){const auto serial=s.display.ReserveTransition();
      p.args[6]=std::uint32_t(serial);p.args[7]=std::uint32_t(serial>>32);
      p.display_failed=!serial||!s.NativeValue(std::uint64_t(p.window.game)+0x1ba78,p.args[5]);
    }
    if(const auto address=s.DisplayPlayerAddress(p)){
      if(const auto *id=s.capture.identities.Find(world::ObjectDomain::Entity,address)){
        p.player_entry_flags=1u|(id->Retired()?2u:0u)|(id->candidate?4u:0u)|(id->readiness<<8);
        if(s.DisplayPlayerIdentity(*id))p.event_player={1,id->local_id,id->use};
      }
    }
    if(p.kind==BinaryProductionKind::BossIntro)for(unsigned parent=i-1;parent;--parent){const auto &outer=s.producing[parent-1];
      if(outer.kind==BinaryProductionKind::TransitionStart){p.args[6]=outer.args[6];p.args[7]=outer.args[7];break;}}
    if(p.kind==BinaryProductionKind::MapCache)p.display_failed=!s.NativeValue(std::uint64_t(p.window.room_address)+4,p.args[0]);
    s.EndRead();return;}
}
void BinaryEntities::ProductionRandom(std::uint32_t token,std::uint32_t value) noexcept {
  auto &s=*impl_;if(!s.Main())return;
  for(unsigned i=s.production_depth;i;--i){auto &p=s.producing[i-1];if(p.token!=token)continue;
    if(p.kind==BinaryProductionKind::FortuneSource){p.args[2]=value;return;}
    if(p.kind==BinaryProductionKind::MapCache){if(!s.ReadProduction(p)){s.Failure({CaptureFailure::Reentrant});return;}std::uint32_t count=0,focus=0;
      if(s.NativeValue(std::uint64_t(p.window.game)+0x182cc,count)&&value<count&&count<=527&&
         s.NativeValue(std::uint64_t(p.window.game)+36+std::uint64_t(value)*184+4,focus))
        s.display.MapFocus(p.object,p.args[0],p.window.game_frame,focus);
      else p.display_failed=true;s.EndRead();return;}}
}
// 捕获正常LoadText实际消费的最终UTF16，再有界转UTF8；不额外本地化或重跑文字随机。
void BinaryEntities::ProductionBannerText(std::uint32_t token,std::uint32_t text,bool subtitle) noexcept {
  auto &s=*impl_;if(!s.Main())return;
  for(unsigned i=s.production_depth;i;--i){auto &p=s.producing[i-1];if(p.token!=token||p.kind!=BinaryProductionKind::ItemBanner)continue;
    if(!s.ReadProduction(p)){s.Failure({CaptureFailure::Reentrant});return;}std::array<wchar_t,4096> value{};unsigned count=0;
    if(text)for(;count<value.size()-1;++count){if(!s.NativeValue(std::uint64_t(text)+count*2,value[count])){p.display_failed=true;break;}if(!value[count])break;}
    if(count==value.size()-1)p.display_failed=true;
    auto &out=subtitle?p.subtitle:p.title;auto &seen=subtitle?p.subtitle_seen:p.title_seen;
    if(seen)p.display_failed=true;
    const int size=count?WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value.data(),int(count),out.data(),int(out.size()-1),nullptr,nullptr):0;
    if(count&&!size)p.display_failed=true;else {out[std::size_t(size)]=0;seen=true;}
    s.EndRead();return;}
}
// 9F7970是正常选样本后的共同尾叶：即使本地source为空也会到达，不能以Play后端成功分配世界实例。
void BinaryEntities::ProductionAudioPan(std::uint32_t token,std::uint32_t stream) noexcept {auto &s=*impl_;if(!s.Main())return;
  for(unsigned i=s.production_depth;i;--i){auto &p=s.producing[i-1];if(p.token!=token)continue;if(p.kind!=BinaryProductionKind::SfxPlay)return;
    if(!s.ReadProduction(p)){s.Failure({CaptureFailure::Reentrant});return;}std::array<std::uint32_t,3> table{};std::uint32_t samples=0;
    bool valid=p.sound>=0 && stream>=24 && s.NativeValue(p.object,table) && table[0]<=table[1] && table[1]<=table[2] &&
      (table[1]-table[0])%440==0 && std::uint32_t(p.sound)<(table[1]-table[0])/440;
    const auto effect=std::uint64_t(table[0])+std::uint64_t(std::uint32_t(p.sound))*440;
    valid=valid && s.NativeValue(effect+400,samples) && samples>0 && samples<=7 && stream>=effect+32 &&
      (stream-effect-32)%56==0 && (stream-effect-32)/56<samples;
    if(valid){p.stream=stream;++p.selected;}else p.display_failed=true;
    s.EndRead();return;}}
// 正常返回才封存；所有失败阻断新完整显示目标，不改变Authority输入或原生调用次数。
void BinaryEntities::ProductionEnd(std::uint32_t token,bool normal) noexcept {
  auto &s=*impl_;if(!s.Main() || !s.production_depth)return;auto p=s.producing[s.production_depth-1];
  if(p.token!=token){s.Failure({CaptureFailure::Reentrant});s.production_depth=0;return;}
  --s.production_depth;
  if(normal&&p.kind==BinaryProductionKind::TransitionFinish){
    // ChangeRoom的正常完成会退休旧room；重新取得真实新room生产窗，不沿用旧凭据。
    if(s.capture.collector->NativeProduction(p.window,p.token))p.witness=s.Witness(p.window,CapturePhase::Production);else normal=false;
  }
  if(!normal || !s.capture.collector->NativeWindowCurrent(p.window)){
    if(!normal)Invalidate();
    s.Failure({CaptureFailure::SourceChanged});return;}
  if(s.revision==UINT64_MAX){s.Failure({CaptureFailure::SequenceExhausted});return;}p.witness.revision=++s.revision;
  if(!s.ReadProduction(p)){s.Failure({CaptureFailure::Reentrant});return;}
  if(p.kind>=BinaryProductionKind::MusicPlay||p.kind>=BinaryProductionKind::SfxPlay&&p.kind<=BinaryProductionKind::SfxAmbient){
    bool nested_music=false;
    if(p.kind>=BinaryProductionKind::MusicPlay&&p.kind<=BinaryProductionKind::MusicLayerOff)
      for(unsigned i=0;i<s.production_depth;++i)nested_music|=s.producing[i].kind>=BinaryProductionKind::MusicPlay&&s.producing[i].kind<=BinaryProductionKind::MusicLayerOff;
    if(nested_music){s.EndRead();return;}
    EntityFacts facts;facts.subject={3,p.window.run,1,1};facts.address=p.window.game;facts.ready=true;facts.manager=p.manager;
    CaptureDiagnostic diagnostic;bool ok=!p.display_failed;
    if(p.kind==BinaryProductionKind::SfxPlay){if(p.audio_world&&p.selected){ok=ok&&p.selected==1;
      if(ok)BinaryNativeSfxFallback(p.kind,reinterpret_cast<void *>(p.object),p.sound,p.volume,p.frame_delay,p.loop,p.pitch,p.pan,reinterpret_cast<void *>(p.stream),p.ambient);} }
    else if(p.kind>=BinaryProductionKind::SfxVolume&&p.kind<=BinaryProductionKind::SfxAmbient){if(p.audio_world){p.args[0]=std::uint32_t(p.sound);p.args[1]=std::bit_cast<std::uint32_t>(p.volume);p.args[2]=std::bit_cast<std::uint32_t>(p.pitch);
        if(ok)BinaryNativeSfxFallback(p.kind,reinterpret_cast<void *>(p.object),p.sound,p.volume,0,false,p.pitch,0,nullptr,-1);}}
    else {if(p.kind==BinaryProductionKind::ItemBanner){p.args[6]=p.title_seen?world::PackWord(p.title.data()):0;p.args[7]=p.subtitle_seen?world::PackWord(p.subtitle.data()):0;}
      if(const auto address=s.DisplayPlayerAddress(p)){
        const auto *id=s.capture.identities.Find(world::ObjectDomain::Entity,address);
        if(!id||id->Retired()||id->local_id!=p.event_player.local||id->use!=p.event_player.use){
          // 只在实际身份复核失败时格式化，保留入口与返回差别，避免继续猜测是读取资格撤销。
          const auto flags=id?1u|(id->Retired()?2u:0u)|(id->candidate?4u:0u)|(id->readiness<<8):0u;
          std::snprintf(diagnostic.detail.data(),diagnostic.detail.size(),
            "production=%u player=%08x entry=%x exit=%x saved=%llu/%llu now=%llu/%llu",
            unsigned(p.kind),address,p.player_entry_flags,flags,
            static_cast<unsigned long long>(p.event_player.local),static_cast<unsigned long long>(p.event_player.use),
            static_cast<unsigned long long>(id?id->local_id:0),static_cast<unsigned long long>(id?id->use:0));
          p.event_player={};
        }
      }
      ok=ok&&s.display.Observe(p.kind,p.object,p.args,p.witness,facts,s.source,s.DisplayRead(),std::span(s.links).first(s.link_used),diagnostic,&p.event_player);}
    if(!ok){
      if(diagnostic.reason==CaptureFailure::None)FailCapture(diagnostic,CaptureFailure::MissingQualification,{facts.subject});
      // 只在失败时附具名生产入口信息；正常采集不格式化逐帧日志。
      if(!diagnostic.detail[0])std::snprintf(diagnostic.detail.data(),diagnostic.detail.size(),
        "production_kind=%u before_observe=%u",unsigned(p.kind),unsigned(p.display_failed));
      s.display.Fault(diagnostic);s.Failure(diagnostic);
    }
    s.EndRead();return;
  }
  s.EndRead();
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
  const auto withdraw=[&]{s.transferring.Reset();s.transfer_source.reset();s.publisher.Reset();s.publication->Clear();};
  CaptureControl control;while(s.exchange.PopControl(control)){
    if(control.kind==CaptureControlKind::Begin||control.kind==CaptureControlKind::Stop){withdraw();}
    else if(control.kind==CaptureControlKind::Invalidate){
      s.transferring.Reset();s.transfer_source.reset();s.publication->Clear();
    }else if(control.kind==CaptureControlKind::RetireBody){
      // 只退出快照上叠加的 Body。ordinary 和正在交接的事件保持可用。
      s.publisher.DiscardBody();s.publication->DiscardBody();
    }
  }
  if(stopping){withdraw();for(;;){auto ticket=s.exchange.Take();if(!ticket.Packet())break;}return;}
  if(s.transferring.Packet()){
    // 独立观测／Recorder没有网络订阅者；空Peer集合的接管在本consumer完成，避免卡住快槽。
    if(!s.network_publication.load(std::memory_order_acquire))s.transfer_source->events_transferred.store(1,std::memory_order_release);
    if(!s.transferring.Current()){s.transferring.Reset();s.transfer_source.reset();}
    else if(!s.transfer_source->events_transferred.load(std::memory_order_acquire)||!EventsTransferred(s.transferring))return;
    else {s.transferring.Reset();s.transfer_source.reset();}
  }
  for(unsigned n=0;n<9;++n){const auto latest=s.publisher.Latest();
    auto ticket=s.exchange.Take(latest?latest->ordinary.revision:0);const auto *packet=ticket.Packet();if(!packet)break;
    s.worker_samples.duration[unsigned(replica::stream::CpuStage::CaptureQueueAge)].Observe(ticket.QueuedMicros());
    // 邮箱只复制统计值，worker 不访问任何采集段内存。
    struct MetricsSample {Impl &source;~MetricsSample(){auto &value=source.worker_samples;
      value.lane_packets[0]=source.worker_ordinary;value.lane_packets[1]=source.worker_body;
      value.stale_rejections=source.worker_failures;source.worker_metrics.Publish(value);}} sample{s};
    try {const auto catalog=Catalog(ticket);const auto &scope=packet->witness.scope;
      if(scope.session!=s.resource_session||scope.run!=s.resource_run){s.published_resources.clear();s.resource_session=scope.session;s.resource_run=scope.run;}
      auto resources=world::binary::ExportStateResources(catalog.resources,s.published_resources);
      auto value=s.publisher.Capture(*packet,resources,&ticket,[](void *p) noexcept {return static_cast<CaptureExchange::ReadTicket *>(p)->Current();},
        {&s,[](void *owner,HostCapturePhase phase,bool entering) noexcept {
          auto &source=*static_cast<Impl *>(owner);
          if(entering)source.worker_sample.emplace(source.worker_samples,phase==HostCapturePhase::Index?
            replica::stream::CpuStage::CaptureIndex:replica::stream::CpuStage::Encode);
          else source.worker_sample.reset();
        }});if(!ticket.Current())continue;
      if(!value)continue;
      s.published_resources=std::move(resources);
      s.publication->Publish(value,packet->witness.phase==CapturePhase::Ordinary);
#ifdef ISAACCS_E2E
      E09Host(*value,*packet);
#endif
      {std::lock_guard lock(s.publish_status);s.publish_failure={};}
      ++(packet->witness.phase==CapturePhase::Ordinary?s.worker_ordinary:s.worker_body);
      if(packet->witness.phase==CapturePhase::Ordinary){s.transfer_source=std::move(value);s.transferring=std::move(ticket);break;}
    }catch(const std::exception &error){++s.worker_failures;std::lock_guard lock(s.publish_status);strncpy_s(s.publish_failure.data(),s.publish_failure.size(),error.what(),_TRUNCATE);}
    catch(...){++s.worker_failures;std::lock_guard lock(s.publish_status);strncpy_s(s.publish_failure.data(),s.publish_failure.size(),"binary_publish_exception",_TRUNCATE);}
  }
}
void BinaryEntities::NetworkPublication(bool enabled) noexcept {auto &s=*impl_;if(!s.Main())return;
  const auto before=s.network_publication.exchange(enabled?1:0,std::memory_order_acq_rel);if(enabled&&!before&&s.stats.active)Invalidate();}
std::shared_ptr<const HostStatePublication> BinaryEntities::Publication() const noexcept {
  const auto &s=*impl_;return s.Main()&&s.stats.active&&!s.invalidated?s.publication:nullptr;
}
}
