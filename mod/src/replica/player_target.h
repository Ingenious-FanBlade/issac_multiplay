#pragma once
#include "replica/entity_target.h"
#include <array>
#include <map>
#include <memory>

namespace cs::replica {
// 身体存在、控制资格与公开roster位置互不替代；Null、缺失、未解析保留原状态。
struct PlayerReferenceTarget final {
  std::string status;
  SubjectKey key{};
  bool has_target=false;
};
struct PlayerMembershipTarget final {
  PlayerReferenceTarget participant,group;
  std::uint64_t control_session=0,binding_revision=0;
  unsigned ordinal=0,body_role=0;
  bool known=false,active=false,can_consume=false;
};
struct PlayerBodyTarget final {
  SubjectKey key{};
  std::uint64_t shape_revision=0;
  int player_type=0;
  bool dead=false,is_sub_player=false,in_public_roster=false;
  bool native_membership_known=false,room_member=false,render_member=false,update_member=false,native_valid=false;
  PlayerMembershipTarget membership;
  std::map<std::string,PlayerReferenceTarget> relations;
  // Entity本体只存在一份；下标指向PlayerTarget.entities，History图仍在source内解析。
  unsigned entity_index=0;
  Json history,continuity,source;
  std::vector<std::string> missing;
};
struct PlayerControlGroupTarget final {
  SubjectKey participant{},key{};
  unsigned ordinal=0;
  std::uint64_t control_session=0,binding_revision=0;
  std::vector<SubjectKey> bodies,members;
};
// 不可变自有纯值合同；解析成功不表示已原生应用，也不补造非活动身体的读取资格。
struct PlayerTarget final {
  Scope scope{};
  std::uint64_t revision=0;
  DomainCapture entity_capture{},player_capture{};
  bool membership_complete=false,public_complete=false,native_source_complete=false;
  std::shared_ptr<const EntityTarget> entities;
  std::vector<PlayerBodyTarget> bodies;
  std::vector<PlayerReferenceTarget> roster;
  std::array<PlayerReferenceTarget,4> backup_slots;
  std::vector<PlayerControlGroupTarget> control_groups;
  std::vector<std::string> missing;
  Json source;
};
bool HasPlayerTarget(const Value &) noexcept;
std::shared_ptr<const PlayerTarget> ParsePlayerTarget(const Value &,std::shared_ptr<const EntityTarget> entities={});
const Json &PlayerApplicationCatalog();
Json PlayerCapabilities();
} // namespace cs::replica
