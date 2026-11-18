#include "state/client_plan.h"
#include <bit>
#include "state/client_memory_resource.h"
#include <algorithm>
#include <cstring>
#include <map>
#include <tuple>
#include <unordered_map>

namespace cs::state {
namespace {
#include "state_host_fields.inc"
namespace F=host_field;
[[noreturn]] void Fail(StateReadiness r,const char *why){throw ClientShadowError(r,why);}
void Require(bool condition,const char *why){if(!condition)Fail(StateReadiness::InvalidReference,why);}
template<class T>T Exact(std::span<const std::byte> bytes) {
  if(bytes.size()!=sizeof(T))Fail(StateReadiness::InvalidEncoding,"client_plan_scalar_size");
  T value;std::memcpy(&value,bytes.data(),sizeof(value));return value;
}
std::span<const std::byte> Required(ClientShadowRecordView record,std::uint16_t field) {
  const auto *f=record.Field(field);if(!f||!f->present)Fail(StateReadiness::SourcePending,"client_plan_required_field");return f->bytes;
}
template<class T>T Number(ClientShadowRecordView record,std::uint16_t field){return Exact<T>(Required(record,field));}
// 只用于对象独占段内选桶；发生碰撞仍比较完整路径，不以摘要代替身份。
std::uint32_t FieldBucket(std::uint32_t path)noexcept {
  auto value=(2166136261u^path)*16777619u;
  value^=value>>16;value*=0x85ebca6bu;value^=value>>13;value*=0xc2b2ae35u;return value^(value>>16);
}
// 固定小值仍从可信数字布局取得边界；网络不能提供偏移或原生布局。
template<class T>T Fixed(std::uint16_t layout,std::uint16_t field,std::span<const std::byte> bytes) {
  const auto *f=FindField(layout,field);
  if(!f||f->wire_offset==kDynamicOffset||f->min_size!=sizeof(T)||f->max_size!=sizeof(T)||f->wire_offset>bytes.size()||sizeof(T)>bytes.size()-f->wire_offset)
    Fail(StateReadiness::InvalidEncoding,"client_plan_fixed_field");return Exact<T>(bytes.subspan(f->wire_offset,sizeof(T)));
}
bool Descends(std::uint16_t id,std::uint16_t root) {
  for(unsigned depth=0;id&&depth<32;++depth){if(id==root)return true;const auto *l=FindLayout(id);if(!l)return false;id=l->parent;}return false;
}
ClientInstallDomain Domain(const PresentationMember &m) {
  if(m.space==1&&m.layout==197)return ClientInstallDomain::Player;
  if(m.space==1&&Descends(m.layout,100)&&!FindLayout(m.layout)->base_prefix)return ClientInstallDomain::Entity;
  if(m.space==2&&Descends(m.layout,199)&&!FindLayout(m.layout)->base_prefix)return ClientInstallDomain::Grid;
  if(m.space==3&&m.layout==237)return ClientInstallDomain::Run;
  if(m.space==4&&m.layout==276)return ClientInstallDomain::Level;
  if(m.space==6&&m.layout==270)return ClientInstallDomain::Descriptor;
  if(m.space==7&&m.layout==239)return ClientInstallDomain::Room;
  if(m.space==9&&m.layout==195)return ClientInstallDomain::Participant;
  if(m.space==10&&m.layout==196)return ClientInstallDomain::ControlGroup;
  if(m.space==12&&m.layout==437)return ClientInstallDomain::Sfx;
  Fail(StateReadiness::InvalidReference,"client_plan_unregistered_domain");
}
template<class K>using WorkMap=std::pmr::map<K,std::uint32_t>;
using PathKey=std::tuple<std::uint32_t,std::uint16_t,std::uint16_t,std::uint16_t,std::uint16_t>;
using FieldKey=std::tuple<std::uint32_t,std::uint32_t,std::uint16_t,std::uint16_t>;
// 数字路径只做精确查找；完整tuple相等比较负责区分碰撞，不依赖桶顺序生成计划。
// 所有键成员都是32位以内的无符号编号；新增更宽成员时必须显式调整混合方式。
struct WorkKeyHash {
  template<class... T> std::size_t operator()(const std::tuple<T...> &key)const noexcept {
    static_assert(((sizeof(T)<=sizeof(std::uint32_t))&&...));
    std::uint32_t mixed=2166136261u;
    std::apply([&](auto... part){((mixed=(mixed^std::uint32_t(part))*16777619u),...);},key);
    mixed^=mixed>>16;mixed*=0x7feb352du;mixed^=mixed>>15;
    return mixed;
  }
};
template<class K>using WorkIndex=std::pmr::unordered_map<K,std::uint32_t,WorkKeyHash>;
}
bool ClientCostumeProjection(std::uint16_t layout,std::uint16_t field) noexcept {
  return layout==F::EntityPlayer::kLayout&&(field==F::EntityPlayer::_costumeSpriteDescs_0||
    field==F::EntityPlayer::_costumeSpriteDescs_1||field==F::EntityPlayer::_costumeSpriteDescs_2||field==F::EntityPlayer::_costumeSpriteDescs_3);
}
bool ClientCostumeDirectoryMatches(const ClientShadowStateRef &a,const ClientShadowStateRef &b,
    const PresentationMember &player) noexcept {
  if(!a||!b)return false;
  const auto x=a->Find(player),y=b->Find(player);
  return x&&y&&(a==b||(x->SameField(*y,F::EntityPlayer::costume_topology_revision)&&
    x->SameField(*y,F::EntityPlayer::_costumeSpriteDescs_0)&&x->SameField(*y,F::EntityPlayer::_playerCostumeMap)));
}
ClientStateInstallPlan::ClientStateInstallPlan(const std::shared_ptr<ClientStateMemory> &m)
  :objects_(ClientStateAllocator<ClientInstallObject>(m)),retired_(ClientStateAllocator<StateMemberEntry>(m)),
   records_(ClientStateAllocator<ClientInstallRecord>(m)),block_ends_(ClientStateAllocator<ClientInstallBlockEnd>(m)),
   paths_(ClientStateAllocator<ClientInstallPath>(m)),
   data_(ClientStateAllocator<std::shared_ptr<const ClientInstallRecordData>>(m)),
   field_records_(ClientStateAllocator<std::uint32_t>(m)),value_records_(ClientStateAllocator<std::uint32_t>(m)),
   binding_records_(ClientStateAllocator<std::uint32_t>(m)),private_records_(ClientStateAllocator<std::uint32_t>(m)),
   field_ranges_(ClientStateAllocator<FieldRange>(m)),
   field_lookup_(ClientStateAllocator<FieldSlot>(m)),
   presentation_fields_(ClientStateAllocator<std::uint32_t>(m)),presentation_values_(ClientStateAllocator<std::uint32_t>(m)),
   native_binding_values_(ClientStateAllocator<std::uint32_t>(m)),
   native_display_values_(ClientStateAllocator<std::uint32_t>(m)),
   descriptors_(ClientStateAllocator<ClientInstallDescriptor>(m)),grids_(ClientStateAllocator<ClientInstallGrid>(m)),
   entities_(ClientStateAllocator<ClientInstallEntity>(m)),
   players_(ClientStateAllocator<ClientInstallPlayer>(m)),groups_(ClientStateAllocator<ClientInstallGroup>(m)),
   anchors_(ClientStateAllocator<std::uint32_t>(m)),events_(ClientStateAllocator<ClientInstallEvent>(m)),
   transitions_(ClientStateAllocator<ClientInstallTransition>(m)),
   native_needs_(ClientStateAllocator<ClientInstallNativeNeed>(m)),
   stage_objects_(ClientStateAllocator<std::uint32_t>(m)),stage_fields_(ClientStateAllocator<std::uint32_t>(m)),
   bytes_(ClientStateAllocator<std::byte>(m)) {
  world_.doors.fill(kClientPlanNone);world_.occupancy.fill(kNullIndex);
}
std::span<const std::byte> ClientStateInstallPlan::Bytes(ClientInstallSlice s)const {
  if(s.block)return {s.block,s.size};
  if(s.offset>bytes_.size()||s.size>bytes_.size()-s.offset)Fail(StateReadiness::InvalidEncoding,"client_plan_slice");return std::span(bytes_).subspan(s.offset,s.size);
}
ClientInstallField ClientStateInstallPlan::Field(std::uint32_t index)const noexcept {
  const auto record=field_records_[index];
  auto field=data_[record]->fields[index-records_[record].first_field];
  field.record=record;
  return field;
}
ClientInstallValue ClientStateInstallPlan::Value(std::uint32_t index)const noexcept {
  const auto record=value_records_[index];
  return Value(record,index-records_[record].first_value);
}
ClientInstallValue ClientStateInstallPlan::Value(std::uint32_t record,std::uint32_t local)const noexcept {
  auto value=data_[record]->values[local];
  value.record=record;
  const auto &r=records_[record];
  if(r.object!=kClientPlanNone&&private_records_[r.object]==record){
    if(value.field!=kClientPlanNone)value.field+=r.first_field;
    if(value.parent_field!=kClientPlanNone)value.parent_field+=r.first_field;
  }
  return value;
}
ClientInstallBinding ClientStateInstallPlan::RebaseBinding(std::uint32_t record,ClientInstallBinding link)const noexcept {
  link.value+=records_[record].first_value;
  if(!link.is_resource&&!link.is_null&&(link.object>=objects_.size()||objects_[link.object].member.identity!=link.member)){
    const auto i=std::lower_bound(objects_.begin(),objects_.end(),link.member,[](const auto &a,const auto &b){return a.member.identity<b;});
    link.object=i!=objects_.end()&&i->member.identity==link.member?std::uint32_t(i-objects_.begin()):kClientPlanNone;
  }
  return link;
}
ClientInstallBinding ClientStateInstallPlan::Binding(std::uint32_t index)const noexcept {
  const auto record=binding_records_[index];
  return RebaseBinding(record,data_[record]->bindings[index-records_[record].first_binding]);
}
std::optional<ClientInstallBinding> ClientStateInstallPlan::BindingForValue(std::uint32_t record,std::uint32_t local)const noexcept {
  const auto &links=data_[record]->bindings;
  const auto i=std::lower_bound(links.begin(),links.end(),local,[](const auto &link,auto value){return link.value<value;});
  if(i==links.end()||i->value!=local)return {};
  return RebaseBinding(record,*i);
}
std::uint32_t ClientStateInstallPlan::CanonicalField(std::uint32_t object,std::uint32_t path)const noexcept {
  if(path>=normalized_path_count_||object>=field_ranges_.size())return kClientPlanNone;
  if(const auto record=private_records_[object];record!=kClientPlanNone){
    const auto &lookup=data_[record]->lookup;
    const auto i=std::lower_bound(lookup.begin(),lookup.end(),path,[](const auto &slot,auto key){return slot.path<key;});
    return i!=lookup.end()&&i->path==path?records_[record].first_field+i->local:kClientPlanNone;
  }
  const auto range=field_ranges_[object];if(!range.count)return kClientPlanNone;
  if(range.base!=kClientPlanNone){
    if(path<range.base||path-range.base>=range.count)return kClientPlanNone;
    return field_lookup_[range.first+path-range.base].field;
  }
  auto slot=FieldBucket(path)&(range.count-1);
  for(std::uint32_t n=0;n<range.count;++n,slot=(slot+1)&(range.count-1)){
    const auto &entry=field_lookup_[range.first+slot];if(entry.field==kClientPlanNone)return kClientPlanNone;
    if(entry.path==path)return entry.field;
  }
  return kClientPlanNone;
}
ClientShadowStateRef ClientStateInstallPlan::Effective(BodyStream stream)const {
  if(unsigned(stream)>1)Fail(StateReadiness::InvalidEncoding,"client_plan_stream");
  const auto i=unsigned(stream);
  return streams_[i]?streams_[i]:peer_context_[i].lock();
}
std::uint32_t ClientStateInstallPlan::RecordIndex(BodyStream stream,const PresentationMember &identity)const noexcept {
  if(unsigned(stream)>1||!candidate_)return kClientPlanNone;
  const auto own=candidate_->State(stream);
  if(!own)return kClientPlanNone;
  const auto position=own->RecordPosition(identity);
  if(!position)return kClientPlanNone;
  const auto world=candidate_->State(BodyStream::World);
  return *position+(stream==BodyStream::Body&&world?std::uint32_t(world->RecordCount()):0u);
}
bool ClientStateInstallPlan::ReusableRecord(const ClientInstallRecord &record,std::uint32_t previous_mask) const noexcept {
  return record.event==kClientPlanNone&&record.context.base_version&&record.object<objects_.size()&&
    objects_[record.object].action==ClientInstallAction::Reuse&&record.mask==previous_mask&&
    !(record.structure_mask|record.dependency_mask);
}
bool ClientStateInstallPlan::SameRecordShape(std::uint32_t record,const ClientStateInstallPlan &other,std::uint32_t other_record)const noexcept {
  if(record>=records_.size()||other_record>=other.records_.size())return false;
  const auto &a=records_[record];const auto &b=other.records_[other_record];
  if(a.event!=kClientPlanNone||b.event!=kClientPlanNone||a.layout!=b.layout||a.stream!=b.stream||
     a.field_count!=b.field_count||a.value_count!=b.value_count||a.binding_count!=b.binding_count)return false;
  const auto &x=data_[record];const auto &y=other.data_[other_record];
  if(!x||!y)return false;
  // 共享同一不可变描述数据：形状由构造保证一致。
  if(x==y)return true;
  if(x->fields.size()!=y->fields.size()||x->values.size()!=y->values.size()||x->bindings.size()!=y->bindings.size())return false;
  const bool xp=a.object!=kClientPlanNone&&a.object<private_records_.size()&&private_records_[a.object]==record;
  const bool yp=b.object!=kClientPlanNone&&b.object<other.private_records_.size()&&other.private_records_[b.object]==other_record;
  // 私有记录的字段归属按记录内序号存放，共享记录按全局序号存放；统一换成记录内序号再比。
  const auto local=[](std::uint32_t field,bool private_record,std::uint32_t first){
    return field==kClientPlanNone||private_record?field:field-first;
  };
  for(std::size_t i=0;i<x->fields.size();++i){
    const auto &f=x->fields[i];const auto &g=y->fields[i];
    if(f.path!=g.path||f.definition!=g.definition||f.elements!=g.elements||f.slot_mask!=g.slot_mask||
       f.present!=g.present||f.payload_present!=g.payload_present||f.phase!=g.phase)return false;
  }
  for(std::size_t i=0;i<x->values.size();++i){
    const auto &v=x->values[i];const auto &w=y->values[i];
    if(v.path!=w.path||v.layout!=w.layout||v.component!=w.component||v.operation!=w.operation||
       local(v.field,xp,a.first_field)!=local(w.field,yp,b.first_field)||
       local(v.parent_field,xp,a.first_field)!=local(w.parent_field,yp,b.first_field))return false;
  }
  for(std::size_t i=0;i<x->bindings.size();++i){
    const auto &l=x->bindings[i];const auto &m=y->bindings[i];
    if(l.value!=m.value||l.member!=m.member||l.resource!=m.resource||l.is_resource!=m.is_resource||l.is_null!=m.is_null)return false;
  }
  return true;
}
std::uint32_t ClientStateInstallPlan::MemberRecordCount()const noexcept {
  std::uint32_t n=0;
  while(n<records_.size()&&records_[n].event==kClientPlanNone)++n;
  for(auto i=n;i<records_.size();++i)if(records_[i].event==kClientPlanNone)return kClientPlanNone;
  return n;
}
bool ClientStateInstallPlan::SameStructure(const ClientStateInstallPlan &o)const noexcept {
  if(objects_.size()!=o.objects_.size()||entities_.size()!=o.entities_.size()||players_.size()!=o.players_.size()||
     groups_.size()!=o.groups_.size()||anchors_.size()!=o.anchors_.size()||grids_.size()!=o.grids_.size()||
     descriptors_.size()!=o.descriptors_.size()||native_needs_.size()!=o.native_needs_.size()||
     stage_objects_.size()!=o.stage_objects_.size()||o.paths_.size()>paths_.size()||
     o.normalized_path_count_>normalized_path_count_)return false;
  for(std::size_t i=0;i<objects_.size();++i){
    const auto &a=objects_[i];const auto &b=o.objects_[i];
    if(a.member.identity!=b.member.identity||a.domain!=b.domain||a.description!=b.description||a.streams!=b.streams)return false;
  }
  const auto &w=world_;const auto &v=o.world_;
  if(w.run!=v.run||w.level!=v.level||w.room!=v.room||w.sfx!=v.sfx||w.current_descriptor!=v.current_descriptor||
     w.doors!=v.doors||w.occupancy!=v.occupancy||w.grid_width!=v.grid_width||w.grid_height!=v.grid_height||
     w.current_entry!=v.current_entry||w.dimension!=v.dimension||w.replace_world!=v.replace_world||w.replace_body!=v.replace_body)return false;
  for(std::size_t i=0;i<entities_.size();++i){
    const auto &a=entities_[i];const auto &b=o.entities_[i];
    if(a.object!=b.object||a.type!=b.type||a.variant!=b.variant||a.subtype!=b.subtype||a.init_seed!=b.init_seed||
       a.order!=b.order||a.configuration!=b.configuration)return false;
  }
  for(std::size_t i=0;i<players_.size();++i){
    const auto &a=players_[i];const auto &b=o.players_[i];
    if(a.object!=b.object||a.group!=b.group||a.topology!=b.topology||a.player_type!=b.player_type||a.roster!=b.roster||
       a.membership!=b.membership||a.role!=b.role||a.control!=b.control)return false;
  }
  for(std::size_t i=0;i<groups_.size();++i){
    const auto &a=groups_[i];const auto &b=o.groups_[i];
    if(a.object!=b.object||a.participant!=b.participant||a.first_anchor!=b.first_anchor||a.anchor_count!=b.anchor_count||
       a.session!=b.session||a.binding!=b.binding||a.ordinal!=b.ordinal)return false;
  }
  if(!std::equal(anchors_.begin(),anchors_.end(),o.anchors_.begin())||
     !std::equal(stage_objects_.begin(),stage_objects_.end(),o.stage_objects_.begin()))return false;
  for(std::size_t i=0;i<grids_.size();++i){
    const auto &a=grids_[i];const auto &b=o.grids_[i];
    if(a.object!=b.object||a.index!=b.index||a.spawn_seed!=b.spawn_seed||a.type!=b.type||a.variant!=b.variant||
       a.state!=b.state||a.has_spawn_seed!=b.has_spawn_seed)return false;
  }
  for(std::size_t i=0;i<descriptors_.size();++i){
    const auto &a=descriptors_[i];const auto &b=o.descriptors_[i];
    if(a.object!=b.object||a.source_slot!=b.source_slot||a.ordinary_ordinal!=b.ordinary_ordinal)return false;
  }
  for(std::size_t i=0;i<native_needs_.size();++i){
    const auto &a=native_needs_[i];const auto &b=o.native_needs_[i];
    if(a.kind!=b.kind||a.id!=b.id||a.second_id!=b.second_id)return false;
  }
  for(std::size_t i=0;i<o.paths_.size();++i){
    const auto &a=paths_[i];const auto &b=o.paths_[i];
    if(a.parent!=b.parent||a.step!=b.step||a.normalized!=b.normalized||a.field_path!=b.field_path||a.canonical!=b.canonical)return false;
  }
  const auto members=MemberRecordCount();
  if(members==kClientPlanNone||members!=o.MemberRecordCount())return false;
  for(std::uint32_t n=0;n<members;++n){
    const auto &a=records_[n];const auto &b=o.records_[n];
    if(a.object!=b.object||a.first_field!=b.first_field||a.first_value!=b.first_value||a.first_binding!=b.first_binding||
       !SameRecordShape(n,o,n))return false;
  }
  return true;
}
// 构建器唯一可变；发布后不修改路径、正文、资源顺序或执行描述。
struct ClientInstallBuilder final:BodyNodeObserver {
  const ClientStatePreparedTarget &target;
  const ClientStateTables &tables;
  std::shared_ptr<ClientStateInstallPlan> out;
  // 索引只活到本次数字计划封存；节点和桶均成块计费，不逐节点竞争Peer记账锁。
  ClientStateMemoryResource index_upstream;
  std::pmr::monotonic_buffer_resource index_memory;
  WorkIndex<PathKey> path_index;
  // 与暂存／原子writer共用同一规范路径结果，避免两次主线程重建纯值索引。
  WorkIndex<PathKey> normalized_index;
  WorkIndex<FieldKey> field_index;
  WorkMap<std::uint64_t> loops;
  WorkMap<std::int32_t> definitions;
  WorkMap<std::uint32_t> overlays;
  // 只在本后台工作片链借已接受计划。路径重定位表只含数字，完成后随工作区归还。
  struct Accepted {
    std::shared_ptr<const ClientStateInstallPlan> plan;
    std::pmr::vector<std::uint32_t> paths;
    explicit Accepted(std::pmr::memory_resource *resource):paths(resource) {}
  };
  std::array<Accepted,2> accepted;
  // 借用的数据没有可写入口；后处理按记录跳过全部已封存区间。
  std::pmr::vector<ClientInstallRecordData *> fresh;
  ClientInstallRecordData *writing=nullptr;
  std::uint32_t inherited_paths=0,scan_record=0;
  // 纯值构建游标保留在同一候选内，分片之间不会发布半个计划。
  enum class Pass {Objects,World,Players,Records,Events,PathFields,FieldCounts,FieldRanges,FieldIndex,Rules,Transitions,Commands,Done};
#ifdef ISAACCS_E2E
  // 保存一个后台工作片的细分时间，交还 worker 前一次归并。
  struct Timing {
    static constexpr unsigned first=unsigned(ClientApplyStage::PlanImport);
    std::array<ClientApplyDetailTimer::Clock::duration,unsigned(ClientApplyStage::PlanCommands)-first+1> elapsed{};
    ClientApplyDetailTimer Scope(ClientApplyStage stage) noexcept {
      return ClientApplyDetailTimer(stage==ClientApplyStage::Count?nullptr:&elapsed[unsigned(stage)-first]);
    }
    void Report(ClientApplyMetrics &metrics,std::uint8_t streams) const noexcept {
      for(unsigned i=0;i<elapsed.size();++i)if(elapsed[i].count())
        metrics.Mark(streams,ClientApplyStage(first+i),ClientApplyDetailTimer::Microseconds(elapsed[i]));
    }
  };
#endif
  Pass pass=Pass::Objects;
  unsigned stream=0,command=0;
  std::size_t item=0;
  ClientShadowVisitCursor cursor;
  bool record_open=false;
  std::uint32_t accepted_record=kClientPlanNone,imported=0;
  // World 导入和重走共用同一块游标，断点可落在块内。
  std::uint32_t world_blocks=0,world_rank=0;
  std::array<std::uint64_t,2> reused_records{};
  std::uint64_t reused_world_blocks=0;
  std::uint64_t shared_world_records=0;
  std::array<std::array<std::uint64_t,4>,2> volume{};
  ClientStateVector<ProjectionFact> facts;
  std::uint32_t record=0,visits=0;
  std::span<const std::byte> decoding;
  ClientInstallBuilder(const ClientStatePreparedTarget &t,const ClientStateTables &s,
      const std::array<std::weak_ptr<const ClientStateInstallPlan>,2> &previous)
    :target(t),tables(s),out(std::allocate_shared<ClientStateInstallPlan>(ClientStateAllocator<ClientStateInstallPlan>(s.Memory()),s.Memory())),
     index_upstream(s.Memory(),memory::Pool::Workspace),index_memory(8192,&index_upstream),
     path_index(&index_memory),normalized_index(&index_memory),field_index(&index_memory),loops(&index_memory),definitions(&index_memory),overlays(&index_memory),
     accepted{Accepted(&index_memory),Accepted(&index_memory)},fresh(&index_memory),
     facts(ClientStateAllocator<ProjectionFact>(s.Memory(),memory::Pool::Workspace)) {
    facts.resize(128);
    for(unsigned i=0;i<2;++i)if(t.candidate&&t.candidate->State(BodyStream(i))&&t.previous[i]){
      auto plan=previous[i].lock();
      if(plan&&plan->Target().route==t.candidate->Target().route&&plan->Effective(BodyStream(i))==t.previous[i]){
        accepted[i].paths.resize(plan->Paths().size(),kClientPlanNone);accepted[i].plan=std::move(plan);
      }
    }
    // 沿用受限的路径命名空间；形状长期变化时从当前目标重建，历史路径不会无界累积。
    if(const auto &plan=accepted[0].plan;plan&&plan->Paths().size()<=16384){
      inherited_paths=std::uint32_t(plan->Paths().size());
      out->paths_.assign(plan->Paths().begin(),plan->Paths().end());
      out->normalized_path_count_=plan->NormalizedPathCount();
      for(std::uint32_t i=0;i<inherited_paths;++i){
        const auto &p=out->paths_[i];const auto &s=p.step;
        path_index.emplace(PathKey{p.parent,s.layout,s.field,s.element,s.choice_layout},i);
        const auto parent=p.parent==kClientPlanNone?kClientPlanNone:out->paths_[p.parent].normalized;
        normalized_index.emplace(PathKey{parent,s.layout,p.canonical,s.element,s.choice_layout},p.normalized);
        accepted[0].paths[i]=i;
      }
    }
  }
  std::uint32_t Object(const PresentationMember &id)const {
    const auto &objects=out->objects_;const auto i=std::lower_bound(objects.begin(),objects.end(),id,[](const auto &a,const auto &b){return a.member.identity<b;});
    return i!=objects.end()&&i->member.identity==id?std::uint32_t(i-objects.begin()):kClientPlanNone;
  }
  ClientShadowRecordView Root(std::uint32_t object,unsigned stream=0)const {
    if(object>=out->objects_.size()||!target.effective[stream])Fail(StateReadiness::SourcePending,"client_plan_root_missing");
    const auto r=target.effective[stream]->Find(out->objects_[object].member.identity);if(!r)Fail(StateReadiness::SourcePending,"client_plan_root_missing");return *r;
  }
  std::uint32_t Reference(std::uint16_t index,const ClientMemberSnapshotRef &members)const {
    if(index==kNullIndex)return kClientPlanNone;
    const auto *entry=FindStateMember(members->Directory(),index);if(!entry)Fail(StateReadiness::InvalidReference,"client_plan_reference_missing");
    const auto object=Object(entry->identity);if(object==kClientPlanNone)Fail(StateReadiness::SourcePending,"client_plan_reference_not_in_target");return object;
  }
  std::uint32_t Reference(ClientShadowRecordView from,std::uint16_t field)const {
    return Reference(Number<std::uint16_t>(from,field),target.effective[0]->Members());
  }
  std::uint32_t Path(std::span<const CodecPathStep> path) {
    std::uint32_t parent=kClientPlanNone;
    for(const auto &step:path) {
      PathKey key{parent,step.layout,step.field,step.element,step.choice_layout};const auto i=path_index.find(key);
      if(i!=path_index.end()){parent=i->second;continue;}
      BodySourceFieldInfo format;
      Require(DescribeBodySourceField(step.layout,step.field,format),"client_plan_path_definition");
      parent=AddPath(parent,step,format.canonical,key);
    }
    return parent;
  }
  std::uint32_t AddPath(std::uint32_t parent,const CodecPathStep &step,std::uint16_t canonical,const PathKey &key) {
    if(out->paths_.size()-inherited_paths>=262144)Fail(StateReadiness::Budget,"client_plan_path_budget");
    const auto normalized_parent=parent==kClientPlanNone?kClientPlanNone:out->paths_[parent].normalized;
    const PathKey normalized_key{normalized_parent,step.layout,canonical,step.element,step.choice_layout};
    const auto [at,inserted]=normalized_index.emplace(normalized_key,out->normalized_path_count_);
    if(inserted)++out->normalized_path_count_;
    const auto next=std::uint32_t(out->paths_.size());
    out->paths_.push_back({parent,step,at->second,kClientPlanNone,canonical});
    path_index.emplace(key,next);
    return next;
  }
  // 相同旧路径最多重定位一次，保留完整条件臂及规范字段归属。
  std::uint32_t ImportPath(Accepted &source,std::uint32_t path) {
    if(path==kClientPlanNone)return path;
    auto &mapped=source.paths[path];
    if(mapped!=kClientPlanNone)return mapped;
    const auto &old=source.plan->Paths()[path];
    const auto parent=ImportPath(source,old.parent);
    const auto &step=old.step;
    const PathKey key{parent,step.layout,step.field,step.element,step.choice_layout};
    const auto i=path_index.find(key);
    return mapped=i==path_index.end()?AddPath(parent,step,old.canonical,key):i->second;
  }
  ClientInstallSlice Slice(std::span<const std::byte> bytes)const {
    if(bytes.empty())return {};
    if(out->records_[record].event==kClientPlanNone)return {0,std::uint32_t(bytes.size()),bytes.data()};
    const auto a=reinterpret_cast<std::uintptr_t>(decoding.data()),b=reinterpret_cast<std::uintptr_t>(bytes.data());
    if(b<a||b-a>decoding.size()||bytes.size()>decoding.size()-(b-a))Fail(StateReadiness::InvalidEncoding,"client_plan_unowned_node");
    return {out->records_[record].bytes.offset+std::uint32_t(b-a),std::uint32_t(bytes.size())};
  }
  static bool PresentationLayout(std::uint16_t layout) noexcept {
    return (layout>=438&&layout<=441)||(layout>=448&&layout<=455);
  }
  void Field(const BodyFieldNodeView &node)override {
    Require(!node.path.empty(),"client_plan_field_path");const auto path=Path(node.path);const auto &step=node.path.back();
    const auto *definition=node.definition;
    if(!definition){
      definition=FindField(step.layout,step.field);const auto *handler=FindClientField(step.layout,step.field);
      Require(definition&&handler&&handler->operation==definition->operation,"client_plan_field_dispatch_missing");
    }
    auto phase=ClientInstallPhase::Values;const auto &r=out->records_[record];
    if(r.event!=kClientPlanNone)phase=ClientInstallPhase::Events;
    else {
      const auto domain=out->objects_[r.object].domain;const auto *root=FindField(node.path.front().layout,node.path.front().field);
      if(domain==ClientInstallDomain::Run||domain==ClientInstallDomain::Sfx||
        (domain==ClientInstallDomain::Room&&root->block>=1&&root->block!=3))phase=ClientInstallPhase::Display;
      BodySourceFieldInfo format;Require(DescribeBodySourceField(step.layout,step.field,format),"client_plan_field_format");
      if(format.linked&&format.resource_table==255)phase=ClientInstallPhase::RelationFields;
    }
    AddField({record,path,std::uint32_t(definition-state::Fields().data()),node.elements,node.slot_mask,Slice(node.bytes),node.present,node.payload_present,phase});
  }
  void AddField(const ClientInstallField &field) {
    if(out->FieldCount()+writing->fields.size()>=262144)Fail(StateReadiness::Budget,"client_plan_field_budget");
    const auto n=std::uint32_t(writing->fields.size());
    writing->fields.push_back(field);
    const auto &path=out->paths_[field.path];
    const auto &definition=state::Fields()[field.definition];
    if(PresentationLayout(definition.layout))writing->presentation_fields.push_back(n);
    if(out->records_[record].event==kClientPlanNone&&
        (definition.nested_layout||definition.operation==Operation::Container||definition.operation==Operation::Choice))
      writing->stage_fields.push_back(n);
    if(!field_index.emplace(FieldKey{record,path.parent,path.step.layout,path.step.field},out->records_[record].first_field+n).second)
      Fail(StateReadiness::InvalidEncoding,"client_plan_duplicate_field");
  }
  std::uint32_t AddValue(const ClientInstallValue &node) {
    if(out->ValueCount()+writing->values.size()>=262144)Fail(StateReadiness::Budget,"client_plan_value_budget");
    const auto value=std::uint32_t(writing->values.size());writing->values.push_back(node);
    if(node.layout&&(PresentationLayout(node.layout)||out->records_[record].event!=kClientPlanNone))
      writing->presentation_values.push_back(value);
    // NativeBindings的两个显示节点循环只消费这三个布局，完整检查仍在主线程执行。
    if(node.layout==F::OverlayDisplayState::kLayout||node.layout==F::ProceduralDisplayDefinition::kLayout||
        node.layout==F::ProceduralDisplaySet::kLayout)writing->native_display_values.push_back(value);
    // 与主线程特殊字段的消费谓词一致；在已有解码遍历中登记，避免提交时再扫所有标量。
    if(!node.layout&&node.path!=kClientPlanNone){
      const auto &step=out->paths_[node.path].step;
      if(node.operation==Operation::Reference||node.operation==Operation::Resource||
          (step.layout==F::AnimationState::kLayout&&(step.field==F::AnimationState::layer_frames||
            step.field==F::AnimationState::null_layer_frames)))writing->native_binding_values.push_back(value);
    }
    return value;
  }
  void Node(const BodyNodeView &node)override {
    const auto value=AddValue({record,Path(node.path),node.layout,node.component,node.operation,Slice(node.bytes)});
    if(node.layout||(node.operation!=Operation::Reference&&node.operation!=Operation::Resource))return;
    const auto index=Exact<std::uint16_t>(node.bytes);ClientInstallBinding link;link.value=value;link.is_null=index==kNullIndex;link.is_resource=node.operation==Operation::Resource;
    if(link.is_resource)link.resource={node.resource_table,index};
    else if(!link.is_null) {
      const auto *member=FindStateMember(out->records_[record].members->Directory(),index);Require(member,"client_plan_binding_member");
      link.member=member->identity;link.object=Object(link.member);
      if(link.object==kClientPlanNone&&out->records_[record].event!=kClientPlanNone)Fail(
        (StateEventCosmetic(out->events_[out->records_[record].event].kind)?StateReadiness::SourcePending:StateReadiness::EventGap),"client_plan_binding_retired_target");
    }
    writing->bindings.push_back(link);
  }
  void ImportField(Accepted &source,std::uint32_t index) {
    auto field=source.plan->Field(index);
    field.record=record;field.path=ImportPath(source,field.path);AddField(field);
  }
  void ImportValue(Accepted &source,std::uint32_t index) {
    auto node=source.plan->Value(index);
    node.record=record;node.path=ImportPath(source,node.path);node.field=node.parent_field=kClientPlanNone;
    AddValue(node);
  }
  void ImportBinding(ClientInstallBinding binding,std::uint32_t value) {
    binding.value=value;
    if(!binding.is_resource&&!binding.is_null){
      binding.object=Object(binding.member);
    }
    writing->bindings.push_back(binding);
  }
  void OpenRecord(ClientInstallRecord r) {
    r.first_field=std::uint32_t(out->FieldCount());
    r.first_value=std::uint32_t(out->ValueCount());
    r.first_binding=std::uint32_t(out->BindingCount());
    record=std::uint32_t(out->records_.size());
    out->records_.push_back(std::move(r));
  }
  void NewRecordData() {
    auto data=std::allocate_shared<ClientInstallRecordData>(
      ClientStateAllocator<ClientInstallRecordData>(out->Memory()),out->Memory());
    writing=data.get();
    fresh.push_back(writing);
    out->data_.push_back(std::move(data));
  }
  void CloseRecord() {
    const auto &data=*out->data_[record];
    auto &r=out->records_[record];
    r.field_count=std::uint32_t(data.fields.size());
    r.value_count=std::uint32_t(data.values.size());
    r.binding_count=std::uint32_t(data.bindings.size());
    Require(r.first_field+r.field_count<=262144&&r.first_value+r.value_count<=262144,"client_plan_record_budget");
    out->field_records_.resize(r.first_field+r.field_count,record);
    out->value_records_.resize(r.first_value+r.value_count,record);
    out->binding_records_.resize(r.first_binding+r.binding_count,record);
    const auto append=[](auto &dest,const auto &local,std::uint32_t base){
      for(auto index:local)dest.push_back(base+index);
    };
    append(out->presentation_fields_,data.presentation_fields,r.first_field);
    append(out->presentation_values_,data.presentation_values,r.first_value);
    append(out->native_binding_values_,data.native_binding_values,r.first_value);
    append(out->native_display_values_,data.native_display_values,r.first_value);
    writing=nullptr;
  }
  // 内容、路径命名空间和字段归属均与前代一致。只借数字记录，块正文由本批直接持有。
  bool ShareRecord(Accepted &source) {
    if(!inherited_paths||accepted_record==kClientPlanNone)return false;
    auto &r=out->records_[record];
    if(r.changed_mask||out->private_records_[r.object]!=record)return false;
    const auto &old=source.plan->records_[accepted_record];
    const auto &data=source.plan->data_[accepted_record];
    if(source.plan->private_records_[old.object]!=accepted_record||!data->shareable)return false;
    for(auto blocks=r.mask;blocks;blocks&=blocks-1){
      const auto b=std::countr_zero(blocks);
      if(r.blocks[b].bytes.block!=old.blocks[b].bytes.block)return false;
    }
    // 绑定访问时会按当前成员表重定位。缺席目标仍保留完整身份，供后续帧补挂。
    const auto shift=[&](const ClientInstallBlockEnd &end){
      return ClientInstallBlockEnd{r.first_field+(end.field-old.first_field),
        r.first_value+(end.value-old.first_value),r.first_binding+(end.binding-old.first_binding)};
    };
    for(unsigned i=0;i<unsigned(std::popcount(r.mask));++i)
      out->block_ends_.push_back(shift(source.plan->block_ends_[old.first_block_end+i]));
    out->data_.push_back(data);
    fresh.push_back(nullptr);
    CloseRecord();
    reused_world_blocks+=std::popcount(r.mask);
    ++shared_world_records;
    ++reused_records[0];
    return true;
  }
  // Body 沿用整记录导入，保留此前的路径登记方式。
  bool ImportRecordStep(const ClientInstallRecord &old,Accepted &source) {
    const auto total=old.field_count+old.value_count+old.binding_count;
    const auto end=(std::min)(total,imported+128u);
    for(;imported<end;++imported){
      if(imported<old.field_count){
        ImportField(source,old.first_field+imported);
      }else if(const auto value=imported-old.field_count;value<old.value_count){
        ImportValue(source,old.first_value+value);
      }else{
        const auto binding=source.plan->Binding(old.first_binding+value-old.value_count);
        ImportBinding(binding,binding.value-old.first_value);
      }
    }
    return imported==total;
  }
  static ClientInstallBlockEnd BlockStart(const ClientStateInstallPlan &plan,const ClientInstallRecord &r,unsigned rank) {
    if(rank)return plan.block_ends_[r.first_block_end+rank-1];
    return {r.first_field,r.first_value,r.first_binding};
  }
  // SameBlock 已核同一正文 owner、数字结构和引用语义。按原节点顺序导入，
  // 因此路径首见顺序、字段／值下标及随后五趟处理与重走一致。
  bool ImportWorldBlock(ClientShadowRecordView root,std::uint8_t block,unsigned &remaining,Accepted &source) {
    const auto &old=source.plan->Records()[accepted_record];
    const auto before=BlockStart(*source.plan,old,world_rank);
    const auto end=source.plan->block_ends_[old.first_block_end+world_rank];
    const auto after=BlockStart(*out,out->records_[record],world_rank);
    auto field=before.field+std::uint32_t(out->FieldCount()+writing->fields.size()-after.field);
    auto value=before.value+std::uint32_t(out->ValueCount()+writing->values.size()-after.value);
    auto link=before.binding+std::uint32_t(out->BindingCount()+writing->bindings.size()-after.binding);
    while(remaining&&(field!=end.field||value!=end.value)){
      const auto count=(std::min)({remaining,64u,end.field-field+end.value-value});
      auto fields=root.BlockFieldNodes(block,cursor.node,count);
      for(unsigned i=0;i<count;++i,fields>>=1){
        if(fields&1){ImportField(source,field++);continue;}
        const auto next_value=std::uint32_t(writing->values.size());
        ImportValue(source,value);
        if(link<end.binding){
          const auto binding=source.plan->Binding(link);
          if(binding.value==value){ImportBinding(binding,next_value);++link;}
        }
        ++value;
      }
      cursor.node+=count;remaining-=count;
    }
    if(field!=end.field||value!=end.value)return false;
    Require(link==end.binding,"client_plan_reused_block_bindings");
    return true;
  }
  // 从数字实例路径查询已定界字段；不会在主线程重解变长头或按名称找字段。
  ClientInstallField At(std::uint32_t rec,std::uint32_t parent,std::uint16_t layout,std::uint16_t field)const {
    const auto i=field_index.find({rec,parent,layout,field});if(i==field_index.end())Fail(StateReadiness::SourcePending,"client_plan_nested_field_missing");return out->Field(i->second);
  }
  template<class T>T AtNumber(const ClientInstallValue &instance,std::uint16_t field)const {
    const auto &f=At(instance.record,instance.path,instance.layout,field);Require(f.present,"client_plan_nested_field_absent");return Exact<T>(out->Bytes(f.bytes));
  }
  void Sample(const ClientInstallValue &instance,std::uint16_t field)const {
    const auto index=AtNumber<std::uint16_t>(instance,field);BodySourceFieldInfo format;
    Require(index!=kNullIndex&&DescribeBodySourceField(instance.layout,field,format),"client_plan_sample_missing");
    const auto *asset=FindResource(out->records_[instance.record].resources->Directory(),{format.resource_table,index});
    if(!asset)Fail(StateReadiness::MissingClosure,"client_plan_sample_declaration_missing");
    Require(Fixed<std::uint8_t>(F::ResourceAudio::kLayout,F::ResourceAudio::audio_class,asset->payload)==0&&
      Fixed<std::uint8_t>(F::ResourceAudio::kLayout,F::ResourceAudio::has_intro,asset->payload)==0,"client_plan_sample_audio_class");
  }
  template<class Fn>void Decode(Fn &&fn) {
    // scratch需求重试不能留下半套数字描述或重复资源／引用。
    const auto np=out->paths_.size(),nf=writing->fields.size(),nv=writing->values.size(),nb=writing->bindings.size();
    const auto normalized=out->normalized_path_count_;
    const auto pf=writing->presentation_fields.size(),pv=writing->presentation_values.size();
    const auto bv=writing->native_binding_values.size(),dv=writing->native_display_values.size(),sf=writing->stage_fields.size();
    for(;;)try{fn();return;}catch(const BodyScratchError &e) {
      if(e.Region()!=BodyScratchRegion::Facts||e.Requested()>262144)throw;
      for(auto i=path_index.begin();i!=path_index.end();)if(i->second>=np)i=path_index.erase(i);else ++i;
      // 规范索引与实际路径一起回卷，scratch扩容重试不保留未完成的数字描述。
      for(auto i=normalized_index.begin();i!=normalized_index.end();)if(i->second>=normalized)i=normalized_index.erase(i);else ++i;
      out->normalized_path_count_=normalized;
      const auto first=out->records_[record].first_field+nf;
      for(auto i=field_index.begin();i!=field_index.end();)if(i->second>=first)i=field_index.erase(i);else ++i;
      out->paths_.resize(np);writing->fields.resize(nf);writing->values.resize(nv);writing->bindings.resize(nb);
      writing->presentation_fields.resize(pf);writing->presentation_values.resize(pv);
      writing->native_binding_values.resize(bv);writing->native_display_values.resize(dv);writing->stage_fields.resize(sf);
      facts.resize(std::max<std::size_t>(e.Requested(),facts.size()*2));
    }
  }
  void Objects() {
    Require(target.candidate&&target.effective[0],"client_plan_world_context_missing");
    out->candidate_=target.candidate;
    out->streams_=target.effective;
    const auto streams=StateTargetStreams(target.candidate->Target());
    if(streams!=3){
      // 单流计划的对侧仅用于本批准备和配对复查。提交后的服装复用只借仍存活的旧见证。
      const auto peer=streams==1?1u:0u;
      out->peer_context_[peer]=out->streams_[peer];
      out->streams_[peer].reset();
    }
    const auto &scope=target.candidate->Target().route.scope;
    for(unsigned stream=0;stream<2;++stream)if(const auto state=target.effective[stream]) {
      Require(state->Context().scope==scope&&state->Context().stream==BodyStream(stream),"client_plan_effective_scope");
      if(const auto next=target.candidate->State(BodyStream(stream)))Require(next==state,"client_plan_effective_not_candidate");
    }
    for(const auto &member:target.effective[0]->Members()->Directory().entries)
      if(member.identity.layout==195||target.effective[0]->Find(member.identity))
        out->objects_.push_back({member,Domain(member.identity)});
    std::sort(out->objects_.begin(),out->objects_.end(),[](const auto &a,const auto &b){return a.member.identity<b.member.identity;});
    const auto &descriptor=target.candidate->Target();
    if(descriptor.count==2&&!descriptor.sections[0].base_version){
      // 双流完整基线在旧场景排空后安装。影子的历史成员仍可用于校验，
      // 但不能将同身份的旧原生对象列为本次借用。
      for(auto &object:out->objects_)object.action=ClientInstallAction::Join;
    }else for(const auto &change:target.candidate->Changes()) {
      if(change.kind==ClientShadowChangeKind::Leave){out->retired_.push_back(change.before);continue;}
      const auto i=Object(change.after.identity);Require(i!=kClientPlanNone,"client_plan_change_target");
      out->objects_[i].action=change.kind==ClientShadowChangeKind::Join?ClientInstallAction::Join:
        change.kind==ClientShadowChangeKind::Reshape?ClientInstallAction::Reshape:ClientInstallAction::Remap;
      if(change.kind==ClientShadowChangeKind::Reshape)out->retired_.push_back(change.before);
    }
    for(unsigned s=0;s<target.candidate->Target().count;++s){const auto &section=target.candidate->Target().sections[s];
      if(section.stream==BodyStream::World)out->world_.replace_world=section.base_version==0;else out->world_.replace_body=section.base_version==0;}
    for(unsigned stream=0;stream<2;++stream)if(const auto state=target.candidate->State(BodyStream(stream)))for(std::size_t i=0;i<state->RecordCount();++i) {
      const auto root=state->Record(i);const auto object=Object(root.Identity());
      if(stream&&object==kClientPlanNone)continue; // Body 只写已知的 World 对象。
      Require(object!=kClientPlanNone,"client_plan_record_object");
      out->objects_[object].streams|=std::uint8_t(1u<<stream);
    }
  }
  bool RecordStep(
#ifdef ISAACCS_E2E
      Timing *timing
#endif
      ) {
    if(stream==2)return true;
    const auto state=target.candidate->State(BodyStream(stream));
    if(!state||item==state->RecordCount()){++stream;item=0;return false;}
    const auto root=state->Record(item);
    if(!record_open){
      const auto object=Object(root.Identity());
      if(stream&&object==kClientPlanNone){++item;return false;}
      Require(object!=kClientPlanNone,"client_plan_record_object");
      ClientInstallRecord r;r.object=object;r.layout=root.Identity().layout;r.stream=BodyStream(stream);
      r.context=state->Context();r.members=state->Members();r.resources=state->Resources();r.mask=root.Mask();
      const auto previous=target.previous[stream]?target.previous[stream]->Find(root.Identity()):std::nullopt;
      for(unsigned b=0;b<32;++b)if(r.mask&(std::uint32_t(1)<<b))if(const auto block=root.Block(std::uint8_t(b))){
        const auto bit=std::uint32_t(1)<<b;
        r.blocks[b]={block->version,block->game_frame,block->source_revision,{0,std::uint32_t(block->bytes.size()),block->bytes.data()}};
        if(!previous||!root.SameBlock(*previous,std::uint8_t(b)))r.changed_mask|=bit;
        if(!previous||!root.SameContent(*previous,std::uint8_t(b)))r.value_mask|=bit;
        if(!previous||!root.SameStructure(*previous,std::uint8_t(b)))r.structure_mask|=bit;
        if(!previous||!root.SameDependencies(*previous,std::uint8_t(b)))r.dependency_mask|=bit;
      }
      volume[stream][0]+=std::popcount(r.value_mask);
      volume[stream][1]+=std::popcount(r.structure_mask);
      volume[stream][2]+=std::popcount(r.dependency_mask);
      volume[stream][3]+=std::popcount(r.mask&~r.structure_mask);
      auto &source=accepted[stream];
      const bool compatible=previous&&r.context.base_version&&out->objects_[object].action==ClientInstallAction::Reuse&&
        r.mask==previous->Mask()&&(stream==0||(!r.changed_mask&&out->ReusableRecord(r,previous->Mask())));
      accepted_record=source.plan&&compatible?
        source.plan->RecordIndex(BodyStream(stream),root.Identity()):kClientPlanNone;
      imported=0;
      if(!stream){
        r.first_block_end=std::uint32_t(out->block_ends_.size());
        world_blocks=r.mask;world_rank=0;
      }
      OpenRecord(std::move(r));
      const auto domain=out->objects_[object].domain;
      if(!stream&&(domain==ClientInstallDomain::Entity||domain==ClientInstallDomain::Grid)){
        out->private_records_[object]=record;
#ifdef ISAACCS_E2E
        auto measured=timing->Scope(ClientApplyStage::PlanImport);
#endif
        if(ShareRecord(source)){++item;return false;}
      }
      NewRecordData();
      record_open=true;
    }
    const bool reuse=accepted_record!=kClientPlanNone;
    auto &source=accepted[stream];
    bool full_reuse=reuse,done=false;
    if(stream==0){
      const auto &r=out->records_[record];
      full_reuse=reuse&&!r.changed_mask;
      unsigned remaining=128;
      while(world_blocks&&remaining){
        const auto block=std::uint8_t(std::countr_zero(world_blocks));
        const bool reuse_block=reuse&&!(r.changed_mask&(1u<<block));
#ifdef ISAACCS_E2E
        auto measured=timing->Scope(reuse_block?ClientApplyStage::PlanImport:ClientApplyStage::PlanWalk);
#endif
        if(!(reuse_block?ImportWorldBlock(root,block,remaining,source):root.VisitBlockSome(block,*this,cursor.node,remaining)))
          return false;
        out->block_ends_.push_back({r.first_field+std::uint32_t(writing->fields.size()),
          r.first_value+std::uint32_t(writing->values.size()),r.first_binding+std::uint32_t(writing->bindings.size())});
        reused_world_blocks+=reuse_block;
        world_blocks&=world_blocks-1;++world_rank;cursor.node=0;
      }
      if(!world_blocks){
        // 整条记录只有一个根布局标记，始终位于块片段之后。
        Node({{},r.layout,Operation::Nested,255,{}});done=true;
      }
    }else{
#ifdef ISAACCS_E2E
      auto measured=timing->Scope(reuse?ClientApplyStage::PlanImport:ClientApplyStage::PlanWalk);
#endif
      done=reuse?ImportRecordStep(source.plan->Records()[accepted_record],source):root.VisitSome(root.Mask(),*this,cursor,128);
    }
    if(done){
      CloseRecord();
      reused_records[stream]+=full_reuse;
      ++item;cursor={};record_open=false;
    }
    return false;
  }
  void World();
  void Players();
  void Event(std::size_t);
  void Rule(const ClientInstallValue &);
  void Transitions();
  bool CommandStep();
  // 工作片只枚举新建数据，借用整条记录时一步越过其逻辑区间。
  template<class Select,class Fn> bool FreshStep(Select select,Fn apply) {
    unsigned budget=128;
    while(budget&&scan_record<fresh.size()){
      auto *data=fresh[scan_record];
      if(data){
        auto &entries=select(*data);
        if(item<entries.size()){
          apply(entries[item],std::uint32_t(item),*data);
          ++item;--budget;
          continue;
        }
      }
      ++scan_record;item=0;--budget;
    }
    if(scan_record!=fresh.size())return false;
    scan_record=0;item=0;return true;
  }
  bool Step(
#ifdef ISAACCS_E2E
      Timing *timing
#endif
      ) {
#ifdef ISAACCS_E2E
    const auto stage=[&]{
      switch(pass){
        case Pass::PathFields:return ClientApplyStage::PlanPaths;
        case Pass::FieldCounts:return ClientApplyStage::PlanCounts;
        case Pass::FieldRanges:return ClientApplyStage::PlanRanges;
        case Pass::FieldIndex:return ClientApplyStage::PlanIndex;
        case Pass::Rules:return ClientApplyStage::PlanRules;
        case Pass::Commands:return ClientApplyStage::PlanCommands;
        default:return ClientApplyStage::Count;
      }
    }();
    auto measured=timing->Scope(stage);
#endif
    switch(pass){
      case Pass::Objects:
        Require(bool(target.facts),"client_plan_target_facts_missing");out->facts_=target.facts;Objects();
        out->private_records_.resize(out->objects_.size(),kClientPlanNone);
        pass=Pass::World;break;
      case Pass::World:World();pass=Pass::Players;break;
      case Pass::Players:Players();pass=Pass::Records;break;
      case Pass::Records:
        if(RecordStep(
#ifdef ISAACCS_E2E
            timing
#endif
            )){pass=Pass::Events;item=0;}
        break;
      case Pass::Events:
        if(item<target.events.size())Event(item++);
        else {pass=Pass::PathFields;item=0;}break;
      case Pass::PathFields:{
        // 所有普通／事件路径齐备后再找完整字段；每片最多128项，不额外阻塞后台预算。
        const auto end=(std::min)(out->paths_.size(),item+128);
        for(;item<end;++item){auto &p=out->paths_[item];
          if(item<inherited_paths&&p.field_path!=kClientPlanNone)continue;
          if(p.step.element==kNullIndex&&!p.step.choice_layout){p.field_path=p.normalized;continue;}
          const auto parent=p.parent==kClientPlanNone?kClientPlanNone:out->paths_[p.parent].normalized;
          const auto at=normalized_index.find(PathKey{parent,p.step.layout,p.canonical,kNullIndex,std::uint16_t{0}});
          p.field_path=at==normalized_index.end()?kClientPlanNone:at->second;
        }
        if(item==out->paths_.size()){
          out->field_ranges_.resize(out->objects_.size());pass=Pass::FieldCounts;item=0;
        }
        break;
      }
      case Pass::FieldCounts:{
        const auto done=FreshStep([](auto &data)->auto &{return data.fields;},
          [&](const auto &field,std::uint32_t local,auto &data){
            const auto &r=out->records_[scan_record];
            if(r.event!=kClientPlanNone)return;
            Require(r.object<out->field_ranges_.size(),"client_plan_field_object");
            const auto path=out->paths_[field.path].field_path;
            Require(path!=kClientPlanNone,"client_plan_field_path_missing");
            if(out->private_records_[r.object]==scan_record){
              data.lookup.push_back({path,local});
              return;
            }
            auto &range=out->field_ranges_[r.object];
            ++range.count;
            range.first=(std::min)(range.first,path);range.base=(std::max)(range.base,path);
          });
        if(done){pass=Pass::FieldRanges;item=0;}
        break;
      }
      case Pass::FieldRanges:{
        // 路径区间不超过字段数两倍时直接定位；否则沿用半空散列表，空对象不分配。
        // 稠密路径不会比旧散列表占用更多槽，所有空间仍由Client Values预算计费。
        const auto end=(std::min)(out->field_ranges_.size(),item+128);
        for(;item<end;++item){
          const auto record=out->private_records_[item];
          if(record!=kClientPlanNone&&fresh[record]){
            // 相同规范键保留记录中首先出现的字段。
            auto &lookup=fresh[record]->lookup;
            std::sort(lookup.begin(),lookup.end(),[](auto a,auto b){return std::tie(a.path,a.local)<std::tie(b.path,b.local);});
          }
          auto &range=out->field_ranges_[item];
          const auto first=item?std::uint64_t(out->field_ranges_[item-1].first)+out->field_ranges_[item-1].count:0;
          std::uint64_t capacity=range.count?2:0;
          auto base=kClientPlanNone;
          if(range.count&&std::uint64_t(range.base)-range.first+1<=std::uint64_t(range.count)*2){
            capacity=std::uint64_t(range.base)-range.first+1;base=range.first;
          }else while(capacity<std::uint64_t(range.count)*2)capacity*=2;
          Require(first+capacity<=UINT32_MAX,"client_plan_field_index_capacity");
          range={std::uint32_t(first),std::uint32_t(capacity),base};
        }
        if(item==out->field_ranges_.size()){
          if(item){const auto &last=out->field_ranges_.back();out->field_lookup_.resize(std::size_t(last.first)+last.count);}
          pass=Pass::FieldIndex;item=0;
        }
        break;
      }
      case Pass::FieldIndex:{
        const auto done=FreshStep([](auto &data)->auto &{return data.fields;},
          [&](const auto &field,std::uint32_t local,auto &){
            const auto &record=out->records_[scan_record];
            if(record.event!=kClientPlanNone||out->private_records_[record.object]==scan_record)return;
            const auto path=out->paths_[field.path].field_path;
            const auto range=out->field_ranges_[record.object];
            const auto index=record.first_field+local;
            Require(range.count!=0,"client_plan_field_index_capacity");
            if(range.base!=kClientPlanNone){
              Require(path>=range.base&&path-range.base<range.count,"client_plan_field_index_capacity");
              auto &entry=out->field_lookup_[range.first+path-range.base];
              if(entry.field==kClientPlanNone)entry={path,index};
              return;
            }
            auto slot=FieldBucket(path)&(range.count-1);
            for(std::uint32_t n=0;n<range.count;++n,slot=(slot+1)&(range.count-1)){
              auto &entry=out->field_lookup_[range.first+slot];
              if(entry.field==kClientPlanNone){entry={path,index};return;}
              if(entry.path==path)return;
            }
            Require(false,"client_plan_field_index_capacity");
          });
        if(done)pass=Pass::Rules;
        break;
      }
      case Pass::Rules:{
        const auto done=FreshStep([](auto &data)->auto &{return data.values;},
          [&](auto &value,std::uint32_t,auto &){
            const auto &record=out->records_[scan_record];
            if(!value.layout&&record.event==kClientPlanNone){
              const auto &path=out->paths_[value.path];
              const auto own_field=[&](std::uint32_t key){
                const auto index=out->CanonicalField(record.object,key);
                if(index==kClientPlanNone||out->private_records_[record.object]!=scan_record)return index;
                return index-record.first_field;
              };
              value.field=own_field(path.field_path);
              Require(value.field!=kClientPlanNone,"client_plan_value_field_missing");
              if(path.parent!=kClientPlanNone)value.parent_field=own_field(out->paths_[path.parent].field_path);
            }
            Rule(value);
          });
        if(done)pass=Pass::Transitions;
        break;
      }
      case Pass::Transitions:Transitions();pass=Pass::Commands;item=0;break;
      case Pass::Commands:
        if(CommandStep()){
          auto &metrics=tables.Memory()->ApplyMetrics();
          for(unsigned i=0;i<2;++i)for(unsigned n=0;n<4;++n)if(volume[i][n])
            metrics.Volume(std::uint8_t(1u<<i),ClientApplyVolume(n),volume[i][n]);
          for(unsigned i=0;i<2;++i)if(reused_records[i])
            metrics.Volume(std::uint8_t(1u<<i),ClientApplyVolume::PlanReusedRecords,reused_records[i]);
          if(reused_world_blocks)metrics.Volume(1,ClientApplyVolume::PlanReusedWorldBlocks,reused_world_blocks);
          if(shared_world_records)metrics.Volume(1,ClientApplyVolume::PlanSharedRecords,shared_world_records);
          pass=Pass::Done;
        }
        break;
      case Pass::Done:return true;
    }
    return pass==Pass::Done;
  }
};
void ClientInstallBuilder::World() {
  auto &w=out->world_;const auto &scope=target.candidate->Target().route.scope;
  const auto unique=[&](std::uint32_t &slot,std::uint32_t object){Require(slot==kClientPlanNone,"client_plan_duplicate_world_root");slot=object;};
  std::array<std::uint32_t,527> descriptors;descriptors.fill(kClientPlanNone);
  unsigned participants=0;
  for(std::uint32_t i=0;i<out->objects_.size();++i){const auto &o=out->objects_[i];const auto &id=o.member.identity;
    switch(o.domain) {
      case ClientInstallDomain::Run:Require(id.id==scope.run,"client_plan_run_identity");unique(w.run,i);break;
      case ClientInstallDomain::Level:Require(id.id==scope.floor,"client_plan_level_identity");unique(w.level,i);break;
      case ClientInstallDomain::Room:Require(id.id==scope.room,"client_plan_room_identity");unique(w.room,i);break;
      case ClientInstallDomain::Sfx:Require(id.id==scope.run,"client_plan_sfx_identity");unique(w.sfx,i);break;
      case ClientInstallDomain::Participant:Require(++participants<=4,"client_plan_participant_limit");break;
      case ClientInstallDomain::Descriptor:{
        // Host SceneSeed明确以物理槽+1建立稳定身份；不从可变ListIndex反推。
        Require(id.id>=1&&id.id<=527&&id.id!=526&&id.use==scope.floor,"client_plan_descriptor_identity");
        const auto slot=std::uint16_t(id.id-1);Require(descriptors[slot]==kClientPlanNone,"client_plan_descriptor_slot_conflict");descriptors[slot]=i;
        out->objects_[i].description=std::uint32_t(out->descriptors_.size());
        out->descriptors_.push_back({i,slot,slot<507?slot:kNullIndex});break;
      }
      default:break;
    }
  }
  Require(w.run!=kClientPlanNone&&w.level!=kClientPlanNone&&w.room!=kClientPlanNone&&w.sfx!=kClientPlanNone,"client_plan_world_roots_missing");
  // fast body只检查其真正依赖的完整world／Player拓扑，不重走地图与NPC资源图。
  if(!target.candidate->State(BodyStream::World))return;
  const auto room=Root(w.room),level=Root(w.level);w.grid_width=Number<std::uint32_t>(room,F::Room::grid_width);w.grid_height=Number<std::uint32_t>(room,F::Room::grid_height);
  Require(w.grid_width&&w.grid_height&&std::uint64_t(w.grid_width)*w.grid_height<=448,"client_plan_room_grid_extent");
  const auto slot=Number<std::int32_t>(room,F::Room::descriptor),dim=Number<std::int32_t>(room,F::Room::dimension);
  w.dimension=Number<std::uint8_t>(level,F::LevelGamePrefix::current_dimension);w.current_entry=Number<std::int32_t>(level,F::LevelGamePrefix::current_entry_index);
  Require(dim>=0&&dim<=2&&w.dimension==dim&&slot>=0&&slot<527&&slot!=525,"client_plan_current_descriptor_slot");
  w.current_descriptor=descriptors[std::size_t(slot)];Require(w.current_descriptor!=kClientPlanNone,"client_plan_current_descriptor_missing");
  unsigned ordinary=0;for(unsigned s=0;s<507;++s)if(descriptors[s]!=kClientPlanNone){Require(s==ordinary,"client_plan_ordinary_descriptor_gap");++ordinary;}
  for(const auto &d:out->descriptors_) {
    const auto r=Root(d.object);const auto dimension=Number<std::uint8_t>(r,F::RoomDescriptor::dimension);Require(dimension<=2,"client_plan_descriptor_dimension");
    const auto current=Number<std::uint8_t>(r,F::RoomDescriptor::spawn_seed_current);
    Require(current==unsigned(d.object==w.current_descriptor),"client_plan_descriptor_seed_owner");
    const auto *seed=r.Field(F::RoomDescriptor::spawn_seed);Require(seed&&seed->present==bool(current),"client_plan_descriptor_seed_presence");
    if(current)Require(dimension==w.dimension,"client_plan_current_descriptor_dimension");
    const auto data=Number<std::uint16_t>(r,F::RoomDescriptor::data),over=Number<std::uint16_t>(r,F::RoomDescriptor::override_data);
    Require(data!=kNullIndex||over!=kNullIndex,"client_plan_descriptor_configuration_missing");
    const auto doors=Exact<std::array<std::int32_t,8>>(Required(r,F::RoomDescriptor::doors));
    for(const auto door:doors)Require((door>=0&&door<169)||(door>=-20&&door<=-1&&door!=-19)||door==-100||door==-101,"client_plan_descriptor_door_target");
  }
  w.occupancy=Exact<std::array<std::uint16_t,507>>(Required(level,F::LevelGamePrefix::room_offset));
  for(std::size_t i=0;i<w.occupancy.size();++i)if(const auto s=w.occupancy[i];s!=kNullIndex) {
    Require(s<527&&s!=525&&descriptors[s]!=kClientPlanNone,"client_plan_occupancy_descriptor");
    Require(Number<std::uint8_t>(Root(descriptors[s]),F::RoomDescriptor::dimension)==i/169,"client_plan_occupancy_dimension");
  }
  if(w.current_entry>=0)Require(w.current_entry<169&&w.occupancy[std::size_t(w.dimension)*169+w.current_entry]==slot,"client_plan_entry_quadrant");
  else Require(w.current_entry>=-20&&w.current_entry!=-19&&506-w.current_entry==slot,"client_plan_special_entry");
  std::array<std::uint32_t,448> grids;grids.fill(kClientPlanNone);
  for(std::uint32_t i=0;i<out->objects_.size();++i)if(out->objects_[i].domain==ClientInstallDomain::Grid) {
    const auto root=Root(i);ClientInstallGrid g;g.object=i;g.index=Number<std::uint32_t>(root,F::GridEntity::grid_index);
    Require(g.index<w.grid_width*w.grid_height&&grids[g.index]==kClientPlanNone,"client_plan_grid_index_conflict");grids[g.index]=i;
    const auto *field=FindField(root.Identity().layout,F::GridEntity::desc);Require(field,"client_plan_grid_descriptor");const auto desc=Required(root,F::GridEntity::desc);
    g.type=Fixed<std::int32_t>(field->nested_layout,0,desc);g.variant=Fixed<std::int32_t>(field->nested_layout,1,desc);g.state=Fixed<std::int32_t>(field->nested_layout,2,desc);
    Require(GridFactoryLayout(g.type)==root.Identity().layout,"client_plan_grid_factory_type");
    if(field->nested_layout==198){g.has_spawn_seed=true;g.spawn_seed=Fixed<std::uint32_t>(field->nested_layout,3,desc);}
    if(root.Identity().layout==F::GridEntityFire::kLayout&&g.state>=4)Require(Exact<std::uint16_t>(Required(root,F::GridEntityFire::embers).first(2))==0,"client_plan_ember_state");
    if(root.Identity().layout==F::GridEntityRock::kLayout) {
      const auto count=Exact<std::uint16_t>(Required(root,F::GridEntityRock::rubble_offsets).first(2)),animation=Number<std::uint16_t>(root,F::GridEntityRock::rubble_animation);
      Require((g.state==2||(!count&&animation==kNullIndex))&&(!count||animation!=kNullIndex),"client_plan_rubble_state");
    }
    out->objects_[i].description=std::uint32_t(out->grids_.size());
    out->grids_.push_back(g);
  }
  const auto doors=Exact<std::array<std::uint16_t,8>>(Required(room,F::Room::door_slots));
  const auto positions=Exact<std::array<std::int32_t,8>>(Required(room,F::Room::door_grid_positions));
  for(unsigned i=0;i<8;++i)if(doors[i]!=kNullIndex) {
    const auto object=Reference(doors[i],target.effective[0]->Members());
    Require(object!=kClientPlanNone&&out->objects_[object].member.identity.layout==228&&positions[i]>=0&&std::uint32_t(positions[i])<w.grid_width*w.grid_height,"client_plan_room_door_binding");w.doors[i]=object;
  }
  const auto present=Number<std::uint8_t>(room,F::Room::wave_sprite_present);const auto *wave=room.Field(F::Room::shockwave_overlay_sprite);
  Require(wave&&wave->present==bool(present),"client_plan_wave_presence");
}
void ClientInstallBuilder::Players() {
  const auto world=target.effective[0];std::array<bool,4> ordinals{};std::array<bool,256> ranks{};unsigned rank_count=0;
  for(std::uint32_t i=0;i<out->objects_.size();++i)if(out->objects_[i].domain==ClientInstallDomain::ControlGroup) {
    const auto r=Root(i);ClientInstallGroup group;group.object=i;group.participant=Reference(r,F::PlayerControlGroup::participant);
    Require(group.participant!=kClientPlanNone&&out->objects_[group.participant].domain==ClientInstallDomain::Participant,"client_plan_group_participant");
    group.ordinal=Number<std::uint8_t>(r,F::PlayerControlGroup::ordinal);group.session=Number<std::uint64_t>(r,F::PlayerControlGroup::control_session);group.binding=Number<std::uint64_t>(r,F::PlayerControlGroup::binding_revision);
    Require(group.ordinal<4&&!ordinals[group.ordinal]&&group.session&&group.binding,"client_plan_group_ordinal_or_binding");ordinals[group.ordinal]=true;
    for(const auto &prior:out->groups_)Require(prior.participant!=group.participant&&prior.session==group.session,"client_plan_group_participant_conflict");
    const auto anchors=Required(r,F::PlayerControlGroup::active_anchors);const auto count=Exact<std::uint16_t>(anchors.first(2));
    Require(count<=64&&anchors.size()==2u+2u*count,"client_plan_anchor_count");group.first_anchor=std::uint32_t(out->anchors_.size());group.anchor_count=count;
    for(unsigned n=0;n<count;++n){const auto object=Reference(Exact<std::uint16_t>(anchors.subspan(2+n*2,2)),world->Members());
      Require(object!=kClientPlanNone&&out->objects_[object].domain==ClientInstallDomain::Player,"client_plan_anchor_player");
      Require(std::find(out->anchors_.begin(),out->anchors_.end(),object)==out->anchors_.end(),"client_plan_duplicate_anchor");out->anchors_.push_back(object);}
    if(const auto previous=target.previous[0];previous&&previous->Context().scope.session==world->Context().scope.session) {
      if(const auto old=previous->Find(r.Identity())) {
        const auto *participant=FindStateMember(previous->Members()->Directory(),Number<std::uint16_t>(*old,F::PlayerControlGroup::participant));
        Require(participant&&participant->identity==out->objects_[group.participant].member.identity&&Number<std::uint8_t>(*old,F::PlayerControlGroup::ordinal)==group.ordinal&&
          Number<std::uint64_t>(*old,F::PlayerControlGroup::control_session)==group.session&&Number<std::uint64_t>(*old,F::PlayerControlGroup::binding_revision)<=group.binding,"client_plan_group_assignment_changed");
      }
    }
    out->groups_.push_back(group);
  }
  std::array<unsigned,4> bodies{};
  for(std::uint32_t i=0;i<out->objects_.size();++i)if(out->objects_[i].domain==ClientInstallDomain::Player) {
    Require(out->players_.size()<256,"client_plan_player_limit");const auto r=Root(i);ClientInstallPlayer player;player.object=i;
    player.player_type=Number<std::int32_t>(r,F::EntityPlayer::_playerType);
    // World 先建立本帧的服装目录，Body 应用时比对活对象的对应拓扑。
    player.group=Reference(r,F::EntityPlayer::control_group);player.topology=Number<std::uint32_t>(r,F::EntityPlayer::costume_topology_revision);
    // Host 为每个身体独立记录目录修订。结构变更由上面的 joint 规则协调，
    // 同一目标里的 P0/P6 必须按身体身份配对，其他身体可以仍在各自的旧修订。
    Require(player.topology!=0,"client_plan_player_costume_topology");
    player.membership=Number<std::uint8_t>(r,F::EntityPlayer::native_membership);player.role=Number<std::uint8_t>(r,F::EntityPlayer::body_role);player.control=Number<std::uint8_t>(r,F::EntityPlayer::control_flags);player.roster=Number<std::uint16_t>(r,F::EntityPlayer::roster_rank);
    Require(player.membership<=15&&(!(player.membership&2)||(player.membership&1))&&(!(player.membership&12)||(player.membership&2)),"client_plan_player_membership");
    Require(player.role<=6&&player.control<=3&&(!(player.control&2)||(player.control&1)),"client_plan_player_control");
    if(player.group==kClientPlanNone)Require(player.role==6&&!player.control&&player.roster==kNullIndex,"client_plan_ungrouped_player");
    else {
      const auto g=std::find_if(out->groups_.begin(),out->groups_.end(),[&](const auto &v){return v.object==player.group;});Require(g!=out->groups_.end(),"client_plan_player_group");
      Require(++bodies[g->ordinal]<=64,"client_plan_group_body_limit");
      const auto first=out->anchors_.begin()+g->first_anchor,last=first+g->anchor_count;
      Require(std::find(first,last,i)==last||bool(player.control&1),"client_plan_inactive_anchor");
    }
    if(player.roster!=kNullIndex){Require(player.roster<256&&!ranks[player.roster],"client_plan_roster_duplicate");ranks[player.roster]=true;++rank_count;}
    if(target.candidate->Target().count==2){
      const auto body=target.effective[1];
      const auto pose=body?body->Find(r.Identity()):std::nullopt;
      if(!pose)Fail(StateReadiness::SourcePending,"client_plan_player_pose_missing");
      if(Number<std::uint32_t>(*pose,F::EntityPlayer::render_topology_revision)!=player.topology)
        Fail(StateReadiness::SourcePending,"client_plan_player_pose_topology");
    }
    out->players_.push_back(player);
  }
  for(unsigned n=0;n<rank_count;++n)Require(ranks[n],"client_plan_roster_gap");
  for(const auto &g:out->groups_)for(auto n=g.first_anchor;n<g.first_anchor+g.anchor_count;++n) {
    const auto p=std::find_if(out->players_.begin(),out->players_.end(),[&](const auto &v){return v.object==out->anchors_[n];});
    Require(p!=out->players_.end()&&p->group==g.object&&(p->control&1),"client_plan_anchor_group_or_active");
  }
  if(target.candidate->State(BodyStream::World))for(std::uint32_t i=0;i<out->objects_.size();++i) {
    const auto domain=out->objects_[i].domain;if(domain!=ClientInstallDomain::Entity&&domain!=ClientInstallDomain::Player)continue;
    const auto r=Root(i);BodySourceFieldInfo config;Require(DescribeBodySourceField(r.Identity().layout,F::Entity::selected_config,config),"client_plan_entity_configuration_plan");
    out->objects_[i].description=std::uint32_t(out->entities_.size());
    out->entities_.push_back({i,Number<std::uint32_t>(r,F::Entity::type),Number<std::uint32_t>(r,F::Entity::variant),Number<std::uint32_t>(r,F::Entity::subtype),
      Number<std::uint32_t>(r,F::Entity::init_seed),Number<std::uint32_t>(r,F::Entity::authority_order),{config.resource_table,Number<std::uint16_t>(r,F::Entity::selected_config)}});
  }
}
void ClientInstallBuilder::Event(std::size_t i) {
  const auto &source=target.events[i];const auto &event=source.value;const auto kind=StateEventKind(event.purpose);
  Require((out->events_.empty()||event.sequence>out->events_.back().sequence)&&StateEventLayout(kind)==event.layout,"client_plan_event_order_or_kind");
  ClientInstallEvent e;e.record=std::uint32_t(out->records_.size());e.kind=kind;e.sequence=event.sequence;e.game_frame=event.logical_frame;
  if(event.owner!=kNullIndex){
    const auto *owner=FindStateMember(source.members->Directory(),event.owner);
    Require(owner,"client_plan_event_owner");
    e.owner_object=Object(owner->identity);
  }
  out->events_.push_back(e);ClientInstallRecord r;r.event=std::uint32_t(i);r.layout=event.layout;r.stream=BodyStream::Value;
  r.context=target.effective[0]->Context();r.context.member_epoch=source.members->Directory().epoch;r.context.resource_epoch=source.resources->Directory().epoch;
  r.members=source.members;r.resources=source.resources;r.mask=1;r.bytes={std::uint32_t(out->bytes_.size()),std::uint32_t(event.payload.size())};
  out->bytes_.insert(out->bytes_.end(),event.payload.begin(),event.payload.end());
  OpenRecord(r);NewRecordData();
  decoding=out->Bytes(r.bytes);ResourceBodyContext context(source.resources->Directory());
  Decode([&]{BodyCodecScratch scratch{facts,0,{},0,this};const auto result=DecodeValue(r.layout,decoding,scratch,&context,{256u<<10,262144-visits,32,true});visits+=result.field_visits;});
  CloseRecord();
}
void ClientInstallBuilder::Rule(const ClientInstallValue &v) {
    if(!v.layout)return;
    const auto &r=out->records_[v.record];
    switch(v.layout) {
      case F::GridFireEmber::kLayout:{
        const auto age=AtNumber<std::int32_t>(v,F::GridFireEmber::age),life=AtNumber<std::int32_t>(v,F::GridFireEmber::max_life);
        Require(age>=0&&age<life&&life<=29,"client_plan_ember_lifetime");break;
      }
      case F::PlayerCostumeMap::kLayout:{
        const auto index=AtNumber<std::uint16_t>(v,F::PlayerCostumeMap::index);if(index==kNullIndex)break;
        const auto costume=Required(Root(r.object),F::EntityPlayer::_costumeSpriteDescs_0);
        Require(index<Exact<std::uint16_t>(costume.first(2)),"client_plan_costume_map_index");break;
      }
      case F::FamiliarUmbilicalBabyData::kLayout:{
        const auto f=field_index.find({v.record,v.path,v.layout,F::FamiliarUmbilicalBabyData::rope_nodes});
        // 首Sprite等其它投影没有rope_nodes；实际绳索投影的非空owner至少一个节点。
        if(f!=field_index.end())Require(out->Field(f->second).elements>=1,"client_plan_umbilical_rope_empty");break;
      }
      case F::MusicRequestState::kLayout:{
        const auto revision=AtNumber<std::uint64_t>(v,F::MusicRequestState::revision),generation=AtNumber<std::uint64_t>(v,F::MusicRequestState::generation);
        const auto id=AtNumber<std::int32_t>(v,F::MusicRequestState::id),queued=AtNumber<std::int32_t>(v,F::MusicRequestState::queued);
        Require(revision&&(!id||generation),"client_plan_music_generation");
        if(id)out->native_needs_.push_back({ClientInstallNativeNeedKind::Music,v.record,id,0,generation});
        if(queued)out->native_needs_.push_back({ClientInstallNativeNeedKind::Music,v.record,queued,0,revision});
        if(AtNumber<std::uint8_t>(v,F::MusicRequestState::jingle_active)) {
          const auto jingle=AtNumber<std::int32_t>(v,F::MusicRequestState::jingle_id);const auto gen=AtNumber<std::uint64_t>(v,F::MusicRequestState::jingle_generation);
          Require(jingle&&gen,"client_plan_jingle_generation");out->native_needs_.push_back({ClientInstallNativeNeedKind::MusicJingle,v.record,jingle,0,gen});
        }break;
      }
      case F::LoopSoundSet::kLayout:Require(AtNumber<std::uint64_t>(v,F::LoopSoundSet::revision)!=0,"client_plan_loop_revision");break;
      case F::LoopSoundState::kLayout:{
        const auto instance=AtNumber<std::uint64_t>(v,F::LoopSoundState::instance),generation=AtNumber<std::uint64_t>(v,F::LoopSoundState::generation);
        Require(instance&&generation&&loops.emplace(instance,v.record).second,"client_plan_duplicate_loop_instance");
        Sample(v,F::LoopSoundState::asset);break;
      }
      case F::OverlayDisplayState::kLayout:{
        const auto player=AtNumber<std::uint16_t>(v,F::OverlayDisplayState::player);const auto controller=AtNumber<std::int32_t>(v,F::OverlayDisplayState::controller);
        const auto state=AtNumber<std::int32_t>(v,F::OverlayDisplayState::state);const auto generation=AtNumber<std::uint64_t>(v,F::OverlayDisplayState::generation);
        Require(state>=0&&state<=2&&controller>=-1&&controller<64&&(!state||generation),"client_plan_overlay_state_or_generation");
        if(player!=kNullIndex){const auto *m=FindStateMember(r.members->Directory(),player);Require(m&&m->identity.space==1&&m->identity.layout==197,"client_plan_overlay_player");}
        Require((controller==-1||!state||player!=kNullIndex)&&overlays.emplace(unsigned(controller+1),v.record).second,
          "client_plan_duplicate_or_unbound_overlay_owner");break;
      }
      case F::ProceduralDisplayDefinition::kLayout:{
        const auto id=AtNumber<std::int32_t>(v,F::ProceduralDisplayDefinition::id);
        Require(id==~std::int32_t(out->Paths()[v.path].step.element)&&id>=-1024&&definitions.emplace(id,v.record).second,
          "client_plan_procedural_definition_index");
        const auto gfx=out->Bytes(At(v.record,v.path,v.layout,F::ProceduralDisplayDefinition::gfx).bytes);
        Require((gfx.size()==2&&Exact<std::uint16_t>(gfx)==0)||StateImagePathValid(gfx),"client_plan_procedural_gfx");
        const auto resource=AtNumber<std::uint16_t>(v,F::ProceduralDisplayDefinition::configuration);BodySourceFieldInfo f;
        Require(resource!=kNullIndex&&DescribeBodySourceField(v.layout,F::ProceduralDisplayDefinition::configuration,f),"client_plan_procedural_configuration");
        const auto config=ResourceBodyContext(r.resources->Directory()).Item(f.resource_table,resource);
        Require(config&&config->id==std::uint32_t(id),"client_plan_procedural_configuration_identity");break;
      }
      case F::SfxPlayEvent::kLayout:Sample(v,F::SfxPlayEvent::asset);break;
      case F::FortuneEvent::kLayout:Require(AtNumber<std::int32_t>(v,F::FortuneEvent::index)>=-1,"client_plan_fortune_index");break;
      case F::ItemBannerEvent::kLayout:{
        const auto style=AtNumber<std::uint8_t>(v,F::ItemBannerEvent::style);const auto config=AtNumber<std::uint16_t>(v,F::ItemBannerEvent::configuration);
        Require(style<=1&&((style==0)==(config!=kNullIndex)),"client_plan_banner_style_or_configuration");
        const auto player=AtNumber<std::uint16_t>(v,F::ItemBannerEvent::player);
        if(player!=kNullIndex){const auto *m=FindStateMember(r.members->Directory(),player);Require(m&&m->identity.space==1&&m->identity.layout==197,"client_plan_banner_player");}break;
      }
      case F::BossDisplayState::kLayout:{
        using namespace F::BossDisplayState;
        const auto gen=AtNumber<std::uint64_t>(v,generation);
        const auto first=AtNumber<std::uint32_t>(v,boss_id1),second=AtNumber<std::uint32_t>(v,boss_id2);
        const auto room=AtNumber<std::int32_t>(v,room_index),dim=AtNumber<std::int32_t>(v,dimension);
        Require(gen?first&&room>=0&&room<169&&dim>=0&&dim<3:!first&&!second&&!room&&!dim,"client_plan_boss_baseline");
        if(gen)out->native_needs_.push_back({ClientInstallNativeNeedKind::BossIntro,v.record,first,second,gen});
        break;
      }
      case F::BossIntroEvent::kLayout:{
        const auto dimension=AtNumber<std::int32_t>(v,F::BossIntroEvent::dimension);Require(dimension>=0&&dimension<=2,"client_plan_boss_dimension");
        auto &e=out->events_[r.event];e.transition=AtNumber<std::uint64_t>(v,F::BossIntroEvent::transition);
        break;
      }
      case F::RoomTransitionEvent::kLayout:{
        auto &e=out->events_[r.event];e.transition=AtNumber<std::uint64_t>(v,F::RoomTransitionEvent::transition);
        const auto done=AtNumber<std::uint8_t>(v,F::RoomTransitionEvent::completed);const auto room=AtNumber<std::uint64_t>(v,F::RoomTransitionEvent::final_room);
        const auto dimension=AtNumber<std::int32_t>(v,F::RoomTransitionEvent::dimension),final=AtNumber<std::int32_t>(v,F::RoomTransitionEvent::final_dimension);
        // 分别记录已接受维度、实际房间维度与完成身份的校验失败，便于定位原生生产边界。
        Require(e.transition&&done<=1&&bool(done)==bool(room),"client_plan_transition_scope_or_completion");
        Require(dimension>=0&&dimension<=2,"client_plan_transition_requested_dimension");
        Require(final>=0&&final<=2,"client_plan_transition_final_dimension");
        const auto player=AtNumber<std::uint16_t>(v,F::RoomTransitionEvent::player);
        if(player!=kNullIndex){const auto *m=FindStateMember(r.members->Directory(),player);Require(m&&m->identity.space==1&&m->identity.layout==197,"client_plan_transition_player");}break;
      }
      default:return;
    }
    // 有独立规则的记录每批仍参与校验，包括全局唯一性检查。
    fresh[v.record]->shareable=false;
}
void ClientInstallBuilder::Transitions() {
  // 转场关联以Host序号建索引；开始／完成同时存在时，其请求元组必须一致。
  WorkMap<std::uint64_t> transitions{&index_memory};
  for(std::uint32_t i=0;i<out->events_.size();++i) {
    const auto &event=out->events_[i];if(!event.transition)continue;
    auto [at,inserted]=transitions.emplace(event.transition,std::uint32_t(out->transitions_.size()));
    if(inserted)out->transitions_.push_back({event.transition});
    if(event.kind!=StateEventKind::RoomTransition)continue;
    const auto &done=At(event.record,kClientPlanNone,F::RoomTransitionEvent::kLayout,F::RoomTransitionEvent::completed);
    auto &pair=out->transitions_[at->second];auto &slot=Exact<std::uint8_t>(out->Bytes(done.bytes))?pair.finish:pair.start;
    Require(slot==kClientPlanNone,"client_plan_duplicate_transition_phase");slot=i;
  }
  for(const auto &pair:out->transitions_)if(pair.start!=kClientPlanNone&&pair.finish!=kClientPlanNone) {
    const auto &start=out->events_[pair.start],&finish=out->events_[pair.finish];Require(start.sequence<finish.sequence,"client_plan_transition_phase_order");
    for(const auto field:{F::RoomTransitionEvent::requested_index,F::RoomTransitionEvent::direction,F::RoomTransitionEvent::animation,F::RoomTransitionEvent::dimension}) {
      const auto a=out->Bytes(At(start.record,kClientPlanNone,F::RoomTransitionEvent::kLayout,field).bytes),b=out->Bytes(At(finish.record,kClientPlanNone,F::RoomTransitionEvent::kLayout,field).bytes);
      Require(std::equal(a.begin(),a.end(),b.begin(),b.end()),"client_plan_transition_request_conflict");
    }
  }
}
bool ClientInstallBuilder::CommandStep() {
  // 只有两个实际消费的暂存列表需要封存；写段按安装器自己的原生阶段排序。
  if(!command){
    const auto n=out->objects_.size();
    const auto end=(std::min)(n*(unsigned(ClientInstallDomain::Sfx)+1),item+128);
    for(;item<end;++item){
      const auto index=std::uint32_t(item%n);
      const auto &object=out->objects_[index];
      if(unsigned(object.domain)==item/n&&(object.streams||object.action!=ClientInstallAction::Reuse))
        out->stage_objects_.push_back(index);
    }
    if(item==n*(unsigned(ClientInstallDomain::Sfx)+1)){++command;item=0;}
    return false;
  }
  // 延续原 StageFields 的逆字段顺序，只拼接各记录已封存的暂存项。
  const auto count=out->records_.size();
  const auto end=(std::min)(count,item+128);
  for(;item<end;++item){
    const auto record=count-1-item;
    const auto base=out->records_[record].first_field;
    const auto &fields=out->data_[record]->stage_fields;
    for(auto i=fields.rbegin();i!=fields.rend();++i)out->stage_fields_.push_back(base+*i);
  }
  return item==count;
}
// 等待槽持有构建器与其 arena。每个工作片重新取得同一进程的执行租约。
struct ClientInstallPlanWork final {
  ClientInstallBuilder builder;
  ClientInstallPlanWork(const ClientStatePreparedTarget &target,const ClientStateTables &tables,
      const std::array<std::weak_ptr<const ClientStateInstallPlan>,2> &accepted):builder(target,tables,accepted) {}
};
std::shared_ptr<const ClientStateInstallPlan> BuildClientStateInstallPlanStep(const ClientStatePreparedTarget &target,
    const ClientStateTables &tables,std::shared_ptr<ClientInstallPlanWork> &work,std::chrono::steady_clock::time_point stop,
    const std::array<std::weak_ptr<const ClientStateInstallPlan>,2> &accepted) {
  ClientStateWorkspace workspace;
  try{
    if(!work)work=std::allocate_shared<ClientInstallPlanWork>(ClientStateAllocator<ClientInstallPlanWork>(tables.Memory()),target,tables,accepted);
#ifdef ISAACCS_E2E
    struct Slice {
      ClientInstallBuilder::Timing timing;
      ClientApplyMetrics &metrics;
      std::uint8_t streams;
      ~Slice(){timing.Report(metrics,streams);}
    } slice{{},tables.Memory()->ApplyMetrics(),StateTargetStreams(target.candidate->Target())};
#endif
    do{
      if(work->builder.Step(
#ifdef ISAACCS_E2E
          &slice.timing
#endif
          )){auto plan=work->builder.out;work.reset();return plan;}
    }while(std::chrono::steady_clock::now()<stop);
    return {};
  }catch(const BodyVisitError &){work.reset();Fail(StateReadiness::Budget,"client_plan_visit_budget");}
   catch(const BodyDependencyError &){work.reset();Fail(StateReadiness::SourcePending,"client_plan_body_dependency");}
   catch(const Gap &){work.reset();Fail(StateReadiness::InvalidEncoding,"client_plan_invalid_body");}
   catch(...){work.reset();throw;}
}
std::shared_ptr<const ClientStateInstallPlan> BuildClientStateInstallPlan(const ClientStatePreparedTarget &target,const ClientStateTables &tables) {
  std::shared_ptr<ClientInstallPlanWork> work;
  return BuildClientStateInstallPlanStep(target,tables,work,std::chrono::steady_clock::time_point::max());
}

} // namespace cs::state
