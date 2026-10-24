#pragma once
#include <cstdint>
namespace cs::offset {
// 仅适用于 v1.9.7.12；名称和依据见 docs/hooks/repentogon-binding.md。
inline constexpr uintptr_t Game = 0x7E49F4, Manager = 0x7E4A1C, LuaEngine = 0x7E4A0C;
inline constexpr uintptr_t InputManager = 0x7CE438;
// tx-04-04 静态复核：Console+0x104 为 std::string*，旧资料的 0x68158 有误。
inline constexpr uintptr_t GameConsole = 0x67FD4, CommandCapture = 0x680D8;
inline constexpr uintptr_t GameStateFlags = 0x25AC0, RunActive = 0x25B02;
inline constexpr uintptr_t PlayerManager = 0x1BAA8, Starting = 0x4AD10;
inline constexpr uintptr_t ControllerIndex = 0x160C, PlayerIndex = 0x1610, MoveDirection = 0x1618;
inline constexpr uintptr_t KeyboardVtable = 0x73B490, GamepadVtable = 0x73B3F0;
} // namespace cs::offset
