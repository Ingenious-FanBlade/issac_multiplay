#include "state/client_validation.h"
#include <algorithm>
#include <cstring>
#include <string_view>
#include <set>

namespace cs::state {
namespace {
#include "state_host_fields.inc"
namespace F=host_field;
[[noreturn]] void Fail(StateReadiness r,const char *why){throw ClientShadowError(r,why);}
template<class T>T Exact(std::span<const std::byte> bytes) {if(bytes.size()!=sizeof(T))Fail(StateReadiness::InvalidEncoding,"client_validation_scalar_size");T value;std::memcpy(&value,bytes.data(),sizeof(value));return value;}
std::uint32_t Number(ClientShadowRecordView record,std::uint16_t field) {
  const auto *value=record.Field(field);if(!value||!value->present)Fail(StateReadiness::SourcePending,"client_validation_topology_missing");return Exact<std::uint32_t>(value->bytes);
}
bool Descends(std::uint16_t id,std::uint16_t root) {
  for(unsigned depth=0;id&&depth<32;++depth){if(id==root)return true;const auto *layout=FindLayout(id);if(!layout)return false;id=layout->parent;}return false;
}
enum class ReferenceKind:std::uint8_t {Unknown,Entity,Player,Grid,Descriptor,Participant,Group};
bool ReferenceTarget(const Field &field,const PresentationMember &member) {
  // 产品启动后首次绑定生成表；逐节点只按layout/field数字索引，不重新匹配处理器文本。
  static const auto kinds=[] {
    std::vector<ReferenceKind> out;out.reserve(Fields().size());
    for(const auto &f:Fields()) {
      const std::string_view target=f.target?f.target:"";ReferenceKind kind=ReferenceKind::Unknown;
      if(target=="entity"||target=="entities"||target=="Entity"||target=="entity-members")kind=ReferenceKind::Entity;
      else if(target=="player")kind=ReferenceKind::Player;else if(target=="grid")kind=ReferenceKind::Grid;
      else if(target=="room-descriptor")kind=ReferenceKind::Descriptor;else if(target=="participant")kind=ReferenceKind::Participant;
      else if(target=="control-group")kind=ReferenceKind::Group;out.push_back(kind);
    }return out;
  }();
  switch(kinds[std::size_t(&field-Fields().data())]) {
    case ReferenceKind::Entity:return member.space==1&&Descends(member.layout,100);
    case ReferenceKind::Player:return member.space==1&&member.layout==197;
    case ReferenceKind::Grid:return member.space==2&&Descends(member.layout,199);
    case ReferenceKind::Descriptor:return member.space==6&&member.layout==270;
    case ReferenceKind::Participant:return member.space==9&&member.layout==195;
    case ReferenceKind::Group:return member.space==10&&member.layout==196;
    default:return false;
  }
}
class Context final:public BodyCodecContext {
public:
  Context(ClientResourceSnapshotRef r,ClientShadowRecordView own,std::optional<ClientShadowRecordView> paired)
    :resources(std::move(r)),record(own),other(paired) {}
  std::optional<CodecItemConfig> Item(std::uint8_t table,std::uint16_t index)const override{return ResourceBodyContext(resources->Directory()).Item(table,index);}
  std::optional<bool> SharedPresence(const CodecLocation &location)const override {
    const auto read=[&](ClientShadowRecordView r)->std::optional<bool>{for(const auto &f:r.Projections())if(f.kind==ProjectionFactKind::SharedPresence&&
      f.layout==location.layout&&f.group==location.canonical_field&&f.depth==location.parents.size()&&std::equal(location.parents.begin(),location.parents.end(),f.parents.begin()))return f.value!=0;return {};};
    if(const auto value=read(record))return value;return other?read(*other):std::nullopt;
  }
  ClientResourceSnapshotRef resources;ClientShadowRecordView record;std::optional<ClientShadowRecordView> other;
};
class Links final:public BodyNodeObserver {
public:
  Links(const ClientStateTables &t,ClientMemberSnapshotRef m,ClientResourceSnapshotRef r)
    :tables(t),members(std::move(m)),resources(std::move(r)),roots(ClientStateAllocator<ResourceKey>(t.Memory(),memory::Pool::Workspace)) {}
  void Node(const BodyNodeView &node)override {
    if(node.layout||(node.operation!=Operation::Reference&&node.operation!=Operation::Resource))return;
    if(node.path.empty())Fail(StateReadiness::InvalidEncoding,"client_validation_reference_path");
    const auto index=Exact<std::uint16_t>(node.bytes);if(index==kNullIndex)return;
    const auto &path=node.path.back();const auto *field=FindField(path.layout,path.field);
    if(!field)Fail(StateReadiness::InvalidEncoding,"client_validation_reference_field");
    if(node.operation==Operation::Reference) {
      const auto *member=FindStateMember(members->Directory(),index);
      if(!member||!ReferenceTarget(*field,member->identity))Fail(StateReadiness::InvalidReference,"client_validation_reference_type");
    }else {
      ResourceKey key{node.resource_table,index};if(!FindResource(resources->Directory(),key))Fail(StateReadiness::MissingClosure,"client_validation_resource_missing");
      roots.push_back(key);
    }
  }
  const ClientStateTables &tables;ClientMemberSnapshotRef members;ClientResourceSnapshotRef resources;
  ClientStateVector<ResourceKey> roots;
};
class Ready final:public ResourceReadyContext {
public:
  explicit Ready(const ClientStateTables &t):tables(t) {}
  bool Materialized(const ResourceDirectoryView &directory,const ResourceEntryView &entry)const override {
    const auto state=tables.ResourceState(directory.epoch,entry.key);
    if(state==ClientResourceState::MissingLocalResource)Fail(StateReadiness::MissingLocalResource,"client_validation_resource_file_missing");
    if(state==ClientResourceState::Failed)Fail(StateReadiness::Lifecycle,"client_validation_resource_prepare_failed");
    if(state!=ClientResourceState::Ready)Fail(StateReadiness::ResourcePending,"client_validation_resource_pending");return true;
  }
  const ClientStateTables &tables;
};
struct Scratch {
  ClientStateVector<std::byte> bytes;
  ClientStateVector<ProjectionFact> facts;
  ClientStateVector<BodyFieldView> fields;
  ClientStateVector<std::uint32_t> order;
  ClientStateVector<std::uint8_t> state,heights;
  explicit Scratch(const std::shared_ptr<ClientStateMemory> &m)
    :bytes(ClientStateAllocator<std::byte>(m,memory::Pool::Workspace)),facts(ClientStateAllocator<ProjectionFact>(m,memory::Pool::Workspace)),
     fields(ClientStateAllocator<BodyFieldView>(m,memory::Pool::Workspace)),order(ClientStateAllocator<std::uint32_t>(m,memory::Pool::Workspace)),
     state(ClientStateAllocator<std::uint8_t>(m,memory::Pool::Workspace)),heights(ClientStateAllocator<std::uint8_t>(m,memory::Pool::Workspace)) {
    fields.resize(BodySourceMaximumFields());facts.resize(16);
  }
  BodyCodecScratch Body(BodyNodeObserver *observer=nullptr){return {facts,0,fields,0,observer};}
  template<class Fn>void Decode(Fn &&fn) {
    for(;;)try {fn();return;}catch(const BodyScratchError &e) {
      if(e.Region()!=BodyScratchRegion::Facts||e.Requested()>262144)throw;
      facts.resize(std::max<std::size_t>(e.Requested(),facts.size()*2));
    }
  }
  void Closure(const ClientStateTables &tables,const ClientResourceSnapshotRef &resources,Links &links,const ClientStateWorkspace &workspace) {
    std::sort(links.roots.begin(),links.roots.end());links.roots.erase(std::unique(links.roots.begin(),links.roots.end()),links.roots.end());
    const auto size=resources->Directory().entries.size();order.resize(size);state.resize(size);heights.resize(size);
    ResourceGraphScratch graph{{},{},order,state,heights};const auto result=tables.Prepare(resources,links.roots,graph,workspace);
    if(!result.closed)Fail(StateReadiness::MissingClosure,"client_validation_resource_closure");
    if(!result.materialized) {
      for(const auto n:result.materialization_order){const auto &entry=resources->Directory().entries[n];Ready(tables).Materialized(resources->Directory(),entry);}
      Fail(StateReadiness::ResourcePending,"client_validation_resource_closure_pending");
    }
  }
};
}
std::vector<ClientShadowLayoutBinding> ClientStateRootBindings() {
  std::vector<ClientShadowLayoutBinding> out;
  for(const auto &layout:Layouts())if(IsStateMemberLayout(layout.id)) {
    if(Descends(layout.id,100)&&!layout.base_prefix)out.push_back({1,layout.id,false});
    else if(Descends(layout.id,199)&&!layout.base_prefix)out.push_back({2,layout.id,false});
  }
  for(const auto [space,layout]:{std::pair{3u,237u},std::pair{4u,276u},std::pair{6u,270u},std::pair{7u,239u},
      std::pair{9u,195u},std::pair{10u,196u},std::pair{12u,437u}})out.push_back({space,std::uint16_t(layout),layout==195});
  return out;
}
ClientStateVector<ClientStateResourceNeed> ClientStateRequiredResources(const ClientStatePreparedTarget &target,const ClientStateTables &tables) {
  ClientStateWorkspace workspace;return ClientStateRequiredResources(target,tables,workspace);
}
ClientStateVector<ClientStateResourceNeed> ClientStateRequiredResources(const ClientStatePreparedTarget &target,const ClientStateTables &tables,const ClientStateWorkspace &workspace) {
  workspace.RequireCurrentThread();
  if(!target.candidate)Fail(StateReadiness::InvalidEncoding,"client_resource_target_missing");
  ClientStateVector<ClientStateResourceNeed> out{ClientStateAllocator<ClientStateResourceNeed>(tables.Memory())};Scratch scratch(tables.Memory());
  std::set<std::uint64_t,std::less<std::uint64_t>,ClientStateAllocator<std::uint64_t>> seen{ClientStateAllocator<std::uint64_t>(tables.Memory(),memory::Pool::Workspace)};
  std::uint32_t visits=0;
  const auto append=[&](const ClientResourceSnapshotRef &resources,Links &links) {
    std::sort(links.roots.begin(),links.roots.end());links.roots.erase(std::unique(links.roots.begin(),links.roots.end()),links.roots.end());
    const auto count=resources->Directory().entries.size();scratch.order.resize(count);scratch.state.resize(count);scratch.heights.resize(count);
    ResourceGraphScratch graph{{},{},scratch.order,scratch.state,scratch.heights};const auto closure=tables.Prepare(resources,links.roots,graph,workspace);
    if(!closure.closed)Fail(StateReadiness::MissingClosure,"client_resource_target_not_closed");
    for(const auto n:closure.materialization_order) {
      const auto key=resources->Directory().entries[n].key;
      const auto identity=(std::uint64_t(resources->Directory().epoch)<<24)|(std::uint64_t(key.table)<<16)|key.index;
      if(seen.insert(identity).second)out.push_back({resources,key});
    }
  };
  for(unsigned stream=0;stream<2;++stream)if(const auto own=target.candidate->State(BodyStream(stream))) {
    const auto other=target.effective[1-stream];Links links(tables,own->Members(),own->Resources());
    for(std::size_t i=0;i<own->RecordCount();++i) {
      const auto record=own->Record(i);const auto paired=other?other->Find(record.Identity()):std::nullopt;Context context(own->Resources(),record,paired);
      scratch.bytes.clear();for(unsigned b=0;b<32;++b)if(const auto block=record.Block(std::uint8_t(b)))scratch.bytes.insert(scratch.bytes.end(),block->bytes.begin(),block->bytes.end());
      // 每条根单独解码，保留此前根已收集的引用；scratch扩容重试先撤本根未完成的追加。
      const auto before=links.roots.size();scratch.Decode([&]{links.roots.resize(before);auto body=scratch.Body(&links);
        const auto value=DecodeBody(record.Identity().layout,record.Mask(),BodyStream(stream),scratch.bytes,body,&context,{32u<<20,262144-visits,32,true});visits+=value.field_visits;});
    }
    append(own->Resources(),links);
  }
  for(const auto &event:target.events) {
    Links links(tables,event.members,event.resources);ResourceBodyContext context(event.resources->Directory());
    scratch.Decode([&]{links.roots.clear();auto body=scratch.Body(&links);const auto value=DecodeValue(event.value.layout,event.value.payload,body,&context,{256u<<10,262144-visits,32,true});visits+=value.field_visits;});
    append(event.resources,links);
  }
  return out;
}
StateReadiness ValidateClientStateTarget(const ClientStatePreparedTarget &target,const ClientStateTables &tables) {
  try {ClientStateWorkspace workspace;return ValidateClientStateTarget(target,tables,workspace);}
  catch(const ClientTableError &e){return e.Failure()==ClientTableFailure::WorkspaceBusy?StateReadiness::Budget:StateReadiness::Lifecycle;}
}
StateReadiness ValidateClientStateTarget(const ClientStatePreparedTarget &target,const ClientStateTables &tables,const ClientStateWorkspace &workspace) {
  try {
    workspace.RequireCurrentThread();
    if(!target.candidate||!target.candidate->Frame().decoded)Fail(StateReadiness::InvalidEncoding,"client_validation_undecoded");
    Scratch scratch(tables.Memory());const auto &candidate=*target.candidate;
    std::uint32_t visits=0;
    for(unsigned stream=0;stream<2;++stream)if(const auto own=candidate.State(BodyStream(stream))) {
      const auto other=target.effective[1-stream];
      for(std::size_t i=0;i<own->RecordCount();++i) {
        const auto record=own->Record(i);const auto paired=other?other->Find(record.Identity()):std::nullopt;
        if(record.Identity().layout==197) {
          if(!paired)Fail(StateReadiness::SourcePending,"client_validation_player_paired_state");
          const auto &body=stream==1?record:*paired;const auto &world=stream==0?record:*paired;
          if(Number(body,F::EntityPlayer::render_topology_revision)!=Number(world,F::EntityPlayer::costume_topology_revision))
            Fail(StateReadiness::SourcePending,"client_validation_player_render_topology");
        }
        scratch.bytes.clear();for(unsigned b=0;b<32;++b)if(const auto block=record.Block(std::uint8_t(b)))scratch.bytes.insert(scratch.bytes.end(),block->bytes.begin(),block->bytes.end());
        Context context(own->Resources(),record,paired);Links links(tables,own->Members(),own->Resources());
        scratch.Decode([&]{links.roots.clear();auto body=scratch.Body(&links);
          const auto value=DecodeBody(record.Identity().layout,record.Mask(),BodyStream(stream),scratch.bytes,body,&context,{32u<<20,262144,32,true});
          if(value.field_visits>262144-visits)Fail(StateReadiness::Budget,"client_validation_target_visit_budget");visits+=value.field_visits;});
        scratch.Closure(tables,own->Resources(),links,workspace);
        Ready ready(tables);scratch.Decode([&]{auto body=scratch.Body();ValidateResourceBindings(record.Identity().layout,record.Mask(),BodyStream(stream),scratch.bytes,own->Resources()->Directory(),ready,body,&context,{32u<<20,262144,32,true});});
      }
    }
    for(const auto &event:target.events) {
      if(!event.members||!event.resources||event.members->Route()!=candidate.Target().route)Fail(StateReadiness::Lifecycle,"client_validation_event_snapshot");
      Links links(tables,event.members,event.resources);ResourceBodyContext context(event.resources->Directory());
      scratch.Decode([&]{links.roots.clear();auto body=scratch.Body(&links);const auto value=DecodeValue(event.value.layout,event.value.payload,body,&context,{256u<<10,262144-visits,32,true});visits+=value.field_visits;});
      scratch.Closure(tables,event.resources,links,workspace);Ready ready(tables);
      scratch.Decode([&]{auto body=scratch.Body();ValidateResourceBindings(event.value.layout,1,BodyStream::Value,event.value.payload,event.resources->Directory(),ready,body,&context,{256u<<10,262144,32,true});});
    }
    return StateReadiness::Ready;
  }catch(const ClientShadowError &e){return e.Readiness();}
   catch(const ClientTableError &e){return e.Failure()==ClientTableFailure::Budget||e.Failure()==ClientTableFailure::WorkspaceBusy?StateReadiness::Budget:StateReadiness::Lifecycle;}
   catch(const BodyVisitError &){return StateReadiness::Budget;}
   catch(const Gap &){return StateReadiness::InvalidEncoding;}
}
} // namespace cs::state
