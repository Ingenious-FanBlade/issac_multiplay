#include "state/client_tables.h"
#include <algorithm>
#include <tuple>

namespace cs::state {
namespace {
#include "state_protocol.inc"
[[noreturn]] void Fail(ClientTableFailure failure,const char *reason) {throw ClientTableError(failure,reason);}
using Workspace=ClientStateWorkspace;
struct TableHeader {std::uint32_t epoch=0,previous=0;std::uint16_t count=0;};
std::uint64_t Bits(std::span<const std::byte> bytes) {
  if(bytes.size()>8)Fail(ClientTableFailure::Internal,"client_table_integer_width");
  std::uint64_t result=0;for(std::size_t i=0;i<bytes.size();++i)result|=std::uint64_t(std::to_integer<unsigned>(bytes[i]))<<(8*i);return result;
}
// 分配前按同源固定头校验scope／版本并取准确count；不信任任意网络偏移或分配上限。
TableHeader Header(const StateRoute &route,std::span<const std::byte> bytes) {
  if(bytes.size()<kW_StateCommonHeaderBytes+kW_StateTableHeaderBytes || bytes.size()>StateMessageBodyLimit(StateLane::Context))
    Fail(ClientTableFailure::InvalidEncoding,"client_table_message_size");
  std::array<BodyFieldView,std::size_t(StateCommonHeaderField::count)> fields{};BodyCodecScratch scratch;scratch.root_fields=fields;
  const auto common=DecodeValue(kW_StateCommonHeader,bytes.first(kW_StateCommonHeaderBytes),scratch);
  const auto get=[&](unsigned index){if(index>=common.fields.size())Fail(ClientTableFailure::Internal,"client_common_header_plan");return Bits(common.fields[index].bytes);};
  const StateScope scope{get(unsigned(StateCommonHeaderField::session)),get(unsigned(StateCommonHeaderField::run)),
      get(unsigned(StateCommonHeaderField::floor)),get(unsigned(StateCommonHeaderField::dimension)),
      get(unsigned(StateCommonHeaderField::room)),get(unsigned(StateCommonHeaderField::scene))};
  if(get(unsigned(StateCommonHeaderField::layout_version))!=LayoutVersion() || scope!=route.scope)
    Fail(ClientTableFailure::InvalidEncoding,"client_table_version_or_scope");
  const auto table=DecodeValue(kW_StateTableHeader,bytes.subspan(kW_StateCommonHeaderBytes,kW_StateTableHeaderBytes),scratch);
  const auto value=[&](StateTableHeaderField field){return Bits(table.fields[unsigned(field)].bytes);};
  TableHeader result{std::uint32_t(value(StateTableHeaderField::epoch)),std::uint32_t(value(StateTableHeaderField::previous_epoch)),
      std::uint16_t(value(StateTableHeaderField::entry_count))};
  if(!result.epoch || result.previous>=result.epoch)Fail(ClientTableFailure::InvalidEncoding,"client_table_epoch");return result;
}
auto Key(const PresentationMember &identity) {return std::pair(identity.space,identity.id);}
} // namespace

struct ClientResourceContent final {
  explicit ClientResourceContent(const std::shared_ptr<ClientStateMemory> &memory):bytes(ClientStateAllocator<std::byte>(memory)) {}
  ClientStateVector<std::byte> bytes;
};
ClientMemberSnapshot::ClientMemberSnapshot(const std::shared_ptr<ClientStateMemory> &memory)
    :entries_(ClientStateAllocator<StateMemberEntry>(memory)),update_(ClientStateAllocator<StateMemberEntry>(memory)) {}
StateMemberDirectory ClientMemberSnapshot::Directory() const noexcept {return {route_.scope,epoch_,previous_,entries_,update_};}
ClientResourceSnapshot::ClientResourceSnapshot(const std::shared_ptr<ClientStateMemory> &memory)
    :entries_(ClientStateAllocator<ResourceEntryView>(memory)),update_(ClientStateAllocator<ResourceEntryView>(memory)),
     content_(ClientStateAllocator<std::shared_ptr<const ClientResourceContent>>(memory)),
     edges_(ClientStateAllocator<ResourceEdge>(memory)),offsets_(ClientStateAllocator<std::uint32_t>(memory)) {}
ResourceDirectoryView ClientResourceSnapshot::Directory() const noexcept {return {session_,run_,epoch_,previous_,entries_,update_};}
ResourceGraphPlan ClientResourceSnapshot::Graph() const noexcept {return {session_,run_,epoch_,entries_,edges_,offsets_};}

// 唯一后台可变状态；registry强引用负责最后释放，快照之间仅共享不可变内容项。
struct ClientStateTableStorage final {
  struct History {PresentationMember identity;bool live=false;};
  struct LocalResource {
    ResourceKey key;
    ClientResourceState state=ClientResourceState::Unprepared;
    std::uint64_t generation=0;
    ClientResourcePreparationTicket ticket;
    ClientResourceSnapshotRef pending;
  };
  std::shared_ptr<ClientStateMemory> memory;
  std::thread::id owner;
  StateRoute route{},closed{};
  bool active=false,resource_seen=false;
  std::uint64_t generation=0,serial=0;
  std::uint32_t resource_entries=0,resource_edges=0;
  ClientStateVector<ClientMemberLayoutBinding> bindings;
  ClientStateVector<History> history;
  ClientStateVector<std::shared_ptr<ClientMemberSnapshot>> members;
  ClientStateVector<std::shared_ptr<ClientResourceSnapshot>> resources;
  std::shared_ptr<ClientMemberSnapshot> current_members;
  std::shared_ptr<ClientResourceSnapshot> current_resources;
  ClientStateVector<LocalResource> local;

