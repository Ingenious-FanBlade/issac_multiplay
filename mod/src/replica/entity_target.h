#pragma once
#include "replica/room_grid_target.h"
#include <variant>

namespace cs::replica {
// 纯值分类与原生应用支持分开；Unknown保留原始分类，永不回退成NPC。
enum class EntityCategory : unsigned { Player, NPC, Tear, Projectile, Bomb, Laser, Knife, Pickup, Familiar, Effect, Slot, NativeText, NativeDummy, Unknown };
enum class EntityMemberPresence : unsigned { Current, Retired, NotCaptured };
// 一种类别只拥有自己的派生值；全部JSON均深复制，既不借用入包也不含游戏对象。
// graph_ref仍以EntityTarget.source的整component为解析域，不把分段副本当成闭合图。
struct EntityFields final { Json value; std::vector<std::string> missing; };
// 关系先作为纯值分类型依赖保存；实例解析和构造前置由I05/I09负责。
struct EntityReferenceTarget final {
  std::string path,status;
  SubjectKey target{};
  bool has_target=false;
};
template<EntityCategory Category> struct EntityClassValues final { EntityFields fields; };
using EntityPayload=std::variant<EntityClassValues<EntityCategory::Player>, EntityClassValues<EntityCategory::NPC>,
    EntityClassValues<EntityCategory::Tear>, EntityClassValues<EntityCategory::Projectile>, EntityClassValues<EntityCategory::Bomb>,
    EntityClassValues<EntityCategory::Laser>, EntityClassValues<EntityCategory::Knife>, EntityClassValues<EntityCategory::Pickup>,
    EntityClassValues<EntityCategory::Familiar>, EntityClassValues<EntityCategory::Effect>, EntityClassValues<EntityCategory::Slot>,
    EntityClassValues<EntityCategory::NativeText>, EntityClassValues<EntityCategory::NativeDummy>, EntityClassValues<EntityCategory::Unknown>>;
struct EntityObjectTarget final {
  SubjectKey key{};
  std::uint64_t shape_revision=0;
  EntityCategory category=EntityCategory::Unknown;
  EntityMemberPresence presence=EntityMemberPresence::Current;
  std::string source_class;
  EntityFields common;
  EntityPayload payload;
  // Delirium是同一NPC的专用接口；不重复分配Entity身份。
  EntityFields special;
  std::vector<EntityReferenceTarget> references;
  bool delirium=false, classification_supported=false;
  // Exists/IsDead各自可缺席/失败；不能将死亡但仍存在解释为删除。
  bool exists_known=false,exists=false,dead_known=false,dead=false;
};
struct EntityTarget final {
  Scope scope{};
  std::uint64_t revision=0;
  DomainCapture capture{};
  std::string coverage;
  bool source_complete=false,membership_complete=false,non_player_complete=false;
  std::uint32_t source_member_count=0;
  std::vector<EntityObjectTarget> objects;
  std::vector<SubjectKey> player_keys,non_player_keys;
  // 保留成员原表/失败理由/结构补充及组件内图；只读消费者不能把该值当作已应用结果。
  Json source;
};
// 接受正式模型T，不依赖client_profile、录制、夹具或未来网络传输方式。
EntityTarget ParseEntityTarget(const Value &model);
const Json &EntityApplicationCatalog();
const Value *EntityApplicationPolicy(const std::string &id);
Json EntityCapabilities();
} // namespace cs::replica
