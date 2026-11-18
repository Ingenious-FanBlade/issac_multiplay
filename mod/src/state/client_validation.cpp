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
// 此处只验不可变目录的绑定语义。实际物化在已就绪队列和 Begin 时重核。
class DeclaredResource final:public ResourceReadyContext {
public:
  bool Materialized(const ResourceDirectoryView &,const ResourceEntryView &)const override{return true;}
};
class Links final:public BodyNodeObserver {
public:
  Links(const ClientStateTables &t,ClientMemberSnapshotRef m,ClientResourceSnapshotRef r)
    :members(std::move(m)),resources(std::move(r)),roots(ClientStateAllocator<ResourceKey>(t.Memory(),memory::Pool::Workspace)) {}
  void Node(const BodyNodeView &node)override {
    if(const auto key=ValidateClientDeclaredNode(node,members->Directory(),resources->Directory()))roots.push_back(*key);
  }
  ClientMemberSnapshotRef members;ClientResourceSnapshotRef resources;
  ClientStateVector<ResourceKey> roots;
};
struct Scratch {
  ClientStateVector<ProjectionFact> facts;
  ClientStateVector<BodyFieldView> fields;
  ClientStateVector<std::uint32_t> order;
  ClientStateVector<std::uint8_t> state,heights;
  explicit Scratch(const std::shared_ptr<ClientStateMemory> &m)
    :facts(ClientStateAllocator<ProjectionFact>(m,memory::Pool::Workspace)),
     fields(ClientStateAllocator<BodyFieldView>(m,memory::Pool::Workspace)),order(ClientStateAllocator<std::uint32_t>(m,memory::Pool::Workspace)),
     state(ClientStateAllocator<std::uint8_t>(m,memory::Pool::Workspace)),heights(ClientStateAllocator<std::uint8_t>(m,memory::Pool::Workspace)) {
    fields.resize(BodySourceMaximumFields());facts.resize(128);
  }
  BodyCodecScratch Body(BodyNodeObserver *observer=nullptr){return {facts,0,fields,0,observer};}
  template<class Fn>void Decode(Fn &&fn) {
    for(;;)try {fn();return;}catch(const BodyScratchError &e) {
      if(e.Region()!=BodyScratchRegion::Facts||e.Requested()>262144)throw;
      facts.resize(std::max<std::size_t>(e.Requested(),facts.size()*2));
    }
  }

};
}
std::optional<ResourceKey> ValidateClientDeclaredNode(const BodyNodeView &node,StateMemberDirectory members,
    ResourceDirectoryView resources) {
  if(node.layout){const DeclaredResource declared;ValidateResourceBindingNode(node,resources,declared);return {};}
  if(node.operation!=Operation::Reference&&node.operation!=Operation::Resource)return {};
  if(node.path.empty())Fail(StateReadiness::InvalidEncoding,"client_validation_reference_path");
  const auto index=Exact<std::uint16_t>(node.bytes);
  if(index==kNullIndex)return {};
  const auto &path=node.path.back();const auto *field=FindField(path.layout,path.field);
  if(!field)Fail(StateReadiness::InvalidEncoding,"client_validation_reference_field");
  if(node.operation==Operation::Reference){
    const auto *member=FindStateMember(members,index);
    if(!member||!ReferenceTarget(*field,member->identity))Fail(StateReadiness::InvalidReference,"client_validation_reference_type");
    return {};
  }
  const ResourceKey key{node.resource_table,index};
  if(!FindResource(resources,key))Fail(StateReadiness::InvalidReference,"client_validation_resource_missing");
  return key;
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
// 纯值检查按节点批次推进；事件和资源闭包保留原有有界原子单元。
struct ClientTargetCheckWork final {
  const ClientStatePreparedTarget &target;
  const ClientStateTables &tables;
  Scratch scratch;
  std::shared_ptr<ClientTargetFacts> facts;
  std::set<std::uint64_t,std::less<std::uint64_t>,ClientStateAllocator<std::uint64_t>> seen;
  std::optional<Links> links;
  ClientShadowVisitCursor cursor;
  unsigned stream=0;
  std::size_t member=0,event=0;
  std::uint32_t visits=0;
  std::uint32_t affected=0;
  bool started_record=false;
  ClientTargetCheckWork(const ClientStatePreparedTarget &t,const ClientStateTables &s):
    target(t),tables(s),scratch(s.Memory()),
    facts(std::allocate_shared<ClientTargetFacts>(ClientStateAllocator<ClientTargetFacts>(s.Memory()),s.Memory())),
    seen(ClientStateAllocator<std::uint64_t>(s.Memory(),memory::Pool::Workspace)) {}

  void Append(const ClientStateWorkspace &workspace) {
    if(links->roots.empty())return;
    auto &roots=links->roots;
    std::sort(roots.begin(),roots.end());roots.erase(std::unique(roots.begin(),roots.end()),roots.end());
    const auto directory=links->resources->Directory();const auto count=directory.entries.size();
    scratch.order.resize(count);scratch.state.resize(count);scratch.heights.resize(count);
    ResourceGraphScratch graph{{},{},scratch.order,scratch.state,scratch.heights};
    const auto closure=tables.Prepare(links->resources,roots,graph,workspace);
    if(!closure.closed)Fail(StateReadiness::MissingClosure,"client_resource_target_not_closed");
    for(const auto n:closure.materialization_order){
      const auto key=directory.entries[n].key;
      const auto identity=(std::uint64_t(directory.epoch)<<24)|(std::uint64_t(key.table)<<16)|key.index;
      if(seen.insert(identity).second)facts->resources.push_back({links->resources,key});
    }
  }
  bool Step(const ClientStateWorkspace &workspace) {
    const auto &candidate=*target.candidate;
    if(stream<2){
      const auto own=candidate.State(BodyStream(stream));
      if(own&&member<own->RecordCount()){
        const auto record=own->Record(member);
        if(!links)links.emplace(tables,own->Members(),own->Resources());
        if(const auto &flat=record.Flat()){
          for(const auto &link:flat->links)if(link.is_resource)links->roots.push_back(link.resource);
          ++member;
          return false;
        }
        if(!started_record){
          if(record.Identity().layout==197&&candidate.Target().count==2){
            const auto &other=target.effective[1-stream];
            const auto paired=other?other->Find(record.Identity()):std::nullopt;
            if(!paired)Fail(StateReadiness::MissingClosure,"client_validation_player_paired_state");
            const auto &body=stream==1?record:*paired;const auto &world=stream==0?record:*paired;
            if(Number(body,F::EntityPlayer::render_topology_revision)!=Number(world,F::EntityPlayer::costume_topology_revision))
              Fail(StateReadiness::MissingClosure,"client_validation_player_render_topology");
          }
          if(record.FieldVisits()>262144-visits)Fail(StateReadiness::Budget,"client_validation_target_visit_budget");
          visits+=record.FieldVisits();started_record=true;
          affected=record.Mask();
          // 仅以相同影子块及同资源快照复用已接受的引用检查；实际本机就绪仍在交票前核验。
          const auto &base=target.previous[stream];
          if(base&&base->Resources()==own->Resources())if(const auto before=base->Find(record.Identity()))
            for(unsigned block=0;block<32;++block){
              const auto bit=std::uint32_t(1)<<block;
              if((affected&bit)&&record.SameBlock(*before,std::uint8_t(block))){
                record.AppendResourceKeys(std::uint8_t(block),links->roots);affected&=~bit;
              }
            }
        }
        if(record.VisitSome(affected,*links,cursor,128)){
          started_record=false;cursor={};++member;
        }
      }else{
        if(links)Append(workspace);
        links.reset();member=0;++stream;
      }
      return false;
    }
    if(event==target.events.size())return true;
    const auto &source=target.events[event];
    if(!source.members||!source.resources||source.members->Route()!=candidate.Target().route)
      Fail(StateReadiness::Lifecycle,"client_validation_event_snapshot");
    links.emplace(tables,source.members,source.resources);
    ResourceBodyContext context(source.resources->Directory());
    scratch.Decode([&]{
      links->roots.clear();auto body=scratch.Body(&*links);
      const auto value=DecodeValue(source.value.layout,source.value.payload,body,&context,{256u<<10,262144-visits,32,true});
      visits+=value.field_visits;
    });
    Append(workspace);links.reset();++event;return false;
  }
};
std::shared_ptr<const ClientTargetFacts> InspectClientStateTargetStep(const ClientStatePreparedTarget &target,
    const ClientStateTables &tables,std::shared_ptr<ClientTargetCheckWork> &work,std::chrono::steady_clock::time_point stop) {
  if(!target.candidate||!target.candidate->Frame().decoded)Fail(StateReadiness::InvalidEncoding,"client_validation_undecoded");
  ClientStateWorkspace workspace;
  if(!work)work=std::allocate_shared<ClientTargetCheckWork>(ClientStateAllocator<ClientTargetCheckWork>(tables.Memory()),target,tables);
  try{
    do{if(work->Step(workspace)){auto result=work->facts;work.reset();return result;}}
    while(std::chrono::steady_clock::now()<stop);
    return {};
  }catch(const BodyVisitError &){work.reset();Fail(StateReadiness::Budget,"client_validation_target_visit_budget");}
   catch(const Gap &){work.reset();Fail(StateReadiness::InvalidEncoding,"client_validation_target_encoding");}
   catch(...){work.reset();throw;}
}

} // namespace cs::state
