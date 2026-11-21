#pragma once

#include "core/session.h"

namespace cs::core {

// 可独立开局的原版角色。复活形态和随主角色生成的身体由原生代码管理。
constexpr bool LobbyCharacter(unsigned character) noexcept {
  return character <= 37 && character != 11 && character != 12 &&
         character != 17 && character != 20;
}

// 全部大厅角色的位图（第 n 位＝角色 n）。
constexpr std::uint64_t LobbyCharacterMask() noexcept {
  std::uint64_t mask = 0;
  for (unsigned id = 0; id <= 37; ++id)
    if (LobbyCharacter(id)) mask |= std::uint64_t(1) << id;
  return mask;
}
inline constexpr std::uint64_t kLobbyCharacterMask = LobbyCharacterMask();

// 房主解锁位图：必须含以撒（第 0 位），且只能含大厅角色。由房主随名单广播给客机。
constexpr bool ValidLobbyUnlocked(std::uint64_t unlocked) noexcept {
  return (unlocked & 1) && !(unlocked & ~kLobbyCharacterMask);
}
// 角色是否在房主解锁位图中；房主和客机的选角都以此为准。
constexpr bool LobbyCharacterUnlocked(std::uint64_t unlocked, unsigned character) noexcept {
  return LobbyCharacter(character) && ((unlocked >> character) & 1);
}

enum class LobbyPhase : unsigned { Off, Waiting, Starting, Playing };

// 名单使用稳定的参与者编号。异步线程通过一个整数传递整份快照。
struct LobbyRoster {
  LobbyPhase phase = LobbyPhase::Off;
  unsigned present = 0, ready = 0;
  std::array<unsigned, kMaximumParticipants> characters{};

  bool CanStart() const noexcept {
    return phase == LobbyPhase::Waiting && (present & ~1u) != 0 &&
           ready == (present & ~1u);
  }
  std::uint64_t Pack() const noexcept {
    std::uint64_t value = unsigned(phase) | (present << 2) | (ready << 6);
    for (unsigned i = 0; i < characters.size(); ++i)
      value |= std::uint64_t(characters[i]) << (10 + 6 * i);
    return value;
  }
  static LobbyRoster Unpack(std::uint64_t value) noexcept {
    LobbyRoster result{LobbyPhase(value & 3), unsigned((value >> 2) & 15),
                       unsigned((value >> 6) & 15)};
    for (unsigned i = 0; i < result.characters.size(); ++i)
      result.characters[i] = unsigned((value >> (10 + 6 * i)) & 63);
    return result;
  }
  static bool ValidPacket(std::uint64_t value) noexcept {
    const auto roster = Unpack(value);
    if (roster.Pack() != value || roster.phase == LobbyPhase::Off ||
        !(roster.present & 1) || (roster.ready & ~(roster.present & ~1u)))
      return false;
    for (const auto character : roster.characters)
      if (!LobbyCharacter(character)) return false;
    return true;
  }
};

// 选角和准备使用同一条幂等消息，避免在丢包时看到旧角色配上新准备位。
constexpr unsigned kLobbyReady = 64;
constexpr bool ValidLobbySelection(std::uint64_t value) noexcept {
  return value < 2 * kLobbyReady && LobbyCharacter(unsigned(value) & 63);
}

} // namespace cs::core
