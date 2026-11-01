#include "game/world_bridge.h"
#include "game/world_sources/lua_entity.h"
#include "world/compatibility/encode.h"
#include "binding/world_layout.h"
#include "game/world_sources/value_source.h"
#include "game/native_room_grid.h"
#include "runtime.h"
#include <cstring>
namespace cs::game {
namespace {
namespace obs=world::observation;
// 所有服务请求发生在 Manager 主线程边界；后台只消费 PublicationQueue 的自有值。
obs::CapturePlan Plan() {
  obs::CapturePlan p;
  p.domains.push_back({obs::DomainId::Entity,1u|4u|16u|32u,30,{}});
  p.domains.push_back({obs::DomainId::Grid,1u|4u|16u|32u,30,{}});
  p.domains.push_back({obs::DomainId::Room,53u,30,{16384,262144,12,4ull<<20}});
  p.domains.push_back({obs::DomainId::Level,45u,30,{16384,262144,12,4ull<<20}});
  for(unsigned d=5;d<=11;++d) p.domains.push_back({static_cast<obs::DomainId>(d),obs::SupportedPhases(static_cast<obs::DomainId>(d)),30,{16384,262144,12,(d==6||d>=8?8ull:1ull)<<20}});
  return p;
}
void Latest(void *owner,const obs::ObservationEnvelope &e) noexcept {
  auto &c=*static_cast<WorldCapture *>(owner);
  try {
    if(e.kind!=obs::RecordKind::State) return;
    if(e.schema=="isaac.cs.random.native-global-call") c.latest_global_random=e;
    if(e.schema=="isaac.cs.entity") c.latest_entity=e;
    else if(e.schema=="isaac.cs.grid") c.latest_grid=e;
    else if(e.schema=="isaac.cs.room") c.latest_room=e;
    else if(e.schema=="isaac.cs.level") c.latest_level=e;
    else if(unsigned(e.domain)>=5&&unsigned(e.domain)<=11&&e.schema.find(".",9)==std::string::npos) c.latest_continuity[unsigned(e.domain)-5]=e;
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
// 只有相同run/room的稳定窗口可继承维度与描述；独立事件不能借用过时代次。
void LocationContext(WorldCapture &c,obs::ObservationContext &out,std::uint64_t run,std::uint64_t room) noexcept {
  const auto &h=c.hierarchy;
  out.floor={}; out.dimension={}; out.descriptor={};
  if(h.run!=run||!run) return;
  if(h.floor.local) out.floor={obs::Presence::Known,h.floor};
  if(!h.valid||h.room!=room||!room) return;
  out.dimension={obs::Presence::Known,h.dimension}; out.descriptor={obs::Presence::Known,h.descriptor};
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
  LocationContext(c,e.context,h.run,h.room);
  return e;
}
// 同一 native 复制窗口内顺序调用各域；任一最终来源变化由统一 Commit 撤销。
void Lua(void *owner,const world::BatchHeader &h,const world::RawObject *objects,const world::RawMembership *members) noexcept {
  auto &c=*static_cast<WorldCapture *>(owner);
  // 先读取room建立分层上下文，再由level与Entity/Grid在同批次引用。
  c.current_entity_scope=0;
  for(const unsigned index : {2u,3u,0u,1u,4u,5u,6u,7u,8u,9u,10u}) {
    const auto domain=static_cast<obs::DomainId>(index+1);
    const bool grid=index==1;
    if(!Scheduled(c,domain,h.capture)) continue;
    try {
      EnsureObservationLuaBridge();
      const char *schemas[]={"isaac.cs.entity","isaac.cs.grid","isaac.cs.room","isaac.cs.level","isaac.cs.exploration","isaac.cs.room_save","isaac.cs.player","isaac.cs.run","isaac.cs.random","isaac.cs.presentation","isaac.cs.mod"};
      auto e=Base(c,domain,schemas[index],h); e.context.source=2;
      c.lua_objects=objects; c.lua_object_count=h.object_count; c.lua_members=members; c.lua_members_count=h.membership_count; c.lua_scope=e.context.scope; c.lua_run=h.run; c.lua_room=h.room; c.location_window=true;
      for(const auto &d:c.observation.Plan().domains) if(d.domain==domain) e.result.budget=d.budget;
      const auto budget=e.result.budget;
      // 原生随机流只编码一次，Lua仅取计数摘要；完整状态仍属于同一采集scope和字节预算。
      const auto native_random=index==8?CaptureRandomSources():std::string{};
      const auto reserved=(index>=2?64u:0u)+(native_random.empty()?0u:unsigned(native_random.size()+20));
      auto result=CaptureLuaEntity(budget.objects,budget.nodes,budget.depth,reserved<budget.bytes?std::uint32_t(budget.bytes)-reserved:0,index);
      if(index==8&&result.available&&!result.payload.empty()&&result.payload.back()=='}') {
        if(reserved<budget.bytes&&result.payload.size()+native_random.size()+20<=budget.bytes)
          result.payload.insert(result.payload.size()-1,",\"native_sources\":"+native_random);
        else { result.complete=false;result.reason="native_random_combined_payload_budget"; }
      }
      if(index==0) { ++c.lua_attempts; if(result.available&&result.complete) ++c.lua_complete; else ++c.lua_partial; }
      c.lua_objects=nullptr; c.lua_object_count=0; c.lua_members=nullptr; c.lua_members_count=0; c.location_window=false;
      LocationContext(c,e.context,h.run,h.room);
      e.result.coverage=result.available?(result.complete?obs::Coverage::Complete:obs::Coverage::Partial):obs::Coverage::Failed;
      e.result.range=index>=7?"public-run-random-presentation-mod-v2;actual-call-and-render-separate;adapter-coverage-explicit":index>=4?"public-continuity-v2:scoped-history-save-collections-and-canonical-player-references":index>=2?"public-room-level-v2:identity-config-progress-environment-structure;explicit-query-parameters":grid?"public-grid-v2:all-derived-save-sprite-rng;finite-enums-and-explicit-queries":"public-entity-v2:state-and-finite-queries;unbounded-derived-queries:on-request;external-domains:explicit-references";
      if(!result.reason.empty()) e.result.reasons.push_back(result.reason);
      e.result.objects=result.objects; e.result.nodes=result.nodes; e.result.bytes=result.payload.size(); e.payload=std::move(result.payload);
      if(index>=2&&!e.payload.empty()&&e.payload.front()=='{') e.payload.insert(1,"\"sample_revision\":\""+std::to_string(e.context.scope)+"\",");
      e.result.bytes=e.payload.size();
      const auto scope=e.context.scope;
      if(c.observation.PrepareScope(std::move(e)).accepted) {
        c.pending_scopes[index>=2?index+2:grid?3:0]=scope;
        if(index==0) c.current_entity_scope=scope;
      } else if(index>=4) FinishLuaObservation(index,scope,false);
    } catch(...) { c.location_window=false; c.hierarchy.Invalidate(false); c.lua_objects=nullptr; c.lua_object_count=0; c.lua_members=nullptr; c.lua_members_count=0; c.observation.Invalidate("lua_scope_budget"); }
  }
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
      e.result.range=d?"min-read-v1:grid-public-and-door-only":"min-read-v1:entity-public-and-seven-derived-classes";
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
  if(!stable||!committed) { c.hierarchy.Invalidate(false); c.latest_room={}; c.latest_level={}; c.latest_continuity={}; }
  for(unsigned i=0;i<c.pending_scopes.size();++i) if(auto &scope=c.pending_scopes[i];scope) {
    const auto result=c.observation.CommitScope(scope,!stable?obs::Outcome::Invalidated:committed?obs::Outcome::AuthorityCommitted:obs::Outcome::Cancelled);
    if(i>=6) FinishLuaObservation(i-2,scope,result.accepted);
    scope=0;
  }
}
void Event(void *owner,const world::LifecycleEvent &event) noexcept {
  auto &c=*static_cast<WorldCapture *>(owner);
  // Collector内部的生命周期撤销不经过WorldRevokePlayer，也要保留实际身体失效边。
  if(event.kind==world::EventKind::CurrentPlayerReadWitness&&event.reason==world::Reason::NotReady&&
     c.observation.Needed(obs::DomainId::Player,obs::Phase::Lifecycle)) try {
    obs::ObservationEnvelope edge; edge.domain=obs::DomainId::Player; edge.schema="isaac.cs.player.control"; edge.kind=obs::RecordKind::Relation;
    edge.context=c.observation.BeginScope(obs::Phase::Lifecycle,1); edge.context.outer=event.outer; edge.context.event_watermark=event.sequence;
    edge.payload="{\"event\":\"native_current_player_witness_revoked\",\"body\":{\"space\":1,\"local_id\":\""+std::to_string(event.local_id)+"\",\"use\":\""+std::to_string(event.use)+"\"},\"control_body_slot\":"+std::to_string(event.ordinal)+",\"control_session\":\""+std::to_string(event.session)+"\",\"binding_revision\":\""+std::to_string(event.participant_generation)+"\",\"after\":{\"resolution\":\"NotCaptured\",\"reason\":\"native_witness_revoked;not_entity_destruction\"}}";
    edge.result.coverage=obs::Coverage::Complete; edge.result.range="actual-native-control-read-qualification-revocation"; edge.result.bytes=edge.payload.size();
    const auto scope=edge.context.scope; if(c.observation.PrepareScope(std::move(edge)).accepted) c.observation.CommitScope(scope,obs::Outcome::IndependentPublished);
  } catch(...) { ++c.formal_loss; }
  const auto domain=event.domain?obs::DomainId::Grid:obs::DomainId::Entity;
  if(!c.observation.Needed(domain,obs::Phase::Lifecycle)) return;
  try {
    obs::ObservationEnvelope e; e.domain=domain; e.schema=event.domain?"isaac.cs.grid.lifecycle":"isaac.cs.entity.lifecycle";
    e.kind=obs::RecordKind::Event; e.context=c.observation.BeginScope(obs::Phase::Lifecycle,1);
    e.context.outer=event.outer; e.context.event_watermark=event.sequence;
    if(event.run) e.context.run={obs::Presence::Known,{c.session_key,c.epoch,event.run,obs::IdentitySpace::Run}};
    // native 事件记录真实观察时的room代次；Lua回调只在稳定room上下文内补归属。
    if(event.room) e.context.room={obs::Presence::Known,{c.session_key,c.epoch,event.room,obs::IdentitySpace::RoomInstance}};
    LocationContext(c,e.context,event.run,event.room);
    e.context.mutation_before=e.context.mutation_after=event.mutation;
    e.result.coverage=event.exceptional?obs::Coverage::Partial:obs::Coverage::Complete; e.result.range="native-observed-lifecycle-and-call-pair";
    e.payload="{\"local_id\":\""+std::to_string(event.local_id)+"\",\"use\":\""+std::to_string(event.use)+"\",\"revision\":\""+std::to_string(event.revision)+"\",\"origin_readiness\":"+std::to_string(event.readiness)+",\"kind\":"+std::to_string(unsigned(event.kind))+",\"observation\":"+std::to_string(event.observation)+",\"call\":"+std::to_string(event.call)+",\"parent_call\":"+std::to_string(event.parent)+",\"entering\":"+std::to_string(event.entering)+",\"exceptional\":"+std::to_string(event.exceptional)+",\"candidate_tick\":\""+std::to_string(event.tick)+"\",\"memberships\":"+std::to_string(event.memberships)+"}";
    if(event.domain) {
      // 请求值、实际返回与已观察出生分别编码；不将请求variant/seed覆盖实际描述。
      std::string detail;
      const auto request=static_cast<world::Observation>(event.observation);
      if(request==world::Observation::GridSpawn&&event.arg_count>=5)
        detail=",\"request\":{\"index\":"+std::to_string(event.args[0])+",\"type\":"+std::to_string(event.args[1])+",\"variant\":"+std::to_string(event.args[2])+",\"seed\":"+std::to_string(event.args[3])+",\"var_data\":"+std::to_string(event.args[4])+"}";
      else if(request==world::Observation::GridFree&&event.arg_count>=3)
        detail=",\"request\":{\"index\":"+std::to_string(event.args[0])+",\"path_trail\":"+std::to_string(event.args[1])+",\"keep_decoration\":"+std::to_string(event.args[2])+"}";
      else if(request==world::Observation::GridInit&&event.arg_count)
        detail=",\"request\":{\"seed\":"+std::to_string(event.args[0])+"}";
      if(request==world::Observation::GridSpawn&&(event.kind==world::EventKind::NativeHook||event.kind==world::EventKind::NativeBody)&&!event.entering&&!event.exceptional)
        detail+=",\"returned\":"+std::string(event.result?"true":"false");
      if(const auto *id=c.identities.Find(world::ObjectDomain::Grid,event.address);
         id&&id->local_id==event.local_id&&id->use==event.use&&id->born) {
        detail+=",\"observed_birth\":{\"sequence\":\""+std::to_string(id->birth)+"\",\"owner\":"+std::to_string(id->birth_kind)+",\"type\":"+std::to_string(id->birth_type)+",\"variant\":"+std::to_string(id->birth_variant)+",\"seed\":"+std::to_string(id->birth_seed)+"}";
      }
      if(!detail.empty()) e.payload.insert(e.payload.size()-1,detail);
    }
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
    if(result.accepted) { recorder_consumer=recorder; if(recorder) {GlobalRandomCheckpoint();RandomSourcesCheckpoint();} }
  }
  ArmObservationValueSources(observation.Active()&&(formal_consumer||recorder_consumer));
  ArmGlobalRandom(observation.Needed(obs::DomainId::Random,obs::Phase::ActualCall));
  ArmRandomSources(observation.Needed(obs::DomainId::Random,obs::Phase::ActualCall));
}
bool ObservationLuaNeeded(const char *phase,bool grid) noexcept {
  auto c=CurrentWorldRecording(); if(!IsMain()||!c||!c->observation.Active()) return false;
  return c->observation.Needed(grid?world::observation::DomainId::Grid:world::observation::DomainId::Entity,phase&&!std::strcmp(phase,"Lifecycle")?world::observation::Phase::Lifecycle:phase&&!std::strcmp(phase,"Render")?world::observation::Phase::Render:world::observation::Phase::ActualCall);
}
ObservationSubjectValue ObservationSubject(void *object,bool registering,bool grid) noexcept {
  ObservationSubjectValue result;
  auto c=CurrentWorldRecording(); if(!IsMain()||!c||!c->observation.Active()||!object) return result;
  const auto address=static_cast<std::uint32_t>(reinterpret_cast<uintptr_t>(object));
  const auto domain=grid?world::ObjectDomain::Grid:world::ObjectDomain::Entity;
  auto *id=registering?c->identities.FindOrCreate(domain,address):c->identities.Find(domain,address);
  result.session=c->session_key; result.epoch=c->epoch;
  if(!id) return result;
  if(registering) id->MarkFirstObserved();
  result.local=id->local_id; result.use=id->use; result.revision=id->revision; result.origin=id->origin; result.readiness=id->readiness;
  for(std::uint32_t slot=0;!grid&&slot<world::kControlAnchorCapacity;++slot) {
    const auto anchor=c->collector->CurrentAnchor(slot);
    if(!anchor.valid||anchor.player!=address) continue;
    const auto ordinal=slot/world::kControlBodiesPerParticipant;
    if(c->control_sessions[ordinal]!=anchor.session) {
      c->control_sessions[ordinal]=anchor.session;
      c->participant_keys[ordinal]=c->observation.AllocateRelatedSubject(obs::IdentitySpace::Participant);
      c->control_group_keys[ordinal]=c->observation.AllocateRelatedSubject(obs::IdentitySpace::ControlGroup);
    }
    result.ordinal=ordinal; result.participant=c->participant_keys[ordinal].local; result.group=c->control_group_keys[ordinal].local;
    result.control_session=anchor.session; result.binding_revision=anchor.generation; result.device_revision=c->device_relation_revisions[ordinal]; result.control_active=true; result.body_slot=slot%world::kControlBodiesPerParticipant; break;
  }
  if(!grid) ObservationPlayerMembership(object,result);
  if(registering&&c->lua_objects&&id->local_id<c->lua_subject_scopes.size()) c->lua_subject_scopes[id->local_id]=c->lua_scope;
  const bool current=c->lua_objects&&id->local_id<c->lua_subject_scopes.size()&&c->lua_subject_scopes[id->local_id]==c->lua_scope;
  // v2 当前 Lua 引用还需本 scope 的主体登记；复用地址仅在真实创建 owner 已完成且
  // 本 use 没有 Morph/未知 Init 修订时恢复当前资格。旧 v1 的强出生排序规则保持原样。
  const bool reuse_current=id->born&&id->readiness>=2&&id->revision==0&&(grid||id->current_public_ready);
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
// 枚举同一批次的 Grid 主体，保留所有固定槽及门槽别名，不按逻辑尺寸缩减来源。
std::uint32_t ObservationGrids(void *owner,void (*visit)(void *,void *,std::uint32_t) noexcept) noexcept {
  auto c=CurrentWorldRecording(); if(!IsMain()||!c||!c->lua_objects||!visit) return 1;
  std::uint32_t unavailable=0;
  for(std::uint32_t i=0;i<c->lua_object_count;++i) {
    const auto &o=c->lua_objects[i]; if(o.domain!=unsigned(world::ObjectDomain::Grid)) continue;
    auto *id=c->identities.Find(world::ObjectDomain::Grid,o.address);
    if(!id||id->Retired()) {
      // 实际固定槽仍有对象时，不得悄悄遗漏后声称完整；保留真实身份状态用于诊断。
      ++unavailable;
      if(!Settings.world_e2e_token.empty())Trace("grid_member_excluded","scope=%llu address=%08X local=%llu use=%llu origin=%u lifecycle=%u readiness=%u destroyed=%u candidate=%u owner=%llu",c->lua_scope,o.address,id?id->local_id:0,id?id->use:0,id?id->origin:0,id?id->lifecycle:0,id?id->readiness:0,id?unsigned(id->destroyed):0,id?unsigned(id->candidate):0,id?id->owner_scope:0);
      continue;
    }
    // 仅初始化／实际index／membership同时成立才允许公开派生读取。
    bool ready=o.vptr&&o.field_count;
    for(unsigned f=0;f<o.field_count;++f) if(o.fields[f].status==world::FieldStatus::NotReady||o.fields[f].status==world::FieldStatus::ReadFailed) ready=false;
    if(!ready) { ++unavailable; continue; }
    visit(owner,reinterpret_cast<void *>(uintptr_t(o.address)),id->pending_memberships);
  }
  return unavailable;
}
// 部分官方回调按字符串注册名包装TNT等userdata，可能缺少vanilla元表。
// 只接纳已核定RTTI、已初始化、索引有界且未退休的当前Grid，绝不猜测未知ABI。
bool ObservationValidGrid(void *grid) noexcept {
  auto c=CurrentWorldRecording(); if(!IsMain()||!c||!c->observation.Active()||!grid) return false;
  const auto address=std::uint32_t(reinterpret_cast<uintptr_t>(grid));
  if(const auto *id=c->identities.Find(world::ObjectDomain::Grid,address);id&&id->Retired()) return false;
  auto memory=c->source_memory;
  world::NativeSource reader; reader.owner=&memory;
  reader.copy=[](void *owner,std::uint64_t at,void *out,std::size_t bytes) noexcept {
    auto &m=*static_cast<world::MemoryReader *>(owner);
    return at&&at<=UINT32_MAX&&bytes<=UINT32_MAX-at&&m.copy&&m.copy(std::uint32_t(at),out,bytes);
  };
  reader.image_base=memory.image_base; reader.image_size=memory.image_size;
  std::uint32_t vptr=0,index=UINT32_MAX,type=0; std::uint8_t initialized=0; bool base=false;
  if(world::ClassifyNative(reader,world::ObjectDomain::Grid,address,vptr,base)!=world::Class::Grid) return false;
  return reader.Value(std::uint64_t(address)+4,type)&&type>=1&&type<=27&&
    reader.Value(std::uint64_t(address)+0x1c,initialized)&&initialized&&
    reader.Value(std::uint64_t(address)+0x24,index)&&index<world::layout::LayoutConstants().room_grid_slot_count;
}
// 纯值membership来自同一批，不另调Room或重读整个世界。
void ObservationGridMemberships(void *grid,void *owner,void (*visit)(void *,std::uint32_t,std::uint32_t) noexcept) noexcept {
  auto c=CurrentWorldRecording(); if(!IsMain()||!c||!c->lua_members||!c->lua_objects||!visit) return;
  const auto address=std::uint32_t(reinterpret_cast<uintptr_t>(grid));
  for(std::uint32_t i=0;i<c->lua_members_count;++i) {
    const auto &m=c->lua_members[i]; if(m.object>=c->lua_object_count) continue;
    const auto &o=c->lua_objects[m.object];
    if(o.domain==unsigned(world::ObjectDomain::Grid)&&o.address==address) visit(owner,m.source,m.index);
  }
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
     !reader.copy(address+layout.entity_text_duration_offset,&out.duration,4)||
     !reader.copy(address+layout.entity_text_color_offset,out.text_color,sizeof(out.text_color))||
     !reader.copy(address+layout.entity_text_color_offset+16,&out.color_word,4)) { out.status="ReadFailed"; return out; }
  for(std::uint32_t i=0;i<layout.entity_text_capacity&&i<sizeof(out.text);++i) {
    if(!reader.copy(address+layout.entity_text_buffer_offset+i,&out.text[i],1)) { out.status="ReadFailed"; return out; }
    if(!out.text[i]) { out.text_size=i; out.status="Value"; return out; }
  }
  out.text_size=sizeof(out.text); out.status="Truncated"; return out;
}
// J273 GetNullFrame/IsEventTriggered 的名字枚举未公开；仅复制已加载 ANM2 的名字表，
// 再交由公开 Getter 读取当前姿态，绝不 Load/Update/Play 或执行随机动画。
const char *ObservationSpriteNames(void *sprite,void *owner,void (*visit)(void *,bool,const char *,std::size_t) noexcept) noexcept {
  auto c=CurrentWorldRecording(); if(!IsMain()||(!c&&!NativeRoomGridContextActive())||!sprite||!visit) return "Unavailable";
  // 名字表只读不依赖Recorder；Client只在自有原生对象的主线程借用窗口允许同一读取。
  const auto copy=[&](std::uint32_t address,void *out,std::size_t bytes) noexcept {
    if(c)return c->source_memory.copy(address,out,bytes);
    SIZE_T copied=0;return ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const void *>(uintptr_t(address)),out,bytes,&copied)&&copied==bytes;
  };
  const auto &layout=world::layout::LayoutConstants();
  const auto base=std::uint32_t(reinterpret_cast<uintptr_t>(sprite)); std::size_t total=0;
  for(unsigned event=0;event<2;++event) {
    const auto offset=event?layout.sprite_event_names_offset:layout.sprite_null_names_offset;
    std::uint32_t before[2]{},after[2]{};
    if(!copy(base+offset,before,sizeof(before))) return "ReadFailed";
    if(before[1]>65536||(!before[0]&&before[1])) return "Truncated";
    for(std::uint32_t i=0;i<before[1];++i) {
      std::uint32_t value[6]{}; const auto at=std::uint64_t(before[0])+std::uint64_t(i)*24;
      if(at>UINT32_MAX||!copy(std::uint32_t(at),value,sizeof(value))) return "ReadFailed";
      const auto length=value[4],capacity=value[5];
      if(length>capacity||length>1024||total+length>(1u<<20)) return "Truncated";
      char name[1024]{};
      if(length) {
        if(capacity<16) { if(length>15) return "ReadFailed"; std::memcpy(name,value,length); }
        else if(!value[0]||!copy(value[0],name,length)) return "ReadFailed";
      }
      visit(owner,event!=0,name,length); total+=length;
    }
    if(!copy(base+offset,after,sizeof(after))||std::memcmp(before,after,sizeof(before))) return "ReadFailed";
  }
  return "Value";
}
bool ObservationPublishLua(const char *payload,std::size_t bytes,bool complete,std::uint32_t objects,std::uint32_t nodes,const char *reason,unsigned phase,bool grid) noexcept {
  auto c=CurrentWorldRecording(); if(!IsMain()||!c||!c->observation.Active()||!payload||bytes>(16u<<20)) return false;
  try {
    namespace obs=world::observation;
    obs::ObservationEnvelope e; e.domain=grid?obs::DomainId::Grid:obs::DomainId::Entity; e.schema=grid?(phase==2?"isaac.cs.grid.render":phase==1?"isaac.cs.grid.query":"isaac.cs.grid.lua-event"):phase==3?"isaac.cs.entity.actual-call":phase==2?"isaac.cs.entity.render":phase==1?"isaac.cs.entity.query":"isaac.cs.entity.lua-event"; e.kind=phase==1||phase==2?obs::RecordKind::State:obs::RecordKind::Event;
    e.context=c->observation.BeginScope(phase==2?obs::Phase::Render:phase==1||phase==3?obs::Phase::ActualCall:obs::Phase::Lifecycle,phase==3?1:2);
    e.context.outer=c->collector->CurrentOuterToken();
    e.context.mutation_before=e.context.mutation_after=c->collector->CurrentMutation();
    if(const auto run=c->collector->CurrentRunToken()) e.context.run={obs::Presence::Known,{c->session_key,c->epoch,run,obs::IdentitySpace::Run}};
    if(const auto room=c->collector->CurrentRoomToken()) e.context.room={obs::Presence::Known,{c->session_key,c->epoch,room,obs::IdentitySpace::RoomInstance}};
    LocationContext(*c,e.context,c->collector->CurrentRunToken(),c->collector->CurrentRoomToken());
    e.result.coverage=complete?obs::Coverage::Complete:obs::Coverage::Partial;
    e.result.range=grid?"actual-grid-callback-or-explicit-query":phase==3?"original-loot-call-return":phase==2?"entity-owned-sprite-at-actual-render":phase==1?"explicit-readonly-query":"actual-public-lifecycle-callback";
    e.result.objects=objects; e.result.nodes=nodes; e.result.bytes=bytes;
    if(reason&&*reason) e.result.reasons.emplace_back(reason);
    e.payload.assign(payload,bytes); const auto scope=e.context.scope;
    return c->observation.PrepareScope(std::move(e)).accepted&&c->observation.CommitScope(scope,obs::Outcome::IndependentPublished).accepted;
  } catch(...) { c->observation.Invalidate("lua_event_budget"); return false; }
}
// 公开逻辑位置只在同一完整native复制窗口建立；FIRST_OBSERVED不补造楼层生成历史。
LocationValue ObservationLocation(int stage,int type,std::uint32_t seed,int dimension,int list,int grid,const char *construction) noexcept {
  auto c=CurrentWorldRecording();
  if(!IsMain()||!c||!c->location_window||dimension<0||dimension>2||grid==-19||grid==-100||grid==-101||(list<0&&(grid>=0||grid<-20||grid==-19))||list>=1024) return {};
  auto &h=c->hierarchy;
  if(h.run!=c->lua_run||h.stage!=stage||h.type!=type||h.seed!=seed) h.Invalidate(true);
  h.run=c->lua_run; h.room=c->lua_room; h.stage=stage; h.type=type; h.seed=seed;
  if(!h.floor.local) h.floor=c->observation.AllocateRelatedSubject(obs::IdentitySpace::Floor);
  if(!h.dimensions[dimension].local) h.dimensions[dimension]=c->observation.AllocateRelatedSubject(obs::IdentitySpace::Dimension);
  h.dimension=h.dimensions[dimension]; h.dimension_index=dimension;
  const auto id=ObservationDescriptor(dimension,list,grid,construction);
  h.descriptor={c->session_key,c->epoch,id,obs::IdentitySpace::RoomDescriptor};
  h.valid=h.floor.local&&h.dimension.local&&id;
  return {c->session_key,c->epoch,h.floor.local,h.dimension.local,id,h.run,h.room};
}
std::uint64_t ObservationDescriptor(int dimension,int list,int grid,const char *construction) noexcept {
  auto c=CurrentWorldRecording();
  if(!IsMain()||!c||(!c->location_window&&!c->hierarchy.valid)||dimension<0||dimension>2||grid==-19||grid==-100||grid==-101||(list<0&&(grid>=0||grid<-20||grid==-19))||list>=1024||!c->hierarchy.floor.local) return 0;
  if(!construction||std::strlen(construction)>=384) return 0;
  auto &h=c->hierarchy;
  if(!h.dimensions[dimension].local) h.dimensions[dimension]=c->observation.AllocateRelatedSubject(obs::IdentitySpace::Dimension);
  for(auto &d:h.descriptors) if(d.used&&d.dimension==dimension&&d.list==list&&(list>=0||d.grid==grid)) {
    // 列表槽换成不同网格位置时退休旧描述，不借地址延续身份。
    if(d.grid==grid) {
      // 同一描述槽保持主体，配置/具名种子变化产生独立构建修订；不补造出生历史。
      if(std::strcmp(d.construction.data(),construction)) {
        if(!c->location_window||d.construction_revision==UINT64_MAX) return 0;
        std::strcpy(d.construction.data(),construction); ++d.construction_revision;
      }
      return d.subject.local;
    }
    d.used=false;
  }
  if(!c->location_window) return 0;
  for(auto &d:h.descriptors) if(!d.used) {
    d.used=true; d.dimension=dimension; d.list=list; d.grid=grid; d.construction_revision=1; std::strcpy(d.construction.data(),construction);
    d.subject=c->observation.AllocateRelatedSubject(obs::IdentitySpace::RoomDescriptor); return d.subject.local;
  }
  return 0;
}
// 修订是当前配置/种子见证的版本，不能当作原生Init调用次数。
std::uint64_t ObservationDescriptorRevision(std::uint64_t id) noexcept {
  auto c=CurrentWorldRecording(); if(!IsMain()||!c||!id) return 0;
  for(const auto &d:c->hierarchy.descriptors) if(d.used&&d.subject.local==id) return d.construction_revision;
  return 0;
}
std::uint64_t ObservationDimension(int index) noexcept {
  auto c=CurrentWorldRecording(); if(!IsMain()||!c||index<0||index>2) return 0;
  return c->hierarchy.dimensions[index].local;
}
bool ObservationLocationNeeded() noexcept {
  auto c=CurrentWorldRecording(); return IsMain()&&c&&c->observation.Active()&&
    (c->observation.Needed(obs::DomainId::Room,obs::Phase::Lifecycle)||c->observation.Needed(obs::DomainId::Level,obs::Phase::Transition));
}
void ObservationLocationBoundary(const char *name,bool floor) noexcept {
  auto c=CurrentWorldRecording(); if(!ObservationLocationNeeded()||!c) return;
  try {
    obs::ObservationEnvelope e; e.domain=obs::DomainId::Level; e.schema="isaac.cs.level.transition"; e.kind=obs::RecordKind::Event;
    e.context=c->observation.BeginScope(obs::Phase::Transition,2);
    const auto &h=c->hierarchy;
    e.payload="{\"event\":\""+std::string(name)+"\",\"previous\":{\"run\":\""+std::to_string(h.run)+"\",\"floor\":\""+std::to_string(h.floor.local)+"\",\"dimension\":\""+std::to_string(h.dimension.local)+"\",\"descriptor\":\""+std::to_string(h.descriptor.local)+"\",\"construction_revision\":\""+std::to_string(ObservationDescriptorRevision(h.descriptor.local))+"\",\"room_instance\":\""+std::to_string(h.room)+"\"},\"new_context\":\"UnknownUntilOrdinaryPost\"}";
    c->hierarchy.Invalidate(floor); c->latest_room={}; c->latest_level={}; c->latest_continuity={};
    e.result.coverage=obs::Coverage::Complete; e.result.range="actual-public-boundary;previous-location-only"; e.result.bytes=e.payload.size();
    const auto scope=e.context.scope;
    if(c->observation.PrepareScope(std::move(e)).accepted) c->observation.CommitScope(scope,obs::Outcome::IndependentPublished);
  } catch(...) { c->hierarchy.Invalidate(floor); c->observation.Invalidate("location_boundary_budget"); }
}
bool ObservationPublishLocation(const char *payload,std::size_t bytes,bool complete,std::uint32_t nodes,const char *reason,unsigned domain,unsigned phase) noexcept {
  auto c=CurrentWorldRecording(); if(!ObservationLocationNeeded()||!c||bytes>(16u<<20)||nodes>262144||domain>8||phase>4) return false;
  if(!c->observation.Needed(static_cast<obs::DomainId>(domain+3),phase==4?obs::Phase::ActualCall:phase==3?obs::Phase::Transition:phase==1?obs::Phase::ActualCall:phase==2?obs::Phase::Render:obs::Phase::Lifecycle)) return false;
  try {
    obs::ObservationEnvelope e; e.domain=static_cast<obs::DomainId>(domain+3);
    e.schema=domain>=5?(std::string(domain==5?"isaac.cs.run":domain==6?"isaac.cs.random":domain==7?"isaac.cs.presentation":"isaac.cs.mod")+(phase==4?".actual-call":phase==3?".transition":phase==2?".render":phase==1?".query":".event")):domain>=2?(std::string(domain==2?"isaac.cs.exploration":domain==3?"isaac.cs.room_save":"isaac.cs.player")+(phase==3?".transition":phase==1?".query":".event")):domain?(phase==1?"isaac.cs.level.query":"isaac.cs.level.event"):(phase==1?"isaac.cs.room.query":phase==2?"isaac.cs.room.render":"isaac.cs.room.event");
    e.kind=phase==1||phase==2?obs::RecordKind::State:obs::RecordKind::Event;
    e.context=c->observation.BeginScope(phase==4?obs::Phase::ActualCall:phase==3?obs::Phase::Transition:phase==1?obs::Phase::ActualCall:phase==2?obs::Phase::Render:obs::Phase::Lifecycle,2);
    const auto run=c->collector->CurrentRunToken(),room=c->collector->CurrentRoomToken();
    if(run) e.context.run={obs::Presence::Known,{c->session_key,c->epoch,run,obs::IdentitySpace::Run}};
    // 过渡期间不关联旧room；完整上下文由下一次普通采集恢复。
    if(phase!=3&&c->hierarchy.valid&&c->hierarchy.room==room&&c->hierarchy.run==run) {
      e.context.room={obs::Presence::Known,{c->session_key,c->epoch,room,obs::IdentitySpace::RoomInstance}};
      LocationContext(*c,e.context,run,room);
    }
    e.context.outer=c->collector->CurrentOuterToken();
    e.payload.assign(payload,bytes); e.result.coverage=complete?obs::Coverage::Complete:obs::Coverage::Partial;
    e.result.range="actual-room-level-callback-or-explicit-readonly-query"; e.result.bytes=bytes; e.result.nodes=nodes;
    if(reason&&*reason) e.result.reasons.emplace_back(reason);
    const auto scope=e.context.scope;
    return c->observation.PrepareScope(std::move(e)).accepted&&c->observation.CommitScope(scope,obs::Outcome::IndependentPublished).accepted;
  } catch(...) { c->observation.Invalidate("location_event_budget"); return false; }
}

}

namespace cs::game {
// 所有身份为服务自有十进制值；context只在主线程同步借用窗口内有效。
std::string ObservationCaptureContext() noexcept {
  auto c=CurrentWorldRecording(); if(!IsMain()||!c) return "{}";
  const auto &h=c->hierarchy;
  return "{\"session\":\""+std::to_string(c->session_key)+"\",\"epoch\":\""+std::to_string(c->epoch)+"\",\"scope\":\""+std::to_string(c->location_window?c->lua_scope:0)+"\",\"run\":\""+std::to_string(c->collector->CurrentRunToken())+"\",\"floor\":\""+std::to_string(h.floor.local)+"\",\"descriptor\":\""+std::to_string(h.valid?h.descriptor.local:0)+"\",\"room_instance\":\""+std::to_string(c->collector->CurrentRoomToken())+"\",\"entity_scope\":\""+std::to_string(c->location_window?c->current_entity_scope:0)+"\"}";
}
std::uint64_t ObservationSaveEntry() noexcept {
  auto c=CurrentWorldRecording(); if(!IsMain()||!c||!c->location_window) return 0;
  return c->observation.AllocateRelatedSubject(obs::IdentitySpace::SaveEntry).local;
}
}

namespace cs::game {
// 在真正的锚点变化处记录全部绑定边，30tick普通采集不会吞掉短暂Join/失效。
// device/key只参与进程内比较；对外用participant、control session与binding revision命名。
void ObservationControlChange(unsigned slot,const world::PlayerAnchor &before,const world::PlayerAnchor &after,const char *reason) noexcept {
  auto c=CurrentWorldRecording(); if(!IsMain()||!c||!c->observation.Needed(obs::DomainId::Player,obs::Phase::Lifecycle)) return;
  try {
    const auto ordinal=slot/world::kControlBodiesPerParticipant;
    auto encode=[&](const world::PlayerAnchor &a) {
      if(!a.valid) return std::string("{\"resolution\":\"NotCaptured\",\"reason\":\"anchor_invalid\"}");
      if(c->control_sessions[ordinal]!=a.session) {
        c->control_sessions[ordinal]=a.session;
        c->participant_keys[ordinal]=c->observation.AllocateRelatedSubject(obs::IdentitySpace::Participant);
        c->control_group_keys[ordinal]=c->observation.AllocateRelatedSubject(obs::IdentitySpace::ControlGroup);
      }
      const auto *id=c->identities.Find(world::ObjectDomain::Entity,a.player);
      return "{\"resolution\":\"KnownCurrentControlAnchor\",\"participant\":\""+std::to_string(c->participant_keys[ordinal].local)+"\",\"control_group\":\""+std::to_string(c->control_group_keys[ordinal].local)+"\",\"control_session\":\""+std::to_string(a.session)+"\",\"binding_revision\":\""+std::to_string(a.generation)+"\",\"body\":{\"space\":1,\"local_id\":\""+std::to_string(id?id->local_id:0)+"\",\"use\":\""+std::to_string(id?id->use:0)+"\",\"resolution\":\""+(id&&!id->Retired()?"KnownHistorical":"Unresolved")+"\"},\"device\":\"owner_qualified_by_control_session_and_binding_revision\"}";
    };
    const auto a=encode(before);
    // 设备/assignment的变化有独立关系修订；不要求把原生地址当设备身份。
    if(before.device!=after.device||before.key!=after.key||before.session!=after.session||before.valid!=after.valid)
      ++c->device_relation_revisions[ordinal];
    const auto b=encode(after);
    obs::ObservationEnvelope e; e.domain=obs::DomainId::Player; e.schema="isaac.cs.player.control"; e.kind=obs::RecordKind::Relation;
    e.context=c->observation.BeginScope(obs::Phase::Lifecycle,1); e.context.outer=c->collector->CurrentOuterToken();
    e.payload="{\"ordinal\":"+std::to_string(ordinal)+",\"body_slot\":"+std::to_string(slot%world::kControlBodiesPerParticipant)+",\"device_relation_revision\":\""+std::to_string(c->device_relation_revisions[ordinal])+"\",\"device_changed\":"+(before.device!=after.device?"true":"false")+",\"assignment_changed\":"+(before.key!=after.key?"true":"false")+",\"reason\":\""+reason+"\",\"before\":"+a+",\"after\":"+b+",\"identity_semantics\":\"anchor_invalid_is_not_entity_destroyed\"}";
    e.result.coverage=obs::Coverage::Complete; e.result.range="actual-control-anchor-change"; e.result.bytes=e.payload.size();
    const auto scope=e.context.scope; if(c->observation.PrepareScope(std::move(e)).accepted) c->observation.CommitScope(scope,obs::Outcome::IndependentPublished);
  } catch(...) { ++c->formal_loss; }
}
}

namespace cs::game {
// Lua关联主体经统一主线程服务分配，不以地址、格号或种子作为身份。
std::uint64_t ObservationRelatedSubject(unsigned space) noexcept {
 auto c=CurrentWorldRecording(); if(!IsMain()||!c||!c->observation.Active()||space<11||space>14) return 0;
 return c->observation.AllocateRelatedSubject(static_cast<world::observation::IdentitySpace>(space)).local;
}
}
