#pragma once
#include <cstdint>
namespace cs::offset {
// 仅适用于 v1.9.7.12；名称和依据见 docs/hooks/repentogon-binding.md。
inline constexpr uintptr_t Game = 0x7E49F4, Manager = 0x7E4A1C,
                           LuaEngine = 0x7E4A0C;
inline constexpr uintptr_t InputManager = 0x7CE438;
// tx-04-04 静态复核：Console+0x104 为 std::string*，旧资料的 0x68158 有误。
inline constexpr uintptr_t GameConsole = 0x67FD4, CommandCapture = 0x680D8;
inline constexpr uintptr_t GameStateFlags = 0x25AC0, RunActive = 0x25B02;
inline constexpr uintptr_t PlayerManager = 0x1BAA8, Starting = 0x4AD10;
inline constexpr uintptr_t ControllerIndex = 0x160C, PlayerIndex = 0x1610,
                           MoveDirection = 0x1618;
inline constexpr uintptr_t KeyboardVtable = 0x73B490, GamepadVtable = 0x73B3F0;
// tx-04-05：官方 .zhl 字段与 T1–T7 静态指令；所有版本布局集中在 binding。
inline constexpr uintptr_t CurrentRoom = 0x18300, RoomIndex = 0x18304,
                           Dimension = 0x1830C;
inline constexpr uintptr_t RoomTransition = 0x1B83C, StageTransition = 0x1BA78;
inline constexpr uintptr_t PauseScreen = 0x2363C, Focus = 0x4AFAA,
                           PauseOnFocusLost = 0x2A1D7;
inline constexpr uintptr_t EntityType = 0x28, EntityValid = 0x170,
                           EntityExists = 0x172;
inline constexpr uintptr_t EntityDead = 0x173, EntityRemoved = 0x177,
                           CoopGhost = 0x207D;
inline constexpr uintptr_t VelocityX = 0x360, VelocityY = 0x364;
inline constexpr uintptr_t ShootX = 0x1624, ShootY = 0x1628, ShootFlag = 0x162C;
inline constexpr uintptr_t DeviceId = 8, DeviceEnabled = 0xD;
inline constexpr uintptr_t DeviceVector = 4;
} // namespace cs::offset
