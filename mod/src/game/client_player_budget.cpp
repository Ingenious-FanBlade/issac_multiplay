#include "game/client_player_budget.h"
#include <rapidjson/writer.h>
namespace cs::game {
namespace {
// Writer只计真实JSON编码字节而不生成另一份大字符串；总节点沿64M口径，字节独立有界。
struct Counter final {
  using Ch=char;std::uint64_t bytes=0;
  void Put(char){if(++bytes>replica::kPlayerTargetBytes)throw replica::ContractError("player_budget:source_bytes");}
  void Flush(){}
};
void Nodes(const replica::Value &v,std::uint64_t &n,unsigned depth=0){
  if(depth>64||++n>replica::kPlayerTargetNodes)throw replica::ContractError("player_budget:source_nodes_or_depth");
  if(v.IsArray())for(const auto &x:v.GetArray())Nodes(x,n,depth+1);
  if(v.IsObject())for(auto i=v.MemberBegin();i!=v.MemberEnd();++i){Nodes(i->name,n,depth+1);Nodes(i->value,n,depth+1);}
}
void Limit(std::uint64_t value,std::uint64_t limit,const char *why){if(value>limit)throw replica::ContractError(std::string("player_budget:")+why);}
}
// 解码之后写入之前聚合一次；draw/安装不再扫描完整JSON或反复解析拓扑。
ClientPlayerBudget ClientMeasurePlayers(const replica::PlayerApplyPlan &plan,const std::vector<ClientPlayerDescription> &values){
  using namespace replica;ClientPlayerBudget b;const auto &t=*plan.target;
  Counter counter;rapidjson::Writer<Counter> writer(counter);t.source.Accept(writer);
  rapidjson::Writer<Counter> entity_writer(counter);t.entities->source.Accept(entity_writer);b.source_bytes=counter.bytes;
  Nodes(t.source,b.source_nodes);Nodes(t.entities->source,b.source_nodes);
  b.participants=unsigned(t.control_groups.size());b.bodies=unsigned(values.size());b.roster=unsigned(t.roster.size());
  Limit(b.participants,kPlayerParticipants,"participants");Limit(b.bodies,kPlayerBodies,"bodies");
  if(b.roster>b.bodies)throw ContractError("player_budget:roster_set");b.non_roster=b.bodies-b.roster;
  for(const auto &g:t.control_groups){Limit(g.members.size(),kPlayerGroupMembers,"group_nodes");b.group_nodes+=unsigned(g.members.size());}
  for(const auto &r:t.backup_slots)b.backups+=r.has_target;
  b.steps=unsigned(plan.steps.size()+plan.children.size()+plan.dependencies.size());Limit(b.steps,kPlayerPlanSteps,"steps");
  b.references=unsigned(plan.dependencies.size());
  b.owned_bytes=b.source_bytes+b.source_nodes*sizeof(Value)+b.bodies*sizeof(ClientPlayerDescription);
  const auto pose=[&](const ClientResourcePose &p){++b.resources;b.layers+=unsigned(p.layers.size());
    b.selectors+=unsigned(p.main_selectors.size()+p.overlay_selectors.size()+p.main_null_selectors.size()+p.overlay_null_selectors.size());
    b.owned_bytes+=sizeof(p)+p.path.size()+p.animation.size()+p.overlay_animation.size()+p.layers.size()*sizeof(ClientLayerPose)+
      (p.main_selectors.size()+p.overlay_selectors.size()+p.main_null_selectors.size()+p.overlay_null_selectors.size())*sizeof(int);
    for(const auto &l:p.layers)b.owned_bytes+=l.name.size()+l.sheet.size();
    for(const auto &s:p.shaders)b.owned_bytes+=s.size();for(const auto &l:p.layer_shaders)for(const auto &s:l)b.owned_bytes+=s.size();};
  for(const auto &d:values){const auto &s=*d.state;const auto &e=*d.equipment;const auto &special=*d.special;
    std::uint64_t inventory=s.collectibles.size()+s.voided.size()+s.wisps.size()+s.smelted.size();
    for(const auto &spoof:s.spoofs){inventory+=spoof.blocked.size();for(const auto &[key,g]:spoof.groups){inventory+=g.persistent.size()+g.temporary.size()+1;b.owned_bytes+=key.size();}}
    // 各原始容器的细限仍由解码器处理；聚合限制不偷偷缩小单个合法容器。
    Limit(e.effects.size(),kPlayerContainerEntries,"body_effects");Limit(e.history->items.size(),kPlayerContainerEntries,"body_history");
    Limit(e.costumes.size(),kPlayerCostumesPerBody,"body_costumes");
    b.inventory+=unsigned(inventory);b.effects+=unsigned(e.effects.size());b.history+=unsigned(e.history->items.size());b.costumes+=unsigned(e.costumes.size());
    b.records+=unsigned(special.records.size()+special.afterimages.size());b.slots+=10;
    if(special.hud.present){++b.hud_owners;b.resources+=4;b.owned_bytes+=0x6e0+0x18;
      for(const auto &h:special.hud.hearts)b.owned_bytes+=h.animation.size()+h.overlay.size();}
    pose(e.main);for(const auto &p:e.inline_sprites)pose(p);pose(special.record_sprite);
    for(const auto &c:e.costumes)pose(c.sprite);
    for(const auto &w:e.weapons)if(w.present){++b.weapons;for(const auto &[name,p]:w.sprites)pose(p);}
    b.references+=unsigned(d.body_relations.size()+d.active_backups.size());
    b.owned_bytes+=inventory*64+e.effects.size()*sizeof(ClientPlayerEquipment::Effect)+e.history->items.size()*sizeof(ClientPlayerHistoryView::Item)+
      special.records.size()*sizeof(ClientPlayerSpecialState::Record)+special.afterimages.size()*sizeof(ClientPlayerSpecialState::AfterImage)+
      s.fields.size()*sizeof(ClientEntityField)+sizeof(s)+sizeof(e)+sizeof(special);
  }
  Limit(std::uint64_t(b.inventory)+b.effects+b.history+b.costumes+b.records,kPlayerAggregateEntries,"aggregate_entries");
  Limit(b.resources,kPlayerResources,"resources");Limit(b.layers,kPlayerLayers,"layers");Limit(b.selectors,kPlayerLayers*4ull,"selectors");
  Limit(b.references,kPlayerReferences,"references");Limit(b.owned_bytes,kPlayerOwnedBytes,"owned_bytes");return b;
}
} // namespace cs::game
