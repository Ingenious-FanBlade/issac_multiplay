#pragma once
#include <string>
namespace cs::game {
// 首次稳定Lua主线程安装，之后常驻；不依赖观测/录制或会话启用。
void PumpModPersistenceHooks() noexcept;
bool ModPersistenceReady() noexcept;
std::string ModPersistenceFields();
bool TestInternalModPersistence() noexcept;
} // namespace cs::game
