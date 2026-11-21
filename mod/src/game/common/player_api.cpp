#include "IsaacRepentance.h"
#include "binding/offsets.h"
#include "game/common/bridge.h"
#include "game/common/display_settings.h"
#include "game/authority/lobby.h"
#include "game/common/context.h"
#include "game/client/display/client_display_bridge.h"
#include "runtime.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
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

// ===========================================================================
// 玩家 HUD／道具历史栏的本端规则（模仿原版 Online，J273 静态核对，未运行）
// ===========================================================================
namespace {
// HistoryHUD（HUD+0x5C64）：两个 0x14 槽，槽 +0 为 Player*；+0x28 为初始化位，
// HUD::PostUpdate 97AE90 见其为 0 时置 1 并调 RecomputeAll 8304B0(false)。
constexpr std::size_t kHistoryHud=0x5C64,kHistorySlotStride=0x14,kHistorySlotCount=2,kHistoryInitFlag=0x28;
// Game+0x25B8C：HUD 布局选择 97E4A0 与 97FFE0 在 Online 下 ≥2 时强制紧凑布局、不隐藏他人槽。
constexpr std::size_t kGameHudOnlineOverride=0x25B8C;
// PlayerHUD 槽：8 个，stride 0x6E0，+0 为 Player*（槽 4—7 为双子身体）。
constexpr std::size_t kPlayerHudStride=0x6E0;
constexpr unsigned kPlayerHudCount=8;
// 当前 Game 的 HUD 且本端显示作用域有效：Host 会话期间，或客机已安装世界的原生显示回调内。
bool LocalHudActive(const void *hud) noexcept {
  return IsMain()&&g_Game&&g_Manager&&hud==&g_Game->_hud&&(ClientBusy()?ClientNativeDisplayCallbacks():HostBusy());
}
// 身体是否属于本端控制组：Host 用 Authority 控制组 0（含多身体），客机用本机组序号。
bool LocalHudPlayer(const Entity_Player *p) noexcept {
  if(!p)return false;
  const auto &roster=g_Game->_playerManager._playerList;
  if(std::find(roster.begin(),roster.end(),p)==roster.end())return false;
  if(!ClientBusy())return HostOwnsFoundHudPlayer(p);
  unsigned local=0,owner=0;
  return ClientLocalOrdinal(local)&&ClientBinaryPlayerOrdinal(p,owner)&&local==owner;
}
// 与 830810 的其余原生条件一致：非合作婴儿（variant）、无 parent、非 type 40。
bool HistoryEligible(const Entity_Player &p) noexcept {
  return p._exists&&!p._variant&&!p._parent._ref&&p._playerType!=40;
}
Entity_Player *HistoryPlayer(const std::byte *history,std::size_t slot) noexcept {
  Entity_Player *player=nullptr;
  std::memcpy(&player,history+slot*kHistorySlotStride,sizeof(player));
  return player;
}
int OnlineHudMode() noexcept {return g_Manager->_optionsConfig._onlineHud;}
bool OnlineHudOverride() noexcept {
  std::int32_t value=0;
  std::memcpy(&value,reinterpret_cast<const std::byte *>(g_Game)+kGameHudOnlineOverride,sizeof(value));
  return value>=2;
}
}

bool HistoryHudSlotAllowed(const void *history,const void *player) noexcept {
  if(!g_Game||history!=reinterpret_cast<const std::byte *>(&g_Game->_hud)+kHistoryHud||!LocalHudActive(&g_Game->_hud))return true;
  return LocalHudPlayer(static_cast<const Entity_Player *>(player));
}

