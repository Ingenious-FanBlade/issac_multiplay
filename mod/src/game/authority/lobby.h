#pragma once
#include "core/lobby.h"
#include <string>

namespace cs::game {
struct SessionRuntime;

// 大厅只在 Manager 主线程修改。房间名单和选角提交后用于原生开局与远端 Join。
struct HostLobby {
  core::LobbyRoster roster;
  // 房主存档已解锁的角色位图；Waiting 期间每帧刷新，并随名单广播给客机。
  std::uint64_t unlocked = 1;
  std::string addresses;
  unsigned port = 0;
  std::uint64_t started_at = 0;
  unsigned locked_members = 0;
  bool native_start_requested = false;
};

void RefreshHostLobby(SessionRuntime &runtime) noexcept;
const char *SelectLobbyCharacter(unsigned character) noexcept;
const char *SetLobbyReady(bool ready) noexcept;
const char *StartLobbyGame() noexcept;
void PumpLobbyStart() noexcept;
bool LobbyCanListen() noexcept;
bool LobbyMenuReady() noexcept;
bool RequestLobbyNativeStart(unsigned character) noexcept;
std::uint64_t LobbyUnlockedCharacters() noexcept;
std::string LobbyFields();
}
