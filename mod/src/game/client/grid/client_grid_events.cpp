#include "game/client/grid/client_grid_events.h"
#include "replica/codec.h"
#include <algorithm>
#include <cstring>

namespace cs::game {
void ClientGridEvents::Reset(const replica::Scope &scope) noexcept {entries_.clear();bytes_=0;retired_sequence_=0;scope_=scope;}
// codec负责严格报文结构；保留原 source_scope/capture_kind/required_state，绝不借普通 tick。
bool ClientGridEvents::Admit(const replica::Message &message,const char *&reason) noexcept {
  try{
    if(message.kind!=replica::Kind::Component){reason="grid_independent_component_required";return false;}
    const auto encoded=replica::Encode(message);const auto &b=message.body;
    if(replica::ReadScope(b["scope"])!=scope_){reason="grid_independent_stale_scope";return false;}
    if(replica::Unsigned(b["domain"])!=unsigned(world::observation::DomainId::Grid)||std::strcmp(b["coverage"].GetString(),"Complete")){
      reason="grid_independent_domain_or_coverage";return false;
    }
    const std::string phase=b["capture_kind"].GetString();
    if(phase!="Lifecycle"&&phase!="Render"&&phase!="ActualCall"){reason="grid_independent_phase";return false;}
    const auto source=replica::Unsigned(b["source_scope"]),required=replica::Unsigned(b["required_state"]);
    if(!message.sequence||encoded.size()>65536){reason="grid_independent_budget";return false;}
    for(const auto &e:entries_)if(e.sequence==message.sequence&&e.source_scope==source&&e.phase==phase){
      const bool same=e.value==b;reason=same?"grid_independent_duplicate":"grid_independent_conflict";return same;
    }
    // 完成的历史只在容量需要时淘汰；未消费记录绝不静默丢弃，也不增加虚假水位。
    if(message.sequence<=retired_sequence_){reason="grid_independent_retired_sequence";return false;}
    while((entries_.size()>=64||bytes_+encoded.size()>(1u<<20))&&!entries_.empty()&&entries_.front().complete){
      retired_sequence_=(std::max)(retired_sequence_,entries_.front().sequence);bytes_-=entries_.front().bytes;entries_.pop_front();}
    if(entries_.size()>=64||bytes_+encoded.size()>(1u<<20)){reason="grid_independent_backpressure";return false;}
    Entry e;e.sequence=message.sequence;e.source_scope=source;e.required=required;e.phase=phase;
    e.value=replica::Copy(b);e.bytes=encoded.size();entries_.push_back(std::move(e));bytes_+=encoded.size();
    reason="grid_independent_accepted_not_presented";return true;
  }catch(...){reason="grid_independent_invalid_or_allocation";return false;}
}
bool ClientGridEvents::Visit(std::uint64_t applied,void *context,Visitor visitor) const noexcept {
  if(!visitor||!applied)return false;
  for(const auto &e:entries_)if(!e.complete&&e.required&&e.required<=applied){if(!visitor(context,e.value,e.sequence))return false;e.offered=true;}
  // required_state=0 的历史没有声明可应用版本；保留供27明确绑定，不能自行套当前场景。
  return true;
}
bool ClientGridEvents::Complete(std::uint64_t sequence,std::uint64_t source,const char *phase) noexcept {
  if(!phase)return false;
  for(auto &e:entries_)if(e.sequence==sequence&&e.source_scope==source&&e.phase==phase&&e.offered){e.complete=true;return true;}return false;
}
unsigned ClientGridEvents::Pending() const noexcept {unsigned n=0;for(const auto &e:entries_)n+=!e.complete;return n;}
} // namespace cs::game
