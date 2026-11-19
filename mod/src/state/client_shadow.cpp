#include "state/client_shadow.h"
#include "state/client_events.h"
#include "state/client_validation.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <map>
#include <tuple>

namespace cs::state {
namespace {
#include "state_protocol.inc"
#include "state_host_fields.inc"
[[noreturn]] void Fail(StateReadiness readiness,const char *reason,std::uint16_t member=kNullIndex,std::uint16_t layout=0,
    std::uint16_t field=kNullIndex) {
  throw ClientShadowError(readiness,reason,member,layout,field);
}
unsigned Stream(BodyStream stream) {
  if(unsigned(stream)>1)Fail(StateReadiness::InvalidEncoding,"shadow_invalid_stream");return unsigned(stream);
}
auto IdentityKey(const PresentationMember &member) {return std::tuple(member.space,member.id,member.use,member.shape,member.layout);}
auto MemberKey(const PresentationMember &member) {return std::pair(member.space,member.id);}
bool BytesEqual(std::span<const std::byte> a,std::span<const std::byte> b) {return std::equal(a.begin(),a.end(),b.begin(),b.end());}
std::uint32_t Mask(std::uint16_t id,BodyStream stream) {
  const auto *layout=FindLayout(id);if(!layout || !IsStateMemberLayout(id) || layout->block_count>32)Fail(StateReadiness::InvalidReference,"shadow_root_layout",kNullIndex,id);
  std::uint32_t mask=0;for(unsigned i=0;i<layout->block_count;++i){const auto &block=Blocks()[layout->first_block+i];
    if(block.max_size && block.body==(stream==BodyStream::Body))mask|=std::uint32_t{1}<<i;}return mask;
}
std::size_t Offset(std::span<const std::byte> outer,std::span<const std::byte> inner) {
  if(outer.empty() && inner.empty())return 0;
  const auto a=reinterpret_cast<std::uintptr_t>(outer.data()),b=reinterpret_cast<std::uintptr_t>(inner.data());
  if(b<a || b-a>outer.size() || inner.size()>outer.size()-(b-a))Fail(StateReadiness::InvalidEncoding,"shadow_slice_not_owned");return b-a;
}
bool SameTicket(const StateApplyTicket &a,const StateApplyTicket &b) {return a.route==b.route && a.serial==b.serial;}
} // namespace

// 每个规范块的字节单独拥有；共享它不延长上一条record／候选的寿命。
struct ClientShadowBytes final {
  explicit ClientShadowBytes(const std::shared_ptr<ClientStateMemory> &memory):bytes(ClientStateAllocator<std::byte>(memory)) {}
  ClientStateVector<std::byte> bytes;
};
// 链接保存编码值及已解析的真实身份。表重绑定时逐值比较，不把新表当作旧字节的解释。
struct ClientShadowLink {
  bool resource=false;
  std::uint16_t index=0;
  PresentationMember member;
  ResourceEntryView entry;
};
bool SameLink(const ClientShadowLink &a,const ClientShadowLink &b) {
  if(a.resource!=b.resource || a.index!=b.index)return false;
  return a.resource?a.entry.key==b.entry.key && a.entry.kind==b.entry.kind && BytesEqual(a.entry.payload,b.entry.payload):a.member==b.member;
}
// 路径及切片偏移独立于暂存正文，换表时不必重建未受影响的数字结构。
struct ClientShadowNode {
  std::uint32_t path=0,offset=0,size=0,elements=0,slot_mask=0,definition=UINT32_MAX;
  std::uint16_t layout=0,component=0;
  std::uint8_t depth=0,resource_table=255;
  Operation operation=Operation::Copy;
  bool field=false,present=false,payload_present=false;
  bool operator==(const ClientShadowNode &) const = default;
};
struct ClientShadowDecodedBlock {
  ClientStateVector<CodecPathStep> paths;
  ClientStateVector<ClientShadowNode> nodes;
  explicit ClientShadowDecodedBlock(const std::shared_ptr<ClientStateMemory> &m,memory::Pool pool)
    :paths(ClientStateAllocator<CodecPathStep>(m,pool)),nodes(ClientStateAllocator<ClientShadowNode>(m,pool)) {}
};
struct ClientShadowBlockData final {
  explicit ClientShadowBlockData(const std::shared_ptr<ClientStateMemory> &memory):links(ClientStateAllocator<ClientShadowLink>(memory)) {}
  ClientShadowBlockView view;
  std::shared_ptr<const ClientShadowBytes> content;
  std::shared_ptr<const ClientShadowDecodedBlock> decoded;
  ClientStateVector<ClientShadowLink> links;
  ClientResourceSnapshotRef resources;
  std::uint32_t visits=0;
};
// 一条序列记录固定完整身份、实际块及数字视图，条件缺席与合法空值保持区别。
struct ClientShadowRecordData final {
  explicit ClientShadowRecordData(const std::shared_ptr<ClientStateMemory> &memory)
      :fields(ClientStateAllocator<BodyFieldView>(memory)),facts(ClientStateAllocator<ProjectionFact>(memory)) {}
  std::uint16_t member=0;
  PresentationMember identity;
  std::uint32_t mask=0,member_epoch=0,resource_epoch=0,visits=0;
  std::array<std::shared_ptr<const ClientShadowBlockData>,32> blocks{};
  ClientFlatImageRef flat;
  ClientStateVector<BodyFieldView> fields;
  ClientStateVector<ProjectionFact> facts;
};
// 完整目标保留准确两表与当前时钟；未变化record可共享，成员顺序只作数字索引。
struct ClientShadowStateData final {
  explicit ClientShadowStateData(const std::shared_ptr<ClientStateMemory> &memory)
      :records(ClientStateAllocator<std::shared_ptr<const ClientShadowRecordData>>(memory)),
       flat_faults(ClientStateAllocator<ClientFlatFault>(memory))
#ifdef ISAACCS_E2E
       ,deferred_resources(ClientStateAllocator<std::uint16_t>(memory))
#endif
       {}
  PresentationBatchContext context;
  ClientMemberSnapshotRef members;
  ClientResourceSnapshotRef resources;
  ClientStateVector<std::shared_ptr<const ClientShadowRecordData>> records;
  ClientStateVector<ClientFlatFault> flat_faults;
#ifdef ISAACCS_E2E
  // 仅 E2E 对照使用。保留值和解码收到的值必须有可区分的证据。
  ClientStateVector<std::uint16_t> deferred_resources;
#endif
  std::shared_ptr<const ClientStateVector<std::uint16_t>> order;
  std::shared_ptr<const ClientStateVector<ClientShadowDependency>> dependencies;
  std::optional<std::uint16_t> Position(const PresentationMember &identity) const noexcept {
    if(!order)return {};
    const auto it=std::lower_bound(order->begin(),order->end(),IdentityKey(identity),[&](auto index,const auto &key){return IdentityKey(records[index]->identity)<key;});
    if(it==order->end()||records[*it]->identity!=identity)return {};
    return *it;
  }
  const ClientShadowRecordData *Find(const PresentationMember &identity) const noexcept {
    const auto position=Position(identity);
    return position?records[*position].get():nullptr;
  }
  std::shared_ptr<const ClientShadowRecordData> FindRef(const PresentationMember &identity) const noexcept {
    const auto *value=Find(identity);if(!value)return {};
    const auto it=std::lower_bound(records.begin(),records.end(),value->member,[](const auto &record,auto index){return record->member<index;});
    return it==records.end()?nullptr:*it;
  }
  void Index(const ClientShadowStateData *base) {
    // 同一成员表和流使用同一索引；块更新仅替换受影响的成员引用。
    if(base&&base->members==members&&base->records.size()==records.size()&&
        std::equal(records.begin(),records.end(),base->records.begin(),[](const auto &a,const auto &b){return a->identity==b->identity;})){
      order=base->order;return;
    }
    const auto memory=records.get_allocator().Memory();
    using Index=ClientStateVector<std::uint16_t>;
    auto next=std::allocate_shared<Index>(ClientStateAllocator<Index>(memory),ClientStateAllocator<std::uint16_t>(memory));
    next->resize(records.size());for(std::size_t i=0;i<next->size();++i)(*next)[i]=std::uint16_t(i);
    std::sort(next->begin(),next->end(),[&](auto a,auto b){return IdentityKey(records[a]->identity)<IdentityKey(records[b]->identity);});
    order=std::move(next);
  }
  void IndexDependencies(const std::shared_ptr<ClientStateMemory> &memory,const ClientShadowStateData *base,bool dirty) {
    if(!dirty&&base&&base->dependencies){dependencies=base->dependencies;return;}
    using Edges=ClientStateVector<ClientShadowDependency>;
    auto edges=std::allocate_shared<Edges>(ClientStateAllocator<Edges>(memory),ClientStateAllocator<ClientShadowDependency>(memory));
    for(const auto &record:records){
      for(const auto &block:record->blocks)if(block)for(const auto &link:block->links)
        edges->push_back({link.resource?PresentationMember{}:link.member,record->identity,
          link.resource?link.entry.key:ResourceKey{},block->view.block,link.resource});
      if(record->flat)for(const auto &link:record->flat->links)
        edges->push_back({link.member,record->identity,link.resource,255,link.is_resource});
    }
    const auto key=[](const auto &edge){return std::tuple(edge.is_resource,edge.member,edge.resource,edge.consumer,edge.block);};
    std::sort(edges->begin(),edges->end(),[&](const auto &a,const auto &b){return key(a)<key(b);});
    edges->erase(std::unique(edges->begin(),edges->end()),edges->end());
    dependencies=std::move(edges);
  }
};
std::uint16_t ClientShadowRecordView::Member() const noexcept {return data_->member;}
const PresentationMember &ClientShadowRecordView::Identity() const noexcept {return data_->identity;}
std::uint32_t ClientShadowRecordView::Mask() const noexcept {return data_->mask;}
const ClientFlatImageRef &ClientShadowRecordView::Flat() const noexcept {return data_->flat;}
std::span<const std::byte> ClientShadowRecordView::FlatValue(std::string_view name) const noexcept {
  const auto *layout=data_->flat?FindFlatLayout(data_->identity.layout):nullptr;
  return layout?ClientFlatNamed(*layout,*data_->flat,name):std::span<const std::byte>{};
}
std::optional<ClientShadowBlockView> ClientShadowRecordView::Block(std::uint8_t block) const noexcept {
  return block<32 && data_->blocks[block]?std::optional(data_->blocks[block]->view):std::nullopt;
}
bool ClientShadowRecordView::SameBlock(ClientShadowRecordView other,std::uint8_t block) const noexcept {
  if(block>=32||data_->identity!=other.data_->identity)return false;
  const auto &a=data_->blocks[block],&b=other.data_->blocks[block];
  return a==b||(a&&b&&a->content==b->content&&a->decoded==b->decoded&&
    std::equal(a->links.begin(),a->links.end(),b->links.begin(),b->links.end(),SameLink));
}
bool ClientShadowRecordView::SameContent(ClientShadowRecordView other,std::uint8_t block) const noexcept {
  if(block>=32||Identity()!=other.Identity())return false;
  const auto &a=data_->blocks[block],&b=other.data_->blocks[block];
  return a==b||(a&&b&&a->content==b->content);
}
bool ClientShadowRecordView::SameStructure(ClientShadowRecordView other,std::uint8_t block) const noexcept {
  if(block>=32||Identity()!=other.Identity())return false;
  const auto &a=data_->blocks[block],&b=other.data_->blocks[block];
  return a==b||(a&&b&&a->decoded==b->decoded);
}
bool ClientShadowRecordView::SameDependencies(ClientShadowRecordView other,std::uint8_t block) const noexcept {
  if(block>=32||Identity()!=other.Identity())return false;
  const auto &a=data_->blocks[block],&b=other.data_->blocks[block];
  return a==b||(a&&b&&std::equal(a->links.begin(),a->links.end(),b->links.begin(),b->links.end(),SameLink));
}
bool ClientShadowRecordView::SameField(ClientShadowRecordView other,std::uint16_t field) const noexcept {
  if(Identity()!=other.Identity())return false;
  const auto *a=Field(field),*b=other.Field(field);
  if(!a||!b||a->block!=b->block||a->present!=b->present)return false;
  if(SameBlock(other,a->block))return true;
  if(!BytesEqual(a->bytes,b->bytes))return false;
  const auto &x=*data_->blocks[a->block],&y=*other.data_->blocks[b->block];
  // 同一字段字节定界相同，再核它引用的资源／成员，避免目录序号相同却换了含义。
  for(const auto &node:x.decoded->nodes)if(!node.field&&!node.layout&&node.depth&&
      x.decoded->paths[node.path].field==field&&(node.operation==Operation::Resource||node.operation==Operation::Reference)){
    std::uint16_t index;
    if(node.size!=sizeof(index))return false;
    std::memcpy(&index,x.view.bytes.data()+node.offset,sizeof(index));
    if(index==kNullIndex)continue;
    const bool resource=node.operation==Operation::Resource;
    const auto matches=[&](const auto &link){return link.index==index&&link.resource==resource&&
      (!resource||link.entry.key.table==node.resource_table);};
    const auto before=std::find_if(x.links.begin(),x.links.end(),matches);
    const auto after=std::find_if(y.links.begin(),y.links.end(),matches);
    if(before==x.links.end()||after==y.links.end()||!SameLink(*before,*after))return false;
  }
  return true;
}
std::uint32_t ClientShadowRecordView::FieldVisits() const noexcept {return data_->visits;}
void ClientShadowRecordView::AppendResourceKeys(std::uint8_t block,ClientStateVector<ResourceKey> &out)const {
  if(block>=32||!data_->blocks[block])return;
  for(const auto &link:data_->blocks[block]->links)if(link.resource)out.push_back(link.entry.key);
}
void ClientShadowRecordView::Visit(std::uint32_t mask,BodyNodeObserver &observer) const {
  ClientShadowVisitCursor cursor;
  while(!VisitSome(mask,observer,cursor,std::numeric_limits<unsigned>::max())){}
}
bool ClientShadowRecordView::VisitSome(std::uint32_t mask,BodyNodeObserver &observer,ClientShadowVisitCursor &cursor,unsigned max_nodes) const {
  if(cursor.block>32)return true;
  for(;cursor.block<32;++cursor.block,cursor.node=0)if(mask&(std::uint32_t(1)<<cursor.block)){
    if(!VisitBlockSome(std::uint8_t(cursor.block),observer,cursor.node,max_nodes))return false;
  }
  // 根跨度不连续，根布局身份仍作为独立元信息交给观察者。
  observer.Node({{},data_->identity.layout,Operation::Nested,255,{}});
  ++cursor.block;return true;
}
bool ClientShadowRecordView::VisitBlockSome(std::uint8_t index,BodyNodeObserver &observer,
    std::size_t &position,unsigned &remaining) const {
  if(index>=data_->blocks.size())Fail(StateReadiness::InvalidEncoding,"shadow_visit_block");
  const auto &block=data_->blocks[index];
  if(!block||!block->decoded)Fail(StateReadiness::InvalidEncoding,"shadow_visit_block");
  const auto &decoded=*block->decoded;
  for(;position<decoded.nodes.size();++position){
    if(!remaining)return false;
    --remaining;const auto &node=decoded.nodes[position];
    const auto path=std::span(decoded.paths).subspan(node.path,node.depth);
    const auto bytes=block->view.bytes.subspan(node.offset,node.size);
    if(node.field)observer.Field({path,bytes,node.elements,node.slot_mask,node.present,node.payload_present,
      node.definition==UINT32_MAX?nullptr:&state::Fields()[node.definition]});
    else observer.Node({path,node.layout,node.operation,node.resource_table,bytes,node.component});
  }
  return true;
}
std::uint64_t ClientShadowRecordView::BlockFieldNodes(std::uint8_t index,std::size_t first,unsigned count) const {
  if(index>=data_->blocks.size())Fail(StateReadiness::InvalidEncoding,"shadow_visit_block_order");
  const auto &block=data_->blocks[index];
  if(!block||!block->decoded||count>64)Fail(StateReadiness::InvalidEncoding,"shadow_visit_block_order");
  const auto &nodes=block->decoded->nodes;
  if(first>nodes.size()||count>nodes.size()-first)Fail(StateReadiness::InvalidEncoding,"shadow_visit_block_order");
  std::uint64_t fields=0;
  for(unsigned i=0;i<count;++i)fields|=std::uint64_t(nodes[first+i].field)<<i;
  return fields;
}
std::span<const BodyFieldView> ClientShadowRecordView::Fields() const noexcept {return data_->fields;}
const BodyFieldView *ClientShadowRecordView::Field(std::uint16_t field) const noexcept {
  const auto it=std::find_if(data_->fields.begin(),data_->fields.end(),[&](const auto &value){return value.field==field;});return it==data_->fields.end()?nullptr:&*it;
}
std::span<const ProjectionFact> ClientShadowRecordView::Projections() const noexcept {return data_->facts;}
ClientShadowState::ClientShadowState(const std::shared_ptr<ClientStateMemory> &memory)
    :data_(std::allocate_shared<ClientShadowStateData>(ClientStateAllocator<ClientShadowStateData>(memory),memory)) {}
const PresentationBatchContext &ClientShadowState::Context() const noexcept {return data_->context;}
std::size_t ClientShadowState::RecordCount() const noexcept {return data_->records.size();}
ClientShadowRecordView ClientShadowState::Record(std::size_t index) const {
  if(index>=data_->records.size())Fail(StateReadiness::InvalidReference,"shadow_record_index");return ClientShadowRecordView(data_->records[index].get());
}
std::optional<ClientShadowRecordView> ClientShadowState::Find(const PresentationMember &identity) const noexcept {
  const auto *value=data_->Find(identity);return value?std::optional(ClientShadowRecordView(value)):std::nullopt;
}
std::optional<std::uint16_t> ClientShadowState::RecordPosition(const PresentationMember &identity) const noexcept {
  return data_->Position(identity);
}
const ClientMemberSnapshotRef &ClientShadowState::Members() const noexcept {return data_->members;}
const ClientResourceSnapshotRef &ClientShadowState::Resources() const noexcept {return data_->resources;}
std::span<const ClientFlatFault> ClientShadowState::FlatFaults() const noexcept {return data_->flat_faults;}
#ifdef ISAACCS_E2E
bool ClientShadowState::DeferredResource(std::uint16_t member) const noexcept {
  const auto &deferred=data_->deferred_resources;
  return std::find(deferred.begin(),deferred.end(),member)!=deferred.end();
}
#endif
const ClientFlatFault *ClientShadowState::FlatFault(const PresentationMember &identity) const noexcept {
  const auto at=std::find_if(data_->flat_faults.begin(),data_->flat_faults.end(),[&](const auto &f){return f.identity==identity;});
  return at==data_->flat_faults.end()?nullptr:&*at;
}
bool ClientShadowState::MemberClosureCompatible(const ClientMemberSnapshotRef &target) const noexcept {
  if(!target||target->Route()!=data_->members->Route())return false;
  if(target==data_->members)return true;
  const auto directory=target->Directory();
  const auto same=[&](std::uint16_t index,const PresentationMember &identity){
    const auto *entry=FindStateMember(directory,index);return entry&&entry->identity==identity;
  };
  for(const auto &record:data_->records){
    if(!same(record->member,record->identity))return false;
    if(record->flat)for(const auto &link:record->flat->links)if(!link.is_resource&&!same(link.index,link.member))return false;
    for(const auto &block:record->blocks)if(block)for(const auto &link:block->links)
      if(!link.resource&&!same(link.index,link.member))return false;
  }
  return true;
}
bool ClientShadowState::ResourceClosureCompatible(const ClientResourceSnapshotRef &target) const noexcept {
  if(!target)return false;
  if(target==data_->resources)return true;
  const auto directory=target->Directory();
  if(directory.session!=data_->context.scope.session||directory.run!=data_->context.scope.run)return false;
  // 同 run 单调追加，较新目录覆盖全部早期键；倒序检查实际消费的键。
  if(directory.epoch>=data_->resources->Directory().epoch)return true;
  for(const auto &record:data_->records){
    if(record->flat)for(const auto &link:record->flat->links)
      if(link.is_resource&&!FindResource(directory,link.resource))return false;
    for(const auto &block:record->blocks)if(block)for(const auto &link:block->links)
      if(link.resource&&!FindResource(directory,link.entry.key))return false;
  }
  return true;
}
std::span<const ClientShadowDependency> ClientShadowState::MemberConsumers(const PresentationMember &member) const noexcept {
  if(!data_->dependencies)return {};
  const auto &edges=*data_->dependencies;
  const auto key=[](const auto &edge){return std::pair(edge.is_resource,edge.member);};
  const auto sought=std::pair(false,member);
  const auto first=std::lower_bound(edges.begin(),edges.end(),sought,[&](const auto &edge,const auto &key_value){return key(edge)<key_value;});
  const auto last=std::upper_bound(first,edges.end(),sought,[&](const auto &key_value,const auto &edge){return key_value<key(edge);});
  return {first,last};
}
std::span<const ClientShadowDependency> ClientShadowState::ResourceConsumers(ResourceKey resource) const noexcept {
  if(!data_->dependencies)return {};
  const auto &edges=*data_->dependencies;
  const auto first_resource=std::partition_point(edges.begin(),edges.end(),[](const auto &edge){return !edge.is_resource;});
  const auto first=std::lower_bound(first_resource,edges.end(),resource,[](const auto &edge,auto key){return edge.resource<key;});
  const auto last=std::upper_bound(first,edges.end(),resource,[](auto key,const auto &edge){return key<edge.resource;});
  return {first,last};
}

// 原始规范正文与其精确帧元数据独立保留，ACK凭据不需要固定整条旧影子根。
struct ClientShadowPacket final {
  explicit ClientShadowPacket(const std::shared_ptr<ClientStateMemory> &memory)
      :bytes(ClientStateAllocator<std::byte>(memory,memory::Pool::Wire)),records(ClientStateAllocator<StateRecordPayload>(memory)),
       events(ClientStateAllocator<PresentationAudioEvent>(memory)) {}
  ClientStateVector<std::byte> bytes;
  ClientStateVector<StateRecordPayload> records;
  ClientStateVector<PresentationAudioEvent> events;
  StateFrameView frame;
  StateTarget target;
  void Freeze(const StateRoute &route,const StateFrameView &source) {
    std::size_t nr=0,ne=0;
    for(unsigned i=0;i<source.section_count;++i){nr+=source.sections[i].records.size();ne+=source.sections[i].presentation.events.size();}
    records.reserve(nr);events.reserve(ne);frame=source;
    for(unsigned i=0;i<source.section_count;++i){const auto &s=source.sections[i];const auto r=records.size(),e=events.size();
      records.insert(records.end(),s.records.begin(),s.records.end());
      events.insert(events.end(),s.presentation.events.begin(),s.presentation.events.end());
      frame.sections[i].records=std::span<const StateRecordPayload>(records).subspan(r,s.records.size());
      frame.sections[i].presentation.events=std::span<const PresentationAudioEvent>(events).subspan(e,s.presentation.events.size());}
    frame.encoded=bytes;target=DescribeStateTarget(route,frame);
  }
};
ClientStateCandidate::ClientStateCandidate(const std::shared_ptr<ClientStateMemory> &memory):changes_(ClientStateAllocator<ClientShadowMemberChange>(memory)) {}
const StateTarget &ClientStateCandidate::Target() const noexcept {return packet_->target;}
const StateFrameView &ClientStateCandidate::Frame() const noexcept {return packet_->frame;}
ClientShadowStateRef ClientStateCandidate::State(BodyStream stream) const {return states_[Stream(stream)];}
std::span<const ClientShadowMemberChange> ClientStateCandidate::Changes() const noexcept {return changes_;}

namespace {
// scratch按真实消息上界分配；投影facts遇到类型化不足才扩充，所有临时容量扣Workspace。
struct ShadowScratch final {
  template<class T> using Vector=ClientStateVector<T>;
  Vector<StateRecordPayload> records;
  Vector<PresentationAudioEvent> events;
  Vector<std::uint16_t> member_order;
  Vector<ProjectionFact> facts,combined;
  Vector<BodyFieldView> fields;
  explicit ShadowScratch(const std::shared_ptr<ClientStateMemory> &memory,std::size_t size)
      :records(ClientStateAllocator<StateRecordPayload>(memory,memory::Pool::Workspace)),
       events(ClientStateAllocator<PresentationAudioEvent>(memory,memory::Pool::Workspace)),
       member_order(ClientStateAllocator<std::uint16_t>(memory,memory::Pool::Workspace)),
       facts(ClientStateAllocator<ProjectionFact>(memory,memory::Pool::Workspace)),
       combined(ClientStateAllocator<ProjectionFact>(memory,memory::Pool::Workspace)),fields(ClientStateAllocator<BodyFieldView>(memory,memory::Pool::Workspace)) {
    const auto head=UsesFlatRecordContract(LayoutVersion())?kFlatRecordHeaderBytes:kW_StateRecordHeaderBytes;
    records.resize((std::min)(std::size_t(kW_frame_records),size/head));member_order.resize(records.size());
    events.resize((std::min)(std::size_t(kW_max_audio_events),size/kW_StateAudioEventHeaderBytes));
    // 普通的 Player 联合帧会产生数十项投影事实，预留基础工作区，避免逐帧抛异常扩容。
    fields.resize(BodySourceMaximumFields());facts.resize(128);
  }
  BodyCodecScratch Body(BodyNodeObserver *observer=nullptr) {BodyCodecScratch body;body.facts=facts;body.root_fields=fields;body.observer=observer;return body;}
  StateFrameScratch Frame() {return {records,events,member_order,Body()};}
  void Grow(const BodyScratchError &error,BodyCodecLimits limits) {
    const auto maximum=error.Region()==BodyScratchRegion::Facts?limits.field_visits:BodySourceMaximumFields();
    const auto current=error.Region()==BodyScratchRegion::Facts?facts.size():fields.size();
    const auto count=(std::min)(std::size_t(maximum),(std::max)({std::size_t(error.Requested()),current*2,std::size_t(16)}));
    if(count<=current || count<error.Requested())throw ClientTableError(ClientTableFailure::Budget,"shadow_scratch_visit_budget",error.Requested(),maximum,memory::Pool::Workspace);
    if(error.Region()==BodyScratchRegion::Facts)facts.resize(count);else fields.resize(count);
  }
};
std::optional<bool> Presence(const ClientShadowRecordData *record,const CodecLocation &location) {
  if(record)for(const auto &fact:record->facts)if(fact.kind==ProjectionFactKind::SharedPresence && fact.layout==location.layout &&
      fact.group==location.canonical_field && fact.depth==location.parents.size() &&
      std::equal(location.parents.begin(),location.parents.end(),fact.parents.begin()))return fact.value!=0;
  return {};
}
class ShadowBodyContext final : public BodyCodecContext {
public:
  ShadowBodyContext(const ClientResourceSnapshotRef &resources,const ClientShadowRecordData *other=nullptr,const ClientShadowRecordData *base=nullptr)
      :directory_(resources->Directory()),other_(other),base_(base) {}
  std::optional<CodecItemConfig> Item(std::uint8_t table,std::uint16_t index) const override {return ResourceBodyContext(directory_).Item(table,index);}
  std::optional<bool> SharedPresence(const CodecLocation &location) const override {
    const auto value=Presence(other_,location);return value?value:Presence(base_,location);
  }
private:
  ResourceDirectoryView directory_;
  const ClientShadowRecordData *other_,*base_;
};
struct ObservedLink {std::uint8_t block=0;ClientShadowLink link;};
class ShadowLinks final : public BodyNodeObserver {
public:
  ShadowLinks(std::uint16_t layout,const ClientMemberSnapshotRef &members,const ClientResourceSnapshotRef &resources,
      const std::shared_ptr<ClientStateMemory> &memory,const std::array<BodyBlockInput,32> &input):
      values(ClientStateAllocator<ObservedLink>(memory,memory::Pool::Workspace)),memory_(memory),input_(input),
      layout_(layout),members_(members->Directory()),resources_(resources->Directory()) {}
  void Reset() {values.clear();for(auto &b:blocks_)if(b){b->paths.clear();b->nodes.clear();}}
  const ClientShadowDecodedBlock *Decoded(unsigned block) const noexcept {return blocks_[block]?&*blocks_[block]:nullptr;}
  void Field(const BodyFieldNodeView &value) override {
    if(value.path.empty())Fail(StateReadiness::InvalidEncoding,"shadow_structure_field_path",kNullIndex,layout_);
    ClientShadowNode n;
    n.field=true;n.elements=value.elements;n.slot_mask=value.slot_mask;n.present=value.present;n.payload_present=value.payload_present;
    const auto &leaf=value.path.back();
    const auto *definition=FindField(leaf.layout,leaf.field);
    const auto *handler=FindClientField(leaf.layout,leaf.field);
    if(!definition||!handler||handler->operation!=definition->operation)
      Fail(StateReadiness::InvalidEncoding,"shadow_structure_field_owner",kNullIndex,leaf.layout,leaf.field);
    n.definition=std::uint32_t(definition-state::Fields().data());
    Remember(value.path,value.bytes,n);
  }
  void Node(const BodyNodeView &node) override {
    if(node.path.empty())return;
    (void)ValidateClientDeclaredNode(node,members_,resources_);
    ClientShadowNode n;
    n.layout=node.layout;n.component=node.component;n.operation=node.operation;n.resource_table=node.resource_table;
    Remember(node.path,node.bytes,n);
    if(node.layout || (node.operation!=Operation::Reference && node.operation!=Operation::Resource))return;
    if(node.path.empty() || node.bytes.size()!=2)Fail(StateReadiness::InvalidEncoding,"shadow_link_shape",kNullIndex,layout_);
    const auto index=std::uint16_t(std::to_integer<unsigned>(node.bytes[0])|(std::to_integer<unsigned>(node.bytes[1])<<8));
    if(index==kNullIndex)return;const auto *field=FindField(layout_,node.path.front().field);
    if(!field)Fail(StateReadiness::InvalidEncoding,"shadow_link_field",kNullIndex,layout_);
    ClientShadowLink link;link.index=index;link.resource=node.operation==Operation::Resource;
    if(link.resource){const auto *entry=FindResource(resources_,{node.resource_table,index});
      if(!entry)Fail(StateReadiness::MissingClosure,"shadow_resource_not_declared",kNullIndex,layout_);link.entry=*entry;}
    else {const auto *entry=FindStateMember(members_,index);if(!entry)Fail(StateReadiness::InvalidReference,"shadow_member_not_declared",index,layout_);link.member=entry->identity;}
    values.push_back({field->block,link});
  }
  ClientStateVector<ObservedLink> values;
private:
  void Remember(std::span<const CodecPathStep> path,std::span<const std::byte> bytes,ClientShadowNode n) {
    const auto *field=path.empty()?nullptr:FindField(layout_,path.front().field);
    if(!field)Fail(StateReadiness::InvalidEncoding,"shadow_node_path",kNullIndex,layout_);
    auto &block=blocks_[field->block];if(!block)block.emplace(memory_,memory::Pool::Workspace);
    n.path=std::uint32_t(block->paths.size());n.depth=std::uint8_t(path.size());
    n.offset=bytes.empty()?0:std::uint32_t(Offset(input_[field->block].bytes,bytes));n.size=std::uint32_t(bytes.size());
    block->paths.insert(block->paths.end(),path.begin(),path.end());block->nodes.push_back(n);
  }
  std::shared_ptr<ClientStateMemory> memory_;
  const std::array<BodyBlockInput,32> &input_;
  std::array<std::optional<ClientShadowDecodedBlock>,32> blocks_;
  std::uint16_t layout_;
  StateMemberDirectory members_;
  ResourceDirectoryView resources_;
};
BodyView DecodeRoot(std::uint16_t layout,std::uint32_t mask,BodyStream stream,std::span<const std::byte> bytes,
    ShadowScratch &scratch,const BodyCodecContext &context,BodyCodecLimits limits) {
  for(;;){auto body=scratch.Body();
    try {return DecodeBody(layout,mask,stream,bytes,body,&context,limits);}
    catch(const BodyScratchError &error){scratch.Grow(error,limits);}}
}
} // namespace

// 所有可变提交／回收状态只有后台owner访问；三个registry保住跨线程纯值借用的最后引用。
// 工作片持有纯值输入和游标。跨收包轮次保留内存账目，执行租约只覆盖一次推进。
struct ClientShadowBuildWork final {
  std::shared_ptr<ClientShadowPacket> packet;
  ShadowScratch scratch;
  StateFrameView frame;
  StateFrameDecodeCursor wire;
  StateAcceptDecision decision;
  std::shared_ptr<ClientStateCandidate> candidate;
  std::array<std::shared_ptr<ClientShadowState>,2> roots{};
  // 非零 base 使用本流解码根，对侧和 base=0 使用开工时的已安装根。
  std::array<ClientShadowStateRef,2> decode_inputs{};
  std::array<ClientStateVector<StateRecordPayload>,2> incoming;
  // 仅表中真实改义的入口向消费者传播，继承块仍核精确引用。
  using Invalidated=std::pair<PresentationMember,std::uint8_t>;
  std::array<ClientStateVector<Invalidated>,2> invalidated;
  std::array<bool,2> dependencies_changed{};
  std::uint32_t remaining;
  std::size_t member=0;
  bool merged=false;
  ClientShadowBuildWork(const std::shared_ptr<ClientStateMemory> &memory,std::size_t size,std::uint32_t visits):
    packet(std::allocate_shared<ClientShadowPacket>(ClientStateAllocator<ClientShadowPacket>(memory),memory)),
    scratch(memory,size),incoming{ClientStateVector<StateRecordPayload>(ClientStateAllocator<StateRecordPayload>(memory,memory::Pool::Workspace)),
      ClientStateVector<StateRecordPayload>(ClientStateAllocator<StateRecordPayload>(memory,memory::Pool::Workspace))},
    invalidated{ClientStateVector<Invalidated>(ClientStateAllocator<Invalidated>(memory,memory::Pool::Workspace)),
      ClientStateVector<Invalidated>(ClientStateAllocator<Invalidated>(memory,memory::Pool::Workspace))},remaining(visits) {}

