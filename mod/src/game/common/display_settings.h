#pragma once
// 本机显示偏好：自身角色高亮描边颜色、其他玩家角色透明度。
// 只影响本机绘制，不进入网络、同步状态或原版 options.ini；Host 与 Client 各自读取本机设置。
// 持久化到 isaac-cs/display.json（与 config.json 同目录）；只在主线程读写。
namespace cs::game {
// 描边颜色表的条目数；0 表示关闭描边，1 起为具体颜色（顺序与 UI 下拉框一致）。
inline constexpr unsigned kOutlineColorCount = 10;
// 透明度档位 0—10，与原版 Online 的 PlayerOpacity 选项同一刻度：alpha = 0.1 + 0.09 × 档位。
inline constexpr unsigned kRemoteOpacityMax = 10;

struct DisplayPreferences final {
  unsigned outline_color = 4;                  // 默认黄色
  unsigned remote_opacity = kRemoteOpacityMax; // 默认不透明（与未设置时的原样绘制一致）
};
// 首次调用时从文件加载；文件缺失或无效时使用默认值。
DisplayPreferences CurrentDisplayPreferences() noexcept;
// 越界返回 false 且不修改；合法值立即生效并写回文件，saved 报告写文件是否成功。
bool SetOutlineColor(unsigned index, bool &saved) noexcept;
bool SetRemoteOpacity(unsigned level, bool &saved) noexcept;

struct OutlineRgb final {
  float r = 1, g = 1, b = 1;
};
// index 为 0（关闭）或越界时返回 false。
bool OutlineColorRgb(unsigned index, OutlineRgb &rgb) noexcept;
// 档位换算成绘制 alpha 系数（0.1—1.0）。
float RemoteOpacityFactor(unsigned level) noexcept;
} // namespace cs::game
