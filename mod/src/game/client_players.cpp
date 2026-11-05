#include "game/client_players.h"
#include "game/client_entity_model.h"
#include "game/client_player_state.h"
#include "game/client_player_equipment.h"
#include "game/client_player_roles.h"
#include "game/client_entity_e2e.h"
#include <algorithm>
#include "replica/stream_codec.h"
#include "game/client_resource_model.h"

namespace cs::game {
namespace {
ClientObjectKey Key(const replica::SubjectKey &v){return {v.run,v.id,v.space};}
ClientObjectReference Ref(const replica::PlayerReferenceTarget &r){
  if(r.status!="Null"&&r.status!="KnownCurrent")throw replica::ContractError("players:required_reference_unresolved");
  return {true,r.status=="Null",Key(r.key)};
}
}
ClientPlayers::ClientPlayers() noexcept {adapter_=ClientPlayerBuiltinAdapter();}
ClientPlayers::~ClientPlayers(){ClientNativePlayersDestroy(native_);}
// 纯值预检不触碰原生；失败不替换上一计划或最后提交A。
bool ClientPlayers::Prepare(std::shared_ptr<const replica::PlayerTarget> target,const char *&reason) noexcept {
  ClientEntityTestMeasure measure("player_parse");
  if(begun_&&!ready_){reason="players_candidate_inflight";return false;}
  diagnostic_={};std::snprintf(diagnostic_.phase.data(),diagnostic_.phase.size(),"plan");
  // 保留同一不可变目标的借用，异常时按实际失败Key恢复角色/shape，避免沿用末体摘要。
  const auto diagnostic_source=target;
  try{
    auto plan=std::make_unique<replica::PlayerApplyPlan>(replica::PlanPlayerTarget(std::move(target),applied_));
    if(!plan->target->native_source_complete){
      for(const auto &body:plan->target->bodies)if(!body.missing.empty())
        throw replica::PlayerContractError("players:complete_native_source_required",body.key,{},body.missing.front());
      throw replica::PlayerContractError("players:complete_native_source_required",{},{},plan->target->missing.empty()?"source_completeness":plan->target->missing.front());
    }
    if(!plan->target->membership_complete)throw replica::ContractError("players:partial_membership_preserves_existing_bodies");
    std::vector<ClientPlayerDescription> values;std::map<ClientObjectKey,std::shared_ptr<const ClientPlayerHistoryView>> histories;
    auto source=std::make_shared<const std::string>(replica::Text(plan->target->entities->source));
    const auto equipment=ClientDescribePlayerEquipment(*plan->target);
    for(const auto &body:plan->target->bodies){
      diagnostic_.body=Key(body.key);diagnostic_.shape=body.shape_revision;diagnostic_.player_type=body.player_type;diagnostic_.body_role=body.membership.body_role;
      if(!body.native_membership_known)throw replica::ContractError("players:native_membership_unavailable");
      ClientPlayerDescription d;d.entity=ClientDescribeEntity(plan->target->entities->objects.at(body.entity_index),plan->dependencies,source,true);
      d.state=ClientDescribePlayerState(plan->target->entities->objects.at(body.entity_index));
      d.role=ClientDescribePlayerRole(plan->target->entities->objects.at(body.entity_index));
      d.special=ClientDescribePlayerSpecial(plan->target->entities->objects.at(body.entity_index));
      if(d.role->is_sub!=body.is_sub_player)throw replica::ContractError("players:entity_player_sub_body_conflict");
      d.equipment=equipment.at(d.entity.key);histories.emplace(d.entity.key,d.equipment->history);
      for(unsigned i=0;i<4;++i){const int slot=d.state->active[i].backup_slot;
        d.active_backups[i]=slot<0?ClientObjectReference{true,true,{}}:Ref(plan->target->backup_slots[unsigned(slot)]);}
      if(d.entity.type!=1||!d.entity.seed)throw replica::ContractError("players:entity_shape_or_seed");
      d.player_type=body.player_type;d.membership_known=body.membership.known;d.ordinal=body.membership.ordinal;d.body_role=body.membership.body_role;
      if(d.state->player_type!=d.player_type)throw replica::ContractError("players:final_state_role_conflict");
      d.active=body.membership.active;d.can_consume=body.membership.can_consume;d.control_session=body.membership.control_session;d.binding_revision=body.membership.binding_revision;
      d.participant=Key(body.membership.participant.key);d.group=Key(body.membership.group.key);
      d.room_member=body.room_member;d.render_member=body.render_member;d.update_member=body.update_member;d.native_valid=body.native_valid;
      for(const auto &change:plan->bodies)if(change.key==body.key){d.state_changed=change.state;d.resources_changed=change.resources;}
      for(unsigned i=0;i<plan->target->roster.size();++i)if(plan->target->roster[i].key==body.key)d.roster_index=int(i);
      for(const auto &[name,r]:body.relations)d.body_relations.emplace(name,Ref(r));
      for(auto name:{"sub_storage","twin_storage","backup_storage","replaced_storage"})if(!d.body_relations.count(name))throw replica::ContractError("players:native_relationship_source_unavailable");
      if(adapter_.prepare&&!adapter_.prepare(adapter_.context,d,reason)){reason_=reason;reason=reason_.c_str();std::snprintf(diagnostic_.reason.data(),diagnostic_.reason.size(),"%s",reason);return false;}
      values.push_back(std::move(d));
    }
    ClientPlayerRolesPrepare(values,*plan->target);
    // 先完成真实值/角色图预检；完整后续adapter缺席仍在世界写前拒绝，不用空回调放行。
    if(!adapter_.prepare||!adapter_.install||!adapter_.retire||!adapter_.relations){reason="players_role_draw_adapter_incomplete";return false;}
    std::array<ClientObjectReference,4> backups;for(unsigned i=0;i<4;++i)backups[i]=Ref(plan->target->backup_slots[i]);
    auto display=ClientDescribePlayerDisplay(plan->target,values);const auto budget=ClientMeasurePlayers(*plan,values);
    plan_=std::move(plan);values_=std::move(values);candidate_histories_=std::move(histories);backups_=backups;
    candidate_display_=std::move(display);budget_=budget;begun_=written_=ready_=false;reason="players_plan_ready";return true;
  }catch(const replica::PlayerContractError &e){
    diagnostic_.body=Key(e.subject);diagnostic_.dependency=Key(e.dependency);
    diagnostic_.shape=0;diagnostic_.player_type=-1;diagnostic_.body_role=0;
    if(diagnostic_source)for(const auto &body:diagnostic_source->bodies)if(Key(body.key)==diagnostic_.body){
      diagnostic_.shape=body.shape_revision;diagnostic_.player_type=body.player_type;diagnostic_.body_role=body.membership.body_role;break;
    }
    std::snprintf(diagnostic_.field.data(),diagnostic_.field.size(),"%s",e.field.c_str());
    reason_=e.what();reason=reason_.c_str();std::snprintf(diagnostic_.reason.data(),diagnostic_.reason.size(),"%s",reason);return false;
  }catch(const std::exception &e){reason_=e.what();reason=reason_.c_str();std::snprintf(diagnostic_.reason.data(),diagnostic_.reason.size(),"%s",reason);return false;}catch(...){reason="players_plan_allocation";return false;}
}
bool ClientPlayers::Cancel() noexcept {if(begun_||written_)return false;plan_.reset();values_.clear();candidate_histories_.clear();candidate_display_.reset();budget_=applied_budget_;return true;}
bool ClientPlayers::Configure(ClientPlayerAdapter adapter) noexcept {if(plan_||native_)return false;adapter_=std::move(adapter);return true;}
bool ClientPlayers::BeginAttempt(std::uint64_t attempt) noexcept {if(!attempt||!plan_||written_)return false;attempt_=attempt;return true;}
ClientObjectStep ClientPlayers::Structure(const ClientNativeRoots &roots) noexcept {
  ClientEntityTestMeasure measure("player_structure_apply");
  const auto failed=[&](const char *reason){ClientObjectStep step{false,written_,true,0,OwnerCount(),reason};step.domain="player";diagnostic_.Record(step,"begin");return step;};
  if(!plan_)return failed("players_plan_missing");
  if(plan_->duplicate)return {true,false,false,0,0,"players_duplicate_no_native_write"};
  if(plan_->rebuild_scene)return failed("players_scene_rebuild_required");
  if(!native_)native_=ClientNativePlayersCreate();if(!native_)return failed("players_owner_allocation");
  const char *why="players_begin";
  if(!begun_){if(!ClientNativePlayersBegin(native_,roots,plan_->target->entity_capture.scope,plan_->target->revision,values_,backups_,adapter_,why))return failed(why);begun_=true;}
  auto step=ClientNativePlayersStructure(native_);written_|=step.written;step.written=written_;diagnostic_.Record(step,"structure");return step;
}
ClientObjectStep ClientPlayers::Finish(const ClientEntityOwners &entities,bool finish) noexcept {
  ClientEntityTestMeasure measure("player_links_check");
  if(plan_&&plan_->duplicate){ready_=true;return {true,false,false,0,0,"players_duplicate_no_native_write"};}
  auto step=ClientNativePlayersFinish(native_,entities,finish);written_|=step.written;step.written=written_;ready_=step.done&&!step.failed;diagnostic_.Record(step,"relations-check");return step;
}
bool ClientPlayers::Commit(std::uint64_t attempt) noexcept {
  if(!plan_||attempt!=attempt_||!ready_||!candidate_display_||!plan_->target->native_source_complete||
      !ClientNativePlayersCommit(native_,plan_->target->revision))return false;
  applied_=plan_->target;display_=std::move(candidate_display_);applied_budget_=budget_;histories_.swap(candidate_histories_);candidate_histories_.clear();plan_.reset();begun_=written_=false;return true;
}
// 所有Player先撤边，非Player才能释放；不能把清理延后到非Player地址已退休之后。
ClientObjectStep ClientPlayers::Unlink(const ClientNativeRoots &roots) noexcept {
  display_.reset();candidate_display_.reset();histories_.clear();candidate_histories_.clear();ready_=false;
  auto step=ClientNativePlayersUnlink(native_,roots);diagnostic_.Record(step,"unlink");return step;
}
ClientObjectStep ClientPlayers::Retire(const ClientNativeRoots &roots) noexcept {
  ClientEntityTestMeasure measure("player_retire");
  display_.reset();candidate_display_.reset();
  histories_.clear();candidate_histories_.clear();
  auto step=ClientNativePlayersRetire(native_,roots);diagnostic_.Record(step,"retire");if(!step.done)return step;
  ClientNativePlayersDestroy(native_);native_=nullptr;plan_.reset();applied_.reset();values_.clear();backups_={};budget_={};attempt_=0;begun_=written_=ready_=false;return step;
}
std::shared_ptr<const ClientPlayerDisplayView> ClientPlayers::Display() const noexcept {
  return applied_&&!written_?display_:nullptr;
}
bool ClientPlayers::Committed(const replica::Scope &scope,std::uint64_t revision) const noexcept {
  return applied_&&!written_&&display_&&applied_->native_source_complete&&
      applied_->scope==scope&&applied_->revision==revision&&display_->scope==scope&&display_->revision==revision;
}
ClientEntityOwners ClientPlayers::Owner() noexcept {return ClientNativePlayersOwner(native_);}
bool ClientPlayers::Visit(void *context,ClientObjectVisitor visitor) const noexcept {return ClientNativePlayersVisit(native_,context,visitor);}
#ifdef ISAACCS_E2E
std::string ClientPlayers::TestInspect(const ClientEntityOwners &entities) const noexcept {
  return applied_&&!written_?ClientNativePlayersInspect(native_,entities):"ok=0\nerror=player_inspect_not_committed\n";
}
#endif
std::shared_ptr<const ClientPlayerHistoryView> ClientPlayers::History(const ClientObjectKey &key) const noexcept {
  if(!applied_||written_)return {};const auto found=histories_.find(key);return found==histories_.end()?nullptr:found->second;
}
ClientObjectReference ClientPlayers::Backup(unsigned slot) const noexcept {
  if(slot>=4||!applied_||written_)return {};
  const auto &r=applied_->backup_slots[slot];return {true,r.status=="Null",Key(r.key)};
}
namespace {
const replica::Value &StreamAt(const replica::Value &v,const char *name){if(!v.IsObject()||!v.HasMember(name))throw replica::ContractError("players_stream_required");return v[name];}
const replica::Value &FastTree(const replica::stream::RangeData &r){return **std::get_if<std::shared_ptr<const replica::Json>>(&r.records.at(0).fields.at(0).value);}
// 新身体快值只覆盖它声明的公开字段；普通私有值仍来自精确ordinary闭包。
void OverlayFast(replica::Json &raw,const replica::stream::RangeData &fast){auto &a=raw.GetAllocator();const auto &tree=FastTree(fast);
  replica::stream::MergePiece(raw["common"],StreamAt(tree,"common"),a);replica::stream::MergePiece(raw["body"],StreamAt(tree,"body"),a);
  auto &sprite=raw["common"]["fields"]["Entity.GetSprite"];replica::stream::MergePiece(sprite,StreamAt(tree,"sprite"),a);replica::stream::FinishPieces(raw,a,fast.revision);
  // 九个公开直接float与私有backing在普通解析器中交叉核对；先保持同一选中时间线。
  auto &body=raw["body"];if(body.HasMember("application_player_state")){auto &values=body["application_player_state"]["values"];const auto &fields=tree["body"]["fields"];
    for(const auto *id:{"EntityPlayer.Damage","EntityPlayer.MoveSpeed","EntityPlayer.MaxFireDelay","EntityPlayer.ShotSpeed","EntityPlayer.TearRange","EntityPlayer.TearHeight","EntityPlayer.TearFallingSpeed","EntityPlayer.TearFallingAcceleration","EntityPlayer.Luck","EntityPlayer.SamsonBerserkCharge","EntityPlayer.IBSCharge"})if(fields.HasMember(id)){replica::Value value;value.CopyFrom(fields[id],a);replica::Put(values,id,std::move(value),a);}}
}
}
std::vector<ClientPlayerDescription> ClientPlayers::CompileStream(const replica::stream::Store &store,const replica::stream::RangeData &range,const std::map<ClientObjectKey,replica::Json> &raws,const std::map<replica::SubjectKey,replica::stream::Range> &fast) {
  if(!applied_||begun_||written_)throw replica::ContractError("players_stream_not_committed");
  const auto player=replica::stream::DomainValue(store,range,"player");const auto &rows=StreamAt(player,"bodies");if(!rows.IsArray()||rows.Size()!=values_.size())throw replica::ContractError("players_stream_membership");std::vector<ClientPlayerDescription> out;
  const replica::Value *captures=nullptr;for(const auto &record:range.records)if(record.member.key.space==7)captures=std::get<std::shared_ptr<const replica::Json>>(record.fields.at(1).value).get();if(!captures)throw replica::ContractError("players_stream_capture_scopes");
  const auto capture=[&](const char *name){const auto &v=StreamAt(*captures,name);return replica::DomainCapture{range.revision,replica::Unsigned(StreamAt(v,"scope")),replica::Unsigned(StreamAt(v,"sequence")),replica::Unsigned(StreamAt(v,"authority_tick"))};};
  std::map<ClientObjectKey,std::shared_ptr<const replica::Json>> sources;
  for(const auto &base:values_){auto found=raws.find(base.entity.key);if(found==raws.end())throw replica::ContractError("players_stream_body_missing");auto raw=replica::Copy(found->second);
    const replica::SubjectKey key{base.entity.key.run,base.entity.key.id,base.entity.key.space};auto f=fast.find(key);if(f!=fast.end()&&f->second->sample.tick>range.sample.tick)OverlayFast(raw,*f->second);
    auto object=ClientStreamEntity(base.entity,raw);ClientPlayerDescription d=base;d.entity=ClientDescribeStreamEntity(base.entity,object);d.state=ClientDescribePlayerState(object);d.role=ClientDescribePlayerRole(object);d.special=ClientDescribePlayerSpecial(object);
    // 私有完整状态继续来自ordinary；已选中的较新直接字段覆盖其同名backing，防止回退。
    if(f!=fast.end()&&f->second->sample.tick>range.sample.tick){auto selected=std::make_shared<ClientPlayerState>(*d.state);for(const auto &value:ClientStreamPlayerFields(StreamAt(FastTree(*f->second),"body"),base.player_type)){auto old=std::find_if(selected->fields.begin(),selected->fields.end(),[&](const auto &v){return v.member==value.member;});if(old==selected->fields.end())throw replica::ContractError("players_fast_backing_missing");*old=value;}d.state=std::move(selected);}
    // 每身体局部图单独解析；其余成员只提供稳定Key集合，避免N次复制全Entity DOM。
    auto entities=std::make_shared<replica::EntityTarget>();entities->objects.push_back(std::move(object));entities->source=replica::Copy(raw);
    for(const auto &e:applied_->entities->objects)if(e.key!=key){replica::EntityObjectTarget shell;shell.key=e.key;shell.presence=e.presence;entities->objects.push_back(std::move(shell));}
    replica::PlayerTarget target;target.scope=applied_->scope;target.revision=range.revision;target.entity_capture=capture("entity");target.player_capture=capture("player");target.entities=entities;
    const replica::Value *row=nullptr;for(const auto &r:rows.GetArray())if(StreamAt(StreamAt(r,"subject"),"target")==raw["subject"]["target"]){row=&r;break;}if(!row)throw replica::ContractError("players_stream_history_missing");
    replica::PlayerBodyTarget body;body.key=key;body.shape_revision=base.entity.shape_revision;body.entity_index=0;body.history=replica::Copy(StreamAt(*row,"history"));target.source=replica::Copy(*row);target.bodies.push_back(std::move(body));
    d.equipment=ClientDescribePlayerEquipment(target).at(d.entity.key);d.state_changed=d.resources_changed=true;sources.emplace(d.entity.key,std::make_shared<const replica::Json>(std::move(raw)));out.push_back(std::move(d));
  }if(!display_)throw replica::ContractError("players_stream_display_missing");candidate_display_=ClientDescribeStreamDisplay(*display_,out,sources,range.revision);return out;
}
ClientPlayerFastValues ClientPlayers::CompileFast(const replica::stream::Store &store,const replica::stream::RangeData &r) const {
  const auto key=Key(r.id.body);auto base=std::find_if(values_.begin(),values_.end(),[&](const auto &v){return v.entity.key==key;});if(base==values_.end()||base->entity.shape_revision!=r.members.at(0).shape||base->binding_revision!=r.sample.binding_revision)throw replica::ContractError("players_fast_shape_binding");
  auto raw=replica::stream::SubjectValue(store,r,r.id.body,"entity");OverlayFast(raw,r);const auto &tree=FastTree(r);
  ClientPlayerFastValues out;out.key=key;out.shape=r.members[0].shape;out.binding=r.sample.binding_revision;out.common=ClientStreamCommonFields(StreamAt(tree,"common"));out.player=ClientStreamPlayerFields(StreamAt(tree,"body"),base->player_type);std::string reason;
  if(!ClientParseResourcePose(raw["common"]["fields"]["Entity.GetSprite"],out.sprite,reason))throw replica::ContractError(reason);
  out.render=ClientStreamPlayerEquipment(*base->equipment,ClientStreamEntity(base->entity,raw));
  if(!display_)throw replica::ContractError("players_fast_display_missing");out.display=ClientDescribeStreamDisplay(*display_,{ClientPlayerFastDescription(*base,out)},{{key,std::make_shared<const replica::Json>(std::move(raw))}},r.revision);return out;
}
ClientObjectStep ClientPlayers::InstallStream(const std::vector<ClientPlayerDescription> &values,const ClientEntityOwners &entities) noexcept {auto step=ClientNativePlayersStream(native_,values,entities);if(step.done&&!step.failed)try{values_=values;for(const auto &d:values)histories_[d.entity.key]=d.equipment->history;display_=std::move(candidate_display_);}catch(...){return {false,true,true,step.executed_prefix,step.pending_owners,"players_stream_value_commit_allocation"};}return step;}
ClientObjectStep ClientPlayers::InstallFast(const ClientPlayerFastValues &v) noexcept {
  try{auto base=std::find_if(values_.begin(),values_.end(),[&](const auto &d){return d.entity.key==v.key;});if(base==values_.end()||!v.display)return {false,false,true,0,0,"players_fast_value_owner"};auto expected=ClientPlayerFastDescription(*base,v);auto step=ClientNativePlayersFast(native_,v);if(step.done&&!step.failed){*base=std::move(expected);display_=v.display;}return step;}
  catch(...){return {false,false,true,0,0,"players_fast_expected_allocation"};}
}
} // namespace cs::game