void RefreshLocalHistoryHud(void *hud) noexcept {
  if(!LocalHudActive(hud)||g_Manager->_optionsConfig._historyHudMode<=0)return;
  auto *history=static_cast<std::byte *>(hud)+kHistoryHud;
  if(history[kHistoryInitFlag]==std::byte{0})return; // 原生本帧已会重算。
  bool stale=false,free_slot=false;
  for(std::size_t i=0;i<kHistorySlotCount;++i){
    auto *player=HistoryPlayer(history,i);
    if(!player)free_slot=true;
    else if(!LocalHudPlayer(player))stale=true;
  }
  if(!stale&&free_slot){
    // 本端合格身体不在任何槽里时也交原生补登记（例如会话开始前槽已被其他身体占满）。
    for(auto *p:g_Game->_playerManager._playerList){
      if(!p||!HistoryEligible(*p)||!LocalHudPlayer(p))continue;
      if(HistoryPlayer(history,0)!=p&&HistoryPlayer(history,1)!=p){stale=true;break;}
    }
  }
  // 原生 RecomputeAll 先经 GetPlayerIdx（已按本端过滤）清掉不合格槽，再按名册补登记并置脏。
  if(stale)history[kHistoryInitFlag]=std::byte{0};
}

bool LocalPlayerHudLayout(const void *hud,const void *player,int &layout) noexcept {
  if(!LocalHudActive(hud))return false;
  // 照搬 97E4A0 的 Online 分支：雅各／以扫（type 19/20）用 1 或 3，其余用 0 或 2。
  const auto *p=static_cast<const Entity_Player *>(player);
  const bool twin_hud=p&&(p->_playerType==19||p->_playerType==20);
  const bool full=!OnlineHudOverride()&&OnlineHudMode()==3;
  layout=full?(twin_hud?1:0):(twin_hud?3:2);
  return true;
}

bool LocalPlayerHudHidden(const void *hud,int slot) noexcept {
  if(!LocalHudActive(hud)||slot<0||unsigned(slot)>=kPlayerHudCount)return false;
  // 97C300 的 Online 分支只在 OnlineHUD=3 时跳过非本机槽；这里按身体归属判断，
  // Host 端本端身体不一定位于槽 0。
  if(OnlineHudOverride()||OnlineHudMode()!=3)return false;
  const Entity_Player *player=nullptr;
  std::memcpy(&player,static_cast<const std::byte *>(hud)+unsigned(slot)*kPlayerHudStride,sizeof(player));
  return player&&!LocalHudPlayer(player);
}

// 客机连接质量行：位置照搬原版 Online 名牌 97F4E0 的本机分支——锚点来自 97E070，
// x−6（非槽 0 再 +8）、y−16，字体 Manager._font1_TeamMeatEx10，缩放 0.5，白色，左对齐。
// C/S 不创建 NetInputDevice，原版名牌不会绘制，这个位置在客机上是空的。
void RenderClientLinkQuality(const void *hud,int slot,int layout) noexcept {
  if(!LocalHudActive(hud)||!ClientBusy()||slot<0||unsigned(slot)>=kPlayerHudCount)return;
  const auto *base=static_cast<const std::byte *>(hud);
  const auto slot_player=[&](unsigned i){
    const Entity_Player *player=nullptr;
    std::memcpy(&player,base+i*kPlayerHudStride,sizeof(player));
    return player;
  };
  if(!LocalHudPlayer(slot_player(unsigned(slot))))return;
  // 本端多身体（雅各／以扫在槽 0／4 等）只在第一个本端槽画一次。
  for(unsigned i=0;i<unsigned(slot);++i)if(LocalHudPlayer(slot_player(i)))return;
  ClientLinkStats stats;
  if(!ClientLinkQuality(stats))return;
  auto &font=g_Manager->_font1_TeamMeatEx10;
  if(!font._isLoaded)return;
  Vector anchor{0,0};
  auto fn=reinterpret_cast<Vector *(__thiscall *)(const void *,Vector *,unsigned,int)>(Base+offset::PlayerHudAnchor);
  fn(hud,&anchor,unsigned(slot),layout);
  if(!std::isfinite(anchor.x)||!std::isfinite(anchor.y))return;
  // 窗口太短（连接或切房刚开始）时比例噪声大，先显示占位。
  constexpr unsigned kMinimumLossWindow=32;
  char rtt[24],loss[24],text[64];
  if(stats.rtt_valid)std::snprintf(rtt,sizeof(rtt),"RTT %ums",(std::min)(stats.rtt_ms,9999u));
  else std::snprintf(rtt,sizeof(rtt),"RTT --");
  if(stats.loss_valid&&stats.loss_window>=kMinimumLossWindow)
    std::snprintf(loss,sizeof(loss),"LOSS %u.%u%%",stats.loss_permille/10,stats.loss_permille%10);
  else std::snprintf(loss,sizeof(loss),"LOSS --");
  std::snprintf(text,sizeof(text),"%s  %s",rtt,loss);
  Vector position{anchor.x-6.0f+(slot?8.0f:0.0f),anchor.y-16.0f};
  KColor color(1.0f,1.0f,1.0f,1.0f);
  FontSettings settings;
  font.DrawString(text,position,Vector(0.5f,0.5f),&color,&settings);
}

