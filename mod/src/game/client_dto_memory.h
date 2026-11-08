#pragma once
#include "game/client_entity_adapters.h"
#include "game/client_grid_native.h"
#include "game/client_player_state.h"
#include "game/client_player_equipment.h"
#include "game/client_player_roles.h"
#include "game/client_presentation_native.h"
#include <type_traits>
#include <set>
#include "core/replication_memory.h"

namespace cs::game::dto_memory {
// 只计本方纯值容器的实际capacity；原生对象、图像与Lua借用不在此展开池内。
// 同一shared_ptr分配在本次全owner计量中只收费一次；标量与内嵌成员由外层sizeof覆盖。
struct Scope;
inline thread_local Scope *active=nullptr;
// 查重索引只活在同步主线程计量栈；先付4MiB工作区，最多65536个共享纯值分配。
struct Scope final {
  memory::Lease workspace=memory::Reserve(memory::Pool::Workspace,4ull<<20);
  std::set<const void *> seen;
  Scope *previous=active;
  Scope(){active=this;}
  ~Scope(){active=previous;}
  Scope(const Scope &)=delete;
  Scope &operator=(const Scope &)=delete;
  void Reset(){seen.clear();}
};
inline bool First(const void *p){if(!p)return false;if(!active)return true;if(active->seen.find(p)!=active->seen.end())return false;if(active->seen.size()>=65536)throw std::runtime_error("dto_memory_shared_capacity");active->seen.insert(p);return true;}
#define CS_DTO_MEMORY_TYPES(X) \
 X(ClientLayerPose) X(ClientResourcePose) X(ClientEntityField) X(ClientEntityRelation) \
 X(ClientEntityClassState::Resource) X(ClientEntityClassState::LaserGeometry) X(ClientEntityClassState) \
 X(ClientObjectDescription) X(ClientGridField) X(ClientGridSprite) X(ClientGridDescription) X(ClientGridCommand) \
 X(ClientPlayerState::Innate) X(ClientPlayerState::Spoof) X(ClientPlayerState) \
 X(ClientPlayerHistoryView) X(ClientPlayerEquipment::Costume) X(ClientPlayerEquipment::Weapon) X(ClientPlayerEquipment) \
 X(ClientPlayerSpecialState::Hud::Heart) X(ClientPlayerSpecialState::Image) X(ClientPlayerSpecialState) X(ClientPlayerDescription) \
 X(ClientNativeDescriptorDescription) X(ClientNativeSceneDescription) X(ClientMapDescriptor) X(ClientMapTarget) X(ClientRoomState) \
 X(ClientPresentationHeart) X(ClientPresentationFX) X(ClientPresentationRay) X(ClientPresentationRayGroup) X(ClientPresentationAux) X(ClientPresentationDescription)
#define CS_DECLARE_DTO_EXTRA(T) inline std::uint64_t Extra(const T &);
CS_DTO_MEMORY_TYPES(CS_DECLARE_DTO_EXTRA)
inline std::uint64_t Extra(const replica::RoomSpawnValue &);
inline std::uint64_t Extra(const replica::RoomConfiguration &);
#undef CS_DECLARE_DTO_EXTRA
#undef CS_DTO_MEMORY_TYPES
template<class T> std::uint64_t Extra(const T &) {static_assert(std::is_trivially_copyable<T>::value,"DTO owning members need explicit accounting");return 0;}
inline std::uint64_t Extra(const std::string &v){return v.capacity()+1+16;}
template<class T,std::size_t N> std::uint64_t Extra(const std::array<T,N> &);
template<class T> std::uint64_t Extra(const std::vector<T> &);
template<class K,class V> std::uint64_t Extra(const std::map<K,V> &);
template<class T> std::uint64_t Extra(const std::shared_ptr<T> &);
template<class T,std::size_t N> std::uint64_t Extra(const std::array<T,N> &v){std::uint64_t n=0;for(const auto &x:v)n+=Extra(x);return n;}
template<class T> std::uint64_t Extra(const std::vector<T> &v){std::uint64_t n=v.capacity()*sizeof(T)+16;for(const auto &x:v)n+=Extra(x);return n;}
template<class K,class V> std::uint64_t Extra(const std::map<K,V> &v){std::uint64_t n=0;for(const auto &x:v)n+=sizeof(x)+64+Extra(x.first)+Extra(x.second);return n;}
template<class T> std::uint64_t Extra(const std::shared_ptr<T> &v){return v&&First(v.get())?sizeof(T)+32+Extra(*v):0;}
template<class T> std::uint64_t Owned(const T &v){return sizeof(T)+Extra(v);}
inline std::uint64_t Extra(const ClientLayerPose &v){return Extra(v.name)+Extra(v.sheet);}
inline std::uint64_t Extra(const ClientResourcePose &v){return Extra(v.path)+Extra(v.animation)+Extra(v.overlay_animation)+Extra(v.shaders)+Extra(v.layer_shaders)+Extra(v.layers)+Extra(v.main_selectors)+Extra(v.overlay_selectors)+Extra(v.main_null_selectors)+Extra(v.overlay_null_selectors);}
inline std::uint64_t Extra(const ClientEntityField &v){return Extra(v.member);}
inline std::uint64_t Extra(const ClientEntityRelation &v){return Extra(v.member)+Extra(v.path)+Extra(v.contract);}
inline std::uint64_t Extra(const ClientEntityClassState::Resource &v){return Extra(v.member)+Extra(v.pose);}
inline std::uint64_t Extra(const ClientEntityClassState::LaserGeometry &v){return Extra(v.optimized)+Extra(v.unoptimized)+Extra(v.indices)+Extra(v.bezier);}
inline std::uint64_t Extra(const ClientEntityClassState &v){return Extra(v.fields)+Extra(v.resources)+Extra(v.npc_path)+Extra(v.npc_secondary_path)+Extra(v.laser)+Extra(v.pickup_cycle)+Extra(v.text)+Extra(v.owned.trail);}
inline std::uint64_t Extra(const ClientObjectDescription &v){return Extra(v.resource)+Extra(v.common_fields)+Extra(v.relations)+Extra(v.class_values)+Extra(v.common_values)+Extra(v.special_values)+Extra(v.component_values)+Extra(v.derived);}
inline std::uint64_t Extra(const ClientGridField &v){return Extra(v.name)+Extra(v.text);}
inline std::uint64_t Extra(const ClientGridSprite &v){return Extra(v.pose);}
inline std::uint64_t Extra(const ClientGridDescription &v){return Extra(v.fields)+Extra(v.sprites)+Extra(v.embers)+Extra(v.rubble);}
inline std::uint64_t Extra(const ClientGridCommand &v){return Extra(v.value);}
inline std::uint64_t Extra(const ClientPlayerState::Innate &v){return Extra(v.persistent)+Extra(v.temporary);}
inline std::uint64_t Extra(const ClientPlayerState::Spoof &v){return Extra(v.blocked)+Extra(v.groups);}
inline std::uint64_t Extra(const ClientPlayerState &v){return Extra(v.fields)+Extra(v.not_applicable)+Extra(v.collectibles)+Extra(v.voided)+Extra(v.wisps)+Extra(v.smelted)+Extra(v.spoofs);}
inline std::uint64_t Extra(const ClientPlayerHistoryView &v){return Extra(v.items);}
inline std::uint64_t Extra(const ClientPlayerEquipment::Costume &v){return Extra(v.sprite);}
inline std::uint64_t Extra(const ClientPlayerEquipment::Weapon &v){return Extra(v.links)+Extra(v.sprites);}
inline std::uint64_t Extra(const ClientPlayerEquipment &v){return Extra(v.effects)+Extra(v.history)+Extra(v.main)+Extra(v.head_animation)+Extra(v.inline_sprites)+Extra(v.costumes)+Extra(v.weapons);}
inline std::uint64_t Extra(const ClientPlayerSpecialState::Hud::Heart &v){return Extra(v.animation)+Extra(v.overlay);}
inline std::uint64_t Extra(const ClientPlayerSpecialState::Image &v){return Extra(v.path)+Extra(v.attributes);}
inline std::uint64_t Extra(const ClientPlayerSpecialState &v){return Extra(v.hud.hearts)+Extra(v.afterimages)+Extra(v.images)+Extra(v.records)+Extra(v.record_sprite);}
inline std::uint64_t Extra(const ClientPlayerDescription &v){return Extra(v.entity)+Extra(v.state)+Extra(v.equipment)+Extra(v.role)+Extra(v.special)+Extra(v.body_relations);}
inline std::uint64_t Extra(const replica::RoomSpawnValue &v){return Extra(v.entries);}
inline std::uint64_t Extra(const replica::RoomConfiguration &v){return Extra(v.content_version)+Extra(v.name)+Extra(v.spawns)+Extra(v.merged);}
inline std::uint64_t Extra(const ClientNativeDescriptorDescription &v){return Extra(v.data)+Extra(v.override_data);}
inline std::uint64_t Extra(const ClientMapDescriptor &v){return Extra(v.icons);}
inline std::uint64_t Extra(const ClientMapTarget &v){return Extra(v.descriptors);}
inline std::uint64_t Extra(const ClientRoomState &v){return Extra(v.grid_paths)+Extra(v.rail_sprite);}
inline std::uint64_t Extra(const ClientNativeSceneDescription &v){return Extra(v.descriptors)+Extra(v.map)+Extra(v.room_state)+Extra(v.backdrop_config.paths)+Extra(v.backdrop_wall_trace.steps)+Extra(v.backdrop_sprites);}
inline std::uint64_t Extra(const ClientPresentationHeart &v){return Extra(v.animation)+Extra(v.overlay);}
inline std::uint64_t Extra(const ClientPresentationFX &v){return Extra(v.attributes)+Extra(v.image)+Extra(v.poses);}
inline std::uint64_t Extra(const ClientPresentationRay &v){return Extra(v.attributes)+Extra(v.image);}
inline std::uint64_t Extra(const ClientPresentationRayGroup &v){return Extra(v.rays);}
inline std::uint64_t Extra(const ClientPresentationAux &v){return Extra(v.poop_image)+Extra(v.poop_poses)+Extra(v.veins)+Extra(v.vein_sprites);}
inline std::uint64_t Extra(const ClientPresentationDescription &v){return Extra(v.stream_map_facts)+Extra(v.room_state)+Extra(v.auxiliary)+Extra(v.map)+Extra(v.minimap_question)+Extra(v.shading)+Extra(v.minimap_poses)+Extra(v.hearts)+Extra(v.fx_layers)+Extra(v.ray_groups);}
} // namespace cs::game::dto_memory
