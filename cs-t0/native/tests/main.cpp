#include "isaac_cs_t0/active_ipc.hpp"
#include "isaac_cs_t0/boundary_ipc.hpp"
#include "isaac_cs_t0/hooks.hpp"
#include "isaac_cs_t0/input_ipc.hpp"
#include "isaac_cs_t0/input_core.hpp"
#include "isaac_cs_t0/input_runtime.hpp"
#include "isaac_cs_t0/lifecycle.hpp"
#include "isaac_cs_t0/loop_ipc.hpp"
#include "isaac_cs_t0/loop_runtime.hpp"
#include "isaac_cs_t0/mode_ipc.hpp"
#include "isaac_cs_t0/mode_latch.hpp"
#include "isaac_cs_t0/offline_attestation_ipc.hpp"
#include "isaac_cs_t0/offline_attestation_policy.hpp"
#include "isaac_cs_t0/raw_apply_ipc.hpp"
#include "isaac_cs_t0/raw_apply_runtime.hpp"
#include "isaac_cs_t0/sha256.hpp"

#include <Windows.h>
#include <emmintrin.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <utility>

int RunInputCoreTests();

namespace {

int g_failures = 0;
int g_calls = 0;
std::uint32_t g_lifecycle_argument = 0;
DWORD g_lifecycle_thread = 0;
thread_local std::uint32_t g_boundary_depth = 0;
std::uint32_t g_boundary_argument = 0;
DWORD g_boundary_thread = 0;
void* g_spawn_owner = nullptr;
std::array<std::uintptr_t, 8> g_spawn_arguments{};
void* g_lua_allocator = nullptr;
void* g_lua_user_data = nullptr;
void* g_lua_closed_state = nullptr;
DWORD g_lua_thread = 0;
void* g_loop_abi_owner = nullptr;
void* g_loop_abi_state = nullptr;
std::uint32_t g_loop_abi_argument = 0;
void* g_input_abi_owner = nullptr;
std::uint32_t g_input_abi_argument = 0;
std::int32_t g_input_abi_controller = 0;
std::int32_t* g_input_abi_selected = nullptr;
float g_input_abi_value = 0.0F;
std::int32_t g_raw_item_slot = 0;

#define CHECK(expression)                                                                          \
    do                                                                                             \
    {                                                                                              \
        if (!(expression))                                                                         \
        {                                                                                          \
            std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expression);           \
            ++g_failures;                                                                          \
        }                                                                                          \
    } while (false)

int __cdecl OriginalValue(int value)
{
    ++g_calls;
    return value + 17;
}

int __cdecl ReplacementValue(int value)
{
    return value + 23;
}

int PassthroughHarness(int value)
{
    using Function = int(__cdecl*)(int);
    Function original = &OriginalValue;
    return original(value);
}

int __cdecl OriginalBoundaryValue(int value)
{
    ++g_calls;
    g_boundary_argument = static_cast<std::uint32_t>(value);
    g_boundary_thread = GetCurrentThreadId();
    return value ^ 0x55AA;
}

int BoundaryPassthroughHarness(int value)
{
    using Function = int(__cdecl*)(int);
    Function original = &OriginalBoundaryValue;
    ++g_boundary_depth;
    const int result = original(value);
    --g_boundary_depth;
    return result;
}

struct SpawnOwnerHarness {
    std::uint32_t marker = 0;

    void* Original(std::uintptr_t a0, std::uintptr_t a1, std::uintptr_t a2,
                   std::uintptr_t a3, std::uintptr_t a4, std::uintptr_t a5,
                   std::uintptr_t a6, std::uintptr_t a7)
    {
        ++g_calls;
        g_spawn_owner = this;
        g_spawn_arguments = {a0, a1, a2, a3, a4, a5, a6, a7};
        return reinterpret_cast<void*>(a0 ^ a7);
    }
};

void* __fastcall SpawnBridgeHarness(SpawnOwnerHarness* self, void*, std::uintptr_t a0,
                                    std::uintptr_t a1, std::uintptr_t a2,
                                    std::uintptr_t a3, std::uintptr_t a4,
                                    std::uintptr_t a5, std::uintptr_t a6,
                                    std::uintptr_t a7)
{
    return self->Original(a0, a1, a2, a3, a4, a5, a6, a7);
}

void* __cdecl OriginalLuaNewstate(void* allocator, void* user_data)
{
    ++g_calls;
    g_lua_allocator = allocator;
    g_lua_user_data = user_data;
    g_lua_thread = GetCurrentThreadId();
    return reinterpret_cast<void*>(0x12345678u);
}

void __cdecl OriginalLuaClose(void* state)
{
    ++g_calls;
    g_lua_closed_state = state;
    g_lua_thread = GetCurrentThreadId();
}

void* LuaNewstateBridgeHarness(void* allocator, void* user_data)
{
    using Function = void*(__cdecl*)(void*, void*);
    Function original = &OriginalLuaNewstate;
    return original(allocator, user_data);
}

void LuaCloseBridgeHarness(void* state)
{
    using Function = void(__cdecl*)(void*);
    Function original = &OriginalLuaClose;
    original(state);
}

std::uint32_t __stdcall OriginalLifecycle(std::uint32_t argument)
{
    ++g_calls;
    g_lifecycle_argument = argument;
    g_lifecycle_thread = GetCurrentThreadId();
    return argument ^ 0xA5A55A5Au;
}

struct LoopAbiOwnerHarness {
    std::uint32_t bool_result = 0;

    bool PhysicalPoll()
    {
        ++g_calls;
        g_loop_abi_owner = this;
        return bool_result != 0;
    }

    void EntityList(std::uint32_t argument)
    {
        ++g_calls;
        g_loop_abi_owner = this;
        g_loop_abi_argument = argument;
    }

    void ApplyState(void* state, std::uint32_t flag)
    {
        ++g_calls;
        g_loop_abi_owner = this;
        g_loop_abi_state = state;
        g_loop_abi_argument = flag;
    }

    __declspec(noinline) void RoomLifecycleOwner()
    {
        ++g_calls;
        g_loop_abi_owner = this;
        g_loop_abi_argument = 59;
    }

    __declspec(noinline) void RoomLifecycleSimulation()
    {
        ++g_calls;
        g_loop_abi_owner = this;
        g_loop_abi_argument = 61;
    }

    __declspec(noinline) void RoomChangeOwner()
    {
        ++g_calls;
        g_loop_abi_owner = this;
        g_loop_abi_argument = 63;
    }

    __declspec(noinline) void RoomChangeSimulation()
    {
        ++g_calls;
        g_loop_abi_owner = this;
        g_loop_abi_argument = 65;
    }
};

struct InputAbiOwnerHarness {
    std::uint32_t marker = 0;

    __declspec(noinline) bool LogicalBool(std::uint32_t action)
    {
        ++g_calls;
        g_input_abi_owner = this;
        g_input_abi_argument = action;
        return (action ^ marker) == 0x13579BDFu;
    }

    __declspec(noinline) float LogicalScalar(std::uint32_t action)
    {
        ++g_calls;
        g_input_abi_owner = this;
        g_input_abi_argument = action;
        return static_cast<float>(action) + 0.375F;
    }

    __declspec(noinline) bool ManagerBool(std::uint32_t primitive,
                                          std::int32_t controller,
                                          std::int32_t* selected)
    {
        ++g_calls;
        g_input_abi_owner = this;
        g_input_abi_argument = primitive;
        g_input_abi_controller = controller;
        g_input_abi_selected = selected;
        if (selected)
            *selected = 7;
        return primitive == 0x41 && controller == -1;
    }

    __declspec(noinline) float ManagerScalar(std::uint32_t primitive,
                                             std::int32_t controller,
                                             std::int32_t* selected)
    {
        ++g_calls;
        g_input_abi_owner = this;
        g_input_abi_argument = primitive;
        g_input_abi_controller = controller;
        g_input_abi_selected = selected;
        if (selected)
            *selected = 9;
        return static_cast<float>(primitive) * 0.25F +
               static_cast<float>(controller);
    }

    __declspec(noinline) void LogicalSetter(std::uint32_t action, float value)
    {
        ++g_calls;
        g_input_abi_owner = this;
        g_input_abi_argument = action;
        g_input_abi_value = value;
    }
};

__declspec(noinline) bool __fastcall InputLogicalBoolBridgeHarness(
    InputAbiOwnerHarness* self, void*, std::uint32_t action)
{
    return self->LogicalBool(action);
}

__declspec(noinline) float __fastcall InputLogicalScalarBridgeHarness(
    InputAbiOwnerHarness* self, void*, std::uint32_t action)
{
    return self->LogicalScalar(action);
}

__declspec(noinline) bool __fastcall InputManagerBoolBridgeHarness(
    InputAbiOwnerHarness* self, void*, std::uint32_t primitive,
    std::int32_t controller, std::int32_t* selected)
{
    return self->ManagerBool(primitive, controller, selected);
}

__declspec(noinline) float __fastcall InputManagerScalarBridgeHarness(
    InputAbiOwnerHarness* self, void*, std::uint32_t primitive,
    std::int32_t controller, std::int32_t* selected)
{
    return self->ManagerScalar(primitive, controller, selected);
}

__declspec(noinline) void __fastcall InputLogicalSetterBridgeHarness(
    InputAbiOwnerHarness* self, void*, std::uint32_t action, float value)
{
    self->LogicalSetter(action, value);
}

__m128d __cdecl OriginalLoopClock()
{
    ++g_calls;
    return _mm_set_pd(-9.5, 1.25);
}

__m128d __cdecl LoopClockBridgeHarness()
{
    using Function = __m128d(__cdecl*)();
    Function original = &OriginalLoopClock;
    return original();
}

bool __fastcall PhysicalPollBridgeHarness(LoopAbiOwnerHarness* self, void*)
{
    return self->PhysicalPoll();
}

void __fastcall EntityListBridgeHarness(LoopAbiOwnerHarness* self, void*,
                                        std::uint32_t argument)
{
    self->EntityList(argument);
}

void __fastcall StateApplyBridgeHarness(LoopAbiOwnerHarness* self, void*, void* state,
                                        std::uint32_t flag)
{
    self->ApplyState(state, flag);
}

__declspec(noinline) void __fastcall StandardRoomLifecycleOwnerBridgeHarness(
    LoopAbiOwnerHarness* self, void*)
{
    self->RoomLifecycleOwner();
}

__declspec(noinline) void __fastcall StandardRoomLifecycleSimulationBridgeHarness(
    LoopAbiOwnerHarness* self, void*)
{
    self->RoomLifecycleSimulation();
}

__declspec(noinline) void __fastcall RoomChangeOwnerBridgeHarness(
    LoopAbiOwnerHarness* self, void*)
{
    self->RoomChangeOwner();
}

__declspec(noinline) void __fastcall RoomChangeSimulationBridgeHarness(
    LoopAbiOwnerHarness* self, void*)
{
    self->RoomChangeSimulation();
}

using VoidThiscallBridgeHarness = void(__fastcall*)(LoopAbiOwnerHarness*, void*);

struct VoidThiscallProbeResult {
    std::uintptr_t stack_before = 0;
    std::uintptr_t stack_after = 0;
    std::uintptr_t ebx_after = 0;
    std::uintptr_t esi_after = 0;
    std::uintptr_t edi_after = 0;
};