// ===========================================================================
// 角色绘制：本端身体高亮描边、他人身体透明度（J273 静态核对，未运行）
// ===========================================================================
// 原版 Online 的做法（只在 NetplayManager 设备表非空时生效，C/S 下永远不触发）：
//   - 描边：783680（身体绘制，Entity_Player::Render 784A60 以 main=1 调用）对本机控制器的身体，
//     用玩家色压入全局覆盖色栈，按 67C8A0 的四个 1 像素偏移各画一次剪影，再画本体；
//   - 透明度：对非本机身体，各绘制点把精灵颜色 alpha 乘以 0.1+0.09×PlayerOpacity。
// 这里用本机设置复刻：描边颜色从 display_settings 取，透明度刻度与原版相同。
// 原版还会改他人泪弹颜色（Entity_Player 泪弹参数 7B67F0），那会写进 Host 的同步状态，这里不做。
namespace {
enum class SessionBody { Unknown, Local, Remote };
// 当前是否处于 C/S 的本端绘制作用域：Host 会话期间，或客机已安装世界的原生显示回调内。
bool PlayerDisplayActive() noexcept {
  return IsMain()&&g_Game&&g_Manager&&(ClientBusy()?ClientNativeDisplayCallbacks():HostBusy());
}
// 身体归属：Host 查控制组拓扑，客机查已安装的身体归属；都含多身体与非名册副身体。
SessionBody PlayerBodyOwner(const void *player) noexcept {
  unsigned owner=0;
  if(ClientBusy()){
    unsigned local=0;
    if(!ClientLocalOrdinal(local)||!ClientBinaryPlayerOrdinal(player,owner))return SessionBody::Unknown;
    return owner==local?SessionBody::Local:SessionBody::Remote;
  }
  if(!HostPlayerParticipant(player,owner))return SessionBody::Unknown;
  return owner==0?SessionBody::Local:SessionBody::Remote;
}
}

RemotePlayerFadeScope::RemotePlayerFadeScope(void *object) noexcept {
  if(!object||!PlayerDisplayActive())return;
  const auto level=CurrentDisplayPreferences().remote_opacity;
  if(level>=kRemoteOpacityMax)return;
  if(PlayerBodyOwner(object)!=SessionBody::Remote)return;
  const float factor=RemoteOpacityFactor(level);
  auto *player=static_cast<Entity_Player *>(object);
  // 与原版各绘制点乘 alpha 的对象一致：主精灵、身体、血喷、举物精灵，以及每件服装精灵。
  const auto add=[&](ANM2 &sprite){
    if(count_==kMaximumSprites)return;
    float *alpha=&sprite._color._tint[3];
    alpha_[count_]=alpha;saved_[count_]=*alpha;++count_;
    *alpha*=factor;
  };
  add(player->_sprite);
  add(player->_bodySprite);
  add(player->_bloodGushSprite);
  add(player->_heldSprite);
  for(auto &costume:player->_costumeSpriteDescs)add(costume._sprite);
}
// 原样恢复绘制前的值；绘制中原生代码自己做的“乘后恢复”不受影响。
RemotePlayerFadeScope::~RemotePlayerFadeScope() noexcept {
  for(unsigned i=count_;i-->0;)*alpha_[i]=saved_[i];
}

