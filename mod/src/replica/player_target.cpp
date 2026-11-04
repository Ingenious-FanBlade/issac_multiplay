#include "replica/player_target.h"
#include <map>
#include <set>
#include <string_view>

namespace cs::replica {
namespace {
using V=Value;
[[noreturn]] void Error(const std::string &p,const char *why){throw ContractError("player:"+p+":"+why);}
const V *Find(const V &v,const char *k){return v.IsObject()&&v.HasMember(k)?&v[k]:nullptr;}
const V &At(const V &v,const char *k){const auto *x=Find(v,k);if(!x)Error(k,"required_missing");return *x;}
std::string Str(const V &v){return v.IsString()?std::string(v.GetString(),v.GetStringLength()):std::string{};}
bool Is(const V *v,const char *s){return v&&v->IsString()&&std::string_view(v->GetString(),v->GetStringLength())==s;}
std::uint64_t U(const V &v){return Unsigned(v);}
const V &Seq(const V &v){if(v.IsArray())return v;if(v.IsObject()&&v.ObjectEmpty()){static const V empty(rapidjson::kArrayType);return empty;}Error("sequence","array_or_explicit_empty_required");}
bool Bool(const V &v){if(!v.IsBool())Error("boolean","type");return v.GetBool();}
unsigned Small(const V &v,unsigned max){const auto n=U(v);if(n>max)Error("integer","range");return unsigned(n);}
const V &Scalar(const V &v){return Is(Find(v,"type"),"int64")?At(v,"value"):v;}
SubjectKey Key(const V &v,const Scope &s,unsigned space){SubjectKey k{U(At(v,"run")),U(At(v,"id")),Small(At(v,"space"),14)};if(k.run!=s.run||!k.id||k.space!=space)Error("key","scope_or_space");return k;}
PlayerReferenceTarget Ref(const V &v,const Scope &s,unsigned space=1){
  PlayerReferenceTarget out;const auto *status=Find(v,"status");out.status=status?Str(*status):Str(At(v,"resolution"));
  static const std::set<std::string> supported={"Null","KnownCurrent","KnownHistorical","Unresolved","NotCaptured","Unavailable","NotApplicable","Retired","AmbiguousReuse","CrossEpoch","UnsupportedTarget","Unknown"};
  if(!supported.count(out.status))Error("reference","status");
  if(const auto *k=Find(v,"target")){out.key=Key(*k,s,space);out.has_target=true;}
  if((out.status=="KnownCurrent"||out.status=="KnownHistorical"||out.status=="Retired")!=out.has_target)Error("reference","target_status");
  return out;
}
SubjectKey Current(const V &v,const Scope &s,unsigned space){const auto r=Ref(v,s,space);if(r.status!="KnownCurrent")Error("subject","current_required");return r.key;}
DomainCapture Capture(const V &v,std::uint64_t revision){const auto &c=At(v,"capture");if(!Is(Find(c,"phase"),"OrdinaryPost"))Error("capture","ordinary_required");return {revision,U(At(c,"scope")),U(At(c,"sequence")),U(At(c,"authority_tick"))};}
PlayerMembershipTarget Membership(const V &v,const Scope &s){
  PlayerMembershipTarget out;if(!Find(v,"participant_local_id")||!Find(v,"control_group_local_id"))return out;
  // Registry把关系里的local_id转换为稳定Key；不把ControllerIndex变成参与者身份。
  out.participant={"KnownCurrent",Key(At(v,"participant_local_id"),s,9),true};
  out.group={"KnownCurrent",Key(At(v,"control_group_local_id"),s,10),true};
  out.ordinal=Small(At(v,"ordinal"),3);out.body_role=Small(At(v,"body_role"),6);
  out.control_session=U(At(v,"control_session"));out.binding_revision=U(At(v,"binding_revision"));
  if(!out.control_session||!out.binding_revision)Error("membership","generation_required");
  out.active=Bool(At(v,"active"));out.can_consume=Bool(At(v,"can_consume"));if(out.can_consume&&!out.active)Error("membership","inactive_consumer");out.known=true;return out;
}
void Gaps(const V &v,const std::string &p,std::vector<std::string> &out){
  if(const auto *status=Find(v,"status")){const auto s=Str(*status);if(s=="ReadFailed"||s=="NotCaptured"||s=="Unavailable"||s=="Truncated"||s=="Unsupported"||s=="Nonfinite"||s=="Unknown"||s=="NotScheduled")out.push_back(p+":"+s);}
  if(v.IsArray()){for(unsigned i=0;i<v.Size();++i)Gaps(v[i],p+"/"+std::to_string(i),out);}
  else if(v.IsObject())for(auto i=v.MemberBegin();i!=v.MemberEnd();++i)Gaps(i->value,p+"/"+Str(i->name),out);
}
}
bool HasPlayerTarget(const V &model) noexcept {
  const auto *c=Find(model,"components"),*p=c?Find(*c,"player"):nullptr,*s=p?Find(*p,"state"):nullptr;
  if(s&&Find(*s,"application_source"))return true;
  // 任一域声明20扩展就必须完整解析；缺另一域不能回退旧单Isaac合同。
  const auto *e=c?Find(*c,"entity"):nullptr,*es=e?Find(*e,"state"):nullptr,*rows=es?Find(*es,"objects"):nullptr;
  if(rows&&rows->IsArray())for(const auto &row:rows->GetArray()){const auto *body=Find(row,"body");if(body&&Find(*body,"application_player"))return true;}
  return false;
}
// 写世界前关联规范Entity与Player两域；只有同scope/token/shape才能共享身体，缺口不补值。
std::shared_ptr<const PlayerTarget> ParsePlayerTarget(const V &model,std::shared_ptr<const EntityTarget> entities){
  ValidateModel(model);auto out=std::make_shared<PlayerTarget>();out->scope=ReadScope(model["scope"]);out->revision=U(model["revision"]);
  if(!entities)entities=std::make_shared<const EntityTarget>(ParseEntityTarget(model));
  if(entities->scope!=out->scope||entities->revision!=out->revision||entities->source!=At(At(model,"components"),"entity"))Error("entity","target_identity");out->entities=std::move(entities);
  const auto &component=At(At(model,"components"),"player"),&state=At(component,"state"),&src=At(state,"application_source");
  if(U(At(src,"version"))!=2||!Is(Find(src,"contract"),"player-source-v2")||!Is(Find(src,"target"),"REPENTOGON-1.1.3/J273")||!Is(Find(src,"pose_scope"),"same-ordinary-window"))Error("source","contract_version");
  if(!Is(Find(src,"final_state"),"player-final-state-v1"))Error("source","final_state_v1_required");
  if(!Is(Find(src,"equipment_state"),"player-equipment-state-v1"))Error("source","equipment_state_v1_required");
  out->entity_capture=out->entities->capture;out->player_capture=Capture(component,out->revision);
  const auto &entity_capture=At(out->entities->source,"capture"),&player_capture=At(component,"capture");
  for(const char *k:{"outer","owner","mutation_before","mutation_after"})if(U(At(entity_capture,k))!=U(At(player_capture,k)))Error("capture","mixed_owner_token_or_mutation");
  if(!U(At(entity_capture,"outer"))||!U(At(entity_capture,"owner"))||U(At(entity_capture,"mutation_before"))!=U(At(entity_capture,"mutation_after")))Error("capture","invalid_owner_window");
  if(out->entity_capture.tick!=out->player_capture.tick||!out->player_capture.tick||U(At(src,"entity_scope"))!=out->entity_capture.scope)Error("capture","mixed_entity_player_window");
  const auto &location=At(state,"location");if(U(At(location,"entity_scope"))!=out->entity_capture.scope||U(At(location,"scope"))!=out->player_capture.scope)Error("canonical","location_scope");
  std::map<SubjectKey,unsigned> indices;for(unsigned i=0;i<out->entities->objects.size();++i){const auto &e=out->entities->objects[i];if(e.category==EntityCategory::Player&&e.presence==EntityMemberPresence::Current)indices.emplace(e.key,i);}
  const auto &membership=At(state,"membership"),&rows=Seq(At(state,"bodies"));if(rows.Size()>256||U(At(membership,"count"))!=rows.Size())Error("membership","count_or_budget");
  out->membership_complete=Bool(At(membership,"complete"))&&out->entities->membership_complete&&Is(Find(component,"coverage"),"Complete");
  std::map<SubjectKey,unsigned> seen;
  for(const auto &row:rows.GetArray()){
    PlayerBodyTarget body;body.key=Current(At(row,"subject"),out->scope,1);
    const auto e=indices.find(body.key);if(e==indices.end()||!seen.emplace(body.key,unsigned(out->bodies.size())).second)Error("body","missing_entity_or_duplicate");
    body.entity_index=e->second;const auto &entity=out->entities->objects[e->second];body.shape_revision=entity.shape_revision;
    if(U(At(At(row,"subject"),"shape_revision"))!=body.shape_revision)Error("body","shape_mismatch");
    const auto &canonical=At(row,"canonical_state");if(!Is(Find(canonical,"schema"),"isaac.cs.entity")||U(At(canonical,"scope"))!=out->entity_capture.scope||U(At(canonical,"source_scope"))!=out->entity_capture.scope||Current(At(canonical,"subject"),out->scope,1)!=body.key)Error("canonical","entity_window_or_subject");
    const auto &relations=At(row,"relations");if(Current(At(relations,"subject"),out->scope,1)!=body.key)Error("relations","subject");
    body.player_type=int(Small(At(relations,"player_type"),40));body.dead=Bool(At(relations,"dead"));body.is_sub_player=Bool(At(relations,"is_sub_player"));body.in_public_roster=Bool(At(relations,"in_public_roster"));
    if(entity.dead_known&&body.dead!=entity.dead)Error("body","dead_state_conflict");
    body.membership=Membership(At(relations,"membership"),out->scope);
    const auto &native_membership=At(row,"native_membership");
    if(U(At(native_membership,"version"))!=1)Error("native_membership","version");
    if(Is(Find(native_membership,"status"),"Value")){
      body.native_membership_known=true;body.room_member=Small(At(native_membership,"room_owners"),1)!=0;
      body.render_member=Small(At(native_membership,"render_members"),1)!=0;body.update_member=Small(At(native_membership,"update_members"),1)!=0;
      body.native_valid=Bool(At(native_membership,"valid"));
      if(body.room_member&&!body.native_valid)Error("native_membership","room_owner_without_valid_use");
      if((body.render_member||body.update_member)&&!body.room_member)Error("native_membership","consumer_without_room_owner");
      const auto &links=At(native_membership,"links");
      if(Is(Find(links,"status"),"Value"))for(const auto *name:{"sub_storage","twin_storage","backup_storage","replaced_storage"}){
        auto ref=Ref(At(links,name),out->scope);if(ref.status!="KnownCurrent"&&ref.status!="Null")body.missing.push_back(std::string(name)+":"+ref.status);body.relations.emplace(name,std::move(ref));
      }else body.missing.push_back("native_relationship_storage:Unavailable");
    }else body.missing.push_back("native_membership:Unavailable");
    for(const char *name:{"main_twin","other_twin","sub_player","parent","child"}){
      auto ref=Ref(At(relations,name),out->scope);if(ref.status!="KnownCurrent"&&ref.status!="Null")body.missing.push_back(std::string(name)+":"+ref.status);body.relations.emplace(name,std::move(ref));
    }
    body.history=Copy(At(row,"history"));body.continuity=Copy(At(row,"continuity"));body.source=Copy(row);
    const auto &fields=std::get<EntityClassValues<EntityCategory::Player>>(entity.payload).fields;
    const auto &field_values=At(fields.value,"fields");
    if(const auto *type=Find(field_values,"EntityPlayer.GetPlayerType");type&&!Find(*type,"status"))if(Small(Scalar(*type),40)!=unsigned(body.player_type))Error("body","player_type_conflict");
    body.missing.insert(body.missing.end(),entity.common.missing.begin(),entity.common.missing.end());body.missing.insert(body.missing.end(),fields.missing.begin(),fields.missing.end());
    const auto &resources=At(fields.value,"application_player_resources");if(U(At(resources,"version"))!=1)Error("player_resources","version");
    const auto &final_state=At(fields.value,"application_player_state");
    if(U(At(final_state,"version"))!=1||!Is(Find(final_state,"scope"),"same-ordinary-window"))Error("player_final_state","version_or_window");
    Gaps(final_state,"application_player_state",body.missing);
    const auto &equipment=At(fields.value,"application_player_equipment");if(U(At(equipment,"version"))!=1||!Is(Find(equipment,"scope"),"same-ordinary-window"))Error("player_equipment","version_or_window");Gaps(equipment,"application_player_equipment",body.missing);
    if(Is(Find(resources,"status"),"Value"))for(const char *name:{"inline_1","inline_2","inline_3","inline_4","inline_5","inline_6","inline_7","inline_8","inline_9","blood_gush","held","inline_12"})At(resources,name);Gaps(resources,"application_player_resources",body.missing);
    const auto &extension=At(fields.value,"application_player");if(U(At(extension,"version"))!=2||!Is(Find(extension,"contract"),"player-source-v2"))Error("body_source","contract_version");
    const auto &domains=At(extension,"slot_domains");
    for(const auto &domain:std::array<std::pair<const char *,unsigned>,4>{{{"active",4},{"pocket",4},{"weapon",5},{"trinket",2}}}){
      const auto &slots=Seq(At(domains,domain.first));if(slots.Size()!=domain.second)Error("slots","domain_size");for(unsigned i=0;i<slots.Size();++i)if(U(slots[i])!=i)Error("slots","domain_order");
    }
    Gaps(body.history,"history",body.missing);
    // 发送省略按目录显式呈现为缺口，I05恢复consumer字段后才能声明完整。
    const auto &manifest=At(extension,"member_manifest"),&values=At(fields.value,"fields");if(!manifest.IsObject())Error("manifest","object_required");
    for(auto group=manifest.MemberBegin();group!=manifest.MemberEnd();++group)for(const auto &m:Seq(group->value).GetArray()){
      const auto id=Str(At(m,"member"));if(id=="Entity.GetSprite")continue;
      if(const auto *v=Find(values,id.c_str()))Gaps(*v,id,body.missing);else body.missing.push_back(id+":MissingOrProjected");
    }
    // 备份/借用身体可没有参与者关系；存在性不依赖当前可输入资格。
    if(!body.membership.known&&body.in_public_roster)body.missing.push_back("membership:Unresolved");
    out->bodies.push_back(std::move(body));
  }
  if(out->membership_complete&&seen.size()!=indices.size())Error("membership","entity_player_set_mismatch");
  std::set<SubjectKey> roster;
  for(const auto &r:Seq(At(state,"roster")).GetArray()){
    if(U(At(r,"index"))!=out->roster.size()||out->roster.size()>=256)Error("roster","order_or_budget");auto ref=Ref(At(r,"body"),out->scope);
    if(ref.status!="KnownCurrent"||!seen.count(ref.key)||!roster.insert(ref.key).second)Error("roster","current_member_required");out->roster.push_back(std::move(ref));
  }
  for(const auto &body:out->bodies)if(body.in_public_roster!=roster.contains(body.key))Error("roster","body_flag_mismatch");
  std::set<unsigned> slots;for(const auto &r:Seq(At(state,"backup_slots")).GetArray()){
    const auto index=Small(At(r,"index"),3);if(!slots.insert(index).second)Error("backup","duplicate");auto ref=Ref(At(r,"body"),out->scope);if(ref.status!="KnownCurrent"&&ref.status!="Null")out->missing.push_back("backup/"+std::to_string(index)+":"+ref.status);if(ref.has_target&&!seen.count(ref.key))out->missing.push_back("backup/"+std::to_string(index)+":body_not_captured");out->backup_slots[index]=std::move(ref);
  }if(slots.size()!=4)Error("backup","four_explicit_slots_required");
  std::set<unsigned> ordinals;std::set<SubjectKey> participants,groups;std::map<SubjectKey,SubjectKey> member_groups;
  for(const auto &r:Seq(At(state,"control_groups")).GetArray()){
    PlayerControlGroupTarget g;g.key=Current(At(r,"subject"),out->scope,10);g.participant=Current(At(r,"participant"),out->scope,9);g.ordinal=Small(At(r,"ordinal"),3);g.control_session=U(At(r,"control_session"));g.binding_revision=U(At(r,"binding_revision"));
    if(!g.control_session||!g.binding_revision||!ordinals.insert(g.ordinal).second||!participants.insert(g.participant).second||!groups.insert(g.key).second)Error("groups","identity_or_generation_conflict");
    std::set<SubjectKey> members,active;
    for(const auto &m:Seq(At(r,"memberships")).GetArray()){
      const auto key=Current(At(m,"subject"),out->scope,1);if(!seen.count(key)||!members.insert(key).second||!member_groups.emplace(key,g.key).second||members.size()>64)Error("groups","membership_conflict_or_budget");
      const auto &body=out->bodies[seen.at(key)];const auto &b=body.membership;
      if(!b.known||b.group.key!=g.key||b.participant.key!=g.participant||b.ordinal!=g.ordinal||b.control_session!=g.control_session||b.binding_revision!=g.binding_revision||b.body_role!=Small(At(m,"body_role"),6)||b.active!=Bool(At(m,"active"))||b.can_consume!=Bool(At(m,"can_consume")))Error("groups","body_relation_conflict");g.members.push_back(key);
    }
    for(const auto &b:Seq(At(r,"bodies")).GetArray()){const auto key=Current(b,out->scope,1);if(!members.count(key)||!active.insert(key).second||!out->bodies[seen.at(key)].membership.active)Error("groups","active_anchor_conflict");g.bodies.push_back(key);}
    out->control_groups.push_back(std::move(g));
  }
  for(const auto &body:out->bodies)if(body.membership.known&&!member_groups.count(body.key))Error("groups","missing_known_member");
  out->public_complete=out->membership_complete&&Is(Find(component,"coverage"),"Complete")&&out->missing.empty();
  for(const auto &body:out->bodies)if(!body.missing.empty())out->public_complete=false;
  // 当前扩展只承诺公开来源；全角色私有draw读集未安装时不能提升native_source_complete。
  out->native_source_complete=false;out->source=Copy(component);return out;
}
const Json &PlayerApplicationCatalog(){static const auto catalog=Parse(
#include "player_application.inc"
);return catalog;}
Json PlayerCapabilities(){return Parse(R"({"id":"player-target-v2","source":"player-source-v2","catalog":1,"model":2,"target":"REPENTOGON-1.1.3/J273","participants":4,"group_bodies":64,"backup_slots":4,"active_slots":4,"pocket_slots":4,"weapon_slots":5,"final_state":"player-final-state-v1","final_state_tasks":"I11-I15:implemented;I39-not-run","equipment_state":"player-equipment-state-v1","equipment_tasks":"I16-I20:implemented;I39-not-run","history":"Scene-committed immutable per-body ordered view","native_apply":false,"structure_owner":"key/use/shape/role-qualified;roster-room-render-control-backup-separated","scope":"same-window Entity body+Player relations/History;owned typed values","legacy":"single ordinary Isaac profile remains separate","private_source":"named-consumer-writer-qualified;not blanket PlayerUpdate eligibility","runtime_acceptance":"I39-not-run"})");}
} // namespace cs::replica
