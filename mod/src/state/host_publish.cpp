#include "state/host_publish.h"
#include "state/budget.h"
#include "core/deferred_values.h"
#include "state_host_fields.inc"
#include <algorithm>
#include <bit>
#include <bitset>
#include <cstring>
#include <map>
#include <tuple>
#include <utility>

namespace cs::state {
namespace {
[[noreturn]] void Fail(const char *s){throw Gap("host_publish",s);}
bool Same(std::span<const std::byte> a,std::span<const std::byte> b){return a.size()==b.size()&&std::equal(a.begin(),a.end(),b.begin());}
using Key=std::tuple<std::uint16_t,std::uint64_t,std::uint64_t>;
Key Identity(CaptureSubject s){return {s.space,s.local,s.use};}
Key Identity(PresentationMember s){return {std::uint16_t(s.space),s.id,s.use};}

constexpr std::uint32_t kMissingMember=std::uint32_t(kNullIndex)+1;
// 只索引本帧实际成员和沿用身份。开放寻址使用连续空间，容量记入 Workspace。
class FrameMemberLookup {
  struct Entry {Key identity;std::uint32_t index=kMissingMember;};
public:
  explicit FrameMemberLookup(std::size_t count)
    : memory_(memory::Reserve(memory::Pool::Workspace,
        std::bit_ceil(2*count+1)*sizeof(Entry),"host_frame_links")),
      entries_(std::bit_ceil(2*count+1)) {}

  void Add(Key key,std::uint32_t index) noexcept {
    auto &entry=entries_[Slot(key)];
    if(entry.index==kMissingMember)entry={key,index};
  }

  std::uint32_t Find(Key key) const noexcept {return entries_[Slot(key)].index;}

private:
  std::size_t Slot(Key key) const noexcept {
    const auto [space,local,use]=key;
    auto value=(local*0x9e3779b97f4a7c15ull)^std::rotl(use*0xbf58476d1ce4e5b9ull,21)^space;
    value^=value>>32;
    const auto mask=entries_.size()-1;
    auto slot=std::size_t(value)&mask;
    while(entries_[slot].index!=kMissingMember && entries_[slot].identity!=key)slot=(slot+1)&mask;
    return slot;
  }