LocalPlayerOutline::LocalPlayerOutline(void *object,bool main) noexcept {
  if(!main||!object||!PlayerDisplayActive())return;
  OutlineRgb rgb;
  if(!OutlineColorRgb(CurrentDisplayPreferences().outline_color,rgb))return;
  if(PlayerBodyOwner(object)!=SessionBody::Local)return;
  // 与 783680 的 Online 条件相同：该判定为真时原函数自己压覆盖色，描边会被顶掉，原版也不画。
  auto ghost=reinterpret_cast<bool(__thiscall *)(void *)>(Base+offset::PlayerOverlayGhost);
  if(ghost(object))return;
  const auto *player=static_cast<const Entity_Player *>(object);
  // 纯色剪影：tint RGB 取 0、alpha 跟随身体精灵（身体半透明时描边同样淡）；colorize 为 0；offset 为描边色。
  // 布局即 ColorMod：_tint[4] @0、_colorize[4] @0x10、_offset[3] @0x20（共 0x2C）。
  color_[3]=player->_sprite._color._tint[3];
  color_[8]=rgb.r;color_[9]=rgb.g;color_[10]=rgb.b;
  if(!(color_[3]>0.0f))return;
  active_=true;
}
// 67C8A0 的四个偏移：(-1,0)、(0,-1)、(1,0)、(0,1)。
void LocalPlayerOutline::Shift(const void *position,unsigned i,void *out) const noexcept {
  static constexpr float dx[4]={-1,0,1,0},dy[4]={0,-1,0,1};
  const auto *p=static_cast<const Vector *>(position);
  auto *o=static_cast<Vector *>(out);
  o->x=p->x+dx[i&3];o->y=p->y+dy[i&3];
}
void LocalPlayerOutline::Push() const noexcept {
  auto push=reinterpret_cast<void(__thiscall *)(const void *)>(Base+offset::ColorOverridePush);
  push(color_);
}
// 照搬 783680/784A60 的弹栈写法：计数减一，减到 0 时环形起点归零。
void LocalPlayerOutline::Pop() const noexcept {
  auto &count=*reinterpret_cast<std::uint32_t *>(Base+offset::ColorOverrideCount);
  auto &start=*reinterpret_cast<std::uint32_t *>(Base+offset::ColorOverrideStart);
  if(!count)return;
  if(--count==0)start=0;
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

// 房主存档已解锁的大厅角色位图（第 n 位＝角色 n）。只在 Manager 主线程调用。
// 每个角色的解锁条件来自原生 EntityConfig_Player::_achievement（+0xE8）：
// -1 表示不需要成就，-2 及其他负值表示隐藏角色，非负值查 PersistentGameData::achievements（+0x38）。
// 尚未选定存档时 PGD 可能仍是旧槽内容，因此只开放不需要成就的角色。以撒始终可选。
std::uint64_t LobbyUnlockedCharacters() noexcept {
  std::uint64_t unlocked = 1;
  if (!IsMain() || !g_Manager) return unlocked;
  const bool save_loaded = g_Manager->_currentSaveSlot >= 1 && g_Manager->_currentSaveSlot <= 3;
  auto *pgd = g_Manager->GetPersistentGameData();
  try {
    for (unsigned id = 1; id <= 37; ++id) {
      if (!core::LobbyCharacter(id)) continue;
      auto *config = g_Manager->_entityConfig.GetPlayer(int(id));
      if (!config) continue;
      const int achievement = config->_achievement;
      const bool available = achievement == -1 ||
          (save_loaded && pgd && achievement >= 0 && achievement < 641 && pgd->achievements[achievement]);
      if (available) unlocked |= std::uint64_t(1) << id;
    }
  } catch (...) {
    return 1;
  }
  return unlocked;
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
