#include "game/client_objects.h"
#include "game/client_resource_model.h"
#include "game/client_entity_model.h"
#include "game/client_entity_adapters.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <string_view>

namespace cs::game {
namespace {
using V=replica::Value;
constexpr unsigned kObjectBudget=kClientEntityMembers,kPlanBudget=kClientEntityCommands;
const V *At(const V *v,const char *key) noexcept {return v&&v->IsObject()&&v->HasMember(key)?&(*v)[key]:nullptr;}
bool Is(const V *v,const char *s) noexcept {return v&&v->IsString()&&std::string_view(v->GetString(),v->GetStringLength())==s;}
const V *Scalar(const V *v) noexcept {return Is(At(v,"type"),"int64")?At(v,"value"):v;}
bool Integer(const V *v,std::int64_t &out) noexcept {
  v=Scalar(v);if(!v)return false;
  if(v->IsInt64()){out=v->GetInt64();return true;}
  if(v->IsUint64()&&v->GetUint64()<=INT64_MAX){out=std::int64_t(v->GetUint64());return true;}
  if(v->IsString()){const auto r=std::from_chars(v->GetString(),v->GetString()+v->GetStringLength(),out);return r.ec==std::errc{}&&r.ptr==v->GetString()+v->GetStringLength();}
  return false;
}
bool Unsigned(const V *v,std::uint64_t &out) noexcept {
  v=Scalar(v);if(!v)return false;
  if(v->IsUint64()){out=v->GetUint64();return true;}
  if(v->IsString()){const auto r=std::from_chars(v->GetString(),v->GetString()+v->GetStringLength(),out);return r.ec==std::errc{}&&r.ptr==v->GetString()+v->GetStringLength();}
  return false;
}
bool Int(const V *v,int &out) noexcept {std::int64_t n=0;if(!Integer(v,n)||n<INT32_MIN||n>INT32_MAX)return false;out=int(n);return true;}
bool UInt(const V *v,unsigned &out) noexcept {std::uint64_t n=0;if(!Unsigned(v,n)||n>UINT32_MAX)return false;out=unsigned(n);return true;}
bool Float(const V *v,float &out) noexcept {
  v=Scalar(v);if(!v)return false;double n=0;
  if(v->IsNumber())n=v->GetDouble();else {std::int64_t x=0;if(!Integer(v,x))return false;n=double(x);}
  if(!std::isfinite(n)||n>std::numeric_limits<float>::max()||n<-std::numeric_limits<float>::max())return false;
  out=float(n);return true;
}
bool Vector(const V *v,ClientPoseVector &out) noexcept {return Is(At(v,"type"),"vector")&&Float(At(v,"x"),out.x)&&Float(At(v,"y"),out.y);}
bool Key(const V *v,ClientObjectKey &key) noexcept {
  return v&&v->IsObject()&&Unsigned(At(v,"run"),key.run)&&key.run&&
      UInt(At(v,"space"),key.space)&&key.space==1&&Unsigned(At(v,"id"),key.id)&&key.id;
}
bool Reference(const V *v,ClientObjectReference &out) noexcept {
  if(!v)return false;out.present=true;
  // 原生Subject(nil)没有local_id/space，15保持resolution；已投影引用使用status。
  if(Is(At(v,"status"),"Null")||Is(At(v,"resolution"),"Null")){out.is_null=true;return !At(v,"target");}
  return Is(At(v,"type"),"reference")&&Is(At(v,"status"),"KnownCurrent")&&Key(At(v,"target"),out.target);
}
struct Parsed final {ClientObjectDescription description;const V *source=nullptr;};
using Objects=std::map<ClientObjectKey,Parsed>;
bool ScalarFieldsDiffer(const Parsed &a,const Parsed &b) {
  const auto *af=At(At(a.source,"common"),"fields"),*bf=At(At(b.source,"common"),"fields");
  for(const char *name:{"Entity.Position","Entity.Velocity","Entity.PositionOffset","Entity.DepthOffset","Entity.Size",
      "Entity.Mass","Entity.CollisionDamage","Entity.EntityCollisionClass","Entity.GridCollisionClass","Entity.Visible"})
    if(*At(af,name)!=*At(bf,name))return true;
  return *At(At(a.source,"body"),"fields")!=*At(At(b.source,"body"),"fields");
}
// 每个已出现字段都必须属于本adapter；未知字段不因代表路径而悄悄变成已应用。
bool FieldsSupported(const V &fields,const std::set<std::string> &supported,std::string &reason) {
  for(auto i=fields.MemberBegin();i!=fields.MemberEnd();++i){
    const std::string name(i->name.GetString(),i->name.GetStringLength());
    if(supported.count(name))continue;
    const auto *rule=replica::Policy(name);
    if(rule&&!Is(At(rule,"ownership"),"ReplicatedState"))continue;
    reason="objects_field_unsupported:"+name;return false;
  }
  return true;
}
bool Object(const V &v,Parsed &p,std::string &reason) {
  auto &d=p.description;const auto *subject=At(&v,"subject");
  if(!Is(At(subject,"status"),"KnownCurrent")||!Key(At(subject,"target"),d.key)||
     !Unsigned(At(subject,"shape_revision"),d.shape_revision)) {reason="objects_subject_invalid";return false;}
  const auto *f=At(At(&v,"common"),"fields");const auto *body=At(&v,"body");const auto *b=At(body,"fields");
  reason="objects_required_field_missing";
  if(!f||!f->IsObject()||!b||!b->IsObject()||
     !UInt(At(f,"Entity.Type"),d.type)||!UInt(At(f,"Entity.Variant"),d.variant)||
     !UInt(At(f,"Entity.SubType"),d.subtype)||!UInt(At(f,"Entity.InitSeed"),d.seed)||!d.seed||
     !Vector(At(f,"Entity.Position"),d.position)||!Vector(At(f,"Entity.Velocity"),d.velocity)||
     !Vector(At(f,"Entity.PositionOffset"),d.position_offset)||!Float(At(f,"Entity.DepthOffset"),d.depth)||
     !Float(At(f,"Entity.Size"),d.size)||!Float(At(f,"Entity.Mass"),d.mass)||
     !Float(At(f,"Entity.CollisionDamage"),d.collision_damage)||
     !Int(At(f,"Entity.EntityCollisionClass"),d.entity_collision)||
     !Int(At(f,"Entity.GridCollisionClass"),d.grid_collision))return false;
  const auto *visible=At(f,"Entity.Visible");if(!visible||!visible->IsBool())return false;d.visible=visible->GetBool();
  const char *refs[]={"Entity.Parent","Entity.Child","Entity.Target","Entity.SpawnerEntity"};
  for(unsigned n=0;n<4;++n)if(!Reference(At(f,refs[n]),d.references[n])){reason="objects_reference_invalid";return false;}
  const auto *sprite=At(f,"Entity.GetSprite");
  if(!sprite||!ClientParseResourcePose(*sprite,d.resource,reason))return false;
  static const std::set<std::string> common={"Entity.Type","Entity.Variant","Entity.SubType","Entity.InitSeed",
      "Entity.Position","Entity.Velocity","Entity.PositionOffset","Entity.DepthOffset","Entity.Size","Entity.Mass",
      "Entity.CollisionDamage","Entity.EntityCollisionClass","Entity.GridCollisionClass","Entity.Visible",
      "Entity.Parent","Entity.Child","Entity.Target","Entity.SpawnerEntity","Entity.GetSprite"};
  if(!FieldsSupported(*f,common,reason))return false;
  if(d.type==1&&d.variant==0&&d.subtype==0&&Is(At(&v,"class"),"EntityPlayer")){
    reason="objects_player_required_field_missing";
    int player_type=-1;
    if(!Int(At(b,"EntityPlayer.GetPlayerType"),player_type)||player_type!=0||
       !Float(At(b,"EntityPlayer.Damage"),d.damage)||!Float(At(b,"EntityPlayer.MoveSpeed"),d.move_speed)||
       !Float(At(b,"EntityPlayer.MaxFireDelay"),d.max_fire_delay)||!Float(At(b,"EntityPlayer.ShotSpeed"),d.shot_speed)||
       !Float(At(b,"EntityPlayer.TearRange"),d.tear_range)||!Float(At(b,"EntityPlayer.Luck"),d.luck)||
       !Int(At(b,"EntityPlayer.GetHearts"),d.hearts)||!Int(At(b,"EntityPlayer.GetMaxHearts"),d.max_hearts)||
       !Int(At(b,"EntityPlayer.GetSoulHearts"),d.soul_hearts)||!Int(At(b,"EntityPlayer.GetBlackHearts"),d.black_hearts)||
       !Int(At(b,"EntityPlayer.GetEternalHearts"),d.eternal_hearts)||!Int(At(b,"EntityPlayer.GetNumCoins"),d.coins)||
       !Int(At(b,"EntityPlayer.GetNumKeys"),d.keys)||!Int(At(b,"EntityPlayer.GetNumBombs"),d.bombs))return false;
    d.player_stats=true;
    static const std::set<std::string> player={"EntityPlayer.GetPlayerType","EntityPlayer.Damage","EntityPlayer.MoveSpeed",
        "EntityPlayer.MaxFireDelay","EntityPlayer.ShotSpeed","EntityPlayer.TearRange","EntityPlayer.Luck",
        "EntityPlayer.GetHearts","EntityPlayer.GetMaxHearts","EntityPlayer.GetSoulHearts","EntityPlayer.GetBlackHearts",
        "EntityPlayer.GetEternalHearts","EntityPlayer.GetNumCoins","EntityPlayer.GetNumKeys","EntityPlayer.GetNumBombs"};
    if(!FieldsSupported(*b,player,reason))return false;
    if(d.max_hearts<0||d.hearts<0||d.hearts>d.max_hearts||d.soul_hearts<0||d.eternal_hearts<0||
       d.coins<0||d.keys<0||d.bombs<0){reason="objects_player_stats_invalid";return false;}
  }else if(d.type==1000&&d.variant==57&&d.subtype<=1&&Is(At(&v,"class"),"EntityEffect")){
    if(!FieldsSupported(*b,{},reason))return false;
  }else {reason="objects_category_unsupported";return false;}
  if(d.entity_collision<0||d.entity_collision>5||d.grid_collision<0||d.grid_collision>6||d.size<0||d.mass<0){reason="objects_scalar_invalid";return false;}
  p.source=&v;return true;
}
bool ParseObjects(const V &model,Objects &out,std::string &reason) {
  // 17提交只覆盖显式具名代表投影；不得把完整15观测模型误报成18—20全字段已应用。
  if(!Is(At(At(&model,"client_profile"),"id"),"tx-04-17-representative-v1")){
    reason="objects_explicit_representative_profile_required";return false;
  }
  const auto *component=At(At(&model,"components"),"entity");
  const auto *state=At(component,"state");const auto *membership=At(state,"membership");
  const auto *rows=At(state,"objects");
  if(!Is(At(component,"coverage"),"Complete")||!At(component,"membership_complete")||
     !At(component,"membership_complete")->IsTrue()||!At(membership,"complete")||!At(membership,"complete")->IsTrue()||
     !rows||!rows->IsArray()){reason="objects_membership_incomplete";return false;}
  if(rows->Size()>kObjectBudget){reason="objects_budget";return false;}
  std::uint64_t run=0;
  if(!Unsigned(At(At(&model,"scope"),"run"),run)||!run){reason="objects_scope_run_invalid";return false;}
  unsigned players=0;
  for(const auto &v:rows->GetArray()){
    Parsed p;if(!Object(v,p,reason))return false;
    if(p.description.key.run!=run){reason="objects_cross_run_subject";return false;}
    players+=p.description.type==1;
    if(!out.emplace(p.description.key,std::move(p)).second){reason="objects_duplicate_key";return false;}
  }
  if(players!=1){reason="objects_representative_main_player_required";return false;}
  for(const auto &pair:out)for(const auto &ref:pair.second.description.references)
    if(!ref.is_null&&!out.count(ref.target)){reason="objects_required_reference_missing";return false;}
  return true;
}
}
ClientObjects::ClientObjects() noexcept {try{entity_adapters_=ClientEntityBuiltinAdapters();}catch(...){failed_=true;reason_="objects_builtin_adapter_allocation";}}
ClientObjects::~ClientObjects(){ClientNativeObjectsDestroy(native_);}
ClientEntityOwners ClientObjects::PlayerOwner() noexcept {return ClientNativeObjectsPlayerOwner(native_);}
bool ClientObjects::ConfigureEntityOwners(ClientEntityOwners owners,std::vector<ClientEntityClassAdapter> adapters) noexcept {
  if(candidate_||native_||committed_||failed_)return false;
  try{std::set<unsigned> categories;for(const auto &a:adapters)if(a.category==0||a.category>=13||!a.prepare||!a.before_init||!a.fields||!a.resources||!a.relations||!a.retire||!categories.insert(a.category).second)return false;
    entity_owners_=owners;entity_adapters_=std::move(adapters);return true;}catch(...){return false;}
}
bool ClientObjects::PrepareTarget(const V &target,const V *applied,const char *&reason,const replica::RoomGridTarget *room_grid) noexcept {
  try {
    if(failed_){reason="objects_failed_requires_retirement";return false;}
    // 纯值阶段的新T可原子替换尚未执行的计划；已执行前缀仍须由场景owner退休。
    if(installed_||(candidate_&&(cursor_||player_cursor_||written_))){reason="objects_plan_inflight";return false;}
    // 正式模型与历史代表投影共用同一native实例表；19不扩大17的支持声明。
    if(!Is(At(At(&target,"client_profile"),"id"),"tx-04-17-representative-v1")){
      if(applied&&(!committed_||*committed_!=*applied)){reason="objects_applied_identity_mismatch";return false;}
      if(!applied&&committed_&&!retired_){reason="objects_current_world_requires_applied";return false;}
      auto desired=std::make_shared<const replica::EntityTarget>(replica::ParseEntityTarget(target));
      auto plan=std::make_unique<replica::EntityApplyPlan>(replica::PlanEntityTarget(desired,applied?entity_applied_:nullptr));
      if(applied&&!entity_applied_){reason="objects_entity_applied_missing";return false;}
      std::vector<ClientObjectDescription> next,before;
      auto source=std::make_shared<const std::string>(replica::Text(desired->source));
      std::shared_ptr<const replica::RoomGridTarget> own_grid;
      if(!room_grid){own_grid=replica::ParseRoomGridTarget(target);room_grid=own_grid.get();}
      std::map<ClientObjectKey,const replica::LiftedGridTarget *> lifted;
      for(const auto &v:room_grid->lifted){
        if(v.descriptor_null||!lifted.emplace(ClientObjectKey{v.owner.run,v.owner.id,v.owner.space},&v).second){reason="objects_lifted_null_or_duplicate";return false;}
      }
      next.reserve(desired->objects.size());
      if(plan->duplicate)next=entity_applied_values_;
      else for(const auto &o:desired->objects){
        if(o.presence!=replica::EntityMemberPresence::Current){next.emplace_back();continue;}
        auto d=ClientDescribeEntity(o,plan->dependencies,source);
        if(d.category==9&&d.variant==136){
          const auto found=lifted.find(d.key);if(found==lifted.end()||!d.derived||desired->capture.tick!=room_grid->grid_capture.tick){reason="objects_lifted_same_target_owner_or_window";return false;}
          auto state=std::make_shared<ClientEntityClassState>(*d.derived);const auto &g=found->second->descriptor;auto &v=state->lifted;
          v.type=g.type;v.variant=g.variant;v.state=g.state;v.spawn_count=g.spawn_count;v.spawn_seed=g.spawn_seed;v.var_data=g.var_data;v.initialized=g.initialized;v.variable_seed=g.variable_seed;state->has_lifted=true;d.derived=std::move(state);lifted.erase(found);
        }
        if(o.category!=replica::EntityCategory::Player){
          const auto a=std::find_if(entity_adapters_.begin(),entity_adapters_.end(),[&](const auto &v){return v.category==d.category;});
          if(a==entity_adapters_.end()){reason="objects_entity_class_adapter_pending_I11_I22";return false;}
          if(!a->prepare(a->context,d,reason)){reason_=reason;reason=reason_.c_str();return false;}
        }next.push_back(std::move(d));
      }
      if(!plan->duplicate&&!lifted.empty()){reason="objects_lifted_requires_effect136";return false;}
      if(!plan->duplicate)for(const auto &d:next)if(d.category==1&&d.type==835&&d.variant==0&&d.derived){
        // 原版只在非空时进入，但随后固定读前两槽；长度一不能靠map旧项补成第二槽。
        if(d.derived->npc_composite_count==1){reason="objects_npc835_two_composite_slots_required";return false;}
        for(unsigned i=0;i<(std::min)(d.derived->npc_composite_count,2u);++i){const auto name="NativeNPC.Composite:"+std::to_string(i);
          const auto relation=std::find_if(d.relations.begin(),d.relations.end(),[&](const auto &r){return r.member==name;});if(relation==d.relations.end()||relation->is_null)continue;
          const auto child=std::find_if(next.begin(),next.end(),[&](const auto &v){return v.key==relation->target;});
          if(child==next.end()||!child->derived){reason="objects_npc835_path_owner_missing";return false;}
          // 普通世界分支只把Type10..999当路径主体；真实Player和其它公共实体保留引用即可。
          // 数值落在NPC范围仍须有真实NPC类别，不能把Dummy同数值误读为NPC容器。
          if(child->type>=10&&child->type<=999&&(child->category!=1||(!child->derived->npc_path.empty()&&(child->derived->npc_path.size()<3||child->derived->npc_path.size()%3)))){reason="objects_npc835_complete_path_owner_required";return false;}
        }
      }
      if(plan->applied){before=entity_applied_values_;if(before.size()!=plan->applied->objects.size()){reason="objects_entity_applied_values_missing";return false;}}
      // 20提供外部owner时只借身体；否则沿17已核合同创建/安装唯一普通Isaac，绝不冒称完整Player。
      std::vector<ClientObjectCommand> player_commands;
      if(!entity_owners_.resolve){
        if(desired->player_keys.size()!=1){reason="objects_builtin_player_requires_one_ordinary_Isaac";return false;}
        for(const auto &d:next)if(d.key.id&&d.category==0){
          if(!d.player_stats||d.type!=1||d.variant||d.subtype||!d.exists||d.dead||d.size<0||d.mass<0||
             d.entity_collision<0||d.entity_collision>5||d.grid_collision<0||d.grid_collision>6||
             d.hearts<0||d.max_hearts<d.hearts||d.soul_hearts<0||d.eternal_hearts<0||d.coins<0||d.keys<0||d.bombs<0){reason="objects_builtin_player_scope_pending_tx04_20";return false;}
          auto support=d;support.full_entity=false;
          const auto old=std::find_if(before.begin(),before.end(),[&](const auto &v){return v.key==d.key;});
          if(old!=before.end()&&(old->shape_revision!=d.shape_revision||old->seed!=d.seed)){reason="objects_builtin_player_shape_requires_rebuild";return false;}
          player_commands.push_back({old==before.end()?ClientObjectOperation::Create:ClientObjectOperation::Change,support});
          player_commands.push_back({ClientObjectOperation::Check,std::move(support)});
        }
      }
      auto usage=ClientMeasureEntities(next,unsigned(plan->steps.size()+player_commands.size()));
      auto copy=std::make_unique<replica::Json>(replica::Copy(target));
      candidate_=std::move(copy);entity_plan_=std::move(plan);entity_old_values_=std::move(before);entity_target_values_=std::move(next);
      player_commands_=std::move(player_commands);player_cursor_=0;budget_=usage;instances_ready_=ready_=capacity_checked_=false;
      commands_.clear();cursor_=prefix_=0;written_=false;retired_=false;entity_preflight_=false;reason="objects_entity_plan_ready";return true;
    }
    if(entity_applied_){reason="objects_profile_change_requires_scene_retirement";return false;}
    Objects desired,old;
    if(!ParseObjects(target,desired,reason_)){reason=reason_.c_str();return false;}
    // A来自唯一原生提交。传入的A与本表历史不一致时不在未知世界继续diff。
    if(applied){
      if(!committed_||*committed_!=*applied){reason="objects_applied_identity_mismatch";return false;}
      if(!ParseObjects(*applied,old,reason_)){reason=reason_.c_str();return false;}
    }else if(committed_&&!retired_){reason="objects_current_world_requires_applied";return false;}
    std::vector<ClientObjectCommand> plan;
    for(const auto &pair:old)if(!desired.count(pair.first)){
      if(pair.second.description.type==1){reason="objects_main_player_removal_requires_scene_rebuild";return false;}
      plan.push_back({ClientObjectOperation::Remove,pair.second.description});
    }
    // 先全部实例，后全部引用，支持合法Parent/Child环；不把环解成临时野指针。
    for(unsigned pass=0;pass<2;++pass)for(const auto &pair:desired){
      const auto found=old.find(pair.first);const auto &d=pair.second.description;
      if((d.type==1)!=(pass==0))continue;
      if(found==old.end())plan.push_back({ClientObjectOperation::Create,d});
      else {
        const auto &before=found->second.description;
        if(d.shape_revision<before.shape_revision){reason="objects_shape_revision_regressed";return false;}
        const bool shape=d.type!=before.type||d.variant!=before.variant||d.subtype!=before.subtype||d.seed!=before.seed;
        if(shape&&d.shape_revision==before.shape_revision){reason="objects_shape_revision_conflict";return false;}
        if(shape&&(d.type==1||before.type==1)){reason="objects_main_player_replacement_requires_scene_rebuild";return false;}
        if(shape)plan.push_back({ClientObjectOperation::Replace,d});
        else {
          const bool fields=ScalarFieldsDiffer(pair.second,found->second);
          const bool resource=*At(At(At(pair.second.source,"common"),"fields"),"Entity.GetSprite")!=
              *At(At(At(found->second.source,"common"),"fields"),"Entity.GetSprite");
          if(fields||resource)plan.push_back({ClientObjectOperation::Change,d,fields,resource});
        }
      }
    }
    for(const auto &pair:desired)plan.push_back({ClientObjectOperation::References,pair.second.description});
    for(const auto &pair:desired)plan.push_back({ClientObjectOperation::Check,pair.second.description});
    plan.push_back({ClientObjectOperation::Check,{}});
    if(plan.size()>kPlanBudget){reason="objects_plan_budget";return false;}
    candidate_=std::make_unique<replica::Json>(replica::Copy(target));commands_=std::move(plan);
    budget_={};budget_.members=unsigned(desired.size());budget_.players=1;budget_.resources=unsigned(desired.size());budget_.commands=unsigned(commands_.size());
    instances_ready_=ready_=false;cursor_=0;prefix_=0;written_=false;retired_=false;reason="objects_plan_ready";return true;
  }catch(const std::exception &e){reason_=e.what();reason=reason_.c_str();return false;}
  catch(...){reason="objects_plan_exception";return false;}
}
bool ClientObjects::CancelPlanned() noexcept {
  if(!candidate_||cursor_||player_cursor_||written_||failed_)return false;
  candidate_.reset();entity_plan_.reset();entity_target_values_.clear();entity_old_values_.clear();entity_preflight_=false;
  commands_.clear();player_commands_.clear();player_cursor_=0;instances_ready_=ready_=false;reason_.clear();prefix_=0;return true;
}
bool ClientObjects::BeginAttempt(std::uint64_t attempt) noexcept {if(!attempt||!candidate_||written_)return false;attempt_=attempt;return true;}
ClientObjectStep ClientObjects::Step(const ClientNativeRoots &roots,unsigned budget,bool finish) noexcept {
  if(failed_)return {false,written_,true,prefix_,0,"objects_failed_requires_retirement"};
  if(ready_)return {true,written_,false,prefix_,ClientNativeObjectsOwnerCount(native_),"objects_checked_pending_scene_commit"};
  if(!candidate_)return {false,written_,false,prefix_,0,"objects_target_missing"};
  if(!native_)native_=ClientNativeObjectsCreate();
  if(!native_)return {false,written_,true,prefix_,0,"objects_owner_allocation"};
  if(entity_plan_){
    if(!capacity_checked_){const char *why="objects_capacity";if(!ClientNativeObjectsCapacity(native_,roots,entity_target_values_,why)){failed_=true;return {false,written_,true,prefix_,ClientNativeObjectsOwnerCount(native_),why};}capacity_checked_=true;}
    if(player_cursor_<player_commands_.size()){
      auto step=ClientNativeObjectsExecute(native_,roots,player_commands_[player_cursor_]);written_|=step.written;prefix_|=step.executed_prefix;
      step.written=written_;step.executed_prefix=prefix_;if(step.failed)failed_=true;else if(step.done)++player_cursor_;
      step.done=false;return step;
    }
    if(!entity_preflight_){const char *why="objects_entity_preflight";
      const auto owners=entity_owners_.resolve?entity_owners_:ClientNativeObjectsPlayerOwner(native_);
      if(!ClientNativeObjectsConfigure(native_,owners,entity_adapters_)){failed_=true;return {false,written_,true,prefix_,0,"objects_entity_configure_failed"};}
      std::vector<ClientObjectKey> players;try{for(const auto &key:entity_plan_->external_players)players.push_back({key.run,key.id,key.space});}
      catch(...){failed_=true;return {false,written_,true,prefix_,0,"objects_entity_dependency_allocation"};}
      if(!ClientNativeObjectsPreflight(native_,roots,entity_target_values_,players,why))return {false,written_,false,prefix_,0,why};
      entity_preflight_=true;
    }
    const auto operation=[](replica::EntityAction a){switch(a){
      case replica::EntityAction::Unlink:return ClientObjectOperation::Unlink;
      case replica::EntityAction::Remove:return ClientObjectOperation::Remove;
      case replica::EntityAction::Acquire:return ClientObjectOperation::Acquire;
      case replica::EntityAction::Initialize:return ClientObjectOperation::Initialize;
      case replica::EntityAction::Fields:return ClientObjectOperation::Fields;
      case replica::EntityAction::Resources:return ClientObjectOperation::Resources;
      case replica::EntityAction::Publish:return ClientObjectOperation::Publish;
      case replica::EntityAction::Link:return ClientObjectOperation::References;
      default:return ClientObjectOperation::Check;}};
    budget=(std::min)(budget,64u);ClientObjectStep last{};
    while(budget--&&cursor_<entity_plan_->steps.size()){
      const auto &p=entity_plan_->steps[cursor_];ClientObjectCommand command;command.operation=operation(p.action);
      if(p.action==replica::EntityAction::Link||p.action==replica::EntityAction::Check||p.action==replica::EntityAction::CheckAll){
        instances_ready_=true;if(!finish)return {false,written_,false,prefix_,ClientNativeObjectsOwnerCount(native_),"objects_instances_ready_pending_grid"};}
      try{if(p.action!=replica::EntityAction::CheckAll)command.value=p.from_applied?entity_old_values_.at(p.object):entity_target_values_.at(p.object);else command.value.full_entity=true;}
      catch(...){failed_=true;return {false,written_,true,prefix_,0,"objects_entity_command_allocation"};}
      auto step=ClientNativeObjectsExecute(native_,roots,command);written_|=step.written;prefix_|=step.executed_prefix;
      step.written=written_;step.executed_prefix=prefix_;if(step.failed){failed_=true;return step;}if(!step.done)return step;last=step;++cursor_;
    }
    if(cursor_!=entity_plan_->steps.size()){last.done=false;last.written=written_;last.executed_prefix=prefix_;return last;}
    entity_installed_=entity_plan_->target;entity_installed_values_=std::move(entity_target_values_);entity_plan_.reset();entity_old_values_.clear();installed_=std::move(candidate_);ready_=true;
    return {true,written_,false,prefix_,ClientNativeObjectsOwnerCount(native_),"objects_non_player_native_checked_pending_scene_commit"};
  }
  budget=(std::min)(budget,64u);
  ClientObjectStep last{};
  while(budget--&&cursor_<commands_.size()){
    if(commands_[cursor_].operation==ClientObjectOperation::References||commands_[cursor_].operation==ClientObjectOperation::Check){
      instances_ready_=true;if(!finish)return {false,written_,false,prefix_,ClientNativeObjectsOwnerCount(native_),"objects_instances_ready_pending_grid"};}
    auto step=ClientNativeObjectsExecute(native_,roots,commands_[cursor_]);
    written_|=step.written;prefix_|=step.executed_prefix;
    if(step.failed){failed_=true;step.written=written_;step.executed_prefix=prefix_;return step;}
    if(!step.done){step.written=written_;step.executed_prefix=prefix_;return step;}
    last=step;
    ++cursor_;
  }
  // 保留本轮真正执行对象的key/instance/前缀，供统一E2E核对替换与地址复用语义。
  if(cursor_!=commands_.size()){last.done=false;last.written=written_;last.executed_prefix=prefix_;return last;}
  installed_=std::move(candidate_);commands_.clear();ready_=true;return {true,written_,false,prefix_,ClientNativeObjectsOwnerCount(native_),"objects_checked_pending_scene_commit"};
}
bool ClientObjects::Commit(std::uint64_t attempt) noexcept {
  if(!attempt||attempt!=attempt_||!ready_||!installed_||failed_)return false;
  committed_=std::move(installed_);entity_applied_=std::move(entity_installed_);entity_applied_values_=std::move(entity_installed_values_);return true;
}
ClientObjectStep ClientObjects::Retire(const ClientNativeRoots &roots) noexcept {
  auto step=ClientNativeObjectsAbandon(native_,roots);
  if(!step.done)return step;
  ClientNativeObjectsDestroy(native_);native_=nullptr;commands_.clear();candidate_.reset();committed_.reset();
  entity_plan_.reset();entity_applied_.reset();entity_target_values_.clear();entity_old_values_.clear();entity_applied_values_.clear();entity_preflight_=false;
  installed_.reset();entity_installed_.reset();entity_installed_values_.clear();player_commands_.clear();player_cursor_=0;ready_=instances_ready_=capacity_checked_=false;attempt_=0;budget_={};
  cursor_=0;prefix_=0;failed_=written_=false;retired_=true;return step;
}
bool ClientObjects::Visit(void *context,ClientObjectVisitor visitor) const noexcept {
  return !candidate_&&!installed_&&!failed_&&committed_&&ClientNativeObjectsVisit(native_,context,visitor);
}
bool ClientObjects::VisitPrepared(std::uint64_t attempt,void *context,ClientObjectVisitor visitor) const noexcept {
  return attempt&&attempt==attempt_&&ready_&&!failed_&&ClientNativeObjectsVisit(native_,context,visitor);
}
bool ClientObjects::MatchIndependent(const ClientObjectKey &key,std::uint64_t shape,std::uint64_t instance) const noexcept {
  return !installed_&&!candidate_&&!failed_&&committed_&&ClientNativeObjectsMatch(native_,key,shape,instance);
}
bool ClientObjects::CheckLifted(const replica::LiftedGridTarget &value,const char *&reason) const noexcept {
  if(candidate_||failed_||!ready_||value.descriptor_null){reason="objects_lifted_not_installed";return false;}
  const ClientObjectKey key{value.owner.run,value.owner.id,value.owner.space};
  for(const auto &d:installed_?entity_installed_values_:entity_applied_values_)if(d.key==key){
    if(d.category!=9||d.variant!=136||!d.derived||!d.derived->has_lifted){reason="objects_lifted_owner_category";return false;}
    const auto &a=d.derived->lifted;const auto &b=value.descriptor;
    if(a.type!=b.type||a.variant!=b.variant||a.state!=b.state||a.spawn_count!=b.spawn_count||a.spawn_seed!=b.spawn_seed||a.var_data!=b.var_data||a.initialized!=b.initialized||a.variable_seed!=b.variable_seed){reason="objects_lifted_descriptor_conflict";return false;}
    return ClientNativeObjectsCheckLifted(native_,key,reason);
  }reason="objects_lifted_owner_missing";return false;
}
} // namespace cs::game
