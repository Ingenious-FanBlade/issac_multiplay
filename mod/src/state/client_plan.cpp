#include "state/client_plan.h"
#include <algorithm>
#include <cstring>
#include <map>
#include <tuple>

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
// 共享投影只向同一完整目标／配对成功根查询，资源条件只查精确快照。
class Context final:public BodyCodecContext {
public:
  Context(ClientResourceSnapshotRef resources,ClientShadowRecordView own,std::optional<ClientShadowRecordView> other)
    :resources_(std::move(resources)),own_(own),other_(other) {}
  std::optional<CodecItemConfig> Item(std::uint8_t table,std::uint16_t index)const override{return ResourceBodyContext(resources_->Directory()).Item(table,index);}
  std::optional<bool> SharedPresence(const CodecLocation &location)const override {
    const auto read=[&](ClientShadowRecordView r)->std::optional<bool>{for(const auto &f:r.Projections())if(f.kind==ProjectionFactKind::SharedPresence&&
      f.layout==location.layout&&f.group==location.canonical_field&&f.depth==location.parents.size()&&std::equal(location.parents.begin(),location.parents.end(),f.parents.begin()))return f.value!=0;return {};};
    if(const auto v=read(own_))return v;return other_?read(*other_):std::nullopt;
  }
private:ClientResourceSnapshotRef resources_;ClientShadowRecordView own_;std::optional<ClientShadowRecordView> other_;
};
template<class K>using WorkMap=std::map<K,std::uint32_t,std::less<K>,ClientStateAllocator<std::pair<const K,std::uint32_t>>>;
using PathKey=std::tuple<std::uint32_t,std::uint16_t,std::uint16_t,std::uint16_t,std::uint16_t>;
using FieldKey=std::tuple<std::uint32_t,std::uint32_t,std::uint16_t,std::uint16_t>;
}
bool ClientCostumeProjection(std::uint16_t layout,std::uint16_t field) noexcept {
  return layout==F::EntityPlayer::kLayout&&(field==F::EntityPlayer::_costumeSpriteDescs_0||
    field==F::EntityPlayer::_costumeSpriteDescs_1||field==F::EntityPlayer::_costumeSpriteDescs_2||field==F::EntityPlayer::_costumeSpriteDescs_3);
}
ClientStateInstallPlan::ClientStateInstallPlan(const std::shared_ptr<ClientStateMemory> &m)
  :objects_(ClientStateAllocator<ClientInstallObject>(m)),retired_(ClientStateAllocator<StateMemberEntry>(m)),
   records_(ClientStateAllocator<ClientInstallRecord>(m)),paths_(ClientStateAllocator<ClientInstallPath>(m)),
   fields_(ClientStateAllocator<ClientInstallField>(m)),values_(ClientStateAllocator<ClientInstallValue>(m)),
   bindings_(ClientStateAllocator<ClientInstallBinding>(m)),resources_(ClientStateAllocator<ClientStateResourceNeed>(m)),
   descriptors_(ClientStateAllocator<ClientInstallDescriptor>(m)),grids_(ClientStateAllocator<ClientInstallGrid>(m)),
   entities_(ClientStateAllocator<ClientInstallEntity>(m)),
   players_(ClientStateAllocator<ClientInstallPlayer>(m)),groups_(ClientStateAllocator<ClientInstallGroup>(m)),
   anchors_(ClientStateAllocator<std::uint32_t>(m)),events_(ClientStateAllocator<ClientInstallEvent>(m)),
   transitions_(ClientStateAllocator<ClientInstallTransition>(m)),
   native_needs_(ClientStateAllocator<ClientInstallNativeNeed>(m)),
   commands_(ClientStateAllocator<ClientInstallCommand>(m)),bytes_(ClientStateAllocator<std::byte>(m)) {
  world_.doors.fill(kClientPlanNone);world_.occupancy.fill(kNullIndex);
}
std::span<const std::byte> ClientStateInstallPlan::Bytes(ClientInstallSlice s)const {
  if(s.offset>bytes_.size()||s.size>bytes_.size()-s.offset)Fail(StateReadiness::InvalidEncoding,"client_plan_slice");return std::span(bytes_).subspan(s.offset,s.size);
}
ClientShadowStateRef ClientStateInstallPlan::Effective(BodyStream stream)const {
  if(unsigned(stream)>1)Fail(StateReadiness::InvalidEncoding,"client_plan_stream");return effective_[unsigned(stream)];
}
// 构建器唯一可变；发布后不修改路径、正文、资源顺序或执行描述。
struct ClientInstallBuilder final:BodyNodeObserver {
  const ClientStatePreparedTarget &target;
  const ClientStateTables &tables;
  const ClientStateWorkspace &workspace;
  std::shared_ptr<ClientStateInstallPlan> out;
  WorkMap<PathKey> path_index;
  WorkMap<FieldKey> field_index;
  ClientStateVector<ProjectionFact> facts;
  std::uint32_t record=0,visits=0;
  std::span<const std::byte> decoding;
  ClientInstallBuilder(const ClientStatePreparedTarget &t,const ClientStateTables &s,const ClientStateWorkspace &w)
    :target(t),tables(s),workspace(w),out(std::allocate_shared<ClientStateInstallPlan>(ClientStateAllocator<ClientStateInstallPlan>(s.Memory()),s.Memory())),
     path_index(ClientStateAllocator<std::pair<const PathKey,std::uint32_t>>(s.Memory(),memory::Pool::Workspace)),
     field_index(ClientStateAllocator<std::pair<const FieldKey,std::uint32_t>>(s.Memory(),memory::Pool::Workspace)),
     facts(ClientStateAllocator<ProjectionFact>(s.Memory(),memory::Pool::Workspace)) {facts.resize(16);}
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
      if(out->paths_.size()>=262144)Fail(StateReadiness::Budget,"client_plan_path_budget");
      const auto next=std::uint32_t(out->paths_.size());out->paths_.push_back({parent,step});path_index.emplace(key,next);parent=next;
    }
    return parent;
  }
  ClientInstallSlice Slice(std::span<const std::byte> bytes)const {
    if(bytes.empty())return {};
    const auto a=reinterpret_cast<std::uintptr_t>(decoding.data()),b=reinterpret_cast<std::uintptr_t>(bytes.data());
    if(b<a||b-a>decoding.size()||bytes.size()>decoding.size()-(b-a))Fail(StateReadiness::InvalidEncoding,"client_plan_unowned_node");
    return {out->records_[record].bytes.offset+std::uint32_t(b-a),std::uint32_t(bytes.size())};
  }
  void Field(const BodyFieldNodeView &node)override {
    if(out->records_[record].paired_costume&&(node.path.empty()||!ClientCostumeProjection(node.path.front().layout,node.path.front().field)))return;
    Require(!node.path.empty(),"client_plan_field_path");const auto path=Path(node.path);const auto &step=node.path.back();
    const auto *definition=FindField(step.layout,step.field);const auto *handler=FindClientField(step.layout,step.field);
    Require(definition&&handler&&handler->operation==definition->operation,"client_plan_field_dispatch_missing");
    if(out->fields_.size()>=262144)Fail(StateReadiness::Budget,"client_plan_field_budget");
    auto phase=ClientInstallPhase::Values;const auto &r=out->records_[record];
    if(r.event!=kClientPlanNone)phase=ClientInstallPhase::Events;
    else {
      const auto domain=out->objects_[r.object].domain;const auto *root=FindField(node.path.front().layout,node.path.front().field);
      if(domain==ClientInstallDomain::Run||domain==ClientInstallDomain::Sfx||
        (domain==ClientInstallDomain::Room&&root->block>=1&&root->block!=3))phase=ClientInstallPhase::Display;
      BodySourceFieldInfo format;Require(DescribeBodySourceField(step.layout,step.field,format),"client_plan_field_format");
      if(format.linked&&format.resource_table==255)phase=ClientInstallPhase::RelationFields;
    }
    const auto n=std::uint32_t(out->fields_.size());out->fields_.push_back({record,path,std::uint32_t(definition-state::Fields().data()),node.elements,node.slot_mask,Slice(node.bytes),node.present,node.payload_present,phase});
    if(!field_index.emplace(FieldKey{record,out->paths_[path].parent,step.layout,step.field},n).second)
      Fail(StateReadiness::InvalidEncoding,"client_plan_duplicate_field");
  }
  void Node(const BodyNodeView &node)override {
    if(out->records_[record].paired_costume&&(node.path.empty()||!ClientCostumeProjection(node.path.front().layout,node.path.front().field)))return;
    if(out->values_.size()>=262144)Fail(StateReadiness::Budget,"client_plan_value_budget");
    const auto path=Path(node.path),value=std::uint32_t(out->values_.size());
    out->values_.push_back({record,path,node.layout,node.component,node.operation,Slice(node.bytes)});
    if(node.layout||(node.operation!=Operation::Reference&&node.operation!=Operation::Resource))return;
    const auto index=Exact<std::uint16_t>(node.bytes);ClientInstallBinding link;link.value=value;link.is_null=index==kNullIndex;link.is_resource=node.operation==Operation::Resource;
    if(link.is_resource)link.resource={node.resource_table,index};
    else if(!link.is_null) {
      const auto *member=FindStateMember(out->records_[record].members->Directory(),index);Require(member,"client_plan_binding_member");
      link.member=member->identity;link.object=Object(link.member);
      if(link.object==kClientPlanNone)Fail(out->records_[record].event==kClientPlanNone?StateReadiness::InvalidReference:
        (StateEventCosmetic(out->events_[out->records_[record].event].kind)?StateReadiness::SourcePending:StateReadiness::EventGap),"client_plan_binding_retired_target");
    }
    out->bindings_.push_back(link);
  }
  // 从数字实例路径查询已定界字段；不会在主线程重解变长头或按名称找字段。
  const ClientInstallField &At(std::uint32_t rec,std::uint32_t parent,std::uint16_t layout,std::uint16_t field)const {
    const auto i=field_index.find({rec,parent,layout,field});if(i==field_index.end())Fail(StateReadiness::SourcePending,"client_plan_nested_field_missing");return out->fields_[i->second];
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
    const auto np=out->paths_.size(),nf=out->fields_.size(),nv=out->values_.size(),nb=out->bindings_.size();
    for(;;)try{fn();return;}catch(const BodyScratchError &e) {
      if(e.Region()!=BodyScratchRegion::Facts||e.Requested()>262144)throw;
      for(auto i=path_index.begin();i!=path_index.end();)if(i->second>=np)i=path_index.erase(i);else ++i;
      for(auto i=field_index.begin();i!=field_index.end();)if(i->second>=nf)i=field_index.erase(i);else ++i;
      out->paths_.resize(np);out->fields_.resize(nf);out->values_.resize(nv);out->bindings_.resize(nb);
      facts.resize(std::max<std::size_t>(e.Requested(),facts.size()*2));
    }
  }
  void Objects() {
    out->candidate_=target.candidate;out->effective_=target.effective;
    Require(target.candidate&&target.effective[0],"client_plan_world_context_missing");
    const auto &scope=target.candidate->Target().route.scope;
    for(unsigned stream=0;stream<2;++stream)if(const auto state=target.effective[stream]) {
      Require(state->Context().scope==scope&&state->Context().stream==BodyStream(stream),"client_plan_effective_scope");
      if(const auto next=target.candidate->State(BodyStream(stream)))Require(next==state,"client_plan_effective_not_candidate");
    }
    for(const auto &member:target.effective[0]->Members()->Directory().entries)out->objects_.push_back({member,Domain(member.identity)});
    std::sort(out->objects_.begin(),out->objects_.end(),[](const auto &a,const auto &b){return a.member.identity<b.member.identity;});
    for(const auto &change:target.candidate->Changes()) {
      if(change.kind==ClientShadowChangeKind::Leave){out->retired_.push_back(change.before);continue;}
      const auto i=Object(change.after.identity);Require(i!=kClientPlanNone,"client_plan_change_target");
      out->objects_[i].action=change.kind==ClientShadowChangeKind::Join?ClientInstallAction::Join:
        change.kind==ClientShadowChangeKind::Reshape?ClientInstallAction::Reshape:ClientInstallAction::Remap;
      if(change.kind==ClientShadowChangeKind::Reshape)out->retired_.push_back(change.before);
    }
    for(unsigned s=0;s<target.candidate->Target().count;++s){const auto &section=target.candidate->Target().sections[s];
      if(section.stream==BodyStream::World)out->world_.replace_world=section.base_version==0;else out->world_.replace_body=section.base_version==0;}
    for(unsigned stream=0;stream<2;++stream)if(const auto state=target.candidate->State(BodyStream(stream)))for(std::size_t i=0;i<state->RecordCount();++i) {
      const auto root=state->Record(i);const auto object=Object(root.Identity());Require(object!=kClientPlanNone,"client_plan_record_object");
      out->objects_[object].streams|=std::uint8_t(1u<<stream);
    }
  }
  void Records() {
    for(unsigned stream=0;stream<2;++stream)if(const auto state=target.effective[stream])for(std::size_t i=0;i<state->RecordCount();++i) {
      const auto root=state->Record(i);const bool paired=!target.candidate->State(BodyStream(stream));
      // 单流仍须为本次Player构造完整costume。配对流的其它字段和已离场对象不重装。
      const auto object=Object(root.Identity());
      if(paired&&(root.Identity().layout!=F::EntityPlayer::kLayout||object==kClientPlanNone||!out->objects_[object].streams))continue;
      ClientInstallRecord r;r.object=object;r.layout=root.Identity().layout;r.stream=BodyStream(stream);r.paired_costume=paired;
      r.context=state->Context();r.members=state->Members();r.resources=state->Resources();r.mask=root.Mask();r.bytes.offset=std::uint32_t(out->bytes_.size());
      if(paired){
        r.mask=0;
        for(const auto field:{F::EntityPlayer::_costumeSpriteDescs_0,F::EntityPlayer::_costumeSpriteDescs_1,
            F::EntityPlayer::_costumeSpriteDescs_2,F::EntityPlayer::_costumeSpriteDescs_3}){
          const auto *definition=FindField(F::EntityPlayer::kLayout,field);Require(definition,"client_plan_costume_definition");
          const auto bit=std::uint32_t(1)<<definition->block;if(root.Mask()&bit)r.mask|=bit;
        }
        Require(r.mask!=0,"client_plan_costume_projection_missing");
      }
      for(unsigned b=0;b<32;++b)if(r.mask&(std::uint32_t(1)<<b))if(const auto block=root.Block(std::uint8_t(b))) {
        const auto offset=std::uint32_t(out->bytes_.size());out->bytes_.insert(out->bytes_.end(),block->bytes.begin(),block->bytes.end());
        r.blocks[b]={block->version,block->game_frame,block->source_revision,{offset,std::uint32_t(block->bytes.size())}};
      }
      r.bytes.size=std::uint32_t(out->bytes_.size())-r.bytes.offset;r.first_field=std::uint32_t(out->fields_.size());
      record=std::uint32_t(out->records_.size());out->records_.push_back(r);decoding=out->Bytes(r.bytes);
      const auto other=target.effective[1-stream];Context context(state->Resources(),root,other?other->Find(root.Identity()):std::nullopt);
      Decode([&]{BodyCodecScratch scratch{facts,0,{},0,this};const auto result=DecodeBody(r.layout,r.mask,r.stream,decoding,scratch,&context,{32u<<20,262144-visits,32,true});visits+=result.field_visits;});
      out->records_[record].field_count=std::uint32_t(out->fields_.size())-r.first_field;
    }
  }
  void World();
  void Players();
  void Events();
  void NestedRules();
  void Commands();
  void PairedCostumeResources();
  std::shared_ptr<const ClientStateInstallPlan> Build() {
    const auto ready=ValidateClientStateTarget(target,tables,workspace);if(ready!=StateReadiness::Ready)Fail(ready,"client_plan_common_validation");
    Objects();World();Players();Records();Events();NestedRules();
    out->resources_=ClientStateRequiredResources(target,tables,workspace);PairedCostumeResources();Commands();return out;
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
    player.group=Reference(r,F::EntityPlayer::control_group);player.topology=Number<std::uint32_t>(r,F::EntityPlayer::costume_topology_revision);
    Require(player.topology&&(!out->world_.topology||out->world_.topology==player.topology),"client_plan_global_player_topology");out->world_.topology=player.topology;
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
    if(const auto body=target.effective[1]) {
      const auto pose=body->Find(r.Identity());if(!pose)Fail(StateReadiness::SourcePending,"client_plan_player_pose_missing");
      if(Number<std::uint32_t>(*pose,F::EntityPlayer::render_topology_revision)!=player.topology)Fail(StateReadiness::SourcePending,"client_plan_player_pose_topology");
    }else Fail(StateReadiness::SourcePending,"client_plan_body_context_missing");
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
    out->entities_.push_back({i,Number<std::uint32_t>(r,F::Entity::type),Number<std::uint32_t>(r,F::Entity::variant),Number<std::uint32_t>(r,F::Entity::subtype),
      Number<std::uint32_t>(r,F::Entity::init_seed),Number<std::uint32_t>(r,F::Entity::authority_order),{config.resource_table,Number<std::uint16_t>(r,F::Entity::selected_config)}});
  }
}
void ClientInstallBuilder::Events() {
  std::uint64_t last=0;
  for(std::uint32_t i=0;i<target.events.size();++i) {
    const auto &source=target.events[i];const auto &event=source.value;const auto kind=StateEventKind(event.purpose);
    Require(event.sequence>last&&StateEventLayout(kind)==event.layout,"client_plan_event_order_or_kind");last=event.sequence;
    ClientInstallEvent e;e.record=std::uint32_t(out->records_.size());e.kind=kind;e.sequence=event.sequence;e.game_frame=event.logical_frame;
    if(event.owner!=kNullIndex){const auto *owner=FindStateMember(source.members->Directory(),event.owner);Require(owner,"client_plan_event_owner");e.owner=owner->identity;e.has_owner=true;}
    out->events_.push_back(e);ClientInstallRecord r;r.event=i;r.layout=event.layout;r.stream=BodyStream::Value;
    r.context=target.effective[0]->Context();r.context.member_epoch=source.members->Directory().epoch;r.context.resource_epoch=source.resources->Directory().epoch;
    r.members=source.members;r.resources=source.resources;r.mask=1;r.bytes={std::uint32_t(out->bytes_.size()),std::uint32_t(event.payload.size())};
    out->bytes_.insert(out->bytes_.end(),event.payload.begin(),event.payload.end());r.first_field=std::uint32_t(out->fields_.size());
    record=std::uint32_t(out->records_.size());out->records_.push_back(r);decoding=out->Bytes(r.bytes);ResourceBodyContext context(source.resources->Directory());
    Decode([&]{BodyCodecScratch scratch{facts,0,{},0,this};const auto result=DecodeValue(r.layout,decoding,scratch,&context,{256u<<10,262144-visits,32,true});visits+=result.field_visits;});
    out->records_[record].field_count=std::uint32_t(out->fields_.size())-r.first_field;
  }
}
void ClientInstallBuilder::NestedRules() {
  WorkMap<std::uint64_t> loops{ClientStateAllocator<std::pair<const std::uint64_t,std::uint32_t>>(tables.Memory(),memory::Pool::Workspace)};
  WorkMap<std::int32_t> definitions{ClientStateAllocator<std::pair<const std::int32_t,std::uint32_t>>(tables.Memory(),memory::Pool::Workspace)};
  WorkMap<std::uint32_t> overlays{ClientStateAllocator<std::pair<const std::uint32_t,std::uint32_t>>(tables.Memory(),memory::Pool::Workspace)};
  // 每个嵌套实例均已有明确边界；这里仅检查完整目标才可判定的语义。
  for(const auto &v:out->values_) {
    if(!v.layout)continue;
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
        if(f!=field_index.end())Require(out->fields_[f->second].elements>=1,"client_plan_umbilical_rope_empty");break;
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
        Require(e.transition&&done<=1&&dimension>=0&&dimension<=2&&final>=0&&final<=2&&bool(done)==bool(room),"client_plan_transition_scope_or_completion");
        const auto player=AtNumber<std::uint16_t>(v,F::RoomTransitionEvent::player);
        if(player!=kNullIndex){const auto *m=FindStateMember(r.members->Directory(),player);Require(m&&m->identity.space==1&&m->identity.layout==197,"client_plan_transition_player");}break;
      }
      default:break;
    }
  }
  // 转场关联以Host序号建索引；开始／完成同时存在时，其请求元组必须一致。
  WorkMap<std::uint64_t> transitions{ClientStateAllocator<std::pair<const std::uint64_t,std::uint32_t>>(tables.Memory(),memory::Pool::Workspace)};
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
void ClientInstallBuilder::PairedCostumeResources() {
  // 单流只增加配对costume实际引用的闭包，并固定其原资源快照；不拉入整份旧world。
  ClientResourceSnapshotRef resources;
  ClientStateVector<ResourceKey> roots{ClientStateAllocator<ResourceKey>(tables.Memory(),memory::Pool::Workspace)};
  for(const auto &link:out->bindings_){
    const auto &r=out->records_[out->values_[link.value].record];
    if(!r.paired_costume||!link.is_resource||link.is_null)continue;
    Require(!resources||resources==r.resources,"client_plan_costume_resource_snapshot");resources=r.resources;roots.push_back(link.resource);
  }
  if(roots.empty())return;
  std::sort(roots.begin(),roots.end());roots.erase(std::unique(roots.begin(),roots.end()),roots.end());
  ClientStateVector<std::uint32_t> order{ClientStateAllocator<std::uint32_t>(tables.Memory(),memory::Pool::Workspace)};
  ClientStateVector<std::uint8_t> state{ClientStateAllocator<std::uint8_t>(tables.Memory(),memory::Pool::Workspace)},
    heights{ClientStateAllocator<std::uint8_t>(tables.Memory(),memory::Pool::Workspace)};
  const auto directory=resources->Directory();order.resize(directory.entries.size());state.resize(order.size());heights.resize(order.size());
  ResourceGraphScratch graph{{},{},order,state,heights};const auto closure=tables.Prepare(resources,roots,graph,workspace);
  if(!closure.closed)Fail(StateReadiness::MissingClosure,"client_plan_costume_resource_closure");
  if(!closure.materialized)Fail(StateReadiness::ResourcePending,"client_plan_costume_resource_pending");
  WorkMap<std::pair<std::uint32_t,ResourceKey>> seen{ClientStateAllocator<std::pair<const std::pair<std::uint32_t,ResourceKey>,std::uint32_t>>(tables.Memory(),memory::Pool::Workspace)};
  for(const auto &need:out->resources_)seen.emplace(std::pair{need.resources->Directory().epoch,need.key},0);
  for(const auto n:closure.materialization_order){const auto key=directory.entries[n].key;
    if(seen.emplace(std::pair{directory.epoch,key},0).second)out->resources_.push_back({resources,key});}
}
void ClientInstallBuilder::Commands() {
  const auto add=[&](ClientInstallPhase phase,std::uint32_t n=kClientPlanNone){out->commands_.push_back({phase,n});};
  for(std::uint32_t i=0;i<out->resources_.size();++i)add(ClientInstallPhase::Resources,i);
  for(std::uint32_t i=0;i<out->native_needs_.size();++i)add(ClientInstallPhase::NativeResources,i);
  const bool world=bool(target.candidate->State(BodyStream::World));
  if(world)add(ClientInstallPhase::CaptureOldRoom);
  // 暂存按域排序；值与关系分开，合法的parent／twin环不会形成创建死锁。
  for(unsigned d=0;d<=unsigned(ClientInstallDomain::Sfx);++d)for(std::uint32_t i=0;i<out->objects_.size();++i)
    if(unsigned(out->objects_[i].domain)==d&&(out->objects_[i].streams||out->objects_[i].action!=ClientInstallAction::Reuse))add(ClientInstallPhase::StageObjects,i);
  // codec字段是后序；反向暂存让外层容器先取得临时承载，正向赋值保留元素提交顺序。
  for(std::size_t i=out->fields_.size();i>0;--i){const auto &f=out->fields_[i-1];const auto &def=state::Fields()[f.definition];
    if(out->records_[f.record].event==kClientPlanNone&&(def.nested_layout||def.operation==Operation::Container||def.operation==Operation::Choice))add(ClientInstallPhase::StageFields,std::uint32_t(i-1));}
  for(std::uint32_t i=0;i<out->fields_.size();++i){const auto &f=out->fields_[i];const auto &def=state::Fields()[f.definition];
    if(f.phase==ClientInstallPhase::Values&&def.operation!=Operation::Reference)add(ClientInstallPhase::Values,i);}
  // 先清空／定长关系字段（含空anchors），再写已解析的目标；不把u16序号当原生指针。
  for(std::uint32_t i=0;i<out->fields_.size();++i)if(out->fields_[i].phase==ClientInstallPhase::RelationFields)add(ClientInstallPhase::RelationFields,i);
  for(std::uint32_t i=0;i<out->bindings_.size();++i){const auto &link=out->bindings_[i];const auto &v=out->values_[link.value];
    if(!link.is_resource&&out->records_[v.record].event==kClientPlanNone)add(ClientInstallPhase::Relations,i);}
  for(std::uint32_t i=0;i<out->fields_.size();++i)if(out->fields_[i].phase==ClientInstallPhase::Display)add(ClientInstallPhase::Display,i);
  if(world)add(ClientInstallPhase::CaptureNewRoom);
  for(std::uint32_t i=0;i<out->events_.size();++i)add(ClientInstallPhase::Events,i);
  add(ClientInstallPhase::Publish);
  // 退休发生在发布后的真实借用排空；本清单不是已完成析构／可以提前释放的回执。
  for(std::uint32_t i=0;i<out->retired_.size();++i)add(ClientInstallPhase::Retire,i);
}
std::shared_ptr<const ClientStateInstallPlan> BuildClientStateInstallPlan(const ClientStatePreparedTarget &target,const ClientStateTables &tables) {
  ClientStateWorkspace workspace;
  try{return ClientInstallBuilder(target,tables,workspace).Build();}
  catch(const BodyVisitError &){Fail(StateReadiness::Budget,"client_plan_visit_budget");}
  catch(const BodyDependencyError &){Fail(StateReadiness::SourcePending,"client_plan_body_dependency");}
  catch(const Gap &){Fail(StateReadiness::InvalidEncoding,"client_plan_invalid_body");}
}
} // namespace cs::state
