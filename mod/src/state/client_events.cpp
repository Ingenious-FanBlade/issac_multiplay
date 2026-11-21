#include "state/client_events.h"
#include <algorithm>

namespace cs::state {
namespace {
[[noreturn]] void Fail(StateReadiness readiness,const char *why) {throw ClientShadowError(readiness,why);}
bool Same(const StateEventView &a,const StateEventView &b) {
  return a.sequence==b.sequence&&a.logical_frame==b.logical_frame&&a.owner==b.owner&&a.purpose==b.purpose&&
    a.flags==b.flags&&a.layout==b.layout&&a.payload.size()==b.payload.size()&&std::equal(a.payload.begin(),a.payload.end(),b.payload.begin());
}
const StateSectionView *World(const ClientStateCandidate &c) {
  for(unsigned i=0;i<c.Frame().section_count;++i)if(c.Frame().sections[i].context.stream==BodyStream::World)return &c.Frame().sections[i];
  return nullptr;
}
}
ClientStateEvents::ClientStateEvents(std::shared_ptr<ClientStateMemory> memory)
  :memory_(std::move(memory)),entries_(ClientStateAllocator<Entry>(memory_)) {entries_.reserve(512);}
void ClientStateEvents::Open(const StateRoute &route) {
  if(active_||!entries_.empty())Fail(StateReadiness::Lifecycle,"client_events_previous_route_busy");
  route_=route;active_=true;installed_=covered_=bytes_=0;first_=1;stats_={};
}
void ClientStateEvents::Close() noexcept {active_=false;entries_.clear();bytes_=0;}
void ClientStateEvents::Receive(const ClientStateCandidateRef &candidate) {
  if(!active_||!candidate||candidate->Target().route!=route_)Fail(StateReadiness::Lifecycle,"client_events_route");
  const auto *section=World(*candidate);if(!section)return;
  const auto &batch=section->presentation;const auto watermark=section->context.event_watermark;
  if(!batch.first_sequence||batch.first_sequence>watermark+1||watermark==UINT64_MAX)Fail(StateReadiness::InvalidEncoding,"client_events_range");
  if(section->context.base_version&&watermark>covered_&&batch.first_sequence>covered_+1)Fail(StateReadiness::EventGap,"client_events_missing_prefix");
  auto count=entries_.size();auto bytes=bytes_;std::array<bool,512> remove{},drop{};
  std::array<bool,512> incoming{};unsigned incoming_index=0;
  // 先核全部重复/冲突；之后才计算有界接管和外观淘汰，必要事件始终不能静默丢弃。
  for(const auto &value:batch.events) {
    const auto n=incoming_index++;
    if(value.sequence<=installed_)continue;
    const auto old=std::lower_bound(entries_.begin(),entries_.end(),value.sequence,[](const Entry &e,auto n){return e.event.value.sequence<n;});
    if(old!=entries_.end()&&old->event.value.sequence==value.sequence) {
      if(!Same(old->event.value,value))Fail(StateReadiness::InvalidEncoding,"client_event_sequence_conflict");
      // 保留首次接收事件的目录与正文。相同序号重传只比较原始值。
      continue;
    }
    if(value.sequence<=covered_) {
      // 已覆盖却没有条目的空隙只能是外观淘汰；迟到报文不能在其中补造必要事件。
      if(!StateEventCosmetic(StateEventKind(value.purpose)))Fail(StateReadiness::InvalidEncoding,"client_event_necessary_rewrites_gap");
      continue;
    }
    incoming[n]=true;++count;bytes+=StateEventHeaderBytes()+value.payload.size();
  }
  unsigned dropped=0;
  while(count>512||bytes+StateEventQueueHeaderBytes()>(256u<<10)) {
    bool found=false;
    for(unsigned i=0;i<entries_.size();++i)if(!remove[i]&&!entries_[i].event.replayed&&StateEventCosmetic(StateEventKind(entries_[i].event.value.purpose))) {
      remove[i]=true;--count;bytes-=StateEventHeaderBytes()+entries_[i].event.value.payload.size();found=true;++dropped;break;
    }
    if(!found)for(unsigned i=0;i<batch.events.size();++i)if(incoming[i]&&!drop[i]&&StateEventCosmetic(StateEventKind(batch.events[i].purpose))) {
      drop[i]=true;--count;bytes-=StateEventHeaderBytes()+batch.events[i].payload.size();found=true;++dropped;break;
    }
    if(!found)Fail(StateReadiness::Budget,"client_events_necessary_receive_budget");
  }
  // reserve已在构造时完成；下面的移动和shared_ptr复制不分配，冲突失败不留下半个批次。
  const auto world=candidate->State(BodyStream::World);
  for(std::size_t i=entries_.size();i>0;--i)if(remove[i-1])entries_.erase(entries_.begin()+i-1);
  incoming_index=0;
  for(const auto &value:batch.events)if(const auto n=incoming_index++;incoming[n]&&!drop[n]&&value.sequence>installed_) {
    const auto old=std::lower_bound(entries_.begin(),entries_.end(),value.sequence,[](const Entry &e,auto n){return e.event.value.sequence<n;});
    if(old!=entries_.end()&&old->event.value.sequence==value.sequence)continue;
    entries_.insert(old,Entry{{value,world->Members(),world->Resources(),false,false},candidate->packet_});
  }
  bytes_=bytes;stats_.dropped_capacity+=dropped;
  if(!covered_&&!installed_&&!section->context.base_version)first_=batch.first_sequence;
  else first_=std::min(first_,batch.first_sequence);
  covered_=std::max(covered_,watermark);
}
ClientStateVector<ClientReceivedEvent> ClientStateEvents::Prepare(const ClientStateCandidate &candidate,std::uint32_t frame) {
  ClientStateVector<ClientReceivedEvent> out{ClientStateAllocator<ClientReceivedEvent>(memory_)};
  const auto *section=World(candidate);if(!section)return out;
  if(!active_||candidate.Target().route!=route_||section->context.event_watermark>covered_)Fail(StateReadiness::EventGap,"client_events_target_not_covered");
  const auto watermark=section->context.event_watermark;
  out.reserve(entries_.size());
  for(auto &entry:entries_)if(entry.event.value.sequence<=watermark) {
    auto &event=entry.event;const auto age=std::uint32_t(frame-event.value.logical_frame);
    if(!event.replayed&&!event.expired&&StateEventCosmetic(StateEventKind(event.value.purpose))&&age<0x80000000u&&age>15) {
      event.expired=true;++stats_.dropped_expired;
    }
    if(!event.replayed&&!event.expired)out.push_back(event);
  }
  return out;
}
bool ClientStateEvents::Replayed(std::span<const std::uint64_t> sequences) noexcept {
  // 先核整个反馈，拒绝乱序或从未接收的序号；不能因后半错误只记前半已播放。
  // 目标在前一批仍在安装时就封存了事件清单，晚到的回执可能引用：
  //   已随前一批 Promote 擦除的序号（<=installed_）、被容量淘汰的外观事件（<=covered_ 且无条目，
  //   Receive 只淘汰外观事件），或本地已判过期的外观事件。原生侧已按水位去重，这里只做记账，不当协议错误。
  std::uint64_t previous=0;
  for(const auto n:sequences) {
    const auto e=std::lower_bound(entries_.begin(),entries_.end(),n,[](const Entry &e,auto value){return e.event.value.sequence<value;});
    const bool found=e!=entries_.end()&&e->event.value.sequence==n;
    if(n<=previous||(!found&&(n>covered_||!n)))return false;previous=n;
  }
  for(const auto n:sequences) {
    auto e=std::lower_bound(entries_.begin(),entries_.end(),n,[](const Entry &e,auto value){return e.event.value.sequence<value;});
    if(e==entries_.end()||e->event.value.sequence!=n)continue;
    if(!e->event.replayed){e->event.replayed=true;++stats_.replayed;}
  }
  return true;
}
bool ClientStateEvents::CanPromote(std::uint64_t watermark) const noexcept {
  if(!active_||watermark<installed_||watermark>covered_)return false;
  return std::none_of(entries_.begin(),entries_.end(),[&](const Entry &e){return e.event.value.sequence<=watermark&&!e.event.replayed&&!e.event.expired;});
}
bool ClientStateEvents::Promote(std::uint64_t watermark) noexcept {
  if(!CanPromote(watermark))return false;
  std::erase_if(entries_,[&](const Entry &e){if(e.event.value.sequence>watermark)return false;bytes_-=StateEventHeaderBytes()+e.event.value.payload.size();return true;});
  installed_=watermark;first_=std::max(first_,watermark+1);return true;
}
} // namespace cs::state