  void FindInvalidated(unsigned stream) {
    const auto &before=decode_inputs[stream];const auto &after=roots[stream];
    if(!after||!after->Context().base_version||!before){dependencies_changed[stream]=true;return;}
    auto &out=invalidated[stream];
    const auto append=[&](auto consumers){for(const auto &edge:consumers)if(edge.block<32)out.emplace_back(edge.consumer,edge.block);};
    if(before->Members()!=after->Members()){
      dependencies_changed[stream]=true;
      const auto target=after->Members()->Directory();
      for(const auto &entry:before->Members()->Directory().entries){
        const auto *next=FindStateMember(target,entry.index);
        if(!next||next->identity!=entry.identity)append(before->MemberConsumers(entry.identity));
      }
    }
    if(before->Resources()!=after->Resources()){
      const auto target=after->Resources()->Directory();
      for(const auto &entry:before->Resources()->Directory().entries){
        const auto consumers=before->ResourceConsumers(entry.key);
        if(consumers.empty())continue;
        const auto *next=FindResource(target,entry.key);
        if(!next||next->kind!=entry.kind||!BytesEqual(next->payload,entry.payload))append(consumers);
      }
    }
    std::sort(out.begin(),out.end());out.erase(std::unique(out.begin(),out.end()),out.end());
  }
  std::uint32_t InvalidatedBlocks(unsigned stream,const PresentationMember &member)const {
    const auto &values=invalidated[stream];std::uint32_t mask=0;
    for(auto i=std::lower_bound(values.begin(),values.end(),Invalidated{member,0});i!=values.end()&&i->first==member;++i)
      mask|=std::uint32_t(1)<<i->second;
    return mask;
  }
};

struct ClientShadowStorage final {
  struct Binding {ClientShadowLayoutBinding input;std::array<std::uint32_t,2> masks{};std::uint32_t record_bytes=32u<<20;unsigned first=0;};
  std::shared_ptr<ClientStateMemory> memory;
  std::thread::id owner;
  StateRoute route{};
  bool active=false;
  ClientStateAcceptance acceptance;
  std::array<ClientShadowStateRef,2> installed{};
  // 环只保留纯值和目录，原生 installed 由应用进度独立持有。
  std::array<ClientStateVector<ClientShadowStateRef>,2> received;
  std::array<std::size_t,2> next_received{};
  std::array<std::uint32_t,2> received_versions{};
  std::array<std::optional<StateAppliedAck>,2> received_acks{};
  std::uint64_t received_events=0;
  // 物理借用按票据持有；撤逻辑资格后仍等对应主线程归还。
  struct Work {
    ClientStateCandidateRef candidate;
    StateApplyTicket ticket;
    bool revoked=false,awaiting_drain=false;
  };
  std::array<Work,2> work{};
  bool Occupied(std::uint8_t mask)const {
    return std::any_of(work.begin(),work.end(),[&](const auto &slot){
      return slot.candidate&&(StateApplyOccupancy(slot.candidate->Target())&mask);
    });
  }
  Work *FindWork(StateApplyTicket ticket) {
    for(auto &slot:work)if(slot.candidate&&SameTicket(slot.ticket,ticket))return &slot;
    return nullptr;
  }
  ClientStateVector<Binding> bindings;
  ClientStateVector<std::shared_ptr<const ClientShadowPacket>> packets;
  ClientStateVector<ClientShadowStateRef> states;
  ClientStateVector<ClientStateCandidateRef> candidates;

