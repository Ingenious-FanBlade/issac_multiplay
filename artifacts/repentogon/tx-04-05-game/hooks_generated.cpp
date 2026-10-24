// 由 gen_binding.py 生成，请修改 binding/hooks.json。
#include "runtime.h"
#include "IsaacRepentance.h"
#include "hooks/cs_hook.h"
#include "binding/offsets.h"
#include "game/bridge.h"
// 自有定义仅借用类型表达 ABI，不构造这些对象。
struct CsStatic {};
struct CsEntityList {};
struct CsLifecycle {};
struct CsDevice {};
CS_HOOK_STATIC(manager_update, Loop, "Manager::Update", Manager, Update, -10000, 0x52e3e0, "", false, 1, 4, Manager, __stdcall, (bool value), void) {
    cs::CallScope scope(record);
    cs::FrameScope frame(scope.main);
    super(value);
}
CS_HOOK_METHOD(game_update, Loop, "Game::Update", Game, Update, 0, 0x2f6d30, "", false, 0, 5, Game, , (), void) {
    cs::CallScope scope(record);
    super();
}
CS_HOOK_STATIC(post_update, Loop, "IsaacCS::PostUpdateCaller", CsStatic, PostUpdateCaller, 0, 0x446c90, "55 8B EC 6A FF 68 ?? ?? ?? ?? 64 A1 00 00 00 00 50 83 EC 1C 53 56 57 A1 ?? ?? ?? ?? 33 C5 50 8D", true, 0, 2, None, __cdecl, (), int) {
    cs::CallScope scope(record);
    auto result = super();
    return result;
}
CS_HOOK_METHOD(game_exit, Loop, "Game::Exit", Game, Exit, 0, 0x2f6270, "", false, 1, 5, None, , (bool shouldSave), void) {
    cs::CallScope scope(record);
    super(shouldSave);
    if (scope.main) cs::RunExit();
}
CS_HOOK_STATIC(new_room, Loop, "IsaacCS::NewRoomCaller", CsStatic, NewRoomCaller, 0, 0x44b930, "55 8B EC 6A FF 68 ?? ?? ?? ?? 64 A1 00 00 00 00 50 83 EC 1C 53 56 57 A1 ?? ?? ?? ?? 33 C5 50 8D", true, 0, 2, None, __cdecl, (), int) {
    cs::CallScope scope(record);
    auto result = super();
    if (scope.main) cs::game::NewRoomReturned();
    return result;
}
CS_HOOK_METHOD(manager_render, Loop, "Manager::Render", Manager, Render, 0, 0x52ec40, "", false, 0, 5, Render, , (), void) {
    cs::CallScope scope(record);
    super();
}
CS_HOOK_METHOD(start_game, Loop, "Manager::ExecuteStartGame", Manager, ExecuteStartGame, 0, 0x532a20, "", false, 0, 5, Start, , (), void) {
    cs::CallScope scope(record);
    super();
}
CS_HOOK_STATIC(post_start, Loop, "LuaEngine::PostGameStart", LuaEngine, PostGameStart, 0, 0x44b660, "", false, 1, 4, None, __stdcall, (unsigned int state), void) {
    cs::CallScope scope(record);
    super(state);
}
CS_HOOK_METHOD(console_check, Loop, "IsaacCS::ConsoleGate", Console, ConsoleGate, 0, 0x287020, "8B D1 56 57 8B 7A 4C 8B 72 40 66 0F 1F 44 00 00 8B 42 50 03 42 4C 3B F8 74 38 85 F6 74 04 8B 06", true, 0, 1, None, , (), bool) {
    cs::CallScope scope(record);
    auto result = super();
    if (scope.main) cs::game::ObserveConsole(result);
    return result;
}
CS_HOOK_METHOD(room_update, Loop, "IsaacCS::RoomUpdate", Room, RoomUpdate, 0, 0x3f7f60, "53 8B DC 83 EC 08 83 E4 F8 83 C4 04 55 8B 6B 04 89 6C 24 04 8B EC 6A FF 68 ?? ?? ?? ?? 64 A1 00", true, 0, 5, None, , (), void) {
    cs::CallScope scope(record);
    cs::game::SimulationScope simulation(cs::game::SimulationKind::Room, this);
    super();
}
CS_HOOK_METHOD(entity_list, Loop, "IsaacCS::EntityListUpdate", CsEntityList, EntityListUpdate, 0, 0x13f00, "53 8B DC 83 EC 08 83 E4 F8 83 C4 04 55 8B 6B 04 89 6C 24 04 8B EC 6A FF 68 ?? ?? ?? ?? 64 A1 00", true, 1, 1, None, , (bool partial), int) {
    cs::CallScope scope(record);
    auto result = super(partial);
    return result;
}
CS_HOOK_METHOD(restore, Loop, "Game::RestoreState", Game, RestoreState, 0, 0x2f43a0, "", false, 2, 5, Restore, , (GameState* state, bool start), void) {
    cs::CallScope scope(record);
    if (scope.main) { cs::game::RestoreBeginning(); cs::RunExit(); }
    super(state, start);
}
CS_HOOK_METHOD(room_lifecycle, Loop, "IsaacCS::RoomLifecycleFSM", CsLifecycle, RoomLifecycleFSM, 0, 0x509100, "55 8B EC 6A FF 68 ?? ?? ?? ?? 64 A1 00 00 00 00 50 83 EC 58 A1 ?? ?? ?? ?? 33 C5 89 45 F0 53 56", true, 0, 1, Lifecycle, , (), uintptr_t) {
    cs::CallScope scope(record);
    auto result = super();
    return result;
}
CS_HOOK_METHOD(door_update, Loop, "RoomTransition::Update", RoomTransition, Update, 0, 0x426430, "", false, 0, 5, Door, , (), void) {
    cs::CallScope scope(record);
    super();
}
CS_HOOK_METHOD(door_nested, Loop, "IsaacCS::DoorNestedOwner", Game, DoorNestedOwner, 0, 0x2fa0f0, "55 8B EC 51 53 56 8B F1 57 33 FF 8B 86 AC BA 01 00 8B D0 8B 9E A8 BA 01 00 2B D3 C1 FA 02 85 D2", true, 2, 1, NestedDoor, , (int room, int dimension), int) {
    cs::CallScope scope(record);
    auto result = super(room, dimension);
    return result;
}
CS_HOOK_STATIC(odd_helper, Loop, "IsaacCS::OddHelper", CsStatic, OddHelper, 0, 0x2f9340, "55 8B EC 83 EC 0C 53 8B 1D ?? ?? ?? ?? 56 57 89 5D FC 8D 8B D4 7F 06 00 E8 ?? ?? ?? ?? 84 C0 0F", true, 0, 2, Odd, __cdecl, (), int) {
    cs::CallScope scope(record);
    cs::game::SimulationScope simulation(cs::game::SimulationKind::Odd);
    auto result = super();
    return result;
}
CS_HOOK_METHOD(input_update, Input, "IsaacCS::InputManagerUpdate", InputManager, InputManagerUpdate, 0, 0x5edcc0, "55 8B EC 6A FF 68 ?? ?? ?? ?? 64 A1 00 00 00 00 50 83 EC 1C 53 56 57 A1 ?? ?? ?? ?? 33 C5 50 8D", true, 0, 1, Input, , (), int) {
    cs::CallScope scope(record);
    auto result = super();
    if (scope.main) cs::game::InputFrameFinished();
    return result;
}
CS_HOOK_METHOD(device_update, Input, "IsaacCS::DeviceCommonUpdate", CsDevice, DeviceCommonUpdate, 0, 0x5ec900, "53 8B DC 83 EC 08 83 E4 F8 83 C4 04 55 8B 6B 04 89 6C 24 04 8B EC 83 EC 10 56 57 8B F9 8B 47 28", true, 0, 1, Device, , (), bool) {
    cs::CallScope scope(record);
    cs::game::DeviceScope device_scope(this);
    auto result = super();
    device_scope.Returned(result);
    return result;
}
CS_HOOK_METHOD(player_update, Player, "Entity_Player::Update", Entity_Player, Update, -10000, 0x37bd40, "", false, 0, 5, None, , (), void) {
    cs::CallScope scope(record);
    cs::game::PlayerScope player_scope(this);
    super();
}
CS_HOOK_METHOD(player_native, Player, "Entity_Player::Update", Entity_Player, Update, 10000, 0x37bd40, "", false, 0, 5, None, , (), void) {
    cs::CallScope scope(record);
    cs::game::PlayerNativeReached(this);
    super();
}
CS_HOOK_METHOD(move, Player, "IsaacCS::ConsumerMove", Entity_Player, ConsumerMove, 0, 0x372540, "55 8B EC 6A FF 68 ?? ?? ?? ?? 64 A1 00 00 00 00 50 83 EC 5C A1 ?? ?? ?? ?? 33 C5 89 45 F0 56 57", true, 0, 5, None, , (), void) {
    cs::CallScope scope(record);
    cs::game::ConsumerScope consumer(this, cs::game::ConsumerKind::Movement);
    super();
}
CS_HOOK_METHOD(bomb, Player, "IsaacCS::ConsumerBomb", Entity_Player, ConsumerBomb, 0, 0x37a210, "55 8B EC 83 E4 F0 81 EC B8 00 00 00 56 57 8B F9 80 BF 7D 20 00 00 00 0F 85 8E 0B 00 00 E8 ?? ??", true, 0, 5, None, , (), void) {
    cs::CallScope scope(record);
    cs::game::ConsumerScope consumer(this, cs::game::ConsumerKind::Bomb);
    super();
}
CS_HOOK_METHOD(active_item, Player, "Entity_Player::ControlActiveItem", Entity_Player, ControlActiveItem, 0, 0x389e50, "", false, 1, 5, None, , (int slot), void) {
    cs::CallScope scope(record);
    cs::game::ConsumerScope consumer(this, cs::game::ConsumerKind::Item);
    super(slot);
}
CS_HOOK_METHOD(card_pill, Player, "IsaacCS::ConsumerCardPill", Entity_Player, ConsumerCardPill, 0, 0x39bdd0, "55 8B EC 6A FF 68 ?? ?? ?? ?? 64 A1 00 00 00 00 50 83 EC 20 53 56 57 A1 ?? ?? ?? ?? 33 C5 50 8D", true, 0, 5, None, , (), void) {
    cs::CallScope scope(record);
    cs::game::ConsumerScope consumer(this, cs::game::ConsumerKind::Pill);
    super();
}
CS_HOOK_METHOD(shoot, Player, "IsaacCS::ConsumerShoot", Entity_Player, ConsumerShoot, 0, 0x373990, "55 8B EC 6A FF 68 ?? ?? ?? ?? 64 A1 00 00 00 00 50 81 EC D4 01 00 00 A1 ?? ?? ?? ?? 33 C5 89 45", true, 0, 5, None, , (), void) {
    cs::CallScope scope(record);
    cs::game::ConsumerScope consumer(this, cs::game::ConsumerKind::Shooting);
    if (!consumer.CallOriginal()) return;
    super();
}
CS_HOOK_METHOD(drop, Player, "IsaacCS::ConsumerDrop", Entity_Player, ConsumerDrop, 0, 0x39c300, "55 8B EC 83 E4 F8 83 EC 14 53 56 57 8B F9 83 7F 2C 00 0F 85 92 04 00 00 80 BF 7D 20 00 00 00 0F", true, 0, 5, None, , (), void) {
    cs::CallScope scope(record);
    cs::game::ConsumerScope consumer(this, cs::game::ConsumerKind::Drop);
    super();
}
CS_HOOK_METHOD(join_pass, Join, "IsaacCS::JoinPass", PlayerManager, JoinPass, 0, 0x591360, "55 8B EC 83 E4 F8 81 EC FC 00 00 00 A1 ?? ?? ?? ?? 33 C4 89 84 24 F8 00 00 00 8B 15 ?? ?? ?? ??", true, 0, 1, Join, , (), unsigned int) {
    cs::CallScope scope(record);
    auto result = super();
    return result;
}
CS_HOOK_METHOD(console_command, Control, "Console::RunCommand", Console, RunCommand, -10000, 0x288100, "", false, 3, 5, Console, , (std_string& in, std_string* out, Entity_Player* player), void) {
    cs::CallScope scope(record);
    if (scope.main && cs::OwnCommand(in)) {
        try {
            auto* game = static_cast<unsigned char*>(cs::GamePointer());
            const bool lua = game && *reinterpret_cast<void**>(game+cs::offset::CommandCapture);
            auto response = cs::Command(in, lua ? "lua" : "console");
            // 与 REPENTOGON 一致：返回缓冲和控制台输出二选一。
            if (out) out->append(response);
            // Print 捕获分支直接追加字符串并返回，不写控制台历史。
            else Print(response, 0xFFFFFFFFu, 150);
        } catch (...) { cs::Trace("command_error", "exception"); }
        return;
    }
    super(in, out, player);
}