static_assert(sizeof(VoidThiscallProbeResult) == 20);

__declspec(naked) void __cdecl ProbeVoidThiscallBridge(VoidThiscallBridgeHarness,
                                                       LoopAbiOwnerHarness*,
                                                       VoidThiscallProbeResult*)
{
    __asm {
        push ebp
        mov ebp, esp
        push ebx
        push esi
        push edi
        mov eax, dword ptr [ebp + 16]
        mov dword ptr [eax], esp
        mov ebx, 13579BDFh
        mov esi, 2468ACE0h
        mov edi, 0A5A55A5Ah
        mov ecx, dword ptr [ebp + 12]
        xor edx, edx
        call dword ptr [ebp + 8]
        mov eax, dword ptr [ebp + 16]
        mov dword ptr [eax + 4], esp
        mov dword ptr [eax + 8], ebx
        mov dword ptr [eax + 12], esi
        mov dword ptr [eax + 16], edi
        pop edi
        pop esi
        pop ebx
        mov esp, ebp
        pop ebp
        ret
    }
}

struct LoopBootstrapThreadHarness {
    std::uint32_t main_thread_id = 0;
    std::uint64_t loop_iteration = 0;
    std::uint32_t unsafe_code = 0;
    std::uint32_t original_calls = 0;
    std::uint32_t phase_events = 0;
    bool iteration_zero_phase_event = false;

    bool EstablishOrCheck(std::uint32_t thread_id, bool establish, bool required)
    {
        if (main_thread_id == 0 && establish)
            main_thread_id = thread_id;
        if (main_thread_id == 0)
            return false;
        const bool matches = main_thread_id == thread_id;
        if (required && !matches)
            unsafe_code = 6;
        return matches;
    }

    void PublishPhase()
    {
        iteration_zero_phase_event |= loop_iteration == 0;
        ++phase_events;
    }

    void NonClock(std::uint32_t thread_id)
    {
        const bool observe = EstablishOrCheck(thread_id, false, true);
        ++original_calls;
        if (observe)
            PublishPhase();
    }

    void Message(std::uint32_t thread_id)
    {
        ++original_calls;
        if (EstablishOrCheck(thread_id, false, true))
            PublishPhase();
    }

    void Clock(std::uint32_t thread_id)
    {
        if (EstablishOrCheck(thread_id, true, true))
        {
            ++loop_iteration;
            PublishPhase();
        }
        ++original_calls;
    }
};

void __stdcall OriginalPostGameStart(std::uint32_t argument)
{
    ++g_calls;
    g_loop_abi_argument = argument;
}

void __stdcall PostGameStartBridgeHarness(std::uint32_t argument)
{
    using Function = void(__stdcall*)(std::uint32_t);
    Function original = &OriginalPostGameStart;
    original(argument);
}

void TestModeLatch()
{
    using namespace isaac_cs_t0::mode;
    ModeLatch online(RequestedMode::OriginalOnline);
    CHECK(online.BeginGeneration(7));
    SignalObservation manager{};
    manager.manager_identity = true;
    CHECK(online.Observe(manager).kind == TransitionKind::None);
    CHECK(online.Observe(manager).kind == TransitionKind::None);
    SignalObservation channel{};
    channel.channel_zero_activity = true;
    const auto frozen = online.Observe(channel);
    CHECK(frozen.kind == TransitionKind::Frozen);
    CHECK(frozen.mode == NetworkMode::OriginalOnline);
    SignalObservation conflict{};
    conflict.manager_changed = true;
    CHECK(online.Observe(conflict).kind == TransitionKind::Retired);
    CHECK(!online.BeginGeneration(7));
    CHECK(online.BeginGeneration(8));

    ModeLatch offline(RequestedMode::Offline);
    CHECK(offline.BeginGeneration(11));
    SignalObservation callback{};
    callback.lua_generation_bound = true;
    callback.simulation_callback = true;
    for (std::uint32_t index = 1; index < kOfflineCallbackThreshold; ++index)
        CHECK(offline.Observe(callback).kind == TransitionKind::None);
    CHECK(offline.Observe(callback).mode == NetworkMode::Offline);

    ModeLatch custom(RequestedMode::CustomAuthorityServer);
    CHECK(!custom.BeginGeneration(12));
    CHECK(custom.State() == NetworkMode::Retired);
}

void TestPassthroughCallsExactlyOnce()
{
    g_calls = 0;
    CHECK(PassthroughHarness(5) == 22);
    CHECK(g_calls == 1);
}

void TestPatchTransactionAndRestore()
{
    using namespace isaac_cs_t0::mode;
    alignas(16) std::array<std::uint8_t, 16> call{};
    call[0] = 0xE8;
    const auto next = reinterpret_cast<std::intptr_t>(call.data() + 5);
    const auto original = reinterpret_cast<std::intptr_t>(&OriginalValue);
    const auto displacement = static_cast<std::int32_t>(original - next);
    std::memcpy(call.data() + 1, &displacement, sizeof(displacement));
    RelativeCallPatch relative{};
    relative.call = call.data();
    std::memcpy(relative.expected, call.data(), 5);
    relative.replacement = reinterpret_cast<void*>(&ReplacementValue);
    void* pointer_slot = reinterpret_cast<void*>(&OriginalValue);
    PointerPatch pointer{&pointer_slot, reinterpret_cast<void*>(&ReplacementValue), nullptr,
                         nullptr, false};
    PatchTransactionFailure failure{};
    CHECK(InstallPatchTransaction(&relative, 1, &pointer, 1, &failure));
    CHECK(failure.stage == PatchFailureStage::None);
    CHECK(failure.index == UINT32_MAX);
    CHECK(failure.win32_error == ERROR_SUCCESS);
    CHECK(relative.installed && pointer.installed);
    CHECK(pointer_slot == reinterpret_cast<void*>(&ReplacementValue));
    CHECK(relative.original == reinterpret_cast<void*>(&OriginalValue));
    CHECK(RestorePatchTransaction(&relative, 1, &pointer, 1));
    CHECK(!relative.installed && !pointer.installed);
    CHECK(pointer_slot == reinterpret_cast<void*>(&OriginalValue));
    CHECK(std::memcmp(call.data(), relative.expected, 5) == 0);
}

void TestPatchTransactionDiagnosticsAreStableAndPrewrite()
{
    using namespace isaac_cs_t0::mode;
    static_assert(static_cast<std::uint32_t>(PatchFailureStage::InvalidArguments) == 1);
    static_assert(static_cast<std::uint32_t>(PatchFailureStage::ThreadOpen) == 6);
    static_assert(static_cast<std::uint32_t>(PatchFailureStage::ApplyPointer) == 10);
    static_assert(static_cast<std::uint32_t>(PatchFailureStage::RollbackRelative) == 12);

    PatchTransactionFailure failure{};
    CHECK(!InstallPatchTransaction(nullptr, 1, nullptr, 0, &failure));
    CHECK(failure.stage == PatchFailureStage::InvalidArguments);
    CHECK(failure.index == UINT32_MAX);
    CHECK(failure.win32_error == ERROR_INVALID_PARAMETER);

    std::array<std::uint8_t, 8> call{};
    RelativeCallPatch relative{};
    relative.call = call.data();
    relative.expected[0] = 0xE8;
    relative.replacement = reinterpret_cast<void*>(&ReplacementValue);
    failure = {};
    CHECK(!InstallPatchTransaction(&relative, 1, nullptr, 0, &failure));
    CHECK(failure.stage == PatchFailureStage::PrepareRelative);
    CHECK(failure.index == 0);
    CHECK(failure.win32_error == ERROR_INVALID_DATA);
    CHECK(!relative.installed);
    CHECK((call == std::array<std::uint8_t, 8>{}));

    void* original = reinterpret_cast<void*>(&OriginalValue);
    void* pointer_slot = original;
    PointerPatch pointer{&pointer_slot, nullptr, nullptr, nullptr, false};
    failure = {};
    CHECK(!InstallPatchTransaction(nullptr, 0, &pointer, 1, &failure));
    CHECK(failure.stage == PatchFailureStage::PreparePointer);
    CHECK(failure.index == 0);
    CHECK(failure.win32_error == ERROR_INVALID_DATA);
    CHECK(!pointer.installed);
    CHECK(pointer_slot == original);
}

void TestPatchTransactionApplyFailureRollsBack()
{
    using namespace isaac_cs_t0::mode;

    alignas(16) std::array<std::uint8_t, 16> call{};
    call[0] = 0xE8;
    const auto next = reinterpret_cast<std::intptr_t>(call.data() + 5);
    const auto original = reinterpret_cast<std::intptr_t>(&OriginalValue);
    const auto displacement = static_cast<std::int32_t>(original - next);
    std::memcpy(call.data() + 1, &displacement, sizeof(displacement));
    const auto original_call = call;

    RelativeCallPatch overlapping_relatives[2]{};
    for (auto& relative : overlapping_relatives)
    {
        relative.call = call.data();
        std::memcpy(relative.expected, call.data(), 5);
        relative.replacement = reinterpret_cast<void*>(&ReplacementValue);
    }

    PatchTransactionFailure failure{};
    CHECK(!InstallPatchTransaction(overlapping_relatives, 2, nullptr, 0, &failure));
    CHECK(failure.stage == PatchFailureStage::ApplyRelative);
    CHECK(failure.index == 1);
    CHECK(failure.win32_error == ERROR_INVALID_DATA);
    CHECK(!overlapping_relatives[0].installed);
    CHECK(!overlapping_relatives[1].installed);
    CHECK(call == original_call);

    RelativeCallPatch relative{};
    relative.call = call.data();
    std::memcpy(relative.expected, call.data(), 5);
    relative.replacement = reinterpret_cast<void*>(&ReplacementValue);
    void* pointer_slot = reinterpret_cast<void*>(&OriginalValue);
    PointerPatch overlapping_pointers[2]{
        {&pointer_slot, reinterpret_cast<void*>(&ReplacementValue), nullptr, nullptr, false},
        {&pointer_slot, reinterpret_cast<void*>(&ReplacementValue), nullptr, nullptr, false},
    };

    failure = {};
    CHECK(!InstallPatchTransaction(&relative, 1, overlapping_pointers, 2, &failure));
    CHECK(failure.stage == PatchFailureStage::ApplyPointer);
    CHECK(failure.index == 1);
    CHECK(failure.win32_error == ERROR_INVALID_DATA);
    CHECK(!relative.installed);
    CHECK(!overlapping_pointers[0].installed);
    CHECK(!overlapping_pointers[1].installed);
    CHECK(call == original_call);
    CHECK(pointer_slot == reinterpret_cast<void*>(&OriginalValue));
}

