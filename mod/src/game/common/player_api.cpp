#include "IsaacRepentance.h"
#include "binding/offsets.h"
#include "game/common/bridge.h"
#include "game/authority/lobby.h"
#include "game/common/context.h"
#include "game/client/display/client_display_bridge.h"
#include "runtime.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace cs::game {
// 共享现有生命周期判定。此 SDK 编译单元保持 C++17。
bool RunActive() noexcept;

namespace {
// 相机的临时名册保持原有身体顺序；有效的本端幽灵也可作为跟随目标。
struct CameraPlayers final {
  std::array<Entity_Player *,64> players{};
  Vector center{0,0};
  unsigned count=0;
};
bool CurrentCamera(const void *camera) noexcept {
  return IsMain()&&TopOwner()==Owner::Manager&&ContextHealthy()&&!InBlockedContext()&&
    g_Manager&&g_Game&&g_Game->_room&&camera&&camera==g_Game->_room->_Camera;
}
CameraPlayers FindLocalCameraPlayers(bool client) noexcept {
  CameraPlayers result;
  const auto &roster=g_Game->_playerManager._playerList;
  unsigned local=0;
  if(client&&!ClientLocalOrdinal(local))return result;
  for(auto *player:roster){
    if(!player||!player->_exists||(player->_dead&&!player->_isCoopGhost))continue;
    unsigned owner=0;
    const bool owned=client?ClientBinaryPlayerOrdinal(player,owner,true)&&owner==local:
      HostOwnsCameraPlayer(g_Game->_room,player);
    if(!owned||!std::isfinite(player->_pos.x)||!std::isfinite(player->_pos.y))continue;
    if(result.count==result.players.size())return {};
    result.players[result.count++]=player;
    result.center.x+=player->_pos.x;
    result.center.y+=player->_pos.y;
  }
  if(result.count){result.center.x/=float(result.count);result.center.y/=float(result.count);}
  return result;
}
// 阻止相机原链内的嵌套调用重复借用同一份原生名册。
thread_local bool local_camera_active=false;

// J273 83C620、83C860、83C9B0 共用的显示槽，不含精灵、字体或游戏属性对象。
struct FoundHudRow final {
  float value,delta,previous,threshold;
  int countdown;
};
static_assert(sizeof(FoundHudRow)==0x14);
constexpr unsigned kFoundHudFirstSlot=0x114,kFoundHudSlotSize=0xcc,kFoundHudRowCount=10;
static_assert(sizeof(void *)+kFoundHudRowCount*sizeof(FoundHudRow)==kFoundHudSlotSize);

std::byte *FoundHudSlot(void *hud,unsigned slot) noexcept {
  return static_cast<std::byte *>(hud)+kFoundHudFirstSlot+slot*kFoundHudSlotSize;
}
void *FoundHudPlayer(const std::byte *slot) noexcept {
  void *player=nullptr;
  std::memcpy(&player,slot,sizeof(player));
  return player;
}
void SetFoundHudPlayer(std::byte *slot,void *player) noexcept {
  std::memcpy(slot,&player,sizeof(player));
}
bool CurrentFoundHud(const void *hud) noexcept {
  return IsMain()&&g_Game&&hud==&g_Game->_hud._statHUD;
}
bool LocalFoundHud(const void *hud) noexcept {
  return CurrentFoundHud(hud)&&(ClientBusy()?ClientNativeDisplayCallbacks():HostBusy());
}
}