  ClientShadowStorage(std::shared_ptr<ClientStateMemory> account,std::thread::id thread,std::span<const ClientShadowLayoutBinding> input)
      :memory(std::move(account)),owner(thread),
       received{ClientStateVector<ClientShadowStateRef>(ClientStateAllocator<ClientShadowStateRef>(memory)),
                ClientStateVector<ClientShadowStateRef>(ClientStateAllocator<ClientShadowStateRef>(memory))},bindings(ClientStateAllocator<Binding>(memory)),
       packets(ClientStateAllocator<std::shared_ptr<const ClientShadowPacket>>(memory)),states(ClientStateAllocator<ClientShadowStateRef>(memory)),
       candidates(ClientStateAllocator<ClientStateCandidateRef>(memory)) {
    for(auto &ring:received)ring.resize(StateBudgets().decode_snapshots);
    Thread();if(input.empty())Fail(StateReadiness::InvalidReference,"shadow_empty_layout_bindings");bindings.reserve(input.size());
    for(const auto &value:input){if(!value.space)Fail(StateReadiness::InvalidReference,"shadow_invalid_member_space");
      Binding bound;bound.input=value;bound.masks={Mask(value.layout,BodyStream::World),Mask(value.layout,BodyStream::Body)};
      if(const auto *flat=FlatRecordLayout(LayoutVersion(),value.layout)){
        for(unsigned stream=0;stream<2;++stream)bound.masks[stream]=
          std::any_of(flat->stream_fields[stream].begin(),flat->stream_fields[stream].end(),[](auto bits){return bits!=0;});
        bindings.push_back(bound);
        continue;
      }
      for(const auto &budget:LayoutBudgets())if(budget.layout==value.layout){bound.record_bytes=budget.record_bytes;break;}
      if(value.metadata_only)bound.masks={};
      const auto *layout=FindLayout(value.layout);bool world_first=false,body_first=false;
      for(unsigned i=0;i<layout->field_count;++i){const auto &field=Fields()[layout->first_field+i];BodySourceFieldInfo info;
        if(!field.max_size || !DescribeBodySourceField(value.layout,field.index,info) || !info.shared_owner)continue;
        const auto *header=FindField(value.layout,info.canonical);if(!header)Fail(StateReadiness::InvalidReference,"shadow_shared_owner_plan");
        const bool a=Blocks()[layout->first_block+header->block].body,b=Blocks()[layout->first_block+field.block].body;
        body_first|=a && !b;world_first|=!a && b;
      }
      if(world_first && body_first)Fail(StateReadiness::InvalidReference,"shadow_cyclic_cross_stream_presence");bound.first=body_first?1u:0u;bindings.push_back(bound);
    }
    const auto key=[](const Binding &value){return std::pair(value.input.space,value.input.layout);};
    std::sort(bindings.begin(),bindings.end(),[&](const auto &a,const auto &b){return key(a)<key(b);});
    for(std::size_t i=1;i<bindings.size();++i)if(key(bindings[i-1])==key(bindings[i]))Fail(StateReadiness::InvalidReference,"shadow_duplicate_layout_binding");
  }
  void Thread() const {if(owner!=std::this_thread::get_id())throw ClientTableError(ClientTableFailure::WrongThread,"shadow_wrong_thread");}
  void Route(const StateRoute &received) const {Thread();if(!active || route!=received)throw ClientTableError(ClientTableFailure::WrongRoute,"shadow_wrong_route");}
  const Binding &Policy(const PresentationMember &member) const {
    const auto key=std::pair(member.space,member.layout);
    const auto it=std::lower_bound(bindings.begin(),bindings.end(),key,[](const Binding &value,const auto &target){return std::pair(value.input.space,value.input.layout)<target;});
    if(it==bindings.end() || std::pair(it->input.space,it->input.layout)!=key)Fail(StateReadiness::InvalidReference,"shadow_unregistered_root",kNullIndex,member.layout);return *it;
  }
  void Collect() {
    Thread();std::erase_if(candidates,[](const auto &value){return value.use_count()==1;});
    std::erase_if(states,[](const auto &value){return value.use_count()==1;});
    std::erase_if(packets,[](const auto &value){return value.use_count()==1;});
  }
  bool Drained() const {
    if(active || Occupied(3))return false;
    for(const auto &value:candidates)if(value.use_count()>1)return false;
    // 未回收候选自身也持有state，先由Collect清理才能给出准确排空结果。
    return candidates.empty() && states.empty() && packets.empty();
  }
  const ClientShadowStateData *Installed(unsigned stream) const noexcept {
    return installed[stream] && !(acceptance.UnusableStreams()&(1u<<stream))?installed[stream]->data_.get():nullptr;
  }
  ClientShadowStateRef DecodeBase(unsigned stream,std::uint32_t version) const {
    for(const auto &root:received[stream])
      if(root&&root->Context().target_version==version)return root;
    return {};
  }
  StateAcceptDecision Envelope(const StateTarget &target) const {
    const auto streams=StateTargetStreams(target);
    if(!active||target.route!=route)return {StateAcceptAction::Drop,streams};
    for(const auto &known:packets)if(StateTargetsConflict(known->target,target))
      return {StateAcceptAction::Terminal,streams};
    for(unsigned i=0;i<target.count;++i){
      const auto &section=target.sections[i];
      if(section.target_version<=received_versions[Stream(section.stream)])return {StateAcceptAction::Reack,streams};
    }
    for(unsigned i=0;i<target.count;++i){
      const auto &section=target.sections[i];
      if(section.stream==BodyStream::World&&section.event_watermark<received_events)
        return {StateAcceptAction::Terminal,streams,StateResyncReason::Event};
      if(section.base_version&&!DecodeBase(Stream(section.stream),section.base_version))
        return {StateAcceptAction::Resync,streams,StateResyncReason::Base};
    }
    return {StateAcceptAction::Decode,streams};
  }
  void ClearReceived(std::uint8_t mask) noexcept {
    for(unsigned i=0;i<2;++i)if(mask&(1u<<i)){
      for(auto &root:received[i])root.reset();
      next_received[i]=0;received_acks[i].reset();
    }
  }
  bool Known(const ClientStateCandidateRef &candidate) const {
    return candidate && std::any_of(candidates.begin(),candidates.end(),[&](const auto &known){return known==candidate;});
  }
  const ClientShadowStateData *Effective(const ClientStateCandidate &candidate,unsigned stream) const {
    return candidate.states_[stream]?candidate.states_[stream]->data_.get():Installed(stream);
  }
  static bool LinksEqual(const ClientShadowBlockData &before,std::span<const ObservedLink> links,unsigned block) {
    std::size_t i=0;for(const auto &value:links)if(value.block==block){if(i>=before.links.size() || !SameLink(before.links[i++],value.link))return false;}return i==before.links.size();
  }
  // 无关表追加保留已有块。实际引用必须在本批精确表中仍解释为同一成员或同一资源声明。
  static bool LinksCurrent(const ClientShadowBlockData &block,const ClientShadowStateData &target) {
    for(const auto &link:block.links){
      if(link.resource){
        const auto *entry=FindResource(target.resources->Directory(),link.entry.key);
        if(!entry||entry->kind!=link.entry.kind||!BytesEqual(entry->payload,link.entry.payload))return false;
      }else{
        const auto *member=FindStateMember(target.members->Directory(),link.index);
        if(!member||member->identity!=link.member)return false;
      }
    }
    return true;
  }