void TestOfflineAttestationSixSlotTransactionIsAtomicAndRestoresInOrder()
{
    using isaac_cs_t0::mode::InstallPatchTransaction;
    using isaac_cs_t0::mode::PatchFailureStage;
    using isaac_cs_t0::mode::PatchTransactionFailure;
    using isaac_cs_t0::mode::PointerPatch;
    using isaac_cs_t0::mode::RestorePatchTransaction;

    static_assert(isaac_cs_t0::offline_attestation::kActiveRunHookMask == 0x3Full);
    static_assert(isaac_cs_t0::offline_attestation::kLoadingMenuHookMask == 0x7Full);
    static_assert(isaac_cs_t0::offline_attestation::kActiveRunSlotCount == 6);
    static_assert(isaac_cs_t0::offline_attestation::kLoadingMenuSlotCount == 7);
    static_assert(sizeof(isaac_cs_t0::offline_attestation::NativeEvent) == 128);
    static_assert(sizeof(isaac_cs_t0::offline_attestation::SharedBlock) == 524648);
    static_assert(offsetof(isaac_cs_t0::offline_attestation::SharedBlock, events) == 360);
    static_assert(sizeof(isaac_cs_t0::active_abi::NativeActiveEvent) == 288);

    using isaac_cs_t0::offline_attestation::ApprovedIdentity;
    using isaac_cs_t0::offline_attestation::kCanonicalTargetSha256;
    using isaac_cs_t0::offline_attestation::kObservedUnpackedTargetSha256;
    using isaac_cs_t0::offline_attestation::kTargetEquivalenceSha256;
    CHECK(ApprovedIdentity(kCanonicalTargetSha256, kObservedUnpackedTargetSha256,
                           kTargetEquivalenceSha256));
    auto wrong_canonical = kCanonicalTargetSha256;
    auto wrong_observed = kObservedUnpackedTargetSha256;
    auto wrong_equivalence = kTargetEquivalenceSha256;
    wrong_canonical[0] ^= 1;
    wrong_observed[1] ^= 1;
    wrong_equivalence[2] ^= 1;
    CHECK(!ApprovedIdentity(wrong_canonical, kObservedUnpackedTargetSha256,
                            kTargetEquivalenceSha256));
    CHECK(!ApprovedIdentity(kCanonicalTargetSha256, wrong_observed,
                            kTargetEquivalenceSha256));
    CHECK(!ApprovedIdentity(kCanonicalTargetSha256, kObservedUnpackedTargetSha256,
                            wrong_equivalence));

    using isaac_cs_t0::offline_attestation::ManagerActivitySignal;
    using isaac_cs_t0::offline_attestation::ManagerNegativeSample;
    using isaac_cs_t0::offline_attestation::ObservationWindowOpen;
    using isaac_cs_t0::offline_attestation::ObservationPolicy;
    using isaac_cs_t0::offline_attestation::RosterSignal;
    using isaac_cs_t0::offline_attestation::RunStateActive;
    using isaac_cs_t0::offline_attestation::RunStateAllowed;
    using isaac_cs_t0::offline_attestation::RunStateSample;
    using isaac_cs_t0::offline_attestation::SameManagerIdentity;
    using isaac_cs_t0::offline_attestation::ValidObservationPolicy;
    using isaac_cs_t0::offline_attestation::ExpectedHookMask;
    using isaac_cs_t0::offline_attestation::ExpectedSlotCount;
    using isaac_cs_t0::offline_attestation::RelocatedImageAddressMatches;
    CHECK(ExpectedHookMask(ObservationPolicy::ActiveRun) == 0x3Fu);
    CHECK(ExpectedHookMask(ObservationPolicy::LoadingMenuInactive) == 0x7Fu);
    CHECK(ExpectedSlotCount(ObservationPolicy::ActiveRun) == 6);
    CHECK(ExpectedSlotCount(ObservationPolicy::LoadingMenuInactive) == 7);
    CHECK(RelocatedImageAddressMatches(0x00400000u, 0x00BF93B4u, 0x007F93B4u));
    CHECK(RelocatedImageAddressMatches(0x00400000u, 0x00B18714u, 0x00718714u));
    CHECK(RelocatedImageAddressMatches(0x00860000u, 0x010593B4u, 0x007F93B4u));
    CHECK(RelocatedImageAddressMatches(0x00860000u, 0x00F78714u, 0x00718714u));
    CHECK(!RelocatedImageAddressMatches(0x00860000u, 0x00B18714u, 0x00718714u));
    const ManagerNegativeSample dormant_local{0x1000u, 0, 0, 0, 0, 0, true};
    CHECK(!ManagerActivitySignal(dormant_local));
    CHECK(!RosterSignal(dormant_local));
    CHECK(SameManagerIdentity(dormant_local, dormant_local));
    auto active_backend = dormant_local;
    active_backend.network_backend = 0x2000u;
    CHECK(ManagerActivitySignal(active_backend));
    auto active_frame = dormant_local;
    active_frame.online_frame = 1;
    CHECK(ManagerActivitySignal(active_frame));
    auto nonempty_roster = dormant_local;
    nonempty_roster.roster_first = 0x3000u;
    CHECK(RosterSignal(nonempty_roster));
    auto unreadable = dormant_local;
    unreadable.readable = false;
    CHECK(ManagerActivitySignal(unreadable));
    CHECK(RosterSignal(unreadable));
    auto changed_manager = dormant_local;
    changed_manager.manager += 4;
    CHECK(!SameManagerIdentity(dormant_local, changed_manager));
    CHECK(!ObservationWindowOpen(false, false));
    CHECK(!ObservationWindowOpen(false, true));
    CHECK(!ObservationWindowOpen(true, true));
    CHECK(ObservationWindowOpen(true, false));
    CHECK(ValidObservationPolicy(0));
    CHECK(ValidObservationPolicy(1));
    CHECK(!ValidObservationPolicy(2));
    const RunStateSample active{true, true, 1};
    const RunStateSample inactive{true, true, 0};
    const RunStateSample high_byte_only{true, true, 0x0100};
    const RunStateSample no_game{false, false, 0};
    const RunStateSample unreadable_run{true, false, 0};
    CHECK(RunStateAllowed(ObservationPolicy::ActiveRun, active));
    CHECK(!RunStateAllowed(ObservationPolicy::ActiveRun, inactive));
    CHECK(!RunStateAllowed(ObservationPolicy::LoadingMenuInactive, active));
    CHECK(RunStateAllowed(ObservationPolicy::LoadingMenuInactive, inactive));
    CHECK(!RunStateActive(high_byte_only));
    CHECK(RunStateAllowed(ObservationPolicy::LoadingMenuInactive,
                          high_byte_only));
    CHECK(!RunStateAllowed(ObservationPolicy::ActiveRun, no_game));
    CHECK(!RunStateAllowed(ObservationPolicy::LoadingMenuInactive, no_game));
    CHECK(!RunStateAllowed(ObservationPolicy::ActiveRun, unreadable_run));
    CHECK(!RunStateAllowed(ObservationPolicy::LoadingMenuInactive,
                           unreadable_run));

    const auto original = reinterpret_cast<void*>(&OriginalValue);
    const auto replacement = reinterpret_cast<void*>(&ReplacementValue);
    std::array<void*, isaac_cs_t0::offline_attestation::kActiveRunSlotCount> slots{};
    slots.fill(original);

    std::array<PointerPatch, isaac_cs_t0::offline_attestation::kActiveRunSlotCount> overlapping{};
    for (std::size_t index = 0; index < overlapping.size(); ++index)
        overlapping[index] = {&slots[index], replacement, nullptr, nullptr, false};
    overlapping.back().slot = &slots[overlapping.size() - 2];

    PatchTransactionFailure failure{};
    CHECK(!InstallPatchTransaction(nullptr, 0, overlapping.data(), overlapping.size(), &failure));
    CHECK(failure.stage == PatchFailureStage::ApplyPointer);
    CHECK(failure.index == overlapping.size() - 1);
    for (std::size_t index = 0; index < slots.size(); ++index)
    {
        CHECK(slots[index] == original);
        CHECK(!overlapping[index].installed);
    }

    std::array<PointerPatch, isaac_cs_t0::offline_attestation::kActiveRunSlotCount> patches{};
    for (std::size_t index = 0; index < patches.size(); ++index)
        patches[index] = {&slots[index], replacement, nullptr, nullptr, false};
    failure = {};
    CHECK(InstallPatchTransaction(nullptr, 0, patches.data(), patches.size(), &failure));
    CHECK(failure.stage == PatchFailureStage::None);
    for (const auto slot : slots)
        CHECK(slot == replacement);

    // SteamNetworking006 vtable slots are the dynamic tail and must restore first.
    CHECK(RestorePatchTransaction(nullptr, 0, patches.data() + 3, 3));
    for (std::size_t index = 0; index < slots.size(); ++index)
        CHECK(slots[index] == (index < 3 ? replacement : original));

    // The three fixed IAT slots restore only after the dynamic slots have drained.
    CHECK(RestorePatchTransaction(nullptr, 0, patches.data(), 3));
    for (std::size_t index = 0; index < slots.size(); ++index)
    {
        CHECK(slots[index] == original);
        CHECK(!patches[index].installed);
    }

    // Loading-at-menu owns one additive PeekMessageW IAT slot ahead of the same Steam tail.
    std::array<void*, isaac_cs_t0::offline_attestation::kLoadingMenuSlotCount> menu_slots{};
    menu_slots.fill(original);
    std::array<PointerPatch, isaac_cs_t0::offline_attestation::kLoadingMenuSlotCount>
        menu_patches{};
    for (std::size_t index = 0; index < menu_patches.size(); ++index)
        menu_patches[index] = {&menu_slots[index], replacement, nullptr, nullptr, false};
    CHECK(InstallPatchTransaction(nullptr, 0, menu_patches.data(), menu_patches.size(),
                                  &failure));
    CHECK(RestorePatchTransaction(nullptr, 0, menu_patches.data() + 4, 3));
    for (std::size_t index = 0; index < menu_slots.size(); ++index)
        CHECK(menu_slots[index] == (index < 4 ? replacement : original));
    CHECK(RestorePatchTransaction(nullptr, 0, menu_patches.data(), 4));
    for (std::size_t index = 0; index < menu_slots.size(); ++index)
    {
        CHECK(menu_slots[index] == original);
        CHECK(!menu_patches[index].installed);
    }
}

void TestSha256()
{
    using namespace isaac_cs_t0::mode;
    constexpr char text[] = "abc";
    const auto digest = Sha256(text, 3);
    constexpr std::array<std::uint8_t, 32> expected{{
        0xBA, 0x78, 0x16, 0xBF, 0x8F, 0x01, 0xCF, 0xEA,
        0x41, 0x41, 0x40, 0xDE, 0x5D, 0xAE, 0x22, 0x23,
        0xB0, 0x03, 0x61, 0xA3, 0x96, 0x17, 0x7A, 0x9C,
        0xB4, 0x10, 0xFF, 0x61, 0xF2, 0x00, 0x15, 0xAD,
    }};
    CHECK(digest == expected);
    const auto pair = Sha256Pair(text, 1, text + 1, 2);
    CHECK(pair == expected);
}

void TestRingLayoutAndSequenceModel()
{
    using namespace isaac_cs_t0::mode;
    static_assert(sizeof(NativeEvent) == 168);
    static_assert(sizeof(SharedModeBlock) == 688296);
    auto* shared = new SharedModeBlock{};
    for (LONG64 sequence = 1; sequence <= 128; ++sequence)
    {
        auto& event = shared->events[static_cast<std::size_t>(sequence - 1) % kEventCapacity];
        event.published_sequence = sequence;
        shared->next_sequence = sequence;
        CHECK(event.published_sequence == sequence);
        shared->controller_sequence = sequence;
    }
    CHECK(shared->next_sequence == shared->controller_sequence);
    CHECK(shared->overwritten_events == 0);
    delete shared;
}

