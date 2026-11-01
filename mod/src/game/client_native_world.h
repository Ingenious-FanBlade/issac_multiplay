#pragma once
#include <cstdint>

namespace cs::game {
// SDK 边界只接收具名纯值；结构不包含网络身份，也不把本地指针带到后台。
// 正式 key 仍由 15 模型/I2 身份表持有，下面的索引只用于原生 descriptor 寻址。
struct ClientNativeSceneDescription final {
  int stage=0,stage_type=0,difficulty=0,challenge=0;
  int config_stage=0,config_mode=0,room_type=0,variant=0,subtype=0;
  int shape=0,width=0,height=0,config_flags=0;
  int room_index=0,list_index=0,safe_grid_index=0,dimension=0,enter_door=-1;
  int allowed_doors=0,descriptor_flags=0,visited_count=0,clear_count=0;
  int door_targets[8]{};
  std::uint32_t spawn_seed=0,decoration_seed=0,award_seed=0;
  // 原生背景surface的真实构建流；来源是Authority Backdrop本体，不能用descriptor猜代。
  std::uint32_t backdrop_seed=0,backdrop_shift1=0,backdrop_shift2=0,backdrop_shift3=0;
  unsigned backdrop=0;
  float water_amount=0;
  float camera_x=0,camera_y=0;
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
ClientNativeStep ClientNativeWorldPrepare(ClientNativeWorldOwner *,const ClientNativeSceneDescription &) noexcept;
// Backdrop.Init 的教程surface生成会读取真实首Player，必须在I2发布roster/EL后调用。
ClientNativeStep ClientNativeWorldFinalize(ClientNativeWorldOwner *) noexcept;
ClientNativeStep ClientNativeWorldCheckCommit(ClientNativeWorldOwner *) noexcept;
ClientNativeStep ClientNativeWorldRetire(ClientNativeWorldOwner *) noexcept;
ClientNativeRoots ClientNativeWorldRoots(ClientNativeWorldOwner *) noexcept;
bool ClientNativeWorldHasPrefix(const ClientNativeWorldOwner *) noexcept;
bool ClientNativeWorldSafeMenu(const ClientNativeWorldOwner *) noexcept;
bool ClientNativeWorldCanReuse(const ClientNativeWorldOwner *,const ClientNativeSceneDescription &) noexcept;
} // namespace cs::game
