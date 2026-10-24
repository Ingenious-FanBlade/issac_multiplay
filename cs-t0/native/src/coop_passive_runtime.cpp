#include "isaac_cs_t0/coop_passive_runtime.hpp"

#include "isaac_cs_t0/active_targets.hpp"
#include "isaac_cs_t0/coop_passive_ipc.hpp"
#include "isaac_cs_t0/hooks.hpp"
#include "isaac_cs_t0/loop_runtime.hpp"
#include "isaac_cs_t0/sha256.hpp"
#include "isaac_cs_t0/target_image.hpp"

#include <Windows.h>
#include <intrin.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <limits>

namespace isaac_cs_t0::coop_passive {
namespace {

constexpr DWORD kDrainTimeoutMs = 5000;
constexpr DWORD kUnsafeResult = 8;
constexpr std::size_t kAdditionalRelativeCount =
    active::kActiveRelativeCount + kCoopSlotCount;
constexpr std::size_t kDeviceAdditionalRelativeCount =
    kAdditionalRelativeCount + kDeviceAdditionalSlotCount;
constexpr std::size_t kAdditionalPointerCount = active::kActivePointerCount;
constexpr std::uint32_t kExpectedImageSize = target_image::kUnpackedImageSize;

constexpr std::uint32_t kInputManagerRva = 0x00857B18u;
constexpr std::uint32_t kGameRootSlotRva = 0x00871678u;
constexpr std::uint32_t kOnlineManagerSlotRva = 0x0087169Cu;
constexpr std::uint32_t kNetworkBackendSlotRva = 0x00879A58u;
constexpr std::uint32_t kPlayerManagerOffset = 0x0001BAA8u;
constexpr std::uint32_t kRunActiveOffset = 0x0002658Au;
constexpr std::uint32_t kHudOffset = 0x0001DA04u;
constexpr std::uint32_t kHudStride = 0x6DCu;
constexpr std::uint32_t kOnlineRosterOffset = 0x0004B3D8u;
constexpr std::uint32_t kOnlineFrameOffset = 0x0004B3E4u;
constexpr std::uint32_t kSteamBackendVtableRva = 0x007A2440u;
constexpr std::uint32_t kPlayerVtableRva = 0x0076BDD0u;
constexpr std::uint32_t kPlayerColRva = 0x007AE31Cu;
constexpr std::uint32_t kPlayerAssignmentOffset = 0x1618u;
constexpr std::uint32_t kPlayerOrdinalOffset = 0x161Cu;
constexpr std::uint32_t kPlayerTwinOffset = 0x1E68u;
constexpr std::uint32_t kPlayerVariantOffset = 0x2Cu;
constexpr std::uint32_t kPlayerParentOffset = 0x3BCu;
constexpr std::uint32_t kDeviceAssignmentOffset = 0x0Cu;
constexpr std::uint32_t kKeyboardVtableRva = 0x007A1678u;
constexpr std::uint32_t kGamepadVtableRva = 0x00782C80u;
constexpr std::size_t kMaximumDeviceCount = 32;
constexpr std::size_t kMaximumPlayerCount = 4;
constexpr std::size_t kMaximumRosterCount = 4;

constexpr std::array<std::uint32_t, kCoopSlotCount> kCoopCallRvas{{
    0x002FADA7u, 0x005BB415u, 0x005BAFFAu, 0x005BB22Au,
    0x005BB424u, 0x005BB004u, 0x005BB4FDu, 0x005BA752u,
}};
constexpr std::array<std::uint32_t, kCoopSlotCount> kCoopTargetRvas{{
    0x005BAC40u, 0x005B9CD0u, 0x005B9CD0u, 0x005BA000u,
    0x003A6450u, 0x003A6450u, 0x005A8620u, 0x005A8620u,
}};
constexpr std::array<std::array<std::uint8_t, 5>, kCoopSlotCount> kCoopBytes{{
    {{0xE8, 0x94, 0xFE, 0x2B, 0x00}},
    {{0xE8, 0xB6, 0xE8, 0xFF, 0xFF}},
    {{0xE8, 0xD1, 0xEC, 0xFF, 0xFF}},
    {{0xE8, 0xD1, 0xED, 0xFF, 0xFF}},
    {{0xE8, 0x27, 0xB0, 0xDE, 0xFF}},
    {{0xE8, 0x47, 0xB4, 0xDE, 0xFF}},
    {{0xE8, 0x1E, 0xD1, 0xFE, 0xFF}},
    {{0xE8, 0xC9, 0xDE, 0xFE, 0xFF}},
}};
constexpr std::array<std::uint32_t, kDeviceAdditionalRelativeCount>
    kAdditionalRelativeOrdinals{{39, 40, 41, 42, 43, 44,
                                 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55}};
constexpr std::array<std::uint32_t, kDeviceAdditionalSlotCount> kDeviceCallRvas{{
    0x005B27EBu, 0x0055B87Fu, 0x0055BCA0u,
}};
constexpr std::array<std::array<std::uint8_t, 5>, kDeviceAdditionalSlotCount>
    kDeviceCallBytes{{
        {{0xE8, 0x60, 0x3C, 0xDF, 0xFF}},
        {{0xE8, 0xCC, 0xAB, 0xE4, 0xFF}},
        {{0xE8, 0xAB, 0xA7, 0xE4, 0xFF}},
    }};
constexpr std::array<std::uint32_t, kAdditionalPointerCount>
    kAdditionalPointerOrdinals{{32, 33, 34, 35, 36, 37, 38}};

using LogicalBoolFn = bool(__thiscall*)(void*, std::uint32_t);
using LogicalScalarFn = float(__thiscall*)(void*, std::uint32_t);
using PlayerUpdateFn = void(__thiscall*)(void*);
using ConsumerFn = void(__thiscall*)(void*);
using ItemConsumerFn = void(__thiscall*)(void*, std::int32_t);
using ManagerFn = std::uint32_t(__thiscall*)(void*);
using SpawnFn = void*(__thiscall*)(void*, std::int32_t);
using ExitFn = void(__thiscall*)(void*, void*, std::uint32_t);
using AssignmentFn = void(__thiscall*)(void*, std::int32_t, std::uint32_t);
using HudFn = void(__thiscall*)(void*);

enum class DeviceKind : std::uint32_t { Keyboard = 1, Gamepad = 2 };

struct DeviceObservation {
    void* object = nullptr;
    void* reference_owner = nullptr;
    std::int32_t assignment = -1;
    DeviceKind kind = DeviceKind::Keyboard;
    bool operator==(const DeviceObservation&) const = default;
};

struct PlayerObservation {
    void* object = nullptr;
    void* device = nullptr;
    std::int32_t assignment = -1;
    std::uint32_t ordinal = kNoOrdinal;
    DeviceKind device_kind = DeviceKind::Keyboard;
    bool operator==(const PlayerObservation&) const = default;
};

struct Topology {
    void* game_root = nullptr;
    void* player_manager = nullptr;
    void* online_manager = nullptr;
    void* network_backend = nullptr;
    std::uint32_t online_frame = 0;
    std::array<DeviceObservation, kMaximumDeviceCount> devices{};
    std::size_t device_count = 0;
    std::array<PlayerObservation, kMaximumPlayerCount> players{};
    std::size_t player_count = 0;
    std::array<void*, 8> hud{};
    bool operator==(const Topology&) const = default;
};

struct VectorHeader {
    std::uint8_t* begin = nullptr;
    std::uint8_t* end = nullptr;
    std::uint8_t* capacity_end = nullptr;
};

class BindingMachine final {
public:
    enum class Observation : std::uint32_t {
        Candidate,
        Bound,
        Stable,
        RelationChanged,
        Retired,
    };

    void Reset() noexcept
    {
        generation_ = 1;
        candidate_pass_ = 0;
        candidate_generation_ = 0;
        candidate_valid_ = false;
        bound_valid_ = false;
        retired_ = false;
        states_.fill(BindingState::Unbound);
        candidate_ = {};
        bound_ = {};
    }

    bool Invalidate(bool force) noexcept
    {
        if (retired_)
            return false;
        const bool had_state = candidate_valid_ || bound_valid_;
        if (!had_state && !force)
            return true;
        if (generation_ == (std::numeric_limits<std::uint64_t>::max)())
            return false;
        ++generation_;
        candidate_pass_ = 0;
        candidate_generation_ = 0;
        candidate_valid_ = false;
        bound_valid_ = false;
        candidate_ = {};
        bound_ = {};
        states_.fill(BindingState::Unbound);
        return true;
    }

    Observation ObserveCompleteManagerPass(std::uint64_t pass,
                                           const Topology& topology) noexcept
    {
        if (retired_)
            return Observation::Retired;
        if (bound_valid_)
            return topology == bound_ ? Observation::Stable
                                      : Observation::RelationChanged;
        if (candidate_valid_ && candidate_generation_ == generation_ &&
            pass == candidate_pass_ + 1 && topology == candidate_)
        {
            bound_ = topology;
            bound_valid_ = true;
            candidate_valid_ = false;
            for (std::size_t ordinal = 0; ordinal < states_.size(); ++ordinal)
                states_[ordinal] = ordinal < topology.player_count
                                       ? BindingState::Bound
                                       : BindingState::Unbound;
            return Observation::Bound;
        }
        candidate_ = topology;
        candidate_pass_ = pass;
        candidate_generation_ = generation_;
        candidate_valid_ = true;
        for (std::size_t ordinal = 0; ordinal < states_.size(); ++ordinal)
            states_[ordinal] = ordinal < topology.player_count
                                   ? BindingState::Candidate
                                   : BindingState::Unbound;
        return Observation::Candidate;
    }

    bool RetireRun() noexcept
    {
        if (retired_)
            return true;
        if (!Invalidate(true))
            return false;
        retired_ = true;
        states_.fill(BindingState::Retired);
        return true;
    }

    void DropCandidate() noexcept
    {
        if (bound_valid_ || retired_)
            return;
        candidate_pass_ = 0;
        candidate_generation_ = 0;
        candidate_valid_ = false;
        candidate_ = {};
        states_.fill(BindingState::Unbound);
    }

    // Admission must be closed and callbacks drained before this is called.
    // Discarding observer-owned identities is not a game lifecycle transition:
    // preserve the last observed generation, including its maximum value.
    void ScrubAfterObservationStopped() noexcept
    {
        candidate_pass_ = 0;
        candidate_generation_ = 0;
        candidate_valid_ = false;
        bound_valid_ = false;
        candidate_ = {};
        bound_ = {};
        states_.fill(retired_ ? BindingState::Retired : BindingState::Unbound);
    }

    std::uint64_t generation() const noexcept { return generation_; }
    bool retired() const noexcept { return retired_; }
    bool bound() const noexcept { return bound_valid_; }
    const Topology& bound_topology() const noexcept { return bound_; }
    const Topology& candidate_topology() const noexcept { return candidate_; }
    const std::array<BindingState, 2>& states() const noexcept { return states_; }
#if defined(ISAAC_CS_T0_TESTING)
    std::uint64_t candidate_generation() const noexcept { return candidate_generation_; }
    std::uint64_t candidate_pass() const noexcept { return candidate_pass_; }
    bool candidate_valid() const noexcept { return candidate_valid_; }
    void SetGenerationForTesting(std::uint64_t generation) noexcept
    {
        generation_ = generation;
    }
#endif

private:
    std::uint64_t generation_ = 1;
    std::uint64_t candidate_pass_ = 0;
    std::uint64_t candidate_generation_ = 0;
    bool candidate_valid_ = false;
    bool bound_valid_ = false;
    bool retired_ = false;
    std::array<BindingState, 2> states_{};
    Topology candidate_{};
    Topology bound_{};
};

HMODULE g_module = nullptr;
HANDLE g_mapping = nullptr;
SharedCoopPassiveBlock* g_shared = nullptr;
std::uint8_t* g_main_base = nullptr;
volatile LONG g_observation_enabled = 0;
volatile LONG g_unloading = 0;
volatile LONG g_active_hooks = 0;
volatile LONG g_failure_reported = 0;
volatile LONG g_device_source_invalid = 0;
std::array<std::uint8_t, kDigestSize> g_token_secret{};
std::array<mode::RelativeCallPatch, active::kActiveRelativeCount> g_active_relative{};
std::array<mode::RelativeCallPatch, kCoopSlotCount> g_coop_relative{};
std::array<mode::RelativeCallPatch, kDeviceAdditionalRelativeCount> g_relative{};
std::array<mode::PointerPatch, kAdditionalPointerCount> g_pointers{};
BindingMachine g_binding{};
std::uint64_t g_manager_pass = 0;
std::uint32_t g_lifecycle_depth = 0;

enum class OrdinaryRouteStage : std::uint32_t {
    Idle,
    SpawnBegun,
    SpawnEnded,
    AssignmentBegun,
    AssignmentEnded,
    HudBegun,
    Complete,
    Rejected,
};

struct OrdinaryRouteTracker {
    bool transaction_active = false;
    void* manager = nullptr;
    std::uint64_t manager_pass = 0;
    OrdinaryRouteStage stage = OrdinaryRouteStage::Idle;
    std::uint64_t route_generation = 0;
    std::uint64_t authorized_generation = 0;
};

OrdinaryRouteTracker g_ordinary_route{};

enum class DeviceRouteStage : std::uint32_t {
    Idle,
    PrimaryUnassignOpen,
    MenuDetached,
    PromptConfirmOpen,
    Detached,
    ReassignOpen,
    AwaitCoherent,
    Candidate,
    Complete,
    Rejected,
};

struct DeviceRouteTracker {
    DeviceRouteStage stage = DeviceRouteStage::Idle;
    Topology baseline{};
    std::uint32_t ordinal = kNoOrdinal;
    std::uint32_t primary_slot = kNoSlot;
    std::uint64_t generation = 0;
    std::uint64_t last_manager_pass = 0;
    std::uint64_t candidate_pass = 0;
    std::int32_t new_assignment = -1;
    bool ordinary_backend_authorized = false;
};
DeviceRouteTracker g_device_route{};
thread_local std::uint32_t g_device_assignment_depth = 0;

thread_local std::uint32_t g_manager_depth = 0;
thread_local std::uint32_t g_join_depth = 0;

#if defined(ISAAC_CS_T0_TESTING)
bool g_testing_context_override_enabled = false;
loop::ObservationContext g_testing_context{};
bool g_testing_topology_override_enabled = false;
std::uint32_t g_testing_topology_result = 0;
Topology g_testing_topology{};
detail::TopologyDiagnostic g_testing_topology_diagnostic =
    detail::TopologyDiagnostic::None;
bool g_testing_force_topology_exception = false;
bool g_testing_last_topology_valid = false;
std::uint32_t g_testing_last_topology_result = 0;
detail::TopologyDiagnostic g_testing_last_topology_diagnostic =
    detail::TopologyDiagnostic::None;
std::uint32_t g_manager_diagnostic_original_calls = 0;
std::uint32_t g_manager_diagnostic_original_return = 0;
const detail::DeviceRebindShutdownDependencies* g_testing_shutdown_dependencies = nullptr;
#endif

LogicalBoolFn g_keyboard_pressed = nullptr;
LogicalBoolFn g_keyboard_triggered = nullptr;
LogicalScalarFn g_keyboard_scalar = nullptr;
LogicalBoolFn g_gamepad_pressed = nullptr;
LogicalBoolFn g_gamepad_triggered = nullptr;
LogicalScalarFn g_gamepad_scalar = nullptr;
PlayerUpdateFn g_player_update = nullptr;
std::array<ConsumerFn, active::kActiveRelativeCount> g_consumers{};
ItemConsumerFn g_item_consumer = nullptr;
ManagerFn g_manager = nullptr;
SpawnFn g_spawn = nullptr;
ExitFn g_exit = nullptr;
AssignmentFn g_assignment = nullptr;
HudFn g_hud = nullptr;

extern "C" bool __fastcall PassiveKeyboardPressedHook(void*, void*, std::uint32_t);
extern "C" bool __fastcall PassiveKeyboardTriggeredHook(void*, void*, std::uint32_t);
extern "C" float __fastcall PassiveKeyboardScalarHook(void*, void*, std::uint32_t);
extern "C" bool __fastcall PassiveGamepadPressedHook(void*, void*, std::uint32_t);
extern "C" bool __fastcall PassiveGamepadTriggeredHook(void*, void*, std::uint32_t);
extern "C" float __fastcall PassiveGamepadScalarHook(void*, void*, std::uint32_t);
extern "C" void __fastcall PassivePlayerUpdateHook(void*, void*);
extern "C" void __fastcall PassiveMovementConsumerHook(void*, void*);
extern "C" void __fastcall PassiveBombConsumerHook(void*, void*);
extern "C" void __fastcall PassiveItemConsumerHook(void*, void*, std::int32_t);
extern "C" void __fastcall PassivePillConsumerHook(void*, void*);
extern "C" void __fastcall PassiveShootingConsumerHook(void*, void*);
extern "C" void __fastcall PassiveDropConsumerHook(void*, void*);
extern "C" std::uint32_t __fastcall CoopManagerOwnerHook(void*, void*);
extern "C" void* __fastcall CoopOrdinarySpawnHook(void*, void*, std::int32_t);
extern "C" void* __fastcall CoopPossessorSpawnHook(void*, void*, std::int32_t);
extern "C" void __fastcall CoopNativeExitHook(void*, void*, void*, std::uint32_t);
extern "C" void __fastcall CoopOrdinaryAssignmentHook(void*, void*, std::int32_t,
                                                       std::uint32_t);
extern "C" void __fastcall CoopPossessorAssignmentHook(void*, void*, std::int32_t,
                                                        std::uint32_t);
extern "C" void __fastcall CoopJoinHudRebuildHook(void*, void*);
extern "C" void __fastcall CoopExitHudRebuildHook(void*, void*);
extern "C" void __fastcall CoopDeviceMenuUnassignHook(void*, void*, std::int32_t,
                                                       std::uint32_t);
extern "C" void __fastcall CoopDeviceDisconnectUnassignHook(void*, void*, std::int32_t,
                                                             std::uint32_t);
extern "C" void __fastcall CoopDeviceReassignHook(void*, void*, std::int32_t,
                                                  std::uint32_t);

bool PrepareAdditionalTargets() noexcept;
bool VerifyAdditionalTargetsRestored() noexcept;
void ResetOrdinaryRouteTracker() noexcept;
void RevokeOrdinaryRouteWitness() noexcept;
bool InvalidateBindings(LifecycleReason reason, bool force) noexcept;
bool RetireStableRegistry() noexcept;
void CompleteManagerPass(void* manager, void* return_address) noexcept;
bool DeviceV2() noexcept;
bool DeviceRoutePending() noexcept;
void RejectDeviceRoute(ErrorCode error = ErrorCode::LifecycleMismatch) noexcept;
bool DeviceRouteAuthorizesBackend() noexcept;
void LifecycleCallback(loop::EmbeddedLifecycleSignal signal, bool entering,
                       void* owner, void* return_address,
                       void* context) noexcept;

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

std::int64_t Qpc() noexcept
{
    LARGE_INTEGER value{};
    return QueryPerformanceCounter(&value) ? value.QuadPart : 0;
}

template <std::size_t Size>
bool Nonzero(const std::array<std::uint8_t, Size>& value) noexcept
{
    return std::any_of(value.begin(), value.end(),
                       [](std::uint8_t byte) { return byte != 0; });
}

bool ScenarioAllowed(std::uint32_t value) noexcept
{
    return value >= static_cast<std::uint32_t>(Scenario::NativeJoin) &&
           value <= static_cast<std::uint32_t>(Scenario::MenuTeardown);
}

bool DeviceV2() noexcept
{
    return g_shared && g_shared->version == kDeviceSchemaVersion &&
           g_shared->scenario == static_cast<std::uint32_t>(Scenario::DeviceRebind);
}

std::size_t AdditionalRelativeCount() noexcept
{
    return DeviceV2() ? kDeviceAdditionalRelativeCount : kAdditionalRelativeCount;
}

std::size_t CombinedSlotCount() noexcept
{
    return DeviceV2() ? kDeviceCombinedSlotCount : kCombinedSlotCount;
}

std::uint64_t CompleteHookMask() noexcept
{
    return DeviceV2() ? kDeviceCompleteHookMask : kCompleteHookMask;
}

bool DeviceRoutePending() noexcept
{
    return DeviceV2() && g_device_route.stage != DeviceRouteStage::Idle &&
           g_device_route.stage != DeviceRouteStage::Complete &&
           g_device_route.stage != DeviceRouteStage::Rejected;
}

bool DeviceRouteAuthorizesBackend() noexcept
{
    return DeviceV2() &&
           (DeviceRoutePending() || g_device_route.stage == DeviceRouteStage::Complete) &&
           g_device_route.ordinary_backend_authorized &&
           g_device_route.generation == g_binding.generation();
}

void SetState(ErrorCode error, NativeState state) noexcept
{
    if (!g_shared)
        return;
    InterlockedExchange(&g_shared->error_code, static_cast<LONG>(error));
    InterlockedExchange(&g_shared->state, static_cast<LONG>(state));
}

bool PublicationEnabled() noexcept
{
    return g_shared &&
           InterlockedCompareExchange(&g_shared->publication_enabled, 0, 0) != 0;
}

bool ObservationEnabled() noexcept
{
    return PublicationEnabled() &&
           (!DeviceV2() || InterlockedCompareExchange(&g_device_source_invalid, 0, 0) == 0) &&
           InterlockedCompareExchange(&g_observation_enabled, 0, 0) != 0 &&
           InterlockedCompareExchange(&g_unloading, 0, 0) == 0 &&
           !g_binding.retired();
}

NativePassiveEvent Event(EventKind kind, std::uint32_t slot = kNoSlot) noexcept
{
    NativePassiveEvent event{};
    event.qpc = Qpc();
    event.thread_id = GetCurrentThreadId();
    event.kind = static_cast<std::uint32_t>(kind);
    event.slot_ordinal = slot;
    event.owner_generation = g_binding.generation();
#if defined(ISAAC_CS_T0_TESTING)
    const auto context = g_testing_context_override_enabled
                             ? g_testing_context
                             : loop::CurrentObservationContext();
#else
    const auto context = loop::CurrentObservationContext();
#endif
    event.loop_iteration = context.loop_iteration;
    event.main_thread_id = context.main_thread_id;
    event.phase = static_cast<std::uint32_t>(context.phase);
    event.depth = context.phase_depth;
    if (context.main_thread_id && context.main_thread_id == event.thread_id)
        event.flags |= kFlagMainThread;
    return event;
}

bool Publish(const NativePassiveEvent& event) noexcept
{
    if (!PublicationEnabled())
        return false;
    const LONG64 sequence = InterlockedIncrement64(&g_shared->next_sequence);
    const LONG64 consumed =
        InterlockedCompareExchange64(&g_shared->controller_sequence, 0, 0);
    if (sequence - consumed > static_cast<LONG64>(kEventCapacity))
    {
        if (DeviceV2())
            InterlockedExchange(&g_device_source_invalid, 1);
        InterlockedIncrement64(&g_shared->overwritten_events);
        InterlockedExchange(&g_observation_enabled, 0);
        InterlockedExchange(&g_shared->publication_enabled, 0);
        SetState(ErrorCode::RingOverflow, NativeState::Unsafe);
        return false;
    }
    auto& slot = g_shared->events[static_cast<std::size_t>(sequence - 1) %
                                  kEventCapacity];
    InterlockedExchange64(&slot.published_sequence, 0);
    CopyEventPayload(slot, event);
    MemoryBarrier();
    InterlockedExchange64(&slot.published_sequence, sequence);
    return true;
}

void DisableObservation(ErrorCode error, std::uint32_t slot = kNoSlot) noexcept
{
    if (DeviceV2())
    {
        InterlockedExchange(&g_device_source_invalid, 1);
        // A wrong-thread callback can revoke admission atomically, but must
        // never race the main thread's private identity/binding structures.
        if (g_shared && static_cast<DWORD>(InterlockedCompareExchange(
                &g_shared->main_thread_id, 0, 0)) == GetCurrentThreadId())
        {
            g_device_route.stage = DeviceRouteStage::Rejected;
            g_device_route.baseline = {};
            g_device_route.ordinary_backend_authorized = false;
            g_binding.Invalidate(false);
            InterlockedExchange64(&g_shared->owner_generation,
                                  static_cast<LONG64>(g_binding.generation()));
        }
    }
    if (!g_shared || InterlockedExchange(&g_failure_reported, 1) != 0)
        return;
    auto event = Event(EventKind::Failure, slot);
    event.flags |= kFlagFailure;
    event.phase = static_cast<std::uint32_t>(error);
    Publish(event);
    InterlockedExchange(&g_observation_enabled, 0);
    SetState(error, NativeState::Failed);
}

void RejectDeviceRoute(ErrorCode error) noexcept
{
    if (!DeviceV2())
        return;
    g_device_route.stage = DeviceRouteStage::Rejected;
    g_device_route.baseline = {};
    g_device_route.ordinary_backend_authorized = false;
    InvalidateBindings(LifecycleReason::DeviceRebind, false);
    DisableObservation(error);
}

bool CheckMainThread() noexcept
{
    if (!g_shared)
        return false;
#if defined(ISAAC_CS_T0_TESTING)
    const auto context = g_testing_context_override_enabled
                             ? g_testing_context
                             : loop::CurrentObservationContext();
#else
    const auto context = loop::CurrentObservationContext();
#endif
    if (context.main_thread_id == 0)
        return false;
    if (context.thread_id != context.main_thread_id)
    {
        DisableObservation(ErrorCode::WrongThread);
        return false;
    }
    const LONG observed = InterlockedCompareExchange(
        &g_shared->main_thread_id, static_cast<LONG>(context.main_thread_id), 0);
    if (observed != 0 && observed != static_cast<LONG>(context.main_thread_id))
    {
        DisableObservation(ErrorCode::WrongThread);
        return false;
    }
    return true;
}

bool WaitForCallbacksDrained() noexcept
{
    const ULONGLONG deadline = GetTickCount64() + kDrainTimeoutMs;
    while (InterlockedCompareExchange(&g_active_hooks, 0, 0) != 0 &&
           GetTickCount64() < deadline)
        Sleep(1);
    return InterlockedCompareExchange(&g_active_hooks, 0, 0) == 0;
}

std::array<std::uint8_t, kDigestSize> Token(std::uint32_t domain,
                                            std::uintptr_t value,
                                            std::uint32_t relation = 0) noexcept
{
    struct Material {
        std::array<std::uint8_t, kDigestSize> secret{};
        std::uint32_t domain = 0;
        std::uintptr_t value = 0;
        std::uint32_t relation = 0;
    } material{};
    material.secret = g_token_secret;
    material.domain = domain;
    material.value = value;
    material.relation = relation;
    return mode::Sha256(&material, sizeof(material));
}

std::array<std::uint8_t, kDigestSize> TopologyDigest(
    const Topology& topology) noexcept
{
    return mode::Sha256Pair(g_token_secret.data(), g_token_secret.size(),
                            &topology, sizeof(topology));
}

void FillRelation(NativePassiveEvent& event, const Topology& topology,
                  std::uint32_t ordinal) noexcept
{
    if (ordinal >= topology.player_count || ordinal > 1)
        return;
    const auto& player = topology.players[ordinal];
    event.local_ordinal = ordinal;
    event.endpoint = ordinal + 1;
    event.flags |= ordinal == 0 ? kFlagEndpointA : kFlagEndpointB;
    event.flags |= kFlagTopologyCoherent | kFlagBindingUnique |
                   kFlagHudMatch | kFlagAssignmentMatch |
                   kFlagLuaVectorEquivalent;
    event.topology_digest = TopologyDigest(topology);
    event.player_digest = Token(1, reinterpret_cast<std::uintptr_t>(player.object));
    event.device_digest = Token(2, reinterpret_cast<std::uintptr_t>(player.device));
    event.assignment_digest =
        Token(3, static_cast<std::uint32_t>(player.assignment), ordinal);
}

} // namespace
} // namespace isaac_cs_t0::coop_passive

