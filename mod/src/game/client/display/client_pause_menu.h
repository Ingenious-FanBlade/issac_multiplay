#pragma once
// 客机本地暂停菜单（原版 PauseScreen，J273 Game+0x2363C）。
// 菜单只属于本机：不暂停 Host、不上线、不改同步状态；Host 世界在菜单背后照常显示。
// 原生输入／更新／绘制复用原函数，只拦截对客机有害的分支：
//   - “退出”改为离开 C/S 会话（原生是写 Game fade 后保存并结束本局）；
//   - 选项里的“重选控制器”（选项类型 1，会重绑同步 Player 的设备）；
//   - PauseScreen::Update 内 Glitched Crown 的拾取物轮换（由 client_pause_pickup_cycle Hook 拦截）。
namespace cs::game {
// 本帧输入处理结果。
enum class ClientPauseAction : unsigned {
  None,    // 无变化（含菜单内导航）
  Opened,  // 本帧刚打开菜单，调用方须立即封住本帧玩法输入
  Exit     // 玩家选择“退出”：菜单已立即关闭，调用方须离开 C/S 会话
};
// Manager 边界、输入泵（ClientPollInput）之后调用；只在客机已发布原生显示时调用。
// local_ordinal：本机 C/S 控制组序号，用来让暂停页显示本机角色的属性与道具。
// may_open：Host 终结分支允许打开（正常游玩或 Host 暂停）、窗口有焦点、无控制台。
// branch：已安装 World 的终结分支数值（未知为 0xFFFFFFFF），只用于拒绝打开时的诊断记录。
ClientPauseAction ClientPauseMenuInput(unsigned local_ordinal,bool may_open,unsigned branch) noexcept;
// 本帧任一本机设备是否触发了打开菜单的动作（PAUSE 或 MENUBACK）；只读，用于外层拒绝诊断。
bool ClientPauseKeyPressed() noexcept;
// 诊断：物理 ESC 按下沿时记录本帧菜单判定涉及的每一项（原生动作触发、阻断字段、Hook、外层条件）。
void ClientPauseDiagnose(bool focused,bool console,bool native,bool branch_known,unsigned branch) noexcept;
// 菜单当前是否打开（PauseScreen.state != 0）。
bool ClientPauseMenuOpen() noexcept;
// 显示推进 30Hz 批次内（具名入口 DisplayPause）调用原生 PauseScreen::Update。
// 返回 true 表示本次调用后菜单已关闭（Dissapear 动画完成）。
bool ClientPauseMenuUpdate() noexcept;
// 立即关闭菜单（不播动画）：退出、会话退休或失败时使用，避免把打开的菜单留给本机菜单／下一局。
void ClientPauseMenuReset() noexcept;
// 拦截 Hook 是否已真实安装；未安装时不允许打开菜单（PauseScreen::Update 会改同步拾取物）。
bool ClientPauseMenuAvailable() noexcept;
} // namespace cs::game
