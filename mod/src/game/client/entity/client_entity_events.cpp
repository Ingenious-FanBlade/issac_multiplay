#include "game/client/entity/client_entity_events.h"
#include "replica/codec.h"
#include <algorithm>
#include <string_view>

namespace cs::game {
void ClientEntityEvents::Reset(const replica::Scope &scope) noexcept {scope_=scope;entries_.clear();bytes_=0;retired_component_=retired_event_=baseline_event_=0;}
bool ClientEntityEvents::Admit(const replica::Message &message,const char *&reason) noexcept {
  try{
    if(message.kind!=replica::Kind::Component&&message.kind!=replica::Kind::Event){reason="entity_independent_kind";return false;}
    const auto encoded=replica::Encode(message);const auto &b=message.body;
    if(replica::ReadScope(b["scope"])!=scope_){reason="entity_independent_stale_scope";return false;}
    if(message.kind==replica::Kind::Event&&replica::Unsigned(b["event_id"])<=baseline_event_){reason="entity_event_represented_by_committed_baseline";return true;}
    Entry next;auto &r=next.receipt;r.kind=message.kind;r.sequence=message.sequence;r.required_state=replica::Unsigned(b["required_state"]);
    if(r.kind==replica::Kind::Component){
      if(replica::Unsigned(b["domain"])!=unsigned(world::observation::DomainId::Entity)||std::string_view(b["coverage"].GetString())!="Complete"){reason="entity_independent_domain_or_coverage";return false;}
      r.source_scope=replica::Unsigned(b["source_scope"]);r.phase=b["capture_kind"].GetString();
      if(r.phase!="Render"&&r.phase!="ActualCall"&&r.phase!="Lifecycle"){reason="entity_independent_phase";return false;}
    }else {r.phase=b["kind"].GetString();next.starts=replica::Unsigned(b["server_tick"]);next.expires=replica::Unsigned(b["expires_tick"]);}
    if(!message.sequence||encoded.size()>(256u<<10)){reason="entity_independent_record_budget";return false;}
    for(const auto &e:entries_)if(e.receipt.kind==r.kind&&e.receipt.sequence==r.sequence){
      const bool same=e.body==b;reason=same?"entity_independent_duplicate_not_replayed":"entity_independent_sequence_conflict";return same;}
    auto &floor=r.kind==replica::Kind::Component?retired_component_:retired_event_;
    if(r.sequence<=floor){reason="entity_independent_retired_sequence";return false;}
    // 只回收有明确终态的历史；未满足依赖的记录背压，不以丢弃冒充已播放。
    while((entries_.size()>=128||bytes_+encoded.size()>(2u<<20))&&!entries_.empty()&&entries_.front().state!=State::Pending){
      const auto &e=entries_.front();auto &old=e.receipt.kind==replica::Kind::Component?retired_component_:retired_event_;
      old=(std::max)(old,e.receipt.sequence);bytes_-=e.bytes;entries_.pop_front();}
    if(entries_.size()>=128||bytes_+encoded.size()>(2u<<20)){reason="entity_independent_backpressure";return false;}
    // Render/Query只能借其明确shape的当前主体；Lifecycle是历史结果，短命主体不因此复建。
    if(r.kind==replica::Kind::Event||r.phase!="Lifecycle"){
      const auto walk=[&](auto &&self,const replica::Value &v)->void {
        if(v.IsArray()){for(const auto &x:v.GetArray())self(self,x);return;}if(!v.IsObject())return;
        if(v.HasMember("type")&&v["type"].IsString()&&std::string_view(v["type"].GetString())=="reference"){
          if(!v.HasMember("status")||!v["status"].IsString()){next.unresolved=true;return;}
          const std::string_view status=v["status"].GetString();if(status=="Null")return;
          if(status!="KnownCurrent"||!v.HasMember("target")||!v.HasMember("shape_revision")){next.unresolved=true;return;}
          const auto &key=v["target"];if(!key.IsObject()||!key.HasMember("run")||!key.HasMember("id")||!key.HasMember("space")){next.unresolved=true;return;}
          Dependency d;d.key={replica::Unsigned(key["run"]),replica::Unsigned(key["id"]),unsigned(replica::Unsigned(key["space"]))};d.shape=replica::Unsigned(v["shape_revision"]);
          // shape=0是合法初始用途；仅Key/shape同时匹配才允许借当前实例。
          if(d.key.run!=scope_.run||d.key.space!=1||!d.key.id||next.dependencies.size()>=4096)next.unresolved=true;else next.dependencies.push_back(d);return;
        }for(auto i=v.MemberBegin();i!=v.MemberEnd();++i)self(self,i->value);
      };walk(walk,r.kind==replica::Kind::Event?b["parameters"]:b["value"]);
      // 独立事件可只含Authority最终坐标/音效结果；有显式主体时必须满足同一Key/shape。
      if(r.kind==replica::Kind::Component&&next.dependencies.empty())next.unresolved=true;
    }
    next.body=replica::Copy(b);next.bytes=encoded.size();entries_.push_back(std::move(next));bytes_+=encoded.size();reason="entity_independent_accepted_not_presented";return true;
  }catch(...){reason="entity_independent_invalid_or_allocation";return false;}
}
bool ClientEntityEvents::Visit(std::uint64_t applied,std::uint64_t tick,void *resolver,Resolve resolve,void *context,Visitor visitor) noexcept {
  if(!applied||!visitor||!resolve)return false;
  for(auto &e:entries_){if(e.state!=State::Pending)continue;const auto &r=e.receipt;
    if(e.expires&&tick>e.expires){e.state=State::Expired;e.offered_at=0;continue;}
    if(r.kind==replica::Kind::Component&&r.phase!="Lifecycle"&&r.required_state&&r.required_state<applied){e.state=State::Superseded;e.offered_at=0;continue;}
    if(e.unresolved||r.required_state>applied||tick<e.starts)continue;
    if(r.kind==replica::Kind::Component&&r.phase!="Lifecycle"&&r.required_state!=applied)continue;
    if(!std::all_of(e.dependencies.begin(),e.dependencies.end(),[&](const auto &d){return resolve(resolver,d.key,d.shape);}))continue;
    if(!visitor(context,e.body,r))return false;e.offered_at=applied;
  }return true;
}
bool ClientEntityEvents::Complete(replica::Kind kind,std::uint64_t sequence,std::uint64_t source,const char *phase,std::uint64_t applied) noexcept {
  if(!phase||!applied)return false;
  for(auto &e:entries_)if(e.receipt.kind==kind&&e.receipt.sequence==sequence&&e.receipt.source_scope==source&&e.receipt.phase==phase&&e.state==State::Pending&&e.offered_at==applied){e.state=State::Presented;return true;}return false;
}
unsigned ClientEntityEvents::Pending() const noexcept {unsigned n=0;for(const auto &e:entries_)n+=e.state==State::Pending;return n;}
ClientEntityEvents::Statistics ClientEntityEvents::Status() const noexcept {
  Statistics out;out.bytes=bytes_;for(const auto &e:entries_)switch(e.state){
    case State::Pending:++out.pending;break;case State::Presented:++out.presented;break;
    case State::Expired:++out.expired;break;case State::Superseded:++out.superseded;break;
  }return out;
}
} // namespace cs::game
