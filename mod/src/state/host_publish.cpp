#include "state/host_publish.h"
#include "state/budget.h"
#include "core/deferred_values.h"
#include <algorithm>
#include <cstring>
#include <map>
#include <tuple>

namespace cs::state {
namespace {
[[noreturn]] void Fail(const char *s){throw Gap("host_publish",s);}
bool Same(std::span<const std::byte> a,std::span<const std::byte> b){return a.size()==b.size()&&std::equal(a.begin(),a.end(),b.begin());}
using Key=std::tuple<std::uint16_t,std::uint64_t,std::uint64_t>;
Key Identity(CaptureSubject s){return {s.space,s.local,s.use};}
Key Identity(PresentationMember s){return {std::uint16_t(s.space),s.id,s.use};}
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
BodyCodecLimits Limits(bool reliable=true){return {std::uint32_t(StateMessageBodyLimit(reliable?StateLane::Baseline:StateLane::World)),262144,32,reliable};}
ResourceDirectoryView Directory(const HostStateSnapshot &s,std::vector<ResourceEntryView> &entries){
  entries.clear();for(const auto &r:s.resources)entries.push_back({r.key,r.kind,r.payload->bytes});
  std::sort(entries.begin(),entries.end(),[](const auto &a,const auto &b){return a.key<b.key;});
  return {s.witness.scope.session,s.witness.scope.run,s.resource_epoch,0,entries,{}};
}
std::uint64_t SnapshotBytes(const HostStateSnapshot &s){
  return sizeof(s)+s.members.capacity()*sizeof(StateMemberEntry)+s.resources.capacity()*sizeof(HostStateResource)+
    s.records.capacity()*sizeof(std::shared_ptr<const HostStateRecord>)+s.events.capacity()*sizeof(HostStateEvent)+256;
}
// 完整根先解码得到共享owner存在性，随后增量投影也只能读取该根的同一份事实。
class FrameContext final:public StateFrameCodecContext,public BodyCodecContext {
public:
  explicit FrameContext(const HostStateSnapshot &s):snapshot(s),directory(Directory(s,resources)),resource_context(directory){}
  std::optional<PresentationMember> Member(std::uint32_t epoch,std::uint16_t index) const override {
    if(epoch!=snapshot.member_epoch)return {};const auto i=std::lower_bound(snapshot.members.begin(),snapshot.members.end(),index,[](const auto &e,auto n){return e.index<n;});
    return i==snapshot.members.end()||i->index!=index?std::nullopt:std::optional(i->identity);
  }
  const BodyCodecContext *BodyContext(const PresentationBatchContext &,std::uint16_t member) const override {current=member;return this;}
  std::optional<CodecItemConfig> Item(std::uint8_t table,std::uint16_t index) const override{return resource_context.Item(table,index);}
  std::optional<bool> SharedPresence(const CodecLocation &loc) const override {
    for(const auto &record:snapshot.records)if(record->member.index==current)
    for(const auto &f:record->projections)if(f.kind==ProjectionFactKind::SharedPresence&&f.layout==loc.layout&&f.group==loc.canonical_field&&f.depth==loc.parents.size()&&
      std::equal(loc.parents.begin(),loc.parents.end(),f.parents.begin()))return f.value!=0;
    return {};
  }
  const HostStateSnapshot &snapshot;
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
}
HostStateBytes::HostStateBytes(std::span<const std::byte> value):memory(memory::Reserve(memory::Pool::Values,sizeof(*this)+value.size()+128,"host_state_bytes")),bytes(value.begin(),value.end()){}
struct HostStateCapture::Impl {
  Scratch scratch;
  HostStateSnapshotRef latest;
  // 不回收run内序号；耗尽明确失败，避免旧事件／在途帧碰上复用的成员号。
  std::map<Key,std::uint16_t> identities;
  memory::Lease memory=memory::Reserve(memory::Pool::Values,0,"host_state_member_ids");
};
HostStateCapture::HostStateCapture():impl_(std::make_unique<Impl>()){}
HostStateCapture::~HostStateCapture()=default;
void HostStateCapture::Reset() noexcept {impl_->latest.reset();impl_->identities.clear();impl_->memory->Resize(0);}
HostStateSnapshotRef HostStateCapture::Latest() const noexcept{return impl_->latest;}
HostStateSnapshotRef HostStateCapture::Capture(const CapturePacket &packet,std::span<const HostStateResource> resources,void *qualification,bool (*current)(void *) noexcept){
  auto &s=*impl_;ScratchJob job{s.scratch};s.scratch.Ensure(2u<<20);if(!ValidCaptureWitness(packet.witness))Fail("invalid_capture");const bool ordinary=packet.witness.phase==CapturePhase::Ordinary;
  if(!ordinary&&packet.witness.phase!=CapturePhase::PlayerPost)Fail("unsupported_capture_phase");
  const auto prior=s.latest;
  if(!ordinary&&(!prior||prior->witness.scope!=packet.witness.scope||prior->ordinary.revision!=packet.witness.required_revision))Fail("body_requires_current_ordinary");
  const bool new_run=!prior||prior->witness.scope.session!=packet.witness.scope.session||prior->witness.scope.run!=packet.witness.scope.run||prior->witness.generation!=packet.witness.generation;
  auto ids=new_run?std::map<Key,std::uint16_t>{}:s.identities;
  // 主线程可能归还最后一个邮箱引用；整个快照的析构交给既有纯值回收线程。
  auto out=memory::DeferredValue<HostStateSnapshot>();out->memory=memory::Reserve(memory::Pool::Values,0,"host_state_snapshot");out->witness=packet.witness;
  out->ordinary=ordinary?packet.witness:prior->ordinary;
  out->structure_revision=new_run?packet.witness.revision:prior->structure_revision;
  if(!ordinary){out->members=prior->members;out->records=prior->records;out->resources=prior->resources;out->member_epoch=prior->member_epoch;out->resource_epoch=prior->resource_epoch;
    out->event_first=prior->event_first;out->event_watermark=prior->event_watermark;}
  else {
    for(const auto &record:packet.records)if(!record.event_sequence){
      const bool anchor=record.layout==195||record.layout==196;
      if((record.stream==BodyStream::Value&&!anchor)||!record.subject.space||!record.subject.local||!record.subject.use||(!record.subject.shape&&!anchor))Fail("invalid_state_root");
      const auto key=Identity(record.subject);auto i=ids.find(key);
      if(i==ids.end()){if(ids.size()>=16384)Fail("member_index_exhausted");i=ids.emplace(key,std::uint16_t(ids.size())).first;}
      const StateMemberEntry member{i->second,MemberOperation::Join,{record.subject.space,record.layout,record.subject.local,record.subject.use,record.subject.shape?record.subject.shape:1}};
      const auto known=std::find_if(out->members.begin(),out->members.end(),[&](const auto &m){return m.index==member.index;});
      if(known!=out->members.end()){if(known->identity!=member.identity)Fail("root_identity_conflict");}else out->members.push_back(member);
    }
    std::sort(out->members.begin(),out->members.end(),[](const auto &a,const auto &b){return a.index<b.index;});
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
  struct Resolve {const CapturePacket &packet;const HostStateSnapshot &snapshot;} resolve{packet,*out};
  CaptureLinkResolver resolver{&resolve,[](void *p,CaptureSubject,bool resource,std::uint8_t table,CaptureLink link,std::uint16_t &value) noexcept {
    const auto &r=*static_cast<Resolve *>(p);if(link.kind==CapturedReferenceKind::Null){if(link.handle)return false;value=kNullIndex;return true;}
    if((link.kind!=CapturedReferenceKind::Identity&&link.kind!=CapturedReferenceKind::Pending)||!link.handle)return false;
    if(resource){for(const auto &e:r.snapshot.resources)if(e.handle==link.handle&&e.key.table==table){value=e.key.index;return true;}return false;}
    if(link.handle>r.packet.references.size())return false;const auto &ref=r.packet.references[std::size_t(link.handle-1)];
    for(const auto &e:r.snapshot.members)if(e.identity.space==ref.space&&e.identity.id==ref.local&&e.identity.use==ref.use){value=e.index;return true;}
    return false;}};
  for(std::uint32_t i=0;i<packet.records.size();++i){const auto &r=packet.records[i];if(r.event_sequence)continue;
    if(r.layout==195)continue; // 纯身份锚点没有正文，不伪造零长状态记录。
    const auto member=std::find_if(out->members.begin(),out->members.end(),[&](const auto &m){return Identity(m.identity)==Identity(r.subject);});
    if(member==out->members.end()||member->identity.shape!=(r.subject.shape?r.subject.shape:1)||member->identity.layout!=r.layout)Fail("body_member_changed");
    const auto stream=r.stream==BodyStream::Value?BodyStream::World:r.stream;
    if(!ordinary&&r.layout==197){const auto world=std::find_if(prior->records.begin(),prior->records.end(),[&](const auto &entry){return entry->member.index==member->index&&entry->stream==BodyStream::World;});
      if(world==prior->records.end()||(*world)->topology!=r.subject.topology||(*world)->binding!=r.subject.binding)Fail("body_requires_matching_ordinary_topology");}
    CapturedBodySource source(packet,i,resolver,s.scratch.links);auto scratch=s.scratch.Body();
    const auto body=EncodeBodyInto(r.layout,r.mask,stream,source,s.scratch.output,scratch,&context.resource_context,Limits());
    auto record=std::make_shared<HostStateRecord>();record->member=*member;record->stream=stream;record->topology=r.subject.topology;record->binding=r.subject.binding;
    record->projections.assign(body.facts.begin(),body.facts.end());
    std::shared_ptr<const HostStateRecord> old;
    if(!new_run&&prior)for(const auto &entry:prior->records)if(entry->member.identity==member->identity&&entry->stream==stream){old=entry;break;}
    // P6／P7结构变化不必升级member shape；仍须先共同提交同拓扑world与body。
    if(ordinary&&r.layout==197&&stream==BodyStream::World&&(!old||old->topology!=record->topology||old->binding!=record->binding))out->structure_revision=packet.witness.revision;
    for(const auto &block:std::span(body.blocks).first(body.block_count)){
      HostStateBlock next{block.block,packet.witness.revision,{}};
      if(old)for(const auto &b:old->blocks)if(b.block==block.block&&Same(b.payload->bytes,block.bytes)){next=b;break;}
      if(!next.payload)next.payload=Own(block.bytes);record->blocks.push_back(std::move(next));
    }
    record->memory=memory::Reserve(memory::Pool::Values,sizeof(HostStateRecord)+record->blocks.capacity()*sizeof(HostStateBlock)+record->projections.capacity()*sizeof(ProjectionFact)+128,"host_state_record");
    // 一根的所有物理块由采集计划一次提供；不能把缺块当成已退休。
    const auto found=std::find_if(out->records.begin(),out->records.end(),[&](const auto &entry){return entry->member.index==member->index&&entry->stream==stream;});
    if(found==out->records.end())out->records.push_back(std::move(record));else *found=std::move(record);
  }
  if(ordinary){
    StateEventQueue events;events.Reset(packet.event_first_sequence?packet.event_first_sequence-1:0);auto scratch=s.scratch.Body();
    if(!QueueCapturedStateEvents(packet,events,resolver,s.scratch.links,s.scratch.output,scratch,&context.resource_context,Limits()))Fail("event_handoff_gap");
    std::array<StateEventView,512> selected{};const auto segment=events.Select(selected,256u<<10);
    out->event_first=segment.first_sequence;out->event_watermark=segment.watermark;
    for(const auto &e:segment.events){HostStateEvent event{e,Own(e.payload)};event.value.payload=event.payload->bytes;out->events.push_back(std::move(event));}
  }
  out->memory->Resize(SnapshotBytes(*out));
  if(current&&!current(qualification))Fail("capture_revoked_during_encoding");
  s.memory->Resize(ids.size()*96);s.identities=std::move(ids);s.latest=out;return out;
}

struct HostStatePublisher::Impl {
  struct SentOwner {std::shared_ptr<const HostStateMessage> value;std::uint64_t sent_ms=0;
    const HostStateMessage *operator->() const noexcept{return value.get();}};
  memory::Lease memory=memory::Reserve(memory::Pool::Values,sizeof(Impl)+192*sizeof(SentOwner),"host_state_peer");
  Scratch scratch;
  StateRoute route;
  HostStateSnapshotRef source,ordinary_source,table_source;
  StateEventQueue events;
  std::array<StateTarget,192> ledger{};
  std::unique_ptr<HostStateAcknowledgements> acknowledgements;
  std::vector<SentOwner> owners;
  std::shared_ptr<const HostStateMessage> pending;
  std::array<std::uint64_t,2> confirmed_sources{},offered_sources{};
  std::array<std::uint32_t,2> targets{};
  StateContextReady sent_tables{},ready{};
  std::vector<StateContextReady> table_receipts;
  std::array<std::uint32_t,2> recovery_history{};
  unsigned table_stage=0;
  bool open=false,first=true,rebuild=false,awaiting_joint=false;
  unsigned last_stream=0;
  std::uint64_t event_seen=0;
  std::uint64_t capture_session=0;
  Impl(){owners.reserve(ledger.size());table_receipts.reserve(std::size_t(StateBudgets().table_snapshots));}
  void Collect(){owners.erase(std::remove_if(owners.begin(),owners.end(),[&](const auto &owner){return std::none_of(ledger.begin(),ledger.end(),[&](const auto &target){return target.count&&target.encoded.data()==owner->target.encoded.data();});}),owners.end());}
};
HostStatePublisher::HostStatePublisher():impl_(std::make_unique<Impl>()){}
HostStatePublisher::~HostStatePublisher()=default;
void HostStatePublisher::Stop() noexcept {auto &s=*impl_;s.pending.reset();s.owners.clear();s.acknowledgements.reset();s.source.reset();s.ordinary_source.reset();s.table_source.reset();s.ledger={};s.open=false;s.events.Reset();}
void HostStatePublisher::Open(const StateRoute &route){Stop();auto &s=*impl_;s.route=route;s.targets={};s.confirmed_sources={};s.offered_sources={};s.sent_tables={};s.ready={};s.table_stage=0;s.first=true;s.rebuild=false;s.awaiting_joint=false;
  s.event_seen=0;s.capture_session=0;s.last_stream=0;s.table_receipts.clear();s.recovery_history={};s.acknowledgements=std::make_unique<HostStateAcknowledgements>(route,s.ledger,&s.events);s.open=true;}
bool HostStatePublisher::Offer(HostStateSnapshotRef source){auto &s=*impl_;if(!s.open||!source)return false;
  // 采集会话属于独立观测owner；线上session必须使用实际C/S握手身份，其余scope逐项相等。
  auto scope=source->witness.scope;const auto capture_session=scope.session;scope.session=s.route.scope.session;
  if(scope!=s.route.scope||(s.capture_session&&s.capture_session!=capture_session))return false;s.capture_session=capture_session;
  if(source->witness.phase==CapturePhase::Ordinary){
    if(s.first){s.events.Reset(source->event_first-1);s.first=false;}
    // 已覆盖批次可重复Offer；未确认事件始终由本Peer队列保留，不能随body/latest淘汰。
    if(source->event_watermark>s.event_seen&&!s.events.CanCover(source->event_first,source->event_watermark))return false;
    // 相同范围也比较未确认事件的精确正文，不能只凭水位相等接受冲突重传。
    for(const auto &e:source->events)if(!s.events.Offer(e.value))return false;
    if(source->event_watermark>s.event_seen){
      if(!s.events.Cover(source->event_first,source->event_watermark))return false;s.event_seen=source->event_watermark;}
    s.events.Expire(source->witness.game_frame);
    if(!s.ordinary_source||source->witness.revision>s.ordinary_source->witness.revision)s.ordinary_source=source;
    if(s.table_source&&source->member_epoch==s.table_source->member_epoch&&source->resource_epoch==s.table_source->resource_epoch&&source->witness.revision>s.table_source->witness.revision)s.table_source=source;
  }
  if(!s.source||source->witness.revision>s.source->witness.revision)s.source=std::move(source);return true;
}
std::shared_ptr<const HostStateMessage> HostStatePublisher::Next(){auto &s=*impl_;if(!s.open||!s.source)return {};if(s.pending)return s.pending;
  ScratchJob job{s.scratch};s.scratch.Ensure(std::size_t(StateBudgets().prepare_wire_bytes));
  const auto confirmed=s.acknowledgements->Confirmed();
  // 未确认旧表事件先用旧完整来源发送，避免新表把旧引用解释成缺失／另一对象。
  const bool can_replace=s.table_source&&confirmed[0].member_epoch==s.table_source->member_epoch&&confirmed[0].resource_epoch==s.table_source->resource_epoch&&
    confirmed[0].event_watermark>=s.table_source->event_watermark&&!s.awaiting_joint;
  if(!s.table_source || (can_replace&&s.table_stage==2&&s.ready.member_epoch==s.table_source->member_epoch&&s.ready.resource_epoch==s.table_source->resource_epoch&&
    (s.source->member_epoch!=s.table_source->member_epoch||s.source->resource_epoch!=s.table_source->resource_epoch))){s.table_source=s.ordinary_source;s.table_stage=0;}
  auto message=std::make_shared<HostStateMessage>();message->source=s.table_source;auto &source=*message->source;
  if(s.table_stage<2){
    std::span<const std::byte> encoded;
    if(s.table_stage==0){std::vector<std::uint16_t> order(source.members.size());encoded=EncodeStateMemberTableInto({s.route.scope,source.member_epoch,0,source.members},s.scratch.output,order);message->kind=StateMessageKind::Members;}
    else {std::vector<ResourceEntryView> resources;const auto directory=Directory(source,resources);auto body=s.scratch.Body();ResourceCodecScratch scratch{body};
      encoded=EncodeResourceTableInto({s.route.scope,LayoutVersion(),source.resource_epoch,0,directory.entries},s.scratch.output,scratch);message->kind=StateMessageKind::Resources;}
    message->lane=StateLane::Context;message->payload=Own(encoded);return s.pending=message;
  }
  if(s.ready.member_epoch!=source.member_epoch||s.ready.resource_epoch!=source.resource_epoch)return {};
  // 表在途期间固定其来源；新拓扑不能用旧表解释。下一次ContextReady后再选择最新同表值。
  if(s.source->member_epoch==source.member_epoch&&s.source->resource_epoch==source.resource_epoch)message->source=s.source;
  const bool joint=s.rebuild||!confirmed[0].target||!confirmed[1].target||confirmed[0].member_epoch!=message->source->member_epoch||confirmed[0].resource_epoch!=message->source->resource_epoch||
    s.confirmed_sources[0]<message->source->structure_revision||s.confirmed_sources[1]<message->source->structure_revision;
  if(joint){if(s.awaiting_joint)return {};message->source=s.table_source;}
  const auto &snapshot=*message->source;
  unsigned stream=0;
  if(!joint){
    const bool world=snapshot.ordinary.revision>s.offered_sources[0]||s.confirmed_sources[0]<s.offered_sources[0]||confirmed[0].event_watermark<snapshot.event_watermark;
    const bool body=snapshot.witness.revision>s.offered_sources[1]||s.confirmed_sources[1]<s.offered_sources[1];
    if(!world&&!body)return {};stream=body&&(!world||s.last_stream==0)?1:0;
  }
  // 普通目标保留恢复槽，正文与来源快照只在ACK或明确退休后回收。
  if(s.acknowledgements->FreeSlots()<=(joint?0:StateBudgets().sent_recovery_slots))return {};
  FrameContext context(snapshot);
  std::vector<std::vector<std::byte>> payloads;payloads.reserve(snapshot.records.size());
  std::array<std::vector<StateRecordPayload>,2> records;
  for(const auto &record:snapshot.records){
    const auto n=unsigned(record->stream);if(!joint&&n!=stream)continue;std::uint32_t mask=0;
    for(const auto &b:record->blocks)if(joint||b.changed>s.confirmed_sources[n])mask|=1u<<b.block;
    if(!mask)continue;payloads.push_back(Join(*record,mask));records[n].push_back({record->member.index,record->member.identity.layout,mask,payloads.back()});
  }
  std::uint64_t fixed=52+(joint?104:52);
  for(unsigned n=0;n<2;++n)for(const auto &r:records[n])fixed+=12+r.bytes.size();
  const auto limit=StateMessageBodyLimit(joint?StateLane::Baseline:StateLane::World);
  const auto recover=[&]{return Recover(s.route,{s.route.scope,3,StateResyncReason::Budget,confirmed[0].target,confirmed[1].target});};
  if(fixed+(joint||stream==0?StateEventQueueHeaderBytes():0)>limit){if(!joint){recover();return {};}Fail("baseline_record_budget");}
  std::array<StateEventView,512> event_storage{};
  const auto segment=s.events.Select(event_storage,std::uint32_t(std::min<std::uint64_t>(256u<<10,limit-fixed)),snapshot.event_watermark);
  if(!joint&&stream==0&&records[0].empty()&&segment.events.empty()&&segment.watermark==confirmed[0].event_watermark&&snapshot.event_watermark>segment.watermark){recover();return {};}
  StateFrameView frame;frame.scope=s.route.scope;frame.layout_version=LayoutVersion();frame.section_count=joint?2:1;
  auto targets=s.targets;
  for(unsigned i=0;i<frame.section_count;++i){const auto n=joint?i:stream;if(targets[n]==UINT32_MAX)Fail("target_sequence_exhausted");++targets[n];}
  for(unsigned i=0;i<frame.section_count;++i){const auto n=joint?i:stream;auto &section=frame.sections[i];auto &c=section.context;
    c.scope=s.route.scope;c.layout_version=LayoutVersion();c.stream=BodyStream(n);c.member_epoch=snapshot.member_epoch;c.resource_epoch=snapshot.resource_epoch;
    c.base_version=joint?0:confirmed[n].target;c.target_version=targets[n];
    const auto &w=joint?snapshot.ordinary:n?snapshot.witness:snapshot.ordinary;c.source_revision=w.revision;c.presentation_batch=w.batch;c.game_frame=w.game_frame;
    if(joint)c.paired_targets=targets;section.records=records[n];
    if(!n){c.installed_event_watermark=confirmed[0].event_watermark;c.event_watermark=segment.watermark;section.presentation.events=segment.events;section.presentation.first_sequence=segment.first_sequence;}
  }
  std::vector<std::uint16_t> order(snapshot.members.size());StateFrameScratch fs;fs.member_order=order;fs.body=s.scratch.Body();
  s.scratch.Ensure(std::size_t(fixed+(joint||stream==0?segment.consumed:0)));
  const auto bytes=EncodeStateFrameInto(frame,s.scratch.output,context,fs,Limits(joint));
  if(s.acknowledgements->PinnedBodyBytes()+bytes.size()>StateBudgets().sent_target_bytes)return {};
  message->payload=Own(bytes);frame.encoded=message->payload->bytes;message->target=DescribeStateTarget(s.route,frame);message->kind=StateMessageKind::Frame;message->lane=StateFrameLane(frame);
  return s.pending=message;
}
void HostStatePublisher::AbandonPending(const std::shared_ptr<const HostStateMessage> &message) noexcept {if(impl_->pending==message)impl_->pending.reset();}
bool HostStatePublisher::Sent(const std::shared_ptr<const HostStateMessage> &message,std::uint64_t now){auto &s=*impl_;if(!s.open||message!=s.pending)return false;
  if(message->kind==StateMessageKind::Frame){
    // 预留owner容量后才登记，RegisterSent之后不会再分配或抛出导致悬空span。
    if(s.owners.size()==s.owners.capacity())return false;
    if(!s.acknowledgements->RegisterSent(message->target))return false;s.owners.push_back({message,now});
    for(unsigned i=0;i<message->target.count;++i){const auto &c=message->target.sections[i];const auto n=unsigned(c.stream);s.targets[n]=c.target_version;s.offered_sources[n]=c.source_revision;}
    if(message->target.count==2)s.awaiting_joint=true;
    else s.last_stream=unsigned(message->target.sections[0].stream);
  }else {++s.table_stage;if(s.table_stage==2){s.sent_tables={s.route.scope,message->source->member_epoch,message->source->resource_epoch};
    if(s.table_receipts.size()==StateBudgets().table_snapshots)s.table_receipts.erase(s.table_receipts.begin());s.table_receipts.push_back(s.sent_tables);}}
  s.pending.reset();return true;
}
bool HostStatePublisher::ContextReady(const StateRoute &route,const StateContextReady &ready){auto &s=*impl_;if(!s.open||!AcceptStateContextReady(s.route,route,ready,s.table_receipts))return false;
  if(ready.member_epoch==s.sent_tables.member_epoch&&ready.resource_epoch==s.sent_tables.resource_epoch)s.ready=ready;return true;}
StateAckResult HostStatePublisher::Applied(const StateRoute &route,const StateAppliedAck &ack){auto &s=*impl_;if(!s.open)return StateAckResult::StaleRoute;
  std::array<std::uint64_t,2> sources=s.confirmed_sources;const auto before=s.acknowledgements->Confirmed();
  for(const auto &owner:s.owners)if(StateTargetAck(owner->target)==ack)for(unsigned i=0;i<owner->target.count;++i){const auto &c=owner->target.sections[i];if(c.target_version>before[unsigned(c.stream)].target)sources[unsigned(c.stream)]=c.source_revision;}
  const auto result=s.acknowledgements->Accept(route,ack);if(result==StateAckResult::Accepted){s.confirmed_sources=sources;if(ack.count==2)s.awaiting_joint=false;if(!s.acknowledgements->RecoveringStreams())s.rebuild=false;s.Collect();}return result;
}
bool HostStatePublisher::Recover(const StateRoute &route,const StateResync &request){auto &s=*impl_;if(!s.open)return false;
  const auto plan=s.acknowledgements->Recover(route,request,true);if(!plan.streams)return false;
  const std::array<std::uint32_t,2> history{request.installed_world,request.installed_body};
  if(s.rebuild&&history==s.recovery_history)return false;
  s.recovery_history=history;s.rebuild=true;s.awaiting_joint=false;s.pending.reset();return true;}
bool HostStatePublisher::RecoverExpired(std::uint64_t now){auto &s=*impl_;if(!s.open||s.rebuild)return false;
  const bool expired=std::any_of(s.owners.begin(),s.owners.end(),[&](const auto &entry){return now<entry.sent_ms||now-entry.sent_ms>=StateBudgets().sent_age_ms;});
  if(!expired)return false;const auto confirmed=s.acknowledgements->Confirmed();
  return Recover(s.route,{s.route.scope,3,StateResyncReason::Base,confirmed[0].target,confirmed[1].target});}
bool HostStatePublisher::RequireRecovery(StateResyncReason reason){auto &s=*impl_;if(!s.open)return false;const auto c=s.acknowledgements->Confirmed();return Recover(s.route,{s.route.scope,3,reason,c[0].target,c[1].target});}
bool HostStatePublisher::Active() const noexcept {const auto &s=*impl_;if(!s.open)return false;const auto c=s.acknowledgements->Confirmed();return c[0].target&&c[1].target&&!s.rebuild;}
bool HostStatePublisher::Preparing() const noexcept {const auto &s=*impl_;if(!s.open||!s.source)return true;const auto c=s.acknowledgements->Confirmed();
  return s.rebuild||s.awaiting_joint||!c[0].target||!c[1].target||c[0].member_epoch!=s.source->member_epoch||c[0].resource_epoch!=s.source->resource_epoch||
    s.confirmed_sources[0]<s.source->structure_revision||s.confirmed_sources[1]<s.source->structure_revision;}
std::uint64_t HostStatePublisher::EventWatermark() const noexcept {const auto &s=*impl_;return s.open?s.acknowledgements->Confirmed()[0].event_watermark:0;}
StateEventQueueStats HostStatePublisher::EventStatistics() const noexcept{return impl_->events.Statistics();}
std::array<StateAckEntry,2> HostStatePublisher::Confirmed() const noexcept {return impl_->acknowledgements?impl_->acknowledgements->Confirmed():std::array<StateAckEntry,2>{};}
}