LocalCameraScope::LocalCameraScope(void *camera) noexcept {
  if(local_camera_active||!CurrentCamera(camera))return;
  const bool client=ClientBusy();
  if(client?!ClientNativeDisplayCallbacks():!HostBusy())return;
  auto &roster=g_Game->_playerManager._playerList;
  if(roster.size()>players_.size())return;
  const auto local=FindLocalCameraPlayers(client);
  if(!local.count)return;
  roster_=&roster;count_=unsigned(roster.size());
  std::copy(roster.begin(),roster.end(),players_.begin());
  // Ultrasmooth 还直接读名单首个身体、身体数和幽灵。缩小列表保留原容量，退栈时原地还原。
  std::copy_n(local.players.begin(),local.count,roster.begin());
  roster.resize(local.count);
  // Drag、Ultrasmooth 和其敌人偏移辅助都走 0x995B30 的缓存分支。
  // 在此供给本端中心，避免沿用游戏模拟阶段的全队缓存或从附加名单混入其他参与者。
  auto &frame=*reinterpret_cast<std::uint32_t *>(Base+offset::PlayerCenterFrame);
  auto *center=reinterpret_cast<void *>(Base+offset::PlayerCenterPosition);
  frame_=frame;std::memcpy(center_.data(),center,sizeof(center_));
  const std::array<float,2> position{local.center.x,local.center.y};
  std::memcpy(center,position.data(),sizeof(position));
  frame=g_Manager->_framecount;
  local_camera_active=true;
}
LocalCameraScope::~LocalCameraScope() noexcept {
  if(!roster_)return;
  auto &roster=*static_cast<vector_EntityPlayerPtr *>(roster_);
  roster.resize(count_);
  for(unsigned i=0;i<count_;++i)roster[i]=static_cast<Entity_Player *>(players_[i]);
  std::memcpy(reinterpret_cast<void *>(Base+offset::PlayerCenterPosition),center_.data(),sizeof(center_));
  *reinterpret_cast<std::uint32_t *>(Base+offset::PlayerCenterFrame)=frame_;
  local_camera_active=false;
}
bool ClientSnapLocalCamera(void *camera) {
  if(!ClientBusy()||!CurrentCamera(camera))return false;
  const auto local=FindLocalCameraPlayers(true);
  if(!local.count)return false;
  // J273 0x91EC20 仅重置相机历史与滚屏值，沿用原生视口和房间边界钳制。
  static_cast<Camera *>(camera)->SnapToPosition(local.center);
  return true;
}

bool FoundHudSlotAllowed(const void *hud,const void *player) noexcept {
  if(!LocalFoundHud(hud))return true;
  const auto &roster=g_Game->_playerManager._playerList;
  const auto *p=static_cast<const Entity_Player *>(player);
  if(!p||std::find(roster.begin(),roster.end(),p)==roster.end()||
     !p->_exists||p->_variant||p->_parent._ref||p->_playerType==40||PlayerIsHologram(const_cast<Entity_Player *>(p)))return false;
  if(!ClientBusy())return HostOwnsFoundHudPlayer(p);
  unsigned local=0,owner=0;
  return ClientLocalOrdinal(local)&&ClientBinaryPlayerOrdinal(p,owner)&&local==owner;
}
FoundHudScope::FoundHudScope(void *hud,bool rendering) noexcept:rendering_(rendering) {
  if(!LocalFoundHud(hud))return;
  hud_=hud;
  for(unsigned i=0;i<previous_.size();++i){
    auto *slot=FoundHudSlot(hud_,i);
    auto *player=FoundHudPlayer(slot);
    if(player&&!FoundHudSlotAllowed(hud_,player)){
      SetFoundHudPlayer(slot,nullptr);
      if(!rendering_)player=nullptr;
    }else if(rendering_)player=nullptr;
    previous_[i]=player;
  }
}
FoundHudScope::~FoundHudScope() noexcept {
  if(!hud_||!rendering_||!CurrentFoundHud(hud_))return;
  for(unsigned i=0;i<previous_.size();++i){
    auto *slot=FoundHudSlot(hud_,i);
    if(previous_[i]&&!FoundHudPlayer(slot))SetFoundHudPlayer(slot,previous_[i]);
  }
}
void FoundHudScope::Returned() noexcept {
  if(!hud_||rendering_||!CurrentFoundHud(hud_))return;
  for(unsigned i=0;i<previous_.size();++i){
    auto *slot=FoundHudSlot(hud_,i);
    auto *player=FoundHudPlayer(slot);
    if(!player||player==previous_[i])continue;
    // 新槽首次生产保留当前值与阈值，清掉旧角色继承来的变化提示。
    std::array<FoundHudRow,kFoundHudRowCount> rows;
    std::memcpy(rows.data(),slot+sizeof(void *),sizeof(rows));
    for(auto &row:rows){row.previous=row.value;row.delta=0;row.countdown=0;}
    std::memcpy(slot+sizeof(void *),rows.data(),sizeof(rows));
  }
}
void ClearClientFoundHudSlots(void *hud) noexcept {
  if(!CurrentFoundHud(hud)||!ClientBusy())return;
  for(unsigned i=0;i<2;++i)SetFoundHudPlayer(FoundHudSlot(hud,i),nullptr);
}

