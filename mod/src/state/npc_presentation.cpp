#include "state/npc_presentation.h"

namespace cs::state {
NpcControlledUi ReadNpcControlledUi(const ReadWindow &window, std::uint32_t npc, std::string_view field) {
  // 原生两个受控UI分支均只以-1判断无控制者；其他有符号值不能擅自当成未受控。
  constexpr std::uint32_t controller_offset = 0xf1c;
  window.Require(field);
  if (!npc || npc > UINT32_MAX - controller_offset)
    throw Gap(field, "npc_control_ui_address");
  const auto address = npc + controller_offset;
  std::int32_t before{}, after{};
  window.Copy(address, std::as_writable_bytes(std::span<std::int32_t>(&before, 1)), field);
  window.Copy(address, std::as_writable_bytes(std::span<std::int32_t>(&after, 1)), field);
  if (before != after) throw Gap(field, "npc_control_ui_source_changed");
  // IsIdxLocalPlayer属于原版NetplayManager，不能作为本C/S转换的读取或调用来源。
  // C/S展示全部权威参与者的受控NPC；本机/远端输入归属由独立输入系统管理。
  return before == -1 ? NpcControlledUi::Uncontrolled : NpcControlledUi::Controlled;
}

NpcControlledUi DecodeNpcControlledUi(std::uint8_t encoded, std::string_view field) {
  // 模式1是版本42误引入的原版Online筛选结果；拒绝它，不能回退查询本机联机/设备状态。
  switch (encoded) {
    case 0: return NpcControlledUi::Uncontrolled;
    case 2: return NpcControlledUi::Controlled;
    default: throw Gap(field, "npc_control_ui_encoding");
  }
}
}