void TestBoundaryRingAndRecursiveCallModel()
{
    using namespace isaac_cs_t0::boundary;
    static_assert(kIpcVersion == 3);
    static_assert(sizeof(NativeEvent) == 104);
    static_assert(sizeof(SharedBoundaryBlock) == 426112);
    auto* shared = new SharedBoundaryBlock{};
    CHECK(shared->magic == 0);
    for (LONG64 sequence = 1; sequence <= 256; ++sequence)
    {
        auto& event = shared->events[static_cast<std::size_t>(sequence - 1) % kEventCapacity];
        event.depth = ++g_boundary_depth;
        event.original_calls = 1;
        event.published_sequence = sequence;
        shared->next_sequence = sequence;
        CHECK(event.depth == 1);
        CHECK(event.original_calls == 1);
        --g_boundary_depth;
        shared->controller_sequence = sequence;
    }
    CHECK(g_boundary_depth == 0);
    CHECK(shared->next_sequence == shared->controller_sequence);
    CHECK(shared->overwritten_events == 0);
    shared->next_sequence = static_cast<LONG64>(kEventCapacity) + 1;
    shared->controller_sequence = 0;
    if (shared->next_sequence - shared->controller_sequence >
        static_cast<LONG64>(kEventCapacity))
        ++shared->overwritten_events;
    CHECK(shared->overwritten_events == 1);
    delete shared;
}

void TestBoundaryEventPayloadCopyDoesNotPublishEarly()
{
    using namespace isaac_cs_t0::boundary;
    NativeEvent source{};
    source.published_sequence = 99;
    source.qpc = 1234567;
    source.generation = 42;
    source.kind = static_cast<std::uint32_t>(EventKind::GameplayEnter);
    source.depth = 3;
    source.return_rva = 0x00555236u;
    source.original_calls = 1;
    NativeEvent destination{};
    destination.published_sequence = 0;
    CopyEventPayload(destination, source);
    CHECK(destination.published_sequence == 0);
    CHECK(destination.qpc == source.qpc);
    CHECK(destination.generation == source.generation);
    CHECK(destination.kind == source.kind);
    CHECK(destination.depth == source.depth);
    CHECK(destination.return_rva == source.return_rva);
    CHECK(destination.original_calls == source.original_calls);
}

void TestBoundaryPassthroughContract()
{
    g_calls = 0;
    g_boundary_argument = 0;
    g_boundary_thread = 0;
    g_boundary_depth = 0;
    const DWORD caller_thread = GetCurrentThreadId();
    CHECK(BoundaryPassthroughHarness(0x1234) == (0x1234 ^ 0x55AA));
    CHECK(g_calls == 1);
    CHECK(g_boundary_argument == 0x1234);
    CHECK(g_boundary_thread == caller_thread);
    CHECK(g_boundary_depth == 0);
}

void TestLoopRingLayoutAndPayloadPublication()
{
    using namespace isaac_cs_t0::loop;
    static_assert(kIpcVersion == 5);
    static_assert(kRelativeHookCount == 27);
    static_assert(kPointerHookCount == 5);
    static_assert(kHookCount == 32);
    static_assert(sizeof(decltype(kCompleteHookMask)) == sizeof(std::uint64_t));
    static_assert(kCompleteHookMask == 0x00000000FFFFFFFFull);
    static_assert(static_cast<std::uint32_t>(EventKind::StandardRoomLifecycleOwnerEnter) == 59);
    static_assert(static_cast<std::uint32_t>(EventKind::StandardRoomLifecycleSimulationExit) ==
                  62);
    static_assert(static_cast<std::uint32_t>(EventKind::RoomChangeOwnerEnter) == 63);
    static_assert(static_cast<std::uint32_t>(EventKind::RoomChangeOwnerExit) == 64);
    static_assert(static_cast<std::uint32_t>(EventKind::RoomChangeSimulationEnter) == 65);
    static_assert(static_cast<std::uint32_t>(EventKind::RoomChangeSimulationExit) == 66);
    static_assert(kEventCapacity == 65536);
    static_assert(sizeof(NativeEvent) == 64);
    static_assert(sizeof(SharedLoopBlock) == 4194432);
    static_assert(offsetof(SharedLoopBlock, hooks_mask) == 72);
    static_assert(sizeof(SharedLoopBlock::hooks_mask) == sizeof(std::uint64_t));
    static_assert(offsetof(SharedLoopBlock, main_thread_id) == 80);
    static_assert(offsetof(SharedLoopBlock, current_loop_iteration) == 88);
    static_assert(offsetof(SharedLoopBlock, events) == 128);
    NativeEvent source{};
    source.published_sequence = 99;
    source.qpc = 123;
    source.loop_iteration = 7;
    source.kind = static_cast<std::uint32_t>(EventKind::InputScalar);
    source.phase = static_cast<std::uint32_t>(Phase::InputQuery);
    source.action = 6;
    source.value_bits = 0x3F800000u;
    source.device_key = 1;
    NativeEvent destination{};
    CopyEventPayload(destination, source);
    CHECK(destination.published_sequence == 0);
    CHECK(destination.qpc == 123);
    CHECK(destination.loop_iteration == 7);
    CHECK(destination.kind == static_cast<std::uint32_t>(EventKind::InputScalar));
    CHECK(destination.phase == static_cast<std::uint32_t>(Phase::InputQuery));
    CHECK(destination.action == 6);
    CHECK(destination.value_bits == 0x3F800000u);
    CHECK(destination.device_key == 1);
}

void TestInputSchemaTwoRingLayoutAndPayloadPublication()
{
    using namespace isaac_cs_t0::input;
    static_assert(kInputIpcSchemaVersion == 2);
    static_assert(kInputEventCapacity == 65536);
    static_assert(kInputHookCount == 11);
    static_assert(kInputHookMask == 0x7FFull);
    static_assert(sizeof(NativeInputEvent) == 80);
    static_assert(sizeof(SharedInputBlock) == 5243352);
    static_assert(offsetof(SharedInputBlock, installed_hooks_mask) == 392);
    static_assert(offsetof(SharedInputBlock, redacted_events) == 456);
    static_assert(offsetof(SharedInputBlock, events) == 472);
    static_assert(static_cast<std::uint32_t>(InputEventKind::ManagerScalarExit) == 14);
    static_assert(static_cast<std::uint32_t>(InputEventKind::Failure) == 19);
    static_assert(static_cast<std::uint32_t>(InputClassification::UniqueGameplay) == 1);
    static_assert(static_cast<std::uint32_t>(InputClassification::UnstableSnapshot) == 5);
    static_assert(static_cast<std::uint32_t>(InputClassification::ExplicitGameplay) == 6);

    NativeInputEvent source{};
    source.published_sequence = 99;
    source.qpc = 1234;
    source.loop_iteration = 42;
    source.kind = static_cast<std::uint32_t>(InputEventKind::ManagerScalarExit);
    source.phase = static_cast<std::uint32_t>(isaac_cs_t0::loop::Phase::InputQuery);
    source.flags = kInputFlagLoopKnown | kInputFlagMainThread |
                   kInputFlagOriginalCalled | kInputFlagValueValid;
    source.controller_id = -1;
    source.selected_device_key = 0;
    source.action = 4;
    source.primitive = 0x107;
    source.value_bits = 0x3F800000u;
    source.classification =
        static_cast<std::uint32_t>(InputClassification::UniqueGameplay);
    NativeInputEvent destination{};
    CopyInputEventPayload(destination, source);
    CHECK(destination.published_sequence == 0);
    CHECK(destination.qpc == source.qpc);
    CHECK(destination.loop_iteration == source.loop_iteration);
    CHECK(destination.kind == source.kind);
    CHECK(destination.controller_id == -1);
    CHECK(destination.action == 4);
    CHECK(destination.primitive == 0x107);
    CHECK(destination.value_bits == 0x3F800000u);
    CHECK(destination.classification == source.classification);
}

void TestInputX86AbiBridgesPreserveArgumentsStackReturnsAndOriginalOnce()
{
    static_assert(sizeof(void*) == 4);
    InputAbiOwnerHarness owner{};
    std::uintptr_t stack_before = 0;
    std::uintptr_t stack_after = 0;

    owner.marker = 0x13579BDFu ^ 0x2468ACE0u;
    g_calls = 0;
    g_input_abi_owner = nullptr;
    g_input_abi_argument = 0;
    __asm mov stack_before, esp
    const bool logical_bool = InputLogicalBoolBridgeHarness(&owner, nullptr, 0x2468ACE0u);
    __asm mov stack_after, esp
    CHECK(logical_bool);
    CHECK(g_calls == 1);
    CHECK(g_input_abi_owner == &owner);
    CHECK(g_input_abi_argument == 0x2468ACE0u);
    CHECK(stack_before == stack_after);

    g_calls = 0;
    g_input_abi_owner = nullptr;
    g_input_abi_argument = 0;
    __asm mov stack_before, esp
    const float logical_scalar = InputLogicalScalarBridgeHarness(&owner, nullptr, 6);
    __asm mov stack_after, esp
    CHECK(logical_scalar == 6.375F);
    CHECK(g_calls == 1);
    CHECK(g_input_abi_owner == &owner);
    CHECK(g_input_abi_argument == 6);
    CHECK(stack_before == stack_after);

    std::int32_t selected = -1;
    g_calls = 0;
    g_input_abi_owner = nullptr;
    g_input_abi_argument = 0;
    g_input_abi_controller = 0;
    g_input_abi_selected = nullptr;
    __asm mov stack_before, esp
    const bool manager_bool =
        InputManagerBoolBridgeHarness(&owner, nullptr, 0x41, -1, &selected);
    __asm mov stack_after, esp
    CHECK(manager_bool);
    CHECK(selected == 7);
    CHECK(g_calls == 1);
    CHECK(g_input_abi_owner == &owner);
    CHECK(g_input_abi_argument == 0x41);
    CHECK(g_input_abi_controller == -1);
    CHECK(g_input_abi_selected == &selected);
    CHECK(stack_before == stack_after);

    selected = -1;
    g_calls = 0;
    g_input_abi_owner = nullptr;
    g_input_abi_argument = 0;
    g_input_abi_controller = 0;
    g_input_abi_selected = nullptr;
    __asm mov stack_before, esp
    const float manager_scalar =
        InputManagerScalarBridgeHarness(&owner, nullptr, 8, 3, &selected);
    __asm mov stack_after, esp
    CHECK(manager_scalar == 5.0F);
    CHECK(selected == 9);
    CHECK(g_calls == 1);
    CHECK(g_input_abi_owner == &owner);
    CHECK(g_input_abi_argument == 8);
    CHECK(g_input_abi_controller == 3);
    CHECK(g_input_abi_selected == &selected);
    CHECK(stack_before == stack_after);

    g_calls = 0;
    g_input_abi_owner = nullptr;
    g_input_abi_argument = 0;
    g_input_abi_value = 0.0F;
    __asm mov stack_before, esp
    InputLogicalSetterBridgeHarness(&owner, nullptr, 11, 0.625F);
    __asm mov stack_after, esp
    CHECK(g_calls == 1);
    CHECK(g_input_abi_owner == &owner);
    CHECK(g_input_abi_argument == 11);
    CHECK(g_input_abi_value == 0.625F);
    CHECK(stack_before == stack_after);
}