  ClientStateTableStorage(std::shared_ptr<ClientStateMemory> account,std::thread::id thread,std::span<const ClientMemberLayoutBinding> policy)
      :memory(std::move(account)),owner(thread),bindings(ClientStateAllocator<ClientMemberLayoutBinding>(memory)),
       history(ClientStateAllocator<History>(memory)),members(ClientStateAllocator<std::shared_ptr<ClientMemberSnapshot>>(memory)),
       resources(ClientStateAllocator<std::shared_ptr<ClientResourceSnapshot>>(memory)),local(ClientStateAllocator<LocalResource>(memory)) {
    Thread();if(policy.empty())Fail(ClientTableFailure::InvalidReference,"client_member_policy_empty");
    bindings.assign(policy.begin(),policy.end());std::sort(bindings.begin(),bindings.end());
    for(std::size_t i=0;i<bindings.size();++i){const auto &binding=bindings[i];const auto *layout=FindLayout(binding.layout);
      if(!binding.space || !layout || !IsStateMemberLayout(binding.layout) || (!layout->block_count&&binding.layout!=195) || (i && binding==bindings[i-1]))
        Fail(ClientTableFailure::InvalidReference,"client_member_policy_invalid");}
    if(StateBudgets().table_snapshots>std::numeric_limits<std::uint16_t>::max())Fail(ClientTableFailure::Internal,"client_snapshot_policy");
    members.reserve(std::size_t(StateBudgets().table_snapshots));resources.reserve(std::size_t(StateBudgets().table_snapshots));
    resource_entries=StateResourceBounds().entries;resource_edges=StateResourceBounds().edges;
  }
  void Thread() const {if(std::this_thread::get_id()!=owner)Fail(ClientTableFailure::WrongThread,"client_tables_wrong_thread");}
  void Route(const StateRoute &received) const {Thread();if(!active || route!=received)Fail(ClientTableFailure::WrongRoute,"client_tables_wrong_route");}
  void Collect() {
    Thread();std::erase_if(members,[&](const auto &value){return value!=current_members && value.use_count()==1;});
    std::erase_if(resources,[&](const auto &value){return value!=current_resources && value.use_count()==1;});
  }
  bool Pinned() const {
    for(const auto &value:members)if(value.use_count()>1+(value==current_members?1:0))return true;
    for(const auto &value:resources)if(value.use_count()>1+(value==current_resources?1:0))return true;
    return false;
  }
  template<class Registry> void Slot(Registry &registry) {
    Collect();if(registry.size()>=StateBudgets().table_snapshots)
      throw ClientTableError(ClientTableFailure::SnapshotLimit,"client_table_snapshots_pinned",registry.size()+1,StateBudgets().table_snapshots);
  }
  ClientMemberSnapshotRef Member(std::uint32_t epoch) const {
    if(active)for(const auto &snapshot:members)if(snapshot->route_==route && snapshot->epoch_==epoch)return snapshot;return {};
  }
  ClientResourceSnapshotRef Resource(std::uint32_t epoch) const {
    if(active)for(const auto &snapshot:resources)if(snapshot->session_==route.scope.session && snapshot->run_==route.scope.run && snapshot->epoch_==epoch)return snapshot;return {};
  }
  void Identity(const StateMemberEntry &entry) const {
    const ClientMemberLayoutBinding binding{entry.identity.space,entry.identity.layout};
    if(!std::binary_search(bindings.begin(),bindings.end(),binding))Fail(ClientTableFailure::InvalidReference,"client_unregistered_member_space_layout");
    const auto it=std::lower_bound(history.begin(),history.end(),Key(entry.identity),[](const History &known,auto key){return Key(known.identity)<key;});
    if(it==history.end() || Key(it->identity)!=Key(entry.identity))return;
    const auto &before=it->identity,&after=entry.identity;
    if(after.use<before.use || (after.use==before.use && (!it->live || after.shape<before.shape)))
      Fail(ClientTableFailure::RetiredIdentity,"client_member_identity_rewind_or_retired");
    if(after.use==before.use && after.shape==before.shape && after.layout!=before.layout)
      Fail(ClientTableFailure::InvalidReference,"client_member_layout_changed_without_shape");
  }
  void Index(const StateMemberEntry &next) const {
    for(const auto &snapshot:members){
      if(snapshot.use_count()<=1+(snapshot==current_members?1:0))continue;
      const auto *before=FindStateMember(snapshot->Directory(),next.index);
      if(before && (Key(before->identity)!=Key(next.identity) || before->identity.use!=next.identity.use))
        Fail(ClientTableFailure::MemberReuse,"client_member_index_still_borrowed");
    }
  }
  struct Transitions final : StateMemberTransitions {
    explicit Transitions(const ClientStateTableStorage &value):storage(value) {}
    bool Allowed(const StateScope &,const StateMemberEntry *,const StateMemberEntry *after) const override {
      if(after){storage.Identity(*after);storage.Index(*after);}return true;
    }
    const ClientStateTableStorage &storage;
  };
  // 构建独立历史，完整成功前不退休旧身份。跨route首次full同样核最高use／shape。
  ClientStateVector<History> HistoryFor(const ClientMemberSnapshot &snapshot) const {
    ClientStateVector<StateMemberEntry> ordered(snapshot.entries_,ClientStateAllocator<StateMemberEntry>(memory,memory::Pool::Workspace));
    std::sort(ordered.begin(),ordered.end(),[](const auto &a,const auto &b){return Key(a.identity)<Key(b.identity);});
    ClientStateVector<History> next{ClientStateAllocator<History>(memory)};next.reserve(history.size()+ordered.size());
    std::size_t a=0,b=0;
    while(a<history.size() || b<ordered.size()){
      if(b==ordered.size() || (a<history.size() && Key(history[a].identity)<Key(ordered[b].identity))){auto old=history[a++];old.live=false;next.push_back(old);}
      else {const auto &entry=ordered[b++];Identity(entry);Index(entry);next.push_back({entry.identity,true});
        if(a<history.size() && Key(history[a].identity)==Key(entry.identity))++a;}
    }
    return next;
  }
  LocalResource *Local(ResourceKey key) {
    const auto it=std::lower_bound(local.begin(),local.end(),key,[](const auto &value,auto target){return value.key<target;});
    return it==local.end() || it->key!=key?nullptr:&*it;
  }
  const LocalResource *Local(ResourceKey key) const {return const_cast<ClientStateTableStorage *>(this)->Local(key);}

