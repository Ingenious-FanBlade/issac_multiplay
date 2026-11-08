#include "game/client_player_view.h"
#include "game/client_dto_memory.h"

namespace cs::game {
std::uint64_t ClientPlayerDisplayView::StreamValueBytes() const {
  // 当前视图复制出的分组、名单、查询索引与字符串均独立计费。
  std::uint64_t n=sizeof(*this)+groups.capacity()*sizeof(replica::PlayerControlGroupTarget)+roster.capacity()*sizeof(replica::PlayerReferenceTarget);
  for(const auto &g:groups)n+=(g.members.capacity()+g.bodies.capacity())*sizeof(replica::SubjectKey)+32;
  for(const auto &r:roster)n+=dto_memory::Extra(r.status);
  for(const auto &r:backups)n+=dto_memory::Extra(r.status);
  for(const auto &[key,b]:bodies){
    n+=sizeof(key)+sizeof(b)+64+dto_memory::Extra(b.state)+dto_memory::Extra(b.equipment)+dto_memory::Extra(b.role)+dto_memory::Extra(b.special);
    for(const auto &[name,r]:b.relations)n+=sizeof(name)+sizeof(r)+64+dto_memory::Extra(name)+dto_memory::Extra(r.status);
  }
  for(const auto &[key,queries]:queries_){n+=sizeof(key)+sizeof(queries)+64;for(const auto &[name,value]:queries)n+=sizeof(name)+sizeof(value)+64+dto_memory::Extra(name);}
  for(const auto &[key,source]:stream_sources_){n+=sizeof(key)+sizeof(source)+64;if(source&&dto_memory::First(source.get()))n+=replica::OwnedJsonBytes(*source)+32;}
  return n;
}
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
  const auto body=queries_.find(key);if(body==queries_.end())return {};
  const auto query=body->second.find(member);if(query==body->second.end())return {};const auto stream=stream_sources_.find(key);
  return stream!=stream_sources_.end()?Query{query->second,stream->second.get()}:source_?Query{query->second,&source_->entities->source}:Query{};
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
// 热提交只更新相应身体的不可变值/查询根；拓扑沿已提交结构，身体修订不冒充整房修订。
std::shared_ptr<const ClientPlayerDisplayView> ClientDescribeStreamDisplay(const ClientPlayerDisplayView &base,const std::vector<ClientPlayerDescription> &values,const std::map<ClientObjectKey,std::shared_ptr<const replica::Json>> &sources,std::uint64_t revision){
  auto out=std::make_shared<ClientPlayerDisplayView>(base);out->stream_revision=revision;
  for(const auto &d:values){auto it=out->bodies.find(d.entity.key);auto raw=sources.find(d.entity.key);if(it==out->bodies.end()||raw==sources.end()||!d.equipment||!d.equipment->history)throw replica::ContractError("player_stream_display_body");
    auto &b=it->second;b.state=d.state;b.equipment=d.equipment;b.special=d.special;b.role=d.role;b.exists=d.entity.exists;b.dead=d.entity.dead;b.sample_revision=revision;
    out->stream_sources_[d.entity.key]=raw->second;auto &queries=out->queries_[d.entity.key];queries.clear();const auto &fields=(*raw->second)["body"]["fields"];for(auto f=fields.MemberBegin();f!=fields.MemberEnd();++f)queries.emplace(std::string(f->name.GetString(),f->name.GetStringLength()),&f->value);
  }
  if(values.size()==out->bodies.size()&&!values.empty()){out->entity_scope=values.front().equipment->history->entity_scope;out->player_scope=values.front().equipment->history->player_scope;out->source_.reset();}
  return out;
}
} // namespace cs::game