void TestCombinedFortyThreePointerTransactionRollbackAndRestore()
{
    using namespace isaac_cs_t0::mode;
    constexpr std::size_t hook_count = 43;
    constexpr std::size_t first_input_hook = 32;
    static_assert(hook_count == 32 + isaac_cs_t0::input::kInputHookCount);

    for (std::size_t failing = first_input_hook; failing < hook_count; ++failing)
    {
        std::array<void*, hook_count> slots{};
        slots.fill(reinterpret_cast<void*>(&OriginalValue));
        std::array<PointerPatch, hook_count> patches{};
        for (std::size_t index = 0; index < hook_count; ++index)
        {
            patches[index] = {&slots[index], reinterpret_cast<void*>(&ReplacementValue),
                              nullptr, nullptr, false};
        }
        // Preparation observes the original value in every slot.  During apply, this alias
        // fails only after the first 32 V5 slots and any preceding input slots were written.
        patches[failing].slot = &slots[0];
        PatchTransactionFailure failure{};
        CHECK(!InstallPatchTransaction(nullptr, 0, patches.data(), patches.size(), &failure));
        CHECK(failure.stage == PatchFailureStage::ApplyPointer);
        CHECK(failure.index == failing);
        CHECK(failure.win32_error == ERROR_INVALID_DATA);
        for (std::size_t index = 0; index < hook_count; ++index)
        {
            CHECK(slots[index] == reinterpret_cast<void*>(&OriginalValue));
            CHECK(!patches[index].installed);
        }
    }

    std::array<void*, hook_count> slots{};
    slots.fill(reinterpret_cast<void*>(&OriginalValue));
    std::array<PointerPatch, hook_count> patches{};
    for (std::size_t index = 0; index < hook_count; ++index)
    {
        patches[index] = {&slots[index], reinterpret_cast<void*>(&ReplacementValue),
                          nullptr, nullptr, false};
    }
    PatchTransactionFailure failure{};
    CHECK(InstallPatchTransaction(nullptr, 0, patches.data(), patches.size(), &failure));
    CHECK(failure.stage == PatchFailureStage::None);
    for (std::size_t index = 0; index < hook_count; ++index)
    {
        CHECK(slots[index] == reinterpret_cast<void*>(&ReplacementValue));
        CHECK(patches[index].installed);
    }
    // Approved embedded shutdown order: restore only the 11 input slots first while all 32 V5
    // slots remain installed, then restore V5 after the input wrapper drain.
    CHECK(RestorePatchTransaction(nullptr, 0, patches.data() + first_input_hook,
                                  hook_count - first_input_hook));
    for (std::size_t index = 0; index < first_input_hook; ++index)
    {
        CHECK(slots[index] == reinterpret_cast<void*>(&ReplacementValue));
        CHECK(patches[index].installed);
    }
    for (std::size_t index = first_input_hook; index < hook_count; ++index)
    {
        CHECK(slots[index] == reinterpret_cast<void*>(&OriginalValue));
        CHECK(!patches[index].installed);
    }
    CHECK(RestorePatchTransaction(nullptr, 0, patches.data(), first_input_hook));
    for (std::size_t index = 0; index < hook_count; ++index)
    {
        CHECK(slots[index] == reinterpret_cast<void*>(&OriginalValue));
        CHECK(!patches[index].installed);
    }
}

struct EmbeddedShutdownPatchHarness {
    static constexpr std::size_t kV5Count = 32;
    static constexpr std::size_t kInputCount = isaac_cs_t0::input::kInputHookCount;
    static constexpr std::size_t kTotalCount = kV5Count + kInputCount;

    std::array<void*, kTotalCount> slots{};
    std::array<isaac_cs_t0::mode::PointerPatch, kTotalCount> patches{};

    EmbeddedShutdownPatchHarness()
    {
        slots.fill(reinterpret_cast<void*>(&OriginalValue));
        for (std::size_t index = 0; index < patches.size(); ++index)
        {
            patches[index] = {&slots[index], reinterpret_cast<void*>(&ReplacementValue),
                              nullptr, nullptr, false};
        }
    }

    bool Install()
    {
        return isaac_cs_t0::mode::InstallPatchTransaction(
            nullptr, 0, patches.data(), patches.size());
    }

    bool RestoreInput()
    {
        return isaac_cs_t0::mode::RestorePatchTransaction(
            nullptr, 0, patches.data() + kV5Count, kInputCount);
    }

    bool RestoreV5()
    {
        return isaac_cs_t0::mode::RestorePatchTransaction(
            nullptr, 0, patches.data(), kV5Count);
    }
};

void TestEmbeddedShutdownFailStopsAndNeverUnloadsEarly()
{
    using isaac_cs_t0::input::detail::EmbeddedShutdownResult;
    using isaac_cs_t0::input::detail::RunEmbeddedShutdownStages;

    // A mid-input restore ownership mismatch may leave a subset restored.  It must not start
    // the input drain, restore V5, or reach the caller's unload edge.
    {
        EmbeddedShutdownPatchHarness patches{};
        CHECK(patches.Install());
        constexpr std::size_t corrupted = EmbeddedShutdownPatchHarness::kV5Count + 5;
        patches.slots[corrupted] = reinterpret_cast<void*>(0x12345678u);
        std::array<std::uint32_t, 3> calls{};
        const auto result = RunEmbeddedShutdownStages(
            [&]() {
                ++calls[0];
                return patches.RestoreInput();
            },
            [&]() {
                ++calls[1];
                return true;
            },
            [&]() {
                ++calls[2];
                return patches.RestoreV5();
            });
        const bool unloaded = result == EmbeddedShutdownResult::Complete;
        CHECK(result == EmbeddedShutdownResult::InputRestoreFailed);
        CHECK((calls == std::array<std::uint32_t, 3>{1, 0, 0}));
        CHECK(!unloaded);
        for (std::size_t index = 0; index < EmbeddedShutdownPatchHarness::kV5Count; ++index)
        {
            CHECK(patches.slots[index] == reinterpret_cast<void*>(&ReplacementValue));
            CHECK(patches.patches[index].installed);
        }
        CHECK(patches.patches[corrupted].installed);
        for (std::size_t index = EmbeddedShutdownPatchHarness::kV5Count;
             index < EmbeddedShutdownPatchHarness::kTotalCount; ++index)
        {
            if (index == corrupted)
                continue;
            CHECK(patches.slots[index] == reinterpret_cast<void*>(&OriginalValue));
            CHECK(!patches.patches[index].installed);
        }

        // Repair only the synthetic corruption so the test process itself leaves no patches.
        patches.slots[corrupted] = reinterpret_cast<void*>(&ReplacementValue);
        CHECK(patches.RestoreInput());
        CHECK(patches.RestoreV5());
    }

    // A drain timeout occurs after all input slots are restored.  V5 must remain installed and
    // the unified unload edge must remain unreachable.
    {
        EmbeddedShutdownPatchHarness patches{};
        CHECK(patches.Install());
        std::array<std::uint32_t, 3> calls{};
        const auto result = RunEmbeddedShutdownStages(
            [&]() {
                ++calls[0];
                return patches.RestoreInput();
            },
            [&]() {
                ++calls[1];
                return false;
            },
            [&]() {
                ++calls[2];
                return patches.RestoreV5();
            });
        const bool unloaded = result == EmbeddedShutdownResult::Complete;
        CHECK(result == EmbeddedShutdownResult::InputDrainFailed);
        CHECK((calls == std::array<std::uint32_t, 3>{1, 1, 0}));
        CHECK(!unloaded);
        for (std::size_t index = 0; index < EmbeddedShutdownPatchHarness::kV5Count; ++index)
        {
            CHECK(patches.slots[index] == reinterpret_cast<void*>(&ReplacementValue));
            CHECK(patches.patches[index].installed);
        }
        for (std::size_t index = EmbeddedShutdownPatchHarness::kV5Count;
             index < EmbeddedShutdownPatchHarness::kTotalCount; ++index)
        {
            CHECK(patches.slots[index] == reinterpret_cast<void*>(&OriginalValue));
            CHECK(!patches.patches[index].installed);
        }
        CHECK(patches.RestoreV5());
    }

    // A V5 restore failure happens only after input restoration/drain, but it is still terminal
    // and cannot reach the unified unload edge.
    {
        EmbeddedShutdownPatchHarness patches{};
        CHECK(patches.Install());
        constexpr std::size_t corrupted_v5 = 9;
        patches.slots[corrupted_v5] = reinterpret_cast<void*>(0x12345678u);
        std::array<std::uint32_t, 3> calls{};
        const auto result = RunEmbeddedShutdownStages(
            [&]() {
                ++calls[0];
                return patches.RestoreInput();
            },
            [&]() {
                ++calls[1];
                return true;
            },
            [&]() {
                ++calls[2];
                return patches.RestoreV5();
            });
        const bool unloaded = result == EmbeddedShutdownResult::Complete;
        CHECK(result == EmbeddedShutdownResult::LoopStopFailed);
        CHECK((calls == std::array<std::uint32_t, 3>{1, 1, 1}));
        CHECK(!unloaded);
        CHECK(patches.patches[corrupted_v5].installed);
        for (std::size_t index = EmbeddedShutdownPatchHarness::kV5Count;
             index < EmbeddedShutdownPatchHarness::kTotalCount; ++index)
        {
            CHECK(patches.slots[index] == reinterpret_cast<void*>(&OriginalValue));
            CHECK(!patches.patches[index].installed);
        }

        patches.slots[corrupted_v5] = reinterpret_cast<void*>(&ReplacementValue);
        CHECK(patches.RestoreV5());
    }

    // The success edge is strictly restore-input -> drain-input -> restore/drain-V5 -> unload.
    {
        EmbeddedShutdownPatchHarness patches{};
        CHECK(patches.Install());
        std::array<std::uint32_t, 3> order{};
        std::size_t next = 0;
        const auto result = RunEmbeddedShutdownStages(
            [&]() {
                order[next++] = 1;
                return patches.RestoreInput();
            },
            [&]() {
                order[next++] = 2;
                return true;
            },
            [&]() {
                order[next++] = 3;
                return patches.RestoreV5();
            });
        const bool unloaded = result == EmbeddedShutdownResult::Complete;
        CHECK(result == EmbeddedShutdownResult::Complete);
        CHECK((order == std::array<std::uint32_t, 3>{1, 2, 3}));
        CHECK(unloaded);
        for (std::size_t index = 0; index < EmbeddedShutdownPatchHarness::kTotalCount; ++index)
        {
            CHECK(patches.slots[index] == reinterpret_cast<void*>(&OriginalValue));
            CHECK(!patches.patches[index].installed);
        }
    }
}

struct InputObservationPolicyHarness {
    bool armed = true;
    std::uint32_t main_thread_id = 0;
    std::uint64_t loop_iteration = 0;
    std::uint32_t original_calls = 0;
    std::uint32_t published = 0;
    std::uint32_t redacted = 0;
    bool unsafe = false;

