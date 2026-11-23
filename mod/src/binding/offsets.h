#pragma once
#include <cstdint>
namespace cs::offset {
// 仅适用于 v1.9.7.12；名称和依据见 docs/archive/hooks/repentogon-binding.md。
inline constexpr uintptr_t Game = 0x7E49F4, Manager = 0x7E4A1C,
                           LuaEngine = 0x7E4A0C;
inline constexpr uintptr_t InputManager = 0x7CE438;
// J273输入前端：先泵窗口/控制器事件，再进入既有InputManagerUpdate，不更新世界。
inline constexpr uintptr_t ClientInputPump = 0x5EC130;
// tx-04-04 静态复核：Console+0x104 为 std::string*，旧资料的 0x68158 有误。
inline constexpr uintptr_t GameConsole = 0x67FD4, CommandCapture = 0x680D8;
inline constexpr uintptr_t GameStateFlags = 0x25AC0, RunActive = 0x25B02;
inline constexpr uintptr_t PlayerManager = 0x1BAA8, Starting = 0x4AD10;
// J273 0x995B30：玩家中心缓存按本机 Manager 帧号复用，其他原生调用也共享此缓存。
inline constexpr uintptr_t PlayerCenterFrame = 0x7D01E8, PlayerCenterPosition = 0x80136C;
// J273 0x97E070：PlayerHUD 槽锚点（thiscall HUD*, Vector* out, slot, layout → out）。
// 只读 HUD 槽偏移、屏幕尺寸与 Online 标志并写出参，无其他副作用；97EB40 与 Online 名牌 97F4E0 共用此锚点。
inline constexpr uintptr_t PlayerHudAnchor = 0x57E070;
// J273 全局覆盖色栈（ANM2 层绘制 404570 在栈非空时用栈顶 ColorMod 代替精灵自身颜色）：
// 0x402A70 thiscall(ColorMod*) 压栈；弹栈无独立函数，原生调用点（783680/784A60）直接把计数
// BE6F40 减一，减到 0 时把环形起点 BE6F3C 置 0。这里登记三者，弹栈照搬同一写法。
inline constexpr uintptr_t ColorOverridePush = 0x2A70, ColorOverrideCount = 0x7E6F40,
                           ColorOverrideStart = 0x7E6F3C;
// J273 0x7D1C90：只读判定（+370 标志且类型 29/38 的关联身体关系）；为真时 783680 自己压入半透明覆盖色，
// 原版 Online 此时不画描边（覆盖色会把描边色顶掉）。
inline constexpr uintptr_t PlayerOverlayGhost = 0x3D1C90;
// tx-04-09：703 helper RVA3A3A40，+1590是各主动槽的1-based Esau Jr备份关联，不是PlayerIndex。
inline constexpr uintptr_t ActiveItemSlotBase = 0x1574, ActiveItemSlotStride = 0x20,
                           EsauJrAssociation = 0x1590, EsauJrStates = 0x80;
inline constexpr uintptr_t ControllerIndex = 0x160C, PlayerIndex = 0x1610,
                           MoveDirection = 0x1618;
inline constexpr uintptr_t KeyboardVtable = 0x73B490, GamepadVtable = 0x73B3F0;
// J273 原始键位 vtable 槽 5/6 直接读取当前和上一轮键盘缓存，不获取输入后端锁。
inline constexpr uintptr_t KeyboardButtonPressed = 0x633CA0,
                           KeyboardButtonTriggered = 0x633CD0;
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
// tx-04-06：ST1–ST9 及本轮物理 worker/Mutex 指令确认；均为目标版本 RVA。
inline constexpr uintptr_t DeviceBaseVtable = 0x732F04, DeviceCtor = 0x5EC570;
inline constexpr uintptr_t DeviceName = 0x3FC, InputBackendMutex = 0x7CE498;
inline constexpr uintptr_t MutexLock = 0x5E60E0, RemoveCoPlayer = 0x590560;
inline constexpr uintptr_t MutexUnlock = 0x5E61A0;
inline constexpr uintptr_t EntityVariant = 0x2C, PlayerType = 0x13BC;
inline constexpr uintptr_t JoinRowKey = 0xC, JoinRowCursor = 0x14,
                           JoinRowStride = 0x14, JoinRows = 4;
inline constexpr uintptr_t CharacterBegin = 0x70, CharacterEnd = 0x74;
} // namespace cs::offset
