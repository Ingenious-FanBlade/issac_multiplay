#include "state/wire_protocol.h"
#include "state/budget.h"
#include <algorithm>
#include <cstring>
#include <tuple>

namespace cs::state {
namespace {
#include "state_protocol.inc"
static_assert(unsigned(StateMessageKind::Frame)==kW_StateFrame && unsigned(StateMessageKind::Members)==kW_MemberTable &&
    unsigned(StateMessageKind::Resources)==kW_ResourceTable && unsigned(StateMessageKind::AppliedAck)==kW_AppliedAck);
[[noreturn]] void Fail(const char *reason) { throw Gap("state_protocol",reason); }
bool Overlap(std::span<const std::byte> a,std::span<const std::byte> b) {
  if (a.empty() || b.empty()) return false;
  const auto x=reinterpret_cast<std::uintptr_t>(a.data()),y=reinterpret_cast<std::uintptr_t>(b.data());
  return x<=y?y-x<a.size():x-y<b.size();
}
template<class T> std::span<const std::byte> ObjectBytes(const T &value) { return std::as_bytes(std::span(&value,1)); }
std::uint64_t Bits(std::span<const std::byte> bytes) {
  if (bytes.empty() || bytes.size()>8) Fail("integer_width");
  std::uint64_t value=0;for (std::size_t i=0;i<bytes.size();++i) value|=std::uint64_t(std::to_integer<unsigned>(bytes[i]))<<(8*i);
  return value;
}
class Cursor {
public:
  explicit Cursor(std::span<const std::byte> bytes):bytes_(bytes) {}
  std::span<const std::byte> Take(std::size_t count) {
    if (count>bytes_.size()-position_) Fail("truncated_message");
    const auto value=bytes_.subspan(position_,count);position_+=count;return value;
  }
  std::size_t Position() const { return position_; }
private:
  std::span<const std::byte> bytes_;std::size_t position_=0;
};
template<class E> using Values=std::array<std::uint64_t,std::size_t(E::count)>;
template<class E> std::uint64_t &At(Values<E> &values,E field) { return values[std::size_t(field)]; }
class FixedSource final : public BodyValueSource {
public:
  explicit FixedSource(std::span<const std::uint64_t> values):values_(values) {}
  std::span<const std::byte> Scalar(const CodecLocation &loc,ScalarKind kind,std::uint32_t count) const override {
    const auto width=kind==ScalarKind::U8?1u:kind==ScalarKind::U16?2u:kind==ScalarKind::U32?4u:kind==ScalarKind::U64?8u:0u;
    if (!width || count!=1 || !loc.parents.empty() || loc.field>=values_.size()) Fail("fixed_header_plan");
    const auto value=values_[loc.field];if (width<8 && value>>(8*width)) Fail("header_narrowing");
    for (unsigned i=0;i<width;++i) bytes_[i]=std::byte(value>>(8*i));return std::span(bytes_).first(width);
  }
  std::uint32_t Count(const CodecLocation &) const override { Fail("fixed_header_container"); }
  bool SharedPresent(const CodecLocation &) const override { Fail("fixed_header_shared"); }
  std::uint32_t SlotMask(const CodecLocation &) const override { Fail("fixed_header_choice"); }
  std::uint16_t SlotLayout(const CodecLocation &,std::uint8_t) const override { Fail("fixed_header_choice"); }
private:
  std::span<const std::uint64_t> values_;mutable std::array<std::byte,8> bytes_{};
};
template<class E> Values<E> Read(std::uint16_t layout,std::span<const std::byte> bytes) {
  std::array<BodyFieldView,std::size_t(E::count)> fields{};BodyCodecScratch scratch;scratch.root_fields=fields;
  const auto body=DecodeValue(layout,bytes,scratch);Values<E> out{};
  if (body.fields.size()!=out.size()) Fail("header_field_count");
  for (const auto &f:body.fields) {if (!f.present || f.field>=out.size()) Fail("header_field");out[f.field]=Bits(f.bytes);}
  return out;
}
template<std::size_t N> void Write(std::uint16_t layout,const std::array<std::uint64_t,N> &values,std::span<std::byte> output) {
  FixedSource source(values);BodyCodecScratch scratch;
  if (EncodeValueInto(layout,source,output,scratch).bytes.size()!=output.size()) Fail("fixed_header_size");
}
void Scope(const StateScope &scope) { if (!scope.session || !scope.run || !scope.scene) Fail("missing_scope"); }
void Common(Cursor &input,const StateScope &expected) {
  const auto v=Read<StateCommonHeaderField>(kW_StateCommonHeader,input.Take(kW_StateCommonHeaderBytes));
  const auto get=[&](StateCommonHeaderField f){return v[std::size_t(f)];};
  const StateScope scope{get(StateCommonHeaderField::session),get(StateCommonHeaderField::run),get(StateCommonHeaderField::floor),
      get(StateCommonHeaderField::dimension),get(StateCommonHeaderField::room),get(StateCommonHeaderField::scene)};
  Scope(scope);if (get(StateCommonHeaderField::layout_version)!=LayoutVersion() || scope!=expected) Fail("version_or_scope");
}
void Common(const StateScope &scope,std::span<std::byte> output) {
  Scope(scope);Values<StateCommonHeaderField> v{};At(v,StateCommonHeaderField::layout_version)=LayoutVersion();
  At(v,StateCommonHeaderField::session)=scope.session;At(v,StateCommonHeaderField::run)=scope.run;
  At(v,StateCommonHeaderField::floor)=scope.floor;At(v,StateCommonHeaderField::dimension)=scope.dimension;
  At(v,StateCommonHeaderField::room)=scope.room;At(v,StateCommonHeaderField::scene)=scope.scene;
  Write(kW_StateCommonHeader,v,output.first(kW_StateCommonHeaderBytes));
}
void Limits(BodyCodecLimits limits,std::size_t bytes) {
  if (!limits.bytes || limits.bytes>(limits.reliable?(32u<<20):65536u) || bytes>limits.bytes ||
      bytes>StateMessageBodyLimit(limits.reliable?StateLane::Baseline:StateLane::World) ||
      !limits.field_visits || !limits.depth || limits.depth>kCodecMaxDepth) Fail("message_budget");
}
void Visit(BodyCodecLimits &limits,std::uint32_t count) {
  if (count>limits.field_visits) Fail("whole_message_visit_budget");limits.field_visits-=count;
}
void Member(const StateMemberEntry &entry,bool full) {
  const auto *layout=FindLayout(entry.identity.layout);
  if (entry.index==kNullIndex || unsigned(entry.operation)>2 || (full && entry.operation!=MemberOperation::Join) ||
      !entry.identity.id || !entry.identity.use || !entry.identity.shape || !layout || layout->protocol) Fail("member_entry");
}
void Members(const StateMemberTable &table) {
  Scope(table.scope);
  if (!table.epoch || table.previous_epoch>=table.epoch || table.entries.size()>kW_member_entries) Fail("member_table_epoch_or_count");
  for (std::size_t i=0;i<table.entries.size();++i) {
    Member(table.entries[i],!table.previous_epoch);
    if (i && table.entries[i-1].index>=table.entries[i].index) Fail("member_table_order");
  }
}
void MemberKeys(std::span<const StateMemberEntry> entries,std::span<std::uint16_t> order) {
  if (order.size()<entries.size() || Overlap(std::as_bytes(entries),std::as_bytes(order))) Fail("member_key_order_scratch");
  order=order.first(entries.size());for (std::size_t i=0;i<entries.size();++i) order[i]=std::uint16_t(i);
  const auto key=[&](std::uint16_t n){return std::tie(entries[n].identity.space,entries[n].identity.id);};
  std::sort(order.begin(),order.end(),[&](auto a,auto b){return key(a)<key(b);});
  for (std::size_t i=1;i<order.size();++i) if (key(order[i-1])==key(order[i])) Fail("duplicate_member_key");
}
void FrameScratch(const StateFrameScratch &scratch,std::span<const std::byte> bytes) {
  const std::array regions{std::as_bytes(scratch.records),std::as_bytes(scratch.groups),std::as_bytes(scratch.events),
      std::as_bytes(scratch.member_order),std::as_bytes(scratch.presentation.dependencies),std::as_bytes(scratch.presentation.contributions),
      std::as_bytes(scratch.presentation.targets),std::as_bytes(scratch.presentation.dependency_order),std::as_bytes(scratch.presentation.contribution_order),
      std::as_bytes(scratch.presentation.body.facts),std::as_bytes(scratch.presentation.body.root_fields)};
  for (std::size_t i=0;i<regions.size();++i) {
    if (Overlap(bytes,regions[i])) Fail("frame_scratch_overlaps_message");
    for (std::size_t j=0;j<i;++j) if (Overlap(regions[i],regions[j])) Fail("frame_scratch_alias");
  }
}
void FrameContexts(const StateFrameView &frame,bool reliable) {
  Scope(frame.scope);
  if (frame.layout_version!=LayoutVersion() || !frame.section_count || frame.section_count>2) Fail("frame_version_or_count");
  std::size_t count=0;
  for (unsigned i=0;i<frame.section_count;++i) {
    const auto &s=frame.sections[i];const auto &c=s.context;
    if (c.scope!=frame.scope || c.layout_version!=frame.layout_version || unsigned(c.stream)>1 ||
        (frame.section_count==2 && unsigned(c.stream)!=i) || !c.member_epoch || !c.resource_epoch ||
        !c.target_version || c.target_version<=c.base_version || !c.source_revision || !c.presentation_batch ||
        (c.stream==BodyStream::Body && (c.installed_event_watermark || c.event_watermark || !s.presentation.events.empty()))) Fail("section_context");
    if (frame.section_count==1 && (c.paired_targets[0] || c.paired_targets[1])) Fail("unpaired_target");
    if (s.records.size()>kW_frame_records-count) Fail("frame_record_budget");count+=s.records.size();
    if (s.presentation.groups.size()>kW_max_result_groups || s.presentation.events.size()>kW_max_audio_events) Fail("section_presentation_budget");
  }
  if (frame.section_count==2) {
    const auto &a=frame.sections[0].context,&b=frame.sections[1].context;
    const std::array<std::uint32_t,2> targets{a.target_version,b.target_version};
    if (!reliable || a.source_revision!=b.source_revision || a.presentation_batch!=b.presentation_batch ||
        a.game_frame!=b.game_frame || a.member_epoch!=b.member_epoch || a.resource_epoch!=b.resource_epoch ||
        bool(a.base_version)!=bool(b.base_version) || a.paired_targets!=targets || b.paired_targets!=targets) Fail("joint_context");
  } else if (reliable!=(!frame.sections[0].context.base_version)) Fail("frame_reliability_does_not_match_lane");
}
void Record(const StateRecordPayload &record,const PresentationBatchContext &section,const StateFrameCodecContext &context,
    BodyCodecScratch &scratch,BodyCodecLimits &remaining) {
  const auto member=context.Member(section.member_epoch,record.member);
  if (record.member==kNullIndex || !member || member->layout!=record.layout || !member->id || !member->use || !member->shape ||
      !FindLayout(record.layout) || FindLayout(record.layout)->protocol) Fail("record_member_or_layout");
  auto limits=remaining;
  const auto body=DecodeBody(record.layout,record.mask,section.stream,record.bytes,scratch,context.BodyContext(section,record.member),limits);
  Visit(remaining,body.field_visits);
}
void UniqueRecords(std::span<const StateRecordPayload> records,std::span<std::uint16_t> order) {
  if (order.size()<records.size()) Fail("record_order_scratch");order=order.first(records.size());
  for (std::size_t i=0;i<records.size();++i) order[i]=records[i].member;
  std::sort(order.begin(),order.end());if (std::adjacent_find(order.begin(),order.end())!=order.end()) Fail("duplicate_section_member");
}
void Ack(const StateAppliedAck &ack) {
  Scope(ack.scope);if (!ack.count || ack.count>2) Fail("ack_count");
  for (unsigned i=0;i<ack.count;++i) {
    const auto &e=ack.entries[i];
    if (unsigned(e.stream)>1 || (ack.count==2 && unsigned(e.stream)!=i) || !e.target || !e.member_epoch || !e.resource_epoch ||
        (e.stream==BodyStream::Body && e.event_watermark)) Fail("ack_entry");
  }
  if (ack.count==1 && ack.entries[1]!=StateAckEntry{}) Fail("ack_unused_entry");
}
} // namespace

StateAgreement LocalStateAgreement() noexcept { return {kW_binary_state_capability,LayoutVersion()}; }
bool CompatibleStateAgreement(StateAgreement a) noexcept { return a.binary_state==kW_binary_state_capability && a.layout_version==LayoutVersion(); }

StateMemberTable DecodeStateMemberTable(const StateScope &scope,std::span<const std::byte> bytes,std::span<StateMemberEntry> storage,std::span<std::uint16_t> order) {
  if (bytes.size()>(32u<<20) || Overlap(bytes,std::as_bytes(storage)) || Overlap(bytes,std::as_bytes(order)) ||
      Overlap(std::as_bytes(storage),std::as_bytes(order))) Fail("member_decode_budget_or_overlap");
  Cursor input(bytes);Common(input,scope);const auto h=Read<StateTableHeaderField>(kW_StateTableHeader,input.Take(kW_StateTableHeaderBytes));
  const auto count=std::size_t(h[std::size_t(StateTableHeaderField::entry_count)]);
  if (count>storage.size() || bytes.size()!=kW_StateCommonHeaderBytes+kW_StateTableHeaderBytes+count*kW_StateMemberEntryBytes) Fail("member_table_length_or_scratch");
  StateMemberTable table{scope,std::uint32_t(h[std::size_t(StateTableHeaderField::epoch)]),
      std::uint32_t(h[std::size_t(StateTableHeaderField::previous_epoch)]),storage.first(count)};
  for (std::size_t i=0;i<count;++i) {
    const auto v=Read<StateMemberEntryField>(kW_StateMemberEntry,input.Take(kW_StateMemberEntryBytes));
    const auto get=[&](StateMemberEntryField f){return v[std::size_t(f)];};
    storage[i]={std::uint16_t(get(StateMemberEntryField::member)),MemberOperation(get(StateMemberEntryField::op)),
        {std::uint32_t(get(StateMemberEntryField::space)),std::uint16_t(get(StateMemberEntryField::layout)),
         get(StateMemberEntryField::id),get(StateMemberEntryField::use),get(StateMemberEntryField::shape)}};
  }
  Members(table);MemberKeys(table.entries,order);return table;
}
std::span<const std::byte> EncodeStateMemberTableInto(const StateMemberTable &table,std::span<std::byte> output,std::span<std::uint16_t> order) {
  Members(table);const auto size=kW_StateCommonHeaderBytes+kW_StateTableHeaderBytes+table.entries.size()*kW_StateMemberEntryBytes;
  if (size>output.size() || Overlap(ObjectBytes(table),output) || Overlap(std::as_bytes(table.entries),output) ||
      Overlap(std::as_bytes(order),output) || Overlap(ObjectBytes(table),std::as_bytes(order))) Fail("member_encode_budget_or_overlap");
  MemberKeys(table.entries,order);
  Common(table.scope,output);Values<StateTableHeaderField> h{};At(h,StateTableHeaderField::epoch)=table.epoch;
  At(h,StateTableHeaderField::previous_epoch)=table.previous_epoch;At(h,StateTableHeaderField::entry_count)=table.entries.size();
  Write(kW_StateTableHeader,h,output.subspan(kW_StateCommonHeaderBytes,kW_StateTableHeaderBytes));
  auto position=kW_StateCommonHeaderBytes+kW_StateTableHeaderBytes;
  for (const auto &e:table.entries) {
    Values<StateMemberEntryField> v{};At(v,StateMemberEntryField::member)=e.index;At(v,StateMemberEntryField::op)=unsigned(e.operation);
    At(v,StateMemberEntryField::space)=e.identity.space;At(v,StateMemberEntryField::layout)=e.identity.layout;
    At(v,StateMemberEntryField::id)=e.identity.id;At(v,StateMemberEntryField::use)=e.identity.use;At(v,StateMemberEntryField::shape)=e.identity.shape;
    Write(kW_StateMemberEntry,v,output.subspan(position,kW_StateMemberEntryBytes));position+=kW_StateMemberEntryBytes;
  }
  return output.first(size);
}
const StateMemberEntry *FindStateMember(const StateMemberDirectory &directory,std::uint16_t index) noexcept {
  const auto it=std::lower_bound(directory.entries.begin(),directory.entries.end(),index,[](const auto &e,std::uint16_t id){return e.index<id;});
  return it==directory.entries.end() || it->index!=index?nullptr:&*it;
}
StateMemberDirectory MergeStateMemberTableInto(const StateMemberDirectory &current,const StateMemberTable &incoming,
    std::span<StateMemberEntry> storage,const StateMemberTransitions &transitions) {
  Members(incoming);
  if (current.epoch) {
    Members({current.scope,current.epoch,0,current.entries});
    if (current.scope!=incoming.scope) Fail("member_directory_scope");
  } else if (!current.entries.empty() || !current.update.empty() || current.previous_epoch) Fail("empty_member_directory");
  if (incoming.epoch==current.epoch) {
    const auto expected=incoming.previous_epoch?current.update:current.entries;
    if ((incoming.previous_epoch && incoming.previous_epoch!=current.previous_epoch) ||
        !std::equal(expected.begin(),expected.end(),incoming.entries.begin(),incoming.entries.end())) Fail("member_epoch_conflict");
    return current;
  }
  if (incoming.epoch<current.epoch || (incoming.previous_epoch && incoming.previous_epoch!=current.epoch)) Fail("member_epoch_chain");
  const auto destination=std::as_bytes(storage);
  if (Overlap(destination,ObjectBytes(current)) || Overlap(destination,ObjectBytes(incoming)) ||
      Overlap(destination,std::as_bytes(current.entries)) || Overlap(destination,std::as_bytes(current.update)) ||
      Overlap(destination,std::as_bytes(incoming.entries))) Fail("member_merge_overlap");
  std::size_t a=0,b=0,count=0;
  const auto append=[&](const StateMemberEntry &e) {
    if (count==storage.size() || count==kW_member_entries) Fail("member_merge_storage");storage[count]=e;storage[count++].operation=MemberOperation::Join;
  };
  while (a<current.entries.size() || b<incoming.entries.size()) {
    const auto *old=a<current.entries.size()?&current.entries[a]:nullptr;
    const auto *next=b<incoming.entries.size()?&incoming.entries[b]:nullptr;
    if (old && (!next || old->index<next->index)) {
      if (incoming.previous_epoch) append(*old);else if (!transitions.Allowed(incoming.scope,old,nullptr)) Fail("member_full_retirement");
      ++a;continue;
    }
    if (!old || next->index<old->index) {
      if (next->operation!=MemberOperation::Join || !transitions.Allowed(incoming.scope,nullptr,next)) Fail("member_join_or_reuse");
      append(*next);++b;continue;
    }
    if (!incoming.previous_epoch) {
      if (old->identity!=next->identity) {
        if (old->identity.space==next->identity.space && old->identity.id==next->identity.id && old->identity.use==next->identity.use &&
            next->identity.shape<=old->identity.shape) Fail("member_full_shape_rewind");
        if (!transitions.Allowed(incoming.scope,old,next)) Fail("member_full_identity_transition");
      }
      append(*next);
    } else if (next->operation==MemberOperation::Join) {
      if (old->identity!=next->identity) Fail("member_join_identity_conflict");append(*old);
    } else if (next->operation==MemberOperation::Leave) {
      if (old->identity!=next->identity || !transitions.Allowed(incoming.scope,old,nullptr)) Fail("member_leave_identity");
    } else {
      if (old->identity.space!=next->identity.space || old->identity.id!=next->identity.id || old->identity.use!=next->identity.use ||
          next->identity.shape<=old->identity.shape || !transitions.Allowed(incoming.scope,old,next)) Fail("member_reshape_identity");
      append(*next);
    }
    ++a;++b;
  }
  // 对新快照按Key排序查重，再恢复线上member顺序；只触碰未发布的新存储。
  auto result=storage.first(count);
  const auto key=[](const auto &e){return std::tie(e.identity.space,e.identity.id);};
  std::sort(result.begin(),result.end(),[&](const auto &x,const auto &y){return key(x)<key(y);});
  for (std::size_t i=1;i<count;++i) if (key(result[i-1])==key(result[i])) Fail("merged_member_key_conflict");
  std::sort(result.begin(),result.end(),[](const auto &x,const auto &y){return x.index<y.index;});
  return {incoming.scope,incoming.epoch,incoming.previous_epoch,result,incoming.entries};
}

StateFrameView InspectStateFrame(const StateScope &scope,std::span<const std::byte> bytes,std::uint64_t installed_events,
    StateFrameScratch &scratch,BodyCodecLimits limits) {
  Limits(limits,bytes.size());
  FrameScratch(scratch,bytes);
  Cursor input(bytes);Common(input,scope);const auto h=Read<StateFrameHeaderField>(kW_StateFrameHeader,input.Take(kW_StateFrameHeaderBytes));
  StateFrameView frame;frame.scope=scope;frame.layout_version=LayoutVersion();frame.section_count=std::uint8_t(h[std::size_t(StateFrameHeaderField::section_count)]);
  if (!frame.section_count || frame.section_count>2 || h[std::size_t(StateFrameHeaderField::flags)]!=(frame.section_count==2?1u:0u)) Fail("frame_flags");
  std::array<std::span<const std::byte>,2> presentation_bytes{};
  std::array<std::uint32_t,2> group_counts{},event_counts{};
  std::size_t records=0,groups=0,events=0;auto remaining=limits;
  Visit(remaining,std::uint32_t(StateCommonHeaderField::count)+std::uint32_t(StateFrameHeaderField::count));
  // 先只定界两段；第二段的真实target可供第一段PairedTarget校验，不猜来源映射。
  for (unsigned i=0;i<frame.section_count;++i) {
    const auto v=Read<StateSectionHeaderField>(kW_StateSectionHeader,input.Take(kW_StateSectionHeaderBytes));
    const auto get=[&](StateSectionHeaderField f){return v[std::size_t(f)];};
    auto &s=frame.sections[i];auto &c=s.context;c.scope=scope;c.layout_version=LayoutVersion();c.stream=BodyStream(get(StateSectionHeaderField::stream));
    c.member_epoch=std::uint32_t(get(StateSectionHeaderField::member_epoch));c.resource_epoch=std::uint32_t(get(StateSectionHeaderField::resource_epoch));
    c.base_version=std::uint32_t(get(StateSectionHeaderField::base_version));c.target_version=std::uint32_t(get(StateSectionHeaderField::target_version));
    c.source_revision=get(StateSectionHeaderField::source_revision);c.presentation_batch=get(StateSectionHeaderField::presentation_batch);
    c.game_frame=std::uint32_t(get(StateSectionHeaderField::game_frame));c.event_watermark=get(StateSectionHeaderField::event_watermark);
    c.installed_event_watermark=c.stream==BodyStream::World?installed_events:0;
    if (get(StateSectionHeaderField::flags)!=(c.base_version?0u:1u)) Fail("baseline_flag");
    const auto n=std::size_t(get(StateSectionHeaderField::record_count));
    group_counts[i]=std::uint32_t(get(StateSectionHeaderField::group_count));event_counts[i]=std::uint32_t(get(StateSectionHeaderField::event_count));
    if (n>scratch.records.size()-records || n>kW_frame_records-records || group_counts[i]>kW_max_result_groups-groups ||
        event_counts[i]>kW_max_audio_events-events || group_counts[i]>scratch.groups.size()-groups || event_counts[i]>scratch.events.size()-events) Fail("frame_count_or_scratch");
    Visit(remaining,std::uint32_t(StateSectionHeaderField::count));
    for (std::size_t j=0;j<n;++j) {
      const auto r=Read<StateRecordHeaderField>(kW_StateRecordHeader,input.Take(kW_StateRecordHeaderBytes));
      scratch.records[records+j]={std::uint16_t(r[std::size_t(StateRecordHeaderField::member)]),std::uint16_t(r[std::size_t(StateRecordHeaderField::layout)]),
          std::uint32_t(r[std::size_t(StateRecordHeaderField::block_mask)]),input.Take(std::size_t(r[std::size_t(StateRecordHeaderField::record_bytes)]))};
      Visit(remaining,std::uint32_t(StateRecordHeaderField::count));
    }
    s.records=scratch.records.subspan(records,n);records+=n;
    const auto start=input.Position();
    for (std::uint32_t j=0;j<group_counts[i];++j) {
      const auto g=Read<StateResultGroupHeaderField>(kW_StateResultGroupHeader,input.Take(kW_StateResultGroupHeaderBytes));
      input.Take(std::size_t(g[std::size_t(StateResultGroupHeaderField::payload_bytes)]));
    }
    for (std::uint32_t j=0;j<event_counts[i];++j) {
      const auto e=Read<StateAudioEventHeaderField>(kW_StateAudioEventHeader,input.Take(kW_StateAudioEventHeaderBytes));
      input.Take(std::size_t(e[std::size_t(StateAudioEventHeaderField::payload_bytes)]));
    }
    presentation_bytes[i]=bytes.subspan(start,input.Position()-start);groups+=group_counts[i];events+=event_counts[i];
  }
  if (input.Position()!=bytes.size()) Fail("frame_trailing_bytes");
  if (frame.section_count==2) for (auto &s:frame.sections) s.context.paired_targets={frame.sections[0].context.target_version,frame.sections[1].context.target_version};
  FrameContexts(frame,limits.reliable);
  // consumed仅记录已定界的区域长度，不暴露未解码groups／events为合法值。
  for (unsigned i=0;i<frame.section_count;++i) {
    frame.sections[i].presentation.consumed=std::uint32_t(presentation_bytes[i].size());
    // count从固定Section头保留在后续解码所需的独立扫描中，不借造span计数。
  }
  frame.encoded=bytes;return frame;
}

StateFrameView DecodeStateFrame(const StateScope &scope,std::span<const std::byte> bytes,std::uint64_t installed_events,
    const StateFrameCodecContext &context,StateFrameScratch &scratch,BodyCodecLimits limits) {
  auto frame=InspectStateFrame(scope,bytes,installed_events,scratch,limits);
  std::size_t position=kW_StateCommonHeaderBytes+kW_StateFrameHeaderBytes,groups=0,events=0;auto remaining=limits;
  Visit(remaining,std::uint32_t(StateCommonHeaderField::count)+std::uint32_t(StateFrameHeaderField::count));
  std::size_t dependencies=0,contributions=0;
  for (unsigned i=0;i<frame.section_count;++i) {
    auto &s=frame.sections[i];UniqueRecords(s.records,scratch.member_order);
    const auto h=Read<StateSectionHeaderField>(kW_StateSectionHeader,bytes.subspan(position,kW_StateSectionHeaderBytes));position+=kW_StateSectionHeaderBytes;
    const auto ng=std::uint32_t(h[std::size_t(StateSectionHeaderField::group_count)]),ne=std::uint32_t(h[std::size_t(StateSectionHeaderField::event_count)]);
    Visit(remaining,std::uint32_t(StateSectionHeaderField::count));
    for (const auto &record:s.records) {
      Record(record,s.context,context,scratch.presentation.body,remaining);position+=kW_StateRecordHeaderBytes+record.bytes.size();
      Visit(remaining,std::uint32_t(StateRecordHeaderField::count));
    }
    const auto region=bytes.subspan(position,s.presentation.consumed);auto local=remaining;local.bytes=std::uint32_t(region.size());
    // 空显示区域仍用剩余正预算校验；其线上长度仍精确为0。
    if (!local.bytes) local.bytes=remaining.bytes;
    auto envelope=scratch.presentation;
    envelope.dependencies=envelope.dependencies.subspan(dependencies);envelope.contributions=envelope.contributions.subspan(contributions);
    s.presentation=DecodePresentationBatch(s.context,ng,ne,region,scratch.groups.subspan(groups,ng),
        scratch.events.subspan(events,ne),envelope,context.BodyContext(s.context,kNullIndex),{local});
    dependencies+=envelope.dependencies_used;contributions+=envelope.contributions_used;
    if (dependencies>kW_max_result_dependencies || contributions>kW_max_result_contributions) Fail("whole_frame_presentation_budget");
    for (const auto &g:s.presentation.groups) Visit(remaining,g.field_visits);
    for (const auto &e:s.presentation.events) Visit(remaining,e.field_visits);
    groups+=ng;events+=ne;position+=region.size();
  }
  frame.decoded=true;return frame;
}

StateLane StateFrameLane(const StateFrameView &frame) {
  if (frame.section_count==2) return StateLane::Baseline;
  if (frame.section_count!=1) Fail("frame_lane_count");
  const auto &c=frame.sections[0].context;
  if (!c.base_version) return StateLane::Baseline;
  if (c.stream==BodyStream::World) return StateLane::World;
  if (c.stream==BodyStream::Body) return StateLane::Body;
  Fail("frame_lane_stream");
}
std::span<const std::byte> EncodeStateFrameInto(StateFrameView &frame,std::span<std::byte> output,
    const StateFrameCodecContext &context,StateFrameScratch &scratch,BodyCodecLimits limits) {
  FrameContexts(frame,limits.reliable);std::uint64_t size=kW_StateCommonHeaderBytes+kW_StateFrameHeaderBytes;
  FrameScratch(scratch,output);
  if (Overlap(ObjectBytes(frame),output)) Fail("frame_encode_metadata_overlap");
  std::uint64_t groups=0,events=0,dependencies=0,contributions=0;
  for (unsigned i=0;i<frame.section_count;++i) {
    const auto &s=frame.sections[i];size+=kW_StateSectionHeaderBytes;
    if (Overlap(std::as_bytes(s.records),output) || Overlap(std::as_bytes(s.presentation.groups),output) ||
        Overlap(std::as_bytes(s.presentation.events),output)) Fail("frame_encode_metadata_overlap");
    for (const auto &r:s.records) {size+=kW_StateRecordHeaderBytes+r.bytes.size();if (Overlap(r.bytes,output)) Fail("frame_record_overlap");}
    for (const auto &g:s.presentation.groups) {
      size+=kW_StateResultGroupHeaderBytes+kW_StateDependencyEntryBytes*std::uint64_t(g.dependencies.size())+
          kW_StateContributionEntryBytes*std::uint64_t(g.contributions.size())+g.payload.size();
      dependencies+=g.dependencies.size();contributions+=g.contributions.size();
      if (Overlap(g.payload,output) || Overlap(std::as_bytes(g.dependencies),output) || Overlap(std::as_bytes(g.contributions),output)) Fail("frame_group_overlap");
    }
    for (const auto &e:s.presentation.events) {size+=kW_StateAudioEventHeaderBytes+e.payload.size();if (Overlap(e.payload,output)) Fail("frame_event_overlap");}
    groups+=s.presentation.groups.size();events+=s.presentation.events.size();
  }
  if (size>output.size() || groups>kW_max_result_groups || events>kW_max_audio_events ||
      dependencies>kW_max_result_dependencies || contributions>kW_max_result_contributions) Fail("frame_output_or_count_budget");
  Limits(limits,std::size_t(size));Common(frame.scope,output);Values<StateFrameHeaderField> h{};
  At(h,StateFrameHeaderField::section_count)=frame.section_count;At(h,StateFrameHeaderField::flags)=frame.section_count==2?1:0;
  Write(kW_StateFrameHeader,h,output.subspan(kW_StateCommonHeaderBytes,kW_StateFrameHeaderBytes));
  auto position=kW_StateCommonHeaderBytes+kW_StateFrameHeaderBytes;auto remaining=limits;
  Visit(remaining,std::uint32_t(StateCommonHeaderField::count)+std::uint32_t(StateFrameHeaderField::count));
  for (unsigned i=0;i<frame.section_count;++i) {
    const auto &s=frame.sections[i];const auto &c=s.context;UniqueRecords(s.records,scratch.member_order);
    Values<StateSectionHeaderField> v{};At(v,StateSectionHeaderField::stream)=unsigned(c.stream);At(v,StateSectionHeaderField::flags)=c.base_version?0:1;
    At(v,StateSectionHeaderField::member_epoch)=c.member_epoch;At(v,StateSectionHeaderField::resource_epoch)=c.resource_epoch;
    At(v,StateSectionHeaderField::base_version)=c.base_version;At(v,StateSectionHeaderField::target_version)=c.target_version;
    At(v,StateSectionHeaderField::source_revision)=c.source_revision;At(v,StateSectionHeaderField::presentation_batch)=c.presentation_batch;
    At(v,StateSectionHeaderField::game_frame)=c.game_frame;At(v,StateSectionHeaderField::record_count)=s.records.size();
    At(v,StateSectionHeaderField::group_count)=s.presentation.groups.size();At(v,StateSectionHeaderField::event_count)=s.presentation.events.size();At(v,StateSectionHeaderField::event_watermark)=c.event_watermark;
    Write(kW_StateSectionHeader,v,output.subspan(position,kW_StateSectionHeaderBytes));position+=kW_StateSectionHeaderBytes;Visit(remaining,std::uint32_t(StateSectionHeaderField::count));
    for (const auto &r:s.records) {
      Record(r,c,context,scratch.presentation.body,remaining);Values<StateRecordHeaderField> rv{};
      At(rv,StateRecordHeaderField::member)=r.member;At(rv,StateRecordHeaderField::layout)=r.layout;
      At(rv,StateRecordHeaderField::block_mask)=r.mask;At(rv,StateRecordHeaderField::record_bytes)=r.bytes.size();
      Write(kW_StateRecordHeader,rv,output.subspan(position,kW_StateRecordHeaderBytes));position+=kW_StateRecordHeaderBytes;
      if (!r.bytes.empty()) std::memcpy(output.data()+position,r.bytes.data(),r.bytes.size());
      position+=r.bytes.size();Visit(remaining,std::uint32_t(StateRecordHeaderField::count));
    }
    auto local=remaining;local.bytes=limits.bytes-std::uint32_t(position);
    if (!local.bytes) local.bytes=remaining.bytes;
    std::uint32_t visits=0;
    const auto encoded=EncodePresentationBatchInto(c,s.presentation.groups,s.presentation.events,output.subspan(position),scratch.presentation,
        context.BodyContext(c,kNullIndex),{local},&visits);position+=encoded.size();
    // 复用正式编码检查的真实成本，不信调用者计数，也不重复解码全部结果。
    Visit(remaining,visits);
  }
  if (position!=size) Fail("frame_encoded_size");frame.encoded=output.first(position);frame.decoded=true;return frame.encoded;
}

StateContextReady DecodeStateContextReady(const StateScope &scope,std::span<const std::byte> bytes) {
  if (bytes.size()!=kW_StateCommonHeaderBytes+kW_StateContextReadyBytes) Fail("context_ready_size");
  Cursor input(bytes);Common(input,scope);const auto v=Read<StateContextReadyField>(kW_StateContextReady,input.Take(kW_StateContextReadyBytes));
  StateContextReady out{scope,std::uint32_t(v[std::size_t(StateContextReadyField::member_epoch)]),
      std::uint32_t(v[std::size_t(StateContextReadyField::resource_epoch)])};
  if (!out.member_epoch || !out.resource_epoch) Fail("context_ready_epoch");return out;
}
std::span<const std::byte> EncodeStateContextReadyInto(const StateContextReady &ready,std::span<std::byte> output) {
  constexpr auto size=kW_StateCommonHeaderBytes+kW_StateContextReadyBytes;
  if (output.size()<size || !ready.member_epoch || !ready.resource_epoch || Overlap(ObjectBytes(ready),output)) Fail("context_ready_output");
  Common(ready.scope,output);Values<StateContextReadyField> v{};At(v,StateContextReadyField::member_epoch)=ready.member_epoch;At(v,StateContextReadyField::resource_epoch)=ready.resource_epoch;
  Write(kW_StateContextReady,v,output.subspan(kW_StateCommonHeaderBytes,kW_StateContextReadyBytes));return output.first(size);
}
StateAppliedAck DecodeStateAppliedAck(const StateScope &scope,std::span<const std::byte> bytes) {
  Cursor input(bytes);Common(input,scope);const auto h=Read<StateAppliedAckHeaderField>(kW_StateAppliedAckHeader,input.Take(kW_StateAppliedAckHeaderBytes));
  StateAppliedAck ack;ack.scope=scope;ack.count=std::uint8_t(h[std::size_t(StateAppliedAckHeaderField::entry_count)]);
  if (!ack.count || ack.count>2 || h[std::size_t(StateAppliedAckHeaderField::flags)]!=(ack.count==2?1u:0u) ||
      bytes.size()!=kW_StateCommonHeaderBytes+kW_StateAppliedAckHeaderBytes+ack.count*kW_StateAppliedAckEntryBytes) Fail("ack_size_or_joint");
  for (unsigned i=0;i<ack.count;++i) {
    const auto v=Read<StateAppliedAckEntryField>(kW_StateAppliedAckEntry,input.Take(kW_StateAppliedAckEntryBytes));
    const auto get=[&](StateAppliedAckEntryField f){return v[std::size_t(f)];};
    ack.entries[i]={BodyStream(get(StateAppliedAckEntryField::stream)),std::uint32_t(get(StateAppliedAckEntryField::target_version)),
        std::uint32_t(get(StateAppliedAckEntryField::member_epoch)),std::uint32_t(get(StateAppliedAckEntryField::resource_epoch)),get(StateAppliedAckEntryField::event_watermark)};
  }
  Ack(ack);return ack;
}
std::span<const std::byte> EncodeStateAppliedAckInto(const StateAppliedAck &ack,std::span<std::byte> output) {
  Ack(ack);const auto size=kW_StateCommonHeaderBytes+kW_StateAppliedAckHeaderBytes+ack.count*kW_StateAppliedAckEntryBytes;
  if (output.size()<size || Overlap(ObjectBytes(ack),output)) Fail("ack_output");Common(ack.scope,output);
  Values<StateAppliedAckHeaderField> h{};At(h,StateAppliedAckHeaderField::entry_count)=ack.count;At(h,StateAppliedAckHeaderField::flags)=ack.count==2?1:0;
  Write(kW_StateAppliedAckHeader,h,output.subspan(kW_StateCommonHeaderBytes,kW_StateAppliedAckHeaderBytes));
  auto position=kW_StateCommonHeaderBytes+kW_StateAppliedAckHeaderBytes;
  for (unsigned i=0;i<ack.count;++i) {
    const auto &e=ack.entries[i];Values<StateAppliedAckEntryField> v{};At(v,StateAppliedAckEntryField::stream)=unsigned(e.stream);
    At(v,StateAppliedAckEntryField::target_version)=e.target;At(v,StateAppliedAckEntryField::member_epoch)=e.member_epoch;
    At(v,StateAppliedAckEntryField::resource_epoch)=e.resource_epoch;At(v,StateAppliedAckEntryField::event_watermark)=e.event_watermark;
    Write(kW_StateAppliedAckEntry,v,output.subspan(position,kW_StateAppliedAckEntryBytes));position+=kW_StateAppliedAckEntryBytes;
  }
  return output.first(size);
}
StateResync DecodeStateResync(const StateScope &scope,std::span<const std::byte> bytes) {
  if (bytes.size()!=kW_StateCommonHeaderBytes+kW_StateResyncHeaderBytes) Fail("resync_size");
  Cursor input(bytes);Common(input,scope);const auto v=Read<StateResyncHeaderField>(kW_StateResyncHeader,input.Take(kW_StateResyncHeaderBytes));
  return {scope,std::uint8_t(v[std::size_t(StateResyncHeaderField::stream_mask)]),StateResyncReason(v[std::size_t(StateResyncHeaderField::reason)]),
      std::uint32_t(v[std::size_t(StateResyncHeaderField::installed_world)]),std::uint32_t(v[std::size_t(StateResyncHeaderField::installed_body)])};
}
std::span<const std::byte> EncodeStateResyncInto(const StateResync &resync,std::span<std::byte> output) {
  constexpr auto size=kW_StateCommonHeaderBytes+kW_StateResyncHeaderBytes;
  if (output.size()<size || Overlap(ObjectBytes(resync),output)) Fail("resync_output");Common(resync.scope,output);Values<StateResyncHeaderField> v{};
  At(v,StateResyncHeaderField::stream_mask)=resync.streams;At(v,StateResyncHeaderField::reason)=unsigned(resync.reason);
  At(v,StateResyncHeaderField::installed_world)=resync.installed_world;At(v,StateResyncHeaderField::installed_body)=resync.installed_body;
  Write(kW_StateResyncHeader,v,output.subspan(kW_StateCommonHeaderBytes,kW_StateResyncHeaderBytes));return output.first(size);
}
StateAppliedAck StateFrameAck(const StateFrameView &frame) {
  StateAppliedAck ack;ack.scope=frame.scope;ack.count=frame.section_count;
  if (!ack.count || ack.count>2) Fail("frame_ack_count");
  for (unsigned i=0;i<ack.count;++i) {
    const auto &c=frame.sections[i].context;ack.entries[i]={c.stream,c.target_version,c.member_epoch,c.resource_epoch,c.event_watermark};
  }
  Ack(ack);return ack;
}
} // namespace cs::state