    void Observe(std::uint32_t thread_id, isaac_cs_t0::loop::Phase phase,
                 std::uint32_t return_rva = 1)
    {
        ++original_calls;
        if (!armed || main_thread_id == 0 || loop_iteration == 0)
            return;
        if (thread_id != main_thread_id)
        {
            unsafe = true;
            armed = false;
            return;
        }
        if (phase == isaac_cs_t0::loop::Phase::OutsideEnvelope)
        {
            ++redacted;
            return;
        }
        if (return_rva == 0)
        {
            ++redacted;
            return;
        }
        ++published;
    }
};

void TestInputBootstrapOutsideAndWrongThreadPolicy()
{
    InputObservationPolicyHarness harness{};
    harness.Observe(7, isaac_cs_t0::loop::Phase::InputQuery);
    CHECK(harness.original_calls == 1);
    CHECK(harness.published == 0);
    CHECK(harness.redacted == 0);
    CHECK(!harness.unsafe);

    harness.main_thread_id = 7;
    harness.loop_iteration = 1;
    harness.Observe(7, isaac_cs_t0::loop::Phase::OutsideEnvelope);
    CHECK(harness.original_calls == 2);
    CHECK(harness.published == 0);
    CHECK(harness.redacted == 1);
    CHECK(!harness.unsafe);

    harness.Observe(7, isaac_cs_t0::loop::Phase::InputQuery);
    CHECK(harness.original_calls == 3);
    CHECK(harness.published == 1);
    CHECK(harness.redacted == 1);
    CHECK(!harness.unsafe);

    harness.Observe(7, isaac_cs_t0::loop::Phase::InputQuery, 0);
    CHECK(harness.original_calls == 4);
    CHECK(harness.published == 1);
    CHECK(harness.redacted == 2);
    CHECK(!harness.unsafe);

    harness.Observe(8, isaac_cs_t0::loop::Phase::InputQuery);
    CHECK(harness.original_calls == 5);
    CHECK(harness.published == 1);
    CHECK(harness.unsafe);
    CHECK(!harness.armed);

    harness.Observe(7, isaac_cs_t0::loop::Phase::InputQuery);
    CHECK(harness.original_calls == 6);
    CHECK(harness.published == 1);
}

void TestInputReturnRvaIsStrictlyBoundedToMainImage()
{
    using isaac_cs_t0::input::detail::BoundedTargetRva;
    const auto* base = reinterpret_cast<const void*>(0x00400000u);
    constexpr std::uint32_t image_size = 0x0092C000u;
    CHECK(BoundedTargetRva(base, image_size, base) == 0);
    CHECK(BoundedTargetRva(base, image_size,
                           reinterpret_cast<const void*>(0x00400001u)) == 1);
    CHECK(BoundedTargetRva(base, image_size,
                           reinterpret_cast<const void*>(0x00D2BFFFu)) ==
          image_size - 1);
    CHECK(BoundedTargetRva(base, image_size,
                           reinterpret_cast<const void*>(0x00D2C000u)) == 0);
    CHECK(BoundedTargetRva(base, image_size,
                           reinterpret_cast<const void*>(0x003FFFFFu)) == 0);
    CHECK(BoundedTargetRva(base, image_size,
                           reinterpret_cast<const void*>(0x70000000u)) == 0);
    CHECK(BoundedTargetRva(nullptr, image_size,
                           reinterpret_cast<const void*>(0x00401000u)) == 0);
    CHECK(BoundedTargetRva(base, image_size, nullptr) == 0);
}

void TestRawApplySchemaThreeLayoutAndPayloadPublication()
{
    using namespace isaac_cs_t0::raw_apply;
    static_assert(kSchemaVersion == 3);
    static_assert(kLegacyInputHookMask == 0x7FFull);
    static_assert(kRawSourceMask == 0x3ull);
    static_assert(kConsumerHookMask == 0x7Full);
    static_assert(sizeof(NativeRawApplyEvent) == 128);
    static_assert(offsetof(NativeRawApplyEvent, thread_id) == 48);
    static_assert(offsetof(NativeRawApplyEvent, error_code) == 124);
    static_assert(offsetof(SharedRawApplyBlock, state) == 448);
    static_assert(offsetof(SharedRawApplyBlock, events) == 576);
    static_assert(sizeof(SharedRawApplyBlock) == 8389184);
    CHECK(kCatalogSha256[0] == 0xE2 && kCatalogSha256[31] == 0xD6);
    CHECK(kCapturePlanSha256[0] == 0x20 && kCapturePlanSha256[31] == 0x62);

    NativeRawApplyEvent source{};
    source.published_sequence = 91;
    source.qpc = 12;
    source.consumer_transaction = 44;
    source.kind = static_cast<std::uint32_t>(EventKind::PlayerConsumerExit);
    source.hook_site_rva = 0x385BC2;
    source.primitive = kNoValue;
    NativeRawApplyEvent destination{};
    destination.published_sequence = 7;
    CopyEventPayload(destination, source);
    CHECK(destination.published_sequence == 7);
    CHECK(destination.qpc == 12);
    CHECK(destination.consumer_transaction == 44);
    CHECK(destination.hook_site_rva == 0x385BC2);
    CHECK(destination.primitive == kNoValue);
}

struct RawConsumerAbiOwner {
    void Plain()
    {
        ++g_calls;
        g_input_abi_owner = this;
    }
    void Item(std::int32_t slot)
    {
        ++g_calls;
        g_input_abi_owner = this;
        g_raw_item_slot = slot;
    }
};

extern "C" void __fastcall RawPlainConsumerBridge(RawConsumerAbiOwner* self, void*)
{
    self->Plain();
}

extern "C" void __fastcall RawItemConsumerBridge(RawConsumerAbiOwner* self, void*,
                                                   std::int32_t slot)
{
    self->Item(slot);
}

void TestRawConsumerX86AbiAndOriginalOnce()
{
    RawConsumerAbiOwner owner{};
    std::uintptr_t stack_before = 0;
    std::uintptr_t stack_after = 0;
    g_calls = 0;
    g_input_abi_owner = nullptr;
    __asm mov stack_before, esp
    RawPlainConsumerBridge(&owner, nullptr);
    __asm mov stack_after, esp
    CHECK(g_calls == 1);
    CHECK(g_input_abi_owner == &owner);
    CHECK(stack_before == stack_after);

    g_calls = 0;
    g_input_abi_owner = nullptr;
    g_raw_item_slot = 0;
    __asm mov stack_before, esp
    RawItemConsumerBridge(&owner, nullptr, -7);
    __asm mov stack_after, esp
    CHECK(g_calls == 1);
    CHECK(g_input_abi_owner == &owner);
    CHECK(g_raw_item_slot == -7);
    CHECK(stack_before == stack_after);
}

