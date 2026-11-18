#pragma once
#include <array>
#include <cstdint>
#include <string_view>

namespace cs::state {
// 图像路径描述由本机装载实际 Image，线上不假定 Host 已创建该图像。
inline constexpr std::uint8_t kResourceImagePathKind=10;

// SDK C++17 边界只读的具名资源输入；文本借用精确资源快照，不含游戏地址或媒体内容。
// numbers 槽与同源资源布局的字段号一致；字符串／层目录单列，不重新解析线上字节。
struct ResourceLocalLayer {std::uint16_t id=0;std::uint32_t frames=0;};
struct ResourceLocalDescription {
  std::uint8_t table=255,kind=255;
  std::uint16_t index=UINT16_MAX;
  std::array<std::uint64_t,16> numbers{};
  std::array<std::string_view,2> text{};
  std::array<std::uint16_t,2> layer_count{};
  std::array<std::array<ResourceLocalLayer,256>,2> layers{};
};
} // namespace cs::state