  // 用实际已安装值归一已经完整解码的 World 记录。块等价要求字节、定界和精确引用均相同。
  bool NormalizeWorldRecord(ClientShadowRecordData &record,const ClientShadowRecordData &current,
      const ClientResourceSnapshotRef &resources) {
    for(unsigned i=0;i<record.blocks.size();++i){
      auto &block=record.blocks[i];
      const auto &accepted=current.blocks[i];
      if(block==accepted||!block||!accepted||block->visits!=accepted->visits||
          (block->content!=accepted->content&&!BytesEqual(block->view.bytes,accepted->view.bytes))||
          !std::equal(block->links.begin(),block->links.end(),accepted->links.begin(),accepted->links.end(),SameLink))continue;
      if(block->decoded!=accepted->decoded&&
          (block->decoded->nodes!=accepted->decoded->nodes||block->decoded->paths!=accepted->decoded->paths))continue;

      auto field=std::lower_bound(record.fields.begin(),record.fields.end(),i,[](const auto &value,unsigned block){return value.block<block;});
      for(;field!=record.fields.end()&&field->block==i;++field){
        field->bytes=field->bytes.empty()?std::span<const std::byte>{}:
          accepted->view.bytes.subspan(Offset(block->view.bytes,field->bytes),field->bytes.size());
      }
      if(!accepted->resources||accepted->resources==resources){
        block=accepted;continue;
      }
      // 资源链接的 payload 借自本目标快照；值、结构及来源沿用已安装块，避免额外固定旧表。
      auto rebound=std::allocate_shared<ClientShadowBlockData>(ClientStateAllocator<ClientShadowBlockData>(memory),*block);
      rebound->view=accepted->view;rebound->content=accepted->content;rebound->decoded=accepted->decoded;
      block=std::move(rebound);
    }
    return current.member==record.member&&current.member_epoch==record.member_epoch&&current.resource_epoch==record.resource_epoch&&
      current.visits==record.visits&&current.mask==record.mask&&current.blocks==record.blocks&&current.facts==record.facts&&
      std::equal(current.fields.begin(),current.fields.end(),record.fields.begin(),record.fields.end(),[](const auto &a,const auto &b){
        return a.field==b.field&&a.block==b.block&&a.present==b.present&&a.bytes.size()==b.bytes.size()&&
          (a.bytes.empty()||a.bytes.data()==b.bytes.data());
      });
  }