void TestConcretePhysicalProjectionAndPrivacy()
{
    using namespace isaac_cs_t0;
    using namespace raw_apply::detail;
    std::array<input::PrimitiveBinding, input::kMaximumLogicalActionCount> bindings{};
    for (auto& binding : bindings)
        binding.fill(input::kUnboundPrimitive);
    bindings[0][0] = 0x41;
    bindings[1][0] = 0x42;
    std::array<std::uint8_t, kKeyboardPrimitiveCount> keyboard{};
    keyboard[0x41] = 1;
    std::array<float, input::kGameplayActionCount> output{};
    std::uint16_t valid = 0;
    CHECK(ProjectKeyboard(bindings.data(), bindings.size(), true, keyboard, output,
                          valid) == ProjectionStatus::Ok);
    CHECK(valid == 0x0FFFu);
    CHECK(output[0] == 1.0F);
    CHECK(output[1] == 0.0F);

    // Cross-gameplay aliases redact only the affected actions.
    bindings[1][0] = 0x41;
    CHECK(ProjectKeyboard(bindings.data(), bindings.size(), true, keyboard, output,
                          valid) == ProjectionStatus::Ok);
    CHECK((valid & 0x0003u) == 0);
    CHECK((valid & 0x0FFCu) == 0x0FFCu);

    // Projection starts from an explicit gameplay slot.  An outside-gameplay owner is
    // neither inspected nor persisted and therefore does not invalidate that slot.
    bindings[1][0] = 0x42;
    bindings[12][0] = 0x42;
    CHECK(ProjectKeyboard(bindings.data(), bindings.size(), true, keyboard, output,
                          valid) == ProjectionStatus::Ok);
    CHECK((valid & 0x0003u) == 0x0003u);
    const auto manager_outside =
        input::ClassifyPrimitiveAction(0x42, bindings.data(), bindings.size());
    CHECK(manager_outside.classification ==
          input::PrimitiveActionClass::AmbiguousOrOutsideGameplay);

    // The observed default-like shoot arrows alias menu actions20..23.  The projection
    // remains action-indexed and emits only 4..7, never the outside action or primitive.
    constexpr std::array<std::uint32_t, 4> shoot_primitives{{0x107, 0x106, 0x109,
                                                             0x108}};
    for (std::size_t index = 0; index < shoot_primitives.size(); ++index)
    {
        bindings[4 + index][0] = shoot_primitives[index];
        bindings[20 + index][0] = shoot_primitives[index];
        keyboard[shoot_primitives[index]] = 1;
    }
    constexpr std::array<std::uint32_t, 4> edge_primitives{{0x45, 0x20, 0x51, 0x155}};
    constexpr std::array<std::uint32_t, 4> edge_outside_actions{{25, 14, 24, 29}};
    for (std::size_t index = 0; index < edge_primitives.size(); ++index)
    {
        bindings[8 + index][0] = edge_primitives[index];
        bindings[edge_outside_actions[index]][0] = edge_primitives[index];
        keyboard[edge_primitives[index]] = 1;
    }
    CHECK(ProjectKeyboard(bindings.data(), bindings.size(), true, keyboard, output,
                          valid) == ProjectionStatus::Ok);
    CHECK(valid == 0x0FFFu);
    for (std::uint32_t action = 4; action < input::kGameplayActionCount; ++action)
        CHECK(output[action] == 1.0F);

    // Duplicate slots in one action are idempotent, but a gameplay/gameplay owner
    // collision redacts every affected action, even when an outside alias also exists.
    bindings[4][1] = shoot_primitives[0];
    CHECK(ProjectKeyboard(bindings.data(), bindings.size(), true, keyboard, output,
                          valid) == ProjectionStatus::Ok);
    CHECK((valid & (1u << 4)) != 0);
    bindings[5][1] = shoot_primitives[0];
    CHECK(ProjectKeyboard(bindings.data(), bindings.size(), true, keyboard, output,
                          valid) == ProjectionStatus::Ok);
    CHECK((valid & ((1u << 4) | (1u << 5))) == 0);
    bindings[4][1] = input::kUnboundPrimitive;
    bindings[5][1] = input::kUnboundPrimitive;

    bindings[2][0] = static_cast<std::uint32_t>(kKeyboardPrimitiveCount);
    CHECK(ProjectKeyboard(bindings.data(), bindings.size(), true, keyboard, output,
                          valid) == ProjectionStatus::Ok);
    CHECK((valid & 0x0004u) == 0);

    for (auto& binding : bindings)
        binding.fill(input::kUnboundPrimitive);

    GamepadConcreteState gamepad{};
    gamepad.flags = 0x22;
    gamepad.deadzone = 0.2F;
    gamepad.button_low = 1u << 3;
    gamepad.directional_axes[1] = 0.7F;
    bindings[0][0] = 3;
    bindings[1][0] = kGamepadAxisPrimitiveBase + 1;
    bindings[12][0] = 3;
    CHECK(ProjectGamepad(bindings.data(), bindings.size(), gamepad, output, valid) ==
          ProjectionStatus::Ok);
    CHECK((valid & 0x0003u) == 0x0003u);
    CHECK(output[0] == 1.0F);
    CHECK(std::fabs(output[1] - 0.625F) < 0.0001F);
    const auto manager_gamepad_outside =
        input::ClassifyPrimitiveAction(3, bindings.data(), bindings.size());
    CHECK(manager_gamepad_outside.classification ==
          input::PrimitiveActionClass::AmbiguousOrOutsideGameplay);
    bindings[2][0] = 3;
    CHECK(ProjectGamepad(bindings.data(), bindings.size(), gamepad, output, valid) ==
          ProjectionStatus::Ok);
    CHECK((valid & ((1u << 0) | (1u << 2))) == 0);
    bindings[2][0] = input::kUnboundPrimitive;

    // device+0x40 is a directional magnitude cache.  Negative values cannot be repaired
    // with abs(): the opposite direction has its own slot, so this action is redacted.
    gamepad.directional_axes[1] = -0.7F;
    CHECK(ProjectGamepad(bindings.data(), bindings.size(), gamepad, output, valid) ==
          ProjectionStatus::Ok);
    CHECK((valid & 0x0001u) != 0);
    CHECK((valid & 0x0002u) == 0);
    gamepad.directional_axes[1] = 0.7F;

    bindings[2][0] = kGamepadSpecialPrimitiveFirst;
    gamepad.mappings[2] = {kGamepadSpecialPrimitiveFirst, 3};
    CHECK(ProjectGamepad(bindings.data(), bindings.size(), gamepad, output, valid) ==
          ProjectionStatus::Ok);
    CHECK((valid & 0x0004u) == 0);
    gamepad.mappings[2] = {};
    // Direct mode has an explicit legal zero branch when +0x90 rejects codes above
    // 0xABCA and +0x94 rejects non-special codes.
    gamepad.flags = 0x22;
    bindings[2][0] = kGamepadAxisPrimitiveBase + kGamepadDirectionalAxisCount;
    CHECK(ProjectGamepad(bindings.data(), bindings.size(), gamepad, output, valid) ==
          ProjectionStatus::Ok);
    CHECK((valid & 0x0004u) != 0 && output[2] == 0.0F);
    bindings[2][0] = input::kUnboundPrimitive;
    gamepad.deadzone = 1.0F;
    CHECK(ProjectGamepad(bindings.data(), bindings.size(), gamepad, output, valid) ==
          ProjectionStatus::Ok);
    CHECK((valid & 0x0001u) != 0);
    CHECK((valid & 0x0002u) == 0);

    // Mapped mode inlines the 29-entry table and rejects duplicate/unstable identities.
    gamepad.deadzone = 0.2F;
    gamepad.flags = 0x02;
    bindings[0][0] = 0x1000;
    bindings[1][0] = input::kUnboundPrimitive;
    gamepad.mappings[0] = {0x1000, 3};
    CHECK(ProjectGamepad(bindings.data(), bindings.size(), gamepad, output, valid) ==
          ProjectionStatus::Ok);
    CHECK((valid & 0x0001u) != 0 && output[0] == 1.0F);
    gamepad.mappings[1] = {0x1000, 4};
    CHECK(ProjectGamepad(bindings.data(), bindings.size(), gamepad, output, valid) ==
          ProjectionStatus::Ok);
    CHECK((valid & 0x0001u) == 0);

    auto bindings_second = bindings;
    CHECK(StableBindingsEqual(bindings, bindings.size(), bindings_second,
                              bindings_second.size()));
    bindings_second[20][0] ^= 1;
    CHECK(StableBindingsEqual(bindings, bindings.size(), bindings_second,
                              bindings_second.size()));
    bindings_second[20][0] ^= 1;
    bindings_second[0][0] ^= 1;
    CHECK(!StableBindingsEqual(bindings, bindings.size(), bindings_second,
                               bindings_second.size()));
    auto keyboard_second = keyboard;
    CHECK(StableKeyboardStateEqual(true, keyboard, true, keyboard_second));
    keyboard_second[0x41] = 0;
    CHECK(!StableKeyboardStateEqual(true, keyboard, true, keyboard_second));
    auto gamepad_second = gamepad;
    CHECK(StableGamepadStateEqual(gamepad, gamepad_second));
    gamepad_second.button_low ^= 1;
    CHECK(!StableGamepadStateEqual(gamepad, gamepad_second));

    ProjectionHistory history{};
    std::array<float, input::kGameplayActionCount> neutral{};
    auto edges = UpdateProjectionHistory(history, 7, neutral, 0x0FFFu);
    CHECK(edges.rising == 0 && edges.falling == 0);
    auto held = neutral;
    held[0] = 1.0F;
    edges = UpdateProjectionHistory(history, 7, held, 0x0FFFu);
    CHECK(edges.rising == 1 && edges.falling == 0);
    edges = UpdateProjectionHistory(history, 7, neutral, 0x0FFFu);
    CHECK(edges.rising == 0 && edges.falling == 1);
    edges = UpdateProjectionHistory(history, 8, held, 0x0FFFu);
    CHECK(edges.rising == 0 && edges.falling == 0);

    CHECK(!LegacyActionCacheProjectionEnabled());
    CHECK(PhysicalProjectionContextEligible(kPhysicalPollReturnRva, kInputQueryPhase));
    CHECK(!PhysicalProjectionContextEligible(kPhysicalPollReturnRva, kInputQueryPhase + 1));
    CHECK(!PhysicalProjectionContextEligible(kPhysicalPollReturnRva + 1, kInputQueryPhase));
    CHECK(ProjectionPayloadScopeValid(0, UINT32_MAX));
    CHECK(ProjectionPayloadScopeValid(11, UINT32_MAX));
    CHECK(!ProjectionPayloadScopeValid(12, UINT32_MAX));
    CHECK(!ProjectionPayloadScopeValid(4, 0x107));
}

void TestCombinedSchemaThreeFiftySlotTransactionRollbackAndRestore()
{
    using namespace isaac_cs_t0::mode;
    constexpr std::size_t relative_count = 33;
    constexpr std::size_t pointer_count = 17;
    static_assert(relative_count + pointer_count == 50);
    std::array<std::array<std::uint8_t, 8>, relative_count> calls{};
    std::array<std::array<std::uint8_t, 8>, relative_count> originals{};
    std::array<RelativeCallPatch, relative_count> relatives{};
    for (std::size_t index = 0; index < relative_count; ++index)
    {
        auto& call = calls[index];
        call[0] = 0xE8;
        const auto next = reinterpret_cast<std::intptr_t>(call.data() + 5);
        const auto target = reinterpret_cast<std::intptr_t>(&OriginalValue);
        const auto displacement = static_cast<std::int32_t>(target - next);
        std::memcpy(call.data() + 1, &displacement, sizeof(displacement));
        originals[index] = call;
        relatives[index].call = call.data();
        std::memcpy(relatives[index].expected, call.data(), 5);
        relatives[index].replacement = reinterpret_cast<void*>(&ReplacementValue);
    }
    std::array<void*, pointer_count> slots{};
    slots.fill(reinterpret_cast<void*>(&OriginalValue));
    std::array<PointerPatch, pointer_count> pointers{};
    for (std::size_t index = 0; index < pointer_count; ++index)
        pointers[index] = {&slots[index], reinterpret_cast<void*>(&ReplacementValue),
                           nullptr, nullptr, false};

    // Fail after all 33 relative slots and eleven pointer slots have been applied.
    pointers[11].slot = &slots[0];
    PatchTransactionFailure failure{};
    CHECK(!InstallPatchTransaction(relatives.data(), relatives.size(), pointers.data(),
                                   pointers.size(), &failure));
    CHECK(failure.stage == PatchFailureStage::ApplyPointer);
    CHECK(failure.index == 11);
    for (std::size_t index = 0; index < relative_count; ++index)
    {
        CHECK(calls[index] == originals[index]);
        CHECK(!relatives[index].installed);
    }
    for (std::size_t index = 0; index < pointer_count; ++index)
    {
        CHECK(slots[index] == reinterpret_cast<void*>(&OriginalValue));
        CHECK(!pointers[index].installed);
    }

    pointers[11].slot = &slots[11];
    failure = {};
    CHECK(InstallPatchTransaction(relatives.data(), relatives.size(), pointers.data(),
                                  pointers.size(), &failure));
    // Schema 3 shutdown restores its 6 relative and 12 pointer hooks first while the
    // companion V5 27-relative/5-pointer set remains installed.
    CHECK(RestorePatchTransaction(relatives.data() + 27, 6, pointers.data() + 5, 12));
    for (std::size_t index = 0; index < 27; ++index)
        CHECK(relatives[index].installed);
    for (std::size_t index = 27; index < relative_count; ++index)
        CHECK(!relatives[index].installed);
    for (std::size_t index = 0; index < 5; ++index)
        CHECK(pointers[index].installed);
    for (std::size_t index = 5; index < pointer_count; ++index)
        CHECK(!pointers[index].installed);
    CHECK(RestorePatchTransaction(relatives.data(), 27, pointers.data(), 5));
    for (std::size_t index = 0; index < relative_count; ++index)
        CHECK(calls[index] == originals[index]);
    for (void* slot : slots)
        CHECK(slot == reinterpret_cast<void*>(&OriginalValue));
}

void TestEmbeddedLoopCompanionIdentityRequiresExactCrossMappingMatch()
{
    using isaac_cs_t0::loop::EmbeddedObserverIdentity;
    using isaac_cs_t0::loop::detail::CompanionIdentityMatches;
    using isaac_cs_t0::loop::detail::CompanionMappingIdentityAllowed;
    using isaac_cs_t0::loop::detail::CompanionScenarioAllowed;
    CHECK(!CompanionScenarioAllowed(0, false));
    CHECK(!CompanionScenarioAllowed(0, true));
    for (std::uint32_t scenario = 1; scenario <= 10; ++scenario)
    {
        CHECK(CompanionScenarioAllowed(scenario, false));
        CHECK(CompanionScenarioAllowed(scenario, true));
    }
    for (std::uint32_t scenario = 11; scenario <= 16; ++scenario)
    {
        CHECK(!CompanionScenarioAllowed(scenario, false));
        CHECK(CompanionScenarioAllowed(scenario, true));
    }
    CHECK(!CompanionScenarioAllowed(17, false));
    CHECK(!CompanionScenarioAllowed(17, true));
    EmbeddedObserverIdentity expected{};
    expected.target_pid = 1234;
    expected.scenario = 7;
    for (std::size_t index = 0; index < expected.nonce.size(); ++index)
        expected.nonce[index] = static_cast<std::uint8_t>(index + 1);

    CHECK(CompanionIdentityMatches(1234, 7, expected.nonce, expected));
    CHECK(!CompanionIdentityMatches(1235, 7, expected.nonce, expected));
    CHECK(!CompanionIdentityMatches(1234, 6, expected.nonce, expected));
    auto mismatched_nonce = expected.nonce;
    mismatched_nonce[17] ^= 0x80;
    CHECK(!CompanionIdentityMatches(1234, 7, mismatched_nonce, expected));
    std::array<std::uint8_t, 32> zero_nonce{};
    CHECK(!CompanionIdentityMatches(1234, 7, zero_nonce, expected));

    expected.scenario = 11;
    CHECK(CompanionMappingIdentityAllowed(1234, 11, expected.nonce, true,
                                          &expected));
    CHECK(!CompanionMappingIdentityAllowed(1234, 11, expected.nonce, false,
                                           nullptr));
    CHECK(!CompanionMappingIdentityAllowed(1234, 11, expected.nonce, false,
                                           &expected));
    CHECK(!CompanionMappingIdentityAllowed(1234, 11, expected.nonce, true,
                                           nullptr));
    expected.scenario = 16;
    CHECK(CompanionMappingIdentityAllowed(1234, 16, expected.nonce, true,
                                          &expected));
    CHECK(!CompanionMappingIdentityAllowed(1234, 17, expected.nonce, true,
                                           &expected));
    CHECK(CompanionMappingIdentityAllowed(1234, 10, expected.nonce, false,
                                          nullptr));
}

