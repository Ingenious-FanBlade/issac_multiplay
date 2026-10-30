#include "game/world_bridge.h"
#include "game/world_sources/lua_entity.h"
#include "world/compatibility/encode.h"
#include "binding/world_layout.h"
#include "game/world_sources/value_source.h"
#include "runtime.h"
#include <cstring>
namespace cs::game {
namespace {
namespace obs=world::observation;
// 所有服务请求发生在 Manager 主线程边界；后台只消费 PublicationQueue 的自有值。
obs::CapturePlan Plan() {
  obs::CapturePlan p;
  p.domains.push_back({obs::DomainId::Entity,1u|4u|16u|32u,30,{}});
  p.domains.push_back({obs::DomainId::Grid,1u|4u,30,{}});
  return p;
}
void Latest(void *owner,const obs::ObservationEnvelope &e) noexcept {
  auto &c=*static_cast<WorldCapture *>(owner);
  try {
    if(e.kind!=obs::RecordKind::State) return;
    if(e.schema=="isaac.cs.entity") c.latest_entity=e;
    else if(e.domain==obs::DomainId::Grid) c.latest_grid=e;
  } catch(...) { ++c.formal_loss; }
}
void Record(void *owner,const obs::ObservationEnvelope &e) noexcept {
  auto &c=*static_cast<WorldCapture *>(owner);
  try { if(!c.publications.Push(obs::EncodeEnvelope(e))) Trace("observation_record_loss","epoch=%llu loss=%llu",c.epoch,c.publications.LossSequence()); }
  catch(...) { c.publications.NoteLoss(); }
}
bool Scheduled(const WorldCapture &c,obs::DomainId domain,std::uint64_t capture) noexcept {
  if(!c.observation.Needed(domain,obs::Phase::OrdinaryPost)) return false;
  for(const auto &d:c.observation.Plan().domains) if(d.domain==domain) return capture%(d.interval?d.interval:1)==0;
  return false;
}
obs::ObservationEnvelope Base(WorldCapture &c,obs::DomainId domain,const char *schema,const world::BatchHeader &h) {
  obs::ObservationEnvelope e;
  e.domain=domain; e.schema=schema;
  e.context=c.observation.BeginScope(obs::Phase::OrdinaryPost,1);
  e.context.authority_tick={obs::Presence::Known,h.tick};
  e.context.outer=h.outer; e.context.owner=h.scope;
  e.context.mutation_before=e.context.mutation_after=h.mutation;
  e.context.event_watermark=h.event_watermark;
  if(h.run) e.context.run={obs::Presence::Known,{c.session_key,c.epoch,h.run,obs::IdentitySpace::Run}};
  if(h.room) e.context.room={obs::Presence::Known,{c.session_key,c.epoch,h.room,obs::IdentitySpace::RoomInstance}};
  return e;
}
void Lua(void *owner,const world::BatchHeader &h,const world::RawObject *objects) noexcept {
  auto &c=*static_cast<WorldCapture *>(owner);
  if(!Scheduled(c,obs::DomainId::Entity,h.capture)) return;
  try {
    EnsureObservationLuaBridge();
    auto e=Base(c,obs::DomainId::Entity,"isaac.cs.entity",h);
    e.context.source=2;
    c.lua_objects=objects; c.lua_object_count=h.object_count; c.lua_scope=e.context.scope;
    for(const auto &d:c.observation.Plan().domains) if(d.domain==obs::DomainId::Entity) e.result.budget=d.budget;
    const auto budget=e.result.budget;
    auto result=CaptureLuaEntity(budget.objects,budget.nodes,budget.depth,std::uint32_t(budget.bytes)); ++c.lua_attempts;
    c.lua_objects=nullptr; c.lua_object_count=0;
    if(result.available&&result.complete) ++c.lua_complete; else ++c.lua_partial;
    e.result.coverage=result.available?(result.complete?obs::Coverage::Complete:obs::Coverage::Partial):obs::Coverage::Failed;
    e.result.range="public-entity-v2:state-and-finite-queries;unbounded-derived-queries:on-request;external-domains:explicit-references";
    if(!result.reason.empty()) e.result.reasons.push_back(result.reason);
    e.result.objects=result.objects; e.result.nodes=result.nodes; e.result.bytes=result.payload.size();
    e.payload=std::move(result.payload);
    const auto scope=e.context.scope;
    if(c.observation.PrepareScope(std::move(e)).accepted) c.pending_scopes[0]=scope;
  } catch(...) { c.lua_objects=nullptr; c.lua_object_count=0; c.observation.Invalidate("lua_scope_budget"); }
}
void Native(void *owner,const world::BatchHeader &h,const world::RawObject *objects,const world::RawMembership *members) noexcept {
  auto &c=*static_cast<WorldCapture *>(owner);
  try {
    for(unsigned d=0;d<2;++d) {
      const auto domain=d?obs::DomainId::Grid:obs::DomainId::Entity;
      if(!Scheduled(c,domain,h.capture)) continue;
      auto e=Base(c,domain,d?"isaac.cs.grid.legacy":"isaac.cs.entity.native-v1",h);
      bool complete=true;
      for(unsigned source=d?11:0;source<=(d?12u:10u);++source)
        if(h.sources[source].status&&h.sources[source].status!=world::kSourceNotApplicable) complete=false;
      for(unsigned i=0;i<h.object_count;++i) if(objects[i].domain==d) {
        const auto &object=objects[i];
        if(object.kind==unsigned(world::Class::Unknown)) complete=false;
        if(object.rng.before_status!=1||object.rng.after_status!=1||object.rng.before!=object.rng.after) complete=false;
        for(unsigned f=0;f<object.field_count;++f) {
          const auto &field=object.fields[f];
          if(field.status!=world::FieldStatus::Value&&field.status!=world::FieldStatus::NotApplicable) complete=false;
          if(field.status==world::FieldStatus::Value&&field.encoding==world::Encoding::F32) {
            std::uint32_t bits=0; std::memcpy(&bits,field.raw.data(),4);
            if((bits&0x7f800000u)==0x7f800000u) complete=false;
          }
          if(field.status==world::FieldStatus::Value&&field.encoding==world::Encoding::Reference&&field.relation!=world::Relation::Null&&field.relation!=world::Relation::KnownCurrent) complete=false;
        }
      }
      e.result.coverage=complete?obs::Coverage::Complete:obs::Coverage::Partial;
      e.result.range=d?"min-read-v1:grid-public-and-door-only;full-grid:tx-04-11":"min-read-v1:entity-public-and-seven-derived-classes";
      if(!complete) e.result.reasons.push_back("native_domain_fields_sources_or_rng_incomplete");
      for(unsigned i=0;i<h.object_count;++i) if(objects[i].domain==d) ++e.result.objects;
      bool truncated=false;
      e.payload=world::compatibility::EncodeDomain(h,objects,members,static_cast<world::ObjectDomain>(d),truncated); e.result.bytes=e.payload.size();
      if(truncated) { e.result.coverage=obs::Coverage::Partial; e.result.reasons.push_back("native_payload_budget"); }
      const auto scope=e.context.scope;
      if(c.observation.PrepareScope(std::move(e)).accepted) c.pending_scopes[d+1]=scope;
    }
  } catch(...) { c.observation.Invalidate("native_projection_budget"); }
}
void Commit(void *owner,bool committed,bool stable) noexcept {
  auto &c=*static_cast<WorldCapture *>(owner);
  for(auto &scope:c.pending_scopes) if(scope) {
    c.observation.CommitScope(scope,!stable?obs::Outcome::Invalidated:committed?obs::Outcome::AuthorityCommitted:obs::Outcome::Cancelled); scope=0;
  }
}
void Event(void *owner,const world::LifecycleEvent &event) noexcept {
  auto &c=*static_cast<WorldCapture *>(owner);
  const auto domain=event.domain?obs::DomainId::Grid:obs::DomainId::Entity;
  if(!c.observation.Needed(domain,obs::Phase::Lifecycle)) return;
  try {
    obs::ObservationEnvelope e; e.domain=domain; e.schema=event.domain?"isaac.cs.grid.lifecycle":"isaac.cs.entity.lifecycle";
    e.kind=obs::RecordKind::Event; e.context=c.observation.BeginScope(obs::Phase::Lifecycle,1);
    e.context.outer=event.outer; e.context.event_watermark=event.sequence;
    e.context.mutation_before=e.context.mutation_after=event.mutation;
    e.result.coverage=event.exceptional?obs::Coverage::Partial:obs::Coverage::Complete; e.result.range="native-observed-lifecycle-and-call-pair";
    e.payload="{\"local_id\":\""+std::to_string(event.local_id)+"\",\"use\":\""+std::to_string(event.use)+"\",\"revision\":\""+std::to_string(event.revision)+"\",\"origin_readiness\":"+std::to_string(event.readiness)+",\"kind\":"+std::to_string(unsigned(event.kind))+",\"observation\":"+std::to_string(event.observation)+",\"call\":"+std::to_string(event.call)+",\"parent_call\":"+std::to_string(event.parent)+",\"entering\":"+std::to_string(event.entering)+",\"exceptional\":"+std::to_string(event.exceptional)+",\"candidate_tick\":\""+std::to_string(event.tick)+"\",\"memberships\":"+std::to_string(event.memberships)+"}";
    e.result.bytes=e.payload.size(); const auto scope=e.context.scope;
    if(c.observation.PrepareScope(std::move(e)).accepted) c.observation.CommitScope(scope,obs::Outcome::IndependentPublished);
  } catch(...) { c.observation.Invalidate("event_budget"); }
}
void Gap(void *owner,const world::GapRecord &gap) noexcept {
  auto &c=*static_cast<WorldCapture *>(owner);
  if(!c.observation.Active()) return;
  try {
    obs::ObservationEnvelope e; e.schema="isaac.cs.entity.coverage"; e.kind=obs::RecordKind::Gap;
    // Gap 没有完成普通窗口；保留真实 outer 和候选 tick，而不是伪造完整 OrdinaryPost。
    e.context=c.observation.BeginScope(obs::Phase::Lifecycle,1); e.context.outer=gap.outer;
    e.context.event_watermark=gap.event_watermark; e.result.coverage=obs::Coverage::NotScheduled;
    e.result.range="ordinary-native-window"; e.result.reasons.push_back(world::ReasonName(gap.reason));
    e.payload="{\"candidate_tick\":\""+std::to_string(gap.tick)+"\",\"attempt\":\""+std::to_string(gap.attempt)+"\",\"authority_kind\":"+std::to_string(gap.authority_kind)+"}";
    const auto scope=e.context.scope;
    if(c.observation.PrepareScope(std::move(e)).accepted) c.observation.CommitScope(scope,obs::Outcome::IndependentPublished);
  } catch(...) { c.observation.Invalidate("gap_budget"); }
}
}
void WorldCapture::ConnectSources() noexcept { collector->SetCaptureSink({this,Lua,Native,Commit,Event,Gap}); }
void WorldCapture::SetConsumers(bool formal,bool recorder) noexcept {
  if(formal!=formal_consumer) {
    const auto result=formal?observation.AttachConsumer({1,this,Latest,Plan()}):observation.DetachConsumer(1);
    // 请求遭遇在途 scope 时保留原状态，下一主线程边界重试，不能伪报已接入。
    if(result.accepted) formal_consumer=formal;
  }
  if(recorder!=recorder_consumer) {
    const auto result=recorder?observation.AttachConsumer({2,this,Record,Plan()}):observation.DetachConsumer(2);
    if(result.accepted) recorder_consumer=recorder;
  }
  ArmObservationValueSources(observation.Active()&&(formal_consumer||recorder_consumer));
}
bool ObservationLuaNeeded(const char *phase) noexcept {
  auto c=CurrentWorldRecording(); if(!IsMain()||!c||!c->observation.Active()) return false;
  return c->observation.Needed(world::observation::DomainId::Entity,phase&&!std::strcmp(phase,"Lifecycle")?world::observation::Phase::Lifecycle:phase&&!std::strcmp(phase,"Render")?world::observation::Phase::Render:world::observation::Phase::ActualCall);
}
ObservationSubjectValue ObservationSubject(void *object,bool registering) noexcept {
  ObservationSubjectValue result;
  auto c=CurrentWorldRecording(); if(!IsMain()||!c||!c->observation.Active()||!object) return result;
  const auto address=static_cast<std::uint32_t>(reinterpret_cast<uintptr_t>(object));
  auto *id=registering?c->identities.FindOrCreate(world::ObjectDomain::Entity,address):c->identities.Find(world::ObjectDomain::Entity,address);
  result.session=c->session_key; result.epoch=c->epoch;
  if(!id) return result;
  if(registering) id->MarkFirstObserved();
  result.local=id->local_id; result.use=id->use; result.revision=id->revision; result.origin=id->origin; result.readiness=id->readiness;
  for(std::uint32_t slot=0;slot<world::kControlAnchorCapacity;++slot) {
    const auto anchor=c->collector->CurrentAnchor(slot);
    if(!anchor.valid||anchor.player!=address) continue;
    const auto ordinal=slot/world::kControlBodiesPerParticipant;
    if(c->control_sessions[ordinal]!=anchor.session) {
      c->control_sessions[ordinal]=anchor.session;
      c->participant_keys[ordinal]=c->observation.AllocateRelatedSubject(obs::IdentitySpace::Participant);
      c->control_group_keys[ordinal]=c->observation.AllocateRelatedSubject(obs::IdentitySpace::ControlGroup);
    }
    result.ordinal=ordinal; result.participant=c->participant_keys[ordinal].local; result.group=c->control_group_keys[ordinal].local;
    result.control_session=anchor.session; result.binding_revision=anchor.generation; break;
  }
  if(registering&&c->lua_objects&&id->local_id<c->lua_subject_scopes.size()) c->lua_subject_scopes[id->local_id]=c->lua_scope;
  const bool current=c->lua_objects&&id->local_id<c->lua_subject_scopes.size()&&c->lua_subject_scopes[id->local_id]==c->lua_scope;
  // v2 当前 Lua 引用还需本 scope 的主体登记；复用地址仅在真实创建 owner 已完成且
  // 本 use 没有 Morph/未知 Init 修订时恢复当前资格。旧 v1 的强出生排序规则保持原样。
  const bool reuse_current=id->born&&id->readiness>=2&&id->revision==0&&id->current_public_ready;
  result.status=id->Retired()?6:registering?1:current&&(!id->reused||reuse_current)?1:id->reused?7:3;
  return result;
}
std::uint32_t ObservationEntities(void *owner,void (*visit)(void *,void *,std::uint32_t) noexcept) noexcept {
  auto c=CurrentWorldRecording(); if(!IsMain()||!c||!c->lua_objects||!visit) return 0;
  std::uint32_t unavailable=0;
  for(std::uint32_t i=0;i<c->lua_object_count;++i) {
    const auto &o=c->lua_objects[i]; if(o.domain!=unsigned(world::ObjectDomain::Entity)) continue;
    auto *id=c->identities.Find(world::ObjectDomain::Entity,o.address);
    if(!id||id->Retired()) continue;
    // 原生基础复制尚未取得资格时，不构造可调用派生 Getter 的 userdata。
    if(!o.field_count||!o.vptr) { ++unavailable; continue; }
    visit(owner,reinterpret_cast<void *>(uintptr_t(o.address)),o.readiness);
  }
  return unavailable;
}
// 公开 Wiki 没有 Text/Dummy 派生接口；只补 J273 已解释字段，未知私有内存不推断。
EntitySupplementValue EntitySupplement(void *object) noexcept {
  EntitySupplementValue out;
  auto c=CurrentWorldRecording(); if(!IsMain()||!c||!object) return out;
  const auto reader=c->source_memory;
  const auto address=std::uint32_t(reinterpret_cast<uintptr_t>(object));
  std::uint32_t vptr=0,col=0,td=0;
  if(!reader.copy||!reader.copy(address,&vptr,4)||vptr<4||!reader.copy(vptr-4,&col,4)||
     col<reader.image_base||col-reader.image_base>reader.image_size-20||!reader.copy(col+12,&td,4)) return out;
  const auto &layout=world::layout::LayoutConstants();
  if(td-reader.image_base==layout.entity_dummy_td_rva) { out.class_name="NativeDummy"; out.status="Value"; return out; }
  if(td-reader.image_base!=layout.entity_text_td_rva) return out;
  out.class_name="NativeText";
  auto *id=c->identities.Find(world::ObjectDomain::Entity,address);
  if(!id||id->Retired()||id->readiness<2) { out.status="NotCaptured"; return out; }
  if(!reader.copy(address+layout.entity_text_counter_offset,&out.counter,4)||
     !reader.copy(address+layout.entity_text_duration_offset,&out.duration,4)) { out.status="ReadFailed"; return out; }
  for(std::uint32_t i=0;i<layout.entity_text_capacity&&i<sizeof(out.text);++i) {
    if(!reader.copy(address+layout.entity_text_buffer_offset+i,&out.text[i],1)) { out.status="ReadFailed"; return out; }
    if(!out.text[i]) { out.text_size=i; out.status="Value"; return out; }
  }
  out.text_size=sizeof(out.text); out.status="Truncated"; return out;
}
// J273 GetNullFrame/IsEventTriggered 的名字枚举未公开；仅复制已加载 ANM2 的名字表，
// 再交由公开 Getter 读取当前姿态，绝不 Load/Update/Play 或执行随机动画。
const char *ObservationSpriteNames(void *sprite,void *owner,void (*visit)(void *,bool,const char *,std::size_t) noexcept) noexcept {
  auto c=CurrentWorldRecording(); if(!IsMain()||!c||!sprite||!visit) return "Unavailable";
  const auto &reader=c->source_memory; const auto &layout=world::layout::LayoutConstants();
  const auto base=std::uint32_t(reinterpret_cast<uintptr_t>(sprite)); std::size_t total=0;
  for(unsigned event=0;event<2;++event) {
    const auto offset=event?layout.sprite_event_names_offset:layout.sprite_null_names_offset;
    std::uint32_t before[2]{},after[2]{};
    if(!reader.copy(base+offset,before,sizeof(before))) return "ReadFailed";
    if(before[1]>65536||(!before[0]&&before[1])) return "Truncated";
    for(std::uint32_t i=0;i<before[1];++i) {
      std::uint32_t value[6]{}; const auto at=std::uint64_t(before[0])+std::uint64_t(i)*24;
      if(at>UINT32_MAX||!reader.copy(std::uint32_t(at),value,sizeof(value))) return "ReadFailed";
      const auto length=value[4],capacity=value[5];
      if(length>capacity||length>1024||total+length>(1u<<20)) return "Truncated";
      char name[1024]{};
      if(length) {
        if(capacity<16) { if(length>15) return "ReadFailed"; std::memcpy(name,value,length); }
        else if(!value[0]||!reader.copy(value[0],name,length)) return "ReadFailed";
      }
      visit(owner,event!=0,name,length); total+=length;
    }
    if(!reader.copy(base+offset,after,sizeof(after))||std::memcmp(before,after,sizeof(before))) return "ReadFailed";
  }
  return "Value";
}
bool ObservationPublishLua(const char *payload,std::size_t bytes,bool complete,std::uint32_t objects,std::uint32_t nodes,const char *reason,unsigned phase) noexcept {
  auto c=CurrentWorldRecording(); if(!IsMain()||!c||!c->observation.Active()||!payload||bytes>(16u<<20)) return false;
  try {
    namespace obs=world::observation;
    obs::ObservationEnvelope e; e.schema=phase==3?"isaac.cs.entity.actual-call":phase==2?"isaac.cs.entity.render":phase==1?"isaac.cs.entity.query":"isaac.cs.entity.lua-event"; e.kind=phase==1||phase==2?obs::RecordKind::State:obs::RecordKind::Event;
    e.context=c->observation.BeginScope(phase==2?obs::Phase::Render:phase==1||phase==3?obs::Phase::ActualCall:obs::Phase::Lifecycle,phase==3?1:2);
    e.result.coverage=complete?obs::Coverage::Complete:obs::Coverage::Partial;
    e.result.range=phase==3?"original-loot-call-return":phase==2?"entity-owned-sprite-at-actual-render":phase==1?"explicit-readonly-query":"actual-public-lifecycle-callback";
    e.result.objects=objects; e.result.nodes=nodes; e.result.bytes=bytes;
    if(reason&&*reason) e.result.reasons.emplace_back(reason);
    e.payload.assign(payload,bytes); const auto scope=e.context.scope;
    return c->observation.PrepareScope(std::move(e)).accepted&&c->observation.CommitScope(scope,obs::Outcome::IndependentPublished).accepted;
  } catch(...) { c->observation.Invalidate("lua_event_budget"); return false; }
}
}
