#include "IsaacRepentance.h"
#include "game/bridge.h"
#include "runtime.h"

namespace cs::game {
// 对应公开 EntityPlayer:IsHologram()；不推进动画、不修改角色或控制器。
bool PlayerIsHologram(void *player) noexcept {
  return IsMain() && player &&
         static_cast<Entity_Player *>(player)->IsHologram();
}
} // namespace cs::game
