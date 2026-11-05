#include "replica/stream_capture.h"
#include <algorithm>
#include <set>

namespace cs::replica::stream {
namespace {
namespace obs=world::observation;
[[noreturn]] void Fail(const char *s){throw ContractError(s);}
std::string Str(const Value &v){if(!v.IsString())Fail("stream_source_string");return {v.GetString(),v.GetStringLength()};}
const Value &At(const Value &v,const char *k){if(!v.IsObject()||!v.HasMember(k))Fail("stream_source_field_missing");return v[k];}
SubjectKey ReadKey(const Value &v){return {Unsigned(At(v,"run")),Unsigned(At(v,"id")),unsigned(Unsigned(At(v,"space")))};}
std::string SourceLocal(std::uint64_t epoch,std::uint64_t local){return std::to_string(epoch)+"/"+std::to_string(local);}
Version V(const Range &r){return {r->id,r->revision};}
std::shared_ptr<const Json> Own(const Value &v){return std::make_shared<const Json>(Copy(v));}
Field Tree(unsigned id,const Value &v){return {id,FieldStatus::Value,Own(v)};}
Sample OrdinarySample(const obs::LiveBatch &b){Sample s;s.epoch=b.epoch;s.tick=b.tick;s.outer=b.outer;s.owner=b.owner;s.mutation=b.mutation;s.loss=b.loss;return s;}
Json Object(){return Json(rapidjson::kObjectType);}
// 精确值比较只用于具名完整范围的版本复用；不做摘要或泛型子树去重。
bool Contents(const RangeData &a,const RangeData &b){return a.world==b.world&&a.id==b.id&&a.members==b.members&&a.records==b.records&&a.dependencies==b.dependencies;}
std::string Category(const std::string &s){if(s=="EntityPlayer")return "Player";return s;}
constexpr const char *domains[]={"entity","grid","room","level","exploration","room_save","player","run","random","presentation","mod"};
// 主Sprite按具名owner匹配实例代次；不能把同身体的任意辅助Sprite或相同路径当作它。
std::uint64_t MainSprite(const Value &v){
  std::uint64_t result=0;
  const auto visit=[&](const auto &self,const Value &x)->void{
    if(x.IsObject()){
      if(x.HasMember("fields")&&x["fields"].IsObject()&&x["fields"].HasMember("Entity.GetSprite")){
        const auto &sprite=x["fields"]["Entity.GetSprite"];
        if(sprite.IsObject()&&sprite.HasMember("resource_generation")){if(result)Fail("player_post_resource_owner_ambiguous");result=Unsigned(sprite["resource_generation"]);}
      }
      for(auto i=x.MemberBegin();i!=x.MemberEnd();++i)self(self,i->value);
    }else if(x.IsArray())for(const auto &item:x.GetArray())self(self,item);
  };visit(visit,v);return result;
}
}
const Layouts &CaptureLayouts(){
  static const Layouts layouts=[](){
    Layouts result;
    const auto add=[&](const char *name,RangeKind kind,unsigned fields){Layout l;l.kind=kind;for(unsigned i=1;i<=fields;++i){l.fields.push_back(i);l.types[i]=6;}result.emplace(name,std::move(l));};
    add("stream.context.v1",RangeKind::Context,1);add("stream.structure.v1",RangeKind::Structure,2);
    add("stream.resource.v1",RangeKind::Resource,1);add("stream.world.v1",RangeKind::WorldDynamic,1);add("stream.player.v1",RangeKind::PlayerFast,1);
    return result;
  }();return layouts;
}
std::uint64_t Capture::Next(){if(next_revision_>=UINT64_MAX-1)Fail("stream_capture_sequence_exhausted");return ++next_revision_;}
void Capture::Begin(std::uint64_t session) noexcept {
  Stop();exhausted_=false;authority_session_=session;
  try {
    status_={};status_.reason=session?"stream_wait_ordinary":"stream_authority_missing";
    layouts_=CaptureLayouts();
  }catch(...){authority_session_=0;try{status_.reason="stream_layout_allocation";}catch(...){}}
}
void Capture::Stop() noexcept {
  current_.clear();members_.clear();retirements_.clear();bodies_.clear();body_publications_.clear();latest_.reset();baseline_.reset();store_.reset();registry_.reset();layouts_.clear();
  authority_session_=source_session_=source_epoch_=source_run_=source_room_=0;world_={};last_capture_=last_tick_=loss_=invalidation_=0;
  status_.ready=false;status_.baseline_requested=true;try{status_.reason="stream_capture_off";}catch(...){}
}
void Capture::Invalidate(const char *reason) noexcept {
  // 已借出的const值仍由传输持有；撤销的是可用于新目标的资格。
  status_.ready=false;++status_.invalidations;bodies_.clear();body_publications_.clear();latest_.reset();
  // 冷内容仍是不可变值，当前适用资格已撤销；只有下一完整窗口逐字段重读相等才可复用版本。
  // 身体子帧没有这一复用规则，必须重新取得真实更新见证。
  for(auto i=current_.begin();i!=current_.end();)if(i->first.kind==RangeKind::PlayerFast)i=current_.erase(i);else ++i;
  if(!exhausted_)try{status_.reason=reason?reason:"stream_invalidated";}catch(...){}
}
void Capture::BodyGap(const char *reason) noexcept {++status_.body_gaps;try{status_.body_reason=reason;}catch(...){}}
// Authority分配语义代次，源session/epoch只作映射依据；未知连续性不会沿用旧场景。
void Capture::Scope(const obs::ObservationContext &c){
  if(!authority_session_||c.run.presence!=obs::Presence::Known||c.floor.presence!=obs::Presence::Known||
     c.dimension.presence!=obs::Presence::Known||c.room.presence!=obs::Presence::Known||
     !c.run.value.local||!c.floor.value.local||!c.dimension.value.local||!c.room.value.local)Fail("stream_context_location_missing");
  if(!registry_||c.session!=source_session_||c.epoch!=source_epoch_||c.run.value.local!=source_run_){
    if(next_run_==UINT64_MAX)Fail("stream_run_exhausted");
    current_.clear();members_.clear();retirements_.clear();bodies_.clear();body_publications_.clear();latest_.reset();baseline_.reset();
    world_={authority_session_,++next_run_,0,0,0,0};source_session_=c.session;source_epoch_=c.epoch;source_run_=c.run.value.local;source_room_=0;
    registry_=std::make_unique<Registry>(authority_session_,world_.run,source_session_);registry_->StrictSources();
    registry_->BindSource(source_session_,source_epoch_,std::to_string(source_run_));
    last_capture_=last_tick_=0;
  }
  Json temp;auto &a=temp.GetAllocator();
  const auto related=[&](unsigned space,std::uint64_t local){return Unsigned(registry_->Key(space,SourceLocal(source_epoch_,local),"",a)["id"]);};
  const auto floor=related(4,c.floor.value.local),dimension=related(5,c.dimension.value.local),room=related(7,c.room.value.local);
  if(!store_||world_.floor!=floor||world_.dimension!=dimension||world_.room!=room){
    if(next_scene_==UINT64_MAX)Fail("stream_scene_exhausted");
    world_.floor=floor;world_.dimension=dimension;world_.room=room;world_.scene=++next_scene_;
    store_=std::make_unique<Store>(world_);current_.clear();members_.clear();retirements_.clear();bodies_.clear();body_publications_.clear();latest_.reset();baseline_.reset();
    status_.baseline_requested=true;status_.ready=false;
  }
  source_room_=c.room.value.local;
}
Member Capture::Subject(const Value &s,const std::string &category,bool declare){
  if(s.HasMember("session")&&Unsigned(s["session"])!=source_session_)Fail("stream_member_cross_session");
  if(s.HasMember("epoch")&&Unsigned(s["epoch"])!=source_epoch_)Fail("stream_member_cross_epoch");
  if(Str(At(s,"resolution"))!="KnownCurrent")Fail("stream_member_not_current");
  const auto space=unsigned(Unsigned(At(s,"space")));
  const auto local=Unsigned(At(s,"local_id")),use=Unsigned(At(s,"use"));
  if((space!=1&&space!=2)||!local||!use)Fail("stream_source_identity");
  const auto name=SourceLocal(source_epoch_,local);if(declare)registry_->DeclareCurrent(space,name,std::to_string(use));
  Json temp;const auto key=ReadKey(registry_->Key(space,name,std::to_string(use),temp.GetAllocator()));
  return {key,use,Unsigned(At(s,"revision")),category};
}
// 只复用整份具名完整范围；调用方已经在本窗重读，未读/Partial从不到达此处。
Range Capture::Make(RangeData d,bool reuse){
  if(reuse)if(const auto i=current_.find(d.id);i!=current_.end()&&Contents(*i->second,d))return i->second;
  d.revision=Next();d.sample.sequence=d.revision;auto range=Freeze(std::move(d),layouts_);
  const auto decision=store_->Admit(range,layouts_);if(!decision)Fail(decision.reason);
  current_[range->id]=range;return range;
}
void Capture::Collect(){
  std::vector<Version> roots;for(const auto &[id,r]:current_)roots.push_back(V(r));
  for(const auto &[key,r]:bodies_)roots.push_back(V(r));
  store_->Collect(roots);
}
void Capture::Ordinary(const obs::LiveBatch &b) noexcept {
  if(!authority_session_||exhausted_)return;
  try {
    if(b.domains.size()!=11||!b.tick||!b.outer||!b.owner||(b.capture<=last_capture_&&b.epoch==source_epoch_))Fail("stream_ordinary_missing_or_repeated");
    const auto &c=b.domains.front()->context;
    const bool revoked=b.invalidation!=invalidation_||b.loss!=loss_;
    if(revoked)Invalidate("stream_source_invalidation");
    Scope(c);loss_=b.loss;invalidation_=b.invalidation;
    if(last_tick_&&b.tick<=last_tick_&&b.epoch==source_epoch_)Fail("stream_ordinary_tick_repeated");
    std::array<Json,11> sources;
    // 本次解析和分层临时副本也占展开池；不能等Freeze后才追记内存。
    std::uint64_t source_bytes=11*65536;for(const auto &e:b.domains){if(e->stream_payload.size()>(256ull<<20)/16)Fail("stream_source_workspace_budget");source_bytes+=16ull*e->stream_payload.size();}
    auto source_memory=memory::Reserve(memory::Pool::Values,source_bytes);
    std::array<const obs::ObservationEnvelope *,11> envelopes{};
    for(const auto &e:b.domains){
      const auto index=unsigned(e->domain)-1;if(index>=11||envelopes[index])Fail("stream_duplicate_domain");
      const auto &x=e->context;
      if(e->result.coverage!=obs::Coverage::Complete||x.phase!=obs::Phase::OrdinaryPost||x.session!=b.session||x.epoch!=b.epoch||x.capture_index!=b.capture||
         x.authority_tick.presence!=obs::Presence::Known||x.authority_tick.value!=b.tick||x.outer!=b.outer||x.owner!=b.owner||x.mutation_before!=b.mutation||x.mutation_after!=b.mutation||
         x.plan_revision!=b.plan_revision||x.run.value!=c.run.value||x.floor.value!=c.floor.value||x.dimension.value!=c.dimension.value||x.room.value!=c.room.value)Fail("stream_mixed_or_incomplete_window");
      if(e->stream_payload.empty())Fail("stream_layered_source_missing");
      sources[index]=Parse(e->stream_payload);envelopes[index]=e.get();
      if(Str(At(sources[index],"contract"))!="stream-source-v1")Fail("stream_source_schema");
    }
    // 先登记全体身份，再投影任何关系；相同Key同时作为Player和Entity唯一主体。
    std::map<SubjectKey,Member> members;
    for(unsigned d=0;d<2;++d)for(const auto &entry:At(sources[d],"members").GetArray()){
      const auto member=Subject(At(entry,"subject"),Category(Str(At(entry,"category"))),true);
      if(!members.emplace(member.key,member).second)Fail("stream_member_conflict");
    }
    const Member room{{world_.run,world_.room,7},1,0,"Room"};members.emplace(room.key,room);
    if(members.size()>16384)Fail("stream_members_budget");
    // 只有合格完整成员缺席产生离表事实，控制资格变化不删除身体。
    Json departures(rapidjson::kArrayType);auto &da=departures.GetAllocator();
    for(const auto &[key,t]:retirements_){
      Value item(rapidjson::kObjectType);Put(item,"run",Decimal(key.run,da),da);Put(item,"space",Value(key.space),da);Put(item,"id",Decimal(key.id,da),da);
      Put(item,"use",Decimal(t.member.use,da),da);Put(item,"shape",Decimal(t.member.shape,da),da);Put(item,"source_epoch",Decimal(t.epoch,da),da);
      Put(item,"event",Decimal(t.event,da),da);Put(item,"outer",Decimal(t.outer,da),da);Put(item,"scene",Decimal(world_.scene,da),da);
      Put(item,"reason",std::string("ObservedNativePhysicalRetirement"),da);departures.PushBack(item,da);
    }
    for(const auto &[key,m]:members_)if(!members.contains(key)&&!retirements_.contains(key)){
      Value item(rapidjson::kObjectType);Put(item,"run",Decimal(key.run,da),da);Put(item,"space",Value(key.space),da);Put(item,"id",Decimal(key.id,da),da);
      Put(item,"use",Decimal(m.use,da),da);Put(item,"shape",Decimal(m.shape,da),da);Put(item,"reason",std::string("AbsentFromCompleteMembership;not_physical_destruction"),da);departures.PushBack(item,da);
    }
    std::map<SubjectKey,Json> structural,resource,dynamic,joint;
    for(const auto &[key,m]:members){structural.emplace(key,Object());resource.emplace(key,Object());dynamic.emplace(key,Object());joint.emplace(key,Object());}
    const auto append=[&](std::map<SubjectKey,Json> &target,unsigned domain,const Value &items){
      if(!items.IsArray())Fail("stream_source_partition_array");
      for(const auto &entry:items.GetArray()){
        auto key=room.key;if(entry.HasMember("subject"))key=Subject(entry["subject"],"",false).key;
        const auto it=target.find(key);if(it==target.end())Fail("stream_partition_member_missing");
        auto &out=it->second;auto &a=out.GetAllocator();const auto name=std::string(domains[domain]);
        if(!out.HasMember(name.c_str()))Put(out,name.c_str(),Value(rapidjson::kArrayType),a);
        out[name.c_str()].PushBack(registry_->Project(At(entry,"value"),a,b.epoch),a);
      }
    };
    std::vector<Range> versions;auto sample=OrdinarySample(b);
    for(unsigned d=0;d<11;++d){
      append(structural,d,At(sources[d],"structure"));append(resource,d,At(sources[d],"resource"));
      append(dynamic,d,At(sources[d],"world"));append(joint,d,At(sources[d],"bodies"));
      RangeData context;context.world=world_;context.id={RangeKind::Context,std::string("context/")+domains[d],{}};context.sample=sample;context.complete=true;
      const auto value=registry_->Project(At(sources[d],"context"),sources[d].GetAllocator(),b.epoch);
      context.members={room};context.records={{room,"stream.context.v1",{Tree(1,value)}}};versions.push_back(Make(std::move(context),true));
    }
    // 玩家联合私有状态不伪装成60Hz PlayerFast，也不使静态Structure/Resource因每步姿态变化而重发。
    for(const auto &[key,m]:members)if(m.category=="Player"){
      RangeData body;body.world=world_;body.id={RangeKind::Context,"body-joint/"+std::to_string(key.id),{}};body.sample=sample;body.complete=true;
      body.members={m};body.records={{m,"stream.context.v1",{Tree(1,joint.at(key))}}};versions.push_back(Make(std::move(body),true));
    }
    RangeData structure;structure.world=world_;structure.id={RangeKind::Structure,"world-members",{}};structure.sample=sample;structure.complete=structure.membership_complete=true;
    const Value empty_departures(rapidjson::kArrayType);
    for(const auto &[key,m]:members){structure.members.push_back(m);structure.records.push_back({m,"stream.structure.v1",{Tree(1,structural.at(key)),Tree(2,key==room.key?static_cast<const Value &>(departures):empty_departures)}});}
    // 离表原因位于该普通发布中；结构的可复用内容由当前成员/关系决定，不积累无界墓碑。
    auto s=Make(std::move(structure),true);versions.push_back(s);
    RangeData resources;resources.world=world_;resources.id={RangeKind::Resource,"native-resources",{}};resources.sample=sample;resources.complete=true;
    for(const auto &[key,m]:members){resources.members.push_back(m);resources.records.push_back({m,"stream.resource.v1",{Tree(1,resource.at(key))}});}
    auto r=Make(std::move(resources),true);versions.push_back(r);
    RangeData world;world.world=world_;world.id={RangeKind::WorldDynamic,"ordinary-room",{}};world.sample=sample;world.complete=world.membership_complete=true;world.dependencies={V(s),V(r)};
    for(const auto &v:versions)if(v->id.kind==RangeKind::Context)world.dependencies.push_back(V(v));
    if(world.dependencies.size()>64)Fail("stream_context_dependency_budget");
    for(const auto &[key,m]:members)if(m.category!="Player"){world.members.push_back(m);world.records.push_back({m,"stream.world.v1",{Tree(1,dynamic.at(key))}});}
    auto w=Make(std::move(world),false);versions.push_back(w);
    auto publication=std::make_shared<Publication>();publication->world=world_;publication->world_dynamic=w;publication->ranges=versions;
    if(status_.baseline_requested){
      std::vector<obs::ObservationEnvelope> rows;rows.reserve(11);
      for(const auto *e:envelopes){if(e->schema.find(".stream")!=std::string::npos)Fail("stream_baseline_full_source_required");rows.push_back(*e);rows.back().stream_payload.clear();}
      auto cold=replica::Capture(Connected(world_,1),Next(),rows,*registry_);
      RangeData data;data.world=world_;data.id={RangeKind::Context,"cold-components",{}};data.sample=sample;data.complete=true;data.components=Own(cold["components"]);
      auto full=Make(std::move(data),false);versions.push_back(full);
      RangeData manifest;manifest.world=world_;manifest.id={RangeKind::Manifest,"baseline",{}};manifest.sample=sample;manifest.complete=manifest.membership_complete=true;
      manifest.members=s->members;manifest.event_watermark=b.event_watermark;
      // 18—20实际安装域：Entity/Grid/Room/Player；其余域是完整保留，不能冒充全部原生应用。
      manifest.native_domains=(1ull<<0)|(1ull<<1)|(1ull<<2)|(1ull<<6);manifest.retained_domains=2047ull&~manifest.native_domains;
      for(const auto &v:versions)manifest.required.push_back(V(v));
      auto m=Make(std::move(manifest),false);versions.push_back(m);publication->manifest=m;publication->ranges=versions;
      baseline_=publication;status_.baseline_requested=false;
    }
    for(const auto &range:publication->ranges)publication->validity.push_back({V(range),range->sample,w->sample,b.invalidation});
    // 结构变更先撤旧身体视图；普通更新不把未更新身体重复发布成新子帧。
    for(auto i=bodies_.begin();i!=bodies_.end();)if(!members.contains(i->first)||i->second->dependencies.front()!=V(s)){body_publications_.erase(i->first);i=bodies_.erase(i);}else ++i;
    std::set<RangeId> active;for(const auto &range:versions)active.insert(range->id);
    for(auto i=current_.begin();i!=current_.end();){
      const bool body=i->first.kind==RangeKind::PlayerFast;
      const bool joint=i->first.kind==RangeKind::Context&&i->first.name.starts_with("body-joint/");
      if((body&&!bodies_.contains(i->first.body))||(joint&&!active.contains(i->first)))i=current_.erase(i);else ++i;
    }
    members_=std::move(members);latest_=std::move(publication);last_capture_=b.capture;last_tick_=b.tick;
    status_.ready=true;status_.ordinary++;status_.world_sequence=w->sample.sequence;status_.reason="stream_ordinary_committed";Collect();
  }catch(const ContractError &e){Invalidate(e.what());if(e.code.find("exhausted")!=std::string::npos)exhausted_=true;}
  catch(const std::runtime_error &e){Invalidate(e.what());}
  catch(...){Invalidate("stream_capture_allocation_or_source");}
}
// 逐身体子帧依赖明确普通版本，不改完整成员、不提升整房序号、不调用原生更新。
void Capture::PlayerPost(const BodySample &b,const std::string &payload) noexcept {
  try {
    if(!status_.ready||!registry_||!store_||b.epoch!=source_epoch_||b.run!=source_run_||b.room!=source_room_||!b.binding||!b.tick||!b.outer||!b.owner)Fail("player_post_scope_or_dependency");
    const auto structure=current_.find({RangeKind::Structure,"world-members",{}}),resources=current_.find({RangeKind::Resource,"native-resources",{}});
    if(structure==current_.end()||resources==current_.end())Fail("player_post_structure_missing");
    auto source_memory=memory::Reserve(memory::Pool::Values,65536+16ull*payload.size());
    auto source=Parse(payload);if(Str(At(source,"contract"))!="player-fast-source-v1")Fail("player_post_source_schema");
    const auto member=Subject(At(source,"subject"),"Player",false);const auto current=members_.find(member.key);
    if(current==members_.end()||current->second!=member||member.use!=b.use||member.shape!=b.shape||Unsigned(source["subject"]["local_id"])!=b.local)Fail("player_post_identity_changed");
    std::uint64_t resource_generation=0;
    for(const auto &record:resources->second->records)if(record.member.key==member.key)
      resource_generation=MainSprite(**std::get_if<std::shared_ptr<const Json>>(&record.fields.front().value));
    if(!resource_generation||resource_generation!=Unsigned(At(source,"resource_generation")))Fail("player_post_resource_changed");
    if(const auto old=bodies_.find(member.key);old!=bodies_.end()&&old->second->sample.tick>=b.tick)Fail("player_post_duplicate_update");
    RangeData data;data.world=world_;data.id={RangeKind::PlayerFast,"body-dynamic",member.key};data.complete=data.membership_complete=true;
    data.sample.epoch=b.epoch;data.sample.tick=b.tick;data.sample.outer=b.outer;data.sample.owner=b.owner;data.sample.mutation=b.mutation;data.sample.loss=loss_;
    data.sample.last_native_update_tick=b.tick;data.sample.binding_revision=b.binding;data.sample.required_world_sequence=status_.world_sequence;data.sample.kind=b.kind;
    data.dependencies={V(structure->second),V(resources->second),V(latest_->world_dynamic)};data.members={member};
    Json projected;projected.SetObject();auto &a=projected.GetAllocator();for(const char *k:{"common","body","sprite"})Put(projected,k,registry_->Project(At(source,k),a,b.epoch),a);
    data.records={{member,"stream.player.v1",{Tree(1,projected)}}};auto range=Make(std::move(data),false);
    auto publication=std::make_shared<Publication>();publication->world=world_;publication->world_dynamic=latest_->world_dynamic;
    publication->ranges=latest_->ranges;publication->ranges.push_back(range);publication->player_fast=range;
    publication->validity=latest_->validity;publication->validity.push_back({V(range),range->sample,range->sample,invalidation_});
    bodies_[member.key]=std::move(range);body_publications_[member.key]=std::move(publication);
    ++status_.bodies;status_.body_reason=b.kind==CaptureKind::OddPlayer?"player_post_odd_committed":"player_post_ordinary_committed";Collect();
  }catch(const ContractError &e){BodyGap(e.what());if(e.code=="player_post_resource_changed"||e.code=="player_post_resource_owner_ambiguous")Invalidate(e.what());}
  catch(...){BodyGap("player_post_allocation");}
}
// 物理退休只接观察到正常完成的destroyed事实；离表在Ordinary中另记，不据缺枚举伪造析构。
void Capture::PhysicalRetire(unsigned space,std::uint64_t epoch,std::uint64_t local,std::uint64_t use,std::uint64_t shape,std::uint64_t event,std::uint64_t outer) noexcept {
  if(!registry_||epoch!=source_epoch_||!local||!use)return;
  try {
    Json temp;const auto key=ReadKey(registry_->Key(space,SourceLocal(epoch,local),std::to_string(use),temp.GetAllocator()));
    if(!retirements_.contains(key)&&retirements_.size()>=16384)Fail("stream_retirement_budget");
    Member member{key,use,shape,space==1?"Entity":"Grid"};if(const auto found=members_.find(key);found!=members_.end()){member=found->second;member.shape=shape;}
    retirements_.try_emplace(key,Retirement{member,epoch,event,outer});registry_->RetireSource(space,SourceLocal(epoch,local),std::to_string(use));Invalidate("stream_physical_retirement");
  }
  catch(const ContractError &e){Invalidate(e.what());if(e.code=="stream_retirement_budget")exhausted_=true;}
  catch(...){Invalidate("stream_retirement_or_unknown_identity");}
}
std::vector<std::shared_ptr<const Publication>> Capture::Bodies() const {
  std::vector<std::shared_ptr<const Publication>> out;if(status_.ready)for(const auto &[key,p]:body_publications_)out.push_back(p);return out;
}
} // namespace cs::replica::stream