  // 只借已成功安装的同身份base；完整检查后封存新record，异常不改变任何已发布根。
  std::shared_ptr<const ClientShadowRecordData> MakeRecord(const StateMemberEntry &member,const Binding &policy,
      ClientShadowStateData &target,const StateRecordPayload *incoming,const std::shared_ptr<const ClientShadowRecordData> &base,
      const ClientShadowRecordData *retained,const std::shared_ptr<const ClientShadowRecordData> &current,
      const ClientShadowRecordData *other,ShadowScratch &scratch,BodyCodecLimits wire_limits,std::uint32_t &remaining,std::uint32_t invalidated
#ifdef ISAACCS_E2E
      ,ClientApplyDetailTimer::Clock::duration &normalize_elapsed
#endif
      ) {
    const auto mask=policy.masks[Stream(target.context.stream)];
    bool same_presence=true;if(base && other)for(const auto &fact:base->facts)if(fact.kind==ProjectionFactKind::SharedPresence){
      const CodecLocation location{std::span(fact.parents).first(fact.depth),fact.layout,fact.field,fact.group};const auto value=Presence(other,location);
      if(value && *value!=(fact.value!=0)){same_presence=false;break;}}
    if(base&&(!current||base==current)&&!incoming&&same_presence&&base->member==member.index&&
        base->member_epoch==target.context.member_epoch&&base->resource_epoch==target.context.resource_epoch)return base;
    auto result=std::allocate_shared<ClientShadowRecordData>(ClientStateAllocator<ClientShadowRecordData>(memory),memory);
    result->member=member.index;result->identity=member.identity;result->mask=mask;
    result->member_epoch=target.context.member_epoch;result->resource_epoch=target.context.resource_epoch;
    if(base)result->blocks=base->blocks;
    ShadowBodyContext context(target.resources,other,base.get());
    if(incoming){
      if(incoming->layout!=member.identity.layout || !incoming->mask || (incoming->mask&~mask))
        Fail(StateReadiness::InvalidEncoding,"shadow_record_layout_or_stream",member.index,member.identity.layout);
      const auto body=DecodeRoot(incoming->layout,incoming->mask,target.context.stream,incoming->bytes,scratch,context,wire_limits);
      for(unsigned i=0;i<body.block_count;++i){const auto &block=body.blocks[i];
        if(const auto &old=result->blocks[block.block];old&&BytesEqual(old->view.bytes,block.bytes))continue;
        auto value=std::allocate_shared<ClientShadowBlockData>(ClientStateAllocator<ClientShadowBlockData>(memory),memory);
        // 携带的块独立验证；先借逐字节相等的存储，完整检查后才可复用引用语义和来源。
        const auto *storage=current?current.get():retained;
        const auto old=storage?storage->blocks[block.block]:nullptr;
        if(old&&BytesEqual(old->view.bytes,block.bytes))value->content=old->content;
        else {
          auto content=std::allocate_shared<ClientShadowBytes>(ClientStateAllocator<ClientShadowBytes>(memory),memory);
          content->bytes.assign(block.bytes.begin(),block.bytes.end());value->content=std::move(content);
        }
        value->view={block.block,target.context.target_version,target.context.game_frame,target.context.member_epoch,
          target.context.resource_epoch,target.context.source_revision,value->content->bytes};result->blocks[block.block]=std::move(value);}
    }
    std::uint64_t total=0;for(unsigned i=0;i<32;++i)if(mask&(std::uint32_t{1}<<i)){
      if(!result->blocks[i])Fail(StateReadiness::MissingClosure,"shadow_incomplete_new_member_or_shape",member.index,member.identity.layout);
      total+=result->blocks[i]->view.bytes.size();}
    if(total>(32u<<20))throw ClientTableError(ClientTableFailure::Budget,"shadow_complete_record_bytes",total,32u<<20,memory::Pool::Workspace);
    std::array<BodyBlockInput,32> input{};
    for(unsigned i=0;i<32;++i)if(mask&(std::uint32_t(1)<<i)){
      const auto &block=result->blocks[i];
      input[i].bytes=block->view.bytes;
      if(!base||block!=base->blocks[i]||!block->decoded||
          ((invalidated&(std::uint32_t(1)<<i))&&!LinksCurrent(*block,target)))continue;
      // 共享存在位可来自同记录其它块或配对流，重新定界实际消费者。
      if(std::any_of(base->facts.begin(),base->facts.end(),[&](const auto &fact){
          return fact.root_block==i&&fact.kind==ProjectionFactKind::SharedPresence;
        }))continue;
      const auto first=std::lower_bound(base->fields.begin(),base->fields.end(),i,[](const auto &f,unsigned b){return f.block<b;});
      const auto last=std::upper_bound(first,base->fields.end(),i,[](unsigned b,const auto &f){return b<f.block;});
      input[i].cached_fields={first,last};input[i].cached_facts=base->facts;
      input[i].cached_visits=block->visits;input[i].links=!block->links.empty();
    }
    if(!remaining)throw ClientTableError(ClientTableFailure::Budget,"shadow_complete_target_visits",1,0,memory::Pool::Workspace);
    BodyCodecLimits complete{32u<<20,remaining,wire_limits.depth,true};
    ShadowLinks links(member.identity.layout,target.members,target.resources,memory,input);
    BodyView body;
    for(;;){
      links.Reset();auto decoder=scratch.Body(&links);
      try{body=DecodeBodyBlocks(member.identity.layout,mask,target.context.stream,input,decoder,&context,complete);break;}
      catch(const BodyScratchError &e){scratch.Grow(e,complete);}
      catch(const BodyVisitError &e){throw ClientTableError(ClientTableFailure::Budget,"shadow_complete_target_visits",e.Requested(),e.Available(),memory::Pool::Workspace);}
    }
    if(body.field_visits>remaining)throw ClientTableError(ClientTableFailure::Budget,"shadow_complete_target_visits");remaining-=body.field_visits;
    // 只持有真正的投影事实及根字段视图，不保存整个递归字段树或临时observer路径。
    result->visits=body.field_visits;result->facts.assign(body.facts.begin(),body.facts.end());
    result->fields.assign(body.fields.begin(),body.fields.end());
    for(unsigned i=0;i<body.block_count;++i){const auto &block=body.blocks[i];const auto &before=result->blocks[block.block];
      const bool carried=incoming&&(incoming->mask&(std::uint32_t{1}<<block.block));
      const bool same_links=block.reused||LinksEqual(*before,links.values,block.block);
      if(!carried&&!same_links)Fail(StateReadiness::MissingClosure,"shadow_inherited_link_rebound",member.index,member.identity.layout);
      const auto *observed=links.Decoded(block.block);
      // 结构模板与正文生命周期独立。标量变化可继续借相同数字路径、定界与字段模板。
      const auto decoded=before->decoded?before->decoded:
        base&&base->blocks[block.block]?base->blocks[block.block]->decoded:
        retained&&retained->blocks[block.block]?retained->blocks[block.block]->decoded:nullptr;
      const bool same_nodes=block.reused||(decoded&&((!observed&&decoded->nodes.empty())||
        (observed&&decoded->nodes==observed->nodes&&decoded->paths==observed->paths)));
      const bool changed=!same_links||!same_nodes||!before->decoded;
      if(!changed&&(!before->resources||before->resources==target.resources))continue;
      auto value=std::allocate_shared<ClientShadowBlockData>(ClientStateAllocator<ClientShadowBlockData>(memory),memory);
      value->view=before->view;value->content=before->content;
      value->visits=block.field_visits;
      if(carried&&changed)value->view={block.block,target.context.target_version,target.context.game_frame,target.context.member_epoch,
        target.context.resource_epoch,target.context.source_revision,value->content->bytes};
      if(same_nodes)value->decoded=decoded;
      else {
        auto cache=std::allocate_shared<ClientShadowDecodedBlock>(ClientStateAllocator<ClientShadowDecodedBlock>(memory),memory,memory::Pool::Values);
        if(observed){
          cache->paths.assign(observed->paths.begin(),observed->paths.end());
          cache->nodes.assign(observed->nodes.begin(),observed->nodes.end());
        }
        value->decoded=std::move(cache);
      }
      if(block.reused){
        value->links=before->links;
        for(auto &link:value->links)if(link.resource){link.entry=*FindResource(target.resources->Directory(),link.entry.key);value->resources=target.resources;}
      }else for(const auto &link:links.values)if(link.block==block.block){
        value->links.push_back(link.link);if(link.link.resource)value->resources=target.resources;
      }
      // 没有链接的旧块无需携带旧表；其值owner独立共享，不固定旧record／candidate。
      result->blocks[block.block]=std::move(value);
    }
    if(!policy.masks[1-Stream(target.context.stream)]){
      scratch.combined.assign(result->facts.begin(),result->facts.end());ValidateProjectionFacts(scratch.combined,true);}
    if(current){
#ifdef ISAACCS_E2E
      ClientApplyDetailTimer measured(&normalize_elapsed);
#endif
      if(NormalizeWorldRecord(*result,*current,target.resources))return current;
    }
    // 发送了相同正文与语义时，成员子表也沿用旧版本。
    if(base&&same_presence&&base->member==member.index&&base->member_epoch==result->member_epoch&&base->resource_epoch==result->resource_epoch&&
        result->blocks==base->blocks)return base;
    return result;
  }

