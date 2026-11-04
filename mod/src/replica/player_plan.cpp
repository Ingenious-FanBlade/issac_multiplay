#include "replica/player_plan.h"
#include "replica/entity_plan.h"
#include <algorithm>
#include <map>
#include <set>
#include <string_view>

namespace cs::replica {
namespace {
[[noreturn]] void Fail(const char *why){throw ContractError(std::string("player_plan:")+why);}
const Value &Fields(const PlayerTarget &t,const PlayerBodyTarget &b){return std::get<EntityClassValues<EntityCategory::Player>>(t.entities->objects.at(b.entity_index).payload).fields.value;}
using Graph=std::map<std::uint64_t,const Value *>;
void Nodes(const Value &v,Graph &g){if(v.IsArray())for(const auto &x:v.GetArray())Nodes(x,g);else if(v.IsObject()){
  if(v.HasMember("node")&&v.HasMember("type")&&v["type"].IsString()&&std::string_view(v["type"].GetString())=="table")g.emplace(Unsigned(v["node"]),&v);
  for(auto i=v.MemberBegin();i!=v.MemberEnd();++i)Nodes(i->value,g);}}
// 只在原组件中解析图；节点改号不会重建owner，节点内容改变也不能被相同graph_ref遮住。
bool Equal(const Value &a,const Value &t,const Graph &ag,const Graph &tg,std::set<std::pair<const Value *,const Value *>> &seen,unsigned &budget){
  if(!budget--)Fail("comparison_budget");
  const auto resolve=[](const Value &v,const Graph &g)->const Value &{if(v.IsObject()&&v.HasMember("type")&&v["type"].IsString()&&std::string_view(v["type"].GetString())=="graph_ref"){
    const auto i=g.find(Unsigned(v["node"]));if(i==g.end())Fail("dangling_graph_ref");return *i->second;}return v;};
  const auto &x=resolve(a,ag),&y=resolve(t,tg);if(!seen.emplace(&x,&y).second)return true;
  if(x.GetType()!=y.GetType())return false;
  if(x.IsArray()){if(x.Size()!=y.Size())return false;for(unsigned i=0;i<x.Size();++i)if(!Equal(x[i],y[i],ag,tg,seen,budget))return false;return true;}
  if(x.IsObject()){
    const auto table=[](const Value &v){return v.HasMember("type")&&v["type"].IsString()&&std::string_view(v["type"].GetString())=="table";};
    const bool xt=table(x),yt=table(y);if(xt!=yt)return false;
    const unsigned xn=x.MemberCount()-unsigned(xt&&x.HasMember("node")),yn=y.MemberCount()-unsigned(yt&&y.HasMember("node"));if(xn!=yn)return false;
    for(auto i=x.MemberBegin();i!=x.MemberEnd();++i){if(xt&&std::string_view(i->name.GetString(),i->name.GetStringLength())=="node")continue;if(!y.HasMember(i->name)||!Equal(i->value,y[i->name],ag,tg,seen,budget))return false;}return true;
  }return x==y;
}
bool Same(const Value &a,const Value &t,const Graph &ag,const Graph &tg){std::set<std::pair<const Value *,const Value *>> seen;unsigned budget=1048576;return Equal(a,t,ag,tg,seen,budget);}
bool MemberSame(const Value &a,const Value &t,const char *name,const Graph &ag,const Graph &tg){const bool x=a.HasMember(name),y=t.HasMember(name);return x==y&&(!x||Same(a[name],t[name],ag,tg));}
std::map<SubjectKey,unsigned> Index(const PlayerTarget &t){
  if(!t.membership_complete)Fail("complete_membership_required_for_absence");
  std::map<SubjectKey,unsigned> out;
  for(unsigned i=0;i<t.bodies.size();++i)if(!out.emplace(t.bodies[i].key,i).second)Fail("duplicate_body");return out;
}
bool ShapeFieldsDiffer(const PlayerTarget &a,const PlayerBodyTarget &x,const PlayerTarget &t,const PlayerBodyTarget &y){
  const auto &af=a.entities->objects[x.entity_index].common.value["fields"],&tf=t.entities->objects[y.entity_index].common.value["fields"];
  for(auto name:{"Entity.Type","Entity.Variant","Entity.SubType","Entity.InitSeed"})if(af[name]!=tf[name])return true;return false;
}
// 子owner保留完整原始值及整组件图；节点号仅作组件内寻址，不能充当原生owner身份。
void Children(PlayerApplyPlan &p,const PlayerBodyTarget *a,const PlayerBodyTarget *t,const Graph &ag,const Graph &tg){
  const auto key=t?t->key:a->key;
  const std::pair<PlayerChildKind,std::vector<const char *>> groups[]={
    {PlayerChildKind::Inventory,{"EntityPlayer.GetCollectiblesList","EntityPlayer.GetVoidedCollectiblesList","EntityPlayer.GetWispCollectiblesList","EntityPlayer.GetSpoofedCollectiblesList","EntityPlayer.GetSmeltedTrinkets","EntityPlayer.GetTrinket"}},
    {PlayerChildKind::Active,{"EntityPlayer.GetActiveItemDesc","EntityPlayer.GetActiveCharge","EntityPlayer.GetBatteryCharge","EntityPlayer.GetActiveSubCharge"}},
    {PlayerChildKind::Pocket,{"EntityPlayer.GetPocketItem","EntityPlayer.GetCard","EntityPlayer.GetPill","EntityPlayer.QueuedItem"}},
    {PlayerChildKind::Effects,{"EntityPlayer.GetEffects","EntityPlayer.HasPlayerForm","EntityPlayer.GetPlayerFormCounter"}},
    {PlayerChildKind::Costume,{"EntityPlayer.GetCostumeSpriteDescs","EntityPlayer.GetCostumeLayerMap"}},
    {PlayerChildKind::Weapon,{"EntityPlayer.GetWeapon","EntityPlayer.GetWeaponModifiers","EntityPlayer.GetActiveWeaponEntity"}},
    {PlayerChildKind::History,{}}};
  for(const auto &[kind,members]:groups){bool changed=!a||!t;
    if(!changed&&kind==PlayerChildKind::History)changed=a->history!=t->history||p.applied->source!=p.target->source; // History图独立归Player组件，保守保持完整图失效。
    if(!changed){const auto &x=Fields(*p.applied,*a)["fields"],&y=Fields(*p.target,*t)["fields"];
      for(auto name:members){const bool has=x.HasMember(name),next=y.HasMember(name);if(has!=next||(has&&!Same(x[name],y[name],ag,tg))){changed=true;break;}}}
    if(changed)p.children.push_back({key,kind,!a,!t,true});
  }
}
}
PlayerApplyPlan PlanPlayerTarget(std::shared_ptr<const PlayerTarget> t,std::shared_ptr<const PlayerTarget> a){
  if(!t||!t->revision||t->bodies.size()>256)Fail("target_or_body_budget");
  PlayerApplyPlan p;p.target=std::move(t);p.applied=std::move(a);const auto desired=Index(*p.target);
  std::map<SubjectKey,unsigned> old;
  std::set<SubjectKey> replacements;
  Graph ag,tg;Nodes(p.target->entities->source,tg);if(p.applied)Nodes(p.applied->entities->source,ag);
  if(p.applied){
    if(p.target->scope!=p.applied->scope)Fail("scope_change_requires_scene_retirement");
    if(p.target->revision<p.applied->revision)Fail("stale_revision");
    if(p.target->revision==p.applied->revision){
      if(p.target->source!=p.applied->source||p.target->entities->source!=p.applied->entities->source)Fail("revision_content_conflict");
      p.duplicate=true;return p;
    }old=Index(*p.applied);
  }
  for(const auto &[key,i]:old){const auto &before=p.applied->bodies[i];auto next=desired.find(key);
    bool replace=false;
    if(next!=desired.end()){const auto &after=p.target->bodies[next->second];
      if(after.shape_revision<before.shape_revision)Fail("shape_revision_regressed");
      const bool shape=after.player_type!=before.player_type||ShapeFieldsDiffer(*p.applied,before,*p.target,after);
      replace=after.shape_revision!=before.shape_revision||shape;
      if(shape&&after.shape_revision==before.shape_revision)Fail("shape_changed_without_revision");
      if(replace)replacements.insert(key);
    }
    // 当前逐角色原位换形和递归删除合同尚归I22—I28；显式整场景重建，不能当作setter。
    if(next==desired.end()||replace){p.rebuild_scene=true;p.steps.push_back({PlayerAction::Unlink,key,i,true});p.steps.push_back({PlayerAction::Remove,key,i,true});Children(p,&before,nullptr,ag,tg);}
  }
  for(const auto &[key,i]:desired){const auto &body=p.target->bodies[i];const auto prev=old.find(key);
    const auto *before=prev==old.end()?nullptr:&p.applied->bodies[prev->second];
    const bool create=!before||replacements.count(key);
    PlayerBodyChange change;change.key=key;change.create=create;change.replace=before&&create;
    change.membership=create||before->in_public_roster!=body.in_public_roster||before->room_member!=body.room_member||before->render_member!=body.render_member||before->update_member!=body.update_member||before->source["relations"]["membership"]!=body.source["relations"]["membership"];
    if(!create)for(auto name:{"roster","backup_slots","control_groups"})change.membership|=p.applied->source["state"][name]!=p.target->source["state"][name];
    change.state=create||!Same(p.applied->entities->objects[before->entity_index].common.value,p.target->entities->objects[body.entity_index].common.value,ag,tg)||!Same(Fields(*p.applied,*before),Fields(*p.target,body),ag,tg);
    change.resources=create;
    if(!create){const auto &x=Fields(*p.applied,*before),&y=Fields(*p.target,body);
      change.resources=!MemberSame(p.applied->entities->objects[before->entity_index].common.value["fields"],p.target->entities->objects[body.entity_index].common.value["fields"],"Entity.GetSprite",ag,tg)||!MemberSame(x,y,"application_player_resources",ag,tg);
      for(auto name:{"EntityPlayer.GetBodySprite","EntityPlayer.GetCostumeSpriteDescs","EntityPlayer.GetCostumeLayerMap"})change.resources|=!MemberSame(x["fields"],y["fields"],name,ag,tg);
    }
    change.relations=create||before->source["relations"]!=body.source["relations"];
    p.bodies.push_back(change);
    if(before&&(before->room_member!=body.room_member||before->native_valid!=body.native_valid))p.rebuild_scene=true;
    if(create){p.steps.push_back({PlayerAction::Acquire,key,i});p.steps.push_back({PlayerAction::Initialize,key,i});}
    if(change.membership)p.steps.push_back({PlayerAction::Membership,key,i});
    if(change.state)p.steps.push_back({PlayerAction::State,key,i});
    if(change.resources)p.steps.push_back({PlayerAction::Resources,key,i});
    Children(p,create?nullptr:before,&body,ag,tg);
    const auto &entity=p.target->entities->objects.at(body.entity_index);
    for(const auto &r:entity.references){auto d=DescribeEntityDependency(entity,r);
      // 装备内部的owner/mainEntity也保留原始路径；不能覆盖成Player同名的另一个槽。
      if(d.member.empty())continue;
      // 内嵌Weapon的关系按最近fields成员解释，保留完整path区分五个slot。
      const auto slash=r.path.rfind("/fields/");if(slash!=std::string::npos){auto nested=r;nested.path=r.path.substr(slash);auto named=DescribeEntityDependency(entity,nested);named.path=r.path;d=std::move(named);}
      static const std::set<std::string> installed={"Entity.Parent","Entity.Child","Entity.Target","Entity.SpawnerEntity","Entity.GetMinecart","EntityPlayer.GetMainTwin","EntityPlayer.GetOtherTwin","EntityPlayer.GetSubPlayer","EntityPlayer.GetFlippedForm","EntityPlayer.GetHeldEntity","EntityPlayer.GetActiveWeaponEntity","Weapon.GetOwner","Weapon.GetMainEntity"};
      if(!installed.count(d.member)){d.phase=EntityDependencyPhase::ValueOnly;d.kind=EntityDependencyKind::AuthorityValue;}
      if(d.phase!=EntityDependencyPhase::ValueOnly&&!d.is_null){
        if(!d.has_target||d.status!="KnownCurrent")Fail("required_reference_unresolved");
        if(d.target.run!=p.target->scope.run)Fail("cross_run_reference");
        if(d.target.space==1){bool found=false;
          for(const auto &other:p.target->entities->objects)if(other.key==d.target&&other.presence==EntityMemberPresence::Current){
            if(d.expected!=EntityCategory::Unknown&&d.expected!=other.category)Fail("reference_category_mismatch");found=true;break;}
          if(!found)Fail("required_reference_missing");
        }
      }p.dependencies.push_back(std::move(d));
    }
    for(const auto &[slot,r]:body.relations){
      if(r.status!="Null"&&r.status!="KnownCurrent")Fail("body_relation_unresolved");
      if(r.has_target&&(slot=="parent"||slot=="child")){
        if(std::none_of(p.target->entities->objects.begin(),p.target->entities->objects.end(),[&](const auto &e){return e.key==r.key&&e.presence==EntityMemberPresence::Current;}))Fail("body_entity_relation_missing");
      }else if(r.has_target&&!desired.count(r.key))Fail("body_relation_missing");
      if(r.has_target&&slot!="parent"&&slot!="child"){const auto &other=p.target->bodies.at(desired.at(r.key));if(body.membership.known&&other.membership.known&&body.membership.group.key!=other.membership.group.key)Fail("body_relation_cross_control_group");}
    }
  }
  // 所有身体先取得结构，再安装跨身体/Entity后置边，允许合法构造环。
  for(const auto &[key,i]:desired)p.steps.push_back({PlayerAction::Link,key,i});
  for(const auto &[key,i]:desired)p.steps.push_back({PlayerAction::Check,key,i});
  p.cost=unsigned(p.steps.size()+p.children.size()+p.dependencies.size());
  if(p.cost>65536)Fail("plan_budget");return p;
}
} // namespace cs::replica