void TestLoopX86AbiBridgesPreserveRegistersArgumentsAndCleanup()
{
    static_assert(sizeof(void*) == 4);
    g_calls = 0;
    alignas(16) double clock_result[2]{};
    _mm_store_pd(clock_result, LoopClockBridgeHarness());
    CHECK(g_calls == 1);
    CHECK(clock_result[0] == 1.25);
    CHECK(clock_result[1] == -9.5);

    LoopAbiOwnerHarness owner{1};
    g_calls = 0;
    g_loop_abi_owner = nullptr;
    std::uintptr_t stack_before = 0;
    std::uintptr_t stack_after = 0;
    __asm mov stack_before, esp
    const bool poll_result = PhysicalPollBridgeHarness(&owner, nullptr);
    __asm mov stack_after, esp
    CHECK(poll_result);
    CHECK(g_calls == 1);
    CHECK(g_loop_abi_owner == &owner);
    CHECK(stack_before == stack_after);

    g_calls = 0;
    g_loop_abi_owner = nullptr;
    g_loop_abi_argument = 0;
    __asm mov stack_before, esp
    EntityListBridgeHarness(&owner, nullptr, 0x13579BDFu);
    __asm mov stack_after, esp
    CHECK(g_calls == 1);
    CHECK(g_loop_abi_owner == &owner);
    CHECK(g_loop_abi_argument == 0x13579BDFu);
    CHECK(stack_before == stack_after);

    g_calls = 0;
    g_loop_abi_owner = nullptr;
    g_loop_abi_state = nullptr;
    g_loop_abi_argument = 0;
    void* state = reinterpret_cast<void*>(0x2468ACE0u);
    __asm mov stack_before, esp
    StateApplyBridgeHarness(&owner, nullptr, state, 1);
    __asm mov stack_after, esp
    CHECK(g_calls == 1);
    CHECK(g_loop_abi_owner == &owner);
    CHECK(g_loop_abi_state == state);
    CHECK(g_loop_abi_argument == 1);
    CHECK(stack_before == stack_after);

    g_calls = 0;
    g_loop_abi_argument = 0;
    __asm mov stack_before, esp
    PostGameStartBridgeHarness(0xA5A55A5Au);
    __asm mov stack_after, esp
    CHECK(g_calls == 1);
    CHECK(g_loop_abi_argument == 0xA5A55A5Au);
    CHECK(stack_before == stack_after);
}

void TestLoopVoidThiscallLifecycleBridgesPreserveAbi()
{
    LoopAbiOwnerHarness owner{};
    for (const auto [bridge, expected_tag] :
         std::array<std::pair<VoidThiscallBridgeHarness, std::uint32_t>, 4>{{
             {&StandardRoomLifecycleOwnerBridgeHarness, 59},
             {&StandardRoomLifecycleSimulationBridgeHarness, 61},
             {&RoomChangeOwnerBridgeHarness, 63},
             {&RoomChangeSimulationBridgeHarness, 65},
         }})
    {
        g_calls = 0;
        g_loop_abi_owner = nullptr;
        g_loop_abi_argument = 0;
        VoidThiscallProbeResult result{};
        ProbeVoidThiscallBridge(bridge, &owner, &result);
        CHECK(g_calls == 1);
        CHECK(g_loop_abi_owner == &owner);
        CHECK(g_loop_abi_argument == expected_tag);
        CHECK(result.stack_before == result.stack_after);
        CHECK(result.ebx_after == 0x13579BDFu);
        CHECK(result.esi_after == 0x2468ACE0u);
        CHECK(result.edi_after == 0xA5A55A5Au);
    }
}

void TestLoopBootstrapThreadIdentityIsFailClosedWithoutVacuousEvidence()
{
    constexpr std::uint32_t main_thread = 0x1234u;
    constexpr std::uint32_t worker_thread = 0x5678u;
    LoopBootstrapThreadHarness harness{};

    harness.NonClock(worker_thread);
    CHECK(harness.original_calls == 1);
    CHECK(harness.main_thread_id == 0);
    CHECK(harness.phase_events == 0);
    CHECK(harness.unsafe_code == 0);

    for (int wrapper = 0; wrapper < 3; ++wrapper)
        harness.Message(worker_thread);
    CHECK(harness.original_calls == 4);
    CHECK(harness.main_thread_id == 0);
    CHECK(harness.phase_events == 0);
    CHECK(harness.unsafe_code == 0);
    CHECK(!harness.iteration_zero_phase_event);

    harness.Clock(main_thread);
    CHECK(harness.original_calls == 5);
    CHECK(harness.main_thread_id == main_thread);
    CHECK(harness.loop_iteration == 1);
    CHECK(harness.phase_events == 1);
    CHECK(harness.unsafe_code == 0);
    CHECK(!harness.iteration_zero_phase_event);

    harness.NonClock(main_thread);
    harness.Message(main_thread);
    CHECK(harness.original_calls == 7);
    CHECK(harness.phase_events == 3);
    CHECK(harness.unsafe_code == 0);
    CHECK(!harness.iteration_zero_phase_event);

    harness.NonClock(worker_thread);
    CHECK(harness.original_calls == 8);
    CHECK(harness.phase_events == 3);
    CHECK(harness.unsafe_code == 6);
    CHECK(!harness.iteration_zero_phase_event);
}

void TestSpawnBridgePreservesThiscallOwnerAndEightArguments()
{
    g_calls = 0;
    g_spawn_owner = nullptr;
    g_spawn_arguments = {};
    SpawnOwnerHarness owner{0xA5A55A5Au};
    void* result = SpawnBridgeHarness(&owner, nullptr, 1, 2, 3, 4, 5, 6, 7, 0x55);
    CHECK(result == reinterpret_cast<void*>(0x54));
    CHECK(g_calls == 1);
    CHECK(g_spawn_owner == &owner);
    CHECK((g_spawn_arguments == std::array<std::uintptr_t, 8>{1, 2, 3, 4, 5, 6, 7, 0x55}));
}

void TestLuaLifecycleBridgesCallOriginalExactlyOnce()
{
    g_calls = 0;
    g_lua_allocator = nullptr;
    g_lua_user_data = nullptr;
    g_lua_thread = 0;
    void* allocator = reinterpret_cast<void*>(0x11112222u);
    void* user_data = reinterpret_cast<void*>(0x33334444u);
    CHECK(LuaNewstateBridgeHarness(allocator, user_data) ==
          reinterpret_cast<void*>(0x12345678u));
    CHECK(g_calls == 1);
    CHECK(g_lua_allocator == allocator);
    CHECK(g_lua_user_data == user_data);
    CHECK(g_lua_thread == GetCurrentThreadId());

    g_calls = 0;
    g_lua_closed_state = nullptr;
    void* state = reinterpret_cast<void*>(0x55556666u);
    LuaCloseBridgeHarness(state);
    CHECK(g_calls == 1);
    CHECK(g_lua_closed_state == state);
    CHECK(g_lua_thread == GetCurrentThreadId());
}

void TestLifecycleEvidenceIsFailClosed()
{
    using namespace isaac_cs_t0::mode;
    g_calls = 0;
    g_lifecycle_argument = 0;
    g_lifecycle_thread = 0;
    const DWORD caller_thread = GetCurrentThreadId();
    CHECK(InvokeLifecycleOnce(&OriginalLifecycle, 0x13579BDFu) == 0xB6F2C185u);
    CHECK(g_calls == 1);
    CHECK(g_lifecycle_argument == 0x13579BDFu);
    CHECK(g_lifecycle_thread == caller_thread);
    CHECK(IsRunActive(1));
    CHECK(IsRunActive(0x0101));
    CHECK(!IsRunActive(0));
    CHECK(!IsRunActive(kLifecycleUnavailable));
    CHECK(IsStartObservation(1, 1));
    CHECK(!IsStartObservation(0, 1));
    CHECK(IsTeardownObservation(1, 0, true));
    CHECK(!IsTeardownObservation(1, 1, true));
    CHECK(!IsTeardownObservation(1, 0, false));
}

} // namespace

int main()
{
    g_failures += RunInputCoreTests();
    TestModeLatch();
    TestPassthroughCallsExactlyOnce();
    TestPatchTransactionAndRestore();
    TestPatchTransactionDiagnosticsAreStableAndPrewrite();
    TestPatchTransactionApplyFailureRollsBack();
    TestOfflineAttestationSixSlotTransactionIsAtomicAndRestoresInOrder();
    TestSha256();
    TestRingLayoutAndSequenceModel();
    TestBoundaryRingAndRecursiveCallModel();
    TestBoundaryEventPayloadCopyDoesNotPublishEarly();
    TestBoundaryPassthroughContract();
    TestLoopRingLayoutAndPayloadPublication();
    TestInputSchemaTwoRingLayoutAndPayloadPublication();
    TestInputX86AbiBridgesPreserveArgumentsStackReturnsAndOriginalOnce();
    TestCombinedFortyThreePointerTransactionRollbackAndRestore();
    TestEmbeddedShutdownFailStopsAndNeverUnloadsEarly();
    TestInputBootstrapOutsideAndWrongThreadPolicy();
    TestInputReturnRvaIsStrictlyBoundedToMainImage();
    TestRawApplySchemaThreeLayoutAndPayloadPublication();
    TestRawConsumerX86AbiAndOriginalOnce();
    TestConcretePhysicalProjectionAndPrivacy();
    TestCombinedSchemaThreeFiftySlotTransactionRollbackAndRestore();
    TestEmbeddedLoopCompanionIdentityRequiresExactCrossMappingMatch();
    TestLoopX86AbiBridgesPreserveRegistersArgumentsAndCleanup();
    TestLoopVoidThiscallLifecycleBridgesPreserveAbi();
    TestLoopBootstrapThreadIdentityIsFailClosedWithoutVacuousEvidence();
    TestSpawnBridgePreservesThiscallOwnerAndEightArguments();
    TestLuaLifecycleBridgesCallOriginalExactlyOnce();
    TestLifecycleEvidenceIsFailClosed();
    if (g_failures)
    {
        std::fprintf(stderr, "%d independent T0 Native checks failed\n", g_failures);
        return 1;
    }
    std::puts("PASS independent T0 Native harness");
    std::puts("{\"identity\":\"IsaacCsT0-NativeHarness-v1\",\"totalCases\":39,\"status\":\"PASS\"}");
    return 0;
}