namespace isaac_cs_t0::coop_passive::detail {

std::array<std::uint8_t, 32> TopologyDiagnosticDigest(
    TopologyDiagnostic diagnostic) noexcept
{
    static_assert(static_cast<std::uint32_t>(
                      TopologyDiagnostic::OfflineOnlineFrameNonzero) == 25);
    constexpr char kDomain[] =
        "IsaacCsT0-CoopPassive-TopologyDiagnostic-v1";
    const auto raw = static_cast<std::uint32_t>(diagnostic);
    if (raw == 0 ||
        raw > static_cast<std::uint32_t>(
                  TopologyDiagnostic::OfflineOnlineFrameNonzero))
        return {};
    const std::array<std::uint8_t, 4> code{{
        static_cast<std::uint8_t>(raw & 0xFFu),
        static_cast<std::uint8_t>((raw >> 8) & 0xFFu),
        static_cast<std::uint8_t>((raw >> 16) & 0xFFu),
        static_cast<std::uint8_t>((raw >> 24) & 0xFFu),
    }};
    return mode::Sha256Pair(kDomain, sizeof(kDomain) - 1,
                            code.data(), code.size());
}

} // namespace isaac_cs_t0::coop_passive::detail

#if defined(ISAAC_CS_T0_TESTING)
namespace isaac_cs_t0::coop_passive {
namespace {

detail::AbiOriginalOnceProbe* g_abi_probe = nullptr;
std::size_t g_abi_slot = 0;
void* g_abi_self = nullptr;
std::uint32_t g_abi_action = 0;
std::int32_t g_abi_i32 = 0;
std::uint32_t g_abi_u32 = 0;
bool g_abi_arguments_ok = true;

enum class BackendProbeRouteMode : std::uint32_t {
    None,
    Exact,
    Partial,
    OutOfOrder,
    StaleGeneration,
    CrossGeneration,
    Possessor,
    Exit,
    AssignmentOnly,
};

struct BackendProbeOriginalCounts {
    std::uint32_t manager = 0;
    std::uint32_t spawn = 0;
    std::uint32_t possessor_spawn = 0;
    std::uint32_t exit = 0;
    std::uint32_t assignment = 0;
    std::uint32_t hud = 0;
    std::uint64_t generation_before = 0;
    std::uint64_t generation_after_spawn = 0;
    std::uint64_t generation_after_assignment = 0;
};

BackendProbeRouteMode g_backend_probe_route_mode =
    BackendProbeRouteMode::None;
BackendProbeOriginalCounts g_backend_probe_counts{};
bool g_backend_probe_possessor_call = false;
constexpr std::uint32_t kBackendProbeManagerReturn = 0xC0DEB045u;

void* __fastcall BackendProbeSpawnOriginal(void*, void*, std::int32_t) noexcept
{
    if (g_backend_probe_possessor_call)
        ++g_backend_probe_counts.possessor_spawn;
    else
        ++g_backend_probe_counts.spawn;
    return reinterpret_cast<void*>(0x1234BEEFu);
}

void __fastcall BackendProbeExitOriginal(void*, void*, void*,
                                         std::uint32_t) noexcept
{
    ++g_backend_probe_counts.exit;
}

void __fastcall BackendProbeAssignmentOriginal(void*, void*, std::int32_t,
                                               std::uint32_t) noexcept
{
    ++g_backend_probe_counts.assignment;
}

void __fastcall BackendProbeHudOriginal(void*, void*) noexcept
{
    ++g_backend_probe_counts.hud;
}

void BackendProbeExactRoute(void* self) noexcept
{
    CoopOrdinarySpawnHook(self, nullptr, 2);
    g_backend_probe_counts.generation_after_spawn = g_binding.generation();
    CoopOrdinaryAssignmentHook(self, nullptr, 1, 1);
    g_backend_probe_counts.generation_after_assignment = g_binding.generation();
    CoopJoinHudRebuildHook(self, nullptr);
}

std::uint32_t __fastcall BackendProbeManagerOriginal(void* self,
                                                     void*) noexcept
{
    ++g_backend_probe_counts.manager;
    switch (g_backend_probe_route_mode)
    {
    case BackendProbeRouteMode::Exact:
        BackendProbeExactRoute(self);
        break;
    case BackendProbeRouteMode::Partial:
        CoopOrdinarySpawnHook(self, nullptr, 2);
        g_backend_probe_counts.generation_after_spawn = g_binding.generation();
        break;
    case BackendProbeRouteMode::OutOfOrder:
        CoopOrdinarySpawnHook(self, nullptr, 2);
        g_backend_probe_counts.generation_after_spawn = g_binding.generation();
        CoopJoinHudRebuildHook(self, nullptr);
        CoopOrdinaryAssignmentHook(self, nullptr, 1, 1);
        g_backend_probe_counts.generation_after_assignment =
            g_binding.generation();
        break;
    case BackendProbeRouteMode::StaleGeneration:
        BackendProbeExactRoute(self);
        g_binding.Invalidate(true);
        break;
    case BackendProbeRouteMode::CrossGeneration:
        BackendProbeExactRoute(self);
        CoopOrdinaryAssignmentHook(self, nullptr, 1, 1);
        break;
    case BackendProbeRouteMode::Possessor:
        g_backend_probe_possessor_call = true;
        CoopPossessorSpawnHook(self, nullptr, -1);
        g_backend_probe_possessor_call = false;
        break;
    case BackendProbeRouteMode::Exit:
        CoopNativeExitHook(self, nullptr, reinterpret_cast<void*>(0x1234u), 0);
        break;
    case BackendProbeRouteMode::AssignmentOnly:
        CoopOrdinaryAssignmentHook(self, nullptr, 1, 1);
        break;
    case BackendProbeRouteMode::None:
        break;
    }
    return kBackendProbeManagerReturn;
}

void CountAbi() noexcept
{
    if (g_abi_probe && g_abi_slot < g_abi_probe->original_calls.size())
        ++g_abi_probe->original_calls[g_abi_slot];
}

bool __fastcall AbiBoolOriginal(void* self, void*,
                                std::uint32_t action) noexcept
{
    CountAbi();
    g_abi_arguments_ok = g_abi_arguments_ok && self == g_abi_self &&
                         action == g_abi_action;
    return (action & 1u) != 0;
}

float __fastcall AbiScalarOriginal(void* self, void*,
                                    std::uint32_t action) noexcept
{
    CountAbi();
    g_abi_arguments_ok = g_abi_arguments_ok && self == g_abi_self &&
                         action == g_abi_action;
    return static_cast<float>(action) + 0.25F;
}

void __fastcall AbiVoidOriginal(void* self, void*) noexcept
{
    CountAbi();
    g_abi_arguments_ok = g_abi_arguments_ok && self == g_abi_self;
}

std::uint32_t __fastcall AbiManagerOriginal(void* self, void*) noexcept
{
    CountAbi();
    g_abi_arguments_ok = g_abi_arguments_ok && self == g_abi_self;
    return 0xC0DEC045u;
}

std::uint32_t __fastcall ManagerDiagnosticOriginal(void*, void*) noexcept
{
    ++g_manager_diagnostic_original_calls;
    return g_manager_diagnostic_original_return;
}

void __fastcall AbiItemOriginal(void* self, void*,
                                std::int32_t value) noexcept
{
    CountAbi();
    g_abi_arguments_ok = g_abi_arguments_ok && self == g_abi_self &&
                         value == g_abi_i32;
}

void* __fastcall AbiSpawnOriginal(void* self, void*,
                                  std::int32_t value) noexcept
{
    CountAbi();
    g_abi_arguments_ok = g_abi_arguments_ok && self == g_abi_self &&
                         value == g_abi_i32;
    return reinterpret_cast<void*>(0x12345678u);
}

void __fastcall AbiExitOriginal(void* self, void*, void* player,
                                std::uint32_t padding) noexcept
{
    CountAbi();
    g_abi_arguments_ok = g_abi_arguments_ok && self == g_abi_self &&
                         player == reinterpret_cast<void*>(0x2345678Cu) &&
                         padding == g_abi_u32;
}

void __fastcall AbiAssignmentOriginal(void* self, void*,
                                      std::int32_t value,
                                      std::uint32_t propagate) noexcept
{
    CountAbi();
    g_abi_arguments_ok = g_abi_arguments_ok && self == g_abi_self &&
                         value == g_abi_i32 && propagate == g_abi_u32;
}

template <std::size_t Size>
bool AllBytes(const std::array<std::uint8_t, Size>& value,
              std::uint8_t expected) noexcept
{
    return std::all_of(value.begin(), value.end(),
                       [expected](std::uint8_t byte) { return byte == expected; });
}

bool SignalAndWait(HANDLE ready, HANDLE acknowledge) noexcept
{
    return SetEvent(ready) != FALSE &&
           WaitForSingleObject(acknowledge, 30'000) == WAIT_OBJECT_0;
}

} // namespace

namespace detail {

bool AbiOriginalOnceProbe::AllOriginalsCalledExactlyOnce() const noexcept
{
    return std::all_of(original_calls.begin(), original_calls.end(),
                       [](std::uint32_t value) { return value == 1; });
}

bool AbiOriginalOnceProbe::ArgumentsAndReturnsPreserved() const noexcept
{
    return arguments_preserved && returns_preserved;
}

bool AbiOriginalOnceProbe::ObservationFailureStillPassedThrough() const noexcept
{
    return observation_failure_passthrough;
}

bool AbiOriginalOnceProbe::PossessorStoppedWithoutSuppressingOriginal() const noexcept
{
    return possessor_passthrough;
}

bool RunAbiOriginalOnceProbe(AbiOriginalOnceProbe& probe) noexcept
{
    if (g_shared)
        return false;
    probe = {};
    g_abi_probe = &probe;
    g_abi_arguments_ok = true;
    int object = 7;
    g_abi_self = &object;

    const auto saved_keyboard_pressed = g_keyboard_pressed;
    const auto saved_keyboard_triggered = g_keyboard_triggered;
    const auto saved_keyboard_scalar = g_keyboard_scalar;
    const auto saved_gamepad_pressed = g_gamepad_pressed;
    const auto saved_gamepad_triggered = g_gamepad_triggered;
    const auto saved_gamepad_scalar = g_gamepad_scalar;
    const auto saved_player_update = g_player_update;
    const auto saved_consumers = g_consumers;
    const auto saved_item = g_item_consumer;
    const auto saved_manager = g_manager;
    const auto saved_spawn = g_spawn;
    const auto saved_exit = g_exit;
    const auto saved_assignment = g_assignment;
    const auto saved_hud = g_hud;
    const LONG saved_observation =
        InterlockedExchange(&g_observation_enabled, 0);

    g_keyboard_pressed = reinterpret_cast<LogicalBoolFn>(&AbiBoolOriginal);
    g_keyboard_triggered = reinterpret_cast<LogicalBoolFn>(&AbiBoolOriginal);
    g_keyboard_scalar = reinterpret_cast<LogicalScalarFn>(&AbiScalarOriginal);
    g_gamepad_pressed = reinterpret_cast<LogicalBoolFn>(&AbiBoolOriginal);
    g_gamepad_triggered = reinterpret_cast<LogicalBoolFn>(&AbiBoolOriginal);
    g_gamepad_scalar = reinterpret_cast<LogicalScalarFn>(&AbiScalarOriginal);
    g_player_update = reinterpret_cast<PlayerUpdateFn>(&AbiVoidOriginal);
    g_consumers.fill(reinterpret_cast<ConsumerFn>(&AbiVoidOriginal));
    g_item_consumer = reinterpret_cast<ItemConsumerFn>(&AbiItemOriginal);
    g_manager = reinterpret_cast<ManagerFn>(&AbiManagerOriginal);
    g_spawn = reinterpret_cast<SpawnFn>(&AbiSpawnOriginal);
    g_exit = reinterpret_cast<ExitFn>(&AbiExitOriginal);
    g_assignment = reinterpret_cast<AssignmentFn>(&AbiAssignmentOriginal);
    g_hud = reinterpret_cast<HudFn>(&AbiVoidOriginal);

    bool returns_ok = true;
    g_abi_slot = 0; g_abi_action = 11;
    returns_ok = returns_ok && PassiveKeyboardPressedHook(&object, nullptr, 11);
    g_abi_slot = 1; g_abi_action = 12;
    returns_ok = returns_ok && !PassiveKeyboardTriggeredHook(&object, nullptr, 12);
    g_abi_slot = 2; g_abi_action = 7;
    returns_ok = returns_ok &&
                 PassiveKeyboardScalarHook(&object, nullptr, 7) == 7.25F;
    g_abi_slot = 3; g_abi_action = 13;
    returns_ok = returns_ok && PassiveGamepadPressedHook(&object, nullptr, 13);
    g_abi_slot = 4; g_abi_action = 14;
    returns_ok = returns_ok && !PassiveGamepadTriggeredHook(&object, nullptr, 14);
    g_abi_slot = 5; g_abi_action = 8;
    returns_ok = returns_ok &&
                 PassiveGamepadScalarHook(&object, nullptr, 8) == 8.25F;
    g_abi_slot = 6; PassivePlayerUpdateHook(&object, nullptr);
    g_abi_slot = 7; PassiveMovementConsumerHook(&object, nullptr);
    g_abi_slot = 8; PassiveBombConsumerHook(&object, nullptr);
    g_abi_slot = 9; g_abi_i32 = 17;
    PassiveItemConsumerHook(&object, nullptr, 17);
    g_abi_slot = 10; PassivePillConsumerHook(&object, nullptr);
    g_abi_slot = 11; PassiveShootingConsumerHook(&object, nullptr);
    g_abi_slot = 12; PassiveDropConsumerHook(&object, nullptr);
    g_abi_slot = 13;
    returns_ok = returns_ok &&
                 CoopManagerOwnerHook(&object, nullptr) == 0xC0DEC045u;
    g_abi_slot = 14; g_abi_i32 = 2;
    returns_ok = returns_ok &&
                 CoopOrdinarySpawnHook(&object, nullptr, 2) ==
                     reinterpret_cast<void*>(0x12345678u);
    g_abi_slot = 15; g_abi_i32 = -1;
    returns_ok = returns_ok &&
                 CoopPossessorSpawnHook(&object, nullptr, -1) ==
                     reinterpret_cast<void*>(0x12345678u);
    g_abi_slot = 16; g_abi_u32 = 0xA5A5A5A5u;
    CoopNativeExitHook(&object, nullptr,
                       reinterpret_cast<void*>(0x2345678Cu), g_abi_u32);
    g_abi_slot = 17; g_abi_i32 = 3; g_abi_u32 = 1;
    CoopOrdinaryAssignmentHook(&object, nullptr, g_abi_i32, g_abi_u32);
    g_abi_slot = 18; g_abi_i32 = 4; g_abi_u32 = 1;
    CoopPossessorAssignmentHook(&object, nullptr, g_abi_i32, g_abi_u32);
    g_abi_slot = 19; CoopJoinHudRebuildHook(&object, nullptr);
    g_abi_slot = 20; CoopExitHudRebuildHook(&object, nullptr);

    probe.arguments_preserved = g_abi_arguments_ok;
    probe.returns_preserved = returns_ok;
    probe.observation_failure_passthrough = probe.AllOriginalsCalledExactlyOnce();
    probe.possessor_passthrough = probe.original_calls[15] == 1 &&
                                  probe.original_calls[18] == 1;

    g_keyboard_pressed = saved_keyboard_pressed;
    g_keyboard_triggered = saved_keyboard_triggered;
    g_keyboard_scalar = saved_keyboard_scalar;
    g_gamepad_pressed = saved_gamepad_pressed;
    g_gamepad_triggered = saved_gamepad_triggered;
    g_gamepad_scalar = saved_gamepad_scalar;
    g_player_update = saved_player_update;
    g_consumers = saved_consumers;
    g_item_consumer = saved_item;
    g_manager = saved_manager;
    g_spawn = saved_spawn;
    g_exit = saved_exit;
    g_assignment = saved_assignment;
    g_hud = saved_hud;
    InterlockedExchange(&g_observation_enabled, saved_observation);
    g_abi_probe = nullptr;
    return probe.AllOriginalsCalledExactlyOnce() &&
           probe.ArgumentsAndReturnsPreserved() &&
           probe.ObservationFailureStillPassedThrough() &&
           probe.PossessorStoppedWithoutSuppressingOriginal();
}

bool RunBindingMachineProbe(BindingMachineProbe& probe) noexcept
{
    probe = {};
    Topology topology{};
    topology.player_count = 2;
    topology.players[0].object = reinterpret_cast<void*>(0x1000u);
    topology.players[0].device = reinterpret_cast<void*>(0x2000u);
    topology.players[0].ordinal = 0;
    topology.players[1].object = reinterpret_cast<void*>(0x3000u);
    topology.players[1].device = reinterpret_cast<void*>(0x4000u);
    topology.players[1].ordinal = 1;

    BindingMachine machine{};
    machine.Reset();
    const auto first = machine.ObserveCompleteManagerPass(10, topology);
    const bool first_candidate = first == BindingMachine::Observation::Candidate &&
                                 !machine.bound() &&
                                 machine.states()[0] == BindingState::Candidate &&
                                 machine.states()[1] == BindingState::Candidate;
    const auto second = machine.ObserveCompleteManagerPass(11, topology);
    probe.two_complete_passes_required =
        first_candidate && second == BindingMachine::Observation::Bound &&
        machine.bound() && machine.states()[0] == BindingState::Bound &&
        machine.states()[1] == BindingState::Bound;
    probe.endpoint_ordinals_frozen = topology.players[0].ordinal == 0 &&
                                     topology.players[1].ordinal == 1;

    Topology duplicate = topology;
    duplicate.players[1].object = duplicate.players[0].object;
    Topology third = topology;
    third.player_count = 3;
    const auto shape_valid = [](const Topology& value) noexcept {
        return value.player_count <= 2 &&
               (value.player_count < 2 ||
                (value.players[0].object != value.players[1].object &&
                 value.players[0].ordinal == 0 &&
                 value.players[1].ordinal == 1));
    };
    probe.duplicate_and_third_ordinal_rejected =
        !shape_valid(duplicate) && !shape_valid(third);

    const std::uint64_t old_generation = machine.generation();
    const bool invalidated = machine.Invalidate(true);
    const bool unbound = machine.states()[0] == BindingState::Unbound &&
                         machine.states()[1] == BindingState::Unbound;
    const auto after_old_pass = machine.ObserveCompleteManagerPass(11, topology);
    const bool candidate_again =
        after_old_pass == BindingMachine::Observation::Candidate && !machine.bound();
    const auto rebound = machine.ObserveCompleteManagerPass(12, topology);
    probe.lifecycle_invalidates_before_rebind = invalidated && unbound &&
                                                candidate_again &&
                                                rebound == BindingMachine::Observation::Bound;
    probe.old_generation_never_revived = machine.generation() == old_generation + 1 &&
                                         machine.generation() != old_generation;
    BindingMachine rejected_machine = machine;
    const auto rejected_generation = rejected_machine.generation();
    probe.invalid_manager_pass_unbinds_without_retirement =
        rejected_machine.bound() && rejected_machine.Invalidate(true) &&
        !rejected_machine.bound() && !rejected_machine.retired() &&
        rejected_machine.generation() == rejected_generation + 1 &&
        rejected_machine.states()[0] == BindingState::Unbound &&
        rejected_machine.states()[1] == BindingState::Unbound;
    const bool retired = machine.RetireRun();
    probe.menu_teardown_retires_stable_and_temporary_layers =
        retired && machine.retired() &&
        machine.states()[0] == BindingState::Retired &&
        machine.states()[1] == BindingState::Retired &&
        machine.ObserveCompleteManagerPass(13, topology) ==
            BindingMachine::Observation::Retired;
    return probe.two_complete_passes_required && probe.endpoint_ordinals_frozen &&
           probe.duplicate_and_third_ordinal_rejected &&
           probe.lifecycle_invalidates_before_rebind &&
           probe.old_generation_never_revived &&
           probe.invalid_manager_pass_unbinds_without_retirement &&
           probe.menu_teardown_retires_stable_and_temporary_layers;
}

int PublishCrossLanguageFixture(const wchar_t* mapping_name,
                                const wchar_t* ready_event_name,
                                const wchar_t* acknowledge_event_name) noexcept
{
    HANDLE mapping = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, mapping_name);
    HANDLE ready = OpenEventW(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE,
                              ready_event_name);
    HANDLE acknowledge = OpenEventW(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE,
                                    acknowledge_event_name);
    if (!mapping || !ready || !acknowledge)
    {
        if (acknowledge) CloseHandle(acknowledge);
        if (ready) CloseHandle(ready);
        if (mapping) CloseHandle(mapping);
        return 2;
    }
    auto* block = static_cast<SharedCoopPassiveBlock*>(
        MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0,
                      sizeof(SharedCoopPassiveBlock)));
    if (!block || block->magic != kIpcMagic || block->version != kSchemaVersion ||
        block->struct_size != sizeof(SharedCoopPassiveBlock) ||
        block->process_creation_time_100ns != 1 ||
        !AllBytes(block->nonce, 1) ||
        !AllBytes(block->canonical_target_sha256, 2) ||
        !AllBytes(block->observed_target_sha256, 3) ||
        !AllBytes(block->target_equivalence_sha256, 4) ||
        !AllBytes(block->catalog_sha256, 5) || !AllBytes(block->graph_sha256, 6) ||
        !AllBytes(block->plan_sha256, 7) ||
        !AllBytes(block->verification_sha256, 8) ||
        block->loop_mapping_name[0] == L'\0')
    {
        if (block) UnmapViewOfFile(block);
        CloseHandle(acknowledge);
        CloseHandle(ready);
        CloseHandle(mapping);
        return 3;
    }
    NativePassiveEvent sentinel{};
    sentinel.qpc = 11;
    sentinel.loop_iteration = 12;
    sentinel.owner_generation = 13;
    sentinel.topology_digest.fill(0xA1);
    sentinel.player_digest.fill(0xB2);
    sentinel.device_digest.fill(0xC3);
    sentinel.assignment_digest.fill(0xD4);
    sentinel.thread_id = 14;
    sentinel.main_thread_id = 14;
    sentinel.kind = static_cast<std::uint32_t>(EventKind::BindingBound);
    sentinel.slot_ordinal = 45;
    sentinel.phase = 2;
    sentinel.depth = 1;
    sentinel.local_ordinal = 1;
    sentinel.endpoint = 2;
    sentinel.original_calls = 1;
    sentinel.flags = kFlagMainThread | kFlagOriginalCalled |
                     kFlagTopologyCoherent | kFlagEndpointB |
                     kFlagBindingUnique | kFlagHudMatch |
                     kFlagAssignmentMatch | kFlagLuaVectorEquivalent;

