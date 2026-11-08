#include "state/entity_counters.h"

namespace cs::state {
namespace {
// 线上两字显式拆开，不依赖CountdownAnchor的C++对齐或宿主字节序。
constexpr CountdownAnchor CountdownWords(std::uint64_t encoded) noexcept {
  return {static_cast<std::uint32_t>(encoded), static_cast<std::uint32_t>(encoded >> 32)};
}
}

std::uint64_t EncodeEntityCountdown(std::uint32_t game_frame, std::int32_t remaining,
                                    bool advances) noexcept {
  const auto anchor = AnchorCountdown(game_frame, remaining, advances);
  return (std::uint64_t(anchor.value) << 32) | anchor.mode;
}

bool ValidEntityCountdown(std::uint64_t encoded) noexcept {
  return ValidCountdown(CountdownWords(encoded));
}

std::int32_t DecodeEntityCountdown(std::uint32_t display_frame, std::uint64_t encoded) noexcept {
  return DisplayCountdown(display_frame, CountdownWords(encoded));
}

std::uint64_t EncodeLaserTimeout(std::uint32_t game_frame, std::int32_t timeout,
                                 LaserTimeoutClock clock) noexcept {
  if (clock == LaserTimeoutClock::SignedDecreasing)
    return EncodeEntityCounter(game_frame, timeout, EntityCounterMode::Decreasing);
  return EncodeEntityCountdown(game_frame, timeout, clock == LaserTimeoutClock::PositiveCountdown);
}

bool ValidLaserTimeout(std::uint64_t encoded) noexcept {
  return static_cast<std::uint32_t>(encoded) <= 2;
}

std::int32_t DecodeLaserTimeout(std::uint32_t display_frame, std::uint64_t encoded) noexcept {
  if (static_cast<std::uint32_t>(encoded) == 2)
    return DecodeEntityCounter(display_frame, encoded);
  return DecodeEntityCountdown(display_frame, encoded);
}
}