  ClientTableInstall Members(std::span<const std::byte> encoded) {
    Workspace workspace;const auto header=Header(route,encoded);
    if(encoded.size()!=kW_StateCommonHeaderBytes+kW_StateTableHeaderBytes+std::size_t(header.count)*kW_StateMemberEntryBytes)
      Fail(ClientTableFailure::InvalidEncoding,"client_member_table_length");
    ClientStateVector<StateMemberEntry> update{ClientStateAllocator<StateMemberEntry>(memory,memory::Pool::Workspace)};update.resize(header.count);
    ClientStateVector<std::uint16_t> order{ClientStateAllocator<std::uint16_t>(memory,memory::Pool::Workspace)};order.resize(header.count);
    const auto incoming=DecodeStateMemberTable(route.scope,encoded,update,order);Transitions transitions(*this);
    const auto existing=Member(header.epoch);
    if(existing){
      // 重传只比较当时快照，不按当前退休历史重新解释旧操作。
      try {(void)MergeStateMemberTableInto(existing->Directory(),incoming,{},transitions);}
      catch(const Gap &error){throw ClientTableError(ClientTableFailure::EpochConflict,error.what());}
      return ClientTableInstall::Duplicate;
    }
    if(current_members && header.epoch<current_members->epoch_)return ClientTableInstall::Stale;
    const auto current=current_members?current_members->Directory():StateMemberDirectory{};
    if(header.previous && header.previous!=current.epoch)Fail(ClientTableFailure::EpochChain,"client_member_previous_epoch");
    Slot(members);
    auto next=std::allocate_shared<ClientMemberSnapshot>(ClientStateAllocator<ClientMemberSnapshot>(memory),memory);
    next->route_=route;next->epoch_=header.epoch;next->previous_=header.previous;next->update_.assign(update.begin(),update.end());
    for(const auto &entry:update)if(entry.operation!=MemberOperation::Leave)Identity(entry);
    next->entries_.resize((std::min)(std::size_t(kW_member_entries),current.entries.size()+update.size()));
    const StateMemberTable owned{route.scope,header.epoch,header.previous,next->update_};
    const auto merged=MergeStateMemberTableInto(current,owned,next->entries_,transitions);next->entries_.resize(merged.entries.size());
    auto next_history=HistoryFor(*next);
    // registry已预分配，以下提交不分配；任何此前异常仅销毁私有候选。
    members.push_back(next);history.swap(next_history);current_members=std::move(next);return ClientTableInstall::Installed;
  }