    auto& first = block->events[0];
    CopyEventPayload(first, sentinel);
    MemoryBarrier();
    InterlockedExchange64(&first.published_sequence, 1);
    InterlockedExchange64(&block->next_sequence, 1);
    bool complete = SignalAndWait(ready, acknowledge);

    sentinel.qpc = 22;
    auto& second = block->events[1];
    CopyEventPayload(second, sentinel);
    InterlockedExchange64(&second.published_sequence, 0);
    InterlockedExchange64(&block->next_sequence, 2);
    complete = complete && SignalAndWait(ready, acknowledge);
    MemoryBarrier();
    InterlockedExchange64(&second.published_sequence, 2);
    complete = complete && SignalAndWait(ready, acknowledge);

    auto invalid = sentinel;
    invalid.qpc = 33;
    invalid.flags = kKnownFlags | (1u << 31);
    auto& third = block->events[2];
    CopyEventPayload(third, invalid);
    MemoryBarrier();
    InterlockedExchange64(&third.published_sequence, 3);
    InterlockedExchange64(&block->next_sequence, 3);
    complete = complete && SignalAndWait(ready, acknowledge);

    InterlockedExchange64(&block->overwritten_events, 1);
    complete = complete && SignalAndWait(ready, acknowledge);
    InterlockedExchange64(&block->overwritten_events, 0);
    InterlockedExchange64(&block->next_sequence,
                          static_cast<LONG64>(kEventCapacity + 3));
    complete = complete && SignalAndWait(ready, acknowledge);

    UnmapViewOfFile(block);
    CloseHandle(acknowledge);
    CloseHandle(ready);
    CloseHandle(mapping);
    return complete ? 0 : 4;
}

} // namespace detail
} // namespace isaac_cs_t0::coop_passive
#endif

