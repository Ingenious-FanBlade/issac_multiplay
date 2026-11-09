#pragma once
#include "state/nested.h"

namespace cs::state {
// C/S不使用原版Online的本地/远端玩家筛选。保留既有0/2线值，旧模式1不再合法。
// 这里只表达是否显示受控NPC UI，不包含ControllerId、设备地址或参与者输入绑定。
enum class NpcControlledUi : std::uint8_t { Uncontrolled = 0, Controlled = 2 };

// 仅供已由RTTI核定NPC的当前C/S主线程窗口；只读NPC+F1C，不访问Manager/NetplayManager。
// 正式采集窗口由第10步接入，本函数不建立生命周期见证或选择原版Online分支。
NpcControlledUi ReadNpcControlledUi(const ReadWindow &, std::uint32_t npc, std::string_view field);
// Client解码返回自有纯值；第13步绘制owner消费它，不将枚举写回原生ControllerId。
NpcControlledUi DecodeNpcControlledUi(std::uint8_t encoded, std::string_view field);
}
