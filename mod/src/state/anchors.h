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
// 条件倒计时用两个 u32 编码：mode=0 保存原计数（停走、零或负哨兵），mode=1 保存到期帧。
// advances 必须由该字段已核的原生更新条件提供；不能从 Game 帧推进或一次差分猜测。
// 两字必须分别编码，不能依赖本结构在编译器中的对象布局。
struct CountdownAnchor { std::uint32_t mode, value; };
constexpr CountdownAnchor AnchorCountdown(std::uint32_t game_frame, std::int32_t remaining,
                                           bool advances) noexcept {
  const auto raw = std::bit_cast<std::uint32_t>(remaining);
  return advances && remaining > 0 ? CountdownAnchor{1, game_frame + raw} : CountdownAnchor{0, raw};
}
constexpr bool ValidCountdown(CountdownAnchor anchor) noexcept { return anchor.mode <= 1; }
// 调用前必须校验 mode；到期后停在零，不把已结束的倒计时变成负哨兵。
constexpr std::int32_t DisplayCountdown(std::uint32_t display_frame, CountdownAnchor anchor) noexcept {
  if (!anchor.mode) return std::bit_cast<std::int32_t>(anchor.value);
  const auto remaining = std::bit_cast<std::int32_t>(anchor.value - display_frame);
  return remaining > 0 ? remaining : 0;
}
// 原生无夹限计数与普通正倒计时分开：颜色Lifespan连负值也递减，毒/火伤害相位递增。
// 方向由布局处理器固定，线上仍是mode/value两字，不发送每帧变化的计数。
enum class CounterDirection : std::uint8_t { Increasing, Decreasing };
struct SignedCounterAnchor { std::uint32_t mode, value; };
constexpr SignedCounterAnchor AnchorSignedCounter(std::uint32_t game_frame, std::int32_t counter,
                                                   bool advances, CounterDirection direction) noexcept {
  const auto raw = std::bit_cast<std::uint32_t>(counter);
  if (!advances) return {0, raw};
  return {1, direction == CounterDirection::Increasing ? game_frame - raw : game_frame + raw};
}
constexpr bool ValidSignedCounter(SignedCounterAnchor anchor) noexcept { return anchor.mode <= 1; }
// 调用前校验mode；模2^32还原，不把负计数变成哨兵，也不在零处夹限。
constexpr std::int32_t DisplaySignedCounter(std::uint32_t display_frame, SignedCounterAnchor anchor,
                                            CounterDirection direction) noexcept {
  if (!anchor.mode) return std::bit_cast<std::int32_t>(anchor.value);
  return std::bit_cast<std::int32_t>(direction == CounterDirection::Increasing
    ? display_frame - anchor.value : anchor.value - display_frame);
}
}
