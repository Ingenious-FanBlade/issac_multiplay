#pragma once
#include "game/common/bridge.h"
#include "core/lobby.h"
#include <string>
namespace cs::game {
// Client 角色独立于 Authority 会话。
bool ClientBusy() noexcept;
const char *ClientStateName() noexcept;
const char *StartClient(const std::string &ipv4, unsigned port, int device_key, bool lobby=false) noexcept;
const char *UpdateClientLobby(int character, bool ready) noexcept;
core::LobbyRoster ClientLobbyRoster(unsigned &ordinal, unsigned &desired, std::string &details);
std::string ClientLobbyFailure();
#ifdef ISAACCS_E2E
std::string ClientTestExerciseOwnership(const std::string &token) noexcept;
std::string ClientTestExerciseVm(const std::string &token) noexcept;
#endif
void StopClient() noexcept;
std::string ClientFields();
std::string ClientHudTimingFields();
std::string ClientStreamFields(unsigned page);
void ClientUiHeartbeat() noexcept;
// 仅在最外层Manager/受控Input owner执行；采样失败保持中性，绝不建立本地Player绑定。
bool ClientDeviceReturned(void *device, bool success, unsigned polls, bool poll_ok);
void ClientInputFinished() noexcept;
} // namespace cs::game
