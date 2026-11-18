#pragma once
#include "game/client/scene/client_native_roots.h"
#include "game/client/display/client_display_bridge.h"
#include "game/client/display/client_display_memory.h"
#include "state/display_contract.h"
#include <cstddef>
#include <cstdint>
#include <array>
#include <memory>
#include <string_view>

namespace cs::game {
class ClientDisplayWorld;
using ClientDisplayWorldRef=std::shared_ptr<const ClientDisplayWorld>;
enum class DisplayRoot : std::uint8_t { Game,Room,Descriptor };
struct ClientDisplayCopy final {DisplayRoot root;std::uint32_t offset=0;const std::byte *data=nullptr;std::uint32_t size=0;};
// SDK桥仅借用不可变目标的连续值；不把C++20 span或state类型带入官方C++17头。
template<class T> struct ClientDisplayArray final {
  const T *data=nullptr;std::size_t count=0;
  const T *begin() const noexcept{return data;}
  const T *end() const noexcept{return count?data+count:data;}
  std::size_t size() const noexcept{return count;}
};
struct ClientDisplayScopeValue final {std::uint64_t session=0,run=0,floor=0,dimension=0,room=0,scene=0;};
// 数字在构建期由run.json的protocol_constants逐项对照；语义按RE-1优先级选取。
using ClientDisplayBranch=state::DisplayBranch;
struct ClientDisplayMetadata final {
  ClientDisplayScopeValue scope;
  std::uint32_t frame=0,time_base=0,spawn_seed=0;
  int grid=0,dimension=0;
  bool paused=false;ClientDisplayBranch branch=ClientDisplayBranch::Normal;float sfx=1;
  std::uint64_t room_use=0,room_shape=0;
  int transition_mode=0,transition_animation=0,transition_direction=0;
  int transition_boss=0;
  int entry=0,width=0,height=0;
  unsigned challenge=0;
  std::int32_t map_focus[2]{-1,-1};
};
struct ClientDisplayInjections final {
  std::array<std::int32_t,15> pill_effects{};
  std::array<std::uint8_t,15> pill_identified{};
  std::array<float,2> water{};
  float devil=0,angel=0,planetarium=0;
  bool deal_qualified=false,progress_blocked=false;
  // 从同批Host状态提取的原生构建条件与短时FX相位。
  int backdrop_wall_style=0,camera_entry_door=-1,camera_previous_entry=0,poop_fx_countdown=0;
  bool backdrop_tutorial=false,camera_return_focus=false;
  std::uint8_t room_graphics_flags=0;
  std::array<float,2> camera_return_position{};
  std::array<float,11> poop_fx_color{};
};
struct ClientDisplayMapValue final {
  std::uint16_t slot=UINT16_MAX,mask=0;bool present=false,spawn_seed_present=false;
  std::uint8_t frames[16]{};
  int grid=0,dimension=0;
  std::uint32_t display_flags=0,spawn_seed=0;
  std::uint8_t display_bits=0;bool visited=false;
};
ClientDisplayMetadata ClientDisplayMeta(const ClientDisplayWorldRef &) noexcept;
ClientDisplayMemoryRef ClientDisplayMemory(const ClientDisplayWorldRef &) noexcept;
ClientDisplayInjections ClientDisplayInputs(const ClientDisplayWorldRef &) noexcept;
ClientDisplayArray<ClientDisplayCopy> ClientDisplayCopies(const ClientDisplayWorldRef &) noexcept;
unsigned ClientDisplayMapCount(const ClientDisplayWorldRef &) noexcept;
ClientDisplayMapValue ClientDisplayMapAt(const ClientDisplayWorldRef &,unsigned);
bool ClientDisplaySameMap(const ClientDisplayWorldRef &,const ClientDisplayWorldRef &) noexcept;

struct ClientDisplayNative;
struct ClientDisplayMusicRequest;
struct ClientDisplayStep final {
  bool done=false,written=false;
  const char *reason="display_waiting";
  bool rebuilt=false;
};
ClientDisplayNative *ClientDisplayCreate(const ClientDisplayWorldRef &) noexcept;
void ClientDisplayDestroy(ClientDisplayNative *) noexcept;
// 只读判断本窗口是否会经过显示构造或捕获，用于 World 未变字段的略过资格。
ClientDisplayStep ClientDisplayBeforeInstall(ClientDisplayNative *,const ClientDisplayWorldRef &,const ClientNativeRoots &) noexcept;
ClientDisplayStep ClientDisplayInstall(ClientDisplayNative *,const ClientDisplayWorldRef &,const ClientNativeRoots &) noexcept;
// 冷加入只构造本机展示资源，保留常驻原生 Room 与当前 Lua／图形代次。
ClientDisplayStep ClientDisplayFinishInstall(ClientDisplayNative *,const ClientDisplayWorldRef &,const ClientNativeRoots &) noexcept;
void ClientDisplayPublish(ClientDisplayNative *) noexcept;
bool ClientDisplayReleaseRoots(ClientDisplayNative *) noexcept;
void ClientDisplayAbortPending(ClientDisplayNative *) noexcept;
bool ClientDisplayPreserveRoom(ClientDisplayNative *,const ClientNativeRoots &) noexcept;
ClientNativeRoots ClientDisplayRoots(const ClientDisplayNative *) noexcept;
bool ClientDisplayRetire(ClientDisplayNative *,bool preserve_target=false,bool preserve_audio=false) noexcept;
bool ClientDisplayHasWorld(const ClientDisplayNative *) noexcept;
void ClientDisplaySuspend(ClientDisplayNative *) noexcept;
// 仅在显示资格失败时返回第一项真实失配，保留原生期望值和现场值。
struct ClientDisplayIssue {
  const char *reason=nullptr;
  std::uintptr_t expected=0,actual=0;
  std::uint32_t resource=UINT32_MAX;
  std::uint32_t field=UINT32_MAX;
};
bool ClientDisplayCurrent(const ClientDisplayNative *,const ClientNativeRoots &,ClientDisplayIssue *issue=nullptr) noexcept;
void ClientDisplayReportIssue(const ClientDisplayNative *,const ClientDisplayIssue &,const char *phase,
    std::uint64_t route,std::uint64_t scene,std::uint32_t revision) noexcept;
// SDK 单元经 C++17 兼容的纯值入口记录，具体聚合在 C++20 账户实现。
void ClientDisplayRecordMemoryIssue(const ClientDisplayMemoryRef &,const ClientDisplayIssue &,const char *phase,
    std::uint64_t route,std::uint64_t scene,std::uint32_t revision) noexcept;
bool ClientDisplayGraphicsChanged(const ClientDisplayNative *) noexcept;
bool ClientDisplayPaused(const ClientDisplayNative *,bool &) noexcept;
bool ClientDisplayAdvance(ClientDisplayNative *,const ClientNativeRoots &,bool local_blocked) noexcept;
bool ClientDisplayRenderBegin(ClientDisplayNative *,const ClientNativeRoots &) noexcept;
void ClientDisplayRenderEnd(ClientDisplayNative *,const ClientNativeRoots &) noexcept;
bool ClientDisplayRouteRender(ClientDisplayNative *,bool begin) noexcept;
// 原生播放端只接收后台已校验的值与准备层借出的本地资源。
struct ClientDisplaySound final {
  std::uint64_t instance=0,generation=0;
  int id=0;
  float volume=0,pitch=1,pan=0;
  void *asset=nullptr;
  std::string_view path;
};
struct ClientDisplayMusicRequest final {
  std::uint64_t revision=0,generation=0,jingle_generation=0;
  std::int32_t id=0,queued=0,jingle=0;
  float parameter=0,fade=0,pitch=1,target_pitch=1;
  std::uint8_t mode=0,layers=0;
  bool paused=false,enabled=true,jingle_active=false;
};
struct ClientDisplayEvent final {
  unsigned kind=0;
  ClientDisplaySound sound;
  void *player=nullptr,*configuration=nullptr;
  std::string_view title,subtitle,category;
  std::array<std::string_view,32> lines{};
  unsigned line_count=0;
  int index=0,dimension=0;
  std::uint64_t final_room=0;
  bool completed=false;
};
// 基线中的Boss演出当前姿态，主Sprite用-1，extra map用原生层键。
struct ClientDisplayBoss final {
  struct Pose {int key=-1;float frame=0;bool playing=false;};
  std::array<Pose,33> poses{};
  std::uint64_t generation=0;
  unsigned ids[2]{},count=0;
  int index=0,dimension=0;
};
ClientDisplayStep ClientDisplayRestoreBoss(ClientDisplayNative *,const ClientDisplayBoss &) noexcept;
bool ClientDisplaySoundReady(const ClientDisplaySound &) noexcept;
ClientDisplayStep ClientDisplayAudio(ClientDisplayNative *,const ClientDisplayMusicRequest &,
    ClientDisplayArray<ClientDisplaySound>) noexcept;
ClientDisplayStep ClientDisplayEventReplay(ClientDisplayNative *,const ClientDisplayEvent &) noexcept;
bool ClientDisplayCheckPublication(const ClientDisplayNative *) noexcept;
bool ClientDisplayAudioHeld(const ClientDisplayNative *) noexcept;
} // namespace cs::game
