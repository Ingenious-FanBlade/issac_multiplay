#include "isaac_cs_t0/loop_runtime.hpp"

#include "isaac_cs_t0/hooks.hpp"
#include "isaac_cs_t0/world_record.hpp"
#include "isaac_cs_t0/world_e2e.hpp"
#include "isaac_cs_t0/loop_ipc.hpp"
#include "isaac_cs_t0/sha256.hpp"
#include "isaac_cs_t0/target_image.hpp"

#include <Windows.h>
#include <intrin.h>
#include <emmintrin.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace isaac_cs_t0::loop {
namespace {

constexpr std::uint32_t kKeyboardVtableRva = 0x007A1678u;
constexpr std::uint32_t kKeyboardColRva = 0x007B191Cu;
constexpr std::uint32_t kGamepadVtableRva = 0x00782C80u;
constexpr std::uint32_t kGamepadColRva = 0x007B0460u;
constexpr std::uint32_t kPressedRva = 0x0061E8D0u;
constexpr std::uint32_t kTriggeredRva = 0x0061E970u;
constexpr std::uint32_t kScalarRva = 0x0061EA20u;
constexpr std::uint32_t kKeyboardPollRva = 0x0066C650u;
constexpr std::uint32_t kGamepadPollRva = 0x00622940u;
constexpr std::uint32_t kPeekIatRva = 0x00718714u;
constexpr std::uint32_t kTranslateIatRva = 0x00718650u;
constexpr std::uint32_t kDispatchIatRva = 0x007185B8u;
constexpr DWORD kHookDrainTimeoutMs = 5000;
constexpr std::size_t kMaximumEmbeddedPointerCount = 12;
constexpr std::size_t kMaximumEmbeddedRelativeCount = 17;
constexpr std::size_t kMaximumPointerPatchCount =
    kPointerHookCount + kMaximumEmbeddedPointerCount;
constexpr std::size_t kMaximumRelativePatchCount =
    kRelativeHookCount + kMaximumEmbeddedRelativeCount;
constexpr std::size_t kMaximumOrderedPatchCount =
    kMaximumRelativePatchCount + kMaximumPointerPatchCount + 19;

constexpr std::array<std::uint32_t, kRelativeHookCount> kCallRvas{{
    0x00531231u, // loop header clock
    0x005312ECu, // top-level update driver
    0x00555231u, // standard-or-post-conditional attempt
    0x002FBBA5u, // standard tail
    0x00559696u, // run teardown caller 1
    0x005AB84Au, // run teardown caller 2
    0x0033F5B9u, // new-room callback
    0x005312F1u, // top-level render owner
    0x00554F7Du, // InputManager per-full-update sample owner
    0x0061FC97u, // common per-device update
    0x00554D5Eu, // alternate transaction owner attempt
    0x00559B4Du, // alternate room simulation
    0x00559B82u, // alternate transaction tail
    0x002FADFAu, // standard-path predicate
    0x002FAE09u, // standard body begin anchor
    0x002FBA4Du, // standard room simulation
    0x00403E70u, // room entity-list update
    0x00559B21u, // alternate snapshot state apply
    0x002F580Bu, // alternate outcome completion 0
    0x002F61FFu, // alternate outcome completion 1
    0x002F72C9u, // alternate outcome completion 2
    0x002F7721u, // alternate outcome completion 3
    0x002F7A10u, // alternate outcome completion 4
    0x002FB4F0u, // standard room-lifecycle FSM owner
    0x0052FC77u, // standard room-lifecycle nested simulation
    0x002FB51Cu, // ordinary-door room-change state-machine owner
    0x002FE37Cu, // ordinary-door room-change nested Room::Update
}};

constexpr std::array<std::uint32_t, kRelativeHookCount> kTargetRvas{{
    0x006266A0u, 0x005AB6D0u, 0x002FADC0u, 0x004607A0u,
    0x002FA0C0u, 0x002FA0C0u, 0x00465180u, 0x005555C0u,
    0x0061FC00u, 0x0061F280u, 0x00559720u, 0x00402980u,
    0x00464EE0u, 0x0028BBA0u, 0x00308630u, 0x00402980u,
    0x000186C0u,
    0x002F8140u,
    0x00464EE0u, 0x00464EE0u, 0x00464EE0u, 0x00464EE0u,
    0x00464EE0u,
    0x0052F1C0u, 0x00402980u,
    0x004318A0u, 0x00402980u,
}};

constexpr std::array<std::array<std::uint8_t, 5>, kRelativeHookCount> kCallBytes{{
    {{0xE8, 0x6A, 0x54, 0x0F, 0x00}},
    {{0xE8, 0xDF, 0xA3, 0x07, 0x00}},
    {{0xE8, 0x8A, 0x5B, 0xDA, 0xFF}},
    {{0xE8, 0xF6, 0x4B, 0x16, 0x00}},
    {{0xE8, 0x25, 0x0A, 0xDA, 0xFF}},
    {{0xE8, 0x71, 0xE8, 0xD4, 0xFF}},
    {{0xE8, 0xC2, 0x5B, 0x12, 0x00}},
    {{0xE8, 0xCA, 0x42, 0x02, 0x00}},
    {{0xE8, 0x7E, 0xAC, 0x0C, 0x00}},
    {{0xE8, 0xE4, 0xF5, 0xFF, 0xFF}},
    {{0xE8, 0xBD, 0x49, 0x00, 0x00}},
    {{0xE8, 0x2E, 0x8E, 0xEA, 0xFF}},
    {{0xE8, 0x59, 0xB3, 0xF0, 0xFF}},
    {{0xE8, 0xA1, 0x0D, 0xF9, 0xFF}},
    {{0xE8, 0x22, 0xD8, 0x00, 0x00}},
    {{0xE8, 0x2E, 0x6F, 0x10, 0x00}},
    {{0xE8, 0x4B, 0x48, 0xC1, 0xFF}},
    {{0xE8, 0x1A, 0xE6, 0xD9, 0xFF}},
    {{0xE8, 0xD0, 0xF6, 0x16, 0x00}},
    {{0xE8, 0xDC, 0xEC, 0x16, 0x00}},
    {{0xE8, 0x12, 0xDC, 0x16, 0x00}},
    {{0xE8, 0xBA, 0xD7, 0x16, 0x00}},
    {{0xE8, 0xCB, 0xD4, 0x16, 0x00}},
    {{0xE8, 0xCB, 0x3C, 0x23, 0x00}},
    {{0xE8, 0x04, 0x2D, 0xED, 0xFF}},
    {{0xE8, 0x7F, 0x63, 0x13, 0x00}},
    {{0xE8, 0xFF, 0x45, 0x10, 0x00}},
}};

enum class ErrorCode : LONG {
    None = 0,
    MappingInvalid = 1,
    TargetImageMismatch = 2,
    ConflictingModuleOrDebugger = 3,
    HookTransaction = 4,
    EventRingOverflow = 5,
    WrongThread = 6,
    UnsafeShutdown = 8,
    HookDrainTimeout = 9,
};
// helper.cpp reserves initializer result 8 for "do not unload: hooks may remain installed".
static_assert(static_cast<LONG>(ErrorCode::UnsafeShutdown) == 8);

enum class InstallFailureStage : std::uint32_t {
    None = 0,
    CallTarget = 1,
    IatLookup = 2,
    IatRva = 3,
    IatOwner = 4,
    VtableCol = 5,
    VtableQuery = 6,
    PatchTransactionBase = 32,
};

struct InstallFailureDiagnostic {
    std::uint32_t stage = static_cast<std::uint32_t>(InstallFailureStage::None);
    std::uint32_t index = UINT32_MAX;
    DWORD win32_error = ERROR_SUCCESS;
};

using ClockFn = __m128d(__cdecl*)();
using VoidFn = void(__cdecl*)();
using ThisVoidFn = void(__thiscall*)(void*);
using ThisBoolFn = bool(__thiscall*)(void*);
using ThisArgVoidFn = void(__thiscall*)(void*, std::uint32_t);
using ThisTwoArgVoidFn = void(__thiscall*)(void*, void*, std::uint32_t);
using LifecycleFn = std::uint32_t(__stdcall*)(std::uint32_t);
using PostGameStartFn = void(__stdcall*)(std::uint32_t);
using PeekMessageFn = BOOL(WINAPI*)(LPMSG, HWND, UINT, UINT, UINT);
using TranslateMessageFn = BOOL(WINAPI*)(const MSG*);
using DispatchMessageFn = LRESULT(WINAPI*)(const MSG*);

HMODULE g_module = nullptr;
HMODULE g_main_module = nullptr;
std::uint8_t* g_main_base = nullptr;
HANDLE g_mapping = nullptr;
SharedLoopBlock* g_shared = nullptr;
volatile LONG g_unloading = 0;
volatile LONG g_armed = 0;
volatile LONG g_active_hooks = 0;
volatile LONG g_unsafe_reported = 0;
volatile LONG g_embedded = 0;
volatile LONG g_embedded_input_restored = 0;
volatile LONG g_strict_ordered_transaction = 0;

thread_local Phase g_tls_phase = Phase::OutsideEnvelope;
thread_local std::uint32_t g_tls_phase_depth = 0;
thread_local detail::AlternateEnvelopeTracker g_tls_alternate_envelope{};
thread_local void* g_tls_alternate_owner = nullptr;

ClockFn g_clock = nullptr;
VoidFn g_update = nullptr;
ThisVoidFn g_gameplay = nullptr;
VoidFn g_post_update = nullptr;
LifecycleFn g_run_teardown = nullptr;
VoidFn g_new_room = nullptr;
VoidFn g_render = nullptr;
ThisVoidFn g_input_manager_sample = nullptr;
ThisBoolFn g_input_device_update = nullptr;
ThisVoidFn g_alternate_owner = nullptr;
ThisVoidFn g_alternate_room = nullptr;
PostGameStartFn g_alternate_tail = nullptr;
ThisBoolFn g_standard_predicate = nullptr;
ThisVoidFn g_standard_body = nullptr;
ThisVoidFn g_standard_room = nullptr;
ThisArgVoidFn g_entity_list = nullptr;
ThisTwoArgVoidFn g_alternate_state_apply = nullptr;
ThisVoidFn g_standard_room_lifecycle_owner = nullptr;
ThisVoidFn g_standard_room_lifecycle_simulation = nullptr;
ThisVoidFn g_room_change_owner = nullptr;
ThisVoidFn g_room_change_simulation = nullptr;
ThisBoolFn g_keyboard_poll = nullptr;
ThisBoolFn g_gamepad_poll = nullptr;
PeekMessageFn g_peek_message = nullptr;
TranslateMessageFn g_translate_message = nullptr;
DispatchMessageFn g_dispatch_message = nullptr;

std::array<isaac_cs_t0::mode::RelativeCallPatch, kMaximumRelativePatchCount> g_relative_patches{};
std::array<isaac_cs_t0::mode::PointerPatch, kMaximumPointerPatchCount> g_pointer_patches{};
std::array<isaac_cs_t0::mode::OrderedPatchDescriptor, kMaximumOrderedPatchCount>
    g_ordered_patches{};
std::size_t g_relative_patch_count = 0;
std::size_t g_pointer_patch_count = 0;
std::size_t g_ordered_patch_count = 0;
isaac_cs_t0::mode::RelativeCallPatch* g_embedded_input_relative_patches = nullptr;
std::size_t g_embedded_input_relative_patch_count = 0;
isaac_cs_t0::mode::PointerPatch* g_embedded_input_patches = nullptr;
std::size_t g_embedded_input_patch_count = 0;
PhysicalPollObservation g_physical_poll_observation = nullptr;
void* g_physical_poll_observation_context = nullptr;
DeviceUpdateCompleteObservation g_device_update_complete_observation = nullptr;
void* g_device_update_complete_observation_context = nullptr;
PhysicalExitObservation g_physical_exit_observation = nullptr;
void* g_physical_exit_observation_context = nullptr;
AuthorityObservation g_authority_observation = nullptr;
void* g_authority_observation_context = nullptr;
TopUpdateExitObservation g_top_update_exit_observation = nullptr;
void* g_top_update_exit_observation_context = nullptr;
LifecycleObservation g_lifecycle_observation = nullptr;
void* g_lifecycle_observation_context = nullptr;
InstallFailureDiagnostic g_install_failure{};

extern "C" __m128d __cdecl LoopClockHook();
void __cdecl UpdateHook();
extern "C" void __fastcall LoopGameplayHook(void* self, void* unused);
void __cdecl PostUpdateHook();
std::uint32_t __stdcall RunTeardownHook(std::uint32_t argument);
void __cdecl NewRoomHook();
void __cdecl RenderHook();
extern "C" void __fastcall InputManagerSampleHook(void* self, void* unused);
extern "C" bool __fastcall InputDeviceUpdateHook(void* self, void* unused);
extern "C" void __fastcall AlternateOwnerHook(void* self, void* unused);
extern "C" void __fastcall AlternateRoomHook(void* self, void* unused);
void __stdcall AlternateTailHook(std::uint32_t argument);
extern "C" bool __fastcall StandardPredicateHook(void* self, void* unused);
extern "C" void __fastcall StandardBodyHook(void* self, void* unused);
extern "C" void __fastcall StandardRoomHook(void* self, void* unused);
extern "C" void __fastcall EntityListHook(void* self, void* unused, std::uint32_t argument);
extern "C" void __fastcall AlternateStateApplyHook(void* self, void* unused, void* state,
                                                     std::uint32_t flag);
void __stdcall AlternateOutcomeHook(std::uint32_t argument);
extern "C" void __fastcall StandardRoomLifecycleOwnerHook(void* self, void* unused);
extern "C" void __fastcall StandardRoomLifecycleSimulationHook(void* self, void* unused);
bool SnapshotOrderedSlots(std::array<std::uint8_t, 32>& digest) noexcept;
extern "C" void __fastcall RoomChangeOwnerHook(void* self, void* unused);
extern "C" void __fastcall RoomChangeSimulationHook(void* self, void* unused);
extern "C" bool __fastcall KeyboardPollHook(void* self, void* unused);
extern "C" bool __fastcall GamepadPollHook(void* self, void* unused);
BOOL WINAPI PeekMessageHook(LPMSG message, HWND window, UINT minimum, UINT maximum,
                            UINT remove);
BOOL WINAPI TranslateMessageHook(const MSG* message);
LRESULT WINAPI DispatchMessageHook(const MSG* message);

class HookScope final {
public:
    HookScope() noexcept
    {
        InterlockedIncrement(&g_active_hooks);
        if (g_shared)
            InterlockedIncrement(&g_shared->active_hooks);
    }
    ~HookScope()
    {
        if (g_shared)
            InterlockedDecrement(&g_shared->active_hooks);
        InterlockedDecrement(&g_active_hooks);
    }
};

class PhaseScope final {
public:
    explicit PhaseScope(Phase phase) noexcept
        : previous_phase_(g_tls_phase), previous_depth_(g_tls_phase_depth)
    {
        g_tls_phase = phase;
        g_tls_phase_depth = previous_depth_ + 1;
    }