  ClientTableInstall Resources(std::span<const std::byte> encoded,bool rebase) {
    Workspace workspace;const auto header=Header(route,encoded);
    if(header.count>resource_entries)Fail(ClientTableFailure::InvalidEncoding,"client_resource_count");
    ClientStateVector<ResourceEntryView> update{ClientStateAllocator<ResourceEntryView>(memory,memory::Pool::Workspace)};update.resize(header.count);
    ClientStateVector<BodyFieldView> fields{ClientStateAllocator<BodyFieldView>(memory,memory::Pool::Workspace)};fields.resize(BodySourceMaximumFields());
    // 当前资源payload为完整具名值，无跨块共享投影；root_fields仅用于资源用途校验。
    ResourceCodecScratch scratch;scratch.body.root_fields=fields;
    const auto incoming=DecodeResourceTable(route.scope,encoded,update,scratch);
    const auto existing=Resource(header.epoch);
    if(existing && !rebase){try {(void)MergeResourceTableInto(existing->Directory(),incoming,{},scratch);}
      catch(const Gap &error){throw ClientTableError(ClientTableFailure::EpochConflict,error.what());}
      return ClientTableInstall::Duplicate;}
    if(!rebase && current_resources && header.epoch<current_resources->epoch_)return ClientTableInstall::Stale;
    const auto current=current_resources?current_resources->Directory():ResourceDirectoryView{};
    if(!rebase && header.previous && header.previous!=current.epoch)Fail(ClientTableFailure::EpochChain,"client_resource_previous_epoch");
    Slot(resources);
    auto next=std::allocate_shared<ClientResourceSnapshot>(ClientStateAllocator<ClientResourceSnapshot>(memory),memory);
    next->session_=route.scope.session;next->run_=route.scope.run;next->epoch_=header.epoch;next->previous_=header.previous;
    next->entries_.resize((std::min)(std::size_t(resource_entries),current.entries.size()+update.size()));
    const auto merged=rebase?RebaseStateResourceEpoch(closed,route,current,incoming,next->entries_,scratch):
        MergeResourceTableInto(current,incoming,next->entries_,scratch);
    next->entries_.resize(merged.entries.size());next->content_.reserve(next->entries_.size());
    for(auto &entry:next->entries_){
      const auto *known=FindResource(current,entry.key);
      if(known){const auto index=std::size_t(known-current.entries.data());next->content_.push_back(current_resources->content_[index]);}
      else {auto content=std::allocate_shared<ClientResourceContent>(ClientStateAllocator<ClientResourceContent>(memory),memory);
        content->bytes.assign(entry.payload.begin(),entry.payload.end());next->content_.push_back(std::move(content));}
      entry.payload=next->content_.back()->bytes;
    }
    next->update_.reserve(update.size());
    for(const auto &entry:update){const auto *owned=FindResource(next->Directory(),entry.key);
      if(!owned)Fail(ClientTableFailure::Internal,"client_resource_update_not_merged");next->update_.push_back(*owned);}
    std::uint64_t payload=0;for(const auto &entry:next->entries_)payload+=entry.payload.size();
    // 每条资源边至少消费一个u16引用；按实际payload得保守容量，不预占所有最大容器。
    next->edges_.resize(std::size_t((std::min)(std::uint64_t(resource_edges),payload/2)));next->offsets_.resize(next->entries_.size()+1);
    ClientStateVector<std::uint32_t> order{ClientStateAllocator<std::uint32_t>(memory,memory::Pool::Workspace)};order.resize(next->entries_.size());
    ClientStateVector<std::uint8_t> state{ClientStateAllocator<std::uint8_t>(memory,memory::Pool::Workspace)},heights{ClientStateAllocator<std::uint8_t>(memory,memory::Pool::Workspace)};
    state.resize(order.size());heights.resize(order.size());ResourceGraphScratch graph{next->edges_,next->offsets_,order,state,heights};
    const auto plan=BuildResourceGraph(next->Directory(),graph,scratch);next->edges_.resize(plan.edges.size());
    ClientStateVector<LocalResource> next_local{ClientStateAllocator<LocalResource>(memory)};next_local.reserve(next->entries_.size());
    for(const auto &entry:next->entries_){const auto *known=Local(entry.key);if(known)next_local.push_back(*known);else {LocalResource value;value.key=entry.key;next_local.push_back(std::move(value));}}
    if(rebase)resources.clear();
    resources.push_back(next);local.swap(next_local);current_resources=std::move(next);return ClientTableInstall::Installed;
  }
};

ClientStateTables::ClientStateTables(std::shared_ptr<ClientStateMemory> memory,std::thread::id owner,std::span<const ClientMemberLayoutBinding> bindings)
    :memory_(std::move(memory)) {
  if(!memory_)Fail(ClientTableFailure::Internal,"client_tables_memory_required");
  storage_=std::allocate_shared<ClientStateTableStorage>(ClientStateAllocator<ClientStateTableStorage>(memory_),memory_,owner,bindings);
}
ClientStateTables::~ClientStateTables()=default;
void ClientStateTables::Open(const StateRoute &route,StateAgreement agreement,std::uint64_t generation) {
  auto &s=*storage_;s.Thread();
  if(s.active || !route.connection || !route.route || !route.scope.session || !route.scope.run || !route.scope.scene ||
      !generation || !CompatibleStateAgreement(agreement) || (s.route.connection==route.connection && route.route<=s.route.route))
    Fail(ClientTableFailure::WrongRoute,"client_tables_open_route_or_agreement");
  const bool same=s.route.scope.session==route.scope.session && s.route.scope.run==route.scope.run;
  if(!same){if(s.Pinned())Fail(ClientTableFailure::Busy,"client_tables_previous_run_borrowed");
    s.current_members.reset();s.current_resources.reset();s.members.clear();s.resources.clear();
    ClientStateVector<ClientStateTableStorage::History>{ClientStateAllocator<ClientStateTableStorage::History>(memory_)}.swap(s.history);
    ClientStateVector<ClientStateTableStorage::LocalResource>{ClientStateAllocator<ClientStateTableStorage::LocalResource>(memory_)}.swap(s.local);s.generation=0;
  } else if(generation<s.generation)Fail(ClientTableFailure::Generation,"client_resource_generation_rewind");
  s.current_members.reset();s.route=route;s.active=true;s.resource_seen=false;
  if(generation>s.generation){s.generation=generation;for(auto &local:s.local)local.state=ClientResourceState::Unprepared;}
  // 旧route未结束的准备仍固定其输入；只有旧最终反馈能释放，不向新route转移Pending资格。
  for(auto &local:s.local)if(local.pending)local.state=ClientResourceState::Unprepared;s.Collect();
}
void ClientStateTables::Close() {auto &s=*storage_;s.Thread();if(s.active){s.closed=s.route;s.active=false;}s.Collect();}
bool ClientStateTables::Drained() const {const auto &s=*storage_;s.Thread();return !s.active && !s.Pinned();}
void ClientStateTables::Collect() {storage_->Collect();}
ClientTableInstall ClientStateTables::InstallMembers(const StateRoute &route,std::span<const std::byte> encoded) {
  storage_->Route(route);try {return storage_->Members(encoded);}catch(const Gap &error){throw ClientTableError(ClientTableFailure::InvalidEncoding,error.what());}
}
ClientTableInstall ClientStateTables::InstallResources(const StateRoute &route,std::span<const std::byte> encoded) {
  storage_->Route(route);try {const auto result=storage_->Resources(encoded,false);if(result!=ClientTableInstall::Stale)storage_->resource_seen=true;return result;}
  catch(const Gap &error){throw ClientTableError(ClientTableFailure::InvalidEncoding,error.what());}
}
ClientTableInstall ClientStateTables::RebaseResources(const StateRoute &retired,std::span<const std::byte> encoded) {
  auto &s=*storage_;s.Route(s.route);
  if(s.closed!=retired || s.resource_seen || !s.current_resources || s.Pinned())Fail(ClientTableFailure::Busy,"client_resource_rebase_not_fresh_or_drained");
  // 所有旧资源快照无外部引用后，先收窄到唯一当前目录；专用codec逐项核全部既知内容。
  s.Collect();try {const auto result=s.Resources(encoded,true);s.resource_seen=true;return result;}
  catch(const Gap &error){throw ClientTableError(ClientTableFailure::InvalidEncoding,error.what());}
}
ClientMemberSnapshotRef ClientStateTables::Members(std::uint32_t epoch) const {storage_->Thread();return storage_->Member(epoch);}
ClientResourceSnapshotRef ClientStateTables::Resources(std::uint32_t epoch) const {storage_->Thread();return storage_->Resource(epoch);}
std::optional<StateContextReady> ClientStateTables::MetadataReady() const {
  const auto &s=*storage_;s.Thread();if(!s.active || !s.resource_seen || !s.current_members || !s.current_resources)return {};
  return StateContextReady{s.route.scope,s.current_members->Directory().epoch,s.current_resources->Directory().epoch};
}
ClientResourceState ClientStateTables::ResourceState(std::uint32_t epoch,ResourceKey key) const {
  const auto &s=*storage_;s.Thread();const auto snapshot=s.Resource(epoch);
  if(!snapshot || !FindResource(snapshot->Directory(),key))Fail(ClientTableFailure::MissingTable,"client_resource_state_not_declared");
  const auto *value=s.Local(key);return value && value->generation==s.generation?value->state:ClientResourceState::Unprepared;
}
bool ClientStateTables::Materialized(const ResourceDirectoryView &directory,const ResourceEntryView &entry) const {
  const auto &s=*storage_;s.Thread();const auto snapshot=s.Resource(directory.epoch);
  if(!snapshot || snapshot->Directory().entries.data()!=directory.entries.data() || directory.session!=s.route.scope.session ||
      directory.run!=s.route.scope.run || FindResource(directory,entry.key)!=&entry)return false;
  const auto *local=s.Local(entry.key);return local && local->generation==s.generation && local->state==ClientResourceState::Ready;
}
ResourceGraphView ClientStateTables::Prepare(const ClientResourceSnapshotRef &snapshot,std::span<const ResourceKey> roots,ResourceGraphScratch &scratch) const {
  Workspace workspace;return Prepare(snapshot,roots,scratch,workspace);
}
ResourceGraphView ClientStateTables::Prepare(const ClientResourceSnapshotRef &snapshot,std::span<const ResourceKey> roots,ResourceGraphScratch &scratch,const ClientStateWorkspace &workspace) const {
  workspace.RequireCurrentThread();
  const auto &s=*storage_;s.Thread();if(!snapshot || s.Resource(snapshot->Directory().epoch)!=snapshot)Fail(ClientTableFailure::MissingTable,"client_resource_snapshot_not_current_route");
  if(roots.empty())return {true,true,0,0,{}};
  const auto count=snapshot->Directory().entries.size();
  if(scratch.order.size()<count || scratch.state.size()<count || scratch.heights.size()<count)
    Fail(ClientTableFailure::Budget,"client_resource_closure_scratch");
  return PrepareResourceClosure(snapshot->Directory(),snapshot->Graph(),scratch,this,roots);
}
ClientResourcePreparationTicket ClientStateTables::BeginResourcePreparation(std::uint32_t epoch,ResourceKey key) {
  auto &s=*storage_;s.Thread();const auto snapshot=s.Resource(epoch);auto *value=s.Local(key);
  if(!snapshot || !value || !FindResource(snapshot->Directory(),key))Fail(ClientTableFailure::MissingTable,"client_resource_prepare_not_declared");
  if(value->pending)Fail(ClientTableFailure::Busy,"client_resource_prepare_still_busy");
  if(value->generation==s.generation){
    if(value->state==ClientResourceState::Ready)Fail(ClientTableFailure::Busy,"client_resource_already_ready");
    if(value->state==ClientResourceState::MissingLocalResource)Fail(ClientTableFailure::MissingLocalResource,"client_resource_known_missing");
    if(value->state==ClientResourceState::Failed)Fail(ClientTableFailure::ResourceFailure,"client_resource_known_failed");
  }
  if(s.serial==UINT64_MAX)Fail(ClientTableFailure::Generation,"client_resource_prepare_serial_exhausted");
  value->ticket={s.route,key,epoch,s.generation,++s.serial};value->pending=snapshot;value->generation=s.generation;value->state=ClientResourceState::Pending;return value->ticket;
}
bool ClientStateTables::CompleteResourcePreparation(const ClientResourcePreparationTicket &ticket,ClientResourceState result) {
  auto &s=*storage_;s.Thread();if(unsigned(result)>unsigned(ClientResourceState::Failed))Fail(ClientTableFailure::Internal,"client_resource_prepare_result");
  auto *value=s.Local(ticket.key);if(!value || !value->pending || value->ticket!=ticket)return false;
  const bool current=s.active && ticket.route==s.route && ticket.generation==s.generation;
  if(result!=ClientResourceState::Pending){value->pending.reset();value->ticket={};}
  if(current){value->generation=s.generation;value->state=result;}return current;
}
void ClientStateTables::InvalidateResourceGeneration(std::uint64_t generation) {
  auto &s=*storage_;s.Thread();if(!s.active || generation<=s.generation)Fail(ClientTableFailure::Generation,"client_resource_generation_not_fresh");
  s.generation=generation;for(auto &value:s.local)value.state=ClientResourceState::Unprepared;
}
} // namespace cs::state
