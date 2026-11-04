#pragma once
#include "game/client_objects_native.h"
#include <map>

namespace cs::game {
// Player结构与最终状态adapter的边界；所有描述均为本方值，SDK头不越过此边界。
enum class ClientPlayerUse : unsigned { Structure=1, State=2, Relations=4, Draw=8 };
// 供子owner按实际已进入前缀清理；Entered不等于正常返回，不能再次Init或Free。
enum class ClientPlayerPrefix : unsigned { Raw=1,CtorEntered=2,Constructed=4,InitEntered=8,Initialized=16,State=32,Resources=64,Roster=128,Published=256,Linked=512,Checked=1024,FreeEntered=2048,
  Unlinked=4096,AdapterRetired=8192,EquipmentRetired=16384,StateRetired=32768,Detached=65536,CountRetired=131072,Freed=262144,HudEntered=524288,HudRetired=1048576 };
struct ClientPlayerState;
struct ClientPlayerEquipment;
struct ClientPlayerRoleState;
struct ClientPlayerSpecialState;
struct ClientPlayerDescription final {
  ClientObjectDescription entity;
  std::shared_ptr<const ClientPlayerState> state;
  std::shared_ptr<const ClientPlayerEquipment> equipment;
  std::shared_ptr<const ClientPlayerRoleState> role;
  std::shared_ptr<const ClientPlayerSpecialState> special;
  // Esau Jr VarData是槽号，不是地址；引用同T的四个备份槽，供I26角色adapter消费。
  std::array<ClientObjectReference,4> active_backups{};
  ClientObjectKey participant{},group{};
  std::uint64_t control_session=0,binding_revision=0;
  int player_type=0,roster_index=-1;
  unsigned ordinal=0,body_role=0;
  bool membership_known=false,active=false,can_consume=false;
  bool room_member=false,render_member=false,update_member=false,native_valid=false;
  bool state_changed=true,resources_changed=true;
  std::map<std::string,ClientObjectReference> body_relations;
};
// I11—I20的状态/资源子owner由公共执行器管理；角色/draw adapter不重复释放这些Weapon/ANM。
// 缺完整adapter不提升draw-ready或Scene提交。
struct ClientPlayerAdapter final {
  void *context=nullptr;
  bool (*prepare)(void *,const ClientPlayerDescription &,const char *&) noexcept=nullptr;
  bool (*install)(void *,void *,const ClientPlayerDescription &,bool resources,bool check,const char *&)=nullptr;
  bool (*retire)(void *,void *,const ClientPlayerDescription &,unsigned prefix,const char *&)=nullptr;
  // 角色私有/Weapon等拥有型关系由相应adapter处理，通用层只安装核定的EntityPtr/裸借用。
  bool (*relations)(void *,void *,const ClientPlayerDescription &,const ClientEntityOwners &,const ClientNativeRoots &,bool check,bool clear,const char *&)=nullptr;
  std::shared_ptr<void> lifetime;
};
struct ClientNativePlayers;
ClientNativePlayers *ClientNativePlayersCreate() noexcept;
void ClientNativePlayersDestroy(ClientNativePlayers *) noexcept;
unsigned ClientNativePlayersOwnerCount(const ClientNativePlayers *) noexcept;
bool ClientNativePlayersBegin(ClientNativePlayers *,const ClientNativeRoots &,std::uint64_t scope,std::uint64_t revision,
    const std::vector<ClientPlayerDescription> &,const std::array<ClientObjectReference,4> &,const ClientPlayerAdapter &,const char *&) noexcept;
ClientObjectStep ClientNativePlayersStructure(ClientNativePlayers *) noexcept;
ClientObjectStep ClientNativePlayersFinish(ClientNativePlayers *,const ClientEntityOwners &,bool finish) noexcept;
ClientObjectStep ClientNativePlayersRetire(ClientNativePlayers *,const ClientNativeRoots &) noexcept;
// 先关闭全部用途借用，再在有效Manager/context逐体撤边；不释放任何身体或非Player。
ClientObjectStep ClientNativePlayersUnlink(ClientNativePlayers *,const ClientNativeRoots &) noexcept;
ClientEntityOwners ClientNativePlayersOwner(ClientNativePlayers *) noexcept;
bool ClientNativePlayersCommit(ClientNativePlayers *,std::uint64_t revision) noexcept;
bool ClientNativePlayersVisit(ClientNativePlayers *,void *,ClientObjectVisitor) noexcept;
#ifdef ISAACCS_E2E
// 显式E2E Manager窗口回读实际对象、关系、资源和成员集合；失败只返回诊断。
std::string ClientNativePlayersInspect(ClientNativePlayers *,const ClientEntityOwners &) noexcept;
#endif
// 用途/shape/scope/实例共同核对；票据不替代下一次调用时的重新借用。
bool ClientNativePlayersBorrow(ClientNativePlayers *,const ClientNativeRoots &,const ClientObjectKey &,std::uint64_t scope,
    std::uint64_t shape,ClientPlayerUse,ClientEntityBorrow &,const char *&) noexcept;
} // namespace cs::game