  // 两序列仅在同一真实成员的投影上联合检查；body不等待无关NPC／地图／资源就绪。
  void Projections(const ClientStateCandidate &candidate,ShadowScratch &scratch) const {
    for(unsigned stream=0;stream<2;++stream)if(candidate.states_[stream]){
      const auto &own=*candidate.states_[stream]->data_;const auto *other=Effective(candidate,1-stream);
      for(const auto &record:own.records){if(!Policy(record->identity).masks[1-stream])continue;
        const auto *paired=other?other->Find(record->identity):nullptr;
        if(!paired)Fail(StateReadiness::MissingClosure,"shadow_missing_paired_member",record->member,record->identity.layout);
        std::uint64_t bytes=0;for(unsigned block=0;block<32;++block){if(record->blocks[block])bytes+=record->blocks[block]->view.bytes.size();if(paired->blocks[block])bytes+=paired->blocks[block]->view.bytes.size();}
        if(bytes>Policy(record->identity).record_bytes)Fail(StateReadiness::Budget,"shadow_combined_record_byte_budget",record->member,record->identity.layout);
        scratch.combined.assign(record->facts.begin(),record->facts.end());scratch.combined.insert(scratch.combined.end(),paired->facts.begin(),paired->facts.end());
        try {ValidateProjectionFacts(scratch.combined,true);}
        catch(const Gap &){Fail(StateReadiness::MissingClosure,"shadow_incompatible_paired_projections",record->member,record->identity.layout);}
      }
      if(other)for(const auto &record:other->records)if(Policy(record->identity).masks[stream] && !own.Find(record->identity))
        Fail(StateReadiness::MissingClosure,"shadow_unpaired_structural_change",record->member,record->identity.layout);
    }
  }
  void Changes(ClientStateCandidate &candidate) {
    if(!candidate.states_[0])return;
    // 成员进出相对当前已安装 World 计算，前代只用于还原目标。
    const auto next=candidate.states_[0]->Members()->Directory();const auto *before=Installed(0);
    ClientStateVector<StateMemberEntry> old{ClientStateAllocator<StateMemberEntry>(memory,memory::Pool::Workspace)},fresh{ClientStateAllocator<StateMemberEntry>(memory,memory::Pool::Workspace)};
    if(before)for(const auto &entry:before->members->Directory().entries)
      if(entry.identity.layout==195||before->Find(entry.identity))old.push_back(entry);
    for(const auto &entry:next.entries)
      if(entry.identity.layout==195||candidate.states_[0]->Find(entry.identity))fresh.push_back(entry);
    const auto less=[](const auto &a,const auto &b){return MemberKey(a.identity)<MemberKey(b.identity);};
    std::sort(old.begin(),old.end(),less);std::sort(fresh.begin(),fresh.end(),less);candidate.changes_.reserve(old.size()+fresh.size());std::size_t a=0,b=0;
    while(a<old.size() || b<fresh.size()){
      if(b==fresh.size() || (a<old.size() && less(old[a],fresh[b])))candidate.changes_.push_back({ClientShadowChangeKind::Leave,old[a++],{}});
      else if(a==old.size() || less(fresh[b],old[a]))candidate.changes_.push_back({ClientShadowChangeKind::Join,{},fresh[b++]});
      else {const auto &x=old[a++],&y=fresh[b++];
        if(x.identity.use!=y.identity.use){candidate.changes_.push_back({ClientShadowChangeKind::Leave,x,{}});candidate.changes_.push_back({ClientShadowChangeKind::Join,{},y});}
        else if(x.identity!=y.identity)candidate.changes_.push_back({ClientShadowChangeKind::Reshape,x,y});
        else if(x.index!=y.index)candidate.changes_.push_back({ClientShadowChangeKind::Remap,x,y});}
    }
  }
  struct WireContext final : StateFrameCodecContext {
    explicit WireContext(const ClientStateCandidate &value):candidate(value) {}
    std::optional<PresentationMember> Member(std::uint32_t epoch,std::uint16_t index) const override {
      for(const auto &state:candidate.states_)if(state && state->Context().member_epoch==epoch){const auto *entry=FindStateMember(state->Members()->Directory(),index);if(entry)return entry->identity;}return {};
    }
    const BodyCodecContext *BodyContext(const PresentationBatchContext &context,std::uint16_t member) const override {
      const auto &state=candidate.states_[Stream(context.stream)];const ClientShadowRecordData *record=nullptr,*other=nullptr;
      if(member!=kNullIndex){const auto *identity=FindStateMember(state->Members()->Directory(),member);if(!identity)Fail(StateReadiness::InvalidReference,"shadow_frame_member",member);
        record=state->data_->Find(identity->identity);const auto &paired=candidate.states_[1-Stream(context.stream)];other=paired?paired->data_->Find(identity->identity):nullptr;}
      body.emplace(state->Resources(),other,record);return &*body;
    }
    bool FlatRecordHandled(const StateRecordPayload &record,const PresentationBatchContext &context) const noexcept override {
      const auto &state=candidate.states_[unsigned(context.stream)];
      if(!state)return false;
      const auto *entry=FindStateMember(state->Members()->Directory(),record.member);
      if(!entry)return false;
      const auto root=state->Find(entry->identity);
      return state->FlatFault(entry->identity)||(root&&root->Flat());
    }
    const ClientStateCandidate &candidate;
    mutable std::optional<ShadowBodyContext> body;
  };
  // 各流在真实成功 base 上合并。一次推进至少完成一项工作，绝对期限由原等待位持有。
  std::optional<ClientShadowBuildResult> BuildStep(StateLane lane,std::span<const std::byte> bytes,ClientStateTables &tables,
      std::shared_ptr<ClientShadowBuildWork> &work,std::chrono::steady_clock::time_point stop,BodyCodecLimits limits) {
    if(tables.Memory()!=memory)throw ClientTableError(ClientTableFailure::Internal,"shadow_peer_memory_mismatch");
    if(lane!=StateLane::World && lane!=StateLane::Body && lane!=StateLane::Baseline)Fail(StateReadiness::InvalidEncoding,"shadow_frame_lane");
    limits.reliable=lane==StateLane::Baseline;
    limits.bytes=std::uint32_t((std::min)(std::uint64_t(limits.bytes),StateMessageBodyLimit(lane)));
    if(bytes.empty()||bytes.size()>limits.bytes)Fail(StateReadiness::InvalidEncoding,"shadow_frame_message_size");
    ClientStateWorkspace workspace;
    if(!work){
      Collect();
      auto next=std::allocate_shared<ClientShadowBuildWork>(ClientStateAllocator<ClientShadowBuildWork>(memory),memory,bytes.size(),limits.field_visits);
      next->packet->bytes.assign(bytes.begin(),bytes.end());auto frame_scratch=next->scratch.Frame();
      next->frame=InspectStateFrame(route.scope,next->packet->bytes,received_events,frame_scratch,limits);
      if(StateFrameLane(next->frame)!=lane)Fail(StateReadiness::InvalidEncoding,"shadow_lane_context_mismatch");
      const auto target=DescribeStateTarget(route,next->frame);
      next->decision=Envelope(target);
      if(next->decision.action!=StateAcceptAction::Decode)return ClientShadowBuildResult{next->decision,{}};
      for(unsigned i=0;i<2;++i)next->decode_inputs[i]=DecodeBase(i,received_versions[i]);
      next->candidate=std::allocate_shared<ClientStateCandidate>(ClientStateAllocator<ClientStateCandidate>(memory),memory);
      for(unsigned i=0;i<next->frame.section_count;++i){
        const auto &section=next->frame.sections[i];const auto stream=Stream(section.context.stream);
        auto resources=tables.Resources(section.context.resource_epoch);
        if(!resources)return ClientShadowBuildResult{{StateAcceptAction::Wait,StateTargetStreams(target),StateResyncReason::Table},{}};
        ClientMemberSnapshotRef base_members;
        if(section.context.base_version){
          const auto base=DecodeBase(stream,section.context.base_version);
          if(!base)Fail(StateReadiness::BaseLost,"shadow_decode_base_not_retained");
          next->decode_inputs[stream]=base;base_members=base->Members();
        }
        auto members=i==0?tables.FrameMembers(route,section.context.member_epoch,base_members,next->frame.member_update):
          next->roots[0]->Members();
        if(members->Route()!=route||resources->Directory().session!=route.scope.session||resources->Directory().run!=route.scope.run)
          Fail(StateReadiness::Lifecycle,"shadow_table_route");
        auto value=std::allocate_shared<ClientShadowState>(ClientStateAllocator<ClientShadowState>(memory),memory);
        value->data_->context=section.context;value->data_->members=std::move(members);value->data_->resources=std::move(resources);
        value->data_->records.reserve(value->Members()->Directory().entries.size());
        next->roots[stream]=value;next->candidate->states_[stream]=value;
        auto &records=next->incoming[stream];records.assign(section.records.begin(),section.records.end());
        std::sort(records.begin(),records.end(),[](const auto &a,const auto &b){return a.member<b.member;});
        for(std::size_t n=0;n<records.size();++n){
          const auto &record=records[n];const auto *entry=FindStateMember(value->Members()->Directory(),record.member);
          if(!entry||!Policy(entry->identity).masks[stream]||(n&&records[n-1].member==record.member))
            Fail(StateReadiness::InvalidReference,"shadow_duplicate_or_unknown_record",record.member,record.layout);
        }
      }
      work=std::move(next);
      for(unsigned i=0;i<2;++i)work->FindInvalidated(i);
    }
    auto &w=*work;
    std::array<const ClientShadowStateData *,2> bases{};
    for(unsigned i=0;i<2;++i)if(w.roots[i]&&w.roots[i]->Context().base_version){
      bases[i]=w.decode_inputs[i]?w.decode_inputs[i]->data_.get():nullptr;
      const auto decoded_base=DecodeBase(i,w.roots[i]->Context().base_version);
      if(!bases[i]||!decoded_base||bases[i]!=decoded_base->data_.get()||bases[i]->context.target_version!=w.roots[i]->Context().base_version)
        Fail(StateReadiness::BaseLost,"shadow_decode_base_not_retained");
    }
    const auto *world_current=Installed(0);
    // 前代解码才有两个不同的同流根。比较根只参与等价复用，重建与引用校验仍从线上 base 开始。
    const bool world_rebased=lane==StateLane::World&&world_current&&bases[0]!=world_current;
    if(!w.merged){
#ifdef ISAACCS_E2E
      // 前代重建只计成员合并及索引；归一在同一工作片内单计，避免重复归因。
      struct RebuildTiming {
        ClientApplyMetrics &metrics;
        bool active;
        ClientApplyDetailTimer::Clock::time_point begin;
        ClientApplyDetailTimer::Clock::duration normalization{};
        ~RebuildTiming(){
          if(!active)return;
          const auto elapsed=ClientApplyDetailTimer::Clock::now()-begin;
          metrics.Mark(1,ClientApplyStage::DecodeRingBase,ClientApplyDetailTimer::Microseconds(elapsed-normalization));
          if(normalization.count())metrics.Mark(1,ClientApplyStage::DecodeNormalize,ClientApplyDetailTimer::Microseconds(normalization));
        }
      } measured{memory->ApplyMetrics(),world_rebased,world_rebased?ClientApplyDetailTimer::Clock::now():ClientApplyDetailTimer::Clock::time_point{}};
#endif
      const auto entries=(w.roots[0]?w.roots[0]:w.roots[1])->Members()->Directory().entries;
      while(w.member<entries.size()){
        const auto &entry=entries[w.member];const auto &policy=Policy(entry.identity);
        std::array<std::shared_ptr<const ClientShadowRecordData>,2> made{};
        for(unsigned n=0;n<2;++n){
          const auto stream=(policy.first+n)%2;
          if(!w.roots[stream]||!policy.masks[stream])continue;
          const auto base=bases[stream]?bases[stream]->FindRef(entry.identity):nullptr;
          const auto *retained=w.decode_inputs[stream]?w.decode_inputs[stream]->data_->Find(entry.identity):nullptr;
          const auto current=stream==0&&world_rebased?world_current->FindRef(entry.identity):nullptr;
          const auto &opposite=w.decode_inputs[1-stream];
          const auto *other=made[1-stream]?made[1-stream].get():
            !w.roots[1-stream]&&opposite?opposite->data_->Find(entry.identity):nullptr;
          const auto &records=w.incoming[stream];
          const auto at=std::lower_bound(records.begin(),records.end(),entry.index,[](const auto &r,auto i){return r.member<i;});
          const auto *record=at!=records.end()&&at->member==entry.index?&*at:nullptr;
          const auto &section=w.frame.sections[w.frame.section_count==2?stream:0];
          const bool retained_without_image=section.Retains(entry.index)&&!base&&!record;
          if(const auto *flat=FlatRecordLayout(section.context.layout_version,entry.identity.layout)){
            const auto &target=*w.roots[stream];
            const auto *tainted=bases[stream]?w.decode_inputs[stream]->FlatFault(entry.identity):nullptr;
            if(retained_without_image&&!tainted){w.dependencies_changed[stream]=true;continue;}
            try{
              if(tainted&&(!record||record->set_mode!=FlatSetMode::All))
                Fail(StateReadiness::InvalidEncoding,tainted->reason.data(),entry.index,tainted->layout,tainted->field);
              if(!record){
                if(!base||!base->flat)Fail(StateReadiness::InvalidEncoding,"flat_member_without_image",entry.index,flat->id);
                if(!section.Retains(entry.index)&&!ClientFlatLinksCurrent(*base->flat,target.Members(),target.Resources()))
                  Fail(StateReadiness::InvalidReference,"flat_inherited_link_rebound",entry.index,flat->id);
                if(base->member==entry.index)made[stream]=base;
              }
              if(!made[stream]){
                auto next=std::allocate_shared<ClientShadowRecordData>(ClientStateAllocator<ClientShadowRecordData>(memory),memory);
                next->member=entry.index;next->identity=entry.identity;
                next->member_epoch=target.Context().member_epoch;next->resource_epoch=target.Context().resource_epoch;
                next->flat=record?DecodeClientFlat(*flat,*record,section,base?base->flat:ClientFlatImageRef{},
                  target.Members(),target.Resources(),memory,w.remaining):base->flat;
                if(base&&base->member==entry.index&&base->flat==next->flat)made[stream]=base;
                else made[stream]=std::move(next);
              }
              // 联合基线的 World／Body 服装必须指向同一目录。坏 Body 仍按成员隔离。
              if(stream==1&&flat->id==197&&w.roots[0]&&
                 (!other||!other->flat||!ClientFlatPlayerTopologyMatches(*other->flat,*made[stream]->flat)))
                Fail(StateReadiness::MissingClosure,"flat_player_costume_topology",entry.index,flat->id);
            }catch(const ClientShadowError &error){
              if(error.Readiness()==StateReadiness::Budget)throw;
              const auto latest=DecodeBase(stream,received_versions[stream]);
              const auto *old_fault=latest?latest->FlatFault(entry.identity):nullptr;
              auto &fault=w.roots[stream]->data_->flat_faults.emplace_back();
              fault.identity=entry.identity;fault.member=entry.index;
              fault.layout=error.LayoutId()?error.LayoutId():flat->id;fault.field=error.FieldId();
              fault.count=1;
              if(old_fault&&std::string_view(old_fault->reason.data())==error.what())
                fault.count=old_fault->count+unsigned(old_fault->count<UINT32_MAX);
              std::snprintf(fault.reason.data(),fault.reason.size(),"%s",error.what());
              // 允许原生继续显示前值；损坏基线带显式标记，下一份增量不能假装补齐缺失字段。
              made[stream]=base&&base->member==entry.index?base:nullptr;
            }
            if(made[stream])w.roots[stream]->data_->records.push_back(made[stream]);
            if(base!=made[stream])w.dependencies_changed[stream]=true;
            continue;
          }
          if(retained_without_image){w.dependencies_changed[stream]=true;continue;}
          const auto invalidated=w.InvalidatedBlocks(stream,entry.identity);
          if(w.frame.section_count==1&&base&&(invalidated&~(record?record->mask:0u)))
            Fail(StateReadiness::InvalidReference,"shadow_inherited_consumer_missing",entry.index,entry.identity.layout);
          made[stream]=MakeRecord(entry,policy,*w.roots[stream]->data_,record,base,retained,current,other,w.scratch,limits,w.remaining,invalidated
#ifdef ISAACCS_E2E
            ,measured.normalization
#endif
          );
          // 改绑后的前代可以借用含新成员的目录，但新记录的反向边尚未纳入前代索引。
          if(!base)w.dependencies_changed[stream]=true;
          else if(!w.dependencies_changed[stream]&&base!=made[stream]){
            for(unsigned block=0;block<32;++block){
              const auto &a=base->blocks[block],&b=made[stream]->blocks[block];
              if(a!=b&&(!a||!b||!std::equal(a->links.begin(),a->links.end(),b->links.begin(),b->links.end(),SameLink))){
                w.dependencies_changed[stream]=true;break;
              }
            }
          }
          w.roots[stream]->data_->records.push_back(made[stream]);
        }
        ++w.member;
        if(std::chrono::steady_clock::now()>=stop)return {};
      }
      for(unsigned i=0;i<2;++i)if(w.roots[i]){
        w.roots[i]->data_->Index(bases[i]);
        w.roots[i]->data_->IndexDependencies(memory,bases[i],w.dependencies_changed[i]);
      }
      w.merged=true;
    }
    WireContext context(*w.candidate);
    while(!w.frame.decoded){
      auto scratch=w.scratch.Frame();
      try{DecodeStateFrameStep(w.frame,context,scratch,limits,w.wire);}
      catch(const BodyScratchError &error){w.scratch.Grow(error,limits);}
      if(!w.frame.decoded&&std::chrono::steady_clock::now()>=stop)return {};
    }
    // 另一流在工作片间可以推进；在封存前以当前配对状态核资格。
    if(w.frame.section_count==2)Projections(*w.candidate,w.scratch);
    w.packet->Freeze(route,w.frame);w.candidate->packet_=w.packet;
    packets.reserve(packets.size()+1);states.reserve(states.size()+w.frame.section_count);candidates.reserve(candidates.size()+1);
    packets.push_back(w.packet);for(const auto &root:w.roots)if(root)states.push_back(root);
    candidates.push_back(w.candidate);
    // 完整重建及配对检查通过才计一次，跨工作片续做和后续主线程回执各走自己的计数。
    if(world_rebased)memory->ApplyMetrics().Volume(1,ClientApplyVolume::WorldRingBaseDecoded);
    return ClientShadowBuildResult{w.decision,w.candidate};
  }
  struct ValidationContext final : StateCandidateContext {
    ValidationContext(const ClientShadowStorage &owner,const ClientStateCandidateRef &value,const ClientShadowValidation &validator)
        :storage(owner),candidate(value),validation(validator) {}
    StateReadiness Inspect(const StateTarget &target) const override {
      if(!storage.Known(candidate) || target.route!=candidate->Target().route ||
          StateTargetAck(target)!=StateTargetAck(candidate->Target()) ||
          (target.encoded.data()!=candidate->Target().encoded.data()&&!BytesEqual(target.encoded,candidate->Target().encoded)))
        return StateReadiness::InvalidEncoding;
      if(storage.Occupied(StateApplyOccupancy(target)))return StateReadiness::SourcePending;
      if(target.count==2)
      try {
        // 候选等待期间另一流可以前进；实际配对根变化才重新检查。
        ClientStateWorkspace workspace;ShadowScratch scratch(storage.memory,0);storage.Projections(*candidate,scratch);
      } catch(const ClientShadowError &error) {return error.Readiness();}
        catch(const ClientTableError &error) {if(error.Failure()==ClientTableFailure::Budget || error.Failure()==ClientTableFailure::WorkspaceBusy)return StateReadiness::Budget;throw;}
        catch(const Gap &) {return StateReadiness::MissingClosure;}
      return validation.Inspect(*candidate);
    }
    const ClientShadowStorage &storage;
    const ClientStateCandidateRef &candidate;
    const ClientShadowValidation &validation;
  };
  void Recover(std::uint8_t streams,StateResyncReason reason,bool structural) {
    if(!streams || streams>3)Fail(StateReadiness::InvalidEncoding,"shadow_recovery_streams");
    auto affected=std::uint8_t(structural?3:streams);
    for(const auto &slot:work)if(slot.candidate&&(StateTargetStreams(slot.candidate->Target())&affected))
      affected|=StateTargetStreams(slot.candidate->Target());
    acceptance.RequireRecovery(affected,reason,structural);
    ClearReceived(affected);
    const auto unavailable=acceptance.UnusableStreams();
    for(unsigned stream=0;stream<2;++stream)if(unavailable&(1u<<stream)){
      installed[stream].reset();
    }
    for(auto &slot:work)if(slot.candidate&&(StateTargetStreams(slot.candidate->Target())&affected)){
      slot.revoked=true;
    }
  }
};

ClientStateShadow::ClientStateShadow(std::shared_ptr<ClientStateMemory> memory,std::thread::id owner,std::span<const ClientShadowLayoutBinding> bindings)
    :memory_(std::move(memory)) {
  if(!memory_)throw ClientTableError(ClientTableFailure::Internal,"shadow_memory_required");
  storage_=std::allocate_shared<ClientShadowStorage>(ClientStateAllocator<ClientShadowStorage>(memory_),memory_,owner,bindings);
}
ClientStateShadow::~ClientStateShadow()=default;
void ClientStateShadow::Open(const StateRoute &route,StateAgreement agreement) {
  auto &s=*storage_;s.Thread();s.Collect();
  if(!s.Drained())throw ClientTableError(ClientTableFailure::Busy,"shadow_previous_route_not_drained");
  s.acceptance.Open(route,agreement);s.route=route;s.active=true;s.work={};
  s.received_versions={};s.received_events=0;
}
void ClientStateShadow::Close() {
  auto &s=*storage_;s.Thread();s.acceptance.Retire();s.active=false;
  for(auto &slot:s.work)if(slot.candidate)slot.revoked=true;
  s.installed={};s.ClearReceived(3);s.Collect();
}
void ClientStateShadow::Collect() {storage_->Collect();}
void ClientStateShadow::SampleBlockMemory(std::span<const ClientStateCandidateRef> waiting,
    std::span<const std::shared_ptr<ClientShadowBuildWork>> decoding) noexcept {
  auto &s=*storage_;
  try{
    s.Thread();ClientStateWorkspace workspace;
    using Entry=std::pair<const ClientShadowBytes *const,unsigned>;
    std::map<const ClientShadowBytes *,unsigned,std::less<const ClientShadowBytes *>,ClientStateAllocator<Entry>>
      owners{ClientStateAllocator<Entry>(memory_,memory::Pool::Workspace)};
    const auto add=[&](const ClientShadowStateRef &state,unsigned group){
      if(!state)return;
      for(const auto &record:state->data_->records)for(const auto &block:record->blocks)if(block&&block->content)
        owners.try_emplace(block->content.get(),group);
    };
    for(const auto &state:s.installed)add(state,0);
    const auto candidate=[&](const auto &work){if(work)for(unsigned i=0;i<2;++i)add(work->State(BodyStream(i)),1);};
    for(const auto &work:s.work)candidate(work.candidate);
    for(const auto &work:waiting)candidate(work);
    for(const auto &work:decoding)if(work)for(const auto &root:work->roots)add(root,1);
    for(const auto &state:s.states)add(state,2);
    std::array<std::uint64_t,3> bytes{};
    for(const auto &[owner,group]:owners)bytes[group]+=owner->bytes.capacity();
    memory_->ApplyMetrics().SetBlockSample(bytes);
  }catch(...){}
}
bool ClientStateShadow::Drained() const {storage_->Thread();storage_->Collect();return storage_->Drained();}
ClientShadowBuildResult ClientStateShadow::Build(const StateRoute &route,StateLane lane,std::span<const std::byte> bytes,ClientStateTables &tables,BodyCodecLimits limits) {
  std::shared_ptr<ClientShadowBuildWork> work;
  return *BuildStep(route,lane,bytes,tables,work,std::chrono::steady_clock::time_point::max(),limits);
}
std::optional<ClientShadowBuildResult> ClientStateShadow::BuildStep(const StateRoute &route,StateLane lane,std::span<const std::byte> bytes,
    ClientStateTables &tables,std::shared_ptr<ClientShadowBuildWork> &work,std::chrono::steady_clock::time_point stop,BodyCodecLimits limits) {
  storage_->Route(route);
  try {
    auto result=storage_->BuildStep(lane,bytes,tables,work,stop,limits);
    if(result)work.reset();
    return result;
  }catch(const BodyDependencyError &error){
    work.reset();throw ClientShadowError(lane==StateLane::Baseline?StateReadiness::MissingClosure:StateReadiness::SourcePending,error.what(),kNullIndex,error.LayoutId(),error.FieldId());
  }catch(const Gap &error){work.reset();throw ClientShadowError(StateReadiness::InvalidEncoding,error.what());}
   catch(...){work.reset();throw;}
}
StateAcceptDecision ClientStateShadow::Assess(const ClientStateCandidateRef &candidate,const ClientShadowValidation &validation) const {
  const auto &s=*storage_;s.Thread();if(!s.Known(candidate))Fail(StateReadiness::InvalidReference,"shadow_foreign_candidate");
  ClientShadowStorage::ValidationContext context(s,candidate,validation);return s.acceptance.Assess(candidate->Target(),context);
}
StateAcceptDecision ClientStateShadow::AssessEnvelope(const StateTarget &target,const ClientStateTables &) const {
  const auto &s=*storage_;s.Thread();return s.Envelope(target);
}
StateApplyTicket ClientStateShadow::Begin(const ClientStateCandidateRef &candidate,const ClientShadowValidation &validation) {
  auto &s=*storage_;s.Thread();
  if(!s.Known(candidate))Fail(StateReadiness::InvalidReference,"shadow_foreign_candidate");
  const auto occupancy=StateApplyOccupancy(candidate->Target());
  if(s.Occupied(occupancy))throw ClientTableError(ClientTableFailure::Busy,"shadow_native_still_busy");
  ClientShadowStorage::ValidationContext context(s,candidate,validation);
  const auto ticket=s.acceptance.Begin(candidate->Target(),context);
  s.work[StateApplyChannel(candidate->Target())]={candidate,ticket};return ticket;
}
bool ClientStateShadow::CompleteApplication(StateApplyTicket ticket,bool applied,bool drained,ClientStateEvents &events) {
  auto &s=*storage_;s.Thread();auto *slot=s.FindWork(ticket);
  if(!slot||slot->awaiting_drain)return false;
  if(drained){*slot={};return false;}
  const auto &candidate=*slot->candidate;
  const auto &target=candidate.Target();
  if(applied&&s.active&&!slot->revoked&&ticket.route==s.route&&s.acceptance.MarkApplied(target)){
    const auto world=candidate.State(BodyStream::World);
    if(!world||events.Promote(world->Context().event_watermark)){
      for(unsigned i=0;i<2;++i)if(candidate.states_[i])s.installed[i]=candidate.states_[i];
      *slot={};
      return true;
    }
  }
  // 场景不可继续使用时，保留票据直到 Manager 确认原生对象已退出。
  if(s.active)s.Recover(3,StateResyncReason::Apply,true);
  slot->awaiting_drain=true;slot->revoked=true;
  return false;
}
bool ClientStateShadow::CompleteDrain(StateApplyTicket ticket) {
  auto &s=*storage_;s.Thread();auto *slot=s.FindWork(ticket);if(!slot||!slot->awaiting_drain)return false;
  *slot={};return true;
}
void ClientStateShadow::RequireRecovery(std::uint8_t streams,StateResyncReason reason,bool structural) {auto &s=*storage_;s.Thread();s.Recover(streams,reason,structural);}
ClientShadowStateRef ClientStateShadow::Installed(BodyStream stream) const {
  const auto &s=*storage_;s.Thread();const auto index=Stream(stream);return s.active && s.Installed(index)?s.installed[index]:nullptr;
}
std::array<std::uint32_t,2> ClientStateShadow::HistoricalVersions() const {storage_->Thread();return storage_->acceptance.HistoricalVersions();}
ClientStateCandidateRef ClientStateShadow::ForWorldApplication(const ClientStateCandidateRef &candidate) {
  auto &s=*storage_;s.Thread();
  if(!candidate->states_[0])return candidate;
  const auto *previous=s.Installed(0);
  const auto &source=*candidate->states_[0]->data_;
  if(source.flat_faults.empty()&&previous&&previous->context.member_epoch==candidate->states_[0]->Context().member_epoch){
    const auto &fresh=candidate->states_[0]->data_->records;
    if(std::equal(previous->records.begin(),previous->records.end(),fresh.begin(),fresh.end(),
        [](const auto &a,const auto &b){return a->identity==b->identity;}))return candidate;
  }
  // 接收时可能还有更早的主线程票据。成员差分只相对真正应用后的目录生成。
  auto result=std::allocate_shared<ClientStateCandidate>(ClientStateAllocator<ClientStateCandidate>(s.memory),s.memory);
  result->packet_=candidate->packet_;result->states_=candidate->states_;
  if(!source.flat_faults.empty()){
    // 只在应用视图沿用真正装过的实例。坏记录从未安装时也不会因收到更新而触发创建。
    auto view=std::allocate_shared<ClientShadowState>(ClientStateAllocator<ClientShadowState>(s.memory),s.memory);
    auto &out=*view->data_;
    out.context=source.context;out.members=source.members;out.resources=source.resources;
    out.flat_faults=source.flat_faults;out.records=source.records;
    for(const auto &fault:out.flat_faults){
      auto at=std::lower_bound(out.records.begin(),out.records.end(),fault.member,[](const auto &r,auto i){return r->member<i;});
      const auto old=previous&&source.context.base_version?previous->FindRef(fault.identity):nullptr;
      const bool replace=at!=out.records.end()&&(*at)->member==fault.member;
      if(old&&old->member==fault.member){if(replace)*at=old;else out.records.insert(at,old);}
      else if(replace)out.records.erase(at);
    }
    out.Index(nullptr);out.IndexDependencies(s.memory,nullptr,true);
    result->states_[0]=view;s.states.push_back(view);
  }
  s.Changes(*result);s.candidates.push_back(result);
  return result;
}

ClientStateCandidateRef ClientStateShadow::SelectKnownBodyObjects(const ClientStateCandidateRef &candidate) {
  auto &s=*storage_;s.Thread();
  const auto &source=candidate->states_[1];
  const auto &world=s.installed[0];
  if(!source||!world||candidate->states_[0])return candidate;
  const auto &records=source->data_->records;
  const auto entries=world->Members()->Directory();
  const auto known=[&](const auto &record){
    const auto root=world->Find(record->identity);
    if(!root)return false;
    if(record->identity.layout==host_field::EntityPlayer::kLayout){
      if(record->flat){
        if(!root->Flat()||!ClientFlatPlayerTopologyMatches(*root->Flat(),*record->flat))return false;
      }else{
        const auto field=host_field::EntityPlayer::render_topology_revision;
        const auto pose=std::find_if(record->fields.begin(),record->fields.end(),[&](const auto &value){return value.field==field;});
        const auto *layout=root->Field(host_field::EntityPlayer::costume_topology_revision);
        if(pose==record->fields.end()||!layout||!pose->present||!layout->present||!BytesEqual(pose->bytes,layout->bytes))return false;
      }
    }
    if(record->flat)for(const auto &link:record->flat->links)if(!link.is_resource){
      const auto *entry=FindStateMember(entries,link.index);
      if((!entry||entry->identity!=link.member)&&
         std::none_of(entries.entries.begin(),entries.entries.end(),[&](const auto &m){return m.identity==link.member;}))return false;
    }
    for(const auto &block:record->blocks)if(block)for(const auto &link:block->links)if(!link.resource){
      const auto *entry=FindStateMember(entries,link.index);
      if((!entry||entry->identity!=link.member)&&
          std::none_of(entries.entries.begin(),entries.entries.end(),[&](const auto &m){return m.identity==link.member;}))
        return false;
    }
    return true;
  };
  if(std::all_of(records.begin(),records.end(),known))return candidate;
  // 网络基线仍含完整 Body；应用侧只发布实际处理的根，下一帧按该根上次值补写。
  auto view=std::allocate_shared<ClientShadowState>(ClientStateAllocator<ClientShadowState>(s.memory),s.memory);
  auto &out=*view->data_;
  out.context=source->Context();out.members=source->Members();out.resources=source->Resources();
  out.flat_faults=source->data_->flat_faults;
  out.records.reserve(records.size());
  for(const auto &record:records)if(known(record))out.records.push_back(record);
  out.Index(nullptr);out.IndexDependencies(s.memory,nullptr,true);
  auto result=std::allocate_shared<ClientStateCandidate>(ClientStateAllocator<ClientStateCandidate>(s.memory),s.memory);
  result->packet_=candidate->packet_;result->states_[1]=view;
  s.states.push_back(view);s.candidates.push_back(result);
  return result;
}

ClientStateCandidateRef ClientStateShadow::DeferUnreadyObjects(const ClientStateCandidateRef &candidate,const ClientStateTables &tables) {
  auto &s=*storage_;s.Thread();
  const auto &target=candidate->Target();
  if(target.count!=1||!target.sections[0].base_version)return {};
  const auto stream=Stream(target.sections[0].stream);
  const auto &previous=s.installed[stream];
  const auto &proposed=candidate->states_[stream];
  if(!previous||!proposed)return {};
  // 仅派生应用视图。接收环保留真正的 Host 值，下一帧仍能以已 ACK 的目标解码。
  auto view=std::allocate_shared<ClientShadowState>(ClientStateAllocator<ClientShadowState>(s.memory),s.memory);
  auto &out=*view->data_;
  const auto &in=*proposed->data_;
  out.context=in.context;out.members=in.members;out.resources=in.resources;
  out.flat_faults=in.flat_faults;
  out.records=in.records;
  const auto directory=in.members->Directory();
  const auto extent=directory.entries.empty()?0u:unsigned(directory.entries.back().index)+1;
  ClientStateVector<std::uint8_t> missing(extent,0,ClientStateAllocator<std::uint8_t>(s.memory,memory::Pool::Workspace));
  bool deferred=false;
  using RecordRef=std::shared_ptr<const ClientShadowRecordData>;
  const auto retain=[&](RecordRef &record){
    const auto &id=record->identity;
    // 房间及 Game 转场字段属于整份 World。资源未齐时等待完整目标，避免新帧号配上旧阶段。
    if(id.space!=1&&id.space!=2)return false;
    const auto last=previous->data_->FindRef(id);
    if(!last){
      // 对尚未创建的实体／格子不生成 Install 指令。已有逻辑实例换形态留给结构协调。
      if(std::any_of(previous->data_->records.begin(),previous->data_->records.end(),
          [&](const auto &before){return MemberKey(before->identity)==MemberKey(id);}))return false;
      if(!stream)missing[record->member]=1;
      record.reset();deferred=true;return true;
    }
    if(last->member!=record->member)return false;
    if(last->flat&&!ClientFlatLinksCurrent(*last->flat,out.members,out.resources))return false;
    for(const auto &block:last->blocks)if(block)for(const auto &link:block->links)if(!link.resource){
      const auto *member=FindStateMember(directory,link.index);
      if(!member||member->identity!=link.member)return false;
    }
#ifdef ISAACCS_E2E
    out.deferred_resources.push_back(record->member);
#endif
    record=last;deferred=true;return true;
  };
  for(auto &record:out.records){
    bool pending=false;
    if(record->flat){
      if(proposed->FlatFault(record->identity))continue;
      for(const auto &link:record->flat->links)if(link.is_resource){
        const auto readiness=tables.ResourceState(in.context.resource_epoch,link.resource);
        pending|=readiness==ClientResourceState::Unprepared||readiness==ClientResourceState::Pending;
      }
    }
    for(const auto &block:record->blocks)if(block)for(const auto &link:block->links)if(link.resource){
      const auto readiness=tables.ResourceState(in.context.resource_epoch,link.entry.key);
      pending|=readiness==ClientResourceState::Unprepared||readiness==ClientResourceState::Pending;
    }
    if(pending&&!retain(record))return {};
  }
  if(!deferred)return {};
  // 依赖尚未创建目标的记录按相同规则沿用，实际挂引用由第 3 步承接。
  const auto needs_missing=[&](const RecordRef &record){
    if(record->flat)for(const auto &link:record->flat->links)
      if(!link.is_resource&&link.index<missing.size()&&missing[link.index])return true;
    for(const auto &block:record->blocks)if(block)for(const auto &link:block->links)
      if(!link.resource&&link.index<missing.size()&&missing[link.index])return true;
    return false;
  };
  bool removed=false;
  do {
    removed=false;
    for(auto &record:out.records)if(record&&needs_missing(record)){
      if(!retain(record)||(record&&needs_missing(record)))return {};
      removed|=!record;
    }
  }while(removed);
  std::erase_if(out.records,[](const auto &record){return !record;});
  out.Index(&in);out.IndexDependencies(s.memory,&in,true);
  auto result=std::allocate_shared<ClientStateCandidate>(ClientStateAllocator<ClientStateCandidate>(s.memory),s.memory);
  result->packet_=candidate->packet_;result->states_=candidate->states_;
  result->states_[stream]=view;
  if(!stream)s.Changes(*result);
  s.states.push_back(view);s.candidates.push_back(result);
  return result;
}

StateAppliedAck ClientStateShadow::RetainReceived(const ClientStateCandidateRef &candidate) {
  auto &s=*storage_;s.Thread();
  if(!s.Known(candidate)||!candidate->Target().decoded)Fail(StateReadiness::InvalidEncoding,"shadow_receive_unverified");
  const auto ack=StateTargetAck(candidate->Target());
  if(ack.count==2)s.ClearReceived(3);
  for(unsigned i=0;i<ack.count;++i){
    const auto &entry=ack.entries[i];const auto stream=Stream(entry.stream);
    auto &ring=s.received[stream];auto &next=s.next_received[stream];
    ring[next]=candidate->State(entry.stream);next=(next+1)%ring.size();
    s.received_versions[stream]=entry.target;s.received_acks[stream]=ack;
    if(!stream)s.received_events=entry.event_watermark;
  }
  return ack;
}
std::array<std::uint32_t,2> ClientStateShadow::ReceivedVersions() const {storage_->Thread();return storage_->received_versions;}
std::uint64_t ClientStateShadow::ReceivedEvents() const {storage_->Thread();return storage_->received_events;}
std::optional<StateResync> ClientStateShadow::RecoveryRequest() const {
  storage_->Thread();auto request=storage_->acceptance.RecoveryRequest();
  if(request){request->installed_world=storage_->received_versions[0];request->installed_body=storage_->received_versions[1];}
  return request;
}
std::array<std::optional<StateAppliedAck>,2> ClientStateShadow::Reacknowledgements() const {
  storage_->Thread();auto out=storage_->received_acks;
  if(out[0]&&out[1]&&*out[0]==*out[1])out[1].reset();
  return out;
}
} // namespace cs::state
