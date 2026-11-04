#include "game/client_players.h"
#include "game/client_entity_model.h"
#include "game/client_player_state.h"
#include "game/client_player_equipment.h"
#include "game/client_player_roles.h"
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
  if(begun_&&!ready_){reason="players_candidate_inflight";return false;}
  try{
    auto plan=std::make_unique<replica::PlayerApplyPlan>(replica::PlanPlayerTarget(std::move(target),applied_));
    if(!plan->target->membership_complete)throw replica::ContractError("players:partial_membership_preserves_existing_bodies");
    std::vector<ClientPlayerDescription> values;std::map<ClientObjectKey,std::shared_ptr<const ClientPlayerHistoryView>> histories;
    auto source=std::make_shared<const std::string>(replica::Text(plan->target->entities->source));
    const auto equipment=ClientDescribePlayerEquipment(*plan->target);
    for(const auto &body:plan->target->bodies){
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
      if(adapter_.prepare&&!adapter_.prepare(adapter_.context,d,reason)){reason_=reason;reason=reason_.c_str();return false;}
      values.push_back(std::move(d));
    }
    ClientPlayerRolesPrepare(values,*plan->target);
    // 先完成真实值/角色图预检；完整后续adapter缺席仍在世界写前拒绝，不用空回调放行。
    if(!adapter_.prepare||!adapter_.install||!adapter_.retire||!adapter_.relations){reason="players_role_draw_adapter_incomplete";return false;}
    std::array<ClientObjectReference,4> backups;for(unsigned i=0;i<4;++i)backups[i]=Ref(plan->target->backup_slots[i]);
    plan_=std::move(plan);values_=std::move(values);candidate_histories_=std::move(histories);backups_=backups;begun_=written_=ready_=false;reason="players_plan_ready";return true;
  }catch(const std::exception &e){reason_=e.what();reason=reason_.c_str();return false;}catch(...){reason="players_plan_allocation";return false;}
}
bool ClientPlayers::Cancel() noexcept {if(begun_||written_)return false;plan_.reset();values_.clear();candidate_histories_.clear();return true;}
bool ClientPlayers::Configure(ClientPlayerAdapter adapter) noexcept {if(plan_||native_)return false;adapter_=std::move(adapter);return true;}
bool ClientPlayers::BeginAttempt(std::uint64_t attempt) noexcept {if(!attempt||!plan_||written_)return false;attempt_=attempt;return true;}
ClientObjectStep ClientPlayers::Structure(const ClientNativeRoots &roots) noexcept {
  if(!plan_)return {false,false,true,0,0,"players_plan_missing"};
  if(plan_->duplicate)return {true,false,false,0,0,"players_duplicate_no_native_write"};
  if(plan_->rebuild_scene)return {false,false,true,0,0,"players_scene_rebuild_required"};
  if(!native_)native_=ClientNativePlayersCreate();if(!native_)return {false,false,true,0,0,"players_owner_allocation"};
  const char *why="players_begin";
  if(!begun_){if(!ClientNativePlayersBegin(native_,roots,plan_->target->entity_capture.scope,plan_->target->revision,values_,backups_,adapter_,why))return {false,false,true,0,0,why};begun_=true;}
  auto step=ClientNativePlayersStructure(native_);written_|=step.written;step.written=written_;return step;
}
ClientObjectStep ClientPlayers::Finish(const ClientEntityOwners &entities,bool finish) noexcept {
  if(plan_&&plan_->duplicate){ready_=true;return {true,false,false,0,0,"players_duplicate_no_native_write"};}
  auto step=ClientNativePlayersFinish(native_,entities,finish);written_|=step.written;step.written=written_;ready_=step.done&&!step.failed;return step;
}
bool ClientPlayers::Commit(std::uint64_t attempt) noexcept {
  if(!plan_||attempt!=attempt_||!ready_||!ClientNativePlayersCommit(native_,plan_->target->revision))return false;
  applied_=plan_->target;histories_.swap(candidate_histories_);candidate_histories_.clear();plan_.reset();begun_=written_=false;return true;
}
ClientObjectStep ClientPlayers::Retire(const ClientNativeRoots &roots) noexcept {
  histories_.clear();candidate_histories_.clear();
  auto step=ClientNativePlayersRetire(native_,roots);if(!step.done)return step;
  ClientNativePlayersDestroy(native_);native_=nullptr;plan_.reset();applied_.reset();values_.clear();backups_={};attempt_=0;begun_=written_=ready_=false;return step;
}
ClientEntityOwners ClientPlayers::Owner() noexcept {return ClientNativePlayersOwner(native_);}
bool ClientPlayers::Visit(void *context,ClientObjectVisitor visitor) const noexcept {return ClientNativePlayersVisit(native_,context,visitor);}
std::shared_ptr<const ClientPlayerHistoryView> ClientPlayers::History(const ClientObjectKey &key) const noexcept {
  if(!applied_||written_)return {};const auto found=histories_.find(key);return found==histories_.end()?nullptr:found->second;
}
ClientObjectReference ClientPlayers::Backup(unsigned slot) const noexcept {
  if(slot>=4||!applied_||written_)return {};
  const auto &r=applied_->backup_slots[slot];return {true,r.status=="Null",Key(r.key)};
}
} // namespace cs::game