  memory::Lease memory_;
  std::vector<Entry> entries_;
};
// 原生身份 revision 从0起步，线上 shape 保留0为无效；只在封存边界做一一映射。
std::uint64_t WireShape(CaptureSubject subject){
  if(subject.space==1||subject.space==2){
    if(subject.shape==UINT64_MAX)Fail("native_shape_exhausted");
    return subject.shape+1;
  }
  return subject.shape?subject.shape:1;
}
HostStateBytesRef Own(std::span<const std::byte> bytes){return std::make_shared<const HostStateBytes>(bytes);}
// 各编码阶段共用有界工作区，租约覆盖实际capacity，不在游戏主线程分配。
struct Scratch {
  memory::Lease memory;
  std::vector<std::byte> output,links;
  std::vector<ProjectionFact> facts;
  std::vector<BodyFieldView> fields;
  std::uint64_t fixed_bytes=0;
  Scratch():memory(memory::Reserve(memory::Pool::Workspace,0,"host_state_scratch")){
    const auto bytes=std::uint32_t(StateBudgets().prepare_wire_bytes);
    memory->Resize(std::uint64_t(bytes)+65536+8192*sizeof(ProjectionFact)+BodySourceMaximumFields()*sizeof(BodyFieldView));
    output.resize(bytes);links.resize(65536);facts.resize(8192);fields.resize(BodySourceMaximumFields());
    fixed_bytes=memory->Bytes()-bytes;
  }
  BodyCodecScratch Body(){return {facts,0,fields};}
  void Ensure(std::size_t bytes){if(output.size()>=bytes)return;
    memory->Resize(fixed_bytes+output.capacity()+bytes);std::vector<std::byte> next(bytes);output.swap(next);next.clear();std::vector<std::byte>().swap(next);memory->Resize(fixed_bytes+output.capacity());}
  void Trim() noexcept {if(output.capacity()>StateBudgets().prepare_wire_bytes){std::vector<std::byte>().swap(output);memory->Resize(fixed_bytes);}}
};
struct ScratchJob {Scratch &scratch;~ScratchJob(){scratch.Trim();}};
// 成功或异常都结束同一个工作线程计量区间。
struct CaptureSample {
  HostCaptureTiming timing;
  HostCapturePhase phase;
  CaptureSample(HostCaptureTiming t,HostCapturePhase p) noexcept:timing(t),phase(p){if(timing.sample)timing.sample(timing.owner,phase,true);}
  ~CaptureSample(){if(timing.sample)timing.sample(timing.owner,phase,false);}
};
BodyCodecLimits Limits(bool reliable=true){return {std::uint32_t(StateMessageBodyLimit(reliable?StateLane::Baseline:StateLane::World)),262144,32,reliable};}
ResourceDirectoryView Directory(const HostStateSnapshot &s,std::vector<ResourceEntryView> &entries,std::size_t first=0){
  entries.clear();for(const auto &r:std::span(s.resources).subspan(first))entries.push_back({r.key,r.kind,r.payload->bytes});
  std::sort(entries.begin(),entries.end(),[](const auto &a,const auto &b){return a.key<b.key;});
  return {s.witness.scope.session,s.witness.scope.run,s.resource_epoch,0,entries,{}};
}
std::uint64_t SnapshotBytes(const HostStateSnapshot &s){
  return sizeof(s)+s.members.capacity()*sizeof(StateMemberEntry)+s.resources.capacity()*sizeof(HostStateResource)+
    s.records.capacity()*sizeof(std::shared_ptr<const HostStateRecord>)+s.retained_members.capacity()*sizeof(HostRetainedRoot)+
    s.reusable_roots.capacity()*sizeof(PresentationMember)+s.events.capacity()*sizeof(HostStateEvent)+256;
}
const StateMemberEntry *MemberAt(const HostStateSnapshot &snapshot,std::uint16_t index) noexcept {
  const auto &members=snapshot.members;
  const auto at=std::lower_bound(members.begin(),members.end(),index,[](const auto &entry,auto n){return entry.index<n;});
  return at!=members.end()&&at->index==index?&*at:nullptr;
}
bool Retains(const HostStateSnapshot &snapshot,std::uint16_t index) noexcept {
  const auto at=std::lower_bound(snapshot.retained_members.begin(),snapshot.retained_members.end(),index,
    [](const auto &root,auto value){return root.member<value;});
  return at!=snapshot.retained_members.end() && at->member==index;
}

// 普通来源、资源投影及回执共用目录字段。回执只保存后续差分和成员租约所需的元数据。
void CopyFrameDirectory(HostStateSnapshot &target,const HostStateSnapshot &source) {
  target.witness=source.witness;
  target.ordinary=source.ordinary;
  target.flat_versions=source.flat_versions;
  target.member_epoch=source.member_epoch;
  target.resource_epoch=source.resource_epoch;
  target.member_indices=source.member_indices;
  target.members=source.members;
  target.retained_members=source.retained_members;
  target.event_watermark=source.event_watermark;
}

std::shared_ptr<const HostStateRecord> FlatRecordReceipt(const HostStateRecord &record) {
  auto meta=std::make_shared<HostStateRecord>();
  meta->member=record.member;
  meta->stream=record.stream;
  meta->topology=record.topology;
  meta->binding=record.binding;
  meta->member_refs=record.member_refs;
  meta->resource_prefix=record.resource_prefix;
  meta->flat_suspended=record.flat_suspended;
  meta->memory=memory::Reserve(memory::Pool::Values,sizeof(HostStateRecord)+128+
    meta->member_refs.capacity()*sizeof(std::uint16_t),"host_flat_receipt_record");
  return meta;
}

HostStateSnapshotRef FlatReceipt(const HostStateSnapshot &source) {
  if(std::none_of(source.records.begin(),source.records.end(),[](const auto &r){return bool(r->flat);}))return {};
  auto receipt=memory::DeferredValue<HostStateSnapshot>();
  CopyFrameDirectory(*receipt,source);
  receipt->resources=source.resources;
  receipt->records.reserve(source.records.size());
  for(const auto &record:source.records)
    receipt->records.push_back(record->flat_receipt?record->flat_receipt:record);
  receipt->memory=memory::Reserve(memory::Pool::Values,SnapshotBytes(*receipt),"host_flat_receipt");
  return receipt;
}

void MarkRetained(HostStateSnapshot &target,const HostStateSnapshot *before,const CaptureRecord &record,
                  std::uint16_t member,CaptureFailure reason) {
  const auto key=Identity(record.subject);
  const auto matches=[&](const HostRetainedRoot &root){return Key(root.space,root.local,root.use)==key;};
  if(std::any_of(target.retained_members.begin(),target.retained_members.end(),matches))return;
  HostRetainedRoot retained{member,1,reason,record.subject.space,record.subject.local,record.subject.use};
  if(before){
    const auto old=std::find_if(before->retained_members.begin(),before->retained_members.end(),matches);
    if(old!=before->retained_members.end()){
      retained.first=old->first;
      retained.count=old->count+unsigned(old->count!=UINT32_MAX);
    }
  }
  target.retained_members.push_back(retained);
}
// 完整根先解码得到共享owner存在性，随后增量投影也只能读取该根的同一份事实。
class FrameContext final:public StateFrameCodecContext,public BodyCodecContext {
public:
  explicit FrameContext(const HostStateSnapshot &s):FrameContext(s,s){}
  FrameContext(const HostStateSnapshot &values,const HostStateSnapshot &tables)
      :snapshot(values),tables(tables),directory(Directory(tables,resources)),resource_context(directory){}
  std::optional<PresentationMember> Member(std::uint32_t epoch,std::uint16_t index) const override {
    if(epoch!=tables.member_epoch)return {};
    const auto *entry=MemberAt(tables,index);return entry?std::optional(entry->identity):std::nullopt;
  }
  const BodyCodecContext *BodyContext(const PresentationBatchContext &,std::uint16_t member) const override {current=member;return this;}
  std::optional<CodecItemConfig> Item(std::uint8_t table,std::uint16_t index) const override{return resource_context.Item(table,index);}
  std::optional<bool> SharedPresence(const CodecLocation &loc) const override {
    for(const auto &record:snapshot.records)if(record->member.index==current)
    for(const auto &f:record->projections)if(f.kind==ProjectionFactKind::SharedPresence&&f.layout==loc.layout&&f.group==loc.canonical_field&&f.depth==loc.parents.size()&&
      std::equal(loc.parents.begin(),loc.parents.end(),f.parents.begin()))return f.value!=0;
    return {};
  }
  const HostStateSnapshot &snapshot,&tables;
  std::vector<ResourceEntryView> resources;
  ResourceDirectoryView directory;
  ResourceBodyContext resource_context;
  mutable std::uint16_t current=kNullIndex;
};
std::vector<std::byte> Join(const HostStateRecord &record,std::uint32_t mask){
  std::vector<std::byte> out;std::size_t size=0;for(const auto &b:record.blocks)if(mask&(1u<<b.block))size+=b.payload->bytes.size();out.reserve(size);
  for(const auto &b:record.blocks)if(mask&(1u<<b.block))out.insert(out.end(),b.payload->bytes.begin(),b.payload->bytes.end());return out;
}
std::uint32_t Mask(const HostStateRecord &record){std::uint32_t out=0;for(const auto &b:record.blocks)out|=1u<<b.block;return out;}
// 沿用保留本对象上次完整值。其他成员真实离开后，只清理旧值中已失效的引用。
bool SameReference(const HostStateSnapshot &from,const HostStateSnapshot &to,std::uint16_t index) noexcept {
  const auto *before=MemberAt(from,index),*after=MemberAt(to,index);
  return before&&after&&Identity(before->identity)==Identity(after->identity)&&before->identity.layout==after->identity.layout;
}
std::shared_ptr<const HostStateRecord> RetainRecord(const std::shared_ptr<const HostStateRecord> &old,
    const HostStateSnapshot &prior,const HostStateSnapshot &target,std::optional<FrameContext> &context,Scratch &scratch) {
  if(std::all_of(old->member_refs.begin(),old->member_refs.end(),[&](auto index){
      return SameReference(prior,target,index);
    }))return old;
  if(old->flat){
    // 沿用保持镜像与戳。引用已失效时禁止携带旧值，等下次真实采集再解除。
    if(old->flat_suspended)return old;
    auto next=std::make_shared<HostStateRecord>(*old);
    next->flat_suspended=true;
    next->flat_receipt=FlatRecordReceipt(*next);
    next->memory=memory::Reserve(memory::Pool::Values,sizeof(HostStateRecord)+128+
      next->member_refs.capacity()*sizeof(std::uint16_t),"host_flat_retained_record");
    return next;
  }

  std::size_t size=0;
  for(const auto &block:old->blocks)size+=block.payload->bytes.size();
  auto workspace=memory::Reserve(memory::Pool::Workspace,size,"host_retained_record");
  const auto bytes=Join(*old,Mask(*old));
  workspace->Resize(bytes.capacity());
  scratch.Ensure(size);
  std::copy(bytes.begin(),bytes.end(),scratch.output.begin());
  // 观察已验证的引用叶，按原正文偏移修改输出。条件和共享结构仍用旧正文解释。
  class References final:public BodyNodeObserver {
  public:
    const HostStateSnapshot &prior,&target;
    const std::byte *input;
    std::span<std::byte> output;
    References(const HostStateSnapshot &before,const HostStateSnapshot &after,
        const std::byte *bytes,std::span<std::byte> into):prior(before),target(after),input(bytes),output(into) {}
    void Node(const BodyNodeView &node) override {
      if(node.operation!=Operation::Reference||node.resource_table!=255||node.layout||node.bytes.size()!=2)return;
      const auto index=std::uint16_t(std::to_integer<unsigned>(node.bytes[0])|(std::to_integer<unsigned>(node.bytes[1])<<8));
      if(index==kNullIndex||SameReference(prior,target,index))return;
      const auto &step=node.path.back();
      BodySourceFieldInfo field;
      if(!DescribeBodySourceField(step.layout,step.field,field)||field.nonnull)Fail("retained_required_member_unavailable");
      const auto offset=std::size_t(node.bytes.data()-input);
      if(offset>output.size()||node.bytes.size()>output.size()-offset)Fail("retained_reference_range");
      output[offset]=std::byte{0xff};output[offset+1]=std::byte{0xff};
    }
  } refs(prior,target,bytes.data(),std::span(scratch.output).first(size));
  if(!context)context.emplace(prior);
  context->current=old->member.index;
  auto body=scratch.Body();body.observer=&refs;
  const auto decoded=DecodeBody(old->member.identity.layout,Mask(*old),old->stream,bytes,body,&*context,Limits());
  auto next=std::make_shared<HostStateRecord>(*old);
  std::erase_if(next->member_refs,[&](auto index){return !SameReference(prior,target,index);});
  for(auto &block:next->blocks){
    const auto view=std::find_if(decoded.blocks.begin(),decoded.blocks.begin()+decoded.block_count,
      [&](const auto &part){return part.block==block.block;});
    if(view==decoded.blocks.begin()+decoded.block_count)Fail("retained_record_block");
    const auto value=std::span<const std::byte>(scratch.output).subspan(view->bytes.data()-bytes.data(),view->bytes.size());
    if(!Same(value,block.payload->bytes)){block.payload=Own(value);block.changed=target.witness.revision;}
  }
  next->memory=memory::Reserve(memory::Pool::Values,sizeof(HostStateRecord)+next->blocks.capacity()*sizeof(HostStateBlock)+
    next->projections.capacity()*sizeof(ProjectionFact)+next->member_refs.capacity()*sizeof(std::uint16_t)+128,"host_retained_record");
  return next;
}
}
HostStateBytes::HostStateBytes(std::span<const std::byte> value):memory(memory::Reserve(memory::Pool::Values,sizeof(*this)+value.size()+128,"host_state_bytes")),bytes(value.begin(),value.end()){}
namespace {
constexpr std::uint16_t kHostMemberIndices=16384;
struct HostMemberIndex final {
  memory::Lease memory;
  const std::uint16_t value;
  // 每个不可变目录占一个租约；Peer另持有已确认两流目录，事件队列也保留自己的目录。
  std::atomic<std::uint32_t> ownership{0};
  explicit HostMemberIndex(std::uint16_t index)
      :memory(memory::Reserve(memory::Pool::Values,sizeof(*this)+128,"host_member_index")),value(index){}
  bool Recyclable() const noexcept {return ownership.load(std::memory_order_acquire)==0;}
};
using HostMemberIndexRef=std::shared_ptr<HostMemberIndex>;
}
class HostStateMemberIndices final {
  struct Entry {HostMemberIndexRef index;};
public:
  explicit HostStateMemberIndices(std::size_t capacity)
      :memory_(memory::Reserve(memory::Pool::Values,sizeof(*this)+128+capacity*sizeof(Entry),"host_member_index_directory")){
    entries_.reserve(capacity);
    memory_->Resize(sizeof(*this)+128+entries_.capacity()*sizeof(Entry));
  }
  HostStateMemberIndices(const HostStateMemberIndices &)=delete;
  HostStateMemberIndices &operator=(const HostStateMemberIndices &)=delete;
  ~HostStateMemberIndices(){for(const auto &entry:entries_)entry.index->ownership.fetch_sub(1,std::memory_order_acq_rel);}
  void Retain(const HostMemberIndexRef &entry){
    entries_.push_back({entry});
    entry->ownership.fetch_add(1,std::memory_order_relaxed);
  }
private:
  memory::Lease memory_;
  std::vector<Entry> entries_;
};

struct HostStateCapture::Impl {
  Scratch scratch;
  FlatCompareScratch flat_scratch;
  FlatVersions flat_clock{}; // 撤销 Body 候选不回收曾发送的版本号。
  bool body_comparison_reset=false;
  memory::Lease index_memory=memory::Reserve(memory::Pool::Workspace,kOrdinaryCaptureCells*sizeof(std::uint32_t),"host_capture_cell_index");
  std::vector<std::uint32_t> index=std::vector<std::uint32_t>(kOrdinaryCaptureCells);
  Impl(){index_memory->Resize(index.capacity()*sizeof(std::uint32_t));}
  HostStateSnapshotRef latest,ordinary;
  // 编号由在途、已确认和事件目录共同保管；最后一个目录租约释放后才允许复用。
  std::map<Key,HostMemberIndexRef> identities;
  std::vector<std::uint16_t> free_indices;
  std::atomic<std::uint32_t> index_high_water{0},index_reused{0};
  std::atomic<std::uint32_t> flat_padding_failures{0};
  memory::Lease memory=memory::Reserve(memory::Pool::Values,0,"host_state_member_ids");
};
HostStateCapture::HostStateCapture():impl_(std::make_unique<Impl>()){}
HostStateCapture::~HostStateCapture()=default;
void HostStateCapture::Reset() noexcept {
  auto &s=*impl_;s.latest.reset();s.ordinary.reset();s.identities.clear();std::vector<std::uint16_t>().swap(s.free_indices);s.memory->Resize(0);
  s.index_high_water.store(0,std::memory_order_relaxed);s.index_reused.store(0,std::memory_order_relaxed);
  s.flat_padding_failures.store(0,std::memory_order_relaxed);
  s.flat_clock={};s.body_comparison_reset=false;
}
void HostStateCapture::DiscardBody() noexcept {
  auto &s=*impl_;
  if(s.latest!=s.ordinary)s.body_comparison_reset=true;
  s.latest=s.ordinary;
}
HostStateSnapshotRef HostStateCapture::Latest() const noexcept{return impl_->latest;}
HostMemberIndexStats HostStateCapture::MemberIndexStats() const noexcept {
  return {impl_->index_high_water.load(std::memory_order_relaxed),impl_->index_reused.load(std::memory_order_relaxed)};
}
std::uint32_t HostStateCapture::FlatPaddingFailures() const noexcept {
  return impl_->flat_padding_failures.load(std::memory_order_relaxed);
}
HostStateSnapshotRef HostStateCapture::Capture(const CapturePacket &packet,std::span<const HostStateResource> resources,void *qualification,bool (*current)(void *) noexcept,
    HostCaptureTiming timing){
  auto &s=*impl_;ScratchJob job{s.scratch};s.scratch.Ensure((2u<<20)+31);if(!ValidCaptureWitness(packet.witness))Fail("invalid_capture");const bool ordinary=packet.witness.phase==CapturePhase::Ordinary;
  if(!ordinary&&packet.witness.phase!=CapturePhase::PlayerPost)Fail("unsupported_capture_phase");
  const auto prior=s.latest;
  const bool flat_contract=UsesFlatRecordContract(LayoutVersion());
  // Worker 允许普通槽替换中间快照。配对 World 若已被替换，本拍 Body 自行退出。
  if(!ordinary&&(!prior||prior->witness.scope!=packet.witness.scope||prior->ordinary.revision!=packet.witness.required_revision))return {};
  const bool new_run=!prior||prior->witness.scope.session!=packet.witness.scope.session||prior->witness.scope.run!=packet.witness.scope.run||prior->witness.generation!=packet.witness.generation;
  const bool same_scope=!new_run&&prior->witness.scope==packet.witness.scope;
  decltype(s.identities) ids;
  std::vector<std::uint16_t> free_indices;
  std::shared_ptr<HostStateMemberIndices> indices;
  auto high_water=new_run?0u:s.index_high_water.load(std::memory_order_relaxed);
  auto reused=new_run?0u:s.index_reused.load(std::memory_order_relaxed);
  // 主线程可能归还最后一个邮箱引用；整个快照的析构交给既有纯值回收线程。
  auto out=memory::DeferredValue<HostStateSnapshot>();out->memory=memory::Reserve(memory::Pool::Values,0,"host_state_snapshot");out->witness=packet.witness;
  out->ordinary=ordinary?packet.witness:prior->ordinary;
  if(flat_contract){
    out->flat_versions=new_run?FlatVersions{}:s.flat_clock;
    // Ordinary 具有两流同窗样本；PlayerPost 只推进 Body，不改已经发布的 World 戳。
    for(unsigned n=ordinary?0u:1u;n<2;++n){
      if(out->flat_versions[n]==kFlatMaxVersion)Fail("flat_stream_version_exhausted");
      ++out->flat_versions[n];
    }
  }
  out->baseline_request_revision=ordinary&&packet.request_baseline?packet.witness.revision:
    prior&&prior->witness.scope==packet.witness.scope?prior->baseline_request_revision:0;
  out->structure_revision=new_run?packet.witness.revision:prior->structure_revision;
  if(!ordinary){out->members=prior->members;out->retained_members=prior->retained_members;out->member_indices=prior->member_indices;out->records=prior->records;out->resources=prior->resources;out->member_epoch=prior->member_epoch;out->resource_epoch=prior->resource_epoch;
    out->event_first=prior->event_first;out->event_watermark=prior->event_watermark;}
  else {
    if(!new_run){ids=s.identities;free_indices=s.free_indices;}
    // 在候选的私有账本上归还空号，只有完整编码且票据仍有效才提交分配结果。
    for(auto i=ids.begin();i!=ids.end();){
      if(!i->second->Recyclable()){++i;continue;}
      free_indices.push_back(i->second->value);
      i=ids.erase(i);
    }
    indices=std::make_shared<HostStateMemberIndices>(packet.records.size());
    const auto retain=[&](const CaptureRecord &record,std::uint16_t member){
      MarkRetained(*out,same_scope?prior.get():nullptr,record,member,record.retained_reason);
    };
    for(const auto &record:packet.records)if(!record.event_sequence){
      const auto previous=(same_scope&&record.retained)?std::find_if(prior->members.begin(),prior->members.end(),[&](const auto &member){
        return Identity(member.identity)==Identity(record.subject);
      }):std::vector<StateMemberEntry>::const_iterator{};
      const auto known_previous=same_scope&&record.retained&&previous!=prior->members.end();
      // 布局未确认的根留在失败统计里，下次取得真实布局后再分配线上编号。
      if(record.retained&&!known_previous&&!IsStateMemberLayout(record.layout)){
        retain(record,kNullIndex);continue;
      }
      const bool anchor=record.layout==195||record.layout==196;
      if((record.stream==BodyStream::Value&&!anchor)||!record.subject.space||!record.subject.local||!record.subject.use||
          (!record.subject.shape&&!anchor&&record.subject.space!=1&&record.subject.space!=2))Fail("invalid_state_root");
      const auto key=Identity(record.subject);auto i=ids.find(key);
      if(i==ids.end()){
        std::uint16_t index;
        if(!free_indices.empty()){
          index=free_indices.back();free_indices.pop_back();if(reused<UINT32_MAX)++reused;
        }else{
          if(high_water>=kHostMemberIndices)Fail("member_index_exhausted");
          index=std::uint16_t(high_water++);
        }
        i=ids.emplace(key,std::make_shared<HostMemberIndex>(index)).first;
      }
      const StateMemberEntry member=known_previous?*previous:StateMemberEntry{i->second->value,MemberOperation::Join,
        {record.subject.space,record.layout,record.subject.local,record.subject.use,WireShape(record.subject)}};
      if(record.retained)retain(record,member.index);
      const auto known=std::find_if(out->members.begin(),out->members.end(),[&](const auto &m){return m.index==member.index;});
      if(known!=out->members.end()){if(known->identity!=member.identity)Fail("root_identity_conflict");}
      else {out->members.push_back(member);indices->Retain(i->second);}
    }
    std::sort(out->retained_members.begin(),out->retained_members.end(),[](const auto &a,const auto &b){return a.member<b.member;});
    std::sort(out->members.begin(),out->members.end(),[](const auto &a,const auto &b){return a.index<b.index;});
    if(same_scope&&!out->retained_members.empty()){
      std::optional<FrameContext> prior_context;
      for(const auto &old:prior->records){
        const auto root=std::lower_bound(out->retained_members.begin(),out->retained_members.end(),old->member.index,
          [](const auto &entry,auto index){return entry.member<index;});
        if(root!=out->retained_members.end()&&root->member==old->member.index)
          out->records.push_back(RetainRecord(old,*prior,*out,prior_context,s.scratch));
      }
    }
    const bool member_changed=new_run||prior->members!=out->members;
    if(!new_run&&member_changed&&prior->member_epoch==UINT32_MAX)Fail("member_epoch_exhausted");
    out->member_epoch=new_run?1:prior->member_epoch+unsigned(member_changed);
    out->resources.assign(resources.begin(),resources.end());
    const bool resource_changed=new_run||prior->resources.size()!=resources.size();
    if(!new_run){if(resources.size()<prior->resources.size())Fail("resource_directory_rewound");
      for(std::size_t i=0;i<prior->resources.size();++i){const auto &a=prior->resources[i],&b=resources[i];if(a.handle!=b.handle||a.key!=b.key||a.kind!=b.kind||!Same(a.payload->bytes,b.payload->bytes))Fail("resource_identity_rebound");}}
    if(!new_run&&resource_changed&&prior->resource_epoch==UINT32_MAX)Fail("resource_epoch_exhausted");
    out->resource_epoch=new_run?1:prior->resource_epoch+unsigned(resource_changed);
  }
  FrameContext context(*out);
  // 按成员号和流定位上一帧与当前候选，平铺采集无需逐根扫描两个快照。
  using RecordRef=std::shared_ptr<const HostStateRecord>;
  struct RecordSlot {const RecordRef *prior=nullptr;std::size_t output=SIZE_MAX;};
  std::vector<std::array<RecordSlot,2>> record_slots;
  memory::Lease slot_memory;
  if(flat_contract && (!out->members.empty() || (prior&&!prior->members.empty()))){
    const auto hi=(std::max)(out->members.empty()?0:out->members.back().index,prior&& !prior->members.empty()?prior->members.back().index:0);
    slot_memory=memory::Reserve(memory::Pool::Workspace,(std::size_t(hi)+1)*sizeof(record_slots[0]),"host_flat_capture_record_slots");
    record_slots.resize(std::size_t(hi)+1);
    slot_memory->Resize(record_slots.capacity()*sizeof(record_slots[0]));
    if(!new_run)for(const auto &record:prior->records)
      record_slots[record->member.index][unsigned(record->stream)].prior=&record;
    for(std::size_t i=0;i<out->records.size();++i){
      const auto &record=out->records[i];
      record_slots[record->member.index][unsigned(record->stream)].output=i;
    }
  }
  const auto store=[&](RecordRef record){
    if(!record_slots.empty()){
      auto &slot=record_slots[record->member.index][unsigned(record->stream)].output;
      if(slot==SIZE_MAX){slot=out->records.size();out->records.push_back(std::move(record));}
      else out->records[slot]=std::move(record);
      return;
    }
    const auto found=std::find_if(out->records.begin(),out->records.end(),[&](const auto &entry){
      return entry->member.index==record->member.index&&entry->stream==record->stream;
    });
    if(found==out->records.end())out->records.push_back(std::move(record));else *found=std::move(record);
  };
  // 只按当前包的身份目录查一次。正文与平铺补丁都用直接编号表，保留沿用与不存在的区别。
  FrameMemberLookup member_lookup(out->members.size()+packet.records.size());
  for(const auto &entry:out->members)member_lookup.Add(Identity(entry.identity),entry.index);
  for(const auto &entry:packet.records)if(entry.retained)member_lookup.Add(Identity(entry.subject),kNullIndex);
  const auto refs_memory=memory::Reserve(memory::Pool::Workspace,packet.references.size()*sizeof(std::uint32_t),"host_frame_reference_indices");
  std::vector<std::uint32_t> reference_indices;
  reference_indices.reserve(packet.references.size());
  for(const auto &ref:packet.references)reference_indices.push_back(member_lookup.Find({ref.space,ref.local,ref.use}));
  refs_memory->Resize(reference_indices.capacity()*sizeof(std::uint32_t));
  // 解析器只在编码器实际访问链接字段时登记，无 noexcept 回调内分配。
  struct Resolve {
    std::span<const std::uint32_t> reference_indices;
    std::span<const HostStateResource> resources;
    std::array<std::uint16_t,kHostMemberIndices> refs{};
    std::bitset<kHostMemberIndices> seen;
    std::size_t count=0;
    std::uint32_t resource_prefix=0;
    bool collect=true,missing_reference=false;
  } resolve{reference_indices,out->resources};
  CaptureLinkResolver resolver{&resolve,[](void *p,CaptureSubject,bool resource,std::uint8_t table,CaptureLink link,std::uint16_t &value) noexcept {
    auto &r=*static_cast<Resolve *>(p);if(link.kind==CapturedReferenceKind::Null){if(link.handle)return false;value=kNullIndex;return true;}
    if((link.kind!=CapturedReferenceKind::Identity&&link.kind!=CapturedReferenceKind::Pending)||!link.handle)return false;
    if(resource){
      if(link.handle>r.resources.size())return false;
      const auto &entry=r.resources[std::size_t(link.handle-1)];
      if(entry.handle!=link.handle || entry.key.table!=table)return false;
      value=entry.key.index;
      if(r.collect)r.resource_prefix=(std::max)(r.resource_prefix,std::uint32_t(link.handle));
      return true;
    }
    if(link.handle>r.reference_indices.size())return false;
    const auto translated=r.reference_indices[std::size_t(link.handle-1)];
    if(translated<kNullIndex){
      value=std::uint16_t(translated);
      if(r.collect&&!r.seen[value]){r.seen[value]=true;r.refs[r.count++]=value;}
      return true;
    }
    // 首次出现但布局尚未确认的沿用成员没有线上编号。只由字段编码器决定能否用空引用。
    if(translated==kNullIndex){
      r.missing_reference=true;
      value=kNullIndex;
      return true;
    }
    return false;},true};
  bool has_body=false;
  std::optional<FrameContext> reuse_context;
  for(std::uint32_t i=0;i<packet.records.size();++i){const auto &r=packet.records[i];if(r.event_sequence||r.retained)continue;
    if(r.layout==195)continue; // 纯身份锚点没有正文，不伪造零长状态记录。
    const auto member_index=member_lookup.Find(Identity(r.subject));
    const auto *member=member_index<kNullIndex?MemberAt(*out,std::uint16_t(member_index)):nullptr;
    if(!member||member->identity.shape!=WireShape(r.subject)||member->identity.layout!=r.layout){
      if(!ordinary)continue;
      Fail("world_member_changed");
    }
    const auto stream=r.stream==BodyStream::Value?BodyStream::World:r.stream;
    const auto *flat=FlatRecordLayout(LayoutVersion(),r.layout);
    if(bool(flat)!=bool(r.flat_bytes))Fail("capture_record_format_mismatch");
    if(r.reusable_base){
      if(!ordinary||stream!=BodyStream::World||r.layout!=host_field::EntityTear::kLayout||!s.ordinary||
         s.ordinary->ordinary.revision!=r.reusable_base||s.ordinary->ordinary.scope!=packet.witness.scope||
         !std::binary_search(s.ordinary->reusable_roots.begin(),s.ordinary->reusable_roots.end(),member->identity))
        Fail("host_root_reuse_base_missing");
      const auto old=std::find_if(s.ordinary->records.begin(),s.ordinary->records.end(),[&](const auto &entry){
        return entry->member.identity==member->identity&&entry->stream==stream;
      });
      if(old==s.ordinary->records.end())Fail("host_root_reuse_record_missing");
      const auto reused=RetainRecord(*old,*s.ordinary,*out,reuse_context,s.scratch);
      store(reused);
      if(reused==*old)out->reusable_roots.push_back(member->identity);
      continue;
    }
    if(!ordinary&&r.layout==197){const auto world=std::find_if(prior->records.begin(),prior->records.end(),[&](const auto &entry){return entry->member.index==member->index&&entry->stream==BodyStream::World;});
      if(world==prior->records.end()||(*world)->topology!=r.subject.topology||(*world)->binding!=r.subject.binding||
         std::any_of(prior->retained_members.begin(),prior->retained_members.end(),[&](const auto &root){return root.member==member->index;}))continue;}
    resolve.count=0;resolve.resource_prefix=0;resolve.missing_reference=false;
    auto record=std::make_shared<HostStateRecord>();record->member=*member;record->stream=stream;record->topology=r.subject.topology;record->binding=r.subject.binding;
    std::shared_ptr<const HostStateRecord> old;
    if(!record_slots.empty()){
      const auto *entry=record_slots[member->index][unsigned(stream)].prior;
      if(entry&&(*entry)->member.identity==member->identity)old=*entry;
    }else if(!new_run&&prior)
      for(const auto &entry:prior->records)if(entry->member.identity==member->identity&&entry->stream==stream){old=entry;break;}
    if(flat){
      CaptureDiagnostic diagnostic;
      s.flat_scratch.Prepare(*flat);
      auto output=std::span(s.scratch.output);
      const auto padding=(32u-(reinterpret_cast<std::uintptr_t>(output.data())&31u))&31u;
      output=output.subspan(padding);
      {
        CaptureSample encoding(timing,HostCapturePhase::Encode);
        if(TranslateFlatCapture(packet,r,resolver,output,s.flat_scratch.tails,diagnostic)){
          const bool previous_valid=same_scope&&old&&!(stream==BodyStream::Body&&s.body_comparison_reset);
          record->flat=UpdateFlatImage(*flat,output.first(r.flat_bytes),s.flat_scratch.tails,
            previous_valid?old->flat:HostFlatImageRef{},out->flat_versions,s.flat_scratch,
            {r.subject,0,0,r.layout},diagnostic);
          if(!record->flat && diagnostic.reason==CaptureFailure::InvalidValue)
            s.flat_padding_failures.fetch_add(1,std::memory_order_relaxed);
        }
      }
      if(!record->flat){
        for(std::size_t j=0;j<resolve.count;++j)resolve.seen[resolve.refs[j]]=false;
        if(ordinary){
          MarkRetained(*out,same_scope?prior.get():nullptr,r,member->index,diagnostic.reason);
          if(same_scope&&old)store(RetainRecord(old,*prior,*out,reuse_context,s.scratch));
        }
        continue;
      }
    }else{
      const auto source=[&]{CaptureSample indexing(timing,HostCapturePhase::Index);
        return CapturedBodySource(packet,i,resolver,s.scratch.links,s.index);}();
      auto scratch=s.scratch.Body();
      const auto body=[&]{CaptureSample encoding(timing,HostCapturePhase::Encode);
        return EncodeBodyInto(r.layout,r.mask,stream,source,s.scratch.output,scratch,&context.resource_context,Limits());}();
      record->projections.assign(body.facts.begin(),body.facts.end());
      for(const auto &block:std::span(body.blocks).first(body.block_count)){
        HostStateBlock next{block.block,packet.witness.revision,{}};
        if(old)for(const auto &b:old->blocks)if(b.block==block.block&&Same(b.payload->bytes,block.bytes)){next=b;break;}
        if(!next.payload)next.payload=Own(block.bytes);
        record->blocks.push_back(std::move(next));
      }
    }
    if(!ordinary)has_body=true;
    if(r.layout==host_field::PlayerControlGroup::kLayout) {
      // 控制组由同帧的 Player 结构共同构成。Flip 的新锚点未就绪时跳过这一帧，
      // 避免实体沿用逻辑把必需的锚点变为空引用，或让它指向上一绑定的身体状态。
      const auto pending=[&](std::uint16_t index){
        return std::any_of(out->retained_members.begin(),out->retained_members.end(),
          [&](const auto &root){return root.member==index;});
      };
      if(resolve.missing_reference||std::any_of(resolve.refs.begin(),resolve.refs.begin()+resolve.count,pending))
        Fail("host_control_anchor_pending");
    }
    std::sort(resolve.refs.begin(),resolve.refs.begin()+resolve.count);
    record->member_refs.assign(resolve.refs.begin(),resolve.refs.begin()+resolve.count);
    for(const auto index:record->member_refs)resolve.seen[index]=false;
    record->resource_prefix=resolve.resource_prefix;
    if(flat && old && record->flat==old->flat && !old->flat_suspended && record->member==old->member &&
       record->topology==old->topology && record->binding==old->binding &&
       record->resource_prefix==old->resource_prefix && record->member_refs==old->member_refs){
      store(old);
      continue;
    }
    if(flat)record->flat_receipt=FlatRecordReceipt(*record);
    // P6／P7结构变化不必升级member shape；仍须先共同提交同拓扑world与body。
    if(ordinary&&r.layout==197&&stream==BodyStream::World&&(!old||old->topology!=record->topology||old->binding!=record->binding))out->structure_revision=packet.witness.revision;
    record->memory=memory::Reserve(memory::Pool::Values,sizeof(HostStateRecord)+record->blocks.capacity()*sizeof(HostStateBlock)+record->projections.capacity()*sizeof(ProjectionFact)+record->member_refs.capacity()*sizeof(std::uint16_t)+128,"host_state_record");
    // 一根的所有物理块由采集计划一次提供；不能把缺块当成已退休。
    if(!flat&&ordinary&&stream==BodyStream::World&&r.layout==host_field::EntityTear::kLayout&&!resolve.missing_reference)
      out->reusable_roots.push_back(member->identity);
    store(std::move(record));
  }
  if(!ordinary&&!has_body)return {};
  if(ordinary){
    std::sort(out->retained_members.begin(),out->retained_members.end(),[](const auto &a,const auto &b){return a.member<b.member;});
    std::sort(out->reusable_roots.begin(),out->reusable_roots.end());
    resolve.collect=false;
    StateEventQueue events;events.Reset(packet.event_first_sequence?packet.event_first_sequence-1:0);auto scratch=s.scratch.Body();
    if(!QueueCapturedStateEvents(packet,events,resolver,s.scratch.links,s.scratch.output,scratch,&context.resource_context,Limits(),s.index))Fail("event_handoff_gap");
    std::array<StateEventView,512> selected{};const auto segment=events.Select(selected,256u<<10);
    out->event_first=segment.first_sequence;out->event_watermark=segment.watermark;
    for(const auto &e:segment.events){HostStateEvent event{e,Own(e.payload)};event.value.payload=event.payload->bytes;out->events.push_back(std::move(event));}
    out->member_indices=std::move(indices);
  }
  if(flat_contract)out->flat_receipt=FlatReceipt(*out);
  out->memory->Resize(SnapshotBytes(*out));
  if(current&&!current(qualification))Fail("capture_revoked_during_encoding");
  if(ordinary){
    s.memory->Resize(ids.size()*96+free_indices.capacity()*sizeof(std::uint16_t));
    s.identities=std::move(ids);s.free_indices=std::move(free_indices);
    s.index_high_water.store(high_water,std::memory_order_relaxed);s.index_reused.store(reused,std::memory_order_relaxed);
    s.ordinary=out;
  }
  s.flat_clock=out->flat_versions;s.body_comparison_reset=false;
  s.latest=out;return out;
}

namespace {
// 自身和每条真实成员引用必须在相同号码仍代表相同完整身份。资源目录按实际来源前缀追加。
bool RecordCompatible(const HostStateRecord &record,const HostStateSnapshot &from,const HostStateSnapshot &to) noexcept {
  if(record.flat_suspended||record.resource_prefix>to.resources.size())return false;
  if(from.member_epoch==to.member_epoch)return true;
  const auto *root=MemberAt(to,record.member.index);
  if(!root||root->identity!=record.member.identity)return false;
  for(const auto index:record.member_refs){
    const auto *a=MemberAt(from,index),*b=MemberAt(to,index);
    if(!a||!b||a->identity!=b->identity)return false;
  }
  return true;
}
// 资源目录在可靠线上独立推进。普通帧按 Peer 已收到的目录投影，收到 ACK 后保留实际发出的纯值。
HostStateSnapshotRef ProjectResourcePrefix(const HostStateSnapshotRef &source,const HostStateSnapshot &base,
    std::uint32_t epoch,std::size_t count) {
  if(!epoch||count>source->resources.size())return {};
  if(source->resource_epoch<=epoch)return source;
  auto projected=memory::DeferredValue<HostStateSnapshot>();
  CopyFrameDirectory(*projected,*source);
  projected->resource_epoch=epoch;
  projected->resources.assign(source->resources.begin(),source->resources.begin()+count);
  projected->records=source->records;
  const auto extent=source->members.empty()?0u:unsigned(source->members.back().index)+1;
  using RecordRef=std::shared_ptr<const HostStateRecord>;
  auto workspace=memory::Reserve(memory::Pool::Workspace,
    std::uint64_t(extent)*(2*sizeof(RecordRef)+sizeof(bool)),"host_resource_projection");
  std::array<std::vector<RecordRef>,2> previous;
  for(auto &stream:previous)stream.resize(extent);
  for(const auto &record:base.records)if(record->member.index<extent)
    previous[unsigned(record->stream)][record->member.index]=record;
  std::vector<bool> has_world(extent);
  const auto retain=[&](RecordRef &record){
    // 与客机本地资源等待一致，仅实体和 Grid 可逐件沿用；结构 World 等完整资源目录。
    const auto space=record->member.identity.space;
    if(space!=1&&space!=2)return false;
    const auto member=record->member.index;
    if(std::none_of(projected->retained_members.begin(),projected->retained_members.end(),
        [&](const auto &root){return root.member==member;}))
      projected->retained_members.push_back({member,0,CaptureFailure::ResourcePending});
    const auto &old=previous[unsigned(record->stream)][member];
    if(old&&old->member.identity==record->member.identity){
      if(!RecordCompatible(*old,base,*projected))return false;
      record=old;
    }else record.reset();
    return true;
  };
  for(auto &record:projected->records){
    if(record->resource_prefix>count&&!retain(record))return {};
    if(record&&record->stream==BodyStream::World)has_world[record->member.index]=true;
  }
  // 新成员的目录身份正常交接，等可用资源后才补它的值；暂时沿用指向它的根。
  const auto available=[&](const auto &record){
    for(const auto number:record.member_refs){
      const auto *member=MemberAt(*projected,number);
      if(!member||(member->identity.layout!=195&&!has_world[number]))return false;
    }
    return true;
  };
  bool removed=false;
  do {
    removed=false;
    for(auto &record:projected->records)if(record&&!available(*record)){
      const auto member=record->member.index;
      const bool world=record->stream==BodyStream::World;
      if(!retain(record)||(record&&!available(*record)))return {};
      if(!record&&world){has_world[member]=false;removed=true;}
    }
  }while(removed);
  std::erase_if(projected->records,[](const auto &record){return !record;});
  std::sort(projected->retained_members.begin(),projected->retained_members.end(),
    [](const auto &a,const auto &b){return a.member<b.member;});
  projected->flat_receipt=FlatReceipt(*projected);
  projected->memory=memory::Reserve(memory::Pool::Values,SnapshotBytes(*projected),"host_resource_projected_snapshot");
  return projected;
}
// 两张已排序目录的差只含线上已有的 Join／Reshape／Leave。相同身份键的换号或
// 编号直接换身份无法用一份增量表达，交回完整目录，保持现有合并语义。
bool MemberDelta(std::span<const StateMemberEntry> before,std::span<const StateMemberEntry> after,
    std::vector<StateMemberEntry> &changes,std::vector<std::uint16_t> &order){
  std::size_t a=0,b=0;
  while(a<before.size()||b<after.size()){
    if(b==after.size()||(a<before.size()&&before[a].index<after[b].index)){
      auto entry=before[a++];entry.operation=MemberOperation::Leave;changes.push_back(entry);
    }else if(a==before.size()||after[b].index<before[a].index){
      changes.push_back(after[b++]);
    }else {
      const auto &old=before[a++].identity;
      auto entry=after[b++];
      const auto &id=entry.identity;
      if(old==id)continue;
      if(Identity(old)!=Identity(id)||id.shape<=old.shape)return false;
      entry.operation=MemberOperation::Reshape;changes.push_back(entry);
    }
  }
  order.resize(changes.size());
  for(std::size_t i=0;i<order.size();++i)order[i]=std::uint16_t(i);
  const auto key=[&](std::uint16_t i){const auto &id=changes[i].identity;return std::pair(id.space,id.id);};
  std::sort(order.begin(),order.end(),[&](auto x,auto y){return key(x)<key(y);});
  return std::adjacent_find(order.begin(),order.end(),[&](auto x,auto y){return key(x)==key(y);})==order.end();
}
}
struct HostStatePublisher::Impl {
  struct SentOwner {std::shared_ptr<const HostStateMessage> value;std::uint64_t sent_ms=0;
    const HostStateMessage *operator->() const noexcept{return value.get();}};
  memory::Lease memory=memory::Reserve(memory::Pool::Values,sizeof(Impl)+192*sizeof(SentOwner),"host_state_peer");
  Scratch scratch;
  FlatEncodeScratch flat_scratch;
  StateRoute route;
  HostStateSnapshotRef source,ordinary_source;
  // ACK 移走发送账本；实际已确认的两流来源与编码所用目录继续独立保留。
  struct ReceivedFrame {
    HostStateSnapshotRef values,tables;
  };
  std::array<ReceivedFrame,2> received_frames;
  StateEventQueue events;
  std::array<StateTarget,192> ledger{};
  std::unique_ptr<HostStateAcknowledgements> acknowledgements;
  std::vector<SentOwner> owners;
  // 可靠目录槽交接后，与动态帧候选独立；同一时刻只交接一张表。
  std::shared_ptr<const HostStateMessage> pending,table_inflight;
  std::array<std::uint64_t,2> confirmed_sources{},offered_sources{};
  std::array<std::uint32_t,2> offered_resource_epochs{};
  // 同一线路上的持续事件预算不足沿用普通 World 的既有陈旧期限。
  std::optional<std::uint64_t> world_event_budget_since;
  HostWorldProgress world_progress;
  std::array<std::uint32_t,2> targets{};
  std::size_t resource_count=0;
  std::uint32_t resource_base=0;
  std::uint64_t body_dependency_blocks=0;
  std::array<std::uint32_t,2> recovery_history{};
  bool open=false,first=true,rebuild=false,awaiting_joint=false;
  unsigned last_stream=0;
  std::uint64_t event_seen=0;
  std::uint64_t capture_session=0,forced_baseline_revision=0;
  Impl(){owners.reserve(ledger.size());}
  // 两流各有帧数与字节额度。超龄凭据仍可确认，直到 ACK 或账本回收。
  std::pair<std::size_t,std::uint64_t> Outstanding(unsigned stream,std::uint64_t now) const noexcept {
    std::pair<std::size_t,std::uint64_t> usage{};
    for(const auto &owner:owners)if(owner->lane!=StateLane::Baseline&&
        unsigned(owner->target.sections[0].stream)==stream&&now>=owner.sent_ms&&
        now-owner.sent_ms<StateBudgets().sent_age_ms){
      ++usage.first;usage.second+=owner->payload->bytes.size();
    }
    return usage;
  }
  void Collect(){owners.erase(std::remove_if(owners.begin(),owners.end(),[&](const auto &owner){return std::none_of(ledger.begin(),ledger.end(),[&](const auto &target){return target.count&&target.encoded.data()==owner->target.encoded.data();});}),owners.end());}
};
HostStatePublication::HostStatePublication():memory_(memory::Reserve(memory::Pool::Values,sizeof(*this)+128,"host_publication_mailbox")){}
HostStatePublication::View HostStatePublication::Read() const noexcept {
  std::unique_lock lock(mutex_,std::try_to_lock);
  if(!lock||!published_.ordinary||published_.ordinary->ordinary.revision<floor_)return {};
  return published_;
}
void HostStatePublication::Publish(const HostStateSnapshotRef &snapshot,bool ordinary) noexcept {
  View retired;
  {
    std::lock_guard lock(mutex_);
    if(snapshot->ordinary.revision<floor_)return;
    retired.latest=std::exchange(published_.latest,snapshot);
    if(ordinary)retired.ordinary=std::exchange(published_.ordinary,snapshot);
  }
  // 已替换快照在锁外释放，撤销水位不会等待大批规范块析构。
}
void HostStatePublication::InvalidateBefore(std::uint64_t revision) noexcept {
  std::lock_guard lock(mutex_);
  floor_=(std::max)(floor_,revision);
}
void HostStatePublication::DiscardBody() noexcept {
  HostStateSnapshotRef retired;
  {std::lock_guard lock(mutex_);retired=std::exchange(published_.latest,published_.ordinary);}
}
void HostStatePublication::Clear() noexcept {
  View retired;
  {std::lock_guard lock(mutex_);retired=std::exchange(published_,{});}
}
HostStatePublisher::HostStatePublisher():impl_(std::make_unique<Impl>()){}
HostStatePublisher::~HostStatePublisher()=default;
void HostStatePublisher::Stop() noexcept {auto &s=*impl_;s.pending.reset();s.table_inflight.reset();s.owners.clear();s.acknowledgements.reset();s.source.reset();s.ordinary_source.reset();s.received_frames={};s.ledger={};s.resource_base=0;s.resource_count=0;s.open=false;s.events.Reset();}
void HostStatePublisher::Open(const StateRoute &route){Stop();auto &s=*impl_;s.route=route;s.targets={};s.confirmed_sources={};s.offered_sources={};s.offered_resource_epochs={};s.first=true;s.rebuild=false;s.awaiting_joint=false;
  s.event_seen=0;s.capture_session=0;s.forced_baseline_revision=0;s.last_stream=0;s.world_event_budget_since.reset();
  s.recovery_history={};s.acknowledgements=std::make_unique<HostStateAcknowledgements>(route,s.ledger,&s.events);s.open=true;}
bool HostStatePublisher::Offer(HostStateSnapshotRef source){auto &s=*impl_;if(!s.open||!source)return false;
  if(source==s.source||source==s.ordinary_source)return true;
  // 采集会话属于独立观测owner；线上session必须使用实际C/S握手身份，其余scope逐项相等。
  auto scope=source->witness.scope;const auto capture_session=scope.session;scope.session=s.route.scope.session;
  if(scope!=s.route.scope||(s.capture_session&&s.capture_session!=capture_session))return false;s.capture_session=capture_session;
  if(source->witness.phase==CapturePhase::Ordinary){
    if(s.first){s.events.Reset(source->event_first-1);s.first=false;}
    // 已覆盖批次可重复Offer；未确认事件始终由本Peer队列保留，不能随body/latest淘汰。
    if(source->event_watermark>s.event_seen&&!s.events.CanCover(source->event_first,source->event_watermark))return false;
    // 相同范围也比较未确认事件的精确正文，不能只凭水位相等接受冲突重传。
    for(const auto &e:source->events)if(!s.events.Offer(e.value,source->member_indices))return false;
    if(source->event_watermark>s.event_seen){
      if(!s.events.Cover(source->event_first,source->event_watermark))return false;s.event_seen=source->event_watermark;}
    s.events.Expire(source->witness.game_frame);
    if(!s.ordinary_source||source->witness.revision>s.ordinary_source->witness.revision)s.ordinary_source=source;
  }
  if(!s.source||source->witness.revision>s.source->witness.revision)s.source=std::move(source);return true;
}
std::shared_ptr<const HostStateMessage> HostStatePublisher::Next(std::uint64_t now_ms,std::uint64_t,
    std::uint8_t dynamic_streams,HostWorldDeferral &world_deferral){
  world_deferral=HostWorldDeferral::None;
  auto &s=*impl_;if(!s.open||!s.source||!s.ordinary_source)return {};if(s.pending)return s.pending;
  ScratchJob job{s.scratch};s.scratch.Ensure(std::size_t(StateBudgets().prepare_wire_bytes));
  const auto confirmed=s.acknowledgements->Confirmed();
  auto message=std::make_shared<HostStateMessage>();
  const bool joint=s.rebuild||!confirmed[0].target||!confirmed[1].target;
  const bool full=joint;
  // 资源目录仍独立可靠传输。成员目录随状态帧一起解码。
  if(!s.table_inflight&&s.ordinary_source->resource_epoch>s.resource_base){
    const auto &source=*s.ordinary_source;
    if(s.resource_count>source.resources.size())Fail("resource_table_base_rewound");
    const auto additions=source.resources.size()-s.resource_count;
    auto workspace=memory::Reserve(memory::Pool::Workspace,additions*sizeof(ResourceEntryView),"host_resource_table_encode");
    std::vector<ResourceEntryView> resources;resources.reserve(additions);
    workspace->Resize(resources.capacity()*sizeof(ResourceEntryView));
    const auto directory=Directory(source,resources,s.resource_count);
    ResourceCodecScratch scratch{s.scratch.Body()};
    const auto encoded=EncodeResourceTableInto({s.route.scope,LayoutVersion(),source.resource_epoch,s.resource_base,
      directory.entries},s.scratch.output,scratch);
    message->source=source.flat_receipt?source.flat_receipt:s.ordinary_source;message->kind=StateMessageKind::Resources;
    message->lane=StateLane::Context;message->payload=Own(encoded);
    return s.pending=message;
  }
  unsigned stream=0;
  if(joint){
    if(s.awaiting_joint||s.resource_base!=s.ordinary_source->resource_epoch)return {};
    message->source=s.ordinary_source;
  }else{
    const auto world_usage=s.Outstanding(0,now_ms),body_usage=s.Outstanding(1,now_ms);
    const auto has_slot=[](const auto &usage){
      return usage.first<StateBudgets().world_send_window&&usage.second<StateBudgets().dynamic_send_bytes;
    };
    // 遇到丢包仍可发送最新来源，目标号总是新分配，差分只依赖接收 ACK。
    const auto resource_advanced=[&](unsigned stream,const HostStateSnapshot &source){
      return (std::min)(source.resource_epoch,s.resource_base)>s.offered_resource_epochs[stream];
    };
    const bool world_fresh=s.ordinary_source->ordinary.revision>s.offered_sources[0]||
      resource_advanced(0,*s.ordinary_source)||
      (s.resource_base==s.ordinary_source->resource_epoch&&s.events.SentWatermark()<s.ordinary_source->event_watermark);
    const bool world=(dynamic_streams&1)&&world_fresh&&has_slot(world_usage);
    const bool body=(dynamic_streams&2)&&has_slot(body_usage)&&
      (s.source->witness.revision>s.offered_sources[1]||resource_advanced(1,*s.source));
    if((dynamic_streams&1)&&world_fresh&&!has_slot(world_usage)){
      world_deferral=HostWorldDeferral::WindowFull;++s.world_progress.window_skips;
    }
    if(!world&&!body)return {};
    stream=body&&(!world||s.last_stream==0)?1:0;
    message->source=stream?s.source:s.ordinary_source;
  }
  const bool deferring_resources=!joint&&message->source->resource_epoch>s.resource_base;
  if(deferring_resources){
    const auto &received=s.received_frames[stream].values;
    if(!received)return {};
    message->source=ProjectResourcePrefix(message->source,*received,s.resource_base,s.resource_count);
    if(!message->source)return {};
  }
  const auto &snapshot=*message->source;
  message->tables=message->source;
  const auto &tables=*message->tables;
  // 普通目标保留恢复槽，正文与来源快照只在ACK或明确退休后回收。
  if(s.acknowledgements->FreeSlots()<=(joint?0:StateBudgets().sent_recovery_slots))return {};
  FrameContext context(snapshot,tables);
  // 旧引用的语义变化可能不改变数字字节。换表时把受影响记录的消费块显式随帧带齐。
  memory::Lease prior_memory;
  std::array<std::vector<const HostStateRecord *>,2> prior;
  const auto first=joint?0u:stream,last=joint?1u:stream;
  for(auto n=first;n<=last&&!full;++n){
    const auto &before=s.received_frames[n].values->members;
    const auto max_index=(std::max)(before.empty()?0:before.back().index,tables.members.empty()?0:tables.members.back().index);
    const auto bytes=(std::size_t(max_index)+1)*sizeof(const HostStateRecord *);
    if(!prior_memory)prior_memory=memory::Reserve(memory::Pool::Workspace,bytes,"host_frame_prior_members");
    else prior_memory->Resize(prior_memory->Bytes()+bytes);
    prior[n].resize(std::size_t(max_index)+1);
    for(const auto &record:s.received_frames[n].values->records)
      if(unsigned(record->stream)==n)prior[n][record->member.index]=record.get();
  }
  memory::Lease flat_payload_memory;
  std::vector<std::vector<std::byte>> payloads;payloads.reserve(snapshot.records.size());
  std::array<std::vector<StateRecordPayload>,2> records;
  for(const auto &record:snapshot.records){
    const auto n=unsigned(record->stream);if(!joint&&n!=stream)continue;std::uint32_t mask=0;
    // 联合结构路径显式保留完整控制组与Player配对投影，不用另一流旧姿态补P0/P6/P7。
    const bool paired_player=joint&&(record->member.identity.layout==196||record->member.identity.layout==197);
    const auto *old=prior[n].empty()?nullptr:prior[n][record->member.index];
    const bool rebound=!full&&(!old||old->member.identity!=record->member.identity||
      !RecordCompatible(*old,*s.received_frames[n].values,tables));
    if(const auto *flat=FlatRecordLayout(LayoutVersion(),record->member.identity.layout)){
      if(!record->flat || record->flat_suspended)continue;
      const auto *ack=full?nullptr:s.received_frames[n].values.get();
      // ACK 已越过沿用或资源延后的帧时，源版本不是这个成员实际收到的版本。
      const bool all=full||paired_player||rebound||(ack&&Retains(*ack,record->member.index));
      auto data=EncodeFlatRecord(*flat,*record->flat,FlatStream(n),ack?ack->flat_versions[n]:0,all,s.flat_scratch,flat_payload_memory);
      if(!data)continue;
      payloads.push_back(std::move(data->bytes));
      records[n].push_back({record->member.index,record->member.identity.layout,0,payloads.back(),data->mode});
      continue;
    }
    if(!full&&!rebound&&old==record.get())continue;
    // Peer 曾沿用某个根时，源采集 revision 已前进，仍须用实际收到的旧块求差。
    for(const auto &b:record->blocks){
      if(full||paired_player||rebound||std::none_of(old->blocks.begin(),old->blocks.end(),
          [&](const auto &before){return before.block==b.block&&before.payload==b.payload;}))
        mask|=1u<<b.block;
    }
    if(!mask)continue;payloads.push_back(Join(*record,mask));records[n].push_back({record->member.index,record->member.identity.layout,mask,payloads.back()});
  }
  // 优先保留紧凑编号；变动目录相对该流已确认的快照编码，冲突换号时使用完整目录。
  std::span<const StateMemberEntry> entries=tables.members;
  std::vector<StateMemberEntry> changes;
  std::vector<std::uint16_t> order;
  std::vector<std::byte> member_update;
  const auto *base=full?nullptr:s.received_frames[stream].tables.get();
  if(!base||base->member_epoch!=tables.member_epoch){
    const auto capacity=(base?base->members.size():0)+entries.size();
    auto memory=cs::memory::Reserve(cs::memory::Pool::Workspace,
      capacity*(sizeof(StateMemberEntry)+sizeof(std::uint16_t)+32)+128,"host_frame_members");
    std::uint32_t previous=0;
    if(base&&MemberDelta(base->members,entries,changes,order)){entries=changes;previous=base->member_epoch;}
    order.resize(entries.size());
    const auto encoded=EncodeStateMemberTableInto({s.route.scope,tables.member_epoch,previous,entries},s.scratch.output,order);
    member_update.assign(encoded.begin(),encoded.end());
  }
  std::vector<std::byte> retained;
  retained.reserve(snapshot.retained_members.size()*2);
  for(const auto &root:snapshot.retained_members){
    if(root.member==kNullIndex)continue;
    retained.push_back(std::byte(root.member&0xff));retained.push_back(std::byte(root.member>>8));
  }
  std::uint64_t fixed=52+(joint?104:52)+member_update.size()+retained.size()*unsigned(joint?2:1);
  for(unsigned n=0;n<2;++n)for(const auto &r:records[n])fixed+=StateRecordHeaderBytes(r)+r.bytes.size();
  const auto limit=StateMessageBodyLimit(joint?StateLane::Baseline:StateLane::World);
  const auto recover=[&]{return Recover(s.route,{s.route.scope,3,StateResyncReason::Budget,confirmed[0].target,confirmed[1].target});};
  if(fixed+(joint||stream==0?StateEventQueueHeaderBytes():0)>limit){
    if(!joint){
      if(stream)s.offered_sources[1]=snapshot.witness.revision; // 超限拍只发 World。
      else if(!s.table_inflight)recover();
      return {};
    }
    Fail("baseline_record_budget");
  }
  std::array<StateEventView,512> event_storage{};
  const auto event_limit=deferring_resources?(std::min)(snapshot.event_watermark,s.events.SentWatermark()):snapshot.event_watermark;
  const auto segment=s.events.Select(event_storage,std::uint32_t(std::min<std::uint64_t>(256u<<10,limit-fixed)),event_limit);
  if(!joint&&stream==0){
    // 选段仍从已确认水位开始。带宽截断若会使目标水位倒退，跳过当前机会，保持队列和账本。
    if(segment.watermark<s.events.SentWatermark()){
      world_deferral=HostWorldDeferral::EventBudget;
      if(!s.world_event_budget_since)s.world_event_budget_since=now_ms;
      const auto blocked_since=*s.world_event_budget_since;
      if(!s.table_inflight&&now_ms>=blocked_since&&now_ms-blocked_since>=StateBudgets().world_stale_ms)recover();
      return {};
    }
    s.world_event_budget_since.reset();
    if(records[0].empty()&&segment.events.empty()&&segment.watermark==confirmed[0].event_watermark&&event_limit>segment.watermark){
      if(!s.table_inflight)recover();return {};
    }
  }
  StateFrameView frame;frame.scope=s.route.scope;frame.layout_version=LayoutVersion();frame.section_count=joint?2:1;
  auto targets=s.targets;
  for(unsigned i=0;i<frame.section_count;++i){const auto n=joint?i:stream;if(targets[n]==UINT32_MAX)Fail("target_sequence_exhausted");++targets[n];}
  for(unsigned i=0;i<frame.section_count;++i){const auto n=joint?i:stream;auto &section=frame.sections[i];auto &c=section.context;
    c.scope=s.route.scope;c.layout_version=LayoutVersion();c.stream=BodyStream(n);c.member_epoch=tables.member_epoch;c.resource_epoch=tables.resource_epoch;
    c.base_version=full?0:confirmed[n].target;c.target_version=targets[n];
    const auto &w=joint?snapshot.ordinary:n?snapshot.witness:snapshot.ordinary;c.source_revision=w.revision;c.presentation_batch=w.batch;c.game_frame=w.game_frame;
    if(joint)c.paired_targets=targets;section.records=records[n];section.retained_members=retained;
    if(!n){c.installed_event_watermark=confirmed[0].event_watermark;c.event_watermark=segment.watermark;section.presentation.events=segment.events;section.presentation.first_sequence=segment.first_sequence;}
  }
  frame.member_update=member_update;
  order.resize(tables.members.size());StateFrameScratch fs;fs.member_order=order;fs.body=s.scratch.Body();
  s.scratch.Ensure(std::size_t(fixed+(joint||stream==0?segment.consumed:0)));
  const auto bytes=EncodeStateFrameInto(frame,s.scratch.output,context,fs,Limits(joint));
  if(!joint){
    const auto usage=s.Outstanding(stream,now_ms);
    if(bytes.size()>StateBudgets().dynamic_send_bytes-usage.second){
      if(!stream){world_deferral=HostWorldDeferral::WindowFull;++s.world_progress.window_skips;}
      return {};
    }
  }
  if(s.acknowledgements->PinnedBodyBytes()+bytes.size()>StateBudgets().sent_target_bytes)return {};
  message->payload=Own(bytes);frame.encoded=message->payload->bytes;message->target=DescribeStateTarget(s.route,frame);message->kind=StateMessageKind::Frame;message->lane=StateFrameLane(frame);
  if(snapshot.flat_receipt)message->source=message->tables=snapshot.flat_receipt;
  return s.pending=message;
}
void HostStatePublisher::AbandonPending(const std::shared_ptr<const HostStateMessage> &message) noexcept {if(impl_->pending==message)impl_->pending.reset();}
bool HostStatePublisher::StartTable(const std::shared_ptr<const HostStateMessage> &message) noexcept {
  auto &s=*impl_;
  if(!s.open||!message||message!=s.pending||s.table_inflight||message->lane!=StateLane::Context)return false;
  s.table_inflight=std::move(s.pending);
  return true;
}
bool HostStatePublisher::Sent(const std::shared_ptr<const HostStateMessage> &message,std::uint64_t now_ms,std::uint64_t){
  auto &s=*impl_;
  if(!s.open||!message)return false;
  if(message->kind==StateMessageKind::Frame){
    if(message!=s.pending)return false;
    // 预留owner容量后才登记，RegisterSent之后不会再分配或抛出导致悬空span。
    if(s.owners.size()==s.owners.capacity())return false;
    if(!s.acknowledgements->RegisterSent(message->target))return false;s.owners.push_back({message,now_ms});
    for(unsigned i=0;i<message->target.count;++i){const auto &c=message->target.sections[i];const auto n=unsigned(c.stream);
      s.targets[n]=c.target_version;s.offered_sources[n]=c.source_revision;s.offered_resource_epochs[n]=c.resource_epoch;}
    if(message->lane==StateLane::World){
      const auto outstanding=std::uint64_t(s.Outstanding(0,now_ms).first);
      s.world_progress.inflight_max=(std::max)(s.world_progress.inflight_max,outstanding);
    }
    if(message->target.count==2)s.awaiting_joint=true;
    else s.last_stream=unsigned(message->target.sections[0].stream);
    s.pending.reset();
  }else {
    if(message!=s.table_inflight)return false;
  }
  return true;
}
void HostStatePublisher::TableReceived(const HostStateMessage &message){
  auto &s=*impl_;
  if(s.table_inflight.get()!=&message)Fail("host_table_receipt_without_transfer");
  const auto &source=*message.source;
  s.resource_count=source.resources.size();s.resource_base=source.resource_epoch;
  s.table_inflight.reset();
}
StateAckResult HostStatePublisher::Received(const StateRoute &route,const StateAppliedAck &ack){auto &s=*impl_;if(!s.open)return StateAckResult::StaleRoute;
  std::array<std::uint64_t,2> sources=s.confirmed_sources;const auto before=s.acknowledgements->Confirmed();
  auto received_frames=s.received_frames;
  for(const auto &owner:s.owners)if(StateTargetAck(owner->target)==ack)for(unsigned i=0;i<owner->target.count;++i){const auto &c=owner->target.sections[i];const auto n=unsigned(c.stream);
    if(c.target_version>before[n].target){sources[n]=c.source_revision;received_frames[n]={owner->source,owner->tables};}}
  const auto result=s.acknowledgements->Accept(route,ack);
  if(result==StateAckResult::Accepted){
    if(s.acknowledgements->Confirmed()[0].event_watermark>before[0].event_watermark)s.world_event_budget_since.reset();
    s.confirmed_sources=sources;s.received_frames=std::move(received_frames);if(ack.count==2)s.awaiting_joint=false;
    if(!s.acknowledgements->RecoveringStreams())s.rebuild=false;s.Collect();
  }
  return result;
}
bool HostStatePublisher::Recover(const StateRoute &route,const StateResync &request){auto &s=*impl_;if(!s.open)return false;
  const auto plan=s.acknowledgements->Recover(route,request,true);if(!plan.streams)return false;
  const std::array<std::uint32_t,2> history{request.installed_world,request.installed_body};
  if(s.rebuild&&history==s.recovery_history)return false;
  s.recovery_history=history;s.rebuild=true;s.awaiting_joint=false;s.pending.reset();s.table_inflight.reset();
  s.world_event_budget_since.reset();return true;}
bool HostStatePublisher::RecoverExpired(std::uint64_t now){
  auto &s=*impl_;if(!s.open)return false;
  // 给迟到 ACK 保留一段账本时间，超龄的普通帧优先退让给后续发送。
  for(auto &entry:s.ledger)if(entry.count==1&&entry.sections[0].base_version){
    const auto owner=std::find_if(s.owners.begin(),s.owners.end(),[&](const auto &sent){
      return sent->target.encoded.data()==entry.encoded.data();
    });
    if(owner!=s.owners.end()&&now>=owner->sent_ms&&now-owner->sent_ms>=4*StateBudgets().sent_age_ms)entry={};
  }
  s.Collect();
  if(Active()&&s.ordinary_source&&s.ordinary_source->baseline_request_revision>s.confirmed_sources[0]&&
      s.ordinary_source->baseline_request_revision>s.forced_baseline_revision){
    s.forced_baseline_revision=s.ordinary_source->baseline_request_revision;
    return RequireRecovery(StateResyncReason::Lifecycle);
  }
  return false;
}
bool HostStatePublisher::RequireRecovery(StateResyncReason reason){auto &s=*impl_;if(!s.open)return false;const auto c=s.acknowledgements->Confirmed();return Recover(s.route,{s.route.scope,3,reason,c[0].target,c[1].target});}
bool HostStatePublisher::Active() const noexcept {const auto &s=*impl_;if(!s.open)return false;const auto c=s.acknowledgements->Confirmed();return c[0].target&&c[1].target&&!s.rebuild;}
bool HostStatePublisher::Preparing() const noexcept {
  const auto &s=*impl_;if(!s.open||!s.source)return true;
  return s.awaiting_joint||s.rebuild||!Active();
}
std::uint64_t HostStatePublisher::BodyDependencyBlocks() const noexcept {return impl_->body_dependency_blocks;}
HostWorldProgress HostStatePublisher::WorldProgress() const noexcept {return impl_->world_progress;}
std::uint64_t HostStatePublisher::EventWatermark() const noexcept {const auto &s=*impl_;return s.open?s.acknowledgements->Confirmed()[0].event_watermark:0;}
StateEventQueueStats HostStatePublisher::EventStatistics() const noexcept{return impl_->events.Statistics();}
std::array<StateAckEntry,2> HostStatePublisher::Confirmed() const noexcept {return impl_->acknowledgements?impl_->acknowledgements->Confirmed():std::array<StateAckEntry,2>{};}
}