namespace isaac_cs_t0::coop_passive {
namespace {

bool ValidateMapping() noexcept
{
    if (!g_shared || g_shared->magic != kIpcMagic ||
        g_shared->version !=
            (g_shared->scenario == static_cast<std::uint32_t>(Scenario::DeviceRebind)
                 ? kDeviceSchemaVersion : kSchemaVersion) ||
        g_shared->struct_size != sizeof(SharedCoopPassiveBlock) ||
        g_shared->target_pid != GetCurrentProcessId() ||
        !ScenarioAllowed(g_shared->scenario) ||
        g_shared->process_creation_time_100ns == 0 ||
        !Nonzero(g_shared->nonce) ||
        !Nonzero(g_shared->canonical_target_sha256) ||
        !Nonzero(g_shared->observed_target_sha256) ||
        !Nonzero(g_shared->target_equivalence_sha256) ||
        !Nonzero(g_shared->catalog_sha256) ||
        !Nonzero(g_shared->graph_sha256) ||
        !Nonzero(g_shared->plan_sha256) ||
        !Nonzero(g_shared->verification_sha256) ||
        g_shared->loop_mapping_name[0] == L'\0' ||
        std::find(g_shared->loop_mapping_name.begin(),
                  g_shared->loop_mapping_name.end(), L'\0') ==
            g_shared->loop_mapping_name.end())
        return false;
    return g_shared->state == static_cast<LONG>(NativeState::Empty) &&
           g_shared->error_code == 0 && g_shared->installed_hooks_mask == 0 &&
           g_shared->restored_hooks_mask == 0 &&
           g_shared->restored_slot_count == 0 && g_shared->main_thread_id == 0 &&
           g_shared->owner_generation == 0 && g_shared->next_sequence == 0 &&
           g_shared->controller_sequence == 0 &&
           g_shared->overwritten_events == 0 && g_shared->active_hooks == 0 &&
           g_shared->producer_closed == 0 &&
           g_shared->publication_enabled == 0 &&
           g_shared->stable_registry_retired == 0 &&
           g_shared->override_count == 0 && g_shared->apply_count == 0 &&
           g_shared->world_write_count == 0 && g_shared->scheduler_count == 0 &&
           g_shared->transport_count == 0 &&
           g_shared->mode_latch_write_count == 0;
}

bool IsTargetImage() noexcept
{
    wchar_t path[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(nullptr, path, _countof(path));
    if (!length || length >= _countof(path))
        return false;
    const wchar_t* name = std::wcsrchr(path, L'\\');
    name = name ? name + 1 : path;
    return _wcsicmp(name, L"isaac-ng.exe") == 0 &&
           target_image::IsApproved(g_main_base,
                                    target_image::Profile::ApprovedUnpacked);
}

bool HasConflictingRuntime() noexcept
{
    return IsDebuggerPresent() != FALSE ||
           GetModuleHandleW(L"IsaacSync.Native.dll") != nullptr ||
           GetModuleHandleW(L"tractor_beam_native_hook.dll") != nullptr ||
           GetModuleHandleW(L"x32dbg.dll") != nullptr ||
           GetModuleHandleW(L"x64dbg.dll") != nullptr;
}

void GenerateTokenSecret() noexcept
{
    struct SecretMaterial {
        std::array<std::uint8_t, kDigestSize> nonce{};
        std::uint64_t creation_time = 0;
        std::int64_t qpc = 0;
        std::uint32_t pid = 0;
        std::uintptr_t mapping = 0;
    } material{};
    material.nonce = g_shared->nonce;
    material.creation_time = g_shared->process_creation_time_100ns;
    material.qpc = Qpc();
    material.pid = GetCurrentProcessId();
    material.mapping = reinterpret_cast<std::uintptr_t>(g_shared);
    g_token_secret = mode::Sha256(&material, sizeof(material));
}

DWORD Initialize(const wchar_t* mapping_name) noexcept
{
    if (!mapping_name || !*mapping_name || g_shared)
        return static_cast<DWORD>(ErrorCode::MappingInvalid);
    g_mapping = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, mapping_name);
    if (!g_mapping)
        return static_cast<DWORD>(ErrorCode::MappingInvalid);
    g_shared = static_cast<SharedCoopPassiveBlock*>(
        MapViewOfFile(g_mapping, FILE_MAP_ALL_ACCESS, 0, 0,
                      sizeof(SharedCoopPassiveBlock)));
    if (!ValidateMapping())
    {
        SetState(ErrorCode::MappingInvalid, NativeState::Failed);
        return static_cast<DWORD>(ErrorCode::MappingInvalid);
    }
    InterlockedExchange(&g_shared->state,
                        static_cast<LONG>(NativeState::Initializing));
    InterlockedExchange(&g_shared->publication_enabled, 1);
    InterlockedExchange(&g_observation_enabled, 0);
    InterlockedExchange(&g_unloading, 0);
    InterlockedExchange(&g_active_hooks, 0);
    InterlockedExchange(&g_failure_reported, 0);
    InterlockedExchange(&g_device_source_invalid, 0);
    g_main_base = reinterpret_cast<std::uint8_t*>(GetModuleHandleW(nullptr));
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
    if (!PrepareAdditionalTargets())
    {
        SetState(ErrorCode::TargetMismatch, NativeState::Failed);
        return static_cast<DWORD>(ErrorCode::TargetMismatch);
    }
    GenerateTokenSecret();
    g_binding.Reset();
    ResetOrdinaryRouteTracker();
    g_device_route = {};
    g_device_assignment_depth = 0;
    g_manager_pass = 0;
    g_lifecycle_depth = 0;
    g_manager_depth = 0;
    g_join_depth = 0;
    InterlockedExchange64(&g_shared->owner_generation,
                          static_cast<LONG64>(g_binding.generation()));
    Publish(Event(EventKind::NativeInitialized));

    loop::EmbeddedObserverHooks hooks{};
    hooks.relative = g_relative.data();
    hooks.relative_count = AdditionalRelativeCount();
    hooks.relative_ordinals = kAdditionalRelativeOrdinals.data();
    hooks.pointers = g_pointers.data();
    hooks.pointer_count = g_pointers.size();
    hooks.pointer_ordinals = kAdditionalPointerOrdinals.data();
    hooks.strict_ordered_transaction = true;
    hooks.strict_slot_snapshot_before =
        &g_shared->slot_snapshot_before_sha256;
    hooks.lifecycle = &LifecycleCallback;
    loop::EmbeddedObserverIdentity identity{};
    identity.target_pid = g_shared->target_pid;
    identity.scenario = g_shared->scenario;
    identity.nonce = g_shared->nonce;
    const DWORD companion = loop::StartEmbeddedObserver(
        g_shared->loop_mapping_name.data(), hooks, identity);
    if (companion != 0)
    {
        const bool unsafe = companion == kUnsafeResult;
        SetState(unsafe ? ErrorCode::UnsafeShutdown
                        : ErrorCode::HookInstallFailed,
                 unsafe ? NativeState::Unsafe : NativeState::Failed);
        return unsafe ? kUnsafeResult
                      : static_cast<DWORD>(ErrorCode::HookInstallFailed);
    }
    InterlockedExchange64(&g_shared->installed_hooks_mask,
                          static_cast<LONG64>(CompleteHookMask()));
    Publish(Event(EventKind::HooksInstalled));
    InterlockedExchange(&g_observation_enabled, 1);
    SetState(ErrorCode::None, NativeState::Ready);
    return 0;
}

bool ShutdownDrainCallbacks() noexcept
{
#if defined(ISAAC_CS_T0_TESTING)
    if (g_testing_shutdown_dependencies &&
        g_testing_shutdown_dependencies->wait_for_callbacks_drained)
        return g_testing_shutdown_dependencies->wait_for_callbacks_drained(
            g_testing_shutdown_dependencies->context);
#endif
    return WaitForCallbacksDrained();
}

DWORD ShutdownRestoreHooks() noexcept
{
#if defined(ISAAC_CS_T0_TESTING)
    if (g_testing_shutdown_dependencies &&
        g_testing_shutdown_dependencies->restore_embedded_input_hooks)
        return g_testing_shutdown_dependencies->restore_embedded_input_hooks(
            g_testing_shutdown_dependencies->context);
#endif
    return loop::RestoreEmbeddedInputHooks();
}

bool ShutdownVerifyAdditionalTargets() noexcept
{
#if defined(ISAAC_CS_T0_TESTING)
    if (g_testing_shutdown_dependencies &&
        g_testing_shutdown_dependencies->verify_additional_targets_restored)
        return g_testing_shutdown_dependencies->verify_additional_targets_restored(
            g_testing_shutdown_dependencies->context);
#endif
    return VerifyAdditionalTargetsRestored();
}

DWORD ShutdownStopObserver() noexcept
{
#if defined(ISAAC_CS_T0_TESTING)
    if (g_testing_shutdown_dependencies &&
        g_testing_shutdown_dependencies->stop_embedded_observer)
        return g_testing_shutdown_dependencies->stop_embedded_observer(
            g_testing_shutdown_dependencies->context);
#endif
    return loop::StopEmbeddedObserver();
}

bool ShutdownVerifyObserver(std::uint32_t& restored) noexcept
{
#if defined(ISAAC_CS_T0_TESTING)
    if (g_testing_shutdown_dependencies &&
        g_testing_shutdown_dependencies->verify_embedded_observer_restored)
        return g_testing_shutdown_dependencies->verify_embedded_observer_restored(
            g_testing_shutdown_dependencies->context, restored);
#endif
    return loop::VerifyEmbeddedObserverRestored(restored);
}

bool ShutdownSnapshotSlots(std::array<std::uint8_t, 32>& digest) noexcept
{
#if defined(ISAAC_CS_T0_TESTING)
    if (g_testing_shutdown_dependencies &&
        g_testing_shutdown_dependencies->snapshot_embedded_observer_slots)
        return g_testing_shutdown_dependencies->snapshot_embedded_observer_slots(
            g_testing_shutdown_dependencies->context, digest);
#endif
    return loop::SnapshotEmbeddedObserverSlots(digest);
}

DWORD Shutdown() noexcept
{
    if (!g_shared || InterlockedExchange(&g_unloading, 1) != 0 ||
        InterlockedCompareExchange(&g_shared->state, 0, 0) ==
            static_cast<LONG>(NativeState::Unsafe))
        return kUnsafeResult;
    InterlockedExchange(&g_observation_enabled, 0);
    SetState(ErrorCode::None, NativeState::ShuttingDown);
    if (!ShutdownDrainCallbacks())
    {
        SetState(ErrorCode::DrainTimeout, NativeState::Unsafe);
        return kUnsafeResult;
    }
    // Admission is closed and callbacks are drained before private identities
    // are discarded. Evidence for an incomplete transaction remains incomplete.
    if (DeviceV2())
    {
        g_device_route = {};
        g_binding.ScrubAfterObservationStopped();
        ResetOrdinaryRouteTracker();
        SecureZeroMemory(g_token_secret.data(), g_token_secret.size());
    }
    if (ShutdownRestoreHooks() != 0)
    {
        SetState(ErrorCode::UnsafeShutdown, NativeState::Unsafe);
        return kUnsafeResult;
    }
    if (!ShutdownDrainCallbacks() || !ShutdownVerifyAdditionalTargets())
    {
        SetState(ErrorCode::RestoreMismatch, NativeState::Unsafe);
        return kUnsafeResult;
    }
    if (ShutdownStopObserver() != 0)
    {
        SetState(ErrorCode::UnsafeShutdown, NativeState::Unsafe);
        return kUnsafeResult;
    }
    std::uint32_t restored = 0;
    if (!ShutdownVerifyObserver(restored) ||
        restored != CombinedSlotCount() ||
        !ShutdownSnapshotSlots(
            g_shared->slot_snapshot_after_sha256) ||
        g_shared->slot_snapshot_before_sha256 !=
            g_shared->slot_snapshot_after_sha256)
    {
        SetState(ErrorCode::RestoreMismatch, NativeState::Unsafe);
        return kUnsafeResult;
    }
    InterlockedExchange64(&g_shared->restored_hooks_mask,
                          static_cast<LONG64>(CompleteHookMask()));
    InterlockedExchange(&g_shared->restored_slot_count,
                        static_cast<LONG>(restored));
    if (DeviceV2())
    {
        if (!Publish(Event(EventKind::HooksRestored)) ||
            !Publish(Event(EventKind::ProducerClosed)))
        {
            // Publish preserves its specific Unsafe reason on ring failure.
            // A closed publication channel must also never report success.
            if (InterlockedCompareExchange(&g_shared->state, 0, 0) !=
                static_cast<LONG>(NativeState::Unsafe))
                SetState(ErrorCode::UnsafeShutdown, NativeState::Unsafe);
            return kUnsafeResult;
        }
    }
    else
    {
        Publish(Event(EventKind::HooksRestored));
        Publish(Event(EventKind::ProducerClosed));
    }
    InterlockedExchange(&g_shared->producer_closed, 1);
    InterlockedExchange(&g_shared->publication_enabled, 0);
    SetState(ErrorCode::None, NativeState::Stopped);
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
    g_main_base = nullptr;
    g_token_secret.fill(0);
    ResetOrdinaryRouteTracker();
    g_device_route = {};
}

bool Active() noexcept
{
    return ObservationEnabled() && g_shared &&
           InterlockedCompareExchange(&g_shared->state, 0, 0) ==
               static_cast<LONG>(NativeState::Ready);
}

extern "C" DWORD WINAPI IsaacCsT0CoopPassiveInitialize(
    const wchar_t* mapping_name)
{
    return Initialize(mapping_name);
}

extern "C" DWORD WINAPI IsaacCsT0CoopPassiveShutdown(void*)
{
    const DWORD result = Shutdown();
    if (result != 0)
        return result;
    HMODULE module = g_module;
    FreeLibraryAndExitThread(module, 0);
}

} // namespace isaac_cs_t0::coop_passive

namespace isaac_cs_t0::coop_passive {
namespace {

void ResetOrdinaryRouteTracker() noexcept
{
    g_ordinary_route = {};
}

void BeginOrdinaryRouteTransaction(void* manager) noexcept
{
    g_ordinary_route.transaction_active = true;
    g_ordinary_route.manager = manager;
    g_ordinary_route.manager_pass = g_manager_pass;
    g_ordinary_route.stage = OrdinaryRouteStage::Idle;
    g_ordinary_route.route_generation = 0;
}

void RevokeOrdinaryRouteWitness() noexcept
{
    g_ordinary_route.authorized_generation = 0;
    g_ordinary_route.route_generation = 0;
    if (g_ordinary_route.transaction_active)
        g_ordinary_route.stage = OrdinaryRouteStage::Rejected;
}

void EndOrdinaryRouteTransaction() noexcept
{
    if (g_ordinary_route.transaction_active &&
        g_ordinary_route.stage != OrdinaryRouteStage::Idle &&
        g_ordinary_route.stage != OrdinaryRouteStage::Complete)
        RevokeOrdinaryRouteWitness();
    g_ordinary_route.transaction_active = false;
    g_ordinary_route.manager = nullptr;
    g_ordinary_route.manager_pass = 0;
    g_ordinary_route.stage = OrdinaryRouteStage::Idle;
    g_ordinary_route.route_generation = 0;
}

bool StartOrdinaryRoute() noexcept
{
    if (!g_ordinary_route.transaction_active ||
        g_ordinary_route.stage != OrdinaryRouteStage::Rejected ||
        g_ordinary_route.manager_pass != g_manager_pass)
    {
        RevokeOrdinaryRouteWitness();
        return false;
    }
    g_ordinary_route.route_generation = g_binding.generation();
    g_ordinary_route.stage = OrdinaryRouteStage::SpawnBegun;
    return true;
}

bool AdvanceOrdinaryRoute(OrdinaryRouteStage expected,
                          OrdinaryRouteStage next) noexcept
{
    if (!g_ordinary_route.transaction_active ||
        g_ordinary_route.stage != expected ||
        g_ordinary_route.manager_pass != g_manager_pass ||
        g_ordinary_route.route_generation != g_binding.generation())
    {
        RevokeOrdinaryRouteWitness();
        return false;
    }
    g_ordinary_route.stage = next;
    if (next == OrdinaryRouteStage::Complete)
        g_ordinary_route.authorized_generation =
            g_ordinary_route.route_generation;
    return true;
}

bool OrdinaryAssignmentContinuesRoute() noexcept
{
    return g_ordinary_route.transaction_active &&
           g_ordinary_route.stage == OrdinaryRouteStage::SpawnEnded &&
           g_ordinary_route.manager_pass == g_manager_pass &&
           g_ordinary_route.route_generation == g_binding.generation();
}

bool OrdinaryRouteAuthorizesBackend() noexcept
{
    return g_ordinary_route.authorized_generation != 0 &&
           g_ordinary_route.authorized_generation == g_binding.generation();
}

void PublishWrapper(EventKind kind, std::uint32_t slot,
                    std::uint32_t original_calls, void* return_address) noexcept
{
    auto event = Event(kind, slot);
    event.original_calls = original_calls;
    (void)return_address;
    if (original_calls == 1)
        event.flags |= kFlagOriginalCalled;
    Publish(event);
}

extern "C" bool __fastcall PassiveKeyboardPressedHook(void* self, void*,
                                                       std::uint32_t action)
{
    HookScope scope;
    return g_keyboard_pressed(self, action);
}

extern "C" bool __fastcall PassiveKeyboardTriggeredHook(void* self, void*,
                                                         std::uint32_t action)
{
    HookScope scope;
    return g_keyboard_triggered(self, action);
}

extern "C" float __fastcall PassiveKeyboardScalarHook(void* self, void*,
                                                       std::uint32_t action)
{
    HookScope scope;
    return g_keyboard_scalar(self, action);
}

extern "C" bool __fastcall PassiveGamepadPressedHook(void* self, void*,
                                                      std::uint32_t action)
{
    HookScope scope;
    return g_gamepad_pressed(self, action);
}

extern "C" bool __fastcall PassiveGamepadTriggeredHook(void* self, void*,
                                                        std::uint32_t action)
{
    HookScope scope;
    return g_gamepad_triggered(self, action);
}

extern "C" float __fastcall PassiveGamepadScalarHook(void* self, void*,
                                                      std::uint32_t action)
{
    HookScope scope;
    return g_gamepad_scalar(self, action);
}

extern "C" void __fastcall PassivePlayerUpdateHook(void* self, void*)
{
    HookScope scope;
    g_player_update(self);
}

extern "C" void __fastcall PassiveMovementConsumerHook(void* self, void*)
{
    HookScope scope;
    g_consumers[0](self);
}

extern "C" void __fastcall PassiveBombConsumerHook(void* self, void*)
{
    HookScope scope;
    g_consumers[1](self);
}

extern "C" void __fastcall PassiveItemConsumerHook(void* self, void*,
                                                    std::int32_t slot)
{
    HookScope scope;
    g_item_consumer(self, slot);
}

extern "C" void __fastcall PassivePillConsumerHook(void* self, void*)
{
    HookScope scope;
    g_consumers[3](self);
}

extern "C" void __fastcall PassiveShootingConsumerHook(void* self, void*)
{
    HookScope scope;
    g_consumers[4](self);
}

extern "C" void __fastcall PassiveDropConsumerHook(void* self, void*)
{
    HookScope scope;
    g_consumers[5](self);
}

extern "C" std::uint32_t __fastcall CoopManagerOwnerHook(void* self, void*)
{
    HookScope scope;
    const bool entered = ObservationEnabled() && CheckMainThread();
    bool top_level = false;
    if (entered)
    {
        if (g_manager_depth != 0)
            DisableObservation(ErrorCode::ReentrantManager, 45);
        else
        {
            ++g_manager_depth;
            top_level = true;
            BeginOrdinaryRouteTransaction(self);
            PublishWrapper(EventKind::ManagerPassBegin, 45, 0,
                           _ReturnAddress());
        }
    }
    const std::uint32_t result = g_manager(self);
    if (top_level)
    {
        --g_manager_depth;
        CompleteManagerPass(self, _ReturnAddress());
        EndOrdinaryRouteTransaction();
    }
    return result;
}

extern "C" void* __fastcall CoopOrdinarySpawnHook(void* self, void*,
                                                   std::int32_t player_type)
{
    HookScope scope;
    const bool entered = ObservationEnabled() && CheckMainThread();
    if (entered)
    {
        if (g_join_depth++ == 0)
        {
            if (InvalidateBindings(LifecycleReason::NativeJoin, true))
                StartOrdinaryRoute();
        }
        else
        {
            RevokeOrdinaryRouteWitness();
        }
        PublishWrapper(EventKind::OrdinarySpawnBegin, 46, 0,
                       _ReturnAddress());
    }
    void* const result = g_spawn(self, player_type);
    if (entered)
    {
        PublishWrapper(EventKind::OrdinarySpawnEnd, 46, 1,
                       _ReturnAddress());
        AdvanceOrdinaryRoute(OrdinaryRouteStage::SpawnBegun,
                             OrdinaryRouteStage::SpawnEnded);
        --g_join_depth;
    }
    return result;
}

extern "C" void* __fastcall CoopPossessorSpawnHook(void* self, void*,
                                                    std::int32_t player_type)
{
    HookScope scope;
    const bool entered = ObservationEnabled() && CheckMainThread();
    if (entered)
    {
        RevokeOrdinaryRouteWitness();
        InterlockedExchange(&g_observation_enabled, 0);
    }
    void* const result = g_spawn(self, player_type);
    if (entered)
    {
        auto event = Event(EventKind::PossessorSpawnStop, 47);
        event.original_calls = 1;
        event.flags |= kFlagOriginalCalled | kFlagFailure;
        Publish(event);
        DisableObservation(ErrorCode::PossessorPath, 47);
    }
    return result;
}

extern "C" void __fastcall CoopNativeExitHook(void* self, void*, void* player,
                                               std::uint32_t padding)
{
    HookScope scope;
    const bool entered = ObservationEnabled() && CheckMainThread();
    if (entered)
    {
        InvalidateBindings(LifecycleReason::NativeExit, true);
        PublishWrapper(EventKind::ExitBegin, 48, 0, _ReturnAddress());
    }
    g_exit(self, player, padding);
    if (entered)
        PublishWrapper(EventKind::ExitReturnStable, 48, 1,
                       _ReturnAddress());
}

extern "C" void __fastcall CoopOrdinaryAssignmentHook(
    void* self, void*, std::int32_t controller_id, std::uint32_t propagate)
{
    HookScope scope;
    const bool entered = ObservationEnabled() && CheckMainThread();
    bool continues_route = false;
    if (entered)
    {
        continues_route = OrdinaryAssignmentContinuesRoute();
        if (continues_route)
            AdvanceOrdinaryRoute(OrdinaryRouteStage::SpawnEnded,
                                 OrdinaryRouteStage::AssignmentBegun);
        else
            InvalidateBindings(LifecycleReason::OrdinaryAssignment, true);
        PublishWrapper(EventKind::OrdinaryAssignmentBegin, 49, 0,
                       _ReturnAddress());
    }
    g_assignment(self, controller_id, propagate);
    if (entered)
    {
        PublishWrapper(EventKind::OrdinaryAssignmentEnd, 49, 1,
                       _ReturnAddress());
        if (continues_route)
            AdvanceOrdinaryRoute(OrdinaryRouteStage::AssignmentBegun,
                                 OrdinaryRouteStage::AssignmentEnded);
    }
}

extern "C" void __fastcall CoopPossessorAssignmentHook(
    void* self, void*, std::int32_t controller_id, std::uint32_t propagate)
{
    HookScope scope;
    const bool entered = ObservationEnabled() && CheckMainThread();
    if (entered)
    {
        RevokeOrdinaryRouteWitness();
        InterlockedExchange(&g_observation_enabled, 0);
    }
    g_assignment(self, controller_id, propagate);
    if (entered)
    {
        auto event = Event(EventKind::PossessorAssignmentStop, 50);
        event.original_calls = 1;
        event.flags |= kFlagOriginalCalled | kFlagFailure;
        Publish(event);
        DisableObservation(ErrorCode::PossessorPath, 50);
    }
}

extern "C" void __fastcall CoopJoinHudRebuildHook(void* self, void*)
{
    HookScope scope;
    const bool entered = ObservationEnabled() && CheckMainThread();
    if (entered)
    {
        AdvanceOrdinaryRoute(OrdinaryRouteStage::AssignmentEnded,
                             OrdinaryRouteStage::HudBegun);
        PublishWrapper(EventKind::JoinHudBegin, 51, 0, _ReturnAddress());
    }
    g_hud(self);
    if (entered)
    {
        PublishWrapper(EventKind::JoinHudEnd, 51, 1, _ReturnAddress());
        AdvanceOrdinaryRoute(OrdinaryRouteStage::HudBegun,
                             OrdinaryRouteStage::Complete);
    }
}

extern "C" void __fastcall CoopExitHudRebuildHook(void* self, void*)
{
    HookScope scope;
    const bool entered = ObservationEnabled() && CheckMainThread();
    if (entered)
    {
        RevokeOrdinaryRouteWitness();
        PublishWrapper(EventKind::ExitHudBegin, 52, 0, _ReturnAddress());
    }
    g_hud(self);
    if (entered)
        PublishWrapper(EventKind::ExitHudEnd, 52, 1, _ReturnAddress());
}

bool PrepareCoopTargets() noexcept
{
    const std::array<void*, kCoopSlotCount> replacements{{
        reinterpret_cast<void*>(&CoopManagerOwnerHook),
        reinterpret_cast<void*>(&CoopOrdinarySpawnHook),
        reinterpret_cast<void*>(&CoopPossessorSpawnHook),
        reinterpret_cast<void*>(&CoopNativeExitHook),
        reinterpret_cast<void*>(&CoopOrdinaryAssignmentHook),
        reinterpret_cast<void*>(&CoopPossessorAssignmentHook),
        reinterpret_cast<void*>(&CoopJoinHudRebuildHook),
        reinterpret_cast<void*>(&CoopExitHudRebuildHook),
    }};
    __try
    {
        for (std::size_t index = 0; index < kCoopSlotCount; ++index)
        {
            auto* const call = g_main_base + kCoopCallRvas[index];
            if (std::memcmp(call, kCoopBytes[index].data(), 5) != 0 ||
                call[0] != 0xE8)
                return false;
            const auto displacement =
                *reinterpret_cast<const std::int32_t*>(call + 1);
            if (call + 5 + displacement != g_main_base + kCoopTargetRvas[index])
                return false;
            auto& patch = g_coop_relative[index];
            patch = {};
            patch.call = call;
            std::copy(kCoopBytes[index].begin(), kCoopBytes[index].end(),
                      patch.expected);
            patch.replacement = replacements[index];
            patch.original = g_main_base + kCoopTargetRvas[index];
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
    g_manager = reinterpret_cast<ManagerFn>(g_main_base + kCoopTargetRvas[0]);
    g_spawn = reinterpret_cast<SpawnFn>(g_main_base + kCoopTargetRvas[1]);
    g_exit = reinterpret_cast<ExitFn>(g_main_base + kCoopTargetRvas[3]);
    g_assignment =
        reinterpret_cast<AssignmentFn>(g_main_base + kCoopTargetRvas[4]);
    g_hud = reinterpret_cast<HudFn>(g_main_base + kCoopTargetRvas[6]);
    return true;
}

bool PrepareAdditionalTargets() noexcept
{
    g_relative = {};
    g_keyboard_pressed =
        reinterpret_cast<LogicalBoolFn>(g_main_base + 0x0061E8D0u);
    g_keyboard_triggered =
        reinterpret_cast<LogicalBoolFn>(g_main_base + 0x0061E970u);
    g_keyboard_scalar =
        reinterpret_cast<LogicalScalarFn>(g_main_base + 0x0061EA20u);
    g_gamepad_pressed = g_keyboard_pressed;
    g_gamepad_triggered = g_keyboard_triggered;
    g_gamepad_scalar = g_keyboard_scalar;
    g_player_update = reinterpret_cast<PlayerUpdateFn>(g_main_base + 0x00382AF0u);
    g_consumers = {{
        reinterpret_cast<ConsumerFn>(g_main_base + 0x00379B40u),
        reinterpret_cast<ConsumerFn>(g_main_base + 0x00380F40u),
        nullptr,
        reinterpret_cast<ConsumerFn>(g_main_base + 0x003A3D30u),
        reinterpret_cast<ConsumerFn>(g_main_base + 0x0037B020u),
        reinterpret_cast<ConsumerFn>(g_main_base + 0x003A42B0u),
    }};
    g_item_consumer =
        reinterpret_cast<ItemConsumerFn>(g_main_base + 0x00391A40u);

    active::ActiveReplacements active_replacements{};
    active_replacements.relative = {{
        reinterpret_cast<void*>(&PassiveMovementConsumerHook),
        reinterpret_cast<void*>(&PassiveBombConsumerHook),
        reinterpret_cast<void*>(&PassiveItemConsumerHook),
        reinterpret_cast<void*>(&PassivePillConsumerHook),
        reinterpret_cast<void*>(&PassiveShootingConsumerHook),
        reinterpret_cast<void*>(&PassiveDropConsumerHook),
    }};
    active_replacements.pointers = {{
        reinterpret_cast<void*>(&PassiveKeyboardPressedHook),
        reinterpret_cast<void*>(&PassiveKeyboardTriggeredHook),
        reinterpret_cast<void*>(&PassiveKeyboardScalarHook),
        reinterpret_cast<void*>(&PassiveGamepadPressedHook),
        reinterpret_cast<void*>(&PassiveGamepadTriggeredHook),
        reinterpret_cast<void*>(&PassiveGamepadScalarHook),
        reinterpret_cast<void*>(&PassivePlayerUpdateHook),
    }};
    active::TargetFailure failure{};
    if (!active::PrepareActiveTargets(g_main_base, active_replacements,
                                      g_active_relative, g_pointers, failure) ||
        !PrepareCoopTargets())
        return false;
    for (std::size_t index = 0; index < g_active_relative.size(); ++index)
        g_relative[index] = g_active_relative[index];
    for (std::size_t index = 0; index < g_coop_relative.size(); ++index)
        g_relative[g_active_relative.size() + index] = g_coop_relative[index];
    if (DeviceV2())
    {
        const std::array<void*, kDeviceAdditionalSlotCount> replacements{{
            reinterpret_cast<void*>(&CoopDeviceMenuUnassignHook),
            reinterpret_cast<void*>(&CoopDeviceDisconnectUnassignHook),
            reinterpret_cast<void*>(&CoopDeviceReassignHook),
        }};
        constexpr std::array<std::uint32_t, 3> feature_rvas{{
            0x005B27E5u, 0x0055B879u, 0x0055BC9Bu,
        }};
        constexpr std::array<std::array<std::uint8_t, 25>, 3> features{{
            {{0x6A,0x01,0x6A,0xFF,0x8B,0xC8,0xE8,0,0,0,0,
              0xC6,0x87,0xD0,0xAB,0x04,0x00,0x01}},
            {{0x6A,0x01,0x6A,0xFF,0x8B,0xC8,0xE8,0,0,0,0,
              0xC6,0x45,0xFC,0x05,0x8D,0x8D,0x10,0xFF,0xFF,0xFF}},
            {{0x8B,0x0A,0x6A,0x01,0x53,0xE8,0,0,0,0,0xA1,0,0,0,0,
              0x85,0xF6,0x0F,0x44,0xC3,0xA3,0,0,0,0}},
        }};
        constexpr std::array<const char*, 3> masks{{
            "xxxxxxx????xxxxxxx", "xxxxxxx????xxxxxxxxxx",
            "xxxxxx????x????xxxxxx????",
        }};
        __try
        {
            for (std::size_t index = 0; index < replacements.size(); ++index)
            {
                auto* const call = g_main_base + kDeviceCallRvas[index];
                if (std::memcmp(call, kDeviceCallBytes[index].data(), 5) != 0 ||
                    call + 5 + *reinterpret_cast<const std::int32_t*>(call + 1) !=
                        g_main_base + 0x003A6450u)
                    return false;
                for (std::size_t byte = 0; masks[index][byte]; ++byte)
                    if (masks[index][byte] == 'x' &&
                        g_main_base[feature_rvas[index] + byte] != features[index][byte])
                        return false;
                auto& patch = g_relative[kAdditionalRelativeCount + index];
                patch.call = call;
                std::copy(kDeviceCallBytes[index].begin(), kDeviceCallBytes[index].end(),
                          patch.expected);
                patch.replacement = replacements[index];
                patch.original = g_main_base + 0x003A6450u;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }
    return true;
}

bool VerifyAdditionalTargetsRestored() noexcept
{
    __try
    {
        for (std::size_t index = 0; index < AdditionalRelativeCount(); ++index)
        {
            const auto& patch = g_relative[index];
            if (patch.installed || !patch.call ||
                std::memcmp(patch.call, patch.expected, sizeof(patch.expected)) != 0)
                return false;
        }
        for (const auto& patch : g_pointers)
            if (patch.installed || !patch.slot || !patch.expected_original ||
                *patch.slot != patch.expected_original)
                return false;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
    return true;
}

} // namespace
} // namespace isaac_cs_t0::coop_passive

namespace isaac_cs_t0::coop_passive {
namespace {

bool ReadVectorHeader(const void* address, std::size_t stride,
                      std::size_t maximum, VectorHeader& header,
                      std::size_t& count) noexcept
{
    if (!address || stride == 0)
        return false;
    const auto* slots = static_cast<std::uint8_t* const*>(address);
    header = {slots[0], slots[1], slots[2]};
    const auto begin = reinterpret_cast<std::uintptr_t>(header.begin);
    const auto end = reinterpret_cast<std::uintptr_t>(header.end);
    const auto capacity = reinterpret_cast<std::uintptr_t>(header.capacity_end);
    const std::uintptr_t alignment = std::min<std::size_t>(stride, 4);
    if ((begin == 0) != (end == 0) || (begin == 0) != (capacity == 0) ||
        begin % alignment != 0 || end % alignment != 0 ||
        capacity % alignment != 0 || end < begin || capacity < end ||
        (end - begin) % stride != 0)
        return false;
    count = static_cast<std::size_t>((end - begin) / stride);
    return count <= maximum;
}

bool KnownDevice(void* object, DeviceKind& kind) noexcept
{
    if (!object)
        return false;
    auto* const vtable = *reinterpret_cast<std::uint8_t**>(object);
    // The immutable image vtable/COL pairs are validated once by
    // active::PrepareActiveTargets. The frozen C0 Local classifier identifies
    // each live device by its exact vtable; requiring a second object-time COL
    // read here added a condition that the accepted Local sources never used.
    if (vtable == g_main_base + kKeyboardVtableRva)
    {
        kind = DeviceKind::Keyboard;
        return true;
    }
    if (vtable == g_main_base + kGamepadVtableRva)
    {
        kind = DeviceKind::Gamepad;
        return true;
    }
    return false;
}

enum class TopologyReadResult : std::uint32_t {
    Ok,
    Transient,
    ReadFailure,
    Ambiguous,
};

TopologyReadResult TopologyResult(
    TopologyReadResult result, detail::TopologyDiagnostic reason,
    detail::TopologyDiagnostic& diagnostic) noexcept
{
    diagnostic = reason;
    return result;
}

TopologyReadResult RejectOnlineTopology(
    TopologyReadResult result, detail::TopologyDiagnostic reason,
    detail::TopologyDiagnostic& diagnostic) noexcept
{
    RevokeOrdinaryRouteWitness();
    return TopologyResult(result, reason, diagnostic);
}

TopologyReadResult ReadTopology(void* expected_manager,
                                Topology& topology,
                                detail::TopologyDiagnostic& diagnostic,
                                std::uint32_t detached_ordinal = kNoOrdinal) noexcept
{
    topology = {};
    diagnostic = detail::TopologyDiagnostic::None;
    __try
    {
#if defined(ISAAC_CS_T0_TESTING)
        if (g_testing_force_topology_exception)
            RaiseException(EXCEPTION_ACCESS_VIOLATION, 0, 0, nullptr);
#endif
        void* const root = *reinterpret_cast<void**>(g_main_base + kGameRootSlotRva);
        if (!root ||
            (*reinterpret_cast<const std::uint16_t*>(
                 static_cast<std::uint8_t*>(root) + kRunActiveOffset) &
             0x00FFu) == 0)
            return TopologyResult(TopologyReadResult::Transient,
                                  detail::TopologyDiagnostic::RunNotActive,
                                  diagnostic);
        topology.game_root = root;
        topology.player_manager = static_cast<std::uint8_t*>(root) +
                                  kPlayerManagerOffset;
        if (expected_manager && topology.player_manager != expected_manager)
            return TopologyResult(
                TopologyReadResult::Ambiguous,
                detail::TopologyDiagnostic::ManagerOwnerMismatch, diagnostic);
        topology.network_backend =
            *reinterpret_cast<void**>(g_main_base + kNetworkBackendSlotRva);
        if (topology.network_backend)
        {
            if ((reinterpret_cast<std::uintptr_t>(topology.network_backend) &
                 3u) != 0)
                return RejectOnlineTopology(
                    TopologyReadResult::Ambiguous,
                    detail::TopologyDiagnostic::OnlineBackendPresent,
                    diagnostic);
            auto* const backend_vtable =
                *reinterpret_cast<std::uint8_t**>(topology.network_backend);
            if (backend_vtable != g_main_base + kSteamBackendVtableRva)
                return RejectOnlineTopology(
                    TopologyReadResult::Ambiguous,
                    detail::TopologyDiagnostic::OnlineBackendPresent,
                    diagnostic);
        }
        topology.online_manager =
            *reinterpret_cast<void**>(g_main_base + kOnlineManagerSlotRva);
        if (topology.online_manager)
        {
            if ((reinterpret_cast<std::uintptr_t>(topology.online_manager) & 3u) != 0)
                return RejectOnlineTopology(
                    TopologyReadResult::Ambiguous,
                    detail::TopologyDiagnostic::InvalidOnlineManager,
                    diagnostic);
            VectorHeader roster{};
            std::size_t roster_count = 0;
            if (!ReadVectorHeader(static_cast<std::uint8_t*>(topology.online_manager) +
                                      kOnlineRosterOffset,
                                  sizeof(void*), kMaximumRosterCount, roster,
                                  roster_count))
                return RejectOnlineTopology(
                    TopologyReadResult::Transient,
                    detail::TopologyDiagnostic::OnlineRosterHeaderInvalid,
                    diagnostic);
            if (roster_count != 0 || roster.begin || roster.end ||
                roster.capacity_end)
                return RejectOnlineTopology(
                    TopologyReadResult::Ambiguous,
                    detail::TopologyDiagnostic::OnlineRosterPresent, diagnostic);
            topology.online_frame = *reinterpret_cast<const std::uint32_t*>(
                static_cast<const std::uint8_t*>(topology.online_manager) +
                kOnlineFrameOffset);
            if (topology.online_frame != 0)
                return RejectOnlineTopology(
                    TopologyReadResult::Ambiguous,
                    detail::TopologyDiagnostic::OfflineOnlineFrameNonzero,
                    diagnostic);
        }
        if (topology.network_backend && !OrdinaryRouteAuthorizesBackend() &&
            !DeviceRouteAuthorizesBackend())
            return RejectOnlineTopology(
                TopologyReadResult::Ambiguous,
                detail::TopologyDiagnostic::OnlineBackendPresent, diagnostic);

        VectorHeader devices{};
        if (!ReadVectorHeader(g_main_base + kInputManagerRva + 8,
                              sizeof(void*) * 2, kMaximumDeviceCount,
                              devices, topology.device_count))
            return TopologyResult(
                TopologyReadResult::Transient,
                detail::TopologyDiagnostic::DeviceVectorInvalid, diagnostic);
        for (std::size_t index = 0; index < topology.device_count; ++index)
        {
            auto** const entry = reinterpret_cast<void**>(
                devices.begin + index * sizeof(void*) * 2);
            auto& device = topology.devices[index];
            device.object = entry[0];
            device.reference_owner = entry[1];
            if (!device.object || !device.reference_owner ||
                (reinterpret_cast<std::uintptr_t>(device.object) & 3u) != 0 ||
                (reinterpret_cast<std::uintptr_t>(device.reference_owner) & 3u) != 0)
                return TopologyResult(
                    TopologyReadResult::Transient,
                    detail::TopologyDiagnostic::InvalidDeviceEntry, diagnostic);
            if (!KnownDevice(device.object, device.kind))
                return TopologyResult(
                    TopologyReadResult::Ambiguous,
                    detail::TopologyDiagnostic::UnknownDeviceVtable, diagnostic);
            device.assignment = *reinterpret_cast<const std::int32_t*>(
                static_cast<const std::uint8_t*>(device.object) +
                kDeviceAssignmentOffset);
        }

        VectorHeader players{};
        if (!ReadVectorHeader(topology.player_manager, sizeof(void*),
                              kMaximumPlayerCount, players,
                              topology.player_count))
            return TopologyResult(
                TopologyReadResult::Transient,
                detail::TopologyDiagnostic::PlayerVectorInvalid, diagnostic);
        if (topology.player_count == 0)
            return TopologyResult(
                TopologyReadResult::Transient,
                detail::TopologyDiagnostic::PlayerVectorEmpty, diagnostic);
        if (topology.player_count > 2)
            return TopologyResult(
                TopologyReadResult::Ambiguous,
                detail::TopologyDiagnostic::UnexpectedThirdPlayer, diagnostic);
        for (std::size_t index = 0; index < topology.player_count; ++index)
        {
            auto& player = topology.players[index];
            player.object = reinterpret_cast<void**>(players.begin)[index];
            if (!player.object ||
                (reinterpret_cast<std::uintptr_t>(player.object) & 3u) != 0)
                return TopologyResult(
                    TopologyReadResult::Ambiguous,
                    detail::TopologyDiagnostic::InvalidPlayerEntry, diagnostic);
            auto* const vtable = *reinterpret_cast<std::uint8_t**>(player.object);
            if (vtable != g_main_base + kPlayerVtableRva)
                return TopologyResult(
                    TopologyReadResult::Ambiguous,
                    detail::TopologyDiagnostic::UnknownPlayerVtable, diagnostic);
            if (reinterpret_cast<void**>(vtable)[-1] !=
                g_main_base + kPlayerColRva)
                return TopologyResult(
                    TopologyReadResult::Ambiguous,
                    detail::TopologyDiagnostic::UnknownPlayerCol, diagnostic);
            if (*reinterpret_cast<void**>(
                    static_cast<std::uint8_t*>(player.object) +
                    kPlayerTwinOffset) ||
                (DeviceV2() &&
                 (*reinterpret_cast<const std::uint32_t*>(
                      static_cast<const std::uint8_t*>(player.object) +
                      kPlayerVariantOffset) != 0 ||
                  *reinterpret_cast<void**>(static_cast<std::uint8_t*>(player.object) +
                                            kPlayerParentOffset) != nullptr)))
                return TopologyResult(
                    TopologyReadResult::Ambiguous,
                    detail::TopologyDiagnostic::TwinOrPossessor, diagnostic);
            player.assignment = *reinterpret_cast<const std::int32_t*>(
                static_cast<const std::uint8_t*>(player.object) +
                kPlayerAssignmentOffset);
            player.ordinal = *reinterpret_cast<const std::uint32_t*>(
                static_cast<const std::uint8_t*>(player.object) +
                kPlayerOrdinalOffset);
            if (player.ordinal != index)
                return TopologyResult(
                    TopologyReadResult::Transient,
                    detail::TopologyDiagnostic::PlayerOrdinalMismatch,
                    diagnostic);
            const bool permitted_detached = DeviceV2() && index == detached_ordinal;
            if (player.assignment < 0 &&
                (!permitted_detached || player.assignment != -1))
                return TopologyResult(
                    TopologyReadResult::Transient,
                    detail::TopologyDiagnostic::PlayerAssignmentPending,
                    diagnostic);
            std::size_t matches = 0;
            for (std::size_t device_index = 0;
                 device_index < topology.device_count; ++device_index)
            {
                const auto& device = topology.devices[device_index];
                if (player.assignment >= 0 && device.assignment == player.assignment)
                {
                    ++matches;
                    player.device = device.object;
                    player.device_kind = device.kind;
                }
            }
            if (matches != 1 && !(permitted_detached && matches == 0))
                return TopologyResult(
                    TopologyReadResult::Transient,
                    detail::TopologyDiagnostic::AssignmentNotUnique, diagnostic);
            for (std::size_t prior = 0; prior < index; ++prior)
            {
                if (topology.players[prior].object == player.object)
                    return TopologyResult(
                        TopologyReadResult::Ambiguous,
                        detail::TopologyDiagnostic::DuplicatePlayer, diagnostic);
                if (topology.players[prior].device == player.device)
                    return TopologyResult(
                        TopologyReadResult::Transient,
                        detail::TopologyDiagnostic::DuplicateAssignedDevice,
                        diagnostic);
            }
        }

        auto* const hud = static_cast<std::uint8_t*>(root) + kHudOffset;
        for (std::size_t index = 0; index < topology.hud.size(); ++index)
            topology.hud[index] = *reinterpret_cast<void**>(
                hud + index * kHudStride);
        for (std::size_t index = 0; index < 4; ++index)
        {
            const void* const expected =
                index < topology.player_count ? topology.players[index].object
                                              : nullptr;
            if (topology.hud[index] != expected || topology.hud[index + 4])
                return TopologyResult(
                    TopologyReadResult::Transient,
                    detail::TopologyDiagnostic::HudMismatch, diagnostic);
        }
        return TopologyResult(TopologyReadResult::Ok,
                              detail::TopologyDiagnostic::None, diagnostic);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        RevokeOrdinaryRouteWitness();
        return TopologyResult(TopologyReadResult::ReadFailure,
                              detail::TopologyDiagnostic::AccessViolation,
                              diagnostic);
    }
}

TopologyReadResult TakeCoherentTopology(void* manager,
                                        Topology& topology,
                                        detail::TopologyDiagnostic& diagnostic) noexcept
{
    Topology first{};
    const auto first_result = ReadTopology(manager, first, diagnostic);
    if (first_result != TopologyReadResult::Ok)
        return first_result;
    MemoryBarrier();
    Topology second{};
    const auto second_result = ReadTopology(manager, second, diagnostic);
    if (second_result != TopologyReadResult::Ok)
        return second_result;
    if (!(first == second))
        return TopologyResult(
            TopologyReadResult::Transient,
            detail::TopologyDiagnostic::StructuralDoubleReadDrift, diagnostic);
    topology = second;
    return TopologyResult(TopologyReadResult::Ok,
                          detail::TopologyDiagnostic::None, diagnostic);
}

// This relaxed read is only a causal identity check. It can accept a missing
// device for the one detached endpoint, and must never feed BindingMachine.
bool ReadDeviceIdentity(const Topology& baseline, std::uint32_t ordinal,
                         Topology& current) noexcept
{
    if (!DeviceV2() || ordinal > 1 || baseline.player_count != 2)
        return false;
    Topology second{};
    detail::TopologyDiagnostic diagnostic = detail::TopologyDiagnostic::None;
    if (ReadTopology(baseline.player_manager, current, diagnostic, ordinal) !=
            TopologyReadResult::Ok ||
        ReadTopology(baseline.player_manager, second, diagnostic, ordinal) !=
            TopologyReadResult::Ok || current != second)
        return false;
    if (current.game_root != baseline.game_root ||
        current.player_manager != baseline.player_manager ||
        current.online_manager != baseline.online_manager ||
        current.network_backend != baseline.network_backend ||
        current.online_frame != 0 || current.player_count != 2 ||
        current.hud != baseline.hud ||
        current.players[ordinal].object != baseline.players[ordinal].object ||
        current.players[ordinal].ordinal != ordinal ||
        current.players[1 - ordinal] != baseline.players[1 - ordinal])
        return false;
    const auto* other_device = baseline.players[1 - ordinal].device;
    const DeviceObservation* before_device = nullptr;
    const DeviceObservation* after_device = nullptr;
    for (std::size_t index = 0; index < baseline.device_count; ++index)
        if (baseline.devices[index].object == other_device)
            before_device = &baseline.devices[index];
    for (std::size_t index = 0; index < current.device_count; ++index)
        if (current.devices[index].object == other_device)
            after_device = &current.devices[index];
    // A recycled same-address object with a different shared owner is not an
    // unchanged other endpoint. Only the target's device may disappear/rebind.
    if (!before_device || !after_device || *before_device != *after_device)
        return false;
    return true;
}

bool DeviceEvent(EventKind kind, std::uint32_t slot,
                  std::uint32_t original_calls, std::int32_t assignment,
                  bool full_baseline = false) noexcept
{
    auto event = Event(kind, slot);
    const auto ordinal = g_device_route.ordinal;
    if (full_baseline)
        FillRelation(event, g_device_route.baseline, ordinal);
    else
    {
        event.local_ordinal = ordinal;
        event.endpoint = ordinal + 1;
        event.flags |= ordinal == 0 ? kFlagEndpointA : kFlagEndpointB;
        event.player_digest = Token(1, reinterpret_cast<std::uintptr_t>(
            g_device_route.baseline.players[ordinal].object));
        event.assignment_digest = Token(3, static_cast<std::uint32_t>(assignment), ordinal);
    }
    event.phase = static_cast<std::uint32_t>(LifecycleReason::DeviceRebind);
    event.depth = 0;
    event.original_calls = original_calls;
    if (original_calls == 1)
        event.flags |= kFlagOriginalCalled;
    return Publish(event);
}

bool BeginDeviceAssignment(std::uint32_t slot, void* self,
                            std::int32_t assignment,
                            std::uint32_t propagate) noexcept
{
    if (g_device_assignment_depth != 1 || g_manager_depth != 0 ||
        g_lifecycle_depth != 0 || propagate != 1 || g_binding.retired())
    {
        RejectDeviceRoute();
        return false;
    }
    if (g_device_route.stage == DeviceRouteStage::Idle)
    {
        if ((slot != 53 && slot != 54) || assignment != -1 || !g_binding.bound() ||
            g_binding.bound_topology().player_count != 2 ||
            g_binding.states()[0] != BindingState::Bound ||
            g_binding.states()[1] != BindingState::Bound)
        {
            RejectDeviceRoute();
            return false;
        }
        const Topology baseline = g_binding.bound_topology();
        std::uint32_t ordinal = kNoOrdinal;
        for (std::uint32_t index = 0; index < 2; ++index)
            if (baseline.players[index].object == self)
                ordinal = index;
        Topology current{};
        if (ordinal > 1 || !ReadDeviceIdentity(baseline, ordinal, current) ||
            current.players[ordinal].assignment < 0 ||
            current.players[ordinal].assignment != baseline.players[ordinal].assignment)
        {
            RejectDeviceRoute(ErrorCode::TopologyReadFailed);
            return false;
        }
        g_device_route = {};
        g_device_route.baseline = baseline;
        g_device_route.ordinal = ordinal;
        g_device_route.primary_slot = slot;
        g_device_route.ordinary_backend_authorized = OrdinaryRouteAuthorizesBackend();
        g_device_route.stage = DeviceRouteStage::PrimaryUnassignOpen;
        if (!InvalidateBindings(LifecycleReason::DeviceRebind, true))
            return false;
        g_device_route.generation = g_binding.generation();
        g_device_route.last_manager_pass = g_manager_pass;
        return DeviceEvent(EventKind::DeviceUnassignBegin, slot, 0,
                           baseline.players[ordinal].assignment, true);
    }

    Topology current{};
    if (!DeviceRoutePending() ||
        g_device_route.generation != g_binding.generation() ||
        g_device_route.ordinal > 1 ||
        g_device_route.baseline.players[g_device_route.ordinal].object != self ||
        !ReadDeviceIdentity(g_device_route.baseline, g_device_route.ordinal, current) ||
        current.players[g_device_route.ordinal].assignment != -1)
    {
        RejectDeviceRoute();
        return false;
    }
    if (g_device_route.stage == DeviceRouteStage::MenuDetached && slot == 54 &&
        assignment == -1)
    {
        g_device_route.stage = DeviceRouteStage::PromptConfirmOpen;
        return DeviceEvent(EventKind::DeviceUnassignBegin, slot, 0, -1);
    }
    if (g_device_route.stage == DeviceRouteStage::Detached && slot == 55 && assignment >= 0)
    {
        g_device_route.stage = DeviceRouteStage::ReassignOpen;
        g_device_route.new_assignment = assignment;
        return DeviceEvent(EventKind::DeviceReassignBegin, slot, 0, -1);
    }
    RejectDeviceRoute();
    return false;
}

void EndDeviceAssignment(std::uint32_t slot) noexcept
{
    if (!ObservationEnabled())
        return;
    const auto stage = g_device_route.stage;
    const bool reassign = stage == DeviceRouteStage::ReassignOpen && slot == 55;
    const bool primary = stage == DeviceRouteStage::PrimaryUnassignOpen &&
                         slot == g_device_route.primary_slot;
    const bool confirm = stage == DeviceRouteStage::PromptConfirmOpen && slot == 54;
    Topology current{};
    if ((!reassign && !primary && !confirm) ||
        g_device_route.generation != g_binding.generation() ||
        !ReadDeviceIdentity(g_device_route.baseline, g_device_route.ordinal, current) ||
        current.players[g_device_route.ordinal].assignment !=
            (reassign ? g_device_route.new_assignment : -1))
    {
        RejectDeviceRoute(ErrorCode::TopologyReadFailed);
        return;
    }
    if (!DeviceEvent(reassign ? EventKind::DeviceReassignEnd : EventKind::DeviceUnassignEnd,
                     slot, 1, current.players[g_device_route.ordinal].assignment))
        return;
    g_device_route.stage = reassign ? DeviceRouteStage::AwaitCoherent
        : (primary && slot == 53 ? DeviceRouteStage::MenuDetached : DeviceRouteStage::Detached);
}

void DeviceAssignment(std::uint32_t slot, void* self,
                       std::int32_t assignment, std::uint32_t propagate)
{
    HookScope scope;
    struct DepthScope {
        DepthScope() noexcept { ++g_device_assignment_depth; }
        ~DepthScope() { --g_device_assignment_depth; }
    } depth;
    const bool entered = DeviceV2() && ObservationEnabled() && CheckMainThread() &&
                         BeginDeviceAssignment(slot, self, assignment, propagate);
    // Exactly one unmodified call, including every failed observation path.
    g_assignment(self, assignment, propagate);
    if (entered)
        EndDeviceAssignment(slot);
}

extern "C" void __fastcall CoopDeviceMenuUnassignHook(
    void* self, void*, std::int32_t assignment, std::uint32_t propagate)
{
    DeviceAssignment(53, self, assignment, propagate);
}

extern "C" void __fastcall CoopDeviceDisconnectUnassignHook(
    void* self, void*, std::int32_t assignment, std::uint32_t propagate)
{
    DeviceAssignment(54, self, assignment, propagate);
}

extern "C" void __fastcall CoopDeviceReassignHook(
    void* self, void*, std::int32_t assignment, std::uint32_t propagate)
{
    DeviceAssignment(55, self, assignment, propagate);
}

void PublishBinding(EventKind kind, const Topology& topology,
                    std::uint32_t ordinal) noexcept
{
    auto event = Event(kind);
    FillRelation(event, topology, ordinal);
    Publish(event);
}

bool InvalidateBindings(LifecycleReason reason, bool force) noexcept
{
    if (DeviceV2() && reason != LifecycleReason::DeviceRebind &&
        (DeviceRoutePending() || g_device_route.stage == DeviceRouteStage::Complete))
    {
        RejectDeviceRoute();
        return false;
    }
    RevokeOrdinaryRouteWitness();
    if (g_binding.retired())
        return false;
    const Topology previous = g_binding.bound()
                                  ? g_binding.bound_topology()
                                  : g_binding.candidate_topology();
    const std::size_t previous_count = previous.player_count;
    if (!g_binding.Invalidate(force))
    {
        DisableObservation(ErrorCode::GenerationOverflow);
        return false;
    }
    if (g_shared)
        InterlockedExchange64(&g_shared->owner_generation,
                              static_cast<LONG64>(g_binding.generation()));
    for (std::uint32_t ordinal = 0;
         ordinal < static_cast<std::uint32_t>(std::max<std::size_t>(2, previous_count));
         ++ordinal)
    {
        auto event = Event(EventKind::BindingUnbound);
        if (DeviceV2() && reason == LifecycleReason::DeviceRebind)
        {
            event.phase = static_cast<std::uint32_t>(LifecycleReason::DeviceRebind);
            event.depth = 0;
        }
        event.local_ordinal = ordinal;
        event.endpoint = ordinal + 1;
        event.flags |= ordinal == 0 ? kFlagEndpointA : kFlagEndpointB;
        if (ordinal < previous_count)
            FillRelation(event, previous, ordinal);
        Publish(event);
    }
    return true;
}

bool RetireStableRegistry() noexcept
{
    if (DeviceRoutePending() || (DeviceV2() &&
        g_device_route.stage == DeviceRouteStage::Complete))
        RejectDeviceRoute();
    RevokeOrdinaryRouteWitness();
    const Topology previous = g_binding.bound()
                                  ? g_binding.bound_topology()
                                  : g_binding.candidate_topology();
    if (!g_binding.RetireRun())
    {
        DisableObservation(ErrorCode::GenerationOverflow);
        return false;
    }
    if (g_shared)
    {
        InterlockedExchange64(&g_shared->owner_generation,
                              static_cast<LONG64>(g_binding.generation()));
        InterlockedExchange(&g_shared->stable_registry_retired, 1);
    }
    for (std::uint32_t ordinal = 0; ordinal < 2; ++ordinal)
    {
        auto event = Event(EventKind::BindingRetired);
        event.local_ordinal = ordinal;
        event.endpoint = ordinal + 1;
        event.flags |= (ordinal == 0 ? kFlagEndpointA : kFlagEndpointB) |
                       kFlagStableRegistryRetired;
        if (ordinal < previous.player_count)
            FillRelation(event, previous, ordinal);
        Publish(event);
    }
    return true;
}

void PublishBindingStates() noexcept
{
    if (!g_shared)
        return;
    const auto& states = g_binding.states();
    // The v1 shared block deliberately exposes only evidence counters, not raw
    // bindings. State transitions are represented by ordered events.
    (void)states;
}

void ApplyManagerTopology(TopologyReadResult result,
                          const Topology& topology) noexcept
{
    if (DeviceRoutePending())
    {
        if (g_device_route.generation != g_binding.generation() ||
            g_device_assignment_depth != 0 || g_lifecycle_depth != 0 ||
            g_manager_pass != g_device_route.last_manager_pass + 1)
        {
            RejectDeviceRoute();
            return;
        }
        g_device_route.last_manager_pass = g_manager_pass;
        const auto stage = g_device_route.stage;
        if (stage == DeviceRouteStage::MenuDetached || stage == DeviceRouteStage::Detached)
        {
            Topology detached{};
            if (!ReadDeviceIdentity(g_device_route.baseline, g_device_route.ordinal, detached) ||
                detached.players[g_device_route.ordinal].assignment != -1)
                RejectDeviceRoute(ErrorCode::TopologyReadFailed);
            return;
        }
        if ((stage != DeviceRouteStage::AwaitCoherent && stage != DeviceRouteStage::Candidate) ||
            result != TopologyReadResult::Ok)
        {
            RejectDeviceRoute(ErrorCode::TopologyReadFailed);
            return;
        }
        Topology identity{};
        if (!ReadDeviceIdentity(g_device_route.baseline, g_device_route.ordinal, identity) ||
            identity != topology ||
            topology.players[g_device_route.ordinal].assignment != g_device_route.new_assignment)
        {
            RejectDeviceRoute(ErrorCode::TopologyAmbiguous);
            return;
        }
        if (stage == DeviceRouteStage::AwaitCoherent)
        {
            auto changed = Event(EventKind::RelationChange, 55);
            FillRelation(changed, topology, g_device_route.ordinal);
            changed.phase = static_cast<std::uint32_t>(LifecycleReason::DeviceRebind);
            changed.depth = 0;
            changed.flags |= kFlagRelationChanged;
            if (!Publish(changed))
                return;
            g_device_route.stage = DeviceRouteStage::Candidate;
            g_device_route.candidate_pass = g_manager_pass;
        }
        else if (g_manager_pass != g_device_route.candidate_pass + 1 ||
                 topology != g_binding.candidate_topology())
        {
            RejectDeviceRoute();
            return;
        }
        const auto observation = g_binding.ObserveCompleteManagerPass(g_manager_pass, topology);
        if (observation == BindingMachine::Observation::Candidate &&
            stage == DeviceRouteStage::AwaitCoherent)
        {
            for (std::uint32_t ordinal = 0; ordinal < 2; ++ordinal)
                PublishBinding(EventKind::BindingCandidate, topology, ordinal);
        }
        else if (observation == BindingMachine::Observation::Bound &&
                 stage == DeviceRouteStage::Candidate)
        {
            for (std::uint32_t ordinal = 0; ordinal < 2; ++ordinal)
            {
                PublishBinding(EventKind::BindingBound, topology, ordinal);
                PublishBinding(EventKind::LuaVectorEquivalent, topology, ordinal);
            }
            g_device_route.stage = DeviceRouteStage::Complete;
        }
        else
            RejectDeviceRoute();
        return;
    }
    if (DeviceV2() && g_device_route.stage == DeviceRouteStage::Rejected)
        return;
    if (result != TopologyReadResult::Ok)
    {
        RevokeOrdinaryRouteWitness();
        // The frozen passive plan treats every rejected manager topology as a
        // non-evidentiary pass, never as a runtime failure. A prior stable
        // binding must not survive such a gap; invalidate its generation and
        // publish the ordinary unbind boundary. Without a stable binding,
        // simply discard the incomplete candidate.
        if (g_binding.bound())
            InvalidateBindings(LifecycleReason::ObservedRelationChange, true);
        else
            g_binding.DropCandidate();
        return;
    }
    if (g_binding.bound() && !(topology == g_binding.bound_topology()))
    {
        if (!InvalidateBindings(DeviceV2() ? LifecycleReason::DeviceRebind
                                         : LifecycleReason::ObservedRelationChange, true))
            return;
        auto changed = Event(DeviceV2() ? EventKind::UnclassifiedRelationChange
                                       : EventKind::RelationChange);
        changed.flags |= kFlagRelationChanged | kFlagTopologyCoherent;
        if (!DeviceV2())
            changed.flags |= kFlagAssignmentMatch;
        changed.topology_digest = TopologyDigest(topology);
        Publish(changed);
        if (DeviceV2())
            RejectDeviceRoute();
        return;
    }
    const auto observation =
        g_binding.ObserveCompleteManagerPass(g_manager_pass, topology);
    if (observation == BindingMachine::Observation::Candidate)
    {
        for (std::uint32_t ordinal = 0; ordinal < topology.player_count; ++ordinal)
            PublishBinding(EventKind::BindingCandidate, topology, ordinal);
    }
    else if (observation == BindingMachine::Observation::Bound)
    {
        for (std::uint32_t ordinal = 0; ordinal < topology.player_count; ++ordinal)
        {
            PublishBinding(EventKind::BindingBound, topology, ordinal);
            PublishBinding(EventKind::LuaVectorEquivalent, topology, ordinal);
        }
    }
    PublishBindingStates();
}

void CompleteManagerPass(void* manager, void* return_address) noexcept
{
    if (!g_ordinary_route.transaction_active ||
        g_ordinary_route.manager != manager ||
        g_ordinary_route.manager_pass != g_manager_pass)
        RevokeOrdinaryRouteWitness();
    Topology topology{};
    detail::TopologyDiagnostic diagnostic = detail::TopologyDiagnostic::None;
    TopologyReadResult result = TopologyReadResult::Transient;
    const bool observe = ObservationEnabled();
    if (observe)
    {
#if defined(ISAAC_CS_T0_TESTING)
        if (g_testing_topology_override_enabled)
        {
            topology = g_testing_topology;
            result = static_cast<TopologyReadResult>(g_testing_topology_result);
            diagnostic = g_testing_topology_diagnostic;
        }
        else
        {
            result = TakeCoherentTopology(manager, topology, diagnostic);
        }
#else
        result = TakeCoherentTopology(manager, topology, diagnostic);
#endif
    }
#if defined(ISAAC_CS_T0_TESTING)
    g_testing_last_topology_valid = observe;
    g_testing_last_topology_result = static_cast<std::uint32_t>(result);
    g_testing_last_topology_diagnostic = diagnostic;
#endif

    // The end event remains the manager transaction boundary. Topology is
    // sampled before publication only so the same existing event can carry a
    // privacy-safe rejection reason (or the coherent topology digest). State
    // transitions are deliberately applied only after this event is visible.
    auto event = Event(EventKind::ManagerPassEnd, 45);
    event.original_calls = 1;
    event.flags |= kFlagOriginalCalled;
    (void)return_address;
    if (observe)
    {
        event.topology_digest =
            result == TopologyReadResult::Ok
                ? TopologyDigest(topology)
                : detail::TopologyDiagnosticDigest(diagnostic);
    }
    Publish(event);

    ++g_manager_pass;
    if (observe && ObservationEnabled())
        ApplyManagerTopology(result, topology);
}

LifecycleReason LifecycleReasonFor(loop::EmbeddedLifecycleSignal signal) noexcept
{
    switch (signal)
    {
    case loop::EmbeddedLifecycleSignal::NewRoom:
        return LifecycleReason::NewRoom;
    case loop::EmbeddedLifecycleSignal::AlternateEnvelope:
        return LifecycleReason::AlternateEnvelope;
    case loop::EmbeddedLifecycleSignal::RoomLifecycle:
        return LifecycleReason::RoomLifecycle;
    case loop::EmbeddedLifecycleSignal::RunTeardown:
        return LifecycleReason::RunTeardown;
    case loop::EmbeddedLifecycleSignal::RoomChange:
        return LifecycleReason::RoomChange;
    }
    return LifecycleReason::None;
}

void LifecycleCallback(loop::EmbeddedLifecycleSignal signal, bool entering,
                       void*, void*, void*) noexcept
{
    HookScope scope;
    const bool teardown_exit =
        signal == loop::EmbeddedLifecycleSignal::RunTeardown && !entering &&
        g_binding.retired() && PublicationEnabled();
    if ((!ObservationEnabled() && !teardown_exit) || !CheckMainThread())
        return;
    auto event = Event(entering ? EventKind::LifecycleEnter
                                : EventKind::LifecycleExit);
    event.phase = static_cast<std::uint32_t>(LifecycleReasonFor(signal));
    Publish(event);
    if (signal == loop::EmbeddedLifecycleSignal::RunTeardown)
    {
        if (entering)
            RetireStableRegistry();
        return;
    }
    if (entering)
    {
        if (g_lifecycle_depth == UINT32_MAX)
        {
            DisableObservation(ErrorCode::LifecycleMismatch);
            return;
        }
        if (g_lifecycle_depth++ == 0)
            InvalidateBindings(LifecycleReasonFor(signal), true);
    }
    else
    {
        if (g_lifecycle_depth == 0)
        {
            DisableObservation(ErrorCode::LifecycleMismatch);
            return;
        }
        --g_lifecycle_depth;
    }
}

} // namespace
} // namespace isaac_cs_t0::coop_passive

#if defined(ISAAC_CS_T0_TESTING)
namespace isaac_cs_t0::coop_passive::detail {

bool BeginDeviceRebindHarness(std::uint8_t* main_base,
                              SharedCoopPassiveBlock* shared,
                              void* assignment_original,
                              void* manager_original) noexcept
{
    if (g_shared || !main_base || !shared || !assignment_original || !manager_original)
        return false;
    SecureZeroMemory(shared, sizeof(*shared));
    g_shared = shared;
    g_main_base = main_base;
    shared->magic = kIpcMagic;
    shared->version = kDeviceSchemaVersion;
    shared->struct_size = sizeof(*shared);
    shared->scenario = static_cast<std::uint32_t>(Scenario::DeviceRebind);
    shared->state = static_cast<LONG>(NativeState::Ready);
    shared->publication_enabled = 1;
    g_assignment = reinterpret_cast<AssignmentFn>(assignment_original);
    g_manager = reinterpret_cast<ManagerFn>(manager_original);
    g_binding.Reset();
    ResetOrdinaryRouteTracker();
    g_device_route = {};
    g_manager_pass = 0;
    g_manager_depth = 0;
    g_join_depth = 0;
    g_device_assignment_depth = 0;
    g_lifecycle_depth = 0;
    g_token_secret.fill(0xD2u);
    g_testing_context_override_enabled = true;
    g_testing_context = {};
    g_testing_context.main_thread_id = GetCurrentThreadId();
    g_testing_context.thread_id = GetCurrentThreadId();
    g_testing_context.loop_iteration = 1;
    g_testing_topology_override_enabled = false;
    g_testing_force_topology_exception = false;
    InterlockedExchange(&g_observation_enabled, 1);
    InterlockedExchange(&g_unloading, 0);
    InterlockedExchange(&g_active_hooks, 0);
    InterlockedExchange(&g_failure_reported, 0);
    InterlockedExchange(&g_device_source_invalid, 0);
    InterlockedExchange64(&shared->owner_generation, 1);
    return true;
}

void* DeviceRebindAssignmentWrapperAddress(std::uint32_t slot) noexcept
{
    switch (slot)
    {
    case 53: return reinterpret_cast<void*>(&CoopDeviceMenuUnassignHook);
    case 54: return reinterpret_cast<void*>(&CoopDeviceDisconnectUnassignHook);
    case 55: return reinterpret_cast<void*>(&CoopDeviceReassignHook);
    default: return nullptr;
    }
}

bool SetDeviceRebindHarnessOrdinaryOriginals(void* spawn_original,
                                            void* hud_original) noexcept
{
    if (!g_shared || !spawn_original || !hud_original)
        return false;
    g_spawn = reinterpret_cast<SpawnFn>(spawn_original);
    g_hud = reinterpret_cast<HudFn>(hud_original);
    return true;
}

void* DeviceRebindOrdinaryWrapperAddress(std::uint32_t slot) noexcept
{
    switch (slot)
    {
    case 46: return reinterpret_cast<void*>(&CoopOrdinarySpawnHook);
    case 49: return reinterpret_cast<void*>(&CoopOrdinaryAssignmentHook);
    case 51: return reinterpret_cast<void*>(&CoopJoinHudRebuildHook);
    default: return nullptr;
    }
}

void InvokeDeviceRebindAssignment(std::uint32_t slot, void* self,
                                  std::int32_t controller_id,
                                  std::uint32_t propagate) noexcept
{
    const auto wrapper = reinterpret_cast<AssignmentFn>(
        DeviceRebindAssignmentWrapperAddress(slot));
    if (wrapper)
        wrapper(self, controller_id, propagate);
}

std::uint32_t InvokeDeviceRebindManager(void* manager) noexcept
{
    ++g_testing_context.loop_iteration;
    return CoopManagerOwnerHook(manager, nullptr);
}

void SetDeviceRebindHarnessFault(DeviceRebindHarnessFault fault) noexcept
{
    if (!g_shared)
        return;
    switch (fault)
    {
    case DeviceRebindHarnessFault::None:
        g_testing_context.thread_id = GetCurrentThreadId();
        g_testing_context.main_thread_id = GetCurrentThreadId();
        g_testing_force_topology_exception = false;
        if (g_shared->state == static_cast<LONG>(NativeState::Ready))
            InterlockedExchange(&g_observation_enabled, 1);
        break;
    case DeviceRebindHarnessFault::ObservationDisabled:
        InterlockedExchange(&g_observation_enabled, 0);
        break;
    case DeviceRebindHarnessFault::WrongThread:
        g_testing_context.thread_id = g_testing_context.main_thread_id + 1;
        break;
    case DeviceRebindHarnessFault::ReadException:
        g_testing_force_topology_exception = true;
        break;
    case DeviceRebindHarnessFault::RingFull:
        InterlockedExchange64(&g_shared->controller_sequence, 0);
        InterlockedExchange64(&g_shared->next_sequence, static_cast<LONG64>(kEventCapacity));
        break;
    case DeviceRebindHarnessFault::PossessorStop:
        DisableObservation(ErrorCode::PossessorPath);
        break;
    case DeviceRebindHarnessFault::Unsafe:
        InterlockedExchange(&g_observation_enabled, 0);
        SetState(ErrorCode::UnsafeShutdown, NativeState::Unsafe);
        break;
    case DeviceRebindHarnessFault::LifecycleEnter:
        LifecycleCallback(loop::EmbeddedLifecycleSignal::NewRoom, true, nullptr, nullptr, nullptr);
        break;
    case DeviceRebindHarnessFault::LifecycleExit:
        LifecycleCallback(loop::EmbeddedLifecycleSignal::NewRoom, false, nullptr, nullptr, nullptr);
        break;
    case DeviceRebindHarnessFault::MenuTeardown:
        LifecycleCallback(loop::EmbeddedLifecycleSignal::RunTeardown, true, nullptr, nullptr, nullptr);
        break;
    case DeviceRebindHarnessFault::AdvanceGeneration:
        g_binding.Invalidate(true);
        InterlockedExchange64(&g_shared->owner_generation,
                              static_cast<LONG64>(g_binding.generation()));
        break;
    case DeviceRebindHarnessFault::SkipManagerPass:
        ++g_manager_pass;
        break;
    }
}

bool DrainDeviceRebindHarness() noexcept
{
    InterlockedExchange(&g_observation_enabled, 0);
    InterlockedExchange(&g_unloading, 1);
    if (!WaitForCallbacksDrained())
    {
        SetState(ErrorCode::DrainTimeout, NativeState::Unsafe);
        return false;
    }
    return true;
}

DWORD InvokeDeviceRebindShutdownHarness(
    const DeviceRebindShutdownDependencies& dependencies) noexcept
{
    if (!g_testing_context_override_enabled || !DeviceV2() ||
        g_testing_shutdown_dependencies)
        return kUnsafeResult;
    g_testing_shutdown_dependencies = &dependencies;
    const DWORD result = Shutdown();
    g_testing_shutdown_dependencies = nullptr;
    return result;
}

bool SetDeviceRebindHarnessGeneration(std::uint64_t generation) noexcept
{
    if (!g_testing_context_override_enabled || !DeviceV2() || generation == 0 ||
        g_manager_pass != 0 || InterlockedCompareExchange(&g_unloading, 0, 0) != 0)
        return false;
    g_binding.SetGenerationForTesting(generation);
    InterlockedExchange64(&g_shared->owner_generation,
                          static_cast<LONG64>(generation));
    return true;
}

bool PublishDeviceRebindHarnessInitialization() noexcept
{
    if (!g_testing_context_override_enabled || !DeviceV2() ||
        InterlockedCompareExchange64(&g_shared->next_sequence, 0, 0) != 0)
        return false;
    return Publish(Event(EventKind::NativeInitialized)) &&
           Publish(Event(EventKind::HooksInstalled));
}

namespace {

DeviceRebindTopologySnapshot SnapshotDeviceTopology(const Topology& topology) noexcept
{
    DeviceRebindTopologySnapshot snapshot{};
    snapshot.roots = {{reinterpret_cast<std::uintptr_t>(topology.game_root),
                       reinterpret_cast<std::uintptr_t>(topology.player_manager),
                       reinterpret_cast<std::uintptr_t>(topology.online_manager),
                       reinterpret_cast<std::uintptr_t>(topology.network_backend)}};
    snapshot.online_frame = topology.online_frame;
    snapshot.device_count = topology.device_count;
    for (std::size_t index = 0; index < topology.devices.size(); ++index)
    {
        const auto& device = topology.devices[index];
        snapshot.device_objects[index] = reinterpret_cast<std::uintptr_t>(device.object);
        snapshot.device_reference_owners[index] =
            reinterpret_cast<std::uintptr_t>(device.reference_owner);
        snapshot.device_assignments[index] = device.assignment;
        snapshot.device_kinds[index] = static_cast<std::uint32_t>(device.kind);
    }
    snapshot.player_count = topology.player_count;
    for (std::size_t index = 0; index < topology.players.size(); ++index)
    {
        const auto& player = topology.players[index];
        snapshot.player_objects[index] = reinterpret_cast<std::uintptr_t>(player.object);
        snapshot.player_devices[index] = reinterpret_cast<std::uintptr_t>(player.device);
        snapshot.player_assignments[index] = player.assignment;
        snapshot.player_ordinals[index] = player.ordinal;
        snapshot.player_device_kinds[index] = static_cast<std::uint32_t>(player.device_kind);
    }
    for (std::size_t index = 0; index < topology.hud.size(); ++index)
        snapshot.hud[index] = reinterpret_cast<std::uintptr_t>(topology.hud[index]);
    return snapshot;
}

} // namespace

DeviceRebindPrivateStateSnapshot SnapshotDeviceRebindPrivateState() noexcept
{
    DeviceRebindPrivateStateSnapshot snapshot{};
    snapshot.generation = g_binding.generation();
    snapshot.candidate_generation = g_binding.candidate_generation();
    snapshot.candidate_pass = g_binding.candidate_pass();
    snapshot.candidate_valid = g_binding.candidate_valid();
    snapshot.bound_valid = g_binding.bound();
    snapshot.retired = g_binding.retired();
    for (std::size_t index = 0; index < g_binding.states().size(); ++index)
        snapshot.binding_states[index] = static_cast<std::uint32_t>(g_binding.states()[index]);
    snapshot.candidate = SnapshotDeviceTopology(g_binding.candidate_topology());
    snapshot.bound = SnapshotDeviceTopology(g_binding.bound_topology());
    snapshot.device_baseline = SnapshotDeviceTopology(g_device_route.baseline);
    snapshot.device_stage = static_cast<std::uint32_t>(g_device_route.stage);
    snapshot.device_ordinal = g_device_route.ordinal;
    snapshot.device_primary_slot = g_device_route.primary_slot;
    snapshot.device_generation = g_device_route.generation;
    snapshot.device_last_manager_pass = g_device_route.last_manager_pass;
    snapshot.device_candidate_pass = g_device_route.candidate_pass;
    snapshot.device_new_assignment = g_device_route.new_assignment;
    snapshot.device_ordinary_backend_authorized = g_device_route.ordinary_backend_authorized;
    snapshot.ordinary_transaction_active = g_ordinary_route.transaction_active;
    snapshot.ordinary_manager = reinterpret_cast<std::uintptr_t>(g_ordinary_route.manager);
    snapshot.ordinary_manager_pass = g_ordinary_route.manager_pass;
    snapshot.ordinary_stage = static_cast<std::uint32_t>(g_ordinary_route.stage);
    snapshot.ordinary_route_generation = g_ordinary_route.route_generation;
    snapshot.ordinary_authorized_generation = g_ordinary_route.authorized_generation;
    snapshot.token_secret = g_token_secret;
    return snapshot;
}

void EndDeviceRebindHarness() noexcept
{
    // The caller owns the mapping and reads its raw events after this cleanup.
    g_shared = nullptr;
    g_main_base = nullptr;
    g_assignment = nullptr;
    g_manager = nullptr;
    g_spawn = nullptr;
    g_hud = nullptr;
    g_binding.Reset();
    ResetOrdinaryRouteTracker();
    g_device_route = {};
    g_testing_context_override_enabled = false;
    g_testing_topology_override_enabled = false;
    g_testing_force_topology_exception = false;
    g_testing_shutdown_dependencies = nullptr;
    g_token_secret.fill(0);
    InterlockedExchange(&g_observation_enabled, 0);
    InterlockedExchange(&g_unloading, 0);
    InterlockedExchange(&g_device_source_invalid, 0);
}

namespace {

class BackendProbeFixture final {
public:
    BackendProbeFixture() noexcept
    {
        main_image_ = Allocate(kExpectedImageSize);
        root_ = Allocate(kRunActiveOffset + 0x100u);
        online_manager_ = Allocate(kOnlineFrameOffset + 0x10u);
        backend_ = Allocate(0x40u);
        for (auto& device : devices_)
            device = Allocate(0x40u);
        for (auto& player : players_)
            player = Allocate(kPlayerTwinOffset + 0x10u);
        valid_ = main_image_ && root_ && online_manager_ && backend_ &&
                 std::all_of(devices_.begin(), devices_.end(),
                             [](const auto* value) { return value != nullptr; }) &&
                 std::all_of(players_.begin(), players_.end(),
                             [](const auto* value) { return value != nullptr; });
        if (valid_)
            Reset();
    }

    ~BackendProbeFixture()
    {
        for (auto* player : players_)
            Release(player);
        for (auto* device : devices_)
            Release(device);
        Release(backend_);
        Release(online_manager_);
        Release(root_);
        Release(main_image_);
    }

    BackendProbeFixture(const BackendProbeFixture&) = delete;
    BackendProbeFixture& operator=(const BackendProbeFixture&) = delete;

    bool valid() const noexcept { return valid_; }
    std::uint8_t* main_image() const noexcept { return main_image_; }
    void* manager() const noexcept { return root_ + kPlayerManagerOffset; }

    void Reset() noexcept
    {
        SecureZeroMemory(main_image_, kExpectedImageSize);
        SecureZeroMemory(root_, kRunActiveOffset + 0x100u);
        SecureZeroMemory(online_manager_, kOnlineFrameOffset + 0x10u);
        SecureZeroMemory(backend_, 0x40u);
        for (auto* device : devices_)
            SecureZeroMemory(device, 0x40u);
        for (auto* player : players_)
            SecureZeroMemory(player, kPlayerTwinOffset + 0x10u);
        device_entries_.fill(nullptr);
        player_entries_.fill(nullptr);
        roster_entries_.fill(nullptr);

        *reinterpret_cast<void**>(main_image_ + kGameRootSlotRva) = root_;
        *reinterpret_cast<void**>(main_image_ + kOnlineManagerSlotRva) =
            online_manager_;
        *reinterpret_cast<void**>(main_image_ + kNetworkBackendSlotRva) = nullptr;
        *reinterpret_cast<std::uint16_t*>(root_ + kRunActiveOffset) = 1;

        for (std::size_t index = 0; index < devices_.size(); ++index)
        {
            *reinterpret_cast<void**>(devices_[index]) =
                main_image_ + (index == 0 ? kKeyboardVtableRva
                                          : kGamepadVtableRva);
            *reinterpret_cast<std::int32_t*>(devices_[index] +
                                             kDeviceAssignmentOffset) =
                static_cast<std::int32_t>(index);
            device_entries_[index * 2] = devices_[index];
            device_entries_[index * 2 + 1] = devices_[index];

            *reinterpret_cast<void**>(players_[index]) =
                main_image_ + kPlayerVtableRva;
            *reinterpret_cast<std::int32_t*>(players_[index] +
                                             kPlayerAssignmentOffset) =
                static_cast<std::int32_t>(index);
            *reinterpret_cast<std::uint32_t*>(players_[index] +
                                              kPlayerOrdinalOffset) =
                static_cast<std::uint32_t>(index);
            player_entries_[index] = players_[index];
            *reinterpret_cast<void**>(root_ + kHudOffset +
                                      index * kHudStride) = players_[index];
        }
        reinterpret_cast<void**>(main_image_ + kPlayerVtableRva)[-1] =
            main_image_ + kPlayerColRva;

        SetHeader(main_image_ + kInputManagerRva + 8,
                  reinterpret_cast<std::uint8_t*>(device_entries_.data()),
                  reinterpret_cast<std::uint8_t*>(device_entries_.data() +
                                                  device_entries_.size()),
                  reinterpret_cast<std::uint8_t*>(device_entries_.data() +
                                                  device_entries_.size()));
        SetHeader(root_ + kPlayerManagerOffset,
                  reinterpret_cast<std::uint8_t*>(player_entries_.data()),
                  reinterpret_cast<std::uint8_t*>(player_entries_.data() +
                                                  player_entries_.size()),
                  reinterpret_cast<std::uint8_t*>(player_entries_.data() +
                                                  player_entries_.size()));
        SetHeader(online_manager_ + kOnlineRosterOffset, nullptr, nullptr,
                  nullptr);
        *reinterpret_cast<std::uint32_t*>(online_manager_ +
                                          kOnlineFrameOffset) = 0;
    }

    void SetKnownBackend() noexcept
    {
        *reinterpret_cast<void**>(backend_) =
            main_image_ + kSteamBackendVtableRva;
        *reinterpret_cast<void**>(main_image_ + kNetworkBackendSlotRva) =
            backend_;
    }

    void SetMisalignedBackend() noexcept
    {
        *reinterpret_cast<void**>(main_image_ + kNetworkBackendSlotRva) =
            backend_ + 1;
    }

    void SetUnreadableBackend() noexcept
    {
        *reinterpret_cast<void**>(main_image_ + kNetworkBackendSlotRva) =
            reinterpret_cast<void*>(4u);
    }

    void SetUnknownBackend() noexcept
    {
        *reinterpret_cast<void**>(backend_) = main_image_ + kPlayerVtableRva;
        *reinterpret_cast<void**>(main_image_ + kNetworkBackendSlotRva) =
            backend_;
    }

    void SetRosterBeginOnly() noexcept
    {
        auto* const value =
            reinterpret_cast<std::uint8_t*>(roster_entries_.data());
        SetHeader(online_manager_ + kOnlineRosterOffset, value, nullptr,
                  nullptr);
    }

    void SetRosterEndOnly() noexcept
    {
        auto* const value =
            reinterpret_cast<std::uint8_t*>(roster_entries_.data());
        SetHeader(online_manager_ + kOnlineRosterOffset, nullptr, value,
                  nullptr);
    }

    void SetRosterCapacityOnly() noexcept
    {
        auto* const value =
            reinterpret_cast<std::uint8_t*>(roster_entries_.data());
        SetHeader(online_manager_ + kOnlineRosterOffset, nullptr, nullptr,
                  value);
    }

    void SetAllocatedEmptyRoster() noexcept
    {
        auto* const value =
            reinterpret_cast<std::uint8_t*>(roster_entries_.data());
        SetHeader(online_manager_ + kOnlineRosterOffset, value, value, value);
    }

    void SetNonemptyRoster() noexcept
    {
        roster_entries_[0] = players_[0];
        auto* const begin =
            reinterpret_cast<std::uint8_t*>(roster_entries_.data());
        auto* const end = reinterpret_cast<std::uint8_t*>(
            roster_entries_.data() + 1);
        SetHeader(online_manager_ + kOnlineRosterOffset, begin, end, end);
    }

    void SetOnlineFrame() noexcept
    {
        *reinterpret_cast<std::uint32_t*>(online_manager_ +
                                          kOnlineFrameOffset) = 1;
    }

    void SetAssignmentPending() noexcept
    {
        *reinterpret_cast<std::int32_t*>(players_[1] +
                                         kPlayerAssignmentOffset) = -1;
    }

    void SetAssignmentValid() noexcept
    {
        *reinterpret_cast<std::int32_t*>(players_[1] +
                                         kPlayerAssignmentOffset) = 1;
    }

    void SetOrdinalMismatch() noexcept
    {
        *reinterpret_cast<std::uint32_t*>(players_[1] +
                                          kPlayerOrdinalOffset) = 0;
    }

    void SetHudMismatch() noexcept
    {
        *reinterpret_cast<void**>(root_ + kHudOffset + kHudStride) = nullptr;
    }

private:
    static std::uint8_t* Allocate(std::size_t size) noexcept
    {
        return static_cast<std::uint8_t*>(VirtualAlloc(
            nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    }

    static void Release(void* value) noexcept
    {
        if (value)
            VirtualFree(value, 0, MEM_RELEASE);
    }

    static void SetHeader(void* address, std::uint8_t* begin,
                          std::uint8_t* end,
                          std::uint8_t* capacity_end) noexcept
    {
        auto** const slots = reinterpret_cast<std::uint8_t**>(address);
        slots[0] = begin;
        slots[1] = end;
        slots[2] = capacity_end;
    }

    bool valid_ = false;
    std::uint8_t* main_image_ = nullptr;
    std::uint8_t* root_ = nullptr;
    std::uint8_t* online_manager_ = nullptr;
    std::uint8_t* backend_ = nullptr;
    std::array<std::uint8_t*, 2> devices_{};
    std::array<std::uint8_t*, 2> players_{};
    std::array<void*, 4> device_entries_{};
    std::array<void*, 2> player_entries_{};
    std::array<void*, 2> roster_entries_{};
};

} // namespace

bool RunTopologyProbe(std::uint8_t* main_base, void* expected_manager,
                      TopologyProbeResult& result) noexcept
{
    result = {};
    if (!main_base)
        return false;
    auto* const saved_base = g_main_base;
    g_main_base = main_base;
    Topology topology{};
    TopologyDiagnostic diagnostic = TopologyDiagnostic::None;
    const auto classification =
        ReadTopology(expected_manager, topology, diagnostic);
    g_main_base = saved_base;
    result.classification = static_cast<std::uint32_t>(classification);
    result.diagnostic = diagnostic;
    result.player_count = topology.player_count;
    result.device_count = topology.device_count;
    return true;
}

bool RunBackendRouteWitnessProbe(BackendRouteWitnessProbe& probe) noexcept
{
    probe = {};
    if (g_shared)
        return false;

    BackendProbeFixture fixture{};
    if (!fixture.valid())
        return false;
    auto* const block = static_cast<SharedCoopPassiveBlock*>(VirtualAlloc(
        nullptr, sizeof(SharedCoopPassiveBlock), MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE));
    if (!block)
        return false;

    auto* const saved_shared = g_shared;
    auto* const saved_main_base = g_main_base;
    const auto saved_manager = g_manager;
    const auto saved_spawn = g_spawn;
    const auto saved_exit = g_exit;
    const auto saved_assignment = g_assignment;
    const auto saved_hud = g_hud;
    const auto saved_binding = g_binding;
    const auto saved_token_secret = g_token_secret;
    const auto saved_manager_pass = g_manager_pass;
    const auto saved_lifecycle_depth = g_lifecycle_depth;
    const auto saved_ordinary_route = g_ordinary_route;
    const auto saved_manager_depth = g_manager_depth;
    const auto saved_join_depth = g_join_depth;
    const auto saved_backend_route_mode = g_backend_probe_route_mode;
    const auto saved_backend_counts = g_backend_probe_counts;
    const bool saved_backend_possessor = g_backend_probe_possessor_call;
    const LONG saved_observation =
        InterlockedCompareExchange(&g_observation_enabled, 0, 0);
    const LONG saved_unloading =
        InterlockedCompareExchange(&g_unloading, 0, 0);
    const LONG saved_active_hooks =
        InterlockedCompareExchange(&g_active_hooks, 0, 0);
    const LONG saved_failure =
        InterlockedCompareExchange(&g_failure_reported, 0, 0);
    const bool saved_context_override = g_testing_context_override_enabled;
    const auto saved_context = g_testing_context;
    const bool saved_topology_override = g_testing_topology_override_enabled;
    const auto saved_topology_result = g_testing_topology_result;
    const auto saved_topology = g_testing_topology;
    const auto saved_topology_diagnostic = g_testing_topology_diagnostic;
    const bool saved_force_exception = g_testing_force_topology_exception;
    const bool saved_last_topology_valid = g_testing_last_topology_valid;
    const auto saved_last_topology_result = g_testing_last_topology_result;
    const auto saved_last_topology_diagnostic =
        g_testing_last_topology_diagnostic;

    const auto reset_runtime = [&]() noexcept {
        SecureZeroMemory(block, sizeof(*block));
        block->publication_enabled = 1;
        block->state = static_cast<LONG>(NativeState::Ready);
        g_shared = block;
        g_main_base = fixture.main_image();
        g_manager = reinterpret_cast<ManagerFn>(&BackendProbeManagerOriginal);
        g_spawn = reinterpret_cast<SpawnFn>(&BackendProbeSpawnOriginal);
        g_exit = reinterpret_cast<ExitFn>(&BackendProbeExitOriginal);
        g_assignment =
            reinterpret_cast<AssignmentFn>(&BackendProbeAssignmentOriginal);
        g_hud = reinterpret_cast<HudFn>(&BackendProbeHudOriginal);
        g_binding.Reset();
        g_manager_pass = 0;
        g_lifecycle_depth = 0;
        ResetOrdinaryRouteTracker();
        g_manager_depth = 0;
        g_join_depth = 0;
        g_backend_probe_route_mode = BackendProbeRouteMode::None;
        g_backend_probe_counts = {};
        g_backend_probe_possessor_call = false;
        g_token_secret.fill(0x6Bu);
        InterlockedExchange(&g_observation_enabled, 1);
        InterlockedExchange(&g_unloading, 0);
        InterlockedExchange(&g_active_hooks, 0);
        InterlockedExchange(&g_failure_reported, 0);
        g_testing_context_override_enabled = true;
        g_testing_context = {};
        g_testing_context.loop_iteration = 23;
        g_testing_context.thread_id = GetCurrentThreadId();
        g_testing_context.main_thread_id = GetCurrentThreadId();
        g_testing_context.phase = loop::Phase::GameplayUpdate;
        g_testing_context.phase_depth = 1;
        g_testing_topology_override_enabled = false;
        g_testing_topology_result =
            static_cast<std::uint32_t>(TopologyReadResult::Transient);
        g_testing_topology = {};
        g_testing_topology_diagnostic = TopologyDiagnostic::None;
        g_testing_force_topology_exception = false;
        g_testing_last_topology_valid = false;
        g_testing_last_topology_result = 0;
        g_testing_last_topology_diagnostic = TopologyDiagnostic::None;
    };

    const auto fill_case = [&](BackendScreenCaseProbe& value,
                               std::uint32_t observed_return) noexcept {
        const bool route_authorized = OrdinaryRouteAuthorizesBackend();
        if (g_testing_last_topology_valid)
        {
            value.classification = g_testing_last_topology_result;
            value.diagnostic = g_testing_last_topology_diagnostic;
        }
        else
        {
            Topology topology{};
            TopologyDiagnostic diagnostic = TopologyDiagnostic::None;
            const auto classification =
                ReadTopology(fixture.manager(), topology, diagnostic);
            value.classification =
                static_cast<std::uint32_t>(classification);
            value.diagnostic = diagnostic;
        }
        value.manager_end_classification = value.classification;
        value.manager_end_diagnostic = value.diagnostic;
        value.manager_original_calls = g_backend_probe_counts.manager;
        value.expected_return = kBackendProbeManagerReturn;
        value.observed_return = observed_return;
        value.spawn_original_calls = g_backend_probe_counts.spawn;
        value.possessor_spawn_original_calls =
            g_backend_probe_counts.possessor_spawn;
        value.exit_original_calls = g_backend_probe_counts.exit;
        value.assignment_original_calls = g_backend_probe_counts.assignment;
        value.hud_original_calls = g_backend_probe_counts.hud;
        value.generation_before = g_backend_probe_counts.generation_before;
        value.generation_after = g_binding.generation();
        value.route_authorized = route_authorized;
        const LONG64 published_count =
            InterlockedCompareExchange64(&block->next_sequence, 0, 0);
        const auto event_count = static_cast<std::size_t>(
            published_count > 0 ? published_count : 0);
        for (std::size_t index = 0;
             index < event_count && index < kEventCapacity; ++index)
        {
            const auto kind =
                static_cast<EventKind>(block->events[index].kind);
            if (kind == EventKind::BindingCandidate)
                ++value.binding_candidate_count;
            else if (kind == EventKind::BindingBound)
                ++value.binding_bound_count;
            else if (kind == EventKind::Failure)
                ++value.terminal_failure_count;
        }
        const auto& states = g_binding.states();
        value.remains_unbound = !g_binding.bound() && !g_binding.retired() &&
                                states[0] == BindingState::Unbound &&
                                states[1] == BindingState::Unbound;
    };

    const auto run_case = [&](BackendScreenCaseProbe& value,
                              BackendProbeRouteMode route_mode,
                              bool second_pass) noexcept {
        reset_runtime();
        g_backend_probe_route_mode = route_mode;
        g_backend_probe_counts.generation_before = g_binding.generation();
        std::uint32_t observed =
            CoopManagerOwnerHook(fixture.manager(), nullptr);
        if (second_pass)
        {
            g_backend_probe_route_mode = BackendProbeRouteMode::None;
            observed = CoopManagerOwnerHook(fixture.manager(), nullptr);
        }
        fill_case(value, observed);
    };

    fixture.Reset();
    run_case(probe.null_backend_without_route, BackendProbeRouteMode::None,
             true);

    fixture.Reset();
    fixture.SetKnownBackend();
    run_case(probe.known_backend_without_route, BackendProbeRouteMode::None,
             false);

    fixture.Reset();
    fixture.SetKnownBackend();
    run_case(probe.known_backend_exact_route, BackendProbeRouteMode::Exact,
             true);
    probe.exact_generation_before =
        probe.known_backend_exact_route.generation_before;
    probe.exact_generation_after_spawn =
        g_backend_probe_counts.generation_after_spawn;
    probe.exact_generation_after_assignment =
        g_backend_probe_counts.generation_after_assignment;
    const LONG64 exact_published =
        InterlockedCompareExchange64(&block->next_sequence, 0, 0);
    probe.exact_event_count = static_cast<std::uint32_t>(std::min<LONG64>(
        exact_published > 0 ? exact_published : 0,
        static_cast<LONG64>(probe.exact_event_kinds.size())));
    for (std::size_t index = 0; index < probe.exact_event_count; ++index)
    {
        probe.exact_event_kinds[index] = block->events[index].kind;
        probe.exact_event_local_ordinals[index] =
            block->events[index].local_ordinal;
        probe.exact_event_endpoints[index] = block->events[index].endpoint;
        probe.exact_event_generations[index] =
            block->events[index].owner_generation;
    }

    fixture.Reset();
    fixture.SetMisalignedBackend();
    run_case(probe.misaligned_backend, BackendProbeRouteMode::Exact, false);

    fixture.Reset();
    fixture.SetUnreadableBackend();
    run_case(probe.unreadable_backend, BackendProbeRouteMode::Exact, false);

    fixture.Reset();
    fixture.SetUnknownBackend();
    run_case(probe.unknown_backend_vtable, BackendProbeRouteMode::Exact, false);

    fixture.Reset();
    fixture.SetKnownBackend();
    fixture.SetRosterBeginOnly();
    run_case(probe.roster_begin_only, BackendProbeRouteMode::Exact, false);

    fixture.Reset();
    fixture.SetKnownBackend();
    fixture.SetRosterEndOnly();
    run_case(probe.roster_end_only, BackendProbeRouteMode::Exact, false);

    fixture.Reset();
    fixture.SetKnownBackend();
    fixture.SetRosterCapacityOnly();
    run_case(probe.roster_capacity_only, BackendProbeRouteMode::Exact, false);

    fixture.Reset();
    fixture.SetKnownBackend();
    fixture.SetAllocatedEmptyRoster();
    run_case(probe.allocated_empty_roster, BackendProbeRouteMode::Exact, false);

    fixture.Reset();
    fixture.SetKnownBackend();
    fixture.SetNonemptyRoster();
    run_case(probe.nonempty_roster, BackendProbeRouteMode::Exact, false);

    fixture.Reset();
    fixture.SetKnownBackend();
    fixture.SetOnlineFrame();
    run_case(probe.online_frame_present, BackendProbeRouteMode::Exact, false);

    fixture.Reset();
    fixture.SetKnownBackend();
    run_case(probe.partial_route, BackendProbeRouteMode::Partial, false);

    fixture.Reset();
    fixture.SetKnownBackend();
    run_case(probe.out_of_order_route, BackendProbeRouteMode::OutOfOrder,
             false);

    fixture.Reset();
    fixture.SetKnownBackend();
    run_case(probe.stale_generation,
             BackendProbeRouteMode::StaleGeneration, false);

    fixture.Reset();
    fixture.SetKnownBackend();
    run_case(probe.cross_generation_witness,
             BackendProbeRouteMode::CrossGeneration, false);

    fixture.Reset();
    fixture.SetKnownBackend();
    run_case(probe.possessor_route, BackendProbeRouteMode::Possessor, false);

    fixture.Reset();
    fixture.SetKnownBackend();
    run_case(probe.exit_route, BackendProbeRouteMode::Exit, false);

    fixture.Reset();
    fixture.SetKnownBackend();
    run_case(probe.unrelated_assignment,
             BackendProbeRouteMode::AssignmentOnly, false);

    fixture.Reset();
    fixture.SetKnownBackend();
    fixture.SetAssignmentPending();
    run_case(probe.assignment_pending, BackendProbeRouteMode::Exact, false);

    fixture.Reset();
    fixture.SetKnownBackend();
    fixture.SetOrdinalMismatch();
    run_case(probe.player_ordinal_mismatch,
             BackendProbeRouteMode::Exact, false);

    fixture.Reset();
    fixture.SetKnownBackend();
    fixture.SetHudMismatch();
    run_case(probe.hud_mismatch, BackendProbeRouteMode::Exact, false);

    fixture.Reset();
    fixture.SetKnownBackend();
    reset_runtime();
    g_backend_probe_route_mode = BackendProbeRouteMode::Exact;
    g_backend_probe_counts.generation_before = g_binding.generation();
    std::uint32_t intervening_return =
        CoopManagerOwnerHook(fixture.manager(), nullptr);
    fixture.SetAssignmentPending();
    g_backend_probe_route_mode = BackendProbeRouteMode::None;
    intervening_return = CoopManagerOwnerHook(fixture.manager(), nullptr);
    const auto intervening_rejected_classification =
        g_testing_last_topology_result;
    const auto intervening_rejected_diagnostic =
        g_testing_last_topology_diagnostic;
    fixture.SetAssignmentValid();
    intervening_return = CoopManagerOwnerHook(fixture.manager(), nullptr);
    fill_case(probe.intervening_rejected_pass, intervening_return);
    probe.intervening_rejected_pass.manager_end_classification =
        intervening_rejected_classification;
    probe.intervening_rejected_pass.manager_end_diagnostic =
        intervening_rejected_diagnostic;

    g_shared = saved_shared;
    g_main_base = saved_main_base;
    g_manager = saved_manager;
    g_spawn = saved_spawn;
    g_exit = saved_exit;
    g_assignment = saved_assignment;
    g_hud = saved_hud;
    g_binding = saved_binding;
    g_token_secret = saved_token_secret;
    g_manager_pass = saved_manager_pass;
    g_lifecycle_depth = saved_lifecycle_depth;
    g_ordinary_route = saved_ordinary_route;
    g_manager_depth = saved_manager_depth;
    g_join_depth = saved_join_depth;
    g_backend_probe_route_mode = saved_backend_route_mode;
    g_backend_probe_counts = saved_backend_counts;
    g_backend_probe_possessor_call = saved_backend_possessor;
    InterlockedExchange(&g_observation_enabled, saved_observation);
    InterlockedExchange(&g_unloading, saved_unloading);
    InterlockedExchange(&g_active_hooks, saved_active_hooks);
    InterlockedExchange(&g_failure_reported, saved_failure);
    g_testing_context_override_enabled = saved_context_override;
    g_testing_context = saved_context;
    g_testing_topology_override_enabled = saved_topology_override;
    g_testing_topology_result = saved_topology_result;
    g_testing_topology = saved_topology;
    g_testing_topology_diagnostic = saved_topology_diagnostic;
    g_testing_force_topology_exception = saved_force_exception;
    g_testing_last_topology_valid = saved_last_topology_valid;
    g_testing_last_topology_result = saved_last_topology_result;
    g_testing_last_topology_diagnostic = saved_last_topology_diagnostic;
    VirtualFree(block, 0, MEM_RELEASE);
    return true;
}

bool RunManagerDiagnosticProbe(TopologyDiagnostic rejected_diagnostic,
                               ManagerDiagnosticProbe& probe) noexcept
{
    probe = {};
    const auto diagnostic_code =
        static_cast<std::uint32_t>(rejected_diagnostic);
    if (g_shared || diagnostic_code == 0 ||
        diagnostic_code > static_cast<std::uint32_t>(
                              TopologyDiagnostic::OfflineOnlineFrameNonzero))
        return false;

    auto* const block = static_cast<SharedCoopPassiveBlock*>(VirtualAlloc(
        nullptr, sizeof(SharedCoopPassiveBlock), MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE));
    if (!block)
        return false;

    auto* const saved_shared = g_shared;
    auto* const saved_main_base = g_main_base;
    const auto saved_manager = g_manager;
    const auto saved_binding = g_binding;
    const auto saved_token_secret = g_token_secret;
    const auto saved_manager_pass = g_manager_pass;
    const auto saved_ordinary_route = g_ordinary_route;
    const auto saved_manager_depth = g_manager_depth;
    const LONG saved_observation =
        InterlockedCompareExchange(&g_observation_enabled, 0, 0);
    const LONG saved_unloading =
        InterlockedCompareExchange(&g_unloading, 0, 0);
    const LONG saved_failure =
        InterlockedCompareExchange(&g_failure_reported, 0, 0);
    const bool saved_context_override = g_testing_context_override_enabled;
    const auto saved_context = g_testing_context;
    const bool saved_topology_override = g_testing_topology_override_enabled;
    const auto saved_topology_result = g_testing_topology_result;
    const auto saved_topology = g_testing_topology;
    const auto saved_topology_diagnostic = g_testing_topology_diagnostic;
    const bool saved_force_exception = g_testing_force_topology_exception;
    const bool saved_last_topology_valid = g_testing_last_topology_valid;
    const auto saved_last_topology_result = g_testing_last_topology_result;
    const auto saved_last_topology_diagnostic =
        g_testing_last_topology_diagnostic;
    const auto saved_diagnostic_calls = g_manager_diagnostic_original_calls;
    const auto saved_diagnostic_return = g_manager_diagnostic_original_return;

    int manager_object = 0;
    int player_object = 0;
    int device_object = 0;
    Topology normal{};
    normal.game_root = &manager_object;
    normal.player_manager = &manager_object;
    normal.player_count = 1;
    normal.players[0].object = &player_object;
    normal.players[0].device = &device_object;
    normal.players[0].assignment = 7;
    normal.players[0].ordinal = 0;
    normal.players[0].device_kind = DeviceKind::Keyboard;
    normal.device_count = 1;
    normal.devices[0].object = &device_object;
    normal.devices[0].reference_owner = &device_object;
    normal.devices[0].assignment = 7;
    normal.devices[0].kind = DeviceKind::Keyboard;
    normal.hud[0] = &player_object;

    constexpr std::uint32_t kSentinelReturn = 0xC0DEC045u;
    const auto reset = [&]() noexcept {
        SecureZeroMemory(block, sizeof(*block));
        block->publication_enabled = 1;
        block->state = static_cast<LONG>(NativeState::Ready);
        g_shared = block;
        g_main_base = nullptr;
        g_manager = reinterpret_cast<ManagerFn>(&ManagerDiagnosticOriginal);
        g_binding.Reset();
        g_manager_pass = 0;
        ResetOrdinaryRouteTracker();
        g_manager_depth = 0;
        g_token_secret.fill(0x5Au);
        InterlockedExchange(&g_observation_enabled, 1);
        InterlockedExchange(&g_unloading, 0);
        InterlockedExchange(&g_failure_reported, 0);
        g_testing_context_override_enabled = true;
        g_testing_context = {};
        g_testing_context.loop_iteration = 17;
        g_testing_context.thread_id = GetCurrentThreadId();
        g_testing_context.main_thread_id = GetCurrentThreadId();
        g_testing_context.phase = loop::Phase::GameplayUpdate;
        g_testing_context.phase_depth = 1;
        g_testing_topology_override_enabled = false;
        g_testing_topology_result =
            static_cast<std::uint32_t>(TopologyReadResult::Transient);
        g_testing_topology = {};
        g_testing_topology_diagnostic = TopologyDiagnostic::None;
        g_testing_force_topology_exception = false;
        g_testing_last_topology_valid = false;
        g_testing_last_topology_result = 0;
        g_testing_last_topology_diagnostic = TopologyDiagnostic::None;
        g_manager_diagnostic_original_calls = 0;
        g_manager_diagnostic_original_return = kSentinelReturn;
    };
    const auto invoke = [&](ManagerCallProbe& call) noexcept {
        g_manager_diagnostic_original_calls = 0;
        call.expected_return = kSentinelReturn;
        call.observed_return = CoopManagerOwnerHook(&manager_object, nullptr);
        call.original_calls = g_manager_diagnostic_original_calls;
    };

    reset();
    g_testing_topology_override_enabled = true;
    g_testing_topology_result =
        static_cast<std::uint32_t>(TopologyReadResult::Ok);
    g_testing_topology = normal;
    invoke(probe.normal_first);
    invoke(probe.normal_second);
    probe.normal_event_count =
        static_cast<std::uint32_t>(block->next_sequence);
    for (std::size_t index = 0; index < probe.normal_event_kinds.size() &&
                                index < probe.normal_event_count;
         ++index)
        probe.normal_event_kinds[index] = block->events[index].kind;
    if (probe.normal_event_count >= 7)
    {
        probe.first_end_digest = block->events[1].topology_digest;
        probe.candidate_digest = block->events[2].topology_digest;
        probe.second_end_digest = block->events[4].topology_digest;
        probe.bound_digest = block->events[5].topology_digest;
    }

    reset();
    g_testing_topology_override_enabled = true;
    g_testing_topology_result =
        static_cast<std::uint32_t>(TopologyReadResult::Transient);
    g_testing_topology_diagnostic = rejected_diagnostic;
    probe.rejected_diagnostic = rejected_diagnostic;
    probe.expected_rejected_digest =
        TopologyDiagnosticDigest(rejected_diagnostic);
    invoke(probe.rejected);
    probe.rejected_event_count =
        static_cast<std::uint32_t>(block->next_sequence);
    for (std::size_t index = 0; index < probe.rejected_event_kinds.size() &&
                                index < probe.rejected_event_count;
         ++index)
        probe.rejected_event_kinds[index] = block->events[index].kind;
    if (probe.rejected_event_count >= 2)
        probe.rejected_digest = block->events[1].topology_digest;

    reset();
    InterlockedExchange(&g_observation_enabled, 0);
    invoke(probe.observation_disabled);
    probe.observation_disabled_event_count =
        static_cast<std::uint32_t>(block->next_sequence);

    reset();
    ++g_testing_context.main_thread_id;
    invoke(probe.wrong_thread);
    probe.wrong_thread_event_count =
        static_cast<std::uint32_t>(block->next_sequence);
    if (probe.wrong_thread_event_count != 0)
        probe.wrong_thread_last_event_kind =
            block->events[probe.wrong_thread_event_count - 1].kind;

    reset();
    g_testing_force_topology_exception = true;
    invoke(probe.topology_read_exception);
    probe.topology_read_exception_event_count =
        static_cast<std::uint32_t>(block->next_sequence);
    if (probe.topology_read_exception_event_count >= 2)
        probe.topology_read_exception_digest =
            block->events[1].topology_digest;

    reset();
    g_testing_topology_override_enabled = true;
    g_testing_topology_result =
        static_cast<std::uint32_t>(TopologyReadResult::Ok);
    g_testing_topology = normal;
    block->next_sequence = static_cast<LONG64>(kEventCapacity - 1);
    invoke(probe.ring_publish_failure);
    probe.ring_publish_failed =
        block->error_code == static_cast<LONG>(ErrorCode::RingOverflow) &&
        block->state == static_cast<LONG>(NativeState::Unsafe) &&
        block->publication_enabled == 0 &&
        InterlockedCompareExchange(&g_observation_enabled, 0, 0) == 0;

    g_shared = saved_shared;
    g_main_base = saved_main_base;
    g_manager = saved_manager;
    g_binding = saved_binding;
    g_token_secret = saved_token_secret;
    g_manager_pass = saved_manager_pass;
    g_ordinary_route = saved_ordinary_route;
    g_manager_depth = saved_manager_depth;
    InterlockedExchange(&g_observation_enabled, saved_observation);
    InterlockedExchange(&g_unloading, saved_unloading);
    InterlockedExchange(&g_failure_reported, saved_failure);
    g_testing_context_override_enabled = saved_context_override;
    g_testing_context = saved_context;
    g_testing_topology_override_enabled = saved_topology_override;
    g_testing_topology_result = saved_topology_result;
    g_testing_topology = saved_topology;
    g_testing_topology_diagnostic = saved_topology_diagnostic;
    g_testing_force_topology_exception = saved_force_exception;
    g_testing_last_topology_valid = saved_last_topology_valid;
    g_testing_last_topology_result = saved_last_topology_result;
    g_testing_last_topology_diagnostic = saved_last_topology_diagnostic;
    g_manager_diagnostic_original_calls = saved_diagnostic_calls;
    g_manager_diagnostic_original_return = saved_diagnostic_return;
    VirtualFree(block, 0, MEM_RELEASE);
    return true;
}

bool RunInvalidTopologyTransitionProbe(
    TopologyDiagnostic diagnostic, TopologyTransitionProbe& result) noexcept
{
    result = {};
    const auto code = static_cast<std::uint32_t>(diagnostic);
    if (code == static_cast<std::uint32_t>(TopologyDiagnostic::None) ||
        code > static_cast<std::uint32_t>(
                   TopologyDiagnostic::OfflineOnlineFrameNonzero))
        return false;

    Topology topology{};
    topology.player_count = 2;
    topology.players[0].object = reinterpret_cast<void*>(0x1000u);
    topology.players[0].device = reinterpret_cast<void*>(0x2000u);
    topology.players[0].ordinal = 0;
    topology.players[1].object = reinterpret_cast<void*>(0x3000u);
    topology.players[1].device = reinterpret_cast<void*>(0x4000u);
    topology.players[1].ordinal = 1;

    BindingMachine machine{};
    machine.Reset();
    machine.DropCandidate();
    const bool initially_unbound =
        !machine.bound() && !machine.retired() &&
        machine.states()[0] == BindingState::Unbound &&
        machine.states()[1] == BindingState::Unbound;
    machine.ObserveCompleteManagerPass(10, topology);
    machine.DropCandidate();
    result.unbound_stays_unbound =
        initially_unbound && !machine.bound() && !machine.retired() &&
        machine.states()[0] == BindingState::Unbound &&
        machine.states()[1] == BindingState::Unbound;

    machine.ObserveCompleteManagerPass(11, topology);
    machine.ObserveCompleteManagerPass(12, topology);
    const auto bound_generation = machine.generation();
    result.bound_becomes_unbound =
        machine.bound() && machine.Invalidate(true) && !machine.bound() &&
        !machine.retired() && machine.generation() == bound_generation + 1 &&
        machine.states()[0] == BindingState::Unbound &&
        machine.states()[1] == BindingState::Unbound;

    result.first_valid_is_candidate =
        machine.ObserveCompleteManagerPass(13, topology) ==
            BindingMachine::Observation::Candidate &&
        !machine.bound();
    result.second_valid_is_bound =
        machine.ObserveCompleteManagerPass(14, topology) ==
            BindingMachine::Observation::Bound &&
        machine.bound();
    return result.unbound_stays_unbound && result.bound_becomes_unbound &&
           result.first_valid_is_candidate && result.second_valid_is_bound;
}

} // namespace isaac_cs_t0::coop_passive::detail
#endif
