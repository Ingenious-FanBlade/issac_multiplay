#pragma once
#include <cstdint>
#include <array>
#include <vector>
#include "replica/room_configuration.h"
#include "game/client/room/client_room_native.h"
#include "game/client/room/client_map_native.h"
#include "game/client/room/client_backdrop_native.h"

namespace cs::game {
// 背景配置属于Room内嵌Backdrop，不修改共享XML/Host资源；字符串只接受逻辑资源路径。
struct ClientBackdropConfiguration final {
  std::array<std::string,12> paths{};
  std::array<unsigned,7> values{};
  bool operator==(const ClientBackdropConfiguration &b) const {return paths==b.paths&&values==b.values;}
};
// SDK边界只接收具名纯值；Key只用于核对本地承载代次，不分配身份、不携带对象指针。
struct ClientNativeDescriptorDescription final {
  std::uint64_t key_run=0,key_id=0,construction_revision=0;
  int grid=0,safe=0,list=0,dimension=0,storage=-1,source_slot=0;
  std::array<int,4> occupied{};
  unsigned occupied_count=0;
  int config_stage=0,config_mode=0,type=0,subtype=0,shape=0,width=0,height=0;
  std::uint32_t variant=0;
  bool data_null=false,override_null=false;
  replica::RoomConfiguration data{},override_data{};
  int allowed=0,flags=0,display_flags=0,visited=0,clear_count=0;
  std::array<int,8> doors{};
  std::uint32_t spawn=0,decoration=0,award=0;
};
// 正式Key仍由15身份表持有；各索引只用于已核版本的原生数组有界寻址。
struct ClientNativeSceneDescription final {
  int stage=0,stage_type=0,difficulty=0,challenge=0;
  int config_stage=0,config_mode=0,room_type=0,subtype=0;
  std::uint32_t variant=0;
  int shape=0,width=0,height=0,config_flags=0;
  int grid_width=0,grid_height=0;
  int room_index=0,grid_index=0,list_index=0,safe_grid_index=0,dimension=0,enter_door=-1,starting_index=0;
  // 首项为当前descriptor，其余为正式来源实际提供的相关descriptor；不补造未知条目。
  std::vector<ClientNativeDescriptorDescription> descriptors;
  int allowed_doors=0,descriptor_flags=0,visited_count=0,clear_count=0;
  int door_targets[8]{};
  std::uint32_t spawn_seed=0,decoration_seed=0,award_seed=0;
  // 原生背景surface的真实构建流；来源是Authority Backdrop本体，不能用descriptor猜代。
  std::uint32_t backdrop_seed=0,backdrop_shift1=0,backdrop_shift2=0,backdrop_shift3=0;
  unsigned backdrop=0;
  float water_amount=0;
  float camera_x=0,camera_y=0;
  ClientMapTarget map;
  ClientRoomState room_state{};
  ClientBackdropConfiguration backdrop_config{};
  ClientBackdropWallTrace backdrop_wall_trace{};
  std::array<float,4> backdrop_surface_inputs{};
  std::array<ClientResourcePose,4> backdrop_sprites{};
  std::array<bool,4> backdrop_sprite_present{};
};
enum class ClientNativeStepState : unsigned { Progress, Ready, Waiting, Failed };
// 前缀为实际已开始的子步骤；失败时保留，只有真实退休完成后才归零。
struct ClientNativeStep final {
  ClientNativeStepState state=ClientNativeStepState::Waiting;
  unsigned prefix=0;
  bool written=false,safe_menu=false;
  const char *reason="waiting";
};
struct ClientNativeWorldOwner;
// 这些根只在已核主线程用途窗口内借用；不转移 Game/Room/Player 的原生所有权。
struct ClientNativeRoots final {void *game=nullptr,*room=nullptr,*descriptor=nullptr,*camera=nullptr;};
ClientNativeWorldOwner *ClientNativeWorldCreate() noexcept;
void ClientNativeWorldDestroy(ClientNativeWorldOwner *) noexcept;
// 只读本方描述容器容量，不读取游戏对象或图像分配。
std::uint64_t ClientNativeWorldValueBytes(const ClientNativeWorldOwner *);
ClientNativeStep ClientNativeWorldPrepare(ClientNativeWorldOwner *,const ClientNativeSceneDescription &) noexcept;
ClientNativeStep ClientNativeWorldRefresh(ClientNativeWorldOwner *,const ClientNativeSceneDescription &) noexcept;
// Backdrop.Init 的教程surface生成会读取真实首Player，必须在I2发布roster/EL后调用。
ClientNativeStep ClientNativeWorldFinalize(ClientNativeWorldOwner *) noexcept;
ClientNativeStep ClientNativeWorldCheckCommit(ClientNativeWorldOwner *) noexcept;
ClientNativeStep ClientNativeWorldRetire(ClientNativeWorldOwner *) noexcept;
ClientNativeRoots ClientNativeWorldRoots(ClientNativeWorldOwner *) noexcept;
bool ClientNativeWorldHasPrefix(const ClientNativeWorldOwner *) noexcept;
bool ClientNativeWorldSafeMenu(const ClientNativeWorldOwner *) noexcept;
bool ClientNativeWorldCanReuse(const ClientNativeWorldOwner *,const ClientNativeSceneDescription &) noexcept;
} // namespace cs::game