    ~PhaseScope()
    {
        g_tls_phase = previous_phase_;
        g_tls_phase_depth = previous_depth_;
    }

private:
    Phase previous_phase_;
    std::uint32_t previous_depth_;
};

std::int64_t Qpc() noexcept
{
    LARGE_INTEGER value{};
    return QueryPerformanceCounter(&value) ? value.QuadPart : 0;
}

bool Nonzero(const std::array<std::uint8_t, 32>& bytes) noexcept
{
    return std::any_of(bytes.begin(), bytes.end(), [](std::uint8_t value) { return value != 0; });
}

void SetState(ErrorCode error, NativeState state) noexcept
{
    if (!g_shared)
        return;
    InterlockedExchange(&g_shared->error_code, static_cast<LONG>(error));
    InterlockedExchange(&g_shared->state, static_cast<LONG>(state));
}

std::uint32_t ReturnRva(void* address) noexcept
{
    const auto value = reinterpret_cast<std::uintptr_t>(address);
    const auto base = reinterpret_cast<std::uintptr_t>(g_main_base);
    return value >= base && value - base <= UINT32_MAX
               ? static_cast<std::uint32_t>(value - base)
               : 0;
}

NativeEvent Event(EventKind kind, Phase phase) noexcept
{
    NativeEvent event{};
    event.qpc = Qpc();
    event.thread_id = GetCurrentThreadId();
    event.kind = static_cast<std::uint32_t>(kind);
    event.phase = static_cast<std::uint32_t>(phase);
    if (g_shared)
    {
        event.loop_iteration = static_cast<std::uint64_t>(
            InterlockedCompareExchange64(&g_shared->current_loop_iteration, 0, 0));
        if (event.loop_iteration)
            event.flags |= kFlagLoopKnown;
        const LONG main_thread =
            InterlockedCompareExchange(&g_shared->main_thread_id, 0, 0);
        if (main_thread && static_cast<DWORD>(main_thread) == event.thread_id)
            event.flags |= kFlagMainThread;
        else if (main_thread)
            event.flags |= kFlagWrongThread;
    }
    return event;
}

void Publish(const NativeEvent& event) noexcept
{
    if (!g_shared)
        return;
    const LONG64 sequence = InterlockedIncrement64(&g_shared->next_sequence);
    const LONG64 consumed = InterlockedCompareExchange64(&g_shared->controller_sequence, 0, 0);
    if (sequence - consumed > static_cast<LONG64>(kEventCapacity))
    {
        InterlockedIncrement64(&g_shared->overwritten_events);
        SetState(ErrorCode::EventRingOverflow, NativeState::Unsafe);
    }
    NativeEvent& slot = g_shared->events[static_cast<std::size_t>(sequence - 1) % kEventCapacity];
    InterlockedExchange64(&slot.published_sequence, 0);
    CopyEventPayload(slot, event);
    MemoryBarrier();
    InterlockedExchange64(&slot.published_sequence, sequence);
}

bool Armed() noexcept
{
    return g_shared && InterlockedCompareExchange(&g_armed, 0, 0) != 0 &&
           InterlockedCompareExchange(&g_unloading, 0, 0) == 0 &&
           InterlockedCompareExchange(&g_unsafe_reported, 0, 0) == 0;
}

void ReportUnsafe(ErrorCode error) noexcept
{
    if (!g_shared || InterlockedExchange(&g_unsafe_reported, 1) != 0)
        return;
    NativeEvent event = Event(EventKind::Failure, Phase::OutsideEnvelope);
    event.error_code = static_cast<std::uint32_t>(error);
    Publish(event);
    SetState(error, NativeState::Unsafe);
}

bool EstablishOrCheckMainThread(bool establish, bool required) noexcept
{
    if (!g_shared)
        return false;
    const LONG thread = static_cast<LONG>(GetCurrentThreadId());
    LONG main_thread = InterlockedCompareExchange(&g_shared->main_thread_id, 0, 0);
    if (!main_thread && establish)
    {
        InterlockedCompareExchange(&g_shared->main_thread_id, thread, 0);
        main_thread = InterlockedCompareExchange(&g_shared->main_thread_id, 0, 0);
    }
    // Installation can finish in the middle of an existing loop iteration.  Until the
    // next clock anchor freezes the main thread, non-clock hooks are bootstrap
    // passthroughs: they must neither fabricate iteration-zero evidence nor classify
    // an as-yet unknown thread as a mismatch.
    if (!main_thread)
        return false;
    const bool matches = main_thread && main_thread == thread;
    if (required && !matches)
        ReportUnsafe(ErrorCode::WrongThread);
    return matches;
}

void PublishObservedCall(EventKind kind, Phase phase, void* return_address,
                         std::uint32_t original_calls) noexcept
{
    NativeEvent event = Event(kind, phase);
    event.return_rva = ReturnRva(return_address);
    event.original_calls = original_calls;
    if (original_calls)
        event.flags |= kFlagOriginalCalled;
    Publish(event);
}

bool PublishCall(EventKind kind, Phase phase, void* return_address,
                 std::uint32_t original_calls) noexcept
{
    if (!Armed())
        return false;
    PublishObservedCall(kind, phase, return_address, original_calls);
    return true;
}

bool FillInputDevice(NativeEvent& event, void* self, bool require_known = true) noexcept
{
    __try
    {
        void* vtable = self ? *reinterpret_cast<void**>(self) : nullptr;
        if (vtable == g_main_base + kKeyboardVtableRva)
            event.flags |= kFlagKeyboardDevice;
        else if (vtable == g_main_base + kGamepadVtableRva)
            event.flags |= kFlagGamepadDevice;
        else
        {
            if (require_known)
                ReportUnsafe(ErrorCode::HookTransaction);
            return false;
        }
        event.device_key = *reinterpret_cast<const std::uint32_t*>(
            static_cast<const std::uint8_t*>(self) + 0x0C);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        ReportUnsafe(ErrorCode::HookTransaction);
        return false;
    }
}

bool ReadAlternateFlag(void* self, std::uint32_t& value) noexcept
{
    __try
    {
        value = *reinterpret_cast<const std::uint8_t*>(
            static_cast<const std::uint8_t*>(self) + 0x4B130);
        return value <= 1;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

void PublishObservedDeviceCall(EventKind kind, Phase phase, void* return_address,
                               std::uint32_t original_calls, void* self,
                               bool result, bool has_result) noexcept
{
    NativeEvent event = Event(kind, phase);
    event.return_rva = ReturnRva(return_address);
    event.original_calls = original_calls;
    if (original_calls)
        event.flags |= kFlagOriginalCalled;
    if (has_result && result)
        event.flags |= kFlagResultTrue;
    FillInputDevice(event, self, false);
    Publish(event);
}

template <typename Function>
void InvokeVoid(Function original, EventKind enter, EventKind exit, Phase phase,
                void* return_address) noexcept
{
    HookScope scope;
    PhaseScope phase_scope(phase);
    const bool observe = Armed() && EstablishOrCheckMainThread(false, true);
    const bool entered = observe && PublishCall(enter, phase, return_address, 0);
    original();
    if (entered)
        PublishObservedCall(exit, phase, return_address, 1);
}

template <typename Function>
void InvokeThisVoid(Function original, void* self, EventKind enter, EventKind exit,
                    Phase phase, void* return_address) noexcept
{
    HookScope scope;
    PhaseScope phase_scope(phase);
    const bool observe = Armed() && EstablishOrCheckMainThread(false, true);
    const bool entered = observe && PublishCall(enter, phase, return_address, 0);
    original(self);
    if (entered)
        PublishObservedCall(exit, phase, return_address, 1);
}

bool IsTargetImage() noexcept
{
    wchar_t path[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(nullptr, path, _countof(path));
    if (!length || length >= _countof(path))
        return false;
    const wchar_t* name = wcsrchr(path, L'\\');
    name = name ? name + 1 : path;
    if (_wcsicmp(name, L"isaac-ng.exe") != 0)
        return false;
    __try
    {
        return target_image::IsApproved(g_main_base);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool HasConflictingRuntime() noexcept
{
    return IsDebuggerPresent() != FALSE || GetModuleHandleW(L"IsaacSync.Native.dll") != nullptr ||
           GetModuleHandleW(L"tractor_beam_native_hook.dll") != nullptr ||
           GetModuleHandleW(L"x32dbg.dll") != nullptr || GetModuleHandleW(L"x64dbg.dll") != nullptr;
}

bool ValidateMapping(bool embedded,
                     const EmbeddedObserverIdentity* expected_identity) noexcept
{
    if (!g_shared || g_shared->magic != kIpcMagic || g_shared->version != kIpcVersion ||
        g_shared->size != sizeof(SharedLoopBlock) ||
        g_shared->target_pid != GetCurrentProcessId() ||
        g_shared->probe != static_cast<std::uint32_t>(Probe::Passive) ||
        !Nonzero(g_shared->nonce))
        return false;
    return detail::CompanionMappingIdentityAllowed(
        g_shared->target_pid, g_shared->scenario, g_shared->nonce, embedded,
        expected_identity);
}

bool ValidateCallTarget(std::size_t index) noexcept
{
    __try
    {
        const auto* call = g_main_base + kCallRvas[index];
        if (std::memcmp(call, kCallBytes[index].data(), 5) != 0 || call[0] != 0xE8)
            return false;
        const auto displacement = *reinterpret_cast<const std::int32_t*>(call + 1);
        return call + 5 + displacement == g_main_base + kTargetRvas[index];
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool ValidateInputVtables() noexcept
{
    __try
    {
        const std::array<std::uint32_t, 2> vtable_rvas{{kKeyboardVtableRva,
                                                        kGamepadVtableRva}};
        const std::array<std::uint32_t, 2> col_rvas{{kKeyboardColRva, kGamepadColRva}};
        const std::array<std::uint32_t, 3> query_rvas{{kPressedRva, kTriggeredRva,
                                                       kScalarRva}};
        const std::array<std::uint32_t, 2> poll_rvas{{kKeyboardPollRva, kGamepadPollRva}};
        for (std::size_t device = 0; device < vtable_rvas.size(); ++device)
        {
            auto* vtable = g_main_base + vtable_rvas[device];
            auto** slots = reinterpret_cast<void**>(vtable);
            if (slots[-1] != g_main_base + col_rvas[device])
            {
                g_install_failure = {
                    static_cast<std::uint32_t>(InstallFailureStage::VtableCol),
                    static_cast<std::uint32_t>(device), ERROR_INVALID_DATA};
                return false;
            }
            for (std::size_t query = 0; query < query_rvas.size(); ++query)
            {
                const std::size_t slot = (0x30 / sizeof(void*)) + query;
                if (slots[slot] != g_main_base + query_rvas[query])
                {
                    g_install_failure = {
                        static_cast<std::uint32_t>(InstallFailureStage::VtableQuery),
                        static_cast<std::uint32_t>(device * query_rvas.size() + query),
                        ERROR_INVALID_DATA};
                    return false;
                }
            }
            const std::size_t poll_slot = 0x78 / sizeof(void*);
            if (slots[poll_slot] != g_main_base + poll_rvas[device])
            {
                g_install_failure = {
                    static_cast<std::uint32_t>(InstallFailureStage::VtableQuery),
                    static_cast<std::uint32_t>(6 + device), ERROR_INVALID_DATA};
                return false;
            }
        }
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        g_install_failure = {
            static_cast<std::uint32_t>(InstallFailureStage::VtableCol), UINT32_MAX,
            ERROR_INVALID_ACCESS};
        return false;
    }
}

bool InstallHooks(const EmbeddedObserverHooks& additional) noexcept
{
    g_install_failure = {};
    g_relative_patch_count = 0;
    g_pointer_patch_count = 0;
    g_ordered_patch_count = 0;
    InterlockedExchange(&g_strict_ordered_transaction, 0);
    if (additional.entry_count > 19 || (additional.entry_count &&
        (!additional.entries || !additional.entry_ordinals || !additional.strict_ordered_transaction ||
         additional.strict_slot_snapshot_before)) ||
        additional.relative_count > kMaximumEmbeddedRelativeCount ||
        (additional.relative_count != 0 && !additional.relative) ||
        additional.pointer_count > kMaximumEmbeddedPointerCount ||
        (additional.pointer_count != 0 && !additional.pointers) ||
        (additional.strict_ordered_transaction &&
         ((additional.relative_count != 0 && !additional.relative_ordinals) ||
          (additional.pointer_count != 0 && !additional.pointer_ordinals))))
    {
        g_install_failure = {
            static_cast<std::uint32_t>(InstallFailureStage::PatchTransactionBase) +
                static_cast<std::uint32_t>(
                    isaac_cs_t0::mode::PatchFailureStage::InvalidArguments),
            UINT32_MAX, ERROR_INVALID_PARAMETER};
        return false;
    }
    const std::array<void*, kRelativeHookCount> replacements{{
        reinterpret_cast<void*>(&LoopClockHook), reinterpret_cast<void*>(&UpdateHook),
        reinterpret_cast<void*>(&LoopGameplayHook), reinterpret_cast<void*>(&PostUpdateHook),
        reinterpret_cast<void*>(&RunTeardownHook), reinterpret_cast<void*>(&RunTeardownHook),
        reinterpret_cast<void*>(&NewRoomHook), reinterpret_cast<void*>(&RenderHook),
        reinterpret_cast<void*>(&InputManagerSampleHook),
        reinterpret_cast<void*>(&InputDeviceUpdateHook),
        reinterpret_cast<void*>(&AlternateOwnerHook),
        reinterpret_cast<void*>(&AlternateRoomHook),
        reinterpret_cast<void*>(&AlternateTailHook),
        reinterpret_cast<void*>(&StandardPredicateHook),
        reinterpret_cast<void*>(&StandardBodyHook),
        reinterpret_cast<void*>(&StandardRoomHook),
        reinterpret_cast<void*>(&EntityListHook),
        reinterpret_cast<void*>(&AlternateStateApplyHook),
        reinterpret_cast<void*>(&AlternateOutcomeHook),
        reinterpret_cast<void*>(&AlternateOutcomeHook),
        reinterpret_cast<void*>(&AlternateOutcomeHook),
        reinterpret_cast<void*>(&AlternateOutcomeHook),
        reinterpret_cast<void*>(&AlternateOutcomeHook),
        reinterpret_cast<void*>(&StandardRoomLifecycleOwnerHook),
        reinterpret_cast<void*>(&StandardRoomLifecycleSimulationHook),
        reinterpret_cast<void*>(&RoomChangeOwnerHook),
        reinterpret_cast<void*>(&RoomChangeSimulationHook),
    }};
    for (std::size_t index = 0; index < kRelativeHookCount; ++index)
    {
        if (!ValidateCallTarget(index))
        {
            g_install_failure = {
                static_cast<std::uint32_t>(InstallFailureStage::CallTarget),
                static_cast<std::uint32_t>(index), ERROR_INVALID_DATA};
            return false;
        }
        auto& patch = g_relative_patches[index];
        patch.call = g_main_base + kCallRvas[index];
        std::copy(kCallBytes[index].begin(), kCallBytes[index].end(), patch.expected);
        patch.replacement = replacements[index];
    }
    for (std::size_t index = 0; index < additional.relative_count; ++index)
        g_relative_patches[kRelativeHookCount + index] = additional.relative[index];
    g_relative_patch_count = kRelativeHookCount + additional.relative_count;

    void** peek = isaac_cs_t0::mode::FindIatSlot(g_main_module, "PeekMessageW");
    void** translate = isaac_cs_t0::mode::FindIatSlot(g_main_module, "TranslateMessage");
    void** dispatch = isaac_cs_t0::mode::FindIatSlot(g_main_module, "DispatchMessageW");
    const std::array<void**, 3> iat_slots{{peek, translate, dispatch}};
    const std::array<std::uint32_t, 3> iat_rvas{{kPeekIatRva, kTranslateIatRva,
                                                kDispatchIatRva}};
    for (std::size_t index = 0; index < iat_slots.size(); ++index)
    {
        if (!iat_slots[index])
        {
            g_install_failure = {
                static_cast<std::uint32_t>(InstallFailureStage::IatLookup),
                static_cast<std::uint32_t>(index), ERROR_PROC_NOT_FOUND};
            return false;
        }
        if (iat_slots[index] != reinterpret_cast<void**>(g_main_base + iat_rvas[index]))
        {
            g_install_failure = {
                static_cast<std::uint32_t>(InstallFailureStage::IatRva),
                static_cast<std::uint32_t>(index), ERROR_INVALID_ADDRESS};
            return false;
        }
        if (!isaac_cs_t0::mode::IsAddressOwnedBy(*iat_slots[index], L"USER32.dll"))
        {
            g_install_failure = {
                static_cast<std::uint32_t>(InstallFailureStage::IatOwner),
                static_cast<std::uint32_t>(index), ERROR_INVALID_DATA};
            return false;
        }
    }
    if (!ValidateInputVtables())
        return false;

    g_clock = reinterpret_cast<ClockFn>(g_main_base + kTargetRvas[0]);
    g_update = reinterpret_cast<VoidFn>(g_main_base + kTargetRvas[1]);
    g_gameplay = reinterpret_cast<ThisVoidFn>(g_main_base + kTargetRvas[2]);
    g_post_update = reinterpret_cast<VoidFn>(g_main_base + kTargetRvas[3]);
    g_run_teardown = reinterpret_cast<LifecycleFn>(g_main_base + kTargetRvas[4]);
    g_new_room = reinterpret_cast<VoidFn>(g_main_base + kTargetRvas[6]);
    g_render = reinterpret_cast<VoidFn>(g_main_base + kTargetRvas[7]);
    g_input_manager_sample = reinterpret_cast<ThisVoidFn>(g_main_base + kTargetRvas[8]);
    g_input_device_update = reinterpret_cast<ThisBoolFn>(g_main_base + kTargetRvas[9]);
    g_alternate_owner = reinterpret_cast<ThisVoidFn>(g_main_base + kTargetRvas[10]);
    g_alternate_room = reinterpret_cast<ThisVoidFn>(g_main_base + kTargetRvas[11]);
    g_alternate_tail = reinterpret_cast<PostGameStartFn>(g_main_base + kTargetRvas[12]);
    g_standard_predicate = reinterpret_cast<ThisBoolFn>(g_main_base + kTargetRvas[13]);
    g_standard_body = reinterpret_cast<ThisVoidFn>(g_main_base + kTargetRvas[14]);
    g_standard_room = reinterpret_cast<ThisVoidFn>(g_main_base + kTargetRvas[15]);
    g_entity_list = reinterpret_cast<ThisArgVoidFn>(g_main_base + kTargetRvas[16]);
    g_alternate_state_apply =
        reinterpret_cast<ThisTwoArgVoidFn>(g_main_base + kTargetRvas[17]);
    g_standard_room_lifecycle_owner =
        reinterpret_cast<ThisVoidFn>(g_main_base + kTargetRvas[23]);
    g_standard_room_lifecycle_simulation =
        reinterpret_cast<ThisVoidFn>(g_main_base + kTargetRvas[24]);
    g_room_change_owner =
        reinterpret_cast<ThisVoidFn>(g_main_base + kTargetRvas[25]);
    g_room_change_simulation =
        reinterpret_cast<ThisVoidFn>(g_main_base + kTargetRvas[26]);
    g_keyboard_poll = reinterpret_cast<ThisBoolFn>(g_main_base + kKeyboardPollRva);
    g_gamepad_poll = reinterpret_cast<ThisBoolFn>(g_main_base + kGamepadPollRva);
    g_peek_message = reinterpret_cast<PeekMessageFn>(*peek);
    g_translate_message = reinterpret_cast<TranslateMessageFn>(*translate);
    g_dispatch_message = reinterpret_cast<DispatchMessageFn>(*dispatch);

    g_pointer_patches[0] = {
        reinterpret_cast<void**>(g_main_base + kKeyboardVtableRva + 0x78),
        reinterpret_cast<void*>(&KeyboardPollHook), nullptr, L"isaac-ng.exe", false,
        g_main_base + kKeyboardPollRva};
    g_pointer_patches[1] = {
        reinterpret_cast<void**>(g_main_base + kGamepadVtableRva + 0x78),
        reinterpret_cast<void*>(&GamepadPollHook), nullptr, L"isaac-ng.exe", false,
        g_main_base + kGamepadPollRva};
    g_pointer_patches[2] = {peek, reinterpret_cast<void*>(&PeekMessageHook), nullptr,
                            L"USER32.dll", false};
    g_pointer_patches[3] = {translate, reinterpret_cast<void*>(&TranslateMessageHook), nullptr,
                            L"USER32.dll", false};
    g_pointer_patches[4] = {dispatch, reinterpret_cast<void*>(&DispatchMessageHook), nullptr,
                            L"USER32.dll", false};
    for (std::size_t index = 0; index < additional.pointer_count; ++index)
        g_pointer_patches[kPointerHookCount + index] = additional.pointers[index];
    g_pointer_patch_count = kPointerHookCount + additional.pointer_count;

    isaac_cs_t0::mode::PatchTransactionFailure patch_failure{};
    bool installed = false;
    if (additional.strict_ordered_transaction)
    {
        const std::size_t total = kHookCount + additional.relative_count +
                                  additional.pointer_count + additional.entry_count;
        if (total > g_ordered_patches.size())
        {
            g_install_failure = {
                static_cast<std::uint32_t>(InstallFailureStage::PatchTransactionBase) +
                    static_cast<std::uint32_t>(
                        isaac_cs_t0::mode::PatchFailureStage::InvalidArguments),
                UINT32_MAX, ERROR_INSUFFICIENT_BUFFER};
            return false;
        }
        g_ordered_patches.fill({});
        for (std::size_t index = 0; index < kRelativeHookCount; ++index)
            g_ordered_patches[index] = {
                static_cast<std::uint32_t>(index), &g_relative_patches[index], nullptr};
        for (std::size_t index = 0; index < kPointerHookCount; ++index)
        {
            const std::size_t ordinal = kRelativeHookCount + index;
            g_ordered_patches[ordinal] = {
                static_cast<std::uint32_t>(ordinal), nullptr,
                &g_pointer_patches[index]};
        }
        const auto place_relative = [&](std::size_t index) noexcept {
            const std::uint32_t ordinal = additional.relative_ordinals[index];
            if (ordinal < kHookCount || ordinal >= total ||
                g_ordered_patches[ordinal].relative ||
                g_ordered_patches[ordinal].pointer || g_ordered_patches[ordinal].entry)
                return false;
            g_ordered_patches[ordinal] = {
                ordinal, &g_relative_patches[kRelativeHookCount + index], nullptr};
            return true;
        };
        const auto place_pointer = [&](std::size_t index) noexcept {
            const std::uint32_t ordinal = additional.pointer_ordinals[index];
            if (ordinal < kHookCount || ordinal >= total ||
                g_ordered_patches[ordinal].relative ||
                g_ordered_patches[ordinal].pointer || g_ordered_patches[ordinal].entry)
                return false;
            g_ordered_patches[ordinal] = {
                ordinal, nullptr, &g_pointer_patches[kPointerHookCount + index]};
            return true;
        };
        for (std::size_t index = 0; index < additional.relative_count; ++index)
            if (!place_relative(index))
            {
                g_install_failure = {
                    static_cast<std::uint32_t>(InstallFailureStage::PatchTransactionBase) +
                        static_cast<std::uint32_t>(
                            isaac_cs_t0::mode::PatchFailureStage::InvalidArguments),
                    static_cast<std::uint32_t>(index), ERROR_INVALID_PARAMETER};
                return false;
            }
        for (std::size_t index = 0; index < additional.pointer_count; ++index)
            if (!place_pointer(index))
            {
                g_install_failure = {
                    static_cast<std::uint32_t>(InstallFailureStage::PatchTransactionBase) +
                        static_cast<std::uint32_t>(
                            isaac_cs_t0::mode::PatchFailureStage::InvalidArguments),
                    static_cast<std::uint32_t>(index), ERROR_INVALID_PARAMETER};
                return false;
            }
        for (std::size_t index=0;index<additional.entry_count;++index) {
            const auto ordinal=additional.entry_ordinals[index];
            if(ordinal<kHookCount || ordinal>=total || g_ordered_patches[ordinal].relative ||
                g_ordered_patches[ordinal].pointer || g_ordered_patches[ordinal].entry) return false;
            g_ordered_patches[ordinal]={ordinal,nullptr,nullptr,&additional.entries[index]};
        }
        for (std::size_t ordinal = 0; ordinal < total; ++ordinal)
            if (g_ordered_patches[ordinal].ordinal != ordinal ||
                (int(g_ordered_patches[ordinal].relative!=nullptr)+
                 int(g_ordered_patches[ordinal].pointer!=nullptr)+
                 int(g_ordered_patches[ordinal].entry!=nullptr))!=1)
            {
                g_install_failure = {
                    static_cast<std::uint32_t>(InstallFailureStage::PatchTransactionBase) +
                        static_cast<std::uint32_t>(
                            isaac_cs_t0::mode::PatchFailureStage::InvalidArguments),
                    static_cast<std::uint32_t>(ordinal), ERROR_INVALID_PARAMETER};
                return false;
            }
        g_ordered_patch_count = total;
        if (additional.strict_slot_snapshot_before &&
            !SnapshotOrderedSlots(*additional.strict_slot_snapshot_before))
        {
            g_install_failure = {
                static_cast<std::uint32_t>(InstallFailureStage::PatchTransactionBase) +
                    static_cast<std::uint32_t>(
                        isaac_cs_t0::mode::PatchFailureStage::PrepareRelative),
                UINT32_MAX, ERROR_INVALID_DATA};
            g_ordered_patch_count = 0;
            return false;
        }
        installed = isaac_cs_t0::mode::InstallOrderedPatchTransaction(
            g_ordered_patches.data(), total, &patch_failure);
        if (installed)
        {
            InterlockedExchange(&g_strict_ordered_transaction, 1);
        }
    }
    else
    {
        installed = isaac_cs_t0::mode::InstallPatchTransaction(
            g_relative_patches.data(), g_relative_patch_count,
            g_pointer_patches.data(), g_pointer_patch_count, &patch_failure);
    }
    if (!installed)
    {
        g_install_failure = {
            static_cast<std::uint32_t>(InstallFailureStage::PatchTransactionBase) +
                static_cast<std::uint32_t>(patch_failure.stage),
            patch_failure.index, patch_failure.win32_error};
        return false;
    }
    for (std::size_t index = 0; index < additional.relative_count; ++index)
    {
        additional.relative[index].original =
            g_relative_patches[kRelativeHookCount + index].original;
        additional.relative[index].original_displacement =
            g_relative_patches[kRelativeHookCount + index].original_displacement;
        additional.relative[index].replacement_displacement =
            g_relative_patches[kRelativeHookCount + index].replacement_displacement;
        additional.relative[index].installed =
            g_relative_patches[kRelativeHookCount + index].installed;
    }
    for (std::size_t index = 0; index < additional.pointer_count; ++index)
    {
        additional.pointers[index].original =
            g_pointer_patches[kPointerHookCount + index].original;
        additional.pointers[index].installed =
            g_pointer_patches[kPointerHookCount + index].installed;
    }
    g_embedded_input_relative_patches = additional.relative;
    g_embedded_input_relative_patch_count = additional.relative_count;
    g_embedded_input_patches = additional.pointers;
    g_embedded_input_patch_count = additional.pointer_count;
    g_physical_poll_observation = additional.physical_poll;
    g_physical_poll_observation_context = additional.physical_poll_context;
    g_device_update_complete_observation = additional.device_update_complete;
    g_device_update_complete_observation_context =
        additional.device_update_complete_context;
    g_physical_exit_observation = additional.physical_exit;
    g_physical_exit_observation_context = additional.physical_exit_context;
    g_authority_observation = additional.authority;
    g_authority_observation_context = additional.authority_context;
    g_top_update_exit_observation = additional.top_update_exit;
    g_top_update_exit_observation_context = additional.top_update_exit_context;
    g_lifecycle_observation = additional.lifecycle;
    g_lifecycle_observation_context = additional.lifecycle_context;
    InterlockedExchange64(&g_shared->hooks_mask, static_cast<LONG64>(kCompleteHookMask));
    return true;
}

bool AnyHookInstalled() noexcept
{
    return std::any_of(g_relative_patches.begin(),
                       g_relative_patches.begin() + g_relative_patch_count,
                       [](const auto& patch) { return patch.installed; }) ||
           std::any_of(g_ordered_patches.begin(),g_ordered_patches.begin()+g_ordered_patch_count,
                       [](const auto& patch){return patch.entry && patch.entry->installed;}) ||
           std::any_of(g_pointer_patches.begin(),
                       g_pointer_patches.begin() + g_pointer_patch_count,
                       [](const auto& patch) { return patch.installed; });
}

bool SnapshotOrderedSlots(std::array<std::uint8_t, 32>& digest) noexcept
{
    if (g_ordered_patch_count == 0 ||
        g_ordered_patch_count > g_ordered_patches.size())
        return false;
    std::array<std::uint8_t, kMaximumOrderedPatchCount * 5> bytes{};
    std::size_t size = 0;
    __try
    {
        for (std::size_t index = 0; index < g_ordered_patch_count; ++index)
        {
            const auto& descriptor = g_ordered_patches[index];
            if (descriptor.ordinal != index)
                return false;
            if (descriptor.relative && !descriptor.pointer)
            {
                std::memcpy(bytes.data() + size, descriptor.relative->call, 5);
                size += 5;
            }
            else if (descriptor.pointer && !descriptor.relative)
            {
                const auto value = reinterpret_cast<std::uintptr_t>(
                    *descriptor.pointer->slot);
                std::memcpy(bytes.data() + size, &value, sizeof(value));
                size += sizeof(value);
            }
            else
            {
                return false;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
    digest = isaac_cs_t0::mode::Sha256(bytes.data(), size);
    return true;
}

bool RestoreHooks() noexcept
{
    const bool strict =
        InterlockedCompareExchange(&g_strict_ordered_transaction, 0, 0) != 0;
    const bool restored = strict
                              ? isaac_cs_t0::mode::RestoreOrderedPatchTransaction(
                                    g_ordered_patches.data(), kHookCount)
                              : isaac_cs_t0::mode::RestorePatchTransaction(
                                    g_relative_patches.data(), g_relative_patch_count,
                                    g_pointer_patches.data(), g_pointer_patch_count);
    if (restored)
        InterlockedExchange64(&g_shared->hooks_mask, 0);
    return restored;
}

extern "C" __m128d __cdecl LoopClockHook()
{
    HookScope scope;
    PhaseScope phase_scope(Phase::LoopBoundary);
    const bool observe = Armed();
    bool entered = false;
    if (observe)
    {
        EstablishOrCheckMainThread(true, true);
        if (Armed())
        {
            InterlockedIncrement64(&g_shared->current_loop_iteration);
            entered = PublishCall(EventKind::LoopBegin, Phase::LoopBoundary,
                                  _ReturnAddress(), 0);
        }
    }
    const __m128d result = g_clock();
    if (entered)
        PublishObservedCall(EventKind::LoopClockExit, Phase::LoopBoundary,
                            _ReturnAddress(), 1);
    return result;
}

void __cdecl UpdateHook()
{
    HookScope scope;
    PhaseScope phase_scope(Phase::GameplayUpdate);
    void* const return_address = _ReturnAddress();
    const bool observe = Armed() && EstablishOrCheckMainThread(false, true);
    const bool entered = observe && PublishCall(EventKind::UpdateEnter,
                                                Phase::GameplayUpdate,
                                                return_address, 0);
    if(entered) world::StageEvent(world::Stage::Outer,true);
    g_update();
    if(entered) world::StageEvent(world::Stage::Outer,false);
    if(entered) world::e2e::Record(world::e2e::Kind::TopExit,0);
    if (entered && g_top_update_exit_observation)
        g_top_update_exit_observation(return_address,
                                      g_top_update_exit_observation_context);
    if (entered)
        PublishObservedCall(EventKind::UpdateExit, Phase::GameplayUpdate,
                            return_address, 1);
}

extern "C" void __fastcall LoopGameplayHook(void* self, void*)
{
    world::StageEvent(world::Stage::Owner,true,self);
    InvokeThisVoid(g_gameplay, self, EventKind::ConditionalAttemptEnter,
                   EventKind::ConditionalAttemptExit,
                    Phase::GameplayUpdate, _ReturnAddress());
    world::StageEvent(world::Stage::Owner,false,self);
}

void __cdecl PostUpdateHook()
{
    world::StageEvent(world::Stage::Tail,true);
    InvokeVoid(g_post_update, EventKind::StandardTailEnter, EventKind::StandardTailExit,
                Phase::PostUpdate, _ReturnAddress());
    world::StageEvent(world::Stage::Tail,false);
}

std::uint32_t __stdcall RunTeardownHook(std::uint32_t argument)
{
    HookScope scope;
    PhaseScope phase_scope(Phase::RunLifecycle);
    const bool observe = Armed() && EstablishOrCheckMainThread(false, true);
    const bool entered = observe && PublishCall(EventKind::RunTeardownEnter,
                                                Phase::RunLifecycle, _ReturnAddress(), 0);
    if (entered && g_lifecycle_observation)
        g_lifecycle_observation(EmbeddedLifecycleSignal::RunTeardown, true, nullptr,
                                _ReturnAddress(), g_lifecycle_observation_context);
    const std::uint32_t result = g_run_teardown(argument);
    if (entered && g_lifecycle_observation)
        g_lifecycle_observation(EmbeddedLifecycleSignal::RunTeardown, false, nullptr,
                                _ReturnAddress(), g_lifecycle_observation_context);
    if (entered)
        PublishObservedCall(EventKind::RunTeardownExit, Phase::RunLifecycle,
                            _ReturnAddress(), 1);
    return result;
}

void __cdecl NewRoomHook()
{
    HookScope scope;
    PhaseScope phase_scope(Phase::RoomLifecycle);
    void* const return_address = _ReturnAddress();
    const bool observe = Armed() && EstablishOrCheckMainThread(false, true);
    const bool entered = observe && PublishCall(EventKind::NewRoomEnter,
                                                Phase::RoomLifecycle,
                                                return_address, 0);
    if (entered && g_lifecycle_observation)
        g_lifecycle_observation(EmbeddedLifecycleSignal::NewRoom, true, nullptr,
                                return_address, g_lifecycle_observation_context);
    g_new_room();
    if (entered && g_lifecycle_observation)
        g_lifecycle_observation(EmbeddedLifecycleSignal::NewRoom, false, nullptr,
                                return_address, g_lifecycle_observation_context);
    if (entered)
        PublishObservedCall(EventKind::NewRoomExit, Phase::RoomLifecycle,
                            return_address, 1);
}

void __cdecl RenderHook()
{
    InvokeVoid(g_render, EventKind::RenderEnter, EventKind::RenderExit, Phase::Render,
               _ReturnAddress());
}

extern "C" void __fastcall InputManagerSampleHook(void* self, void*)
{
    InvokeThisVoid(g_input_manager_sample, self, EventKind::InputManagerSampleEnter,
                   EventKind::InputManagerSampleExit, Phase::InputQuery, _ReturnAddress());
}

extern "C" bool __fastcall InputDeviceUpdateHook(void* self, void*)
{
    HookScope scope;
    PhaseScope phase_scope(Phase::InputQuery);
    const bool observe = Armed() && EstablishOrCheckMainThread(false, true);
    bool entered = false;
    if (observe && Armed())
    {
        PublishObservedDeviceCall(EventKind::InputDeviceUpdateEnter, Phase::InputQuery,
                                  _ReturnAddress(), 0, self, false, false);
        entered = true;
    }
    const bool result = g_input_device_update(self);
    if (entered && g_device_update_complete_observation)
        g_device_update_complete_observation(
            self, result, _ReturnAddress(), g_device_update_complete_observation_context);
    if (entered)
        PublishObservedDeviceCall(EventKind::InputDeviceUpdateExit, Phase::InputQuery,
                                  _ReturnAddress(), 1, self, result, true);
    return result;
}

extern "C" void __fastcall AlternateOwnerHook(void* self, void*)
{
    HookScope scope;
    PhaseScope phase_scope(Phase::GameplayUpdate);
    const bool observe = Armed() && EstablishOrCheckMainThread(false, true);
    std::uint32_t before = 0;
    const bool readable = ReadAlternateFlag(self, before);
    if (observe && !readable)
        ReportUnsafe(ErrorCode::HookTransaction);
    const bool entered = observe && readable && Armed();
    bool tracked = false;
    if (entered)
    {
        NativeEvent event = Event(EventKind::AlternateOwnerEnter, Phase::GameplayUpdate);
        event.return_rva = ReturnRva(_ReturnAddress());
        event.value_bits = before;
        Publish(event);
        tracked = g_tls_alternate_envelope.BeginOwner() !=
                  detail::AlternateEnvelopeTransition::Invalid;
        if (!tracked)
        {
            g_tls_alternate_envelope.Reset();
            g_tls_alternate_owner = nullptr;
            ReportUnsafe(ErrorCode::HookTransaction);
        }
        else
            g_tls_alternate_owner = self;
    }
    g_alternate_owner(self);
    if (entered)
    {
        if (tracked)
        {
            const auto transition = g_tls_alternate_envelope.EndOwner();
            if (transition == detail::AlternateEnvelopeTransition::Close &&
                Armed() && g_lifecycle_observation)
                g_lifecycle_observation(
                    EmbeddedLifecycleSignal::AlternateEnvelope, false,
                    g_tls_alternate_owner, _ReturnAddress(),
                    g_lifecycle_observation_context);
            if (transition == detail::AlternateEnvelopeTransition::Invalid)
                ReportUnsafe(ErrorCode::HookTransaction);
            g_tls_alternate_owner = nullptr;
        }
        std::uint32_t after = 0;
        const bool after_readable = ReadAlternateFlag(self, after);
        if (!after_readable)
        {
            ReportUnsafe(ErrorCode::HookTransaction);
            return;
        }
        NativeEvent event = Event(EventKind::AlternateOwnerExit, Phase::GameplayUpdate);
        event.return_rva = ReturnRva(_ReturnAddress());
        event.value_bits = after;
        event.original_calls = 1;
        event.flags |= kFlagOriginalCalled;
        Publish(event);
    }
}

extern "C" void __fastcall AlternateRoomHook(void* self, void*)
{
    InvokeThisVoid(g_alternate_room, self, EventKind::AlternateRoomEnter,
                   EventKind::AlternateRoomExit, Phase::EntityMutation, _ReturnAddress());
}

extern "C" void __fastcall AlternateStateApplyHook(void* self, void*, void* state,
                                                     std::uint32_t flag)
{
    HookScope scope;
    PhaseScope phase_scope(Phase::EntityMutation);
    const bool observe = Armed() && EstablishOrCheckMainThread(false, true);
    const bool entered = observe && PublishCall(EventKind::AlternateStateApplyEnter,
                                                Phase::EntityMutation, _ReturnAddress(), 0);
    if (entered &&
        g_tls_alternate_envelope.ObserveStateApply() ==
            detail::AlternateEnvelopeTransition::Open &&
        g_lifecycle_observation)
        g_lifecycle_observation(EmbeddedLifecycleSignal::AlternateEnvelope, true,
                                g_tls_alternate_owner, _ReturnAddress(),
                                g_lifecycle_observation_context);
    g_alternate_state_apply(self, state, flag);
    if (entered)
        PublishObservedCall(EventKind::AlternateStateApplyExit, Phase::EntityMutation,
                            _ReturnAddress(), 1);
}

void __stdcall AlternateTailHook(std::uint32_t argument)
{
    HookScope scope;
    PhaseScope phase_scope(Phase::RunLifecycle);
    const bool observe = Armed() && EstablishOrCheckMainThread(false, true);
    const bool entered = observe && PublishCall(EventKind::AlternateTailEnter,
                                                Phase::RunLifecycle, _ReturnAddress(), 0);
    g_alternate_tail(argument);
    if (entered)
        PublishObservedCall(EventKind::AlternateTailExit, Phase::RunLifecycle,
                            _ReturnAddress(), 1);
}

void __stdcall AlternateOutcomeHook(std::uint32_t argument)
{
    HookScope scope;
    PhaseScope phase_scope(Phase::RunLifecycle);
    const bool observe = Armed() && EstablishOrCheckMainThread(false, true);
    const bool entered = observe && PublishCall(EventKind::AlternateOutcomeEnter,
                                                Phase::RunLifecycle, _ReturnAddress(), 0);
    g_alternate_tail(argument);
    if (entered)
        PublishObservedCall(EventKind::AlternateOutcomeExit, Phase::RunLifecycle,
                            _ReturnAddress(), 1);
}

extern "C" bool __fastcall StandardPredicateHook(void* self, void*)
{
    HookScope scope;
    PhaseScope phase_scope(Phase::GameplayUpdate);
    const bool observe = Armed() && EstablishOrCheckMainThread(false, true);
    const bool entered = observe && PublishCall(EventKind::StandardPredicateEnter,
                                                Phase::GameplayUpdate, _ReturnAddress(), 0);
    const bool result = g_standard_predicate(self);
    if (entered)
    {
        NativeEvent event = Event(EventKind::StandardPredicateExit, Phase::GameplayUpdate);
        event.return_rva = ReturnRva(_ReturnAddress());
        event.original_calls = 1;
        event.flags |= kFlagOriginalCalled;
        if (result)
            event.flags |= kFlagResultTrue;
        Publish(event);
    }
    return result;
}

extern "C" void __fastcall StandardBodyHook(void* self, void*)
{
    InvokeThisVoid(g_standard_body, self, EventKind::StandardBodyEnter,
                   EventKind::StandardBodyExit, Phase::GameplayUpdate, _ReturnAddress());
}

extern "C" void __fastcall StandardRoomHook(void* self, void*)
{
    HookScope scope;
    PhaseScope phase_scope(Phase::EntityMutation);
    void* const return_address = _ReturnAddress();
    const bool observe = Armed() && EstablishOrCheckMainThread(false, true);
    const bool entered = observe && PublishCall(EventKind::StandardRoomEnter,
                                                Phase::EntityMutation,
                                                return_address, 0);
    if(entered) world::StageEvent(world::Stage::Room,true,self);
    // V5 enter is already visible before active pre.  Active post runs before V5 exit so
    // its immutable authority snapshot cannot leak beyond the room update.
    if (entered && g_authority_observation)
        g_authority_observation(true, self, return_address,
                                g_authority_observation_context);
    g_standard_room(self);
    if (entered && g_authority_observation)
        g_authority_observation(false, self, return_address,
                                g_authority_observation_context);
    if(entered) world::StageEvent(world::Stage::Room,false,self);
    if (entered)
        PublishObservedCall(EventKind::StandardRoomExit, Phase::EntityMutation,
                            return_address, 1);
}

extern "C" void __fastcall StandardRoomLifecycleOwnerHook(void* self, void*)
{
    HookScope scope;
    PhaseScope phase_scope(Phase::RoomLifecycle);
    void* const return_address = _ReturnAddress();
    const bool observe = Armed() && EstablishOrCheckMainThread(false, true);
    const bool entered = observe &&
                         PublishCall(EventKind::StandardRoomLifecycleOwnerEnter,
                                     Phase::RoomLifecycle, return_address, 0);
    if (entered && g_lifecycle_observation)
        g_lifecycle_observation(EmbeddedLifecycleSignal::RoomLifecycle, true, self,
                                return_address, g_lifecycle_observation_context);
    g_standard_room_lifecycle_owner(self);
    if (entered && g_lifecycle_observation)
        g_lifecycle_observation(EmbeddedLifecycleSignal::RoomLifecycle, false, self,
                                return_address, g_lifecycle_observation_context);
    if (entered)
        PublishObservedCall(EventKind::StandardRoomLifecycleOwnerExit,
                            Phase::RoomLifecycle, return_address, 1);
}

extern "C" void __fastcall StandardRoomLifecycleSimulationHook(void* self, void*)
{
    InvokeThisVoid(g_standard_room_lifecycle_simulation, self,
                   EventKind::StandardRoomLifecycleSimulationEnter,
                   EventKind::StandardRoomLifecycleSimulationExit, Phase::RoomLifecycle,
                   _ReturnAddress());
}

extern "C" void __fastcall RoomChangeOwnerHook(void* self, void*)
{
    HookScope scope;
    PhaseScope phase_scope(Phase::RoomLifecycle);
    void* const return_address = _ReturnAddress();
    const bool observe = Armed() && EstablishOrCheckMainThread(false, true);
    const bool entered = observe && PublishCall(EventKind::RoomChangeOwnerEnter,
                                                Phase::RoomLifecycle,
                                                return_address, 0);
    if (entered && g_lifecycle_observation)
        g_lifecycle_observation(EmbeddedLifecycleSignal::RoomChange, true, self,
                                return_address, g_lifecycle_observation_context);
    g_room_change_owner(self);
    if (entered && g_lifecycle_observation)
        g_lifecycle_observation(EmbeddedLifecycleSignal::RoomChange, false, self,
                                return_address, g_lifecycle_observation_context);
    if (entered)
        PublishObservedCall(EventKind::RoomChangeOwnerExit, Phase::RoomLifecycle,
                            return_address, 1);
}

extern "C" void __fastcall RoomChangeSimulationHook(void* self, void*)
{
    InvokeThisVoid(g_room_change_simulation, self, EventKind::RoomChangeSimulationEnter,
                   EventKind::RoomChangeSimulationExit, Phase::RoomLifecycle,
                   _ReturnAddress());
}

extern "C" void __fastcall EntityListHook(void* self, void*, std::uint32_t argument)
{
    HookScope scope;
    PhaseScope phase_scope(Phase::EntityMutation);
    const bool observe = Armed() && EstablishOrCheckMainThread(false, true);
    const bool entered = observe && PublishCall(EventKind::EntityListEnter,
                                                Phase::EntityMutation, _ReturnAddress(), 0);
    g_entity_list(self, argument);
    if (entered)
        PublishObservedCall(EventKind::EntityListExit, Phase::EntityMutation,
                            _ReturnAddress(), 1);
}

bool InvokePhysicalPoll(ThisBoolFn original, void* self, void* return_address) noexcept
{
    HookScope scope;
    PhaseScope phase_scope(Phase::InputQuery);
    const bool observe = Armed() && EstablishOrCheckMainThread(false, true);
    bool entered = false;
    if (observe && Armed())
    {
        PublishObservedDeviceCall(EventKind::InputPhysicalPollEnter, Phase::InputQuery,
                                  return_address, 0, self, false, false);
        entered = true;
        if (g_physical_poll_observation)
            g_physical_poll_observation(true, self, false, return_address,
                                        g_physical_poll_observation_context);
    }
    const bool result = original(self);
    if (entered)
    {
        if (g_physical_poll_observation)
            g_physical_poll_observation(false, self, result, return_address,
                                        g_physical_poll_observation_context);
        if (g_physical_exit_observation)
            g_physical_exit_observation(self, result, return_address,
                                        g_physical_exit_observation_context);
        PublishObservedDeviceCall(EventKind::InputPhysicalPollExit, Phase::InputQuery,
                                  return_address, 1, self, result, true);
    }
    return result;
}

extern "C" bool __fastcall KeyboardPollHook(void* self, void*)
{
    return InvokePhysicalPoll(g_keyboard_poll, self, _ReturnAddress());
}

extern "C" bool __fastcall GamepadPollHook(void* self, void*)
{
    return InvokePhysicalPoll(g_gamepad_poll, self, _ReturnAddress());
}

BOOL WINAPI PeekMessageHook(LPMSG message, HWND window, UINT minimum, UINT maximum, UINT remove)
{
    HookScope scope;
    PhaseScope phase_scope(Phase::MessagePump);
    const BOOL result = g_peek_message(message, window, minimum, maximum, remove);
    const bool observe = Armed() && EstablishOrCheckMainThread(false, true);
    if (observe)
    {
        NativeEvent event = Event(EventKind::PeekMessage, Phase::MessagePump);
        event.original_calls = 1;
        event.flags |= kFlagOriginalCalled;
        if (result)
            event.flags |= kFlagResultTrue;
        event.return_rva = ReturnRva(_ReturnAddress());
        Publish(event);
    }
    return result;
}

BOOL WINAPI TranslateMessageHook(const MSG* message)
{
    HookScope scope;
    PhaseScope phase_scope(Phase::MessagePump);
    const BOOL result = g_translate_message(message);
    const bool observe = Armed() && EstablishOrCheckMainThread(false, true);
    if (observe)
    {
        NativeEvent event = Event(EventKind::TranslateMessage, Phase::MessagePump);
        event.original_calls = 1;
        event.flags |= kFlagOriginalCalled;
        if (result)
            event.flags |= kFlagResultTrue;
        event.return_rva = ReturnRva(_ReturnAddress());
        Publish(event);
    }
    return result;
}

LRESULT WINAPI DispatchMessageHook(const MSG* message)
{
    HookScope scope;
    PhaseScope phase_scope(Phase::MessagePump);
    const LRESULT result = g_dispatch_message(message);
    const bool observe = Armed() && EstablishOrCheckMainThread(false, true);
    if (observe)
    {
        NativeEvent event = Event(EventKind::DispatchMessage, Phase::MessagePump);
        event.original_calls = 1;
        event.flags |= kFlagOriginalCalled;
        event.value_bits = static_cast<std::uint32_t>(result);
        event.return_rva = ReturnRva(_ReturnAddress());
        Publish(event);
    }
    return result;
}

DWORD InitializeObserver(const wchar_t* mapping_name,
                         const EmbeddedObserverHooks& additional, bool embedded,
                         const EmbeddedObserverIdentity* expected_identity) noexcept
{
    if (!mapping_name || !*mapping_name || g_shared ||
        (additional.relative_count != 0 && !additional.relative) ||
        additional.relative_count > kMaximumEmbeddedRelativeCount ||
        (additional.pointer_count != 0 && !additional.pointers) ||
        additional.pointer_count > kMaximumEmbeddedPointerCount)
        return static_cast<DWORD>(ErrorCode::MappingInvalid);
    g_mapping = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, mapping_name);
    if (!g_mapping)
        return static_cast<DWORD>(ErrorCode::MappingInvalid);
    g_shared = static_cast<SharedLoopBlock*>(
        MapViewOfFile(g_mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedLoopBlock)));
    if (!ValidateMapping(embedded, expected_identity))
    {
        SetState(ErrorCode::MappingInvalid, NativeState::Failed);
        return static_cast<DWORD>(ErrorCode::MappingInvalid);
    }
    InterlockedExchange(&g_embedded, embedded ? 1 : 0);
    InterlockedExchange(&g_unloading, 0);
    InterlockedExchange(&g_armed, 0);
    InterlockedExchange(&g_unsafe_reported, 0);
    InterlockedExchange(&g_active_hooks, 0);
    InterlockedExchange(&g_embedded_input_restored, 0);
    InterlockedExchange(&g_strict_ordered_transaction, 0);
    g_ordered_patch_count = 0;
    g_embedded_input_patches = nullptr;
    g_embedded_input_patch_count = 0;
    g_embedded_input_relative_patches = nullptr;
    g_embedded_input_relative_patch_count = 0;
    g_physical_poll_observation = nullptr;
    g_physical_poll_observation_context = nullptr;
    g_device_update_complete_observation = nullptr;
    g_device_update_complete_observation_context = nullptr;
    g_physical_exit_observation = nullptr;
    g_physical_exit_observation_context = nullptr;
    g_authority_observation = nullptr;
    g_authority_observation_context = nullptr;
    g_top_update_exit_observation = nullptr;
    g_top_update_exit_observation_context = nullptr;
    g_lifecycle_observation = nullptr;
    g_lifecycle_observation_context = nullptr;
    InterlockedExchange(&g_shared->state, static_cast<LONG>(NativeState::Initializing));
    g_main_module = GetModuleHandleW(nullptr);
    g_main_base = reinterpret_cast<std::uint8_t*>(g_main_module);
    if (!IsTargetImage())
    {
        SetState(ErrorCode::TargetImageMismatch, NativeState::Failed);
        return static_cast<DWORD>(ErrorCode::TargetImageMismatch);
    }
    if (HasConflictingRuntime())
    {
        SetState(ErrorCode::ConflictingModuleOrDebugger, NativeState::Failed);
        return static_cast<DWORD>(ErrorCode::ConflictingModuleOrDebugger);
    }
    Publish(Event(EventKind::NativeInitialized, Phase::OutsideEnvelope));
    if (!InstallHooks(additional))
    {
        const bool unsafe = AnyHookInstalled();
        const ErrorCode code = unsafe ? ErrorCode::UnsafeShutdown : ErrorCode::HookTransaction;
        SetState(code, unsafe ? NativeState::Unsafe : NativeState::Failed);
        NativeEvent event = Event(EventKind::Failure, Phase::OutsideEnvelope);
        event.error_code = static_cast<std::uint32_t>(code);
        event.action = g_install_failure.stage;
        event.value_bits = g_install_failure.index;
        event.device_key = g_install_failure.win32_error;
        Publish(event);
        return static_cast<DWORD>(code);
    }
    Publish(Event(EventKind::HooksInstalled, Phase::OutsideEnvelope));
    InterlockedExchange(&g_armed, 1);
    InterlockedExchange(&g_shared->state, static_cast<LONG>(NativeState::Ready));
    return 0;
}

DWORD StopObserver() noexcept
{
    if (!g_shared || InterlockedExchange(&g_unloading, 1) != 0)
        return static_cast<DWORD>(ErrorCode::UnsafeShutdown);
    InterlockedExchange(&g_shared->state, static_cast<LONG>(NativeState::ShuttingDown));
    InterlockedExchange(&g_armed, 0);
    if (!RestoreHooks())
    {
        SetState(ErrorCode::UnsafeShutdown, NativeState::Unsafe);
        // An embedded restore failure is terminal: the owning input shutdown must not retry
        // or unload a module that can still own any of the 43 hook slots.  Preserve the legacy
        // standalone retry behavior only for the public V5 observer.
        if (InterlockedCompareExchange(&g_embedded, 0, 0) == 0)
            InterlockedExchange(&g_unloading, 0);
        return static_cast<DWORD>(ErrorCode::UnsafeShutdown);
    }
    const ULONGLONG deadline = GetTickCount64() + kHookDrainTimeoutMs;
    while (InterlockedCompareExchange(&g_active_hooks, 0, 0) != 0 &&
           GetTickCount64() < deadline)
        Sleep(1);
    if (InterlockedCompareExchange(&g_active_hooks, 0, 0) != 0)
    {
        SetState(ErrorCode::HookDrainTimeout, NativeState::Unsafe);
        return static_cast<DWORD>(ErrorCode::HookDrainTimeout);
    }
    Publish(Event(EventKind::HooksRestored, Phase::OutsideEnvelope));
    InterlockedExchange(&g_shared->producer_closed, 1);
    InterlockedExchange(&g_shared->state, static_cast<LONG>(NativeState::Stopped));
    return 0;
}

} // namespace

void SetModule(HMODULE module) noexcept
{
    g_module = module;
}

void ProcessDetach() noexcept
{
    if (g_shared)
    {
        UnmapViewOfFile(g_shared);
        g_shared = nullptr;
    }
    if (g_mapping)
    {
        CloseHandle(g_mapping);
        g_mapping = nullptr;
    }
    g_main_module = nullptr;
    g_main_base = nullptr;
    g_pointer_patch_count = 0;
    g_relative_patch_count = 0;
    g_ordered_patch_count = 0;
    g_embedded_input_relative_patches = nullptr;
    g_embedded_input_relative_patch_count = 0;
    g_embedded_input_patches = nullptr;
    g_embedded_input_patch_count = 0;
    g_physical_poll_observation = nullptr;
    g_physical_poll_observation_context = nullptr;
    g_device_update_complete_observation = nullptr;
    g_device_update_complete_observation_context = nullptr;
    g_physical_exit_observation = nullptr;
    g_physical_exit_observation_context = nullptr;
    g_authority_observation = nullptr;
    g_authority_observation_context = nullptr;
    g_top_update_exit_observation = nullptr;
    g_top_update_exit_observation_context = nullptr;
    g_lifecycle_observation = nullptr;
    g_lifecycle_observation_context = nullptr;
    InterlockedExchange(&g_embedded_input_restored, 0);
    InterlockedExchange(&g_strict_ordered_transaction, 0);
}

ObservationContext CurrentObservationContext() noexcept
{
    ObservationContext context{};
    context.thread_id = GetCurrentThreadId();
    context.phase = g_tls_phase;
    context.phase_depth = g_tls_phase_depth;
    if (g_shared)
    {
        context.loop_iteration = static_cast<std::uint64_t>(
            InterlockedCompareExchange64(&g_shared->current_loop_iteration, 0, 0));
        context.main_thread_id = static_cast<std::uint32_t>(
            InterlockedCompareExchange(&g_shared->main_thread_id, 0, 0));
    }
    return context;
}

bool CompanionHealthy() noexcept
{
    if (!g_shared || InterlockedCompareExchange(&g_embedded, 0, 0) == 0 ||
        InterlockedCompareExchange(&g_armed, 0, 0) == 0 ||
        InterlockedCompareExchange(&g_unloading, 0, 0) != 0 ||
        InterlockedCompareExchange(&g_unsafe_reported, 0, 0) != 0)
        return false;
    return InterlockedCompareExchange(&g_shared->state, 0, 0) ==
               static_cast<LONG>(NativeState::Ready) &&
           InterlockedCompareExchange64(&g_shared->hooks_mask, 0, 0) ==
               static_cast<LONG64>(kCompleteHookMask) &&
           InterlockedCompareExchange64(&g_shared->overwritten_events, 0, 0) == 0;
}

DWORD StartEmbeddedObserver(const wchar_t* mapping_name,
                            isaac_cs_t0::mode::PointerPatch* additional_pointers,
                            std::size_t additional_pointer_count,
                            const EmbeddedObserverIdentity& expected_identity) noexcept
{
    if (!additional_pointers || additional_pointer_count == 0)
        return static_cast<DWORD>(ErrorCode::MappingInvalid);
    EmbeddedObserverHooks hooks{};
    hooks.pointers = additional_pointers;
    hooks.pointer_count = additional_pointer_count;
    return InitializeObserver(mapping_name, hooks, true, &expected_identity);
}

DWORD StartEmbeddedObserver(const wchar_t* mapping_name,
                            const EmbeddedObserverHooks& additional_hooks,
                            const EmbeddedObserverIdentity& expected_identity) noexcept
{
    if ((additional_hooks.relative_count == 0 || !additional_hooks.relative) &&
        (additional_hooks.pointer_count == 0 || !additional_hooks.pointers))
        return static_cast<DWORD>(ErrorCode::MappingInvalid);
    return InitializeObserver(mapping_name, additional_hooks, true, &expected_identity);
}

DWORD RestoreEmbeddedInputHooks() noexcept
{
    if (InterlockedCompareExchange(&g_embedded, 0, 0) == 0 || !g_shared ||
        (g_embedded_input_relative_patch_count != 0 &&
         !g_embedded_input_relative_patches) ||
        (g_embedded_input_patch_count != 0 && !g_embedded_input_patches) ||
        g_relative_patch_count != kRelativeHookCount + g_embedded_input_relative_patch_count ||
        g_pointer_patch_count != kPointerHookCount + g_embedded_input_patch_count)
    {
        if (g_shared && InterlockedCompareExchange(&g_embedded, 0, 0) != 0)
        {
            InterlockedExchange(&g_armed, 0);
            InterlockedExchange(&g_unloading, 1);
            SetState(ErrorCode::UnsafeShutdown, NativeState::Unsafe);
        }
        return static_cast<DWORD>(ErrorCode::UnsafeShutdown);
    }
    if (InterlockedCompareExchange(&g_embedded_input_restored, 0, 0) != 0)
        return 0;

    // Disarm the companion observation branch before changing either hook family.  Its V5
    // wrappers remain installed as pass-throughs until the input wrappers have drained.
    InterlockedExchange(&g_shared->state, static_cast<LONG>(NativeState::ShuttingDown));
    InterlockedExchange(&g_armed, 0);
    const bool strict =
        InterlockedCompareExchange(&g_strict_ordered_transaction, 0, 0) != 0;
    const bool restored = strict
                              ? isaac_cs_t0::mode::RestoreOrderedPatchTransaction(
                                    g_ordered_patches.data() + kHookCount,
                                    g_ordered_patch_count - kHookCount)
                              : isaac_cs_t0::mode::RestorePatchTransaction(
                                    g_relative_patches.data() + kRelativeHookCount,
                                    g_embedded_input_relative_patch_count,
                                    g_pointer_patches.data() + kPointerHookCount,
                                    g_embedded_input_patch_count);
    for (std::size_t index = 0; index < g_embedded_input_relative_patch_count; ++index)
    {
        g_embedded_input_relative_patches[index].original =
            g_relative_patches[kRelativeHookCount + index].original;
        g_embedded_input_relative_patches[index].installed =
            g_relative_patches[kRelativeHookCount + index].installed;
    }
    for (std::size_t index = 0; index < g_embedded_input_patch_count; ++index)
    {
        g_embedded_input_patches[index].original =
            g_pointer_patches[kPointerHookCount + index].original;
        g_embedded_input_patches[index].installed =
            g_pointer_patches[kPointerHookCount + index].installed;
    }
    if (!restored)
    {
        InterlockedExchange(&g_unloading, 1);
        SetState(ErrorCode::UnsafeShutdown, NativeState::Unsafe);
        return static_cast<DWORD>(ErrorCode::UnsafeShutdown);
    }
    InterlockedExchange(&g_embedded_input_restored, 1);
    return 0;
}

DWORD StopEmbeddedObserver() noexcept
{
    if (InterlockedCompareExchange(&g_embedded, 0, 0) == 0)
        return static_cast<DWORD>(ErrorCode::UnsafeShutdown);
    return StopObserver();
}

bool VerifyEmbeddedObserverRestored(std::uint32_t& verified_slot_count) noexcept
{
    verified_slot_count = 0;
    if (InterlockedCompareExchange(&g_embedded, 0, 0) == 0 || !g_shared ||
        InterlockedCompareExchange(&g_shared->state, 0, 0) !=
            static_cast<LONG>(NativeState::Stopped) ||
        g_relative_patch_count !=
            kRelativeHookCount + g_embedded_input_relative_patch_count ||
        g_pointer_patch_count != kPointerHookCount + g_embedded_input_patch_count)
        return false;
    __try
    {
        for (std::size_t index = 0; index < g_relative_patch_count; ++index)
        {
            const auto& patch = g_relative_patches[index];
            if (patch.installed || !patch.call ||
                std::memcmp(patch.call, patch.expected, sizeof(patch.expected)) != 0)
                return false;
            ++verified_slot_count;
        }
        for (std::size_t index = 0; index < g_pointer_patch_count; ++index)
        {
            const auto& patch = g_pointer_patches[index];
            if (patch.installed || !patch.slot || !patch.original ||
                *patch.slot != patch.original)
                return false;
            ++verified_slot_count;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
    for(std::size_t i=0;i<g_ordered_patch_count;++i) if(g_ordered_patches[i].entry) {
        const auto& patch=*g_ordered_patches[i].entry;
        if(patch.installed || std::memcmp(patch.entry,patch.expected.data(),patch.length)) return false;
        ++verified_slot_count;
    }
    const auto entry_count=static_cast<std::uint32_t>(std::count_if(
        g_ordered_patches.begin(),g_ordered_patches.begin()+g_ordered_patch_count,
        [](const auto& patch){return patch.entry!=nullptr;}));
    return verified_slot_count == entry_count +
           kRelativeHookCount + kPointerHookCount +
               g_embedded_input_relative_patch_count +
               g_embedded_input_patch_count;
}

bool ObserveEmbeddedRestoredTargets(RestorationObservation observer,void* context) noexcept
{
    std::uint32_t count=0;
    if(!observer || !VerifyEmbeddedObserverRestored(count)) return false;
    __try
    {
        for(std::size_t i=0;i<g_relative_patch_count;++i)
        {
            const auto& patch=g_relative_patches[i];
            observer(static_cast<std::uint32_t>(patch.call-g_main_base),patch.expected,
                patch.call,5,context);
        }
        for(std::size_t i=0;i<g_ordered_patch_count;++i) if(g_ordered_patches[i].entry) {
            const auto& patch=*g_ordered_patches[i].entry;
            observer(static_cast<std::uint32_t>(patch.entry-g_main_base),patch.expected.data(),
                patch.entry,patch.length,context);
        }
        for(std::size_t i=0;i<g_pointer_patch_count;++i)
        {
            const auto& patch=g_pointer_patches[i];
            const auto expected=reinterpret_cast<std::uintptr_t>(patch.original);
            const auto actual=reinterpret_cast<std::uintptr_t>(*patch.slot);
            observer(static_cast<std::uint32_t>(reinterpret_cast<std::uint8_t*>(patch.slot)-g_main_base),
                reinterpret_cast<const std::uint8_t*>(&expected),
                reinterpret_cast<const std::uint8_t*>(&actual),4,context);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
    return true;
}

bool SnapshotEmbeddedObserverSlots(std::array<std::uint8_t, 32>& digest) noexcept
{
    if (InterlockedCompareExchange(&g_strict_ordered_transaction, 0, 0) == 0)
        return false;
    return SnapshotOrderedSlots(digest);
}

extern "C" DWORD WINAPI IsaacCsT0LoopInitialize(const wchar_t* mapping_name)
{
    return InitializeObserver(mapping_name, EmbeddedObserverHooks{}, false, nullptr);
}

extern "C" DWORD WINAPI IsaacCsT0LoopShutdown(void*)
{
    if (InterlockedCompareExchange(&g_embedded, 0, 0) != 0)
        return static_cast<DWORD>(ErrorCode::UnsafeShutdown);
    const DWORD result = StopObserver();
    if (result != 0)
        return result;
    HMODULE module = g_module;
    FreeLibraryAndExitThread(module, 0);
}

} // namespace isaac_cs_t0::loop
