#include "state/client_shadow.h"
#include <algorithm>
#include <cstring>
#include <tuple>

namespace cs::state {
namespace {
#include "state_protocol.inc"
[[noreturn]] void Fail(StateReadiness readiness,const char *reason,std::uint16_t member=kNullIndex,std::uint16_t layout=0) {
  throw ClientShadowError(readiness,reason,member,layout);
}
unsigned Stream(BodyStream stream) {
  if(unsigned(stream)>1)Fail(StateReadiness::InvalidEncoding,"shadow_invalid_stream");return unsigned(stream);
}
auto IdentityKey(const PresentationMember &member) {return std::tuple(member.space,member.id,member.use,member.shape,member.layout);}
auto MemberKey(const PresentationMember &member) {return std::pair(member.space,member.id);}
bool BytesEqual(std::span<const std::byte> a,std::span<const std::byte> b) {return std::equal(a.begin(),a.end(),b.begin(),b.end());}
std::uint32_t Mask(std::uint16_t id,BodyStream stream) {
  const auto *layout=FindLayout(id);if(!layout || layout->protocol || layout->block_count>32)Fail(StateReadiness::InvalidReference,"shadow_root_layout",kNullIndex,id);
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
struct ClientShadowBlockData final {
  explicit ClientShadowBlockData(const std::shared_ptr<ClientStateMemory> &memory):links(ClientStateAllocator<ClientShadowLink>(memory)) {}
  ClientShadowBlockView view;
  std::shared_ptr<const ClientShadowBytes> content;
  ClientStateVector<ClientShadowLink> links;
  ClientMemberSnapshotRef members;
  ClientResourceSnapshotRef resources;
};
// 一条序列记录固定完整身份、实际块及数字视图，条件缺席与合法空值保持区别。
struct ClientShadowRecordData final {
  explicit ClientShadowRecordData(const std::shared_ptr<ClientStateMemory> &memory)
      :fields(ClientStateAllocator<BodyFieldView>(memory)),facts(ClientStateAllocator<ProjectionFact>(memory)) {}
  std::uint16_t member=0;
  PresentationMember identity;
  std::uint32_t mask=0,member_epoch=0,resource_epoch=0;
  std::array<std::shared_ptr<const ClientShadowBlockData>,32> blocks{};
  ClientStateVector<BodyFieldView> fields;
  ClientStateVector<ProjectionFact> facts;
};
// 完整目标保留准确两表与当前时钟；未变化record可共享，成员顺序只作数字索引。
struct ClientShadowStateData final {
  explicit ClientShadowStateData(const std::shared_ptr<ClientStateMemory> &memory)
      :records(ClientStateAllocator<std::shared_ptr<const ClientShadowRecordData>>(memory)),order(ClientStateAllocator<std::uint16_t>(memory)) {}
  PresentationBatchContext context;
  ClientMemberSnapshotRef members;
  ClientResourceSnapshotRef resources;
  ClientStateVector<std::shared_ptr<const ClientShadowRecordData>> records;
  ClientStateVector<std::uint16_t> order;
  const ClientShadowRecordData *Find(const PresentationMember &identity) const noexcept {
    const auto it=std::lower_bound(order.begin(),order.end(),IdentityKey(identity),[&](auto index,const auto &key){return IdentityKey(records[index]->identity)<key;});
    return it==order.end() || records[*it]->identity!=identity?nullptr:records[*it].get();
  }
  std::shared_ptr<const ClientShadowRecordData> FindRef(const PresentationMember &identity) const noexcept {
    const auto *value=Find(identity);if(!value)return {};
    const auto it=std::lower_bound(records.begin(),records.end(),value->member,[](const auto &record,auto index){return record->member<index;});
    return it==records.end()?nullptr:*it;
  }
  void Index() {
    order.resize(records.size());for(std::size_t i=0;i<order.size();++i)order[i]=std::uint16_t(i);
    std::sort(order.begin(),order.end(),[&](auto a,auto b){return IdentityKey(records[a]->identity)<IdentityKey(records[b]->identity);});
  }
};
std::uint16_t ClientShadowRecordView::Member() const noexcept {return data_->member;}
const PresentationMember &ClientShadowRecordView::Identity() const noexcept {return data_->identity;}
std::uint32_t ClientShadowRecordView::Mask() const noexcept {return data_->mask;}
std::optional<ClientShadowBlockView> ClientShadowRecordView::Block(std::uint8_t block) const noexcept {
  return block<32 && data_->blocks[block]?std::optional(data_->blocks[block]->view):std::nullopt;
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
const ClientMemberSnapshotRef &ClientShadowState::Members() const noexcept {return data_->members;}
const ClientResourceSnapshotRef &ClientShadowState::Resources() const noexcept {return data_->resources;}

// 原始规范正文与其精确帧元数据独立保留，ACK凭据不需要固定整条旧影子根。
struct ClientShadowPacket final {
  explicit ClientShadowPacket(const std::shared_ptr<ClientStateMemory> &memory)
      :bytes(ClientStateAllocator<std::byte>(memory,memory::Pool::Wire)),records(ClientStateAllocator<StateRecordPayload>(memory)),
       groups(ClientStateAllocator<ResultGroupView>(memory)),events(ClientStateAllocator<PresentationAudioEvent>(memory)),
       dependencies(ClientStateAllocator<ResultDependency>(memory)),contributions(ClientStateAllocator<ResultContribution>(memory)) {}
  ClientStateVector<std::byte> bytes;
  ClientStateVector<StateRecordPayload> records;
  ClientStateVector<ResultGroupView> groups;
  ClientStateVector<PresentationAudioEvent> events;
  ClientStateVector<ResultDependency> dependencies;
  ClientStateVector<ResultContribution> contributions;
  StateFrameView frame;
  StateTarget target;
  void Freeze(const StateRoute &route,const StateFrameView &source) {
    std::size_t nr=0,ng=0,ne=0,nd=0,nc=0;
    for(unsigned i=0;i<source.section_count;++i){const auto &s=source.sections[i];nr+=s.records.size();ng+=s.presentation.groups.size();ne+=s.presentation.events.size();
      for(const auto &g:s.presentation.groups){nd+=g.dependencies.size();nc+=g.contributions.size();}}
    records.reserve(nr);groups.reserve(ng);events.reserve(ne);dependencies.reserve(nd);contributions.reserve(nc);frame=source;
    for(unsigned i=0;i<source.section_count;++i){const auto &s=source.sections[i];const auto r=records.size(),g=groups.size(),e=events.size();
      records.insert(records.end(),s.records.begin(),s.records.end());
      for(const auto &incoming:s.presentation.groups){auto value=incoming;const auto d=dependencies.size(),c=contributions.size();
        dependencies.insert(dependencies.end(),incoming.dependencies.begin(),incoming.dependencies.end());
        contributions.insert(contributions.end(),incoming.contributions.begin(),incoming.contributions.end());
        value.dependencies=std::span<const ResultDependency>(dependencies).subspan(d,incoming.dependencies.size());
        value.contributions=std::span<const ResultContribution>(contributions).subspan(c,incoming.contributions.size());groups.push_back(value);}
      events.insert(events.end(),s.presentation.events.begin(),s.presentation.events.end());
      frame.sections[i].records=std::span<const StateRecordPayload>(records).subspan(r,s.records.size());
      frame.sections[i].presentation.groups=std::span<const ResultGroupView>(groups).subspan(g,s.presentation.groups.size());
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
  Vector<ResultGroupView> groups;
  Vector<PresentationAudioEvent> events;
  Vector<ResultDependency> dependencies;
  Vector<ResultContribution> contributions;
  Vector<ContributionTarget> targets;
  Vector<std::uint16_t> member_order,dependency_order,contribution_order;
  Vector<ProjectionFact> facts,combined;
  Vector<BodyFieldView> fields;
  Vector<std::byte> merged;
  explicit ShadowScratch(const std::shared_ptr<ClientStateMemory> &memory,std::size_t size)
      :records(ClientStateAllocator<StateRecordPayload>(memory,memory::Pool::Workspace)),groups(ClientStateAllocator<ResultGroupView>(memory,memory::Pool::Workspace)),
       events(ClientStateAllocator<PresentationAudioEvent>(memory,memory::Pool::Workspace)),dependencies(ClientStateAllocator<ResultDependency>(memory,memory::Pool::Workspace)),
       contributions(ClientStateAllocator<ResultContribution>(memory,memory::Pool::Workspace)),targets(ClientStateAllocator<ContributionTarget>(memory,memory::Pool::Workspace)),
       member_order(ClientStateAllocator<std::uint16_t>(memory,memory::Pool::Workspace)),dependency_order(ClientStateAllocator<std::uint16_t>(memory,memory::Pool::Workspace)),
       contribution_order(ClientStateAllocator<std::uint16_t>(memory,memory::Pool::Workspace)),facts(ClientStateAllocator<ProjectionFact>(memory,memory::Pool::Workspace)),
       combined(ClientStateAllocator<ProjectionFact>(memory,memory::Pool::Workspace)),fields(ClientStateAllocator<BodyFieldView>(memory,memory::Pool::Workspace)),
       merged(ClientStateAllocator<std::byte>(memory,memory::Pool::Workspace)) {
    records.resize((std::min)(std::size_t(kW_frame_records),size/kW_StateRecordHeaderBytes));member_order.resize(records.size());
    groups.resize((std::min)(std::size_t(kW_max_result_groups),size/kW_StateResultGroupHeaderBytes));
    events.resize((std::min)(std::size_t(kW_max_audio_events),size/kW_StateAudioEventHeaderBytes));
    dependencies.resize((std::min)(std::size_t(kW_max_result_dependencies),size/kW_StateDependencyEntryBytes));dependency_order.resize(dependencies.size());
    contributions.resize((std::min)(std::size_t(kW_max_result_contributions),size/kW_StateContributionEntryBytes));contribution_order.resize(contributions.size());targets.resize(contributions.size());
    fields.resize(BodySourceMaximumFields());
  }
  BodyCodecScratch Body(BodyNodeObserver *observer=nullptr) {BodyCodecScratch body;body.facts=facts;body.root_fields=fields;body.observer=observer;return body;}
  StateFrameScratch Frame() {return {records,groups,events,member_order,{dependencies,contributions,targets,dependency_order,contribution_order,Body()}};}
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
      const std::shared_ptr<ClientStateMemory> &memory):values(ClientStateAllocator<ObservedLink>(memory,memory::Pool::Workspace)),
      layout_(layout),members_(members->Directory()),resources_(resources->Directory()) {}
  void Node(const BodyNodeView &node) override {
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
  std::uint16_t layout_;
  StateMemberDirectory members_;
  ResourceDirectoryView resources_;
};
BodyView DecodeRoot(std::uint16_t layout,std::uint32_t mask,BodyStream stream,std::span<const std::byte> bytes,
    ShadowScratch &scratch,const BodyCodecContext &context,BodyCodecLimits limits,ShadowLinks *links=nullptr,bool local_visits=false) {
  for(;;){if(links)links->values.clear();auto body=scratch.Body(links);
    try {return DecodeBody(layout,mask,stream,bytes,body,&context,limits);}
    catch(const BodyScratchError &error){scratch.Grow(error,limits);}
    catch(const BodyVisitError &error){if(local_visits)throw ClientTableError(ClientTableFailure::Budget,"shadow_complete_target_visits",error.Requested(),error.Available(),memory::Pool::Workspace);throw;}}
}
bool SameLink(const ClientShadowLink &a,const ClientShadowLink &b) {
  if(a.resource!=b.resource || a.index!=b.index)return false;
  return a.resource?a.entry.key==b.entry.key && a.entry.kind==b.entry.kind && BytesEqual(a.entry.payload,b.entry.payload):a.member==b.member;
}
struct PendingContext final : StateCandidateContext {StateReadiness Inspect(const StateTarget &) const override {return StateReadiness::SourcePending;}};
} // namespace

// 所有可变提交／回收状态只有后台owner访问；三个registry保住跨线程纯值借用的最后引用。
struct ClientShadowStorage final {
  struct Binding {ClientShadowLayoutBinding input;std::array<std::uint32_t,2> masks{};std::uint32_t record_bytes=32u<<20;unsigned first=0;};
  std::shared_ptr<ClientStateMemory> memory;
  std::thread::id owner;
  StateRoute route{};
  bool active=false,revoked=false,awaiting_drain=false;
  ClientStateAcceptance acceptance;
  std::array<ClientShadowStateRef,2> installed{};
  std::array<std::shared_ptr<const ClientShadowPacket>,2> receipts{};
  ClientStateCandidateRef busy;
  StateApplyTicket ticket{};
  ClientStateVector<Binding> bindings;
  ClientStateVector<std::shared_ptr<const ClientShadowPacket>> packets;
  ClientStateVector<ClientShadowStateRef> states;
  ClientStateVector<ClientStateCandidateRef> candidates;

  ClientShadowStorage(std::shared_ptr<ClientStateMemory> account,std::thread::id thread,std::span<const ClientShadowLayoutBinding> input)
      :memory(std::move(account)),owner(thread),bindings(ClientStateAllocator<Binding>(memory)),
       packets(ClientStateAllocator<std::shared_ptr<const ClientShadowPacket>>(memory)),states(ClientStateAllocator<ClientShadowStateRef>(memory)),
       candidates(ClientStateAllocator<ClientStateCandidateRef>(memory)) {
    Thread();if(input.empty())Fail(StateReadiness::InvalidReference,"shadow_empty_layout_bindings");bindings.reserve(input.size());
    for(const auto &value:input){if(!value.space)Fail(StateReadiness::InvalidReference,"shadow_invalid_member_space");
      Binding bound;bound.input=value;bound.masks={Mask(value.layout,BodyStream::World),Mask(value.layout,BodyStream::Body)};
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
    if(active || busy)return false;
    for(const auto &value:candidates)if(value.use_count()>1)return false;
    // 未回收候选自身也持有state，先由Collect清理才能给出准确排空结果。
    return candidates.empty() && states.empty() && packets.empty();
  }
  const ClientShadowStateData *Installed(unsigned stream) const noexcept {
    return installed[stream] && !(acceptance.UnusableStreams()&(1u<<stream))?installed[stream]->data_.get():nullptr;
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

  // 只借已成功安装的同身份base；完整检查后封存新record，异常不改变任何已发布根。
  std::shared_ptr<const ClientShadowRecordData> MakeRecord(const StateMemberEntry &member,const Binding &policy,
      ClientShadowStateData &target,const StateRecordPayload *incoming,const std::shared_ptr<const ClientShadowRecordData> &base,
      const ClientShadowRecordData *other,ShadowScratch &scratch,BodyCodecLimits wire_limits,std::uint32_t &remaining) {
    const auto mask=policy.masks[Stream(target.context.stream)];
    bool same_presence=true;if(base && other)for(const auto &fact:base->facts)if(fact.kind==ProjectionFactKind::SharedPresence){
      const CodecLocation location{std::span(fact.parents).first(fact.depth),fact.layout,fact.field,fact.group};const auto value=Presence(other,location);
      if(value && *value!=(fact.value!=0)){same_presence=false;break;}}
    if(base && !incoming && same_presence && base->member==member.index && base->member_epoch==target.context.member_epoch && base->resource_epoch==target.context.resource_epoch)return base;
    auto result=std::allocate_shared<ClientShadowRecordData>(ClientStateAllocator<ClientShadowRecordData>(memory),memory);
    result->member=member.index;result->identity=member.identity;result->mask=mask;
    result->member_epoch=target.context.member_epoch;result->resource_epoch=target.context.resource_epoch;
    if(base)result->blocks=base->blocks;
    ShadowBodyContext context(target.resources,other,base.get());
    if(incoming){
      if(incoming->layout!=member.identity.layout || !incoming->mask || (incoming->mask&~mask))
        Fail(StateReadiness::InvalidEncoding,"shadow_record_layout_or_stream",member.index,member.identity.layout);
      const auto body=DecodeRoot(incoming->layout,incoming->mask,target.context.stream,incoming->bytes,scratch,context,wire_limits);
      for(unsigned i=0;i<body.block_count;++i){const auto &block=body.blocks[i];auto content=std::allocate_shared<ClientShadowBytes>(ClientStateAllocator<ClientShadowBytes>(memory),memory);
        content->bytes.assign(block.bytes.begin(),block.bytes.end());auto value=std::allocate_shared<ClientShadowBlockData>(ClientStateAllocator<ClientShadowBlockData>(memory),memory);
        value->content=std::move(content);value->view={block.block,target.context.target_version,target.context.game_frame,target.context.member_epoch,
          target.context.resource_epoch,target.context.source_revision,value->content->bytes};result->blocks[block.block]=std::move(value);}
    }
    std::uint64_t total=0;for(unsigned i=0;i<32;++i)if(mask&(std::uint32_t{1}<<i)){
      if(!result->blocks[i])Fail(StateReadiness::MissingClosure,"shadow_incomplete_new_member_or_shape",member.index,member.identity.layout);
      total+=result->blocks[i]->view.bytes.size();}
    if(total>(32u<<20))throw ClientTableError(ClientTableFailure::Budget,"shadow_complete_record_bytes",total,32u<<20,memory::Pool::Workspace);
    scratch.merged.clear();scratch.merged.reserve(std::size_t(total));
    for(unsigned i=0;i<32;++i)if(mask&(std::uint32_t{1}<<i)){const auto bytes=result->blocks[i]->view.bytes;scratch.merged.insert(scratch.merged.end(),bytes.begin(),bytes.end());}
    if(!remaining)throw ClientTableError(ClientTableFailure::Budget,"shadow_complete_target_visits",1,0,memory::Pool::Workspace);
    BodyCodecLimits complete{32u<<20,remaining,wire_limits.depth,true};ShadowLinks links(member.identity.layout,target.members,target.resources,memory);
    const auto body=DecodeRoot(member.identity.layout,mask,target.context.stream,scratch.merged,scratch,context,complete,&links,true);
    if(body.field_visits>remaining)throw ClientTableError(ClientTableFailure::Budget,"shadow_complete_target_visits");remaining-=body.field_visits;
    // 只持有真正的投影事实及根字段视图，不保存整个递归字段树或临时observer路径。
    result->facts.assign(body.facts.begin(),body.facts.end());result->fields.reserve(body.fields.size());
    for(unsigned i=0;i<body.block_count;++i){const auto &block=body.blocks[i];const auto &before=result->blocks[block.block];
      const bool changed=incoming && (incoming->mask&(std::uint32_t{1}<<block.block));
      if(!changed && !LinksEqual(*before,links.values,block.block))Fail(StateReadiness::MissingClosure,"shadow_inherited_link_rebound",member.index,member.identity.layout);
      if(!changed && (!before->members || before->members==target.members) && (!before->resources || before->resources==target.resources))continue;
      auto value=std::allocate_shared<ClientShadowBlockData>(ClientStateAllocator<ClientShadowBlockData>(memory),memory);
      value->view=before->view;value->content=before->content;
      for(const auto &link:links.values)if(link.block==block.block){value->links.push_back(link.link);if(link.link.resource)value->resources=target.resources;else value->members=target.members;}
      // 没有链接的旧块无需携带旧表；其值owner独立共享，不固定旧record／candidate。
      result->blocks[block.block]=std::move(value);
    }
    for(const auto &field:body.fields){auto value=field;if(field.present){
        const auto block=std::find_if(body.blocks.begin(),body.blocks.begin()+body.block_count,[&](const auto &entry){return entry.block==field.block;});
        if(block==body.blocks.begin()+body.block_count)Fail(StateReadiness::InvalidEncoding,"shadow_field_block");
        value.bytes=result->blocks[field.block]->view.bytes.subspan(Offset(block->bytes,field.bytes),field.bytes.size());
      }else value.bytes={};result->fields.push_back(value);}
    if(!policy.masks[1-Stream(target.context.stream)]){
      scratch.combined.assign(result->facts.begin(),result->facts.end());ValidateProjectionFacts(scratch.combined,true);}
    return result;
  }

  // 两序列仅在同一真实成员的投影上联合检查；body不等待无关NPC／地图／资源就绪。
  void Projections(const ClientStateCandidate &candidate,ShadowScratch &scratch) const {
    const bool fast=candidate.states_[1] && !candidate.states_[0] && candidate.states_[1]->Context().base_version;
    const auto missing=fast?StateReadiness::SourcePending:StateReadiness::MissingClosure;
    for(unsigned stream=0;stream<2;++stream)if(candidate.states_[stream]){
      const auto &own=*candidate.states_[stream]->data_;const auto *other=Effective(candidate,1-stream);
      for(const auto &record:own.records){if(!Policy(record->identity).masks[1-stream])continue;
        const auto *paired=other?other->Find(record->identity):nullptr;
        if(!paired)Fail(missing,"shadow_missing_paired_member",record->member,record->identity.layout);
        std::uint64_t bytes=0;for(unsigned block=0;block<32;++block){if(record->blocks[block])bytes+=record->blocks[block]->view.bytes.size();if(paired->blocks[block])bytes+=paired->blocks[block]->view.bytes.size();}
        if(bytes>Policy(record->identity).record_bytes)Fail(StateReadiness::Budget,"shadow_combined_record_byte_budget",record->member,record->identity.layout);
        scratch.combined.assign(record->facts.begin(),record->facts.end());scratch.combined.insert(scratch.combined.end(),paired->facts.begin(),paired->facts.end());
        try {ValidateProjectionFacts(scratch.combined,true);}
        catch(const Gap &){Fail(missing,"shadow_incompatible_paired_projections",record->member,record->identity.layout);}
      }
      if(other)for(const auto &record:other->records)if(Policy(record->identity).masks[stream] && !own.Find(record->identity))
        Fail(fast?StateReadiness::SourcePending:StateReadiness::Lifecycle,"shadow_unpaired_structural_change",record->member,record->identity.layout);
    }
  }
  void Changes(ClientStateCandidate &candidate) {
    if(!candidate.states_[0])return;
    const auto next=candidate.states_[0]->Members()->Directory();const auto *before=Installed(0);
    ClientStateVector<StateMemberEntry> old{ClientStateAllocator<StateMemberEntry>(memory,memory::Pool::Workspace)},fresh{ClientStateAllocator<StateMemberEntry>(memory,memory::Pool::Workspace)};
    if(before){const auto directory=before->members->Directory();old.assign(directory.entries.begin(),directory.entries.end());}
    fresh.assign(next.entries.begin(),next.entries.end());const auto less=[](const auto &a,const auto &b){return MemberKey(a.identity)<MemberKey(b.identity);};
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
    const ClientStateCandidate &candidate;
    mutable std::optional<ShadowBodyContext> body;
  };
  // 先定界处理旧base，再按精确表建立纯值候选；所有临时区在事务结束后统一归还。
  ClientShadowBuildResult Build(StateLane lane,std::span<const std::byte> bytes,ClientStateTables &tables,BodyCodecLimits limits) {
    if(tables.Memory()!=memory)throw ClientTableError(ClientTableFailure::Internal,"shadow_peer_memory_mismatch");
    if(lane!=StateLane::World && lane!=StateLane::Body && lane!=StateLane::Baseline)Fail(StateReadiness::InvalidEncoding,"shadow_frame_lane");
    limits.reliable=lane==StateLane::Baseline;limits.bytes=std::uint32_t((std::min)(std::uint64_t(limits.bytes),StateMessageBodyLimit(lane)));
    if(bytes.empty() || bytes.size()>limits.bytes)Fail(StateReadiness::InvalidEncoding,"shadow_frame_message_size");
    ClientStateWorkspace workspace;Collect();auto packet=std::allocate_shared<ClientShadowPacket>(ClientStateAllocator<ClientShadowPacket>(memory),memory);
    packet->bytes.assign(bytes.begin(),bytes.end());ShadowScratch scratch(memory,bytes.size());auto frame_scratch=scratch.Frame();
    auto frame=InspectStateFrame(route.scope,packet->bytes,acceptance.InstalledEvents(),frame_scratch,limits);
    if(StateFrameLane(frame)!=lane)Fail(StateReadiness::InvalidEncoding,"shadow_lane_context_mismatch");
    auto target=DescribeStateTarget(route,frame);PendingContext pending;const auto decision=acceptance.Assess(target,pending);
    if(decision.action!=StateAcceptAction::Decode)return {decision,{}};
    for(const auto &known:packets)if(StateTargetsConflict(known->target,target))return {{StateAcceptAction::Terminal,StateTargetStreams(target),StateResyncReason::Base},{}};
    auto candidate=std::allocate_shared<ClientStateCandidate>(ClientStateAllocator<ClientStateCandidate>(memory),memory);
    std::array<std::shared_ptr<ClientShadowState>,2> roots{};
    std::array<ClientStateVector<StateRecordPayload>,2> incoming{
      ClientStateVector<StateRecordPayload>(ClientStateAllocator<StateRecordPayload>(memory,memory::Pool::Workspace)),
      ClientStateVector<StateRecordPayload>(ClientStateAllocator<StateRecordPayload>(memory,memory::Pool::Workspace))};
    for(unsigned i=0;i<frame.section_count;++i){const auto &section=frame.sections[i];const auto stream=Stream(section.context.stream);
      auto members=tables.Members(section.context.member_epoch);auto resources=tables.Resources(section.context.resource_epoch);
      if(!members || !resources){const bool fast=frame.section_count==1 && stream==1 && section.context.base_version;
        const auto known=tables.MetadataReady();const bool lost=known && ((!members && known->member_epoch>=section.context.member_epoch) || (!resources && known->resource_epoch>=section.context.resource_epoch));
        return {{fast?StateAcceptAction::Drop:lost?StateAcceptAction::Resync:StateAcceptAction::Wait,StateTargetStreams(target),lost?StateResyncReason::Table:StateResyncReason::Dependency},{}};}
      if(members->Route()!=route || resources->Directory().session!=route.scope.session || resources->Directory().run!=route.scope.run)
        Fail(StateReadiness::Lifecycle,"shadow_table_route");
      auto value=std::allocate_shared<ClientShadowState>(ClientStateAllocator<ClientShadowState>(memory),memory);value->data_->context=section.context;
      value->data_->members=std::move(members);value->data_->resources=std::move(resources);value->data_->records.reserve(value->Members()->Directory().entries.size());
      roots[stream]=value;candidate->states_[stream]=value;incoming[stream].assign(section.records.begin(),section.records.end());
      std::sort(incoming[stream].begin(),incoming[stream].end(),[](const auto &a,const auto &b){return a.member<b.member;});
      for(std::size_t n=0;n<incoming[stream].size();++n){const auto &record=incoming[stream][n];const auto *entry=FindStateMember(value->Members()->Directory(),record.member);
        if(!entry || !Policy(entry->identity).masks[stream] || (n && incoming[stream][n-1].member==record.member))Fail(StateReadiness::InvalidReference,"shadow_duplicate_or_unknown_record",record.member,record.layout);}
    }
    std::array<const ClientShadowStateData *,2> bases{};
    for(unsigned stream=0;stream<2;++stream)if(roots[stream] && roots[stream]->Context().base_version){bases[stream]=Installed(stream);
      if(!bases[stream] || bases[stream]->context.target_version!=roots[stream]->Context().base_version)Fail(StateReadiness::BaseLost,"shadow_base_not_installed");}
    const auto entries=(roots[0]?roots[0]:roots[1])->Members()->Directory().entries;std::uint32_t remaining=limits.field_visits;
    for(const auto &entry:entries){const auto &policy=Policy(entry.identity);std::array<std::shared_ptr<const ClientShadowRecordData>,2> made{};
      for(unsigned n=0;n<2;++n){const auto stream=(policy.first+n)%2;if(!roots[stream] || !policy.masks[stream])continue;
        const auto base=bases[stream]?bases[stream]->FindRef(entry.identity):nullptr;
        const auto *opposite=Effective(*candidate,1-stream);const auto *other=made[1-stream]?made[1-stream].get():opposite?opposite->Find(entry.identity):nullptr;
        if(!other && !candidate->states_[1-stream]){const auto *old=Installed(1-stream);other=old?old->Find(entry.identity):nullptr;}
        const auto &records=incoming[stream];const auto record=std::lower_bound(records.begin(),records.end(),entry.index,[](const auto &value,auto index){return value.member<index;});
        const auto *update=record!=records.end() && record->member==entry.index?&*record:nullptr;
        made[stream]=MakeRecord(entry,policy,*roots[stream]->data_,update,base,other,scratch,limits,remaining);roots[stream]->data_->records.push_back(made[stream]);
      }
    }
    for(auto &root:roots)if(root)root->data_->Index();Projections(*candidate,scratch);Changes(*candidate);
    WireContext context(*candidate);
    for(;;){frame_scratch=scratch.Frame();try {frame=DecodeStateFrame(route.scope,packet->bytes,acceptance.InstalledEvents(),context,frame_scratch,limits);break;}
      catch(const BodyScratchError &error){scratch.Grow(error,limits);}}
    packet->Freeze(route,frame);candidate->packet_=packet;
    // 先预留全部registry位置，再一次发布；不会留下半个joint或将未提交根改成installed。
    packets.reserve(packets.size()+1);states.reserve(states.size()+frame.section_count);candidates.reserve(candidates.size()+1);
    packets.push_back(packet);for(const auto &root:roots)if(root)states.push_back(root);candidates.push_back(candidate);return {decision,candidate};
  }
  struct ValidationContext final : StateCandidateContext {
    ValidationContext(const ClientShadowStorage &owner,const ClientStateCandidateRef &value,const ClientShadowValidation &validator)
        :storage(owner),candidate(value),validation(validator) {}
    StateReadiness Inspect(const StateTarget &target) const override {
      if(!storage.Known(candidate) || target.route!=candidate->Target().route ||
          StateTargetAck(target)!=StateTargetAck(candidate->Target()) || !BytesEqual(target.encoded,candidate->Target().encoded))return StateReadiness::InvalidEncoding;
      if(storage.busy)return StateReadiness::SourcePending;
      for(unsigned i=0;i<target.count;++i){const auto &section=target.sections[i];const auto *base=storage.Installed(Stream(section.stream));
        if(section.base_version && (!base || base->context.target_version!=section.base_version))return StateReadiness::BaseLost;}
      try {
        // 候选等待期间另一流可以前进；只重查当前成员的真实投影，不锁定整个world版本。
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
    const auto newly_affected=std::uint8_t(structural?3:streams);
    acceptance.RequireRecovery(streams,reason,structural);
    const auto unavailable=acceptance.UnusableStreams();for(unsigned stream=0;stream<2;++stream)if(unavailable&(1u<<stream))installed[stream].reset();
    if(busy && (StateTargetStreams(busy->Target())&newly_affected))revoked=true;
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
  s.acceptance.Open(route,agreement);s.route=route;s.active=true;s.revoked=false;s.awaiting_drain=false;s.ticket={};
}
void ClientStateShadow::Close() {
  auto &s=*storage_;s.Thread();s.acceptance.Retire();s.active=false;s.revoked=bool(s.busy);
  s.installed={};s.receipts={};s.Collect();
}
void ClientStateShadow::Collect() {storage_->Collect();}
bool ClientStateShadow::Drained() const {storage_->Thread();storage_->Collect();return storage_->Drained();}
ClientShadowBuildResult ClientStateShadow::Build(const StateRoute &route,StateLane lane,std::span<const std::byte> bytes,ClientStateTables &tables,BodyCodecLimits limits) {
  storage_->Route(route);try {return storage_->Build(lane,bytes,tables,limits);}
  catch(const BodyDependencyError &error){throw ClientShadowError(lane==StateLane::Body?StateReadiness::SourcePending:StateReadiness::MissingClosure,error.what(),kNullIndex,error.LayoutId(),error.FieldId());}
  catch(const Gap &error){throw ClientShadowError(StateReadiness::InvalidEncoding,error.what());}
}
StateAcceptDecision ClientStateShadow::Assess(const ClientStateCandidateRef &candidate,const ClientShadowValidation &validation) const {
  const auto &s=*storage_;s.Thread();if(!s.Known(candidate))Fail(StateReadiness::InvalidReference,"shadow_foreign_candidate");
  ClientShadowStorage::ValidationContext context(s,candidate,validation);return s.acceptance.Assess(candidate->Target(),context);
}
StateApplyTicket ClientStateShadow::Begin(const ClientStateCandidateRef &candidate,const ClientShadowValidation &validation) {
  auto &s=*storage_;s.Thread();if(s.busy)throw ClientTableError(ClientTableFailure::Busy,"shadow_native_still_busy");
  if(!s.Known(candidate))Fail(StateReadiness::InvalidReference,"shadow_foreign_candidate");
  ClientShadowStorage::ValidationContext context(s,candidate,validation);
  const auto ticket=s.acceptance.Begin(candidate->Target(),context);s.busy=candidate;s.ticket=ticket;s.revoked=false;s.awaiting_drain=false;return ticket;
}
std::optional<StateAppliedAck> ClientStateShadow::Complete(StateApplyTicket ticket,StateApplyOutcome outcome,ClientShadowCommit &commit,std::uint8_t affected) {
  auto &s=*storage_;s.Thread();if(!s.busy || !SameTicket(s.ticket,ticket) || s.awaiting_drain)return {};
  if(unsigned(outcome)>unsigned(StateApplyOutcome::FailedAfterWrites) || affected>3 || (outcome!=StateApplyOutcome::FailedAfterWrites && affected))
    Fail(StateReadiness::InvalidEncoding,"shadow_invalid_completion");
  const auto candidate=s.busy;const auto streams=StateTargetStreams(candidate->Target());
  if(outcome==StateApplyOutcome::FailedBeforeWrites){(void)s.acceptance.Finish(ticket,outcome);s.busy.reset();s.ticket={};s.revoked=false;return {};}
  const bool valid=s.active && !s.revoked && ticket.route==s.route;
  if(valid && outcome==StateApplyOutcome::Success){
    try {
      const auto ack=s.acceptance.Finish(ticket,outcome);
      if(ack){for(unsigned stream=0;stream<2;++stream)if(streams&(1u<<stream)){s.installed[stream]=candidate->states_[stream];s.receipts[stream]=candidate->packet_;}
        if(!commit.Promote(*candidate,*ack)){s.Recover(3,StateResyncReason::Apply,true);s.awaiting_drain=true;s.revoked=true;return {};}
        s.busy.reset();s.ticket={};s.revoked=false;return ack;}
    }catch(...){s.Recover(streams,StateResyncReason::Apply,streams==3);s.awaiting_drain=true;s.revoked=true;throw;}
  } else (void)s.acceptance.Finish(ticket,StateApplyOutcome::FailedAfterWrites,affected);
  // 已撤资格的Success也可能写过世界；保留实际busy与内存直到主线程正常清理回报。
  if(s.active)s.Recover(std::uint8_t(streams|affected),StateResyncReason::Apply,(streams|affected)==3);
  s.awaiting_drain=true;s.revoked=true;return {};
}
bool ClientStateShadow::CompleteDrain(StateApplyTicket ticket) {
  auto &s=*storage_;s.Thread();if(!s.busy || !s.awaiting_drain || !SameTicket(ticket,s.ticket))return false;
  s.busy.reset();s.ticket={};s.awaiting_drain=false;s.revoked=false;return true;
}
void ClientStateShadow::RequireRecovery(std::uint8_t streams,StateResyncReason reason,bool structural) {auto &s=*storage_;s.Thread();s.Recover(streams,reason,structural);}
ClientShadowStateRef ClientStateShadow::Installed(BodyStream stream) const {
  const auto &s=*storage_;s.Thread();const auto index=Stream(stream);return s.active && s.Installed(index)?s.installed[index]:nullptr;
}
std::array<std::uint32_t,2> ClientStateShadow::HistoricalVersions() const {storage_->Thread();return storage_->acceptance.HistoricalVersions();}
std::uint64_t ClientStateShadow::InstalledEvents() const {storage_->Thread();return storage_->acceptance.InstalledEvents();}
std::optional<StateResync> ClientStateShadow::RecoveryRequest() const {storage_->Thread();return storage_->acceptance.RecoveryRequest();}
std::array<std::optional<StateAppliedAck>,2> ClientStateShadow::Reacknowledgements() const {storage_->Thread();return storage_->acceptance.Reacknowledgements();}
} // namespace cs::state
