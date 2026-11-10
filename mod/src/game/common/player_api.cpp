#include "IsaacRepentance.h"
#include "binding/offsets.h"
#include "game/common/bridge.h"
#include "runtime.h"

namespace cs::game {
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
} // namespace cs::game
