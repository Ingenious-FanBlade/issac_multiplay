#include "replica/stream_capture.h"
#include "replica/stream_e2e.h"
#include "replica/stream_materialize.h"
#include "replica/stream_partition.h"
#include "replica/stream_source_value.h"
#include <algorithm>
#include <set>
#include <cstdio>

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
// 局部唯一DOM只在此处转成const；移交后调用者不再访问被移走的树。
Field Tree(unsigned id,Json &&v){return {id,FieldStatus::Value,std::make_shared<const Json>(std::move(v))};}
// 发布只共享范围，但引用数组、版本名称和有效性证言仍有独立分配，必须在构造前付费。
std::shared_ptr<Publication> NewPublication(std::size_t count){
  if(count>66)Fail("stream_publication_range_budget");
  auto lease=memory::Reserve(memory::Pool::Values,sizeof(Publication)+128+count*(sizeof(Range)+sizeof(Publication::Validity)+256),"capture_publication");
  auto out=memory::DeferredValue<Publication>();out->memory=std::move(lease);out->ranges.reserve(count);out->validity.reserve(count);return out;
}
void SealPublication(Publication &value){
  std::uint64_t bytes=sizeof(Publication)+128+value.ranges.capacity()*sizeof(Range)+value.validity.capacity()*sizeof(Publication::Validity);
  for(const auto &v:value.validity)if(v.version.id.name.capacity()>std::string{}.capacity())bytes+=v.version.id.name.capacity()+1;
  if(bytes>value.memory->Bytes())Fail("stream_publication_footprint");value.memory->Resize(bytes);
}
Sample OrdinarySample(const obs::LiveBatch &b){Sample s;s.epoch=b.epoch;s.tick=b.tick;s.outer=b.outer;s.owner=b.owner;s.mutation=b.mutation;s.loss=b.loss;return s;}
Json Object(){return Json(rapidjson::kObjectType);}
// 精确值比较只用于具名完整范围的版本复用；不做摘要或泛型子树去重。
bool Contents(const RangeData &a,const RangeData &b){return a.world==b.world&&a.id==b.id&&a.members==b.members&&a.records==b.records&&a.dependencies==b.dependencies;}
#ifdef ISAACCS_E2E
// 只读已比较为不同的纯值，给出第一处差异路径；不修改字段、图编号或范围复用判据。
std::string Difference(const Value &a,const Value &b,const std::string &path,unsigned depth=0){
  if(depth>64)return path+"/depth";
  if(a.GetType()!=b.GetType())return path+"/type";
  if(a.IsArray()){
    if(a.Size()!=b.Size())return path+"/size";
    for(unsigned i=0;i<a.Size();++i)if(a[i]!=b[i])return Difference(a[i],b[i],path+"/"+std::to_string(i),depth+1);
  }else if(a.IsObject()){
    if(a.MemberCount()!=b.MemberCount())return path+"/members";
    for(auto i=a.MemberBegin();i!=a.MemberEnd();++i){const auto j=b.FindMember(i->name);const auto next=path+"/"+std::string(i->name.GetString(),i->name.GetStringLength());if(j==b.MemberEnd())return next+"/absent";if(i->value!=j->value)return Difference(i->value,j->value,next,depth+1);}
  }
  return path;
}
void ChangedRange(const RangeData &a,const RangeData &b){
  std::string path="members";
  if(a.members==b.members){path="records";if(a.records.size()==b.records.size())for(unsigned i=0;i<a.records.size();++i){
    const auto &x=a.records[i],&y=b.records[i];if(x==y)continue;path="record/"+std::to_string(i);
    if(x.member==y.member&&x.layout==y.layout&&x.fields.size()==y.fields.size())for(unsigned j=0;j<x.fields.size();++j){
      const auto &f=x.fields[j],&g=y.fields[j];if(f==g)continue;path+="/field/"+std::to_string(f.id);
      const auto left=std::get_if<std::shared_ptr<const Json>>(&f.value),right=std::get_if<std::shared_ptr<const Json>>(&g.value);
      if(left&&right&&*left&&*right)path=Difference(**left,**right,path);break;
    }break;
  }}
  const auto site=(std::string("source-change:")+std::to_string(unsigned(b.id.kind))+"/"+path).substr(0,384);
  E2EStreamProgress(site.c_str(),{b.revision,a.revision,unsigned(b.id.kind),b.members.size(),a.members.size(),MetricMicros()/1000,0,0});
}
#endif
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
// 逐身体快姿态只能引用精确普通依赖中同一ANM/服装集合。不同图编号按本地定义解析，
// 不按路径/相同动画名猜测资源仍有效；成员或资源改变撤销快源，等下一完整ordinary。
bool RenderResources(const Value &fast,const Value &ordinary){
  using Index=std::map<std::uint64_t,const Value *>;Index left,right;
  const auto index=[](const auto &self,const Value &v,Index &out)->void{if(v.IsObject()){if(v.HasMember("type")&&v["type"]=="table"&&v.HasMember("node"))out.emplace(Unsigned(v["node"]),&v);for(auto i=v.MemberBegin();i!=v.MemberEnd();++i)self(self,i->value,out);}else if(v.IsArray())for(const auto &x:v.GetArray())self(self,x,out);};index(index,fast,left);index(index,ordinary,right);
  unsigned budget=1u<<20;
  const auto compare=[&](const auto &self,const Value &x,const Value &y,unsigned depth)->bool{
    if(!budget--||depth>64)return false;const Value *a=&x,*b=&y;
    const auto resolve=[](const Value *&v,const Index &i){if(v->IsObject()&&v->HasMember("type")&&(*v)["type"]=="graph_ref"){auto f=i.find(Unsigned(At(*v,"node")));if(f==i.end())return false;v=f->second;}return true;};
    if(!resolve(a,left)||!resolve(b,right))return false;
    if(a->IsObject()&&a->HasMember("type")&&(*a)["type"]=="stream_sprite_pose")return b->IsObject()&&a->HasMember("resource_generation")&&b->HasMember("resource_generation")&&(*a)["resource_generation"]==(*b)["resource_generation"];
    if(a->IsObject()&&a->HasMember("type")&&(*a)["type"]=="CostumeSpriteDesc"){
      if(!b->IsObject()||!b->HasMember("fields"))return false;for(const char *name:{"CostumeSpriteDesc.GetItemConfig","CostumeSpriteDesc.GetSprite"})if(!self(self,At(At(*a,"fields"),name),At(At(*b,"fields"),name),depth+1))return false;return true;
    }
    if(a->IsObject()&&a->HasMember("type")&&(*a)["type"]=="table"){
      if(!b->IsObject()||!b->HasMember("entries"))return false;const auto &ae=At(*a,"entries"),&be=At(*b,"entries");if(ae.IsObject()&&ae.ObjectEmpty())return be.IsObject()&&be.ObjectEmpty();if(!ae.IsArray()||!be.IsArray()||ae.Size()!=be.Size())return false;
      for(const auto &e:ae.GetArray()){const Value *match=nullptr;for(const auto &f:be.GetArray())if(At(e,"key")==At(f,"key")){match=&At(f,"value");break;}if(!match||!self(self,At(e,"value"),*match,depth+1))return false;}return true;
    }
    if(a->IsObject()){if(!b->IsObject())return false;for(auto i=a->MemberBegin();i!=a->MemberEnd();++i){if(i->name=="node")continue;if(!b->HasMember(i->name)||!self(self,i->value,(*b)[i->name],depth+1))return false;}return true;}
    if(a->IsArray()){if(!b->IsArray()||a->Size()!=b->Size())return false;for(unsigned i=0;i<a->Size();++i)if(!self(self,(*a)[i],(*b)[i],depth+1))return false;return true;}return *a==*b;
  };
  const auto &body=At(fast,"body"),&old=At(ordinary,"body");
  for(const char *name:{"EntityPlayer.GetBodySprite","EntityPlayer.GetCostumeSpriteDescs","EntityPlayer.GetCostumeLayerMap"})if(!compare(compare,At(At(body,"fields"),name),At(At(old,"fields"),name),0))return false;
  return compare(compare,At(body,"application_player_resources"),At(old,"application_player_resources"),0);
}
}
const Layouts &CaptureLayouts(){
  static const Layouts layouts=[](){
    Layouts result;
    const auto add=[&](const char *name,RangeKind kind,unsigned fields){Layout l;l.kind=kind;for(unsigned i=1;i<=fields;++i){l.fields.push_back(i);l.types[i]=6;}result.emplace(name,std::move(l));};
    add("stream.context.v1",RangeKind::Context,1);add("stream.structure.v1",RangeKind::Structure,2);
    add("stream.resource.v1",RangeKind::Resource,1);add("stream.world.v2",RangeKind::WorldDynamic,2);add("stream.player.v1",RangeKind::PlayerFast,1);
    return result;
  }();return layouts;
}
std::uint64_t Capture::Next(){if(next_revision_>=UINT64_MAX-1)Fail("stream_capture_sequence_exhausted");return ++next_revision_;}
void Capture::Begin(std::uint64_t session) noexcept {
  Stop();metrics_={};exhausted_=false;authority_session_=session;
  try {
    status_={};status_.baseline_request_local_ms=MetricMicros()/1000;status_.reason=session?"stream_wait_ordinary":"stream_authority_missing";
    status_.body_reason="player_post_not_updated";
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
Range Capture::Make(RangeData d,bool reuse,const memory::Lease &prepaid){
  if(reuse)if(const auto i=current_.find(d.id);i!=current_.end()&&Contents(*i->second,d))return i->second;
  d.revision=Next();d.sample.sequence=d.revision;auto range=FreezeCaptured(std::move(d),layouts_,CaptureOwnership{},prepaid);
#ifdef ISAACCS_E2E
  if(range->id.kind==RangeKind::Structure||range->id.kind==RangeKind::Resource)
    if(const auto old=current_.find(range->id);old!=current_.end())ChangedRange(*old->second,*range);
#endif
  const auto decision=store_->Admit(range,layouts_);if(!decision)Fail(decision.reason);
  metrics_.Bind(world_);metrics_.Note(*range,FlowStage::Sample,MetricMicros()/1000);
  current_[range->id]=range;
  return range;
}
void Capture::Collect(){
  std::vector<Version> roots;for(const auto &[id,r]:current_)roots.push_back(V(r));
  for(const auto &[key,r]:bodies_)roots.push_back(V(r));
  store_->Collect(roots);
}
void Capture::Ordinary(const obs::LiveBatch &b) noexcept {
  if(!authority_session_||exhausted_)return;
  Measure measure(metrics_,CpuStage::Build);
  const char *build_stage="context";unsigned build_domain=UINT32_MAX;
  // E2E保留实际构建失败当刻的首因；后续生命周期撤销不能把失败改写为泛化等待。
  const auto failed=[&](const char *reason) noexcept {
    Invalidate(reason);
    // 失败可能发生在部分范围已入Store之后；回收已不再被current依赖的候选，避免重试积累旧版本。
    try{if(store_)Collect();}catch(...){}
#ifdef ISAACCS_E2E
    try{const auto site=(std::string("capture-failure:")+reason).substr(0,384);E2EStreamProgress(site.c_str(),{b.capture,b.tick,b.outer,b.invalidation,b.loss,MetricMicros()/1000,0,0});}catch(...){}
#endif
  };
  try {
    if(b.domains.size()!=11||!b.tick||!b.outer||!b.owner||(b.capture<=last_capture_&&b.epoch==source_epoch_))Fail("stream_ordinary_missing_or_repeated");
    const auto &c=b.domains.front()->context;
    const bool revoked=b.invalidation!=invalidation_||b.loss!=loss_;
    if(revoked)Invalidate("stream_source_invalidation");
    Scope(c);loss_=b.loss;invalidation_=b.invalidation;
    if(last_tick_&&b.tick<=last_tick_&&b.epoch==source_epoch_)Fail("stream_ordinary_tick_repeated");
    std::array<Json,11> sources;
    // 本次解析和分层临时副本也占展开池；不能等Freeze后才追记内存。
    const auto value_limit=memory::ReplicationBudget()->Snapshot()[0].limit;
    std::uint64_t source_bytes=11*65536;for(const auto &e:b.domains){const auto bytes=e->owned_source?e->owned_bytes:e->stream_payload.size();if(bytes>value_limit/16)Fail("stream_source_workspace_budget");source_bytes+=16ull*bytes;}
    auto source_memory=memory::Reserve(memory::Pool::Values,source_bytes,"source_ordinary_temporary");
    std::array<const obs::ObservationEnvelope *,11> envelopes{};
    for(const auto &e:b.domains){
      const auto index=unsigned(e->domain)-1;if(index>=11||envelopes[index])Fail("stream_duplicate_domain");
      build_domain=index;build_stage="source";
      const auto &x=e->context;
      if(e->result.coverage!=obs::Coverage::Complete||x.phase!=obs::Phase::OrdinaryPost||x.session!=b.session||x.epoch!=b.epoch||x.capture_index!=b.capture||
         x.authority_tick.presence!=obs::Presence::Known||x.authority_tick.value!=b.tick||x.outer!=b.outer||x.owner!=b.owner||x.mutation_before!=b.mutation||x.mutation_after!=b.mutation||
         x.plan_revision!=b.plan_revision||x.run.value!=c.run.value||x.floor.value!=c.floor.value||x.dimension.value!=c.dimension.value||x.room.value!=c.room.value)Fail("stream_mixed_or_incomplete_window");
      if(!e->owned_source&&e->stream_payload.empty())Fail("stream_layered_source_missing");
      envelopes[index]=e.get();
    }
    // 先验证整窗，再按域构建；只暂留Entity/Grid以完成全体身份登记，后续域逐个消费。
    const auto load_source=[&](unsigned index){
      const auto *e=envelopes[index];build_domain=index;build_stage="source";
      if(e->owned_source){
        // 每域展开值仅借用到本域分层完成，避免十一域的重复大树同时驻留至冷构建结束。
        Json expanded;const Value *source=&e->owned_source->Read();if(!e->owned_source->Blocks().empty()){build_stage="expand";expanded=ExpandSource(e->owned_source);source=&expanded;}
        build_stage="partition";sources[index]=PartitionSource(*source,index,e->result.budget.nodes);
#ifdef ISAACCS_E2E
        // 冷构建直接记录实际arena容量；同轮进程private峰值仍由节点采样，二者不互相替代。
        if(status_.baseline_requested)E2EStreamProgress("capture-memory:partition",{b.capture,b.tick,index,e->owned_source->Bytes(),expanded.GetAllocator().Capacity(),sources[index].GetAllocator().Capacity(),source_memory->Bytes(),MetricMicros()/1000});
#endif
      }
      else sources[index]=Parse(e->stream_payload);
      if(Str(At(sources[index],"contract"))!="stream-source-v1")Fail("stream_source_schema");
    };
    load_source(0);load_source(1);
    build_stage="members";build_domain=UINT32_MAX;
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
    const auto append=[&](std::map<SubjectKey,Json> &target,unsigned domain,const char *slot){
      const auto &items=At(sources[domain],slot);
      // 保留实际域与分区首因；空对象不能冒充合同要求的空数组。
      if(!items.IsArray())throw ContractError(std::string("stream_source_partition_array:")+domains[domain]+"/"+slot);
      for(const auto &entry:items.GetArray()){
        auto key=room.key;if(entry.HasMember("subject"))key=Subject(entry["subject"],"",false).key;
        const auto it=target.find(key);if(it==target.end())Fail("stream_partition_member_missing");
        auto &out=it->second;auto &a=out.GetAllocator();const auto name=std::string(domains[domain]);
        if(!out.HasMember(name.c_str()))Put(out,name.c_str(),Value(rapidjson::kArrayType),a);
        out[name.c_str()].PushBack(registry_->Project(At(entry,"value"),a,b.epoch),a);
      }
    };
    std::vector<Range> versions;auto sample=OrdinarySample(b);
    std::array<Json,11> contexts;
    for(unsigned d=0;d<11;++d){
      if(d>=2)load_source(d);build_domain=d;build_stage="project";
      append(structural,d,"structure");append(resource,d,"resource");
      append(dynamic,d,"world");append(joint,d,"bodies");
      static_cast<Value &>(contexts[d])=registry_->Project(At(sources[d],"context"),contexts[d].GetAllocator(),b.epoch);
      // 该域身份投影完成后立即销毁分层arena；冷组件构建不再叠加十一域中间树。
      sources[d]=Json();
    }
    build_stage="ranges";build_domain=UINT32_MAX;
    if(!status_.baseline_requested){
      // 热路径分层完成后按实际arena持有量计费；再为后续Tree临时副本和小元数据预付上界。
      // 冷路径额外完整组件仍沿用原保守额度，不能用热路径计量冒充它的全部所有者。
      std::uint64_t held=OwnedJsonBytes(departures)+members.size()*2048+11*65536,staging=0;
      for(const auto &source:sources)held+=OwnedJsonBytes(source);
      for(const auto &context:contexts){held+=OwnedJsonBytes(context);staging+=OwnedValueUpperBound(context);}
      for(const auto *partition:{&structural,&resource,&dynamic,&joint})for(const auto &[key,value]:*partition){held+=OwnedJsonBytes(value);staging+=OwnedValueUpperBound(value);}
      if(held>source_memory->Bytes())Fail("stream_source_temporary_footprint");
      source_memory->Resize(held+staging);
    }
    for(unsigned d=0;d<11;++d){
      RangeData context;context.world=world_;context.id={RangeKind::Context,std::string("context/")+domains[d],{}};context.sample=sample;context.complete=true;
      context.members={room};context.records={{room,"stream.context.v1",{Tree(1,std::move(contexts[d]))}}};versions.push_back(Make(std::move(context),true,source_memory));
    }
    // 玩家联合私有状态不伪装成60Hz PlayerFast，也不使静态Structure/Resource因每步姿态变化而重发。
    for(const auto &[key,m]:members)if(m.category=="Player"){
      RangeData body;body.world=world_;body.id={RangeKind::Context,"body-joint/"+std::to_string(key.id),{}};body.sample=sample;body.complete=true;
      body.members={m};body.records={{m,"stream.context.v1",{Tree(1,std::move(joint.at(key)))}}};versions.push_back(Make(std::move(body),true,source_memory));
    }
    RangeData structure;structure.world=world_;structure.id={RangeKind::Structure,"world-members",{}};structure.sample=sample;structure.complete=structure.membership_complete=true;
    const Value empty_departures(rapidjson::kArrayType);
    for(const auto &[key,m]:members){structure.members.push_back(m);structure.records.push_back({m,"stream.structure.v1",{Tree(1,std::move(structural.at(key))),Tree(2,key==room.key?static_cast<const Value &>(departures):empty_departures)}});}
    // 离表原因位于该普通发布中；结构的可复用内容由当前成员/关系决定，不积累无界墓碑。
    auto s=Make(std::move(structure),true,source_memory);versions.push_back(s);
    RangeData resources;resources.world=world_;resources.id={RangeKind::Resource,"native-resources",{}};resources.sample=sample;resources.complete=true;
    for(const auto &[key,m]:members){resources.members.push_back(m);resources.records.push_back({m,"stream.resource.v1",{Tree(1,std::move(resource.at(key)))}});}
    auto r=Make(std::move(resources),true,source_memory);versions.push_back(r);
    RangeData world;world.world=world_;world.id={RangeKind::WorldDynamic,"ordinary-room",{}};world.sample=sample;world.complete=world.membership_complete=true;world.dependencies={V(s),V(r)};
    for(const auto &v:versions)if(v->id.kind==RangeKind::Context)world.dependencies.push_back(V(v));
    if(world.dependencies.size()>64)Fail("stream_context_dependency_budget");
    Json captures(rapidjson::kObjectType);auto &ca=captures.GetAllocator();
    // 普通结构适配保留每域真实scope/owner；不把冷Baseline的旧scope改tick冒充新采样。
    for(unsigned d=0;d<11;++d){const auto &x=envelopes[d]->context;Value p(rapidjson::kObjectType);
      for(const auto &[name,n]:std::initializer_list<std::pair<const char *,std::uint64_t>>{{"scope",x.scope},{"sequence",x.sequence},{"authority_tick",b.tick},{"outer",b.outer},{"owner",b.owner},{"mutation_before",b.mutation},{"mutation_after",b.mutation},{"source",x.source},{"source_version",x.source_version},{"event_watermark",x.event_watermark}})Put(p,name,Decimal(n,ca),ca);
      Put(p,"phase","OrdinaryPost",ca);Put(captures,domains[d],std::move(p),ca);
    }
    const Value no_captures(rapidjson::kObjectType);
    for(const auto &[key,m]:members)if(m.category!="Player"){world.members.push_back(m);world.records.push_back({m,"stream.world.v2",{Tree(1,std::move(dynamic.at(key))),Tree(2,key==room.key?static_cast<const Value &>(captures):no_captures)}});}
    auto w=Make(std::move(world),false,source_memory);versions.push_back(w);
    const bool cold_requested=status_.baseline_requested;
    auto publication=NewPublication(versions.size()+(cold_requested?2:0));publication->world=world_;publication->world_dynamic=w;publication->ranges=versions;
    if(status_.baseline_requested){
      build_stage="cold-components";
      std::vector<obs::ObservationEnvelope> rows;rows.reserve(11);
      for(const auto *e:envelopes){if(!e->owned_source&&e->schema.find(".stream")!=std::string::npos)Fail("stream_baseline_full_source_required");rows.push_back(*e);rows.back().stream_payload.clear();rows.back().schema=std::string("isaac.cs.")+domains[unsigned(e->domain)-1];}
      auto cold=replica::Capture(Connected(world_,1),Next(),rows,*registry_);
      // 冷组件与文档共享同一arena，移走根值后整体封闭移交，禁止再复制完整components。
      Value components=std::move(cold["components"]);static_cast<Value &>(cold)=std::move(components);
      RangeData data;data.world=world_;data.id={RangeKind::Context,"cold-components",{}};data.sample=sample;data.complete=true;data.components=std::make_shared<const Json>(std::move(cold));
      auto full=Make(std::move(data),false,source_memory);versions.push_back(full);
      RangeData manifest;manifest.world=world_;manifest.id={RangeKind::Manifest,"baseline",{}};manifest.sample=sample;manifest.complete=manifest.membership_complete=true;
      manifest.members=s->members;manifest.event_watermark=b.event_watermark;
      // 18—20实际安装域：Entity/Grid/Room/Player；其余域是完整保留，不能冒充全部原生应用。
      manifest.native_domains=(1ull<<0)|(1ull<<1)|(1ull<<2)|(1ull<<6);manifest.retained_domains=2047ull&~manifest.native_domains;
      for(const auto &v:versions)manifest.required.push_back(V(v));
      auto m=Make(std::move(manifest),false,source_memory);versions.push_back(m);publication->manifest=m;publication->ranges=versions;
    }
    for(const auto &range:publication->ranges)publication->validity.push_back({V(range),range->sample,w->sample,b.invalidation});
    SealPublication(*publication);
    // 结构变更先撤旧身体视图；普通更新不把未更新身体重复发布成新子帧。
    for(auto i=bodies_.begin();i!=bodies_.end();)if(!members.contains(i->first)||i->second->dependencies.front()!=V(s)){body_publications_.erase(i->first);i=bodies_.erase(i);}else ++i;
    std::set<RangeId> active;for(const auto &range:versions)active.insert(range->id);
    for(auto i=current_.begin();i!=current_.end();){
      const bool body=i->first.kind==RangeKind::PlayerFast;
      const bool joint=i->first.kind==RangeKind::Context&&i->first.name.starts_with("body-joint/");
      if((body&&!bodies_.contains(i->first.body))||(joint&&!active.contains(i->first)))i=current_.erase(i);else ++i;
    }
    members_=std::move(members);latest_=std::move(publication);last_capture_=b.capture;last_tick_=b.tick;
    if(cold_requested){baseline_=latest_;status_.baseline_requested=false;status_.baseline_published_local_ms=MetricMicros()/1000;}
    for(const auto &r:latest_->ranges)metrics_.Note(*r,FlowStage::Publish,MetricMicros()/1000);
    status_.ready=true;status_.ordinary++;status_.world_sequence=w->sample.sequence;status_.reason="stream_ordinary_committed";Collect();
  }catch(const ContractError &e){failed(e.what());if(e.code.find("exhausted")!=std::string::npos)exhausted_=true;}
  catch(const std::runtime_error &e){failed(e.what());}
  catch(const std::exception &e){char reason[384];std::snprintf(reason,sizeof(reason),"stream_capture_exception:%s:%u:%.240s",build_stage,build_domain,e.what());failed(reason);}
  catch(...){failed("stream_capture_allocation_or_source");}
}
// 逐身体子帧依赖明确普通版本，不改完整成员、不提升整房序号、不调用原生更新。
void Capture::PlayerPost(const BodySample &b,const capture::Source &payload) noexcept {
  Measure measure(metrics_,CpuStage::Build);
  try {
    if(!status_.ready||!registry_||!store_||b.epoch!=source_epoch_||b.run!=source_run_||b.room!=source_room_||!b.binding||!b.tick||!b.outer||!b.owner||b.required_capture!=last_capture_||last_tick_>b.tick)Fail("player_post_scope_or_dependency");
    const auto structure=current_.find({RangeKind::Structure,"world-members",{}}),resources=current_.find({RangeKind::Resource,"native-resources",{}});
    if(structure==current_.end()||resources==current_.end())Fail("player_post_structure_missing");
    if(!payload)Fail("player_post_source_missing");
    auto source_memory=memory::Reserve(memory::Pool::Values,65536+16ull*payload->Bytes(),"source_body_temporary");
    auto expanded=payload->Blocks().empty()?Json():ExpandSource(payload);
    const auto &source=payload->Blocks().empty()?payload->Read():expanded;if(Str(At(source,"contract"))!="player-fast-source-v1")Fail("player_post_source_schema");
    const auto member=Subject(At(source,"subject"),"Player",false);const auto current=members_.find(member.key);
    if(current==members_.end()||current->second!=member||member.use!=b.use||member.shape!=b.shape||Unsigned(source["subject"]["local_id"])!=b.local)Fail("player_post_identity_changed");
    std::uint64_t resource_generation=0;
    for(const auto &record:resources->second->records)if(record.member.key==member.key)
      resource_generation=MainSprite(**std::get_if<std::shared_ptr<const Json>>(&record.fields.front().value));
    if(!resource_generation||resource_generation!=Unsigned(At(source,"resource_generation")))Fail("player_post_resource_changed");
    const auto ordinary=SubjectValue(*store_,*latest_->world_dynamic,member.key,"entity");
    if(!RenderResources(source,ordinary))Fail("player_post_resource_changed");
    if(const auto old=bodies_.find(member.key);old!=bodies_.end()&&old->second->sample.tick>=b.tick)Fail("player_post_duplicate_update");
    RangeData data;data.world=world_;data.id={RangeKind::PlayerFast,"body-dynamic",member.key};data.complete=data.membership_complete=true;
    data.sample.epoch=b.epoch;data.sample.tick=b.tick;data.sample.outer=b.outer;data.sample.owner=b.owner;data.sample.mutation=b.mutation;data.sample.loss=loss_;
    data.sample.last_native_update_tick=b.tick;data.sample.binding_revision=b.binding;data.sample.required_world_sequence=status_.world_sequence;data.sample.kind=b.kind;
    data.dependencies={V(structure->second),V(resources->second),V(latest_->world_dynamic)};data.members={member};
    Json projected;projected.SetObject();auto &a=projected.GetAllocator();for(const char *k:{"common","body","sprite"})Put(projected,k,registry_->Project(At(source,k),a,b.epoch),a);
    data.records={{member,"stream.player.v1",{Tree(1,std::move(projected))}}};auto range=Make(std::move(data),false,source_memory);
    auto publication=NewPublication(latest_->ranges.size()+1);publication->world=world_;publication->world_dynamic=latest_->world_dynamic;
    publication->ranges=latest_->ranges;publication->ranges.push_back(range);publication->player_fast=range;
    publication->validity=latest_->validity;publication->validity.push_back({V(range),range->sample,range->sample,invalidation_});
    SealPublication(*publication);
    metrics_.Note(*range,FlowStage::Publish,MetricMicros()/1000);
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
