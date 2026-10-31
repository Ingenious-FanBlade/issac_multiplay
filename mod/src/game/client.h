#pragma once
#include "game/bridge.h"
#include <string>

namespace cs::game {
// Client角色独立于Authority SessionRuntime，socket失败也不自动交还原生模拟。
bool ClientBusy() noexcept;
// 教程按键图标使用本地输入设备身份，不传输Authority设备或建立本地Player。
int ClientDeviceKey() noexcept;
const char *ClientStateName() noexcept;
const char *StartClient(const std::string &ipv4, unsigned port, int device_key) noexcept;
void StopClient() noexcept;
std::string ClientFields();
void ClientUiHeartbeat() noexcept;
// 仅在最外层Manager/受控Input owner执行；采样失败保持中性，绝不建立本地Player绑定。
bool ClientDeviceReturned(void *device, bool success, unsigned polls, bool poll_ok);
void ClientInputFinished() noexcept;
} // namespace cs::game
