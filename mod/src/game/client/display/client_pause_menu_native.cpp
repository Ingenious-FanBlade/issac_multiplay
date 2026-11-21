#include "IsaacRepentance.h"
#include "binding/offsets.h"
#include "game/client/display/client_pause_menu.h"
#include "game/common/bridge.h"
#include "game/common/context.h"
#include "runtime.h"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

// 客机本地暂停菜单。J273 静态依据（IDA，未运行测试）：
//   PauseScreen 输入 98CCC0（Game::ProcessInput 6F6CFC 调用）、更新 98D4E0（Game::Update 暂停分支调用）、
//   打开 98ED80、绘制 98DDA0（Game::Render 6F8304，state!=0 时绘制）。
//   客机全部 Player 的 ControllerIndex(+0x160C) 保持 -1，原生输入按“任一本机设备”读取动作。
// 原生输入里对客机有害的分支在调用前拦截，见 client_pause_menu.h。
namespace cs::game {
namespace {
// ---- J273 原生入口（RVA，加 Base 得到地址） ----
constexpr std::uintptr_t kPauseInputRva=0x58CCC0;   // PauseScreen 输入（thiscall，this=Game+0x2363C）
constexpr std::uintptr_t kPauseUpdateRva=0x58D4E0;  // PauseScreen::Update（30Hz）
constexpr std::uintptr_t kPauseShowRva=0x58ED80;    // PauseScreen::Show：state=1、选中“继续”、播放打开音效、计算属性条
constexpr std::uintptr_t kActionTriggeredRva=0x5EE200; // InputManager::IsActionTriggered(action,controller,unk)

// ---- PauseScreen 字段（相对 Game+0x2363C） ----
constexpr std::size_t kPauseState=0x0;      // 0 关闭，1 主菜单，2 选项，3 每日挑战退出确认，4/5 原版 Online
constexpr std::size_t kPauseSelection=0x8;  // 主菜单选中项：0 选项、1 继续、2 退出
constexpr std::size_t kPauseStats=0xC;      // int[6]：速度、射速、伤害、射程、弹速、运气的属性条格数（0..7）
constexpr std::size_t kPauseCompletion=0x840; // 完成标记部件首字段：角色类型（Player+0x13BC）
constexpr std::size_t kPausePlayer=0x95C;   // 显示的 PlayerManager 序号（GetPlayer 参数）
constexpr std::size_t kPauseOptions=0x5B0;  // 内嵌选项菜单对象
// ---- 选项菜单字段（相对 PauseScreen+0x5B0，输入 989970） ----
constexpr std::size_t kOptionsSelection=0x8;
constexpr std::size_t kOptionsReload=0x258;  // 语言变更后的整体重载标志（会重置 Lua 与资源），局内不应出现
constexpr std::size_t kOptionsItemsBegin=0x27C,kOptionsItemsEnd=0x280; // 当前列表的选项类型 vector<int>
constexpr int kOptionControllerReselect=1;   // 局内列表首项：确认时 79E1B0 重绑 Player 设备
// ---- 其他原生字段 ----
constexpr std::size_t kGameMenuBackBlock=0x246A4; // 原生打开条件：MENUBACK 仅在该值为 0 时打开（98CCC0 state0 分支）
constexpr std::size_t kGameHudRefresh=0x22E94,kGameHudRefreshTarget=0x22E9C; // 98D4E0 写入的 HUD 刷新请求
constexpr std::size_t kManagerPopupActive=0x4CD54; // 共享弹窗活动时原生输入直接返回
// ---- Player 字段（暂停页属性条） ----
constexpr std::size_t kPlayerType=0x13BC,kPlayerMaxFireDelay=0x145C,kPlayerShotSpeed=0x1460,kPlayerDamage=0x146C,
                      kPlayerRange=0x147C,kPlayerSpeed=0x155C,kPlayerLuck=0x1560;
// ---- ButtonAction ----
constexpr int kActionPause=12,kActionMenuConfirm=14,kActionMenuBack=15,kActionMenuUp=22,kActionMenuDown=23;

thread_local unsigned sound_depth=0; // >0 时本机菜单音效放行
thread_local unsigned query_depth=0; // >0 时设备虚表包装放行本机按键查询

// 客机的设备查询包装（pointer_patch）对玩法一律返回中性值；菜单读取本机按键时须在此作用域内。
struct QueryScope final {QueryScope() noexcept{++query_depth;}~QueryScope(){--query_depth;}};

// 菜单原生调用期间允许本机 SFX（打开 17、关闭 18、选项调节 237 等）。
struct SoundScope final {SoundScope() noexcept{++sound_depth;}~SoundScope(){--sound_depth;}};

template<class T> T Read(const void *base,std::size_t offset) noexcept {
  T value{};std::memcpy(&value,static_cast<const std::byte *>(base)+offset,sizeof(T));return value;
}
template<class T> void Write(void *base,std::size_t offset,T value) noexcept {
  std::memcpy(static_cast<std::byte *>(base)+offset,&value,sizeof(T));
}
// PauseScreen 对象地址；Game 尚未建立时为空。
std::byte *PauseRoot() noexcept {
  return g_Game?reinterpret_cast<std::byte *>(g_Game)+offset::PauseScreen:nullptr;
}
// 任一本机设备本帧是否触发动作（与客机 Player.ControllerIndex=-1 时原生读取的范围一致）。
bool Triggered(int action) noexcept {
  QueryScope query;
  using Fn=bool(__thiscall *)(void *,int,int,int);
  return reinterpret_cast<Fn>(Base+kActionTriggeredRva)(reinterpret_cast<void *>(Base+offset::InputManager),action,-1,0);
}
void NativeCall(std::uintptr_t rva,std::byte *pause) {
  reinterpret_cast<void(__thiscall *)(void *)>(Base+rva)(pause);
}
// 原生暂停页属性条：98EEF0—98F04C 的同一换算（截断到 0..7 格）。
int StatBar(float value) noexcept {return int(std::fmin(std::fmax(value,0.0f),7.0f));}
// Show 固定显示 PlayerManager 0 号（客机上是 Host 的角色）；改为本机控制组的首个身体，并按原式重算属性条。
// 找不到本机身体时保持原生默认，不猜测。
void ShowLocalPlayer(std::byte *pause,unsigned ordinal) noexcept {
  const auto &players=g_Game->_playerManager._playerList;
  int index=0;
  for(auto *player:players){
    unsigned owner=0;
    if(player&&ClientBinaryPlayerOrdinal(player,owner,true)&&owner==ordinal){
      const void *p=player;
      Write<int>(pause,kPausePlayer,index);
      Write<int>(pause,kPauseCompletion,Read<int>(p,kPlayerType));
      const float tears=float(std::pow(double(30.0f/(Read<float>(p,kPlayerMaxFireDelay)+1.0f)),double(0.75f)));
      const float damage=float(std::pow(double(Read<float>(p,kPlayerDamage)),double(0.56f)));
      const int bars[6]={
        StatBar(Read<float>(p,kPlayerSpeed)*4.5f-2.0f),
        StatBar(tears*2.1203909f-2.0f),
        StatBar(damage*2.231179f-2.0f),
        StatBar((Read<float>(p,kPlayerRange)-230.0f)/60.0f+2.0f),
        StatBar(Read<float>(p,kPlayerShotSpeed)*5.0f-2.5f),
        StatBar(Read<float>(p,kPlayerLuck)+2.5f)};
      std::memcpy(pause+kPauseStats,bars,sizeof(bars));
      return;
    }
    ++index;
  }
}
// 选项菜单当前选中项的类型；越界返回 -1。
int OptionItem(const std::byte *options) noexcept {
  const auto begin=Read<std::uintptr_t>(options,kOptionsItemsBegin),end=Read<std::uintptr_t>(options,kOptionsItemsEnd);
  const int selection=Read<int>(options,kOptionsSelection);
  if(!begin||end<begin||selection<0||std::uintptr_t(selection)>=(end-begin)/sizeof(int))return -1;
  return *reinterpret_cast<const int *>(begin+std::uintptr_t(selection)*sizeof(int));
}
} // namespace

bool ClientPauseMenuSoundAllowed() noexcept {return IsMain()&&sound_depth!=0;}
bool ClientPauseMenuQueryAllowed() noexcept {return IsMain()&&query_depth!=0;}

// 拦截 Hook 状态：0 已安装，1 表中没有，2 未登记，3 无原函数跳板。
unsigned PauseHookState() noexcept {
  std::size_t count=0;auto hooks=Hooks(count);
  for(std::size_t i=0;i<count;++i)if(!std::strcmp(hooks[i]->id,"client_pause_pickup_cycle"))
    return !hooks[i]->registered?2u:(hooks[i]->super&&*hooks[i]->super)?0u:3u;
  return 1u;
}
bool ClientPauseMenuAvailable() noexcept {return PauseHookState()==0;}
void ClientPauseDiagnose(bool focused,bool console,bool native,bool branch_known,unsigned branch) noexcept {
  if(!IsMain()||!g_Game||!g_Manager)return;
  auto *pause=PauseRoot();
  // 输入管理器的设备列表（InputManager+4／+8，98CCC0 与 9EE200 同一来源）。
  const auto *input=reinterpret_cast<const std::byte *>(Base+offset::InputManager);
  const auto devices=(Read<std::uintptr_t>(input,8)-Read<std::uintptr_t>(input,4))/sizeof(void *);
  Trace("client_pause","event=esc_down busy=%u focused=%u console=%u native=%u branch_known=%u branch=%u state=%d "
        "pause=%u back=%u back_dev0=%u confirm=%u back_block=%d popup=%d hook=%u players=%u devices=%u",
        unsigned(ClientBusy()),unsigned(focused),unsigned(console),unsigned(native),unsigned(branch_known),branch,
        Read<int>(pause,kPauseState),unsigned(Triggered(kActionPause)),unsigned(Triggered(kActionMenuBack)),
        [&]{QueryScope query;return unsigned(reinterpret_cast<bool(__thiscall *)(void *,int,int,int)>(Base+kActionTriggeredRva)(
          reinterpret_cast<void *>(Base+offset::InputManager),kActionMenuBack,0,0));}(),
        unsigned(Triggered(kActionMenuConfirm)),Read<int>(g_Game,kGameMenuBackBlock),Read<int>(g_Manager,kManagerPopupActive),
        PauseHookState(),unsigned(g_Game->_playerManager._playerList.size()),unsigned(devices));
}
bool ClientPauseKeyPressed() noexcept {
  if(!IsMain()||!g_Game)return false;
  return Triggered(kActionPause)||(Triggered(kActionMenuBack)&&!Read<int>(g_Game,kGameMenuBackBlock));
}

bool ClientPauseMenuOpen() noexcept {
  auto *pause=IsMain()?PauseRoot():nullptr;
  return pause&&Read<int>(pause,kPauseState)!=0;
}

void ClientPauseMenuReset() noexcept {
  auto *pause=IsMain()?PauseRoot():nullptr;
  if(pause&&Read<int>(pause,kPauseState)!=0)Write<int>(pause,kPauseState,0);
}

ClientPauseAction ClientPauseMenuInput(unsigned local_ordinal,bool may_open,unsigned branch) noexcept {
  if(!IsMain()||!g_Game||!g_Manager)return ClientPauseAction::None;
  if(!ClientBusy()){
    if(ClientPauseKeyPressed())Trace("client_pause","event=open_refused reason=not_busy");
    return ClientPauseAction::None;
  }
  auto *pause=PauseRoot();
  const int state=Read<int>(pause,kPauseState);
  if(state==0){
    // 原生 98CCC0 state0：ACTION_PAUSE 打开；MENUBACK 另要求 Game+0x246A4 为 0。
    // 原生经 Manager 包装 92F6D0（会分派 Lua 输入回调），客机直接读 InputManager，不分派玩法回调。
    // 先判按键，再逐项检查并在拒绝时记录原因；拦截 Hook 未安装时不打开（更新会改同步拾取物）。
    if(!ClientPauseKeyPressed())return ClientPauseAction::None;
    if(!may_open){Trace("client_pause","event=open_refused reason=branch branch=%u",branch);return ClientPauseAction::None;}
    if(g_Game->_playerManager._playerList.empty()){Trace("client_pause","event=open_refused reason=no_players");return ClientPauseAction::None;}
    if(const auto hook=PauseHookState()){Trace("client_pause","event=open_refused reason=hook state=%u",hook);return ClientPauseAction::None;}
    {SoundScope sound;NativeCall(kPauseShowRva,pause);}
    ShowLocalPlayer(pause,local_ordinal);
    Trace("client_pause","event=opened state=%d player=%d",Read<int>(pause,kPauseState),Read<int>(pause,kPausePlayer));
    return ClientPauseAction::Opened;
  }
  // 客机只会进入主菜单与选项；其他状态（每日挑战确认、原版 Online）出现即立即关闭，不调用其处理分支。
  if(state!=1&&state!=2){ClientPauseMenuReset();return ClientPauseAction::None;}
  // 共享弹窗活动时原生输入整体返回，这里同样不处理。
  if(Read<int>(g_Manager,kManagerPopupActive))return ClientPauseAction::None;
  if(state==1){
    // 原生同帧顺序：PAUSE／MENUBACK 关闭优先；否则先上下移动选中项，再按确认处理。
    // 确认落在“退出”时，原生会写 Game fade、保存并结束本局（客机不得执行），这里改为离开会话。
    if(!Triggered(kActionPause)&&!Triggered(kActionMenuBack)&&Triggered(kActionMenuConfirm)){
      int selection=Read<int>(pause,kPauseSelection);
      if(Triggered(kActionMenuUp)){if(--selection<0)selection=2;}
      else if(Triggered(kActionMenuDown)){if(++selection>2)selection=0;}
      if(selection==2){ClientPauseMenuReset();return ClientPauseAction::Exit;}
    }
  }else{
    auto *options=pause+kPauseOptions;
    // 语言重载会重置 Lua 与资源，局内列表没有语言项；出现即视为异常，直接关闭菜单。
    if(Read<std::uint8_t>(options,kOptionsReload)){ClientPauseMenuReset();return ClientPauseAction::None;}
    // “重选控制器”会把同步 Player 的设备重绑到本机设备，与 C/S 输入归属冲突：吞掉这一次确认。
    if(Triggered(kActionMenuConfirm)&&OptionItem(options)==kOptionControllerReselect)return ClientPauseAction::None;
  }
  {SoundScope sound;QueryScope query;NativeCall(kPauseInputRva,pause);}
  const int after=Read<int>(pause,kPauseState);
  if(after!=0&&after!=1&&after!=2)ClientPauseMenuReset();
  return ClientPauseAction::None;
}

bool ClientPauseMenuUpdate() noexcept {
  auto *pause=IsMain()&&ClientBusy()?PauseRoot():nullptr;
  if(!pause||Read<int>(pause,kPauseState)==0)return false;
  // 原生更新在暂停期间每拍请求 HUD 刷新；客机 Host 世界未暂停、HUD 仍逐拍更新，
  // 这里保留原请求值，避免每拍整段刷新，关闭时由调用方补一次刷新请求。
  auto *game=reinterpret_cast<std::byte *>(g_Game);
  const int refresh=Read<int>(game,kGameHudRefresh),refresh_target=Read<int>(game,kGameHudRefreshTarget);
  {SoundScope sound;QueryScope query;NativeCall(kPauseUpdateRva,pause);}
  Write<int>(game,kGameHudRefresh,refresh);Write<int>(game,kGameHudRefreshTarget,refresh_target);
  return Read<int>(pause,kPauseState)==0;
}
} // namespace cs::game
