#include "IsaacRepentance.h"
#include "game/bridge.h"
#include "runtime.h"
#include <cstddef>
#include <windows.h>

namespace cs::game {
bool ReadMemory(const void *source,void *output,std::size_t size) noexcept;
namespace {
template <class T> bool Field(const void *base,std::size_t offset,T &value) noexcept {
  return base&&ReadMemory(static_cast<const unsigned char *>(base)+offset,&value,sizeof(value));
}
}
// J273 GLFW window 的 HDC/HGLRC 在 +0x234/+0x238；SDK 只公开 HWND。
// 必须同时确认 renderer window 的进程归属，不能学习 ImGui 多视口 current。
bool ClientRendererHandles(void *&dc,void *&context) noexcept {
  dc=context=nullptr;
  if(!IsMain()||!__ptr_g_KAGE_Graphics_Manager)return false;
  auto *window=__ptr_g_KAGE_Graphics_Manager->_window;
  void *hwnd=nullptr;
  if(!Field(window,0x218,hwnd)||!hwnd)return false;
  DWORD pid=0;
  if(!GetWindowThreadProcessId(static_cast<HWND>(hwnd),&pid)||pid!=GetCurrentProcessId())return false;
  if(!Field(window,0x234,dc)||!Field(window,0x238,context)||!dc||!context)return false;
  return WindowFromDC(static_cast<HDC>(dc))==static_cast<HWND>(hwnd);
}
// 只观察当前 VM 的身份；借用者必须再检查对应 Lua 代次和 owner 阶段。
void *ClientLuaStateIdentity() noexcept {
  return IsMain()&&g_LuaEngine?g_LuaEngine->_state:nullptr;
}
} // namespace cs::game
