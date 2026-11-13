#include "state/events.h"
#include <algorithm>
#include <cstring>

namespace cs::state {
namespace {
#include "state_host_fields.inc"
namespace F=host_field;
[[noreturn]] void Fail(const char *reason){throw Gap("state_events",reason);}
std::uint32_t Width(ScalarKind k){return k==ScalarKind::U8?1:k==ScalarKind::U16?2:k==ScalarKind::U32?4:k==ScalarKind::U64?8:0;}
std::uint32_t FixedSize(std::uint16_t layout){const auto *p=FindLayout(layout);if(!p||p->block_count!=1)Fail("header_plan");
  const auto &block=Blocks()[p->first_block];if(block.min_size!=block.max_size)Fail("header_plan");return block.min_size;}
class HeaderSource final:public BodyValueSource {
public:
  std::array<std::uint64_t,8> values{};
  std::span<const std::byte> Scalar(const CodecLocation &loc,ScalarKind kind,std::uint32_t count) const override {
    const auto width=Width(kind);if(!width||count!=1||!loc.parents.empty()||loc.field>=values.size()||(width<8&&values[loc.field]>>(8*width)))Fail("header_scalar");
    for(unsigned i=0;i<width;++i)bytes_[i]=std::byte(values[loc.field]>>(8*i));return std::span(bytes_).first(width);
  }
  std::uint32_t Count(const CodecLocation &)const override{Fail("header_container");}
  bool SharedPresent(const CodecLocation &)const override{Fail("header_shared");}
  std::uint32_t SlotMask(const CodecLocation &)const override{Fail("header_choice");}
  std::uint16_t SlotLayout(const CodecLocation &,std::uint8_t)const override{Fail("header_choice");}
private:mutable std::array<std::byte,8> bytes_{};
};
std::array<std::uint64_t,8> Header(std::uint16_t layout,std::span<const std::byte> bytes){
  std::array<BodyFieldView,8> fields{};BodyCodecScratch scratch;scratch.root_fields=fields;
  const auto body=DecodeValue(layout,bytes,scratch);std::array<std::uint64_t,8> values{};
  for(const auto &f:body.fields){if(!f.present||f.field>=8||f.bytes.empty()||f.bytes.size()>8)Fail("header_field");
    for(unsigned i=0;i<f.bytes.size();++i)values[f.field]|=std::uint64_t(std::to_integer<unsigned>(f.bytes[i]))<<(i*8);}
  return values;
}
void Header(std::uint16_t layout,const HeaderSource &source,std::span<std::byte> bytes){BodyCodecScratch scratch;EncodeValueInto(layout,source,bytes,scratch);}
void Event(const StateEventView &e){const auto kind=StateEventKind(e.purpose);
  if(!e.sequence||!StateEventLayout(kind)||e.layout!=StateEventLayout(kind)||e.flags!=unsigned(StateEventCosmetic(kind))||e.voice||e.generation||e.clear_scope)
    Fail("event_header");
}
bool Same(const StateEventView &a,const StateEventView &b){return a.sequence==b.sequence&&a.logical_frame==b.logical_frame&&a.owner==b.owner&&a.purpose==b.purpose&&a.layout==b.layout&&a.flags==b.flags&&
  std::equal(a.payload.begin(),a.payload.end(),b.payload.begin(),b.payload.end());}
}
bool StateEventCosmetic(StateEventKind kind) noexcept{return kind==StateEventKind::Sfx||kind==StateEventKind::ItemBanner||kind==StateEventKind::Fortune;}
std::uint16_t StateEventLayout(StateEventKind kind) noexcept {
  switch(kind){case StateEventKind::Sfx:return F::SfxPlayEvent::kLayout;case StateEventKind::ItemBanner:return F::ItemBannerEvent::kLayout;
    case StateEventKind::Fortune:return F::FortuneEvent::kLayout;case StateEventKind::BossIntro:return F::BossIntroEvent::kLayout;case StateEventKind::RoomTransition:return F::RoomTransitionEvent::kLayout;default:return 0;}
}
std::uint32_t StateEventHeaderBytes() noexcept{return 22;}
std::uint32_t StateEventQueueHeaderBytes() noexcept{return 22;}
StateEventSegment InspectStateEvents(const PresentationBatchContext &context,std::uint32_t count,std::span<const std::byte> bytes,std::span<StateEventView> storage){
  const auto qb=FixedSize(F::StateEventQueueHeader::kLayout),eb=FixedSize(F::StateEventHeader::kLayout);
  if(qb!=StateEventQueueHeaderBytes()||eb!=StateEventHeaderBytes()||context.stream!=BodyStream::World||bytes.size()<qb||count>storage.size()||count>512)Fail("segment_budget");
  const auto h=Header(F::StateEventQueueHeader::kLayout,bytes.first(qb));const auto size=h[F::StateEventQueueHeader::bytes];
  StateEventSegment segment{h[F::StateEventQueueHeader::first_sequence],h[F::StateEventQueueHeader::watermark],storage.first(count)};
  if(!segment.first_sequence||segment.watermark!=context.event_watermark||segment.watermark==UINT64_MAX||segment.first_sequence>segment.watermark+1||
     h[F::StateEventQueueHeader::count]!=count||size>bytes.size()-qb)Fail("segment_header");
  // 新baseline可以建立事件起点；增量不能把客户端尚未覆盖的前缀直接跳成已确认。
  if(context.base_version&&segment.watermark>context.installed_event_watermark&&
     segment.first_sequence>context.installed_event_watermark+1)Fail("segment_missing_prefix");
  std::size_t position=qb;auto previous=segment.first_sequence-1;
  for(unsigned i=0;i<count;++i){if(eb>qb+size-position)Fail("event_truncated");const auto e=Header(F::StateEventHeader::kLayout,bytes.subspan(position,eb));position+=eb;
    const auto payload=e[F::StateEventHeader::bytes];if(payload>qb+size-position)Fail("event_payload_truncated");
    auto &value=storage[i];value={};value.sequence=e[F::StateEventHeader::sequence];value.logical_frame=std::uint32_t(e[F::StateEventHeader::game_frame]);
    value.purpose=std::uint16_t(e[F::StateEventHeader::kind]);value.flags=std::uint8_t(e[F::StateEventHeader::flags]);value.owner=std::uint16_t(e[F::StateEventHeader::owner]);
    value.layout=std::uint16_t(e[F::StateEventHeader::layout]);value.payload=bytes.subspan(position,std::size_t(payload));value.consumed=eb+std::uint32_t(payload);Event(value);
    if(value.sequence<=previous||value.sequence>segment.watermark)Fail("event_order");previous=value.sequence;position+=std::size_t(payload);}
  if(position!=qb+size)Fail("event_trailing_bytes");segment.consumed=std::uint32_t(position);return segment;
}
StateEventSegment DecodeStateEvents(const PresentationBatchContext &context,std::uint32_t count,std::span<const std::byte> bytes,std::span<StateEventView> storage,
    BodyCodecScratch &scratch,const BodyCodecContext *body_context,BodyCodecLimits limits){
  auto segment=InspectStateEvents(context,count,bytes,storage);if(segment.consumed>limits.bytes)Fail("segment_byte_budget");
  std::uint32_t visits=4+count*7;
  for(unsigned i=0;i<count;++i){auto &event=storage[i];if(visits>limits.field_visits)Fail("segment_visit_budget");auto local=limits;local.field_visits-=visits;
    const auto body=DecodeValue(event.layout,event.payload,scratch,body_context,local);event.field_visits=body.field_visits;visits+=body.field_visits;}
  if(visits>limits.field_visits)Fail("segment_visit_budget");segment.field_visits=visits;return segment;
}
std::span<const std::byte> EncodeStateEvents(const PresentationBatchContext &context,const StateEventSegment &segment,std::span<std::byte> output,
    BodyCodecScratch &scratch,const BodyCodecContext *body_context,BodyCodecLimits limits,std::uint32_t &visits){
  const auto qb=FixedSize(F::StateEventQueueHeader::kLayout),eb=FixedSize(F::StateEventHeader::kLayout);std::uint64_t size=qb;
  if(context.stream!=BodyStream::World||segment.events.size()>512||!segment.first_sequence||segment.watermark!=context.event_watermark||segment.watermark==UINT64_MAX||segment.first_sequence>segment.watermark+1)Fail("segment_context");
  if(context.base_version&&segment.watermark>context.installed_event_watermark&&
     segment.first_sequence>context.installed_event_watermark+1)Fail("segment_missing_prefix");
  auto previous=segment.first_sequence-1;visits=4+std::uint32_t(segment.events.size())*7;
  for(const auto &event:segment.events){Event(event);if(event.sequence<=previous||event.sequence>segment.watermark)Fail("event_order");previous=event.sequence;size+=eb+event.payload.size();
    if(size>output.size()||size>limits.bytes||visits>limits.field_visits)Fail("segment_budget");auto local=limits;local.field_visits-=visits;
    visits+=DecodeValue(event.layout,event.payload,scratch,body_context,local).field_visits;}
  if(size>output.size()||size>limits.bytes||visits>limits.field_visits)Fail("segment_budget");
  HeaderSource h;h.values[F::StateEventQueueHeader::first_sequence]=segment.first_sequence;h.values[F::StateEventQueueHeader::watermark]=segment.watermark;
  h.values[F::StateEventQueueHeader::count]=segment.events.size();h.values[F::StateEventQueueHeader::bytes]=size-qb;Header(F::StateEventQueueHeader::kLayout,h,output.first(qb));
  std::size_t position=qb;for(const auto &event:segment.events){HeaderSource e;e.values[F::StateEventHeader::sequence]=event.sequence;e.values[F::StateEventHeader::game_frame]=event.logical_frame;
    e.values[F::StateEventHeader::kind]=event.purpose;e.values[F::StateEventHeader::flags]=event.flags;e.values[F::StateEventHeader::owner]=event.owner;
    e.values[F::StateEventHeader::layout]=event.layout;e.values[F::StateEventHeader::bytes]=event.payload.size();Header(F::StateEventHeader::kLayout,e,output.subspan(position,eb));position+=eb;
    if(!event.payload.empty())std::memcpy(output.data()+position,event.payload.data(),event.payload.size());position+=event.payload.size();}
  return output.first(position);
}
StateEventQueue::StateEventQueue(std::uint32_t count,std::uint32_t bytes){
  if(!count||count>512||bytes<StateEventQueueHeaderBytes()||bytes>256u<<10)throw std::invalid_argument("event_queue_capacity");
  memory_=memory::Reserve(memory::Pool::Values,sizeof(*this)+std::uint64_t(count)*sizeof(Entry)+bytes,"peer_state_events");entries_.resize(count);bytes_.resize(bytes);
}
void StateEventQueue::Reset(std::uint64_t baseline) noexcept {count_=used_=0;offered_=acked_=sent_=baseline;stats_={};}
void StateEventQueue::Remove(std::uint32_t i) noexcept {const auto size=std::uint32_t(entries_[i].value.payload.size()),offset=entries_[i].offset;
  std::memmove(bytes_.data()+offset,bytes_.data()+offset+size,used_-offset-size);used_-=size;
  std::move(entries_.begin()+i+1,entries_.begin()+count_,entries_.begin()+i);--count_;
  for(unsigned n=i;n<count_;++n){auto &entry=entries_[n];entry.offset-=size;entry.value.payload=std::span<const std::byte>(bytes_).subspan(entry.offset,entry.value.payload.size());}}
bool StateEventQueue::Offer(const StateEventView &value) noexcept {
  try {Event(value);}catch(...){return false;}
  if(value.sequence==UINT64_MAX)return false;
  if(value.sequence<=acked_)return true;
  for(unsigned i=0;i<count_;++i)if(entries_[i].value.sequence==value.sequence)return Same(entries_[i].value,value);
  if(value.sequence<=offered_)return StateEventCosmetic(StateEventKind(value.purpose));
  const auto required=std::uint64_t(StateEventQueueHeaderBytes())+StateEventHeaderBytes()+value.payload.size();
  if(required>bytes_.size()){if(StateEventCosmetic(StateEventKind(value.purpose))){offered_=value.sequence;++stats_.dropped_capacity;return true;}return false;}
  // 条目头和队列头同样占256KiB预算，不能用小payload绕过容量限制。
  while(count_==entries_.size()||required+used_+std::uint64_t(count_)*StateEventHeaderBytes()>bytes_.size()){unsigned oldest=count_;
    for(unsigned i=0;i<count_;++i)if(StateEventCosmetic(StateEventKind(entries_[i].value.purpose))){oldest=i;break;}
    if(oldest==count_){if(StateEventCosmetic(StateEventKind(value.purpose))){offered_=value.sequence;++stats_.dropped_capacity;return true;}return false;}
    Remove(oldest);++stats_.dropped_capacity;}
  auto &entry=entries_[count_++];entry={value,used_};if(!value.payload.empty())std::memcpy(bytes_.data()+used_,value.payload.data(),value.payload.size());
  entry.value.payload=std::span<const std::byte>(bytes_).subspan(used_,value.payload.size());used_+=std::uint32_t(value.payload.size());offered_=value.sequence;return true;
}
bool StateEventQueue::CanCover(std::uint64_t first,std::uint64_t watermark) const noexcept {
  return first&&offered_!=UINT64_MAX&&watermark!=UINT64_MAX&&first<=watermark+1&&first<=offered_+1&&watermark>=offered_;
}
bool StateEventQueue::Cover(std::uint64_t first,std::uint64_t watermark) noexcept {
  if(!CanCover(first,watermark))return false;
  offered_=watermark;return true;
}
StateEventSegment StateEventQueue::Select(std::span<StateEventView> out,std::uint32_t budget,std::uint64_t through) const noexcept {
  StateEventSegment result;result.first_sequence=acked_==UINT64_MAX?UINT64_MAX:acked_+1;result.watermark=acked_;std::uint32_t size=StateEventQueueHeaderBytes(),count=0;
  if(budget<size)return result;
  for(unsigned i=0;i<count_;++i){const auto &event=entries_[i].value;if(event.sequence>through)break;const auto bytes=std::uint64_t(StateEventHeaderBytes())+event.payload.size();
    if(count==out.size()||bytes>budget-size)break;out[count++]=event;size+=std::uint32_t(bytes);result.watermark=event.sequence;}
  if(count==count_)result.watermark=offered_;else if(count<count_)result.watermark=entries_[count].value.sequence-1;
  result.watermark=std::max(acked_,std::min(result.watermark,through));
  result.events=out.first(count);result.consumed=size;return result;
}
bool StateEventQueue::Sent(std::uint64_t watermark) noexcept {if(watermark<acked_||watermark>offered_)return false;sent_=std::max(sent_,watermark);return true;}
bool StateEventQueue::Ack(std::uint64_t watermark) noexcept {if(watermark<acked_||watermark>sent_)return false;acked_=watermark;
  while(count_&&entries_[0].value.sequence<=watermark)Remove(0);return true;}
void StateEventQueue::Expire(std::uint32_t frame,std::uint32_t maximum_age) noexcept {
  for(unsigned i=0;i<count_;){const auto &event=entries_[i].value;const auto age=std::uint32_t(frame-event.logical_frame);
    if(StateEventCosmetic(StateEventKind(event.purpose))&&age<0x80000000u&&age>maximum_age){Remove(i);++stats_.dropped_expired;}else ++i;}
}
bool QueueCapturedStateEvents(const CapturePacket &packet,StateEventQueue &queue,CaptureLinkResolver resolver,std::span<std::byte> links,
    std::span<std::byte> encoded,BodyCodecScratch &scratch,const BodyCodecContext *context,BodyCodecLimits limits){
  if(!queue.CanCover(packet.event_first_sequence,packet.event_watermark))return false;
  std::array<std::uint32_t,512> order{};unsigned count=0;
  for(std::uint32_t i=0;i<packet.records.size();++i)if(packet.records[i].event_sequence){if(count==order.size())return false;order[count++]=i;}
  std::sort(order.begin(),order.begin()+count,[&](auto a,auto b){return packet.records[a].event_sequence<packet.records[b].event_sequence;});
  auto previous=packet.event_first_sequence-1;
  for(unsigned n=0;n<count;++n){const auto i=order[n];const auto &record=packet.records[i];
    if(record.event_sequence<=previous||record.event_sequence>packet.event_watermark)return false;previous=record.event_sequence;
    StateEventKind kind{};for(unsigned k=1;k<=5;++k)if(StateEventLayout(StateEventKind(k))==record.layout)kind=StateEventKind(k);
    if(!StateEventLayout(kind)||(record.production.phase!=CapturePhase::Production&&record.production.phase!=CapturePhase::IntentionBinding)||
       record.production.scope.session!=packet.witness.scope.session||record.production.scope.run!=packet.witness.scope.run||
       record.generation!=packet.witness.generation||record.instance!=record.event_sequence||record.purpose!=record.layout||
       record.stream!=BodyStream::Value||record.pass!=unsigned(StateEventCosmetic(kind)))return false;
    // 已确认事件不再解析其旧room成员／资源；latest包重带它们不能拖住新world。
    if(queue.Covered(record.event_sequence))continue;
    CapturedBodySource source(packet,i,resolver,links);const auto body=EncodeValueInto(record.layout,source,encoded,scratch,context,limits);
    StateEventView event;event.sequence=record.event_sequence;event.logical_frame=record.event_game_frame;event.owner=kNullIndex;event.purpose=unsigned(kind);
    event.voice=event.generation=0;event.clear_scope=0;event.flags=std::uint8_t(StateEventCosmetic(kind));event.layout=record.layout;event.payload=body.bytes;
    if(!queue.Offer(event))return false;
  }
  return queue.Cover(packet.event_first_sequence,packet.event_watermark);
}
}
