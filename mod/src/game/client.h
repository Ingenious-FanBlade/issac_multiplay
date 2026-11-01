#pragma once
#include "game/bridge.h"
#include "replica/model.h"
#include <string>

namespace cs::game {
// Client角色独立于Authority SessionRuntime，socket失败也不自动交还原生模拟。
bool ClientBusy() noexcept;
const char *ClientStateName() noexcept;
const char *StartClient(const std::string &ipv4, unsigned port, int device_key) noexcept;
// E2E 只提供有明确来源的纯值目标，复用正式 Client 场景，不启动网络或本地模拟。
const char *ClientTestBegin(const std::string &token, const replica::Scope &scope) noexcept;
std::string ClientTestExerciseOwnership(const std::string &token) noexcept;
bool ClientAdmitTarget(const replica::Scope &scope, std::uint64_t revision,
                       const replica::Value &value) noexcept;
void StopClient() noexcept;
std::string ClientFields();
void ClientUiHeartbeat() noexcept;
// 仅在最外层Manager/受控Input owner执行；采样失败保持中性，绝不建立本地Player绑定。
bool ClientDeviceReturned(void *device, bool success, unsigned polls, bool poll_ok);
void ClientInputFinished() noexcept;
} // namespace cs::game
