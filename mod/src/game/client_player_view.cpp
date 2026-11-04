#include "game/client_player_view.h"

namespace cs::game {
const ClientEntityField *ClientPlayerDisplayBody::Field(const std::string &name) const noexcept {
  if(state)for(const auto &f:state->fields)if(f.member==name)return &f;
  return nullptr; // NotApplicable/未声明保持缺席，不能补成0。
}
const ClientPlayerState::Active *ClientPlayerDisplayBody::ActiveSlot(unsigned slot) const noexcept {
  return state&&slot<state->active.size()?&state->active[slot]:nullptr;
}
const ClientPlayerState::Pocket *ClientPlayerDisplayBody::PocketSlot(unsigned slot) const noexcept {
  return state&&slot<state->pocket.size()?&state->pocket[slot]:nullptr;
}
const ClientPlayerDisplayBody *ClientPlayerDisplayView::Body(const ClientObjectKey &key) const noexcept {
  const auto found=bodies.find(key);return found==bodies.end()?nullptr:&found->second;
}
const replica::PlayerControlGroupTarget *ClientPlayerDisplayView::Participant(unsigned ordinal) const noexcept {
  for(const auto &g:groups)if(g.ordinal==ordinal)return &g;return nullptr;
}
ClientPlayerDisplayView::Query ClientPlayerDisplayView::Captured(const ClientObjectKey &key,const std::string &member) const noexcept {
  if(!source_)return {};
  const auto body=queries_.find(key);if(body==queries_.end())return {};
  const auto query=body->second.find(member);return query==body->second.end()?Query{}:Query{query->second,&source_->entities->source};
}
// 候选视图在写世界前一次建立；只有Scene提交才能发布，失败/取消不污染旧A。
std::shared_ptr<const ClientPlayerDisplayView> ClientDescribePlayerDisplay(
    std::shared_ptr<const replica::PlayerTarget> source,const std::vector<ClientPlayerDescription> &values) {
  if(!source)throw replica::ContractError("player_display:source_missing");const auto &target=*source;
  auto out=std::make_shared<ClientPlayerDisplayView>();out->scope=target.scope;out->revision=target.revision;
  out->source_=std::move(source);
  out->entity_scope=target.entity_capture.scope;out->player_scope=target.player_capture.scope;
  out->groups=target.control_groups;out->roster=target.roster;out->backups=target.backup_slots;
  for(const auto &d:values){
    if(!d.state||!d.equipment||!d.equipment->history||!d.special||!d.role)
      throw replica::ContractError("player_display:incomplete_body");
    ClientPlayerDisplayBody b;b.key=d.entity.key;b.participant=d.participant;b.group=d.group;
    b.shape=d.entity.shape_revision;b.control_session=d.control_session;b.binding_revision=d.binding_revision;
    b.ordinal=d.ordinal;b.body_role=d.body_role;b.player_type=d.player_type;b.roster_index=d.roster_index;
    b.membership_known=d.membership_known;b.active=d.active;b.can_consume=d.can_consume;b.exists=d.entity.exists;b.dead=d.entity.dead;
    b.state=d.state;b.equipment=d.equipment;b.special=d.special;b.role=d.role;
    const auto &history=*d.equipment->history;
    if(history.revision!=target.revision||history.entity_scope!=out->entity_scope||history.player_scope!=out->player_scope)
      throw replica::ContractError("player_display:mixed_history_window");
    bool found=false;for(const auto &source:target.bodies)if(source.key.run==b.key.run&&source.key.id==b.key.id&&source.key.space==b.key.space){
      b.relations=source.relations;
      const auto &fields=std::get<replica::EntityClassValues<replica::EntityCategory::Player>>(target.entities->objects.at(source.entity_index).payload).fields.value["fields"];
      auto &queries=out->queries_[b.key];for(auto i=fields.MemberBegin();i!=fields.MemberEnd();++i)queries.emplace(std::string(i->name.GetString(),i->name.GetStringLength()),&i->value);
      found=true;break;}
    if(!found||!out->bodies.emplace(b.key,std::move(b)).second)throw replica::ContractError("player_display:body_identity");
  }
  if(out->bodies.size()!=target.bodies.size())throw replica::ContractError("player_display:body_set");
  return out;
}
} // namespace cs::game
