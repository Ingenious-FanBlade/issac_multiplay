#pragma once
#include <bit>
#include <cstdint>

namespace cs::state {
// 帧锚点使用 Host u32 帧空间，按模 2^32 运算，避免负 Timeout 或计数回绕引发有符号溢出。
// Client 必须传最近已应用 world 帧的 game_frame，不能使用本地渲染时钟。
constexpr std::int32_t ExpireFrame(std::uint32_t game_frame, std::int32_t remaining) noexcept {
  return std::bit_cast<std::int32_t>(game_frame + std::bit_cast<std::uint32_t>(remaining));
}
constexpr std::int32_t RemainingFrames(std::uint32_t display_frame, std::int32_t expire_frame) noexcept {
  return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(expire_frame) - display_frame);
}
}
