#pragma once
#include "state/anchors.h"

namespace cs::state {
// 子类计数器可按变体递增、递减或停走；方向必须由已核原生分支提供。
// 例如Familiar FireCooldown并非所有变体都递减；未核定字段不能凭名称选择方向。
enum class EntityCounterMode : std::uint32_t { Frozen = 0, Increasing = 1, Decreasing = 2 };
// 线上u64的低32位保存mode，高32位保存value；明确数值排布，不复制C++结构体。
constexpr std::uint64_t EncodeEntityCounter(std::uint32_t game_frame, std::int32_t counter,
                                             EntityCounterMode mode) noexcept {
  const auto anchor = AnchorSignedCounter(game_frame, counter, mode != EntityCounterMode::Frozen,
    mode == EntityCounterMode::Increasing ? CounterDirection::Increasing : CounterDirection::Decreasing);
  return (std::uint64_t(anchor.value) << 32) | static_cast<std::uint32_t>(mode);
}
constexpr bool ValidEntityCounter(std::uint64_t encoded) noexcept {
  return static_cast<std::uint32_t>(encoded) <= 2;
}
// 调用前校验mode；保留负数与模2^32回绕，不把无条件有符号计数夹限为零。
// mode必须表达当前采集窗口真实分支；无法核定活动时钟关系时由owner报告Gap。
constexpr std::int32_t DecodeEntityCounter(std::uint32_t display_frame, std::uint64_t encoded) noexcept {
  const auto mode = static_cast<EntityCounterMode>(static_cast<std::uint32_t>(encoded));
  const auto value = static_cast<std::uint32_t>(encoded >> 32);
  return DisplaySignedCounter(display_frame, {mode == EntityCounterMode::Frozen ? 0u : 1u, value},
    mode == EntityCounterMode::Increasing ? CounterDirection::Increasing : CounterDirection::Decreasing);
}
// 正值倒计时独立于上面的无夹限有符号计数：低32位0=原值、1=到期帧，高32位value。
// advances必须由字段owner提供已核活动条件；未知时钟必须先报Gap，不能传false冒充停走。
std::uint64_t EncodeEntityCountdown(std::uint32_t game_frame, std::int32_t remaining,
                                    bool advances) noexcept;
bool ValidEntityCountdown(std::uint64_t encoded) noexcept;
// 调用前校验编码；以最近已应用world展示时钟还原，正值到零停走，负哨兵保持原值。
std::int32_t DecodeEntityCountdown(std::uint32_t display_frame, std::uint64_t encoded) noexcept;
// Laser Timeout同时存在正值归零和Shrink中递减到负值的路径；字段专属mode不能与递增计数混用。
enum class LaserTimeoutClock : std::uint32_t { Frozen = 0, PositiveCountdown = 1, SignedDecreasing = 2 };
std::uint64_t EncodeLaserTimeout(std::uint32_t game_frame, std::int32_t timeout,
                                 LaserTimeoutClock clock) noexcept;
bool ValidLaserTimeout(std::uint64_t encoded) noexcept;
// mode=1到零停走，mode=2保留负值和回绕；不在Client重演移除、Shrink或伤害。
std::int32_t DecodeLaserTimeout(std::uint32_t display_frame, std::uint64_t encoded) noexcept;
}