// 只读原生选项“插值”（OptionsConfig._enableInterpolation，J273 Manager+0x2A1FB）。
// 开启时Host每个Manager帧都有Authority tick（偶帧StandardRoom、奇帧OddPlayer），约60Hz；
// 关闭时只有偶帧StandardRoom，约30Hz且一个tick内玩家走两个半步。0=关，1=开，2=不可读。
unsigned NativeInterpolationState() noexcept {
  if(!IsMain()||!g_Manager)return 2;
  return g_Manager->_optionsConfig._enableInterpolation?1u:0u;
}
// 对应公开 EntityPlayer:IsHologram()；不推进动画、不修改角色或控制器。
bool PlayerIsHologram(void *player) noexcept {
  return IsMain() && player &&
         static_cast<Entity_Player *>(player)->IsHologram();
}
// J273 RVA5EC130先处理窗口/控制器事件，再进入已登记的5EDCC0输入owner。
// Manager被Client短路后必须保留这一步，否则WM_KEYDOWN不进入原生键盘缓存。
// 不调用Manager、Game、Player或Join更新，不激活窗口、不伪造原生按键缓存。
void ClientPollInput() {
  auto fn=reinterpret_cast<int(__thiscall *)(void *)>(Base+offset::ClientInputPump);
  fn(reinterpret_cast<void *>(Base+offset::InputManager));
}
// 只读SDK公开的Console状态，保留ImGui的state=2屏蔽；不补调会推进消息/动画的ConsoleGate。
bool ClientUiBlocked() noexcept {
  return !g_Game || g_Game->GetConsole()->_state != 0;
}
// 只接受已经完成正常启动的本地菜单，拒绝开场、载入与无菜单根的过渡。
bool ClientMenuAvailable() noexcept {
  if(!g_Game||!g_MenuManager)return false;
  const int menu=g_MenuManager->_selectedMenuID;
  return menu==1||menu==2||menu==3||menu==5;
}

// 标题页和存档选择页也可以监听。只在请求创建游戏时读取选中的存档。
bool LobbyCanListen() noexcept {
  return IsMain() && g_Manager && g_Game && !g_Manager->_networkPlay &&
         !g_Manager->_starting && !RunActive();
}

bool LobbyMenuReady() noexcept {
  return LobbyCanListen() && ClientMenuAvailable() &&
         g_Manager->_currentSaveSlot >= 1 && g_Manager->_currentSaveSlot <= 3;
}

bool RequestLobbyNativeStart(unsigned character) noexcept {
  if (TopOwner() != Owner::Manager || !LobbyMenuReady() || !core::LobbyCharacter(character)) return false;
  try {
    // 与 REPENTOGON 的 StartNewGame 使用同一 Seeds 所有权转移。自定义局关闭成就。
    Seeds seeds;
    seeds.constructor();
    seeds.set_start_seed(0);
    seeds._isCustomRun = true;
    g_Manager->StartNewGame(int(character), 0, seeds, 0);
    return g_Manager->_starting || RunActive();
  } catch (...) {
    return false;
  }
}
} // namespace cs::game
