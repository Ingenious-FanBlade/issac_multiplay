#include "isaac_cs_t0/active_runtime.hpp"

#include "isaac_cs_t0/active_apply.hpp"
#include "isaac_cs_t0/active_dispatcher.hpp"
#include "isaac_cs_t0/active_identity.hpp"
#include "isaac_cs_t0/active_ipc.hpp"
#include "isaac_cs_t0/active_targets.hpp"
#include "isaac_cs_t0/active_witness.hpp"
#include "isaac_cs_t0/loop_runtime.hpp"
#include "isaac_cs_t0/raw_apply_runtime.hpp"
#include "isaac_cs_t0/sha256.hpp"
#include "isaac_cs_t0/target_image.hpp"

#include <Windows.h>
#include <intrin.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cwchar>
#include <cstring>
#include <limits>
#include <optional>

namespace isaac_cs_t0::active {
namespace {

using namespace active_abi;

constexpr std::uint32_t kExpectedImageSize = target_image::kUnpackedImageSize;
constexpr DWORD kDrainTimeoutMs = 5000;
constexpr DWORD kUnsafeResult = 8;
constexpr std::uint32_t kKeyboardVtableRva = 0x007A1678u;
constexpr std::uint32_t kGamepadVtableRva = 0x00782C80u;
constexpr std::uint32_t kKeyboardCurrentStateRva = 0x00878C10u;
constexpr std::uint32_t kGameRootRva = 0x00871678u;
constexpr std::uint32_t kPlayerVectorOffset = 0x0001BAA8u;
constexpr std::uint32_t kPlayerOwnerKeyOffset = 0x1618u;
constexpr std::uint32_t kPhysicalPollReturnRva = 0x0061F30Cu;

using LogicalBoolFn = bool(__thiscall*)(void*, std::uint32_t);
using LogicalScalarFn = float(__thiscall*)(void*, std::uint32_t);
using PlayerUpdateFn = void(__thiscall*)(void*);
using ConsumerFn = void(__thiscall*)(void*);
using ItemConsumerFn = void(__thiscall*)(void*, std::int32_t);

HMODULE g_module = nullptr;
HANDLE g_mapping = nullptr;
SharedActiveBlock* g_shared = nullptr;
std::uint8_t* g_main_base = nullptr;
volatile LONG g_override_enabled = 0;
volatile LONG g_unloading = 0;
volatile LONG g_active_hooks = 0;
volatile LONG g_unsafe_reported = 0;

std::array<isaac_cs_t0::mode::RelativeCallPatch, kActiveRelativeCount> g_relative{};
std::array<isaac_cs_t0::mode::PointerPatch, kActivePointerCount> g_pointers{};

LogicalBoolFn g_keyboard_pressed = nullptr;
LogicalBoolFn g_keyboard_triggered = nullptr;
LogicalScalarFn g_keyboard_scalar = nullptr;
LogicalBoolFn g_gamepad_pressed = nullptr;
LogicalBoolFn g_gamepad_triggered = nullptr;
LogicalScalarFn g_gamepad_scalar = nullptr;
PlayerUpdateFn g_player_update = nullptr;
std::array<ConsumerFn, kActiveRelativeCount> g_consumers{};
ItemConsumerFn g_item_consumer = nullptr;

std::optional<ActiveDispatcher> g_dispatcher;
SyntheticAdapter g_adapter{};
std::array<raw_apply::detail::ProjectionHistory, 2> g_projection_history{};
void* g_current_device = nullptr;
SourceDevice g_current_source = SourceDevice::None;
std::uint32_t g_current_device_key = UINT32_MAX;
void* g_current_player = nullptr;
std::uint32_t g_current_player_ordinal = UINT32_MAX;
std::uint32_t g_current_player_device_key = UINT32_MAX;
std::uint64_t g_owner_generation = 1;
std::uint64_t g_apply_generation = 0;
std::uint64_t g_last_completed_tick = 0;
std::uint64_t g_candidate_tick = 0;
bool g_authority_seen = false;
bool g_authority_passthrough = false;
detail::PauseGapTracker g_pause_gap{};
detail::LifecycleSubmissionGate g_lifecycle_gate{};
detail::PostLoadingAuthorityGate g_loading_recovery{};
bool g_teardown_retired = false;
input::InputCmdDraftV1 g_authority_command{};
SourceDevice g_authority_source = SourceDevice::None;
bool g_authority_command_valid = false;

extern "C" bool __fastcall ActiveKeyboardPressedHook(void*, void*, std::uint32_t);
extern "C" bool __fastcall ActiveKeyboardTriggeredHook(void*, void*, std::uint32_t);
extern "C" float __fastcall ActiveKeyboardScalarHook(void*, void*, std::uint32_t);
extern "C" bool __fastcall ActiveGamepadPressedHook(void*, void*, std::uint32_t);
extern "C" bool __fastcall ActiveGamepadTriggeredHook(void*, void*, std::uint32_t);
extern "C" float __fastcall ActiveGamepadScalarHook(void*, void*, std::uint32_t);
extern "C" void __fastcall ActivePlayerUpdateHook(void*, void*);
extern "C" void __fastcall ActiveMovementConsumerHook(void*, void*);
extern "C" void __fastcall ActiveBombConsumerHook(void*, void*);
extern "C" void __fastcall ActiveItemConsumerHook(void*, void*, std::int32_t);
extern "C" void __fastcall ActivePillConsumerHook(void*, void*);
extern "C" void __fastcall ActiveShootingConsumerHook(void*, void*);
extern "C" void __fastcall ActiveDropConsumerHook(void*, void*);

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

bool WaitForActiveCallbacksDrained() noexcept
{
    const ULONGLONG deadline = GetTickCount64() + kDrainTimeoutMs;
    while (InterlockedCompareExchange(&g_active_hooks, 0, 0) != 0 &&
           GetTickCount64() < deadline)
        Sleep(1);
    return InterlockedCompareExchange(&g_active_hooks, 0, 0) == 0;
}

std::int64_t Qpc() noexcept
{
    LARGE_INTEGER value{};
    return QueryPerformanceCounter(&value) ? value.QuadPart : 0;
}

std::uint32_t FloatBits(float value) noexcept
{
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

template <std::size_t Size>
bool Nonzero(const std::array<std::uint8_t, Size>& value) noexcept
{
    return std::any_of(value.begin(), value.end(),
                       [](std::uint8_t byte) { return byte != 0; });
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
    const auto base = reinterpret_cast<std::uintptr_t>(g_main_base);
    const auto value = reinterpret_cast<std::uintptr_t>(address);
    return base && value > base && value - base < kExpectedImageSize
               ? static_cast<std::uint32_t>(value - base)
               : 0;
}

NativeActiveEvent Event(EventKind kind, Phase phase) noexcept
{
    NativeActiveEvent event{};
    event.qpc = Qpc();
    event.thread_id = GetCurrentThreadId();
    event.kind = static_cast<std::uint32_t>(kind);
    event.phase = static_cast<std::uint32_t>(phase);
    event.client_id = g_shared ? g_shared->client_id : 0;
    event.apply_generation = g_apply_generation;
    event.owner_generation = g_owner_generation;
    event.authority_tick = g_candidate_tick ? g_candidate_tick : g_last_completed_tick;
    const auto context = loop::CurrentObservationContext();
    event.loop_iteration = context.loop_iteration;
    if (context.main_thread_id && context.main_thread_id == event.thread_id)
        event.flags |= kFlagMainThread;
    if (g_candidate_tick && context.phase == loop::Phase::EntityMutation)
        event.flags |= kFlagSteadyStandard;
    return event;
}

bool PublicationEnabled() noexcept
{
    return g_shared &&
           InterlockedCompareExchange(&g_shared->publication_enabled, 0, 0) != 0;
}

bool Publish(const NativeActiveEvent& event) noexcept
{
    if (!PublicationEnabled())
        return false;
    const LONG64 sequence = InterlockedIncrement64(&g_shared->next_sequence);
    const LONG64 consumed =
        InterlockedCompareExchange64(&g_shared->controller_sequence, 0, 0);
    if (sequence - consumed > static_cast<LONG64>(kEventCapacity))
    {
        InterlockedIncrement64(&g_shared->overwritten_events);
        InterlockedExchange(&g_override_enabled, 0);
        InterlockedExchange(&g_unsafe_reported, 1);
        InterlockedExchange(&g_shared->client_retired, 1);
        InterlockedExchange(&g_shared->publication_enabled, 0);
        SetState(ErrorCode::RingOverflow, NativeState::Unsafe);
        return false;
    }
    auto& slot =
        g_shared->events[static_cast<std::size_t>(sequence - 1) % kEventCapacity];
    InterlockedExchange64(&slot.published_sequence, 0);
    CopyEventPayload(slot, event);
    MemoryBarrier();
    InterlockedExchange64(&slot.published_sequence, sequence);
    return true;
}

void PublishFailure(ErrorCode error, std::uint32_t failure_site_rva = 0) noexcept
{
    auto event = Event(EventKind::Failure, Phase::Outside);
    event.error_code = static_cast<std::uint32_t>(error);
    event.hook_site_rva = failure_site_rva;
    Publish(event);
}

void ReportUnsafe(ErrorCode error, std::uint32_t failure_site_rva = 0) noexcept
{
    if (!g_shared || InterlockedExchange(&g_unsafe_reported, 1) != 0)
        return;
    InterlockedExchange(&g_override_enabled, 0);
    g_adapter.Retire();
    if (g_dispatcher)
        g_dispatcher->Retire();
    InterlockedExchange(&g_shared->client_retired, 1);
    PublishFailure(error, failure_site_rva);
    SetState(error, NativeState::Unsafe);
}

void RetireClientGracefully() noexcept
{
    InterlockedExchange(&g_override_enabled, 0);
    g_adapter.Retire();
    if (g_dispatcher)
        g_dispatcher->Retire();
    g_authority_command = {};
    g_authority_source = SourceDevice::None;
    g_authority_command_valid = false;
    g_authority_passthrough = false;
    if (g_shared)
        InterlockedExchange(&g_shared->client_retired, 1);
}

bool BindObservationMainThread(const loop::ObservationContext& context,
                               bool anchor_required) noexcept
{
    if (!g_shared)
        return false;
    const auto binding = detail::BindTrustedMainThread(
        g_shared->main_thread_id, context.main_thread_id, context.thread_id);
    if (binding == detail::MainThreadBinding::Bound)
        return true;
    if (binding == detail::MainThreadBinding::AwaitingAnchor && !anchor_required)
        return false;
    ReportUnsafe(ErrorCode::WrongThread);
    return false;
}

bool OverrideEnabled() noexcept
{
    return g_shared && InterlockedCompareExchange(&g_override_enabled, 0, 0) != 0 &&
           InterlockedCompareExchange(&g_unloading, 0, 0) == 0 &&
           InterlockedCompareExchange(&g_unsafe_reported, 0, 0) == 0 &&
           !g_teardown_retired;
}

void FillCanonical(NativeActiveEvent& event,
                   const input::InputStateDraftV1& state) noexcept
{
    input::EncodedInputStateV1 encoded{};
    if (input::EncodeInputState(state, input::kBaseValidEdgeMask, encoded) ==
        input::CodecError::None)
        event.canonical_state = encoded;
}

void FillCommand(NativeActiveEvent& event, const input::InputCmdDraftV1& command,
                 SourceDevice source) noexcept
{
    event.source_device = static_cast<std::uint32_t>(source);
    event.client_id = command.client_id;
    event.command_sequence = command.sequence;
    event.target_tick = command.target_tick;
    event.edge_flags = command.state.edge_flags;
    FillCanonical(event, command.state);
}

std::uint32_t ActiveComponentCount(const input::InputStateDraftV1& state) noexcept
{
    std::uint32_t count = 0;
    count += state.move_x != 0 ? 1u : 0u;
    count += state.move_y != 0 ? 1u : 0u;
    count += state.aim_x != 0 ? 1u : 0u;
    count += state.aim_y != 0 ? 1u : 0u;
    for (std::uint32_t bit = 0; bit < 4; ++bit)
        count += (state.edge_flags & (1u << bit)) != 0 ? 1u : 0u;
    return count;
}

void PublishDispatchBatch(const DispatchBatch& batch) noexcept
{
    for (std::size_t index = 0; index < batch.count; ++index)
    {
        const DispatchTrace& trace = batch.traces[index];
        if (trace.kind == EventKind::Resolve && trace.command.sequence == 0)
            continue; // An empty authority drain has no command identity to publish.
        auto event = Event(trace.kind, trace.phase);
        event.attempt = trace.attempt;
        FillCommand(event, trace.command, trace.source);
        event.decision = static_cast<std::uint32_t>(trace.decision);
        event.reject_reason = static_cast<std::uint32_t>(trace.reject);
        event.queue_depth = trace.queue_depth;
        event.previous_command_sequence = trace.previous_command_sequence;
        if (trace.has_conflicting_state)
        {
            input::EncodedInputStateV1 conflict{};
            if (input::EncodeInputState(trace.conflicting_state,
                                        input::kBaseValidEdgeMask,
                                        conflict) == input::CodecError::None)
            {
                event.conflicting_state = conflict;
                event.flags |= kFlagConflictingState;
            }
        }
        if (trace.terminal_protocol_failure)
            event.error_code =
                static_cast<std::uint32_t>(ErrorCode::ProtocolViolation);
        Publish(event);
        if (trace.kind == EventKind::Reject)
        {
            auto witness = Event(EventKind::Witness, Phase::Witness);
            witness.attempt = trace.attempt;
            FillCommand(witness, trace.command, trace.source);
            input::EncodedInputStateV1 canonical{};
            if (input::EncodeInputState(trace.command.state,
                                        input::kBaseValidEdgeMask,
                                        canonical) == input::CodecError::None)
            {
                const auto digest = mode::Sha256(canonical.data(), canonical.size());
                witness.witness_before = digest;
                witness.witness_after = digest;
                witness.flags |= kFlagWitnessValid;
                witness.query_kind =
                    static_cast<std::uint32_t>(WitnessKind::RejectedUnchanged);
            }
            Publish(witness);
        }
    }
    const bool terminal =
        batch.client_retired || batch.error != ErrorCode::None;
    if (terminal)
    {
        auto retired = Event(EventKind::ClientRetired, Phase::Outside);
        retired.flags |= kFlagClientRetired;
        Publish(retired);
        detail::HandleClientRetirement(
            terminal, batch.error,
            []() noexcept { RetireClientGracefully(); },
            [](ErrorCode error) noexcept { ReportUnsafe(error); });
    }
}

struct BindingSnapshot {
    std::array<input::PrimitiveBinding, input::kMaximumLogicalActionCount> bindings{};
    std::size_t action_count = 0;
};

bool TakeBindings(const std::uint8_t* bytes, BindingSnapshot& output) noexcept
{
    const auto* begin = *reinterpret_cast<const std::uint8_t* const*>(bytes + 0x10);
    const auto* end = *reinterpret_cast<const std::uint8_t* const*>(bytes + 0x14);
    const auto first = reinterpret_cast<std::uintptr_t>(begin);
    const auto last = reinterpret_cast<std::uintptr_t>(end);
    if (!begin || !end || last < first)
        return false;
    const std::size_t size = static_cast<std::size_t>(last - first);
    if (size < input::kGameplayActionCount * sizeof(input::PrimitiveBinding) ||
        size % sizeof(input::PrimitiveBinding) != 0 ||
        size > output.bindings.size() * sizeof(input::PrimitiveBinding))
        return false;
    output.action_count = size / sizeof(input::PrimitiveBinding);
    std::memcpy(output.bindings.data(), begin,
                input::kGameplayActionCount * sizeof(input::PrimitiveBinding));
    return true;
}

bool SameBindings(const BindingSnapshot& left,
                  const BindingSnapshot& right) noexcept
{
    return raw_apply::detail::StableBindingsEqual(
        left.bindings, left.action_count, right.bindings, right.action_count);
}

bool ReadKeyboardProjection(
    void* device, std::array<float, input::kGameplayActionCount>& values,
    std::uint16_t& valid) noexcept
{
    __try
    {
        const auto* bytes = static_cast<const std::uint8_t*>(device);
        BindingSnapshot first{};
        BindingSnapshot second{};
        std::array<std::uint8_t, raw_apply::detail::kKeyboardPrimitiveCount> state1{};
        std::array<std::uint8_t, raw_apply::detail::kKeyboardPrimitiveCount> state2{};
        if (!TakeBindings(bytes, first))
            return false;
        const bool active1 = (bytes[0x08] & 0x02u) != 0;
        for (std::uint32_t action = 0; action < input::kGameplayActionCount; ++action)
            for (std::uint32_t primitive : first.bindings[action])
                if (primitive != input::kUnboundPrimitive && primitive < state1.size())
                    state1[primitive] = g_main_base[kKeyboardCurrentStateRva + primitive];
        MemoryBarrier();
        if (!TakeBindings(bytes, second))
            return false;
        const bool active2 = (bytes[0x08] & 0x02u) != 0;
        for (std::uint32_t action = 0; action < input::kGameplayActionCount; ++action)
            for (std::uint32_t primitive : second.bindings[action])
                if (primitive != input::kUnboundPrimitive && primitive < state2.size())
                    state2[primitive] = g_main_base[kKeyboardCurrentStateRva + primitive];
        if (!SameBindings(first, second) ||
            !raw_apply::detail::StableKeyboardStateEqual(active1, state1,
                                                         active2, state2))
            return false;
        return raw_apply::detail::ProjectKeyboard(
                   second.bindings.data(), second.action_count, active2,
                   state2, values, valid) ==
               raw_apply::detail::ProjectionStatus::Ok;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool ReadGamepadProjection(
    void* device, std::array<float, input::kGameplayActionCount>& values,
    std::uint16_t& valid) noexcept
{
    __try
    {
        const auto* bytes = static_cast<const std::uint8_t*>(device);
        BindingSnapshot bindings1{};
        BindingSnapshot bindings2{};
        raw_apply::detail::GamepadConcreteState state1{};
        raw_apply::detail::GamepadConcreteState state2{};
        if (!TakeBindings(bytes, bindings1))
            return false;
        state1.flags = *reinterpret_cast<const std::uint16_t*>(bytes + 0x08);
        state1.button_low = *reinterpret_cast<const std::uint32_t*>(bytes + 0x20);
        state1.button_high = *reinterpret_cast<const std::uint32_t*>(bytes + 0x24);
        std::memcpy(state1.directional_axes.data(), bytes + 0x40,
                    sizeof(state1.directional_axes));
        state1.deadzone = *reinterpret_cast<const float*>(bytes + 0xD0);
        std::memcpy(state1.mappings.data(), bytes + 0x388, sizeof(state1.mappings));
        MemoryBarrier();
        if (!TakeBindings(bytes, bindings2))
            return false;
        state2.flags = *reinterpret_cast<const std::uint16_t*>(bytes + 0x08);
        state2.button_low = *reinterpret_cast<const std::uint32_t*>(bytes + 0x20);
        state2.button_high = *reinterpret_cast<const std::uint32_t*>(bytes + 0x24);
        std::memcpy(state2.directional_axes.data(), bytes + 0x40,
                    sizeof(state2.directional_axes));
        state2.deadzone = *reinterpret_cast<const float*>(bytes + 0xD0);
        std::memcpy(state2.mappings.data(), bytes + 0x388, sizeof(state2.mappings));
        if (!SameBindings(bindings1, bindings2) ||
            !raw_apply::detail::StableGamepadStateEqual(state1, state2))
            return false;
        return raw_apply::detail::ProjectGamepad(
                   bindings2.bindings.data(), bindings2.action_count, state2,
                   values, valid) == raw_apply::detail::ProjectionStatus::Ok;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool ReadDevice(void* device, SourceDevice& source, std::uint32_t& key) noexcept
{
    __try
    {
        void* const vtable = device ? *reinterpret_cast<void**>(device) : nullptr;
        if (vtable == g_main_base + kKeyboardVtableRva)
            source = SourceDevice::Keyboard;
        else if (vtable == g_main_base + kGamepadVtableRva)
            source = SourceDevice::Gamepad;
        else
            return false;
        const auto* bytes = static_cast<const std::uint8_t*>(device);
        const std::uint32_t first = *reinterpret_cast<const std::uint32_t*>(bytes + 0x0C);
        MemoryBarrier();
        const std::uint32_t second = *reinterpret_cast<const std::uint32_t*>(bytes + 0x0C);
        if (first != second)
            return false;
        key = first;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool SourceAllowed(SourceDevice source) noexcept
{
    if (!g_shared)
        return false;
    const auto scenario = static_cast<Scenario>(g_shared->scenario);
    if (scenario == Scenario::Controller)
        return source == SourceDevice::Gamepad;
    if (scenario == Scenario::Simultaneous || scenario == Scenario::DeviceSwitch)
        return true;
    return source == SourceDevice::Keyboard;
}

bool BuildRawSample(void* device, SourceDevice source, std::uint32_t key,
                    input::RawInputSample& raw) noexcept
{
    std::array<float, input::kGameplayActionCount> values{};
    std::uint16_t valid = 0;
    const bool read = source == SourceDevice::Keyboard
                          ? ReadKeyboardProjection(device, values, valid)
                          : ReadGamepadProjection(device, values, valid);
    if (!read || valid != 0x0FFFu)
        return false;
    const std::size_t history_index =
        source == SourceDevice::Keyboard ? 0u : 1u;
    const auto edges = raw_apply::detail::UpdateProjectionHistory(
        g_projection_history[history_index], key, values, valid);
    raw.move_x = values[1] - values[0];
    raw.move_y = values[3] - values[2];
    raw.aim_x = values[5] - values[4];
    raw.aim_y = values[7] - values[6];
    if (raw.aim_x != 0.0F || raw.aim_y != 0.0F)
        raw.held_flags |= input::kHeldShoot;
    for (std::uint32_t action = 8; action < input::kGameplayActionCount; ++action)
        if ((edges.rising & (1u << action)) != 0)
            raw.edge_flags |= 1u << (action - 8);
    return true;
}

void AdvanceOwnerGeneration(Lifecycle lifecycle,
                            std::uint32_t before = UINT32_MAX,
                            std::uint32_t after = UINT32_MAX) noexcept
{
    if (g_owner_generation == (std::numeric_limits<std::uint64_t>::max)())
    {
        ReportUnsafe(ErrorCode::AuthorityOverflow);
        return;
    }
    ++g_owner_generation;
    if (g_shared)
        InterlockedExchange64(&g_shared->owner_generation,
                              static_cast<LONG64>(g_owner_generation));
    if (g_dispatcher)
        g_dispatcher->ClearForGeneration();
    g_adapter.ClearGeneration();
    g_authority_command = {};
    g_authority_source = SourceDevice::None;
    g_authority_command_valid = false;
    g_projection_history = {};
    ClearWitnessTls();
    auto event = Event(lifecycle == Lifecycle::OwnerSwitch
                           ? EventKind::OwnerSwitch
                           : EventKind::LifecycleClear,
                       Phase::Outside);
    event.action = static_cast<std::uint32_t>(lifecycle);
    event.owner_before = before;
    event.owner_after = after;
    Publish(event);
}

void PhysicalExit(void* device, bool result, void* return_address, void*) noexcept
{
    HookScope scope;
    if (!OverrideEnabled() || !result || ReturnRva(return_address) != kPhysicalPollReturnRva)
        return;
    const auto context = loop::CurrentObservationContext();
    if (!BindObservationMainThread(context, true))
        return;
    // Lifecycle observation must remain armed so exit/resume can be seen, but no
    // device state may be read or submitted while pause/loading/room is open.
    if (!detail::PhysicalSubmissionAllowedAfterBinding(g_pause_gap,
                                                       g_lifecycle_gate,
                                                       g_loading_recovery))
        return;
    if (!detail::PhysicalOwnerReadyForSubmission(
            static_cast<Scenario>(g_shared->scenario), g_current_player != nullptr))
        return;
    SourceDevice source = SourceDevice::None;
    std::uint32_t key = UINT32_MAX;
    if (!ReadDevice(device, source, key) || !SourceAllowed(source))
        return;
    // Both devices may be polled in one V5 input envelope.  Only the Player-selected
    // device is an ownership switch; alternating poll callbacks are not.
    if (key != g_current_player_device_key)
        return;
    input::RawInputSample raw{};
    if (!BuildRawSample(device, source, key, raw))
    {
        ReportUnsafe(ErrorCode::ProtocolViolation);
        return;
    }
    if (g_current_device && (g_current_device != device ||
                             g_current_device_key != key ||
                             g_current_source != source))
        AdvanceOwnerGeneration(Lifecycle::OwnerSwitch, g_current_device_key, key);
    g_current_device = device;
    g_current_device_key = key;
    g_current_source = source;
    if (!g_dispatcher)
    {
        ReportUnsafe(ErrorCode::ProtocolViolation);
        return;
    }
    const DispatchBatch batch =
        g_dispatcher->SubmitPhysical(raw, source, g_last_completed_tick);
    const DispatchTrace* canonical = nullptr;
    for (std::size_t index = 0; index < batch.count; ++index)
    {
        if (batch.traces[index].kind == EventKind::Canonical)
        {
            canonical = &batch.traces[index];
            break;
        }
    }
    if (canonical)
    {
        auto sample = Event(EventKind::PhysicalSample, Phase::PhysicalSample);
        sample.attempt = canonical->attempt;
        sample.sample_component_count =
            ActiveComponentCount(canonical->command.state);
        sample.return_rva = ReturnRva(return_address);
        FillCommand(sample, canonical->command, source);
        Publish(sample);
    }
    PublishDispatchBatch(batch);
    if (!batch.client_retired)
        g_adapter.SetPendingTarget(g_dispatcher->PendingTargetTick());
}

bool ReadUniquePlayer(void* player, PlayerIdentity& identity) noexcept
{
    __try
    {
        void* root1 = *reinterpret_cast<void**>(g_main_base + kGameRootRva);
        if (!root1)
            return false;
        auto* bytes = static_cast<std::uint8_t*>(root1);
        void** begin1 = *reinterpret_cast<void***>(bytes + kPlayerVectorOffset);
        void** end1 = *reinterpret_cast<void***>(bytes + kPlayerVectorOffset + 4);
        if (!begin1 || !end1 || end1 - begin1 != 1 || begin1[0] != player)
            return false;
        const std::uint32_t key1 = *reinterpret_cast<const std::uint32_t*>(
            static_cast<const std::uint8_t*>(player) + kPlayerOwnerKeyOffset);
        MemoryBarrier();
        void* root2 = *reinterpret_cast<void**>(g_main_base + kGameRootRva);
        void** begin2 = *reinterpret_cast<void***>(bytes + kPlayerVectorOffset);
        void** end2 = *reinterpret_cast<void***>(bytes + kPlayerVectorOffset + 4);
        const std::uint32_t key2 = *reinterpret_cast<const std::uint32_t*>(
            static_cast<const std::uint8_t*>(player) + kPlayerOwnerKeyOffset);
        if (root1 != root2 || begin1 != begin2 || end1 != end2 ||
            begin2[0] != player || key1 != key2)
            return false;
        identity = {0, key2, g_owner_generation};
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool CurrentOwnerStable() noexcept
{
    if (!g_current_player || !g_current_device)
        return false;
    PlayerIdentity player{};
    SourceDevice source = SourceDevice::None;
    std::uint32_t device_key = UINT32_MAX;
    return ReadUniquePlayer(g_current_player, player) && player.ordinal == 0 &&
           player.device_key == g_current_player_device_key &&
           ReadDevice(g_current_device, source, device_key) &&
           source == g_current_source && device_key == g_current_device_key &&
           player.device_key == device_key;
}

bool CurrentPlayerBootstrapStable() noexcept
{
    if (!g_current_player || g_current_device)
        return false;
    PlayerIdentity player{};
    return ReadUniquePlayer(g_current_player, player) && player.ordinal == 0 &&
           player.ordinal == g_current_player_ordinal &&
           player.device_key == g_current_player_device_key;
}

void Authority(bool entering, void*, void*, void*) noexcept
{
    HookScope scope;
    if (!OverrideEnabled())
        return;
    if (!loop::CompanionHealthy())
    {
        ReportUnsafe(ErrorCode::CompanionUnsafe);
        return;
    }
    const auto context = loop::CurrentObservationContext();
    if (!BindObservationMainThread(context, true))
        return;
    if (entering)
    {
        if (g_candidate_tick != 0 || g_authority_passthrough ||
            g_last_completed_tick == (std::numeric_limits<std::uint64_t>::max)())
        {
            ReportUnsafe(ErrorCode::AuthorityOverflow);
            return;
        }
        if (detail::AuthorityObservedWithLoadingRecovery(
                g_pause_gap, g_loading_recovery) ==
            detail::PauseGapTransition::Exit)
        {
            AdvanceOwnerGeneration(Lifecycle::PauseExit);
        }
        g_authority_seen = true;
        g_candidate_tick = g_last_completed_tick + 1;
        const auto scenario = static_cast<Scenario>(g_shared->scenario);
        const bool owner_ready = CurrentOwnerStable();
        const bool preserve_loading_player =
            detail::PreserveLoadingPlayerBootstrap(
                scenario, CurrentPlayerBootstrapStable(),
                g_current_device != nullptr);
        if (!owner_ready && (g_current_player || g_current_device) &&
            !preserve_loading_player)
        {
            const std::uint32_t before = g_current_player_device_key;
            g_current_player = nullptr;
            g_current_player_ordinal = UINT32_MAX;
            g_current_player_device_key = UINT32_MAX;
            g_current_device = nullptr;
            g_current_device_key = UINT32_MAX;
            g_current_source = SourceDevice::None;
            AdvanceOwnerGeneration(Lifecycle::OwnerSwitch, before, UINT32_MAX);
        }
        if (detail::LoadingAuthorityPassthrough(scenario, owner_ready))
        {
            // The loading scenario begins at the menu with no Player.  Preserve
            // original gameplay behavior until a unique Player and its selected
            // concrete device have both appeared after LoadingExit.
            g_authority_passthrough = true;
            g_authority_command = {};
            g_authority_source = SourceDevice::None;
            g_authority_command_valid = false;
            return;
        }
        DispatchBatch batch{};
        const AuthorityResolution resolved = owner_ready && g_dispatcher
                                                 ? g_dispatcher->Resolve(
                                                       g_candidate_tick, batch)
                                                 : AuthorityResolution{};
        PublishDispatchBatch(batch);
        if (!OverrideEnabled())
            return;
        g_authority_command = {};
        g_authority_source = SourceDevice::None;
        g_authority_command_valid = false;
        if (resolved.has_input && resolved.provenance_count != 0)
        {
            g_authority_command.session_id = g_shared->session_id;
            g_authority_command.client_id = g_shared->client_id;
            g_authority_command.sequence =
                resolved.provenance[resolved.provenance_count - 1];
            g_authority_command.target_tick = g_candidate_tick;
            g_authority_command.state = resolved.state;
            g_authority_source = resolved.source;
            g_authority_command_valid = true;
        }
        else if (g_dispatcher)
        {
            g_authority_command_valid = g_dispatcher->PendingCommand(
                g_authority_command, g_authority_source);
        }
        const input::InputStateDraftV1* state =
            resolved.has_input ? &resolved.state : nullptr;
        g_adapter.SetPendingTarget(g_dispatcher ? g_dispatcher->PendingTargetTick() : 0);
        if (!g_adapter.BeginAuthority(g_candidate_tick, g_apply_generation,
                                      g_owner_generation, state))
        {
            ReportUnsafe(ErrorCode::ProtocolViolation);
            return;
        }
        if (resolved.has_input)
        {
            auto applied = Event(EventKind::ApplyCommit, Phase::Apply);
            FillCommand(applied, g_authority_command, g_authority_source);
            applied.sample_component_count =
                ActiveComponentCount(resolved.state);
            Publish(applied);
            input::EncodedInputStateV1 encoded{};
            if (input::EncodeInputState(resolved.state, input::kBaseValidEdgeMask,
                                        encoded) == input::CodecError::None)
                g_shared->effective_state = encoded;
        }
        return;
    }
    if (g_candidate_tick == 0)
    {
        ReportUnsafe(ErrorCode::ProtocolViolation);
        return;
    }
    if (g_authority_passthrough)
        g_authority_passthrough = false;
    else if (!g_adapter.ScopeActive() || !g_adapter.EndAuthority(true))
    {
        ReportUnsafe(ErrorCode::ProtocolViolation);
        return;
    }
    g_last_completed_tick = g_candidate_tick;
    InterlockedExchange64(&g_shared->authority_tick,
                          static_cast<LONG64>(g_last_completed_tick));
    g_candidate_tick = 0;
    g_shared->effective_state.fill(0);
    g_authority_command = {};
    g_authority_source = SourceDevice::None;
    g_authority_command_valid = false;
}

void TopUpdateExit(void*, void*) noexcept
{
    HookScope scope;
    if (!OverrideEnabled())
        return;
    const auto context = loop::CurrentObservationContext();
    if (!BindObservationMainThread(context, false))
        return;
    if (detail::TopUpdateCompletedWithLoadingRecovery(
            g_pause_gap, g_loading_recovery, g_authority_seen) ==
        detail::PauseGapTransition::Enter)
        AdvanceOwnerGeneration(Lifecycle::PauseEnter);
    g_authority_seen = false;
}

void LifecycleCallback(loop::EmbeddedLifecycleSignal signal, bool entering,
                       void*, void*, void*) noexcept
{
    HookScope scope;
    if (!OverrideEnabled())
        return;
    const auto context = loop::CurrentObservationContext();
    if (!BindObservationMainThread(context, false))
        return;
    switch (signal)
    {
    case loop::EmbeddedLifecycleSignal::NewRoom:
    case loop::EmbeddedLifecycleSignal::RoomLifecycle:
    case loop::EmbeddedLifecycleSignal::RoomChange:
        g_pause_gap.Reset();
        g_authority_seen = false;
        if (entering)
            g_lifecycle_gate.Enter(detail::LifecycleInputDomain::Room);
        else
            g_lifecycle_gate.Exit(detail::LifecycleInputDomain::Room);
        AdvanceOwnerGeneration(detail::PublishedRoomLifecycle(signal, entering));
        break;
    case loop::EmbeddedLifecycleSignal::AlternateEnvelope:
        g_pause_gap.Reset();
        g_authority_seen = false;
        detail::ObserveLoadingEnvelope(g_lifecycle_gate, g_loading_recovery,
                                       entering);
        AdvanceOwnerGeneration(entering ? Lifecycle::LoadingEnter
                                        : Lifecycle::LoadingExit);
        break;
    case loop::EmbeddedLifecycleSignal::RunTeardown:
    {
        if (!entering)
            break;
        g_pause_gap.Reset();
        g_loading_recovery.Reset();
        g_authority_seen = false;
        g_lifecycle_gate.Retire();
        AdvanceOwnerGeneration(Lifecycle::RunTeardown);
        g_teardown_retired = true;
        InterlockedExchange(&g_override_enabled, 0);
        g_adapter.Retire();
        if (g_dispatcher)
            g_dispatcher->Retire();
        InterlockedExchange(&g_shared->client_retired, 1);
        auto retired = Event(EventKind::ClientRetired, Phase::Outside);
        retired.action = static_cast<std::uint32_t>(Lifecycle::RunTeardown);
        retired.flags |= kFlagClientRetired;
        Publish(retired);
        break;
    }
    }
}

QueryEligibility Eligibility(void* device, std::uint32_t action) noexcept
{
    const auto context = loop::CurrentObservationContext();
    const PlayerIdentity player = CurrentPlayerIdentity();
    const Consumer consumer = CurrentConsumer();
    SourceDevice live_source = SourceDevice::None;
    std::uint32_t live_device_key = UINT32_MAX;
    const bool live_device_identity =
        ReadDevice(device, live_source, live_device_key) &&
        live_source == g_current_source && live_device_key == g_current_device_key;
    QueryEligibility eligibility{};
    eligibility.main_thread = context.main_thread_id != 0 &&
                              context.thread_id == context.main_thread_id;
    eligibility.steady_standard = g_candidate_tick != 0 &&
                                  context.phase == loop::Phase::EntityMutation;
    eligibility.unique_player = player.ordinal == 0 &&
                                player.owner_generation == g_owner_generation &&
                                g_current_player != nullptr;
    eligibility.unique_device = live_device_identity && device == g_current_device &&
                                player.device_key == live_device_key;
    eligibility.natural_consumer = consumer != Consumer::None;
    eligibility.apply_generation = g_apply_generation;
    eligibility.owner_generation = g_owner_generation;
    eligibility.consumer = consumer;
    if (consumer != Consumer::None && ConsumerForAction(action) != consumer)
        eligibility.natural_consumer = true; // Evaluate detects the cross-family mismatch.
    return eligibility;
}

bool InvokeLogicalBool(LogicalBoolFn original, void* self, std::uint32_t action,
                       QueryKind query, void* return_address) noexcept
{
    HookScope scope;
    const bool original_result = original(self, action);
    if (!OverrideEnabled())
        return original_result;
    const QueryOverride override =
        g_adapter.Evaluate(action, query, Eligibility(self, action));
    if (override.disposition == OverrideDisposition::TerminalPassthrough)
    {
        ReportUnsafe(ErrorCode::OwnerConflict);
        return original_result;
    }
    if (override.disposition == OverrideDisposition::Passthrough)
        return original_result;
    if (!g_authority_command_valid)
    {
        ReportUnsafe(ErrorCode::ProtocolViolation);
        return original_result;
    }
    const bool result = override.disposition == OverrideDisposition::Snapshot
                            ? override.boolean
                            : false;
    auto event = Event(override.disposition == OverrideDisposition::Snapshot
                           ? EventKind::QueryOverride
                           : EventKind::Suppression,
                       Phase::InputQuery);
    event.action = action;
    event.query_kind = static_cast<std::uint32_t>(query);
    event.consumer = static_cast<std::uint32_t>(CurrentConsumer());
    event.original_calls = 1;
    event.original_result_bits = original_result ? 1u : 0u;
    event.effective_result_bits = result ? 1u : 0u;
    event.return_rva = ReturnRva(return_address);
    FillCommand(event, g_authority_command, g_authority_source);
    event.flags |= kFlagOriginalCalled | kFlagQueryOverridden;
    if (result)
        event.flags |= kFlagResultTrue;
    const bool published = Publish(event);
    if (!published || !OverrideEnabled() || !loop::CompanionHealthy())
    {
        if (OverrideEnabled())
            ReportUnsafe(ErrorCode::CompanionUnsafe);
        return original_result;
    }
    return result;
}

float InvokeLogicalScalar(LogicalScalarFn original, void* self, std::uint32_t action,
                          void* return_address) noexcept
{
    HookScope scope;
    const float original_result = original(self, action);
    if (!OverrideEnabled())
        return original_result;
    const QueryOverride override =
        g_adapter.Evaluate(action, QueryKind::Scalar, Eligibility(self, action));
    if (override.disposition == OverrideDisposition::TerminalPassthrough)
    {
        ReportUnsafe(ErrorCode::OwnerConflict);
        return original_result;
    }
    if (override.disposition == OverrideDisposition::Passthrough)
        return original_result;
    if (!g_authority_command_valid)
    {
        ReportUnsafe(ErrorCode::ProtocolViolation);
        return original_result;
    }
    const float result = override.disposition == OverrideDisposition::Snapshot
                             ? override.scalar
                             : 0.0F;
    auto event = Event(override.disposition == OverrideDisposition::Snapshot
                           ? EventKind::QueryOverride
                           : EventKind::Suppression,
                       Phase::InputQuery);
    event.action = action;
    event.query_kind = static_cast<std::uint32_t>(QueryKind::Scalar);
    event.consumer = static_cast<std::uint32_t>(CurrentConsumer());
    event.original_calls = 1;
    event.original_result_bits = FloatBits(original_result);
    event.effective_result_bits = FloatBits(result);
    event.return_rva = ReturnRva(return_address);
    FillCommand(event, g_authority_command, g_authority_source);
    event.flags |= kFlagOriginalCalled | kFlagQueryOverridden;
    if (result != 0.0F)
        event.flags |= kFlagResultTrue;
    const bool published = Publish(event);
    if (!published || !OverrideEnabled() || !loop::CompanionHealthy())
    {
        if (OverrideEnabled())
            ReportUnsafe(ErrorCode::CompanionUnsafe);
        return original_result;
    }
    return result;
}

template <typename Invoke>
void InvokeConsumer(void* self, Consumer consumer, std::uint32_t site,
                    Invoke&& invoke) noexcept
{
    HookScope scope;
    if (detail::PassthroughConsumerWithoutSyntheticAuthority(
            OverrideEnabled(), g_adapter.ScopeActive(),
            g_authority_command_valid, invoke))
        return;
    if (!EnterConsumerScope(self, consumer, g_owner_generation))
    {
        ReportUnsafe(ErrorCode::OwnerConflict, site);
        invoke();
        return;
    }
    mode::Sha256Digest before{};
    mode::Sha256Digest after{};
    const WitnessStatus before_status = CaptureWitness(self, consumer, before);
    invoke();
    const WitnessStatus after_status = CaptureWitness(self, consumer, after);
    const bool left = LeaveConsumerScope(self, consumer, g_owner_generation);
    if (before_status != WitnessStatus::Ok ||
        after_status != WitnessStatus::Ok || !left || !g_authority_command_valid)
    {
        ReportUnsafe(ErrorCode::OwnerConflict, site);
        return;
    }
    auto event = Event(EventKind::Witness, Phase::Witness);
    event.consumer = static_cast<std::uint32_t>(consumer);
    event.original_calls = 1;
    event.hook_site_rva = site;
    event.return_rva = site + 5;
    event.query_kind = static_cast<std::uint32_t>(WitnessKind::WorldState);
    event.witness_before = before;
    event.witness_after = after;
    FillCommand(event, g_authority_command, g_authority_source);
    event.flags |= kFlagOriginalCalled | kFlagWitnessValid;
    Publish(event);
}

bool ValidateMapping() noexcept
{
    std::uint64_t creation = 0;
    FILETIME creation_time{}, exit_time{}, kernel_time{}, user_time{};
    if (!GetProcessTimes(GetCurrentProcess(), &creation_time, &exit_time,
                         &kernel_time, &user_time))
        return false;
    creation = (static_cast<std::uint64_t>(creation_time.dwHighDateTime) << 32) |
               creation_time.dwLowDateTime;
    if (!g_shared || g_shared->magic != kIpcMagic ||
        g_shared->schema_version != kSchemaVersion ||
        g_shared->struct_size != sizeof(SharedActiveBlock) ||
        !active_identity::ApprovedProcessIdentity(
            g_shared->target_pid, GetCurrentProcessId(),
            g_shared->process_creation_time_100ns, creation) ||
        g_shared->scenario < static_cast<std::uint32_t>(Scenario::Keyboard) ||
        g_shared->scenario > static_cast<std::uint32_t>(Scenario::Loopback) ||
        !active_identity::ApprovedIdentity(
            g_shared->target_sha256, g_shared->observed_target_sha256,
            g_shared->target_equivalence_sha256,
            g_shared->mode_gate_sha256, g_shared->loop_gate_sha256,
            g_shared->raw_prereq_sha256,
            g_shared->active_catalog_sha256, g_shared->active_plan_sha256,
            g_shared->input_script_sha256) ||
        !Nonzero(g_shared->nonce) ||
        !Nonzero(g_shared->arm_token_sha256) || !Nonzero(g_shared->session_id) ||
        g_shared->client_id == 0 || g_shared->loop_mapping_name[0] == L'\0' ||
        std::find(g_shared->loop_mapping_name.begin(),
                  g_shared->loop_mapping_name.end(), L'\0') ==
            g_shared->loop_mapping_name.end() ||
        g_shared->installed_active_hooks_mask != 0 ||
        g_shared->restored_active_hooks_mask != 0 ||
        g_shared->restored_slot_count != 0 || g_shared->main_thread_id != 0 ||
        g_shared->apply_generation != 1 || g_shared->authority_tick != 0 ||
        g_shared->owner_generation != 0 || g_shared->next_sequence != 0 ||
        g_shared->controller_sequence != 0 || g_shared->overwritten_events != 0 ||
        g_shared->active_hooks != 0 || g_shared->producer_closed != 0 ||
        g_shared->client_retired != 0 || g_shared->publication_enabled != 0 ||
        std::any_of(g_shared->effective_state.begin(),
                    g_shared->effective_state.end(),
                    [](std::uint8_t byte) { return byte != 0; }) ||
        InterlockedCompareExchange(&g_shared->state, 0, 0) !=
            static_cast<LONG>(NativeState::Empty))
        return false;
    return true;
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

std::uint64_t NewApplyGeneration() noexcept
{
    std::uint64_t value = static_cast<std::uint64_t>(Qpc()) ^
                          (static_cast<std::uint64_t>(GetCurrentProcessId()) << 32) ^
                          reinterpret_cast<std::uintptr_t>(g_shared) ^
                          g_shared->client_id;
    return value == 0 ? 1 : value;
}

bool BindOriginalsAndTargets(TargetFailure& failure) noexcept
{
    g_keyboard_pressed = reinterpret_cast<LogicalBoolFn>(g_main_base + 0x0061E8D0u);
    g_keyboard_triggered = reinterpret_cast<LogicalBoolFn>(g_main_base + 0x0061E970u);
    g_keyboard_scalar = reinterpret_cast<LogicalScalarFn>(g_main_base + 0x0061EA20u);
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
    ActiveReplacements replacements{};
    replacements.relative = {{
        reinterpret_cast<void*>(&ActiveMovementConsumerHook),
        reinterpret_cast<void*>(&ActiveBombConsumerHook),
        reinterpret_cast<void*>(&ActiveItemConsumerHook),
        reinterpret_cast<void*>(&ActivePillConsumerHook),
        reinterpret_cast<void*>(&ActiveShootingConsumerHook),
        reinterpret_cast<void*>(&ActiveDropConsumerHook),
    }};
    replacements.pointers = {{
        reinterpret_cast<void*>(&ActiveKeyboardPressedHook),
        reinterpret_cast<void*>(&ActiveKeyboardTriggeredHook),
        reinterpret_cast<void*>(&ActiveKeyboardScalarHook),
        reinterpret_cast<void*>(&ActiveGamepadPressedHook),
        reinterpret_cast<void*>(&ActiveGamepadTriggeredHook),
        reinterpret_cast<void*>(&ActiveGamepadScalarHook),
        reinterpret_cast<void*>(&ActivePlayerUpdateHook),
    }};
    return PrepareActiveTargets(g_main_base, replacements, g_relative, g_pointers,
                                failure);
}

DWORD Initialize(const wchar_t* mapping_name) noexcept
{
    if (!mapping_name || !*mapping_name || g_shared)
        return static_cast<DWORD>(ErrorCode::MappingInvalid);
    g_mapping = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, mapping_name);
    if (!g_mapping)
        return static_cast<DWORD>(ErrorCode::MappingInvalid);
    g_shared = static_cast<SharedActiveBlock*>(
        MapViewOfFile(g_mapping, FILE_MAP_ALL_ACCESS, 0, 0,
                      sizeof(SharedActiveBlock)));
    if (!ValidateMapping())
    {
        SetState(ErrorCode::MappingInvalid, NativeState::Failed);
        return static_cast<DWORD>(ErrorCode::MappingInvalid);
    }
    InterlockedExchange(&g_shared->state,
                        static_cast<LONG>(NativeState::Initializing));
    InterlockedExchange(&g_shared->publication_enabled, 1);
    InterlockedExchange(&g_unloading, 0);
    InterlockedExchange(&g_override_enabled, 0);
    InterlockedExchange(&g_active_hooks, 0);
    InterlockedExchange(&g_unsafe_reported, 0);
    g_main_base = reinterpret_cast<std::uint8_t*>(GetModuleHandleW(nullptr));
    if (!IsTargetImage())
    {
        SetState(ErrorCode::TargetImageMismatch, NativeState::Failed);
        return static_cast<DWORD>(ErrorCode::TargetImageMismatch);
    }
    TargetFailure target_failure{};
    if (!BindOriginalsAndTargets(target_failure))
    {
        auto failure = Event(EventKind::Failure, Phase::Outside);
        failure.error_code = static_cast<std::uint32_t>(ErrorCode::HookTargetMismatch);
        failure.action = static_cast<std::uint32_t>(target_failure.stage);
        failure.hook_site_rva = target_failure.index;
        Publish(failure);
        SetState(ErrorCode::HookTargetMismatch, NativeState::Failed);
        return static_cast<DWORD>(ErrorCode::HookTargetMismatch);
    }
    g_apply_generation = NewApplyGeneration();
    g_owner_generation = 1;
    g_last_completed_tick = 0;
    g_candidate_tick = 0;
    g_authority_seen = false;
    g_authority_passthrough = false;
    g_pause_gap.Reset();
    g_lifecycle_gate.Reset();
    g_loading_recovery.Reset();
    g_teardown_retired = false;
    g_authority_command = {};
    g_authority_source = SourceDevice::None;
    g_authority_command_valid = false;
    g_current_device = nullptr;
    g_current_device_key = UINT32_MAX;
    g_current_source = SourceDevice::None;
    g_current_player = nullptr;
    g_current_player_ordinal = UINT32_MAX;
    g_current_player_device_key = UINT32_MAX;
    g_projection_history = {};
    g_adapter = {};
    g_dispatcher.emplace(g_shared->session_id, g_shared->client_id,
                         static_cast<Scenario>(g_shared->scenario));
    InterlockedExchange64(&g_shared->apply_generation,
                          static_cast<LONG64>(g_apply_generation));
    InterlockedExchange64(&g_shared->owner_generation,
                          static_cast<LONG64>(g_owner_generation));
    Publish(Event(EventKind::Initialized, Phase::Outside));

    loop::EmbeddedObserverHooks hooks{};
    hooks.relative = g_relative.data();
    hooks.relative_count = g_relative.size();
    hooks.pointers = g_pointers.data();
    hooks.pointer_count = g_pointers.size();
    hooks.physical_exit = &PhysicalExit;
    hooks.authority = &Authority;
    hooks.top_update_exit = &TopUpdateExit;
    hooks.lifecycle = &LifecycleCallback;
    loop::EmbeddedObserverIdentity identity{};
    identity.target_pid = g_shared->target_pid;
    identity.scenario = g_shared->scenario;
    identity.nonce = g_shared->nonce;
    const DWORD companion =
        loop::StartEmbeddedObserver(g_shared->loop_mapping_name.data(), hooks, identity);
    if (companion != 0)
    {
        const bool unsafe = companion == kUnsafeResult;
        SetState(unsafe ? ErrorCode::UnsafeShutdown : ErrorCode::HookInstallFailed,
                 unsafe ? NativeState::Unsafe : NativeState::Failed);
        PublishFailure(unsafe ? ErrorCode::UnsafeShutdown
                              : ErrorCode::HookInstallFailed);
        return unsafe ? kUnsafeResult
                      : static_cast<DWORD>(ErrorCode::HookInstallFailed);
    }
    InterlockedExchange64(&g_shared->installed_active_hooks_mask,
                          static_cast<LONG64>(kCompleteActiveHookMask));
    auto installed = Event(EventKind::HooksInstalled, Phase::Outside);
    installed.queue_depth = kCombinedHookCount;
    Publish(installed);
    InterlockedExchange(&g_override_enabled, 1);
    SetState(ErrorCode::None, NativeState::Ready);
    return 0;
}

DWORD Shutdown() noexcept
{
    if (!g_shared || InterlockedExchange(&g_unloading, 1) != 0 ||
        InterlockedCompareExchange(&g_shared->state, 0, 0) ==
            static_cast<LONG>(NativeState::Unsafe))
        return kUnsafeResult;
    SetState(ErrorCode::None, NativeState::ShuttingDown);
    InterlockedExchange(&g_override_enabled, 0);

    std::uint32_t restored = 0;
    const auto shutdown = detail::RunShutdownStages(
        []() noexcept {
            return detail::DrainCallbacksBeforeRetirement(
                []() noexcept { return WaitForActiveCallbacksDrained(); },
                []() noexcept {
                    g_adapter.Retire();
                    if (g_dispatcher)
                        g_dispatcher->Retire();
                    g_authority_command = {};
                    g_authority_source = SourceDevice::None;
                    g_authority_command_valid = false;
                    InterlockedExchange(&g_shared->client_retired, 1);
                    g_shared->effective_state.fill(0);
                });
        },
        []() noexcept {
            const bool success = loop::RestoreEmbeddedInputHooks() == 0;
            if (success)
                InterlockedExchange64(
                    &g_shared->restored_active_hooks_mask,
                    static_cast<LONG64>(kCompleteActiveHookMask));
            return success;
        },
        []() noexcept {
            return WaitForActiveCallbacksDrained() &&
                   VerifyActiveTargetsRestored(g_relative, g_pointers);
        },
        []() noexcept { return loop::StopEmbeddedObserver() == 0; },
        [&restored]() noexcept {
            return loop::VerifyEmbeddedObserverRestored(restored) &&
                   restored == kCombinedHookCount;
        });
    if (shutdown != detail::ShutdownStageResult::Complete)
    {
        const ErrorCode error =
            shutdown == detail::ShutdownStageResult::CallbackDrainFailed ||
                    shutdown == detail::ShutdownStageResult::ActiveDrainFailed
                ? ErrorCode::DrainTimeout
                : shutdown == detail::ShutdownStageResult::SlotVerificationFailed
                      ? ErrorCode::RestoreMismatch
                      : ErrorCode::UnsafeShutdown;
        ReportUnsafe(error);
        return kUnsafeResult;
    }
    InterlockedExchange(&g_shared->restored_slot_count,
                        static_cast<LONG>(restored));
    Publish(Event(EventKind::HooksRestored, Phase::Outside));
    Publish(Event(EventKind::ProducerClosed, Phase::Outside));
    InterlockedExchange(&g_shared->producer_closed, 1);
    InterlockedExchange(&g_shared->publication_enabled, 0);
    SetState(ErrorCode::None, NativeState::Stopped);
    return 0;
}

} // namespace

extern "C" bool __fastcall ActiveKeyboardPressedHook(void* self, void*,
                                               std::uint32_t action)
{
    return InvokeLogicalBool(g_keyboard_pressed, self, action, QueryKind::Pressed,
                             _ReturnAddress());
}

extern "C" bool __fastcall ActiveKeyboardTriggeredHook(void* self, void*,
                                                 std::uint32_t action)
{
    return InvokeLogicalBool(g_keyboard_triggered, self, action,
                             QueryKind::Triggered, _ReturnAddress());
}

extern "C" float __fastcall ActiveKeyboardScalarHook(void* self, void*,
                                               std::uint32_t action)
{
    return InvokeLogicalScalar(g_keyboard_scalar, self, action, _ReturnAddress());
}

extern "C" bool __fastcall ActiveGamepadPressedHook(void* self, void*,
                                              std::uint32_t action)
{
    return InvokeLogicalBool(g_gamepad_pressed, self, action, QueryKind::Pressed,
                             _ReturnAddress());
}

extern "C" bool __fastcall ActiveGamepadTriggeredHook(void* self, void*,
                                                std::uint32_t action)
{
    return InvokeLogicalBool(g_gamepad_triggered, self, action,
                             QueryKind::Triggered, _ReturnAddress());
}

extern "C" float __fastcall ActiveGamepadScalarHook(void* self, void*,
                                              std::uint32_t action)
{
    return InvokeLogicalScalar(g_gamepad_scalar, self, action, _ReturnAddress());
}

extern "C" void __fastcall ActivePlayerUpdateHook(void* self, void*)
{
    HookScope scope;
    PlayerIdentity identity{};
    bool scoped = false;
    if (OverrideEnabled())
    {
        if (!ReadUniquePlayer(self, identity))
            ReportUnsafe(ErrorCode::OwnerConflict, 0x0076BDDCu);
        else
        {
            if (g_current_player &&
                (g_current_player != self ||
                 g_current_player_ordinal != identity.ordinal ||
                 g_current_player_device_key != identity.device_key))
            {
                AdvanceOwnerGeneration(Lifecycle::OwnerSwitch,
                                       g_current_player_ordinal, identity.ordinal);
                g_current_device = nullptr;
                g_current_device_key = UINT32_MAX;
                g_current_source = SourceDevice::None;
            }
            identity.owner_generation = g_owner_generation;
            g_current_player = self;
            g_current_player_ordinal = identity.ordinal;
            g_current_player_device_key = identity.device_key;
            // Identity discovery is continuous, but witness TLS exists only while an
            // attributable synthetic authority command owns the consumer transaction.
            if (g_adapter.ScopeActive() && g_authority_command_valid)
            {
                scoped = EnterPlayerScope(self, identity);
                if (!scoped && OverrideEnabled())
                    ReportUnsafe(ErrorCode::OwnerConflict, 0x0076BDDCu);
            }
        }
    }
    g_player_update(self);
    if (scoped && !LeavePlayerScope(self, identity))
        ReportUnsafe(ErrorCode::OwnerConflict, 0x0076BDDCu);
}

extern "C" void __fastcall ActiveMovementConsumerHook(void* self, void*)
{
    InvokeConsumer(self, Consumer::Movement, 0x00385BC2u,
                   [self]() noexcept { g_consumers[0](self); });
}

extern "C" void __fastcall ActiveBombConsumerHook(void* self, void*)
{
    InvokeConsumer(self, Consumer::Bomb, 0x00385BC9u,
                   [self]() noexcept { g_consumers[1](self); });
}

extern "C" void __fastcall ActiveItemConsumerHook(void* self, void*, std::int32_t slot)
{
    InvokeConsumer(self, Consumer::Item, 0x00385BD2u,
                   [self, slot]() noexcept { g_item_consumer(self, slot); });
}

extern "C" void __fastcall ActivePillConsumerHook(void* self, void*)
{
    InvokeConsumer(self, Consumer::PillCard, 0x00385BD9u,
                   [self]() noexcept { g_consumers[3](self); });
}

extern "C" void __fastcall ActiveShootingConsumerHook(void* self, void*)
{
    InvokeConsumer(self, Consumer::Shooting, 0x00385BE0u,
                   [self]() noexcept { g_consumers[4](self); });
}

extern "C" void __fastcall ActiveDropConsumerHook(void* self, void*)
{
    InvokeConsumer(self, Consumer::Drop, 0x00385BE7u,
                   [self]() noexcept { g_consumers[5](self); });
}

#ifdef ISAAC_CS_T0_TESTING
namespace detail {
namespace {

AbiOriginalOnceProbe* g_abi_probe = nullptr;
std::uint32_t g_abi_logical_index = 0;
std::uint32_t g_abi_consumer_index = 0;

bool __fastcall AbiBoolOriginal(void*, void*, std::uint32_t action) noexcept
{
    ++g_abi_probe->logical_calls[g_abi_logical_index];
    return action == 11;
}

float __fastcall AbiScalarOriginal(void*, void*, std::uint32_t action) noexcept
{
    ++g_abi_probe->logical_calls[g_abi_logical_index];
    return static_cast<float>(action) + 0.5F;
}

void __fastcall AbiPlayerOriginal(void*, void*) noexcept
{
    ++g_abi_probe->player_calls;
}

void __fastcall AbiConsumerOriginal(void*, void*) noexcept
{
    ++g_abi_probe->consumer_calls[g_abi_consumer_index];
}

void __fastcall AbiItemOriginal(void*, void*, std::int32_t slot) noexcept
{
    ++g_abi_probe->consumer_calls[g_abi_consumer_index];
    g_abi_probe->item_slot = slot;
}

} // namespace

bool RunAbiOriginalOnceProbe(AbiOriginalOnceProbe& probe) noexcept
{
    const auto saved_keyboard_pressed = g_keyboard_pressed;
    const auto saved_keyboard_triggered = g_keyboard_triggered;
    const auto saved_keyboard_scalar = g_keyboard_scalar;
    const auto saved_gamepad_pressed = g_gamepad_pressed;
    const auto saved_gamepad_triggered = g_gamepad_triggered;
    const auto saved_gamepad_scalar = g_gamepad_scalar;
    const auto saved_player = g_player_update;
    const auto saved_consumers = g_consumers;
    const auto saved_item = g_item_consumer;
    const LONG saved_override =
        InterlockedExchange(&g_override_enabled, 0);

    probe = {};
    g_abi_probe = &probe;
    g_keyboard_pressed = reinterpret_cast<LogicalBoolFn>(&AbiBoolOriginal);
    g_keyboard_triggered = reinterpret_cast<LogicalBoolFn>(&AbiBoolOriginal);
    g_keyboard_scalar = reinterpret_cast<LogicalScalarFn>(&AbiScalarOriginal);
    g_gamepad_pressed = reinterpret_cast<LogicalBoolFn>(&AbiBoolOriginal);
    g_gamepad_triggered = reinterpret_cast<LogicalBoolFn>(&AbiBoolOriginal);
    g_gamepad_scalar = reinterpret_cast<LogicalScalarFn>(&AbiScalarOriginal);
    g_player_update = reinterpret_cast<PlayerUpdateFn>(&AbiPlayerOriginal);
    g_consumers.fill(reinterpret_cast<ConsumerFn>(&AbiConsumerOriginal));
    g_item_consumer = reinterpret_cast<ItemConsumerFn>(&AbiItemOriginal);

    std::uint32_t object = 0;
    g_abi_logical_index = 0;
    probe.keyboard_pressed_result =
        ActiveKeyboardPressedHook(&object, nullptr, 11);
    g_abi_logical_index = 1;
    probe.keyboard_triggered_result =
        ActiveKeyboardTriggeredHook(&object, nullptr, 10);
    g_abi_logical_index = 2;
    probe.keyboard_scalar_result =
        ActiveKeyboardScalarHook(&object, nullptr, 7);
    g_abi_logical_index = 3;
    probe.gamepad_pressed_result =
        ActiveGamepadPressedHook(&object, nullptr, 11);
    g_abi_logical_index = 4;
    probe.gamepad_triggered_result =
        ActiveGamepadTriggeredHook(&object, nullptr, 10);
    g_abi_logical_index = 5;
    probe.gamepad_scalar_result =
        ActiveGamepadScalarHook(&object, nullptr, 8);

    ActivePlayerUpdateHook(&object, nullptr);
    g_abi_consumer_index = 0;
    ActiveMovementConsumerHook(&object, nullptr);
    g_abi_consumer_index = 1;
    ActiveBombConsumerHook(&object, nullptr);
    g_abi_consumer_index = 2;
    ActiveItemConsumerHook(&object, nullptr, 17);
    g_abi_consumer_index = 3;
    ActivePillConsumerHook(&object, nullptr);
    g_abi_consumer_index = 4;
    ActiveShootingConsumerHook(&object, nullptr);
    g_abi_consumer_index = 5;
    ActiveDropConsumerHook(&object, nullptr);

    g_keyboard_pressed = saved_keyboard_pressed;
    g_keyboard_triggered = saved_keyboard_triggered;
    g_keyboard_scalar = saved_keyboard_scalar;
    g_gamepad_pressed = saved_gamepad_pressed;
    g_gamepad_triggered = saved_gamepad_triggered;
    g_gamepad_scalar = saved_gamepad_scalar;
    g_player_update = saved_player;
    g_consumers = saved_consumers;
    g_item_consumer = saved_item;
    g_abi_probe = nullptr;
    InterlockedExchange(&g_override_enabled, saved_override);

    return std::all_of(std::begin(probe.logical_calls),
                       std::end(probe.logical_calls),
                       [](std::uint32_t calls) { return calls == 1; }) &&
           probe.player_calls == 1 &&
           std::all_of(std::begin(probe.consumer_calls),
                       std::end(probe.consumer_calls),
                       [](std::uint32_t calls) { return calls == 1; });
}

bool RunConflictRetirementRuntimeProbe(
    ConflictRetirementRuntimeProbe& probe) noexcept
{
    probe = {};
    if (g_shared || g_dispatcher ||
        InterlockedCompareExchange(&g_override_enabled, 0, 0) != 0 ||
        InterlockedCompareExchange(&g_unloading, 0, 0) != 0 ||
        InterlockedCompareExchange(&g_unsafe_reported, 0, 0) != 0 ||
        g_authority_passthrough || g_authority_command_valid ||
        g_authority_command.sequence != 0 ||
        g_authority_source != SourceDevice::None || g_teardown_retired ||
        g_candidate_tick != 0 || g_last_completed_tick != 0 ||
        g_apply_generation != 0 || g_owner_generation != 1 ||
        g_adapter.Evaluate(0, QueryKind::Scalar, {}).disposition !=
            OverrideDisposition::Passthrough)
        return false;
    auto* block = static_cast<SharedActiveBlock*>(VirtualAlloc(
        nullptr, sizeof(SharedActiveBlock), MEM_RESERVE | MEM_COMMIT,
        PAGE_READWRITE));
    if (!block)
        return false;

    const std::array<std::uint8_t, 16> session{{
        1, 2, 3, 4, 5, 6, 7, 8,
        9, 10, 11, 12, 13, 14, 15, 16,
    }};
    const auto reset_runtime = [&](Scenario scenario) noexcept {
        std::memset(block, 0, sizeof(SharedActiveBlock));
        block->publication_enabled = 1;
        block->state = static_cast<LONG>(NativeState::Ready);
        block->error_code = static_cast<LONG>(ErrorCode::None);
        block->scenario = static_cast<std::uint32_t>(scenario);
        block->client_id = 7;
        block->session_id = session;
        g_shared = block;
        InterlockedExchange(&g_override_enabled, 1);
        InterlockedExchange(&g_unloading, 0);
        InterlockedExchange(&g_unsafe_reported, 0);
        g_dispatcher.reset();
        g_dispatcher.emplace(session, 7, scenario);
        g_adapter = SyntheticAdapter{};
        g_authority_command = {};
        g_authority_command.sequence = 99;
        g_authority_source = SourceDevice::Keyboard;
        g_authority_command_valid = true;
        g_authority_passthrough = false;
        g_pause_gap.Reset();
        g_lifecycle_gate.Reset();
        g_loading_recovery.Reset();
        g_teardown_retired = false;
        g_candidate_tick = 0;
        g_last_completed_tick = 10;
        g_apply_generation = 1;
        g_owner_generation = 1;
    };
    const auto event_kind = [&](std::size_t index) noexcept {
        return static_cast<EventKind>(block->events[index].kind);
    };

    reset_runtime(Scenario::Conflict);
    const auto batch = g_dispatcher->SubmitPhysical(
        {}, SourceDevice::Keyboard, g_last_completed_tick);
    PublishDispatchBatch(batch);
    probe.conflict_event_count = static_cast<std::uint32_t>(
        InterlockedCompareExchange64(&block->next_sequence, 0, 0));
    constexpr std::array<EventKind, 9> expected{{
        EventKind::Canonical, EventKind::Send, EventKind::Receive,
        EventKind::Accept, EventKind::Send, EventKind::Receive,
        EventKind::Reject, EventKind::Witness, EventKind::ClientRetired,
    }};
    probe.conflict_event_order = probe.conflict_event_count == expected.size();
    for (std::size_t index = 0;
         probe.conflict_event_order && index < expected.size(); ++index)
    {
        probe.conflict_event_order =
            InterlockedCompareExchange64(
                &block->events[index].published_sequence, 0, 0) ==
                static_cast<LONG64>(index + 1) &&
            event_kind(index) == expected[index];
    }
    if (probe.conflict_event_count >= 8)
    {
        const auto& rejected = block->events[6];
        const auto& witness = block->events[7];
        probe.conflict_witness_unchanged =
            rejected.reject_reason ==
                static_cast<std::uint32_t>(RejectReason::IdentityConflict) &&
            witness.query_kind ==
                static_cast<std::uint32_t>(WitnessKind::RejectedUnchanged) &&
            witness.witness_before == witness.witness_after &&
            (witness.flags & kFlagWitnessValid) != 0;
    }
    probe.conflict_has_no_apply_or_failure = true;
    for (std::size_t index = 0; index < probe.conflict_event_count; ++index)
    {
        const auto kind = event_kind(index);
        if (kind == EventKind::ApplyCommit || kind == EventKind::Failure)
            probe.conflict_has_no_apply_or_failure = false;
    }
    probe.conflict_state_remains_ready =
        InterlockedCompareExchange(&block->state, 0, 0) ==
            static_cast<LONG>(NativeState::Ready) &&
        InterlockedCompareExchange(&block->error_code, 0, 0) ==
            static_cast<LONG>(ErrorCode::None) &&
        InterlockedCompareExchange(&block->client_retired, 0, 0) != 0 &&
        InterlockedCompareExchange(&g_unsafe_reported, 0, 0) == 0;
    probe.conflict_override_disabled =
        InterlockedCompareExchange(&g_override_enabled, 0, 0) == 0 &&
        !g_authority_command_valid && g_authority_command.sequence == 0 &&
        g_authority_source == SourceDevice::None;
    probe.conflict_adapter_passthrough =
        g_adapter.Evaluate(0, QueryKind::Scalar, {}).disposition ==
        OverrideDisposition::TerminalPassthrough;
    probe.conflict_dispatcher_retired =
        g_dispatcher && g_dispatcher->Retired();
    const LONG64 before_callbacks =
        InterlockedCompareExchange64(&block->next_sequence, 0, 0);
    PhysicalExit(nullptr, true, nullptr, nullptr);
    Authority(true, nullptr, nullptr, nullptr);
    TopUpdateExit(nullptr, nullptr);
    probe.post_retirement_callbacks_are_silent =
        InterlockedCompareExchange64(&block->next_sequence, 0, 0) ==
            before_callbacks &&
        InterlockedCompareExchange(&block->state, 0, 0) ==
            static_cast<LONG>(NativeState::Ready);

    reset_runtime(Scenario::Keyboard);
    DispatchBatch protocol{};
    protocol.error = ErrorCode::ProtocolViolation;
    PublishDispatchBatch(protocol);
    probe.protocol_event_count = static_cast<std::uint32_t>(
        InterlockedCompareExchange64(&block->next_sequence, 0, 0));
    probe.protocol_state_is_unsafe =
        InterlockedCompareExchange(&block->state, 0, 0) ==
            static_cast<LONG>(NativeState::Unsafe) &&
        InterlockedCompareExchange(&g_override_enabled, 0, 0) == 0;
    probe.protocol_error_preserved =
        InterlockedCompareExchange(&block->error_code, 0, 0) ==
        static_cast<LONG>(ErrorCode::ProtocolViolation);
    probe.protocol_failure_published =
        probe.protocol_event_count == 2 &&
        event_kind(0) == EventKind::ClientRetired &&
        event_kind(1) == EventKind::Failure &&
        block->events[1].error_code ==
            static_cast<std::uint32_t>(ErrorCode::ProtocolViolation);

    g_dispatcher.reset();
    g_adapter = SyntheticAdapter{};
    g_shared = nullptr;
    InterlockedExchange(&g_override_enabled, 0);
    InterlockedExchange(&g_unloading, 0);
    InterlockedExchange(&g_unsafe_reported, 0);
    g_authority_command = {};
    g_authority_source = SourceDevice::None;
    g_authority_command_valid = false;
    g_authority_passthrough = false;
    g_pause_gap.Reset();
    g_lifecycle_gate.Reset();
    g_loading_recovery.Reset();
    g_teardown_retired = false;
    g_candidate_tick = 0;
    g_last_completed_tick = 0;
    g_apply_generation = 0;
    g_owner_generation = 1;
    VirtualFree(block, 0, MEM_RELEASE);

    return probe.conflict_event_order &&
           probe.conflict_witness_unchanged &&
           probe.conflict_has_no_apply_or_failure &&
           probe.conflict_state_remains_ready &&
           probe.conflict_override_disabled &&
           probe.conflict_adapter_passthrough &&
           probe.conflict_dispatcher_retired &&
           probe.post_retirement_callbacks_are_silent &&
           probe.protocol_state_is_unsafe && probe.protocol_error_preserved &&
           probe.protocol_failure_published;
}

} // namespace detail
#endif

void SetModule(HMODULE module) noexcept
{
    g_module = module;
}

void ProcessDetach() noexcept
{
    ClearWitnessTls();
    g_dispatcher.reset();
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
}

bool Active() noexcept
{
    return OverrideEnabled();
}

extern "C" DWORD WINAPI IsaacCsT0InputActiveInitialize(
    const wchar_t* mapping_name)
{
    return Initialize(mapping_name);
}

extern "C" DWORD WINAPI IsaacCsT0InputActiveShutdown(void*)
{
    const DWORD result = Shutdown();
    if (result != 0)
        return result;
    HMODULE module = g_module;
    FreeLibraryAndExitThread(module, 0);
}

} // namespace isaac_cs_t0::active
