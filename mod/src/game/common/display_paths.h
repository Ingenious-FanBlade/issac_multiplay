#pragma once
#include "game/common/bridge.h"

namespace cs::game {
// Q7仅同一张实际放行表授予显示身份；完整RoomTransition／ItemOverlay混合Update不在表内。
constexpr bool NativeDisplayPath(ClientEntry entry) noexcept {
  return entry==ClientEntry::GameRender||entry==ClientEntry::RoomPreRender||entry==ClientEntry::RoomRender||
    (entry>=ClientEntry::DisplayHudUpdate&&entry<=ClientEntry::DisplayCursor);
}
}
