#include "game/client_players.h"
#include "game/client_entity_model.h"
#include "game/client_player_state.h"
#include "game/client_player_equipment.h"
#include "game/client_player_roles.h"
#include "game/client_entity_e2e.h"
#include <algorithm>

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
} // namespace cs::game
