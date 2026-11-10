#pragma once
#include "game/client/player/client_player_state.h"
#include "game/client/player/client_player_equipment.h"
#include "game/client/player/client_player_roles.h"
#include "replica/player_target.h"

namespace cs::game {
// HUD只持有同一A的自有值；相等值不表示共享，关系仍以具名Key和来源状态表达。
struct ClientPlayerDisplayBody final {
  ClientObjectKey key{},participant{},group{};
  std::uint64_t shape=0,control_session=0,binding_revision=0;
  std::uint64_t sample_revision=0;
  unsigned ordinal=0,body_role=0;
  int player_type=0,roster_index=-1;
  bool membership_known=false,active=false,can_consume=false,exists=false,dead=false;
  std::shared_ptr<const ClientPlayerState> state;
  std::shared_ptr<const ClientPlayerEquipment> equipment;
  std::shared_ptr<const ClientPlayerRoleState> role;
  std::shared_ptr<const ClientPlayerSpecialState> special;
  std::map<std::string,replica::PlayerReferenceTarget> relations;
  // 查询返回已安装的最终量，不调用游戏Getter、EvaluateItems或Lua。
  const ClientEntityField *Field(const std::string &) const noexcept;
  const ClientPlayerState::Active *ActiveSlot(unsigned slot) const noexcept;
  const ClientPlayerState::Pocket *PocketSlot(unsigned slot) const noexcept;
};
// 可保存快照供读取，但继续展示前必须向Scene核对身份；退休不把旧shared_ptr变成新A。
struct ClientPlayerDisplayView final {
  struct Query final {const replica::Value *value=nullptr,*component=nullptr;};
  replica::Scope scope{};
  std::uint64_t revision=0,entity_scope=0,player_scope=0;
  std::uint64_t stream_revision=0;
  std::vector<replica::PlayerControlGroupTarget> groups;
  std::vector<replica::PlayerReferenceTarget> roster;
  std::array<replica::PlayerReferenceTarget,4> backups{};
  std::map<ClientObjectKey,ClientPlayerDisplayBody> bodies;
  const ClientPlayerDisplayBody *Body(const ClientObjectKey &) const noexcept;
  const replica::PlayerControlGroupTarget *Participant(unsigned ordinal) const noexcept;
  // 派生心量/共享结果保留Authority真实查询及参数；component提供同域graph_ref的唯一解析根。
  Query Captured(const ClientObjectKey &,const std::string &member) const noexcept;
  // 自有查询根与索引的实际容量；冷source仍由Scene完整计划租约持有。
  std::uint64_t StreamValueBytes() const;
private:
  std::shared_ptr<const replica::PlayerTarget> source_;
  // 地址只指向上方shared_ptr拥有的不可变纯值；建索引一次，HUD不再扫描组件JSON。
  std::map<ClientObjectKey,std::map<std::string,const replica::Value *>> queries_;
  std::map<ClientObjectKey,std::shared_ptr<const replica::Json>> stream_sources_;
  friend std::shared_ptr<const ClientPlayerDisplayView> ClientDescribePlayerDisplay(
      std::shared_ptr<const replica::PlayerTarget>,const std::vector<ClientPlayerDescription> &);
  friend std::shared_ptr<const ClientPlayerDisplayView> ClientDescribeStreamDisplay(const ClientPlayerDisplayView &,const std::vector<ClientPlayerDescription> &,const std::map<ClientObjectKey,std::shared_ptr<const replica::Json>> &,std::uint64_t);
};
std::shared_ptr<const ClientPlayerDisplayView> ClientDescribePlayerDisplay(
    std::shared_ptr<const replica::PlayerTarget>,const std::vector<ClientPlayerDescription> &);
std::shared_ptr<const ClientPlayerDisplayView> ClientDescribeStreamDisplay(const ClientPlayerDisplayView &,const std::vector<ClientPlayerDescription> &,const std::map<ClientObjectKey,std::shared_ptr<const replica::Json>> &,std::uint64_t);
} // namespace cs::game
