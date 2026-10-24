#include "isaac_cs_t0/raw_apply_runtime.hpp"

#include "isaac_cs_t0/hooks.hpp"
#include "isaac_cs_t0/input_core.hpp"
#include "isaac_cs_t0/loop_runtime.hpp"
#include "isaac_cs_t0/raw_apply_ipc.hpp"

#include <Windows.h>
#include <intrin.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace isaac_cs_t0::raw_apply {
namespace {

constexpr std::uint32_t kExpectedTimestamp = 0x69E6E3A7u;
constexpr std::uint32_t kExpectedImageSize = 0x0092C000u;
constexpr std::uint32_t kGameRootRva = 0x00871678u;
constexpr std::uint32_t kPlayerVectorOffset = 0x0001BAA8u;
constexpr std::uint32_t kPlayerOwnerKeyOffset = 0x1618u;
constexpr std::uint32_t kInputManagerObjectRva = 0x00857B18u;
constexpr std::uint32_t kInputManagerVtableRva = 0x00782950u;
constexpr std::uint32_t kInputManagerColRva = 0x007B0424u;
constexpr std::uint32_t kKeyboardVtableRva = 0x007A1678u;
constexpr std::uint32_t kKeyboardColRva = 0x007B191Cu;
constexpr std::uint32_t kGamepadVtableRva = 0x00782C80u;
constexpr std::uint32_t kGamepadColRva = 0x007B0460u;
constexpr std::uint32_t kManagerPressedRva = 0x00620710u;
constexpr std::uint32_t kManagerTriggeredRva = 0x006207B0u;
constexpr std::uint32_t kManagerScalarRva = 0x00620850u;
constexpr std::uint32_t kLogicalPressedRva = 0x0061E8D0u;
constexpr std::uint32_t kLogicalTriggeredRva = 0x0061E970u;
constexpr std::uint32_t kLogicalScalarRva = 0x0061EA20u;
constexpr std::uint32_t kLogicalSetterRva = 0x0061ED50u;
constexpr std::uint32_t kKeyboardCurrentStateRva = 0x00878C10u;
constexpr std::uint32_t kPlayerVtableSlotRva = 0x0076BDDCu;
constexpr std::uint32_t kPlayerUpdateRva = 0x00382AF0u;
constexpr DWORD kDrainTimeoutMs = 5000;
constexpr DWORD kUnsafeResult = 8;
constexpr std::array<std::uint8_t, 32> kTargetSha256{{
    0xC4, 0x11, 0x85, 0x19, 0xBA, 0x5C, 0x05, 0x18,
    0x26, 0xFE, 0x16, 0x50, 0x77, 0x15, 0xDE, 0x60,
    0x2B, 0x51, 0xBE, 0x4F, 0x2C, 0xD7, 0x15, 0x34,
    0xEA, 0x13, 0x03, 0x7A, 0x0F, 0x9B, 0xAE, 0x30,
}};
constexpr std::array<std::uint8_t, 32> kLoopGateSha256{{
    0xCD, 0xC0, 0xCE, 0x89, 0xA7, 0x39, 0x37, 0x40,
    0xA7, 0x75, 0xE4, 0x0F, 0xE6, 0xE5, 0x88, 0x91,
    0x00, 0x38, 0xFE, 0xC6, 0x8C, 0x40, 0xEB, 0x6D,
    0x6F, 0x5C, 0xDF, 0x7B, 0xC1, 0xF1, 0xA1, 0xAA,
}};

constexpr std::array<std::uint32_t, 6> kConsumerSiteRvas{{
    0x00385BC2u, 0x00385BC9u, 0x00385BD2u,
    0x00385BD9u, 0x00385BE0u, 0x00385BE7u,
}};
constexpr std::array<std::uint32_t, 6> kConsumerTargetRvas{{
    0x00379B40u, 0x00380F40u, 0x00391A40u,
    0x003A3D30u, 0x0037B020u, 0x003A42B0u,
}};
constexpr std::array<std::array<std::uint8_t, 5>, 6> kConsumerCallBytes{{
    {{0xE8, 0x79, 0x3F, 0xFF, 0xFF}},
    {{0xE8, 0x72, 0xB3, 0xFF, 0xFF}},
    {{0xE8, 0x69, 0xBE, 0x00, 0x00}},
    {{0xE8, 0x52, 0xE1, 0x01, 0x00}},
    {{0xE8, 0x3B, 0x54, 0xFF, 0xFF}},
    {{0xE8, 0xC4, 0xE6, 0x01, 0x00}},
}};

enum class DeviceKind : std::uint32_t { Unknown, Keyboard, Gamepad };
using ThisVoidFn = void(__thiscall*)(void*);
using ThisArgVoidFn = void(__thiscall*)(void*, std::int32_t);
using LogicalBoolFn = bool(__thiscall*)(void*, std::uint32_t);
using LogicalScalarFn = float(__thiscall*)(void*, std::uint32_t);
using LogicalSetterFn = void(__thiscall*)(void*, std::uint32_t, float);
using ManagerBoolFn = bool(__thiscall*)(void*, std::uint32_t, std::int32_t, std::int32_t*);
using ManagerScalarFn = float(__thiscall*)(void*, std::uint32_t, std::int32_t,
                                           std::int32_t*);

HMODULE g_module = nullptr;
HANDLE g_mapping = nullptr;
SharedRawApplyBlock* g_shared = nullptr;
std::uint8_t* g_main_base = nullptr;
volatile LONG g_armed = 0;
volatile LONG g_unloading = 0;
volatile LONG g_active_hooks = 0;
volatile LONG g_unsafe_reported = 0;
volatile LONG64 g_next_sample_generation = 0;
volatile LONG64 g_next_consumer_transaction = 0;
volatile LONG64 g_next_context_generation = 0;

std::array<isaac_cs_t0::mode::RelativeCallPatch, 6> g_relative_patches{};
std::array<isaac_cs_t0::mode::PointerPatch, 12> g_pointer_patches{};

ThisVoidFn g_player_update = nullptr;
std::array<ThisVoidFn, 6> g_consumers{};
ThisArgVoidFn g_item_consumer = nullptr;
ManagerBoolFn g_manager_pressed = nullptr;
ManagerBoolFn g_manager_triggered = nullptr;
ManagerScalarFn g_manager_scalar = nullptr;
LogicalBoolFn g_keyboard_pressed = nullptr;
LogicalBoolFn g_keyboard_triggered = nullptr;
LogicalScalarFn g_keyboard_scalar = nullptr;
LogicalBoolFn g_gamepad_pressed = nullptr;
LogicalBoolFn g_gamepad_triggered = nullptr;
LogicalScalarFn g_gamepad_scalar = nullptr;
LogicalSetterFn g_keyboard_setter = nullptr;
LogicalSetterFn g_gamepad_setter = nullptr;

struct SampleState {
    volatile LONG key = static_cast<LONG>(kNoValue);
    volatile LONG64 generation = 0;
    volatile LONG64 loop_iteration = 0;
    volatile LONG thread_id = 0;
};
std::array<SampleState, 2> g_samples{};
std::array<detail::ProjectionHistory, 2> g_projection_history{};

struct ThreadContext {
    void* player = nullptr;
    std::uint32_t player_ordinal = kNoValue;
    std::uint32_t device_key = kNoValue;
    std::uint64_t context_generation = 0;
    std::uint64_t sample_generation = 0;
    DeviceKind sample_device_kind = DeviceKind::Unknown;
    std::uint64_t consumer_transaction = 0;
    std::uint32_t consumer_site = 0;
    std::uint64_t poll_generation = 0;
};
thread_local ThreadContext g_tls{};

extern "C" void __fastcall PlayerUpdateHook(void* self, void* unused);
extern "C" void __fastcall MovementConsumerHook(void* self, void* unused);
extern "C" void __fastcall BombConsumerHook(void* self, void* unused);
extern "C" void __fastcall ItemConsumerHook(void* self, void* unused, std::int32_t slot);
extern "C" void __fastcall PillConsumerHook(void* self, void* unused);
extern "C" void __fastcall ShootingConsumerHook(void* self, void* unused);
extern "C" void __fastcall DropConsumerHook(void* self, void* unused);
extern "C" bool __fastcall RawManagerPressedHook(void*, void*, std::uint32_t, std::int32_t,
                                                std::int32_t*);
extern "C" bool __fastcall RawManagerTriggeredHook(void*, void*, std::uint32_t, std::int32_t,
                                                  std::int32_t*);
extern "C" float __fastcall RawManagerScalarHook(void*, void*, std::uint32_t, std::int32_t,
                                                std::int32_t*);
extern "C" bool __fastcall RawKeyboardPressedHook(void*, void*, std::uint32_t);
extern "C" bool __fastcall RawKeyboardTriggeredHook(void*, void*, std::uint32_t);
extern "C" float __fastcall RawKeyboardScalarHook(void*, void*, std::uint32_t);
extern "C" bool __fastcall RawGamepadPressedHook(void*, void*, std::uint32_t);
extern "C" bool __fastcall RawGamepadTriggeredHook(void*, void*, std::uint32_t);
extern "C" float __fastcall RawGamepadScalarHook(void*, void*, std::uint32_t);
extern "C" void __fastcall RawKeyboardSetterHook(void*, void*, std::uint32_t, float);
extern "C" void __fastcall RawGamepadSetterHook(void*, void*, std::uint32_t, float);

bool PublishConcreteProjection(void* self, DeviceKind kind, std::uint32_t key,
                               std::uint64_t generation, std::uint32_t hook_site_rva,
                               std::uint32_t return_rva, std::uint32_t phase) noexcept;

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

std::uint32_t FloatBits(float value) noexcept
{
    std::uint32_t result = 0;
    std::memcpy(&result, &value, sizeof(result));
    return result;
}

bool Nonzero(const std::array<std::uint8_t, 32>& value) noexcept
{
    return std::any_of(value.begin(), value.end(), [](std::uint8_t byte) {
        return byte != 0;
    });
}

bool Armed() noexcept
{
    return g_shared && InterlockedCompareExchange(&g_armed, 0, 0) != 0 &&
           InterlockedCompareExchange(&g_unloading, 0, 0) == 0 &&
           InterlockedCompareExchange(&g_unsafe_reported, 0, 0) == 0;
}

void SetState(Error error, NativeState state) noexcept
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
    if (!base || !value || value <= base)
        return 0;
    const auto difference = value - base;
    return difference < kExpectedImageSize ? static_cast<std::uint32_t>(difference) : 0;
}

NativeRawApplyEvent Event(EventKind kind, bool lifecycle = false) noexcept
{
    NativeRawApplyEvent event{};
    event.qpc = Qpc();
    event.thread_id = GetCurrentThreadId();
    event.kind = static_cast<std::uint32_t>(kind);
    if (lifecycle)
    {
        event.phase = static_cast<std::uint32_t>(loop::Phase::OutsideEnvelope);
        return event;
    }
    const auto context = loop::CurrentObservationContext();
    event.loop_iteration = context.loop_iteration;
    event.phase = static_cast<std::uint32_t>(context.phase);
    if (context.loop_iteration)
        event.flags |= kFlagLoopKnown;
    if (context.main_thread_id && context.thread_id == context.main_thread_id)
        event.flags |= kFlagMainThread;
    else if (context.main_thread_id)
        event.flags |= kFlagWrongThread;
    return event;
}

void Publish(const NativeRawApplyEvent& event) noexcept
{
    if (!g_shared)
        return;
    const LONG64 sequence = InterlockedIncrement64(&g_shared->next_sequence);
    const LONG64 consumed = InterlockedCompareExchange64(&g_shared->controller_sequence, 0, 0);
    if (sequence - consumed > static_cast<LONG64>(kEventCapacity))
    {
        InterlockedIncrement64(&g_shared->overwritten_events);
        InterlockedExchange(&g_armed, 0);
        InterlockedExchange(&g_unsafe_reported, 1);
        SetState(Error::RingOverflow, NativeState::Unsafe);
    }
    auto& slot = g_shared->events[static_cast<std::size_t>(sequence - 1) % kEventCapacity];
    InterlockedExchange64(&slot.published_sequence, 0);
    CopyEventPayload(slot, event);
    MemoryBarrier();
    InterlockedExchange64(&slot.published_sequence, sequence);
}

void Redact() noexcept
{
    if (g_shared)
        InterlockedIncrement64(&g_shared->redacted_events);
}

void ReportUnsafe(Error error) noexcept
{
    if (!g_shared || InterlockedExchange(&g_unsafe_reported, 1) != 0)
        return;
    InterlockedExchange(&g_armed, 0);
    auto event = Event(EventKind::Failure, true);
    event.error_code = static_cast<std::uint32_t>(error);
    Publish(event);
    SetState(error, NativeState::Unsafe);
}

bool Eligible(NativeRawApplyEvent& event, EventKind kind, void* return_address) noexcept
{
    if (!Armed())
        return false;
    event = Event(kind);
    const auto context = loop::CurrentObservationContext();
    if (!context.main_thread_id || !context.loop_iteration)
        return false;
    if (context.thread_id != context.main_thread_id)
    {
        ReportUnsafe(Error::WrongThread);
        return false;
    }
    if (context.phase == loop::Phase::OutsideEnvelope)
    {
        Redact();
        return false;
    }
    event.return_rva = ReturnRva(return_address);
    if (!event.return_rva)
    {
        Redact();
        return false;
    }
    InterlockedExchange(&g_shared->main_thread_id, static_cast<LONG>(context.main_thread_id));
    InterlockedExchange64(&g_shared->current_loop_iteration,
                          static_cast<LONG64>(context.loop_iteration));
    return Armed();
}

bool Device(void* self, DeviceKind& kind, std::uint32_t& key) noexcept
{
    __try
    {
        if (!self)
            return false;
        void* vtable = *reinterpret_cast<void**>(self);
        if (vtable == g_main_base + kKeyboardVtableRva)
            kind = DeviceKind::Keyboard;
        else if (vtable == g_main_base + kGamepadVtableRva)
            kind = DeviceKind::Gamepad;
        else
            return false;
        key = *reinterpret_cast<const std::uint32_t*>(
            static_cast<const std::uint8_t*>(self) + 0x0C);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

void FillDeviceFlags(NativeRawApplyEvent& event, DeviceKind kind) noexcept
{
    if (kind == DeviceKind::Keyboard)
        event.flags |= kFlagKeyboard;
    else if (kind == DeviceKind::Gamepad)
        event.flags |= kFlagGamepad;
}

std::uint64_t MatchingSample(std::uint32_t key, DeviceKind* matched_kind = nullptr) noexcept
{
    std::uint64_t match = 0;
    std::uint32_t count = 0;
    std::size_t matched_index = 0;
    for (std::size_t index = 0; index < g_samples.size(); ++index)
    {
        auto& sample = g_samples[index];
        const LONG64 before = InterlockedCompareExchange64(&sample.generation, 0, 0);
        const auto sample_key = static_cast<std::uint32_t>(
            InterlockedCompareExchange(&sample.key, 0, 0));
        const LONG64 after = InterlockedCompareExchange64(&sample.generation, 0, 0);
        if (before != 0 && before == after && sample_key == key)
        {
            match = static_cast<std::uint64_t>(before);
            matched_index = index;
            ++count;
        }
    }
    if (count != 1)
        return 0;
    if (matched_kind)
        *matched_kind = matched_index == 0 ? DeviceKind::Keyboard : DeviceKind::Gamepad;
    return match;
}

bool ReadPlayerIdentity(void* self, std::uint32_t& ordinal,
                        std::uint32_t& device_key) noexcept
{
    __try
    {
        void* root1 = *reinterpret_cast<void**>(g_main_base + kGameRootRva);
        if (!root1)
            return false;
        auto* root_bytes = static_cast<std::uint8_t*>(root1);
        void** begin1 = *reinterpret_cast<void***>(root_bytes + kPlayerVectorOffset);
        void** end1 = *reinterpret_cast<void***>(root_bytes + kPlayerVectorOffset + 4);
        if (!begin1 || !end1 || end1 < begin1 || end1 - begin1 > 64)
            return false;
        std::uint32_t found = kNoValue;
        std::uint32_t matches = 0;
        for (void** cursor = begin1; cursor != end1; ++cursor)
        {
            if (*cursor == self)
            {
                found = static_cast<std::uint32_t>(cursor - begin1);
                ++matches;
            }
        }
        MemoryBarrier();
        void* root2 = *reinterpret_cast<void**>(g_main_base + kGameRootRva);
        void** begin2 = *reinterpret_cast<void***>(root_bytes + kPlayerVectorOffset);
        void** end2 = *reinterpret_cast<void***>(root_bytes + kPlayerVectorOffset + 4);
        if (root2 != root1 || begin2 != begin1 || end2 != end1 || matches != 1 ||
            begin2[found] != self)
            return false;
        const auto key1 = *reinterpret_cast<const std::uint32_t*>(
            static_cast<const std::uint8_t*>(self) + kPlayerOwnerKeyOffset);
        MemoryBarrier();
        const auto key2 = *reinterpret_cast<const std::uint32_t*>(
            static_cast<const std::uint8_t*>(self) + kPlayerOwnerKeyOffset);
        if (key1 != key2)
            return false;
        ordinal = found;
        device_key = key2;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

void FillContext(NativeRawApplyEvent& event) noexcept
{
    if (!g_tls.context_generation)
        return;
    event.context_generation = g_tls.context_generation;
    event.consumer_transaction = g_tls.consumer_transaction;
    event.player_ordinal = g_tls.player_ordinal;
    event.device_key = g_tls.device_key;
    event.flags |= kFlagContextKnown | kFlagPlayerOrdinalValid;
    if (g_tls.consumer_transaction)
        event.flags |= kFlagConsumerKnown;
    event.sample_generation = g_tls.sample_generation;
    if (event.sample_generation)
    {
        event.flags |= kFlagSampleKnown;
        FillDeviceFlags(event, g_tls.sample_device_kind);
    }
}

void PublishWitness(EventKind kind, std::uint32_t site, WitnessKind witness,
                    std::uint32_t value, std::uint32_t auxiliary) noexcept
{
    auto event = Event(kind);
    event.hook_site_rva = site;
    event.return_rva = site + 5;
    event.value_bits = value;
    event.witness_aux_bits = auxiliary;
    event.witness_kind = static_cast<std::uint32_t>(witness);
    event.classification = static_cast<std::uint32_t>(Classification::PassiveWorldWitness);
    event.flags |= kFlagValueValid | kFlagWitnessValid;
    if (kind == EventKind::WorldWitnessAfter)
    {
        event.original_calls = 1;
        event.flags |= kFlagOriginalCalled;
    }
    FillContext(event);
    Publish(event);
}

void SnapshotWitness(void* player, std::size_t consumer_index, EventKind kind) noexcept
{
    __try
    {
        const auto* bytes = static_cast<const std::uint8_t*>(player);
        switch (consumer_index)
        {
        case 0:
            if (kind == EventKind::WorldWitnessBefore)
            {
                PublishWitness(kind, kConsumerSiteRvas[0], WitnessKind::MovementDirection,
                               *reinterpret_cast<const std::uint32_t*>(bytes + 0x1624), 0);
                PublishWitness(kind, kConsumerSiteRvas[0], WitnessKind::VelocityF32x2,
                               *reinterpret_cast<const std::uint32_t*>(bytes + 0x360),
                               *reinterpret_cast<const std::uint32_t*>(bytes + 0x364));
            }
            else
            {
                PublishWitness(kind, kConsumerSiteRvas[0], WitnessKind::VelocityF32x2,
                               *reinterpret_cast<const std::uint32_t*>(bytes + 0x360),
                               *reinterpret_cast<const std::uint32_t*>(bytes + 0x364));
                PublishWitness(kind, kConsumerSiteRvas[0], WitnessKind::MovementDirection,
                               *reinterpret_cast<const std::uint32_t*>(bytes + 0x1624), 0);
            }
            break;
        case 1:
            PublishWitness(kind, kConsumerSiteRvas[1], WitnessKind::BombIntentBit,
                           *reinterpret_cast<const std::uint32_t*>(bytes + 0x1378) & 1u, 0);
            break;
        case 4:
            if (kind == EventKind::WorldWitnessBefore)
            {
                PublishWitness(kind, kConsumerSiteRvas[4], WitnessKind::ShootingDirection,
                               *reinterpret_cast<const std::uint32_t*>(bytes + 0x162C), 0);
                PublishWitness(kind, kConsumerSiteRvas[4], WitnessKind::AimVector,
                               *reinterpret_cast<const std::uint32_t*>(bytes + 0x1630),
                               *reinterpret_cast<const std::uint32_t*>(bytes + 0x1634));
                PublishWitness(kind, kConsumerSiteRvas[4], WitnessKind::AimState,
                               *reinterpret_cast<const std::uint8_t*>(bytes + 0x1638), 0);
            }
            else
            {
                PublishWitness(kind, kConsumerSiteRvas[4], WitnessKind::AimState,
                               *reinterpret_cast<const std::uint8_t*>(bytes + 0x1638), 0);
                PublishWitness(kind, kConsumerSiteRvas[4], WitnessKind::AimVector,
                               *reinterpret_cast<const std::uint32_t*>(bytes + 0x1630),
                               *reinterpret_cast<const std::uint32_t*>(bytes + 0x1634));
                PublishWitness(kind, kConsumerSiteRvas[4], WitnessKind::ShootingDirection,
                               *reinterpret_cast<const std::uint32_t*>(bytes + 0x162C), 0);
            }
            break;
        case 5:
            PublishWitness(kind, kConsumerSiteRvas[5], WitnessKind::DropHoldCounter,
                           *reinterpret_cast<const std::uint32_t*>(bytes + 0x17C0), 0);
            break;
        default:
            break; // action 9/10 deliberately have no fabricated universal witness.
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        Redact();
    }
}

template <typename Invoke>
void InvokeConsumer(void* self, std::size_t index, Invoke&& invoke) noexcept
{
    HookScope scope;
    const ThreadContext previous = g_tls;
    std::uint32_t ordinal = kNoValue;
    std::uint32_t device_key = kNoValue;
    const bool identity = ReadPlayerIdentity(self, ordinal, device_key);
    const bool context_matches = identity && previous.player == self &&
                                 previous.player_ordinal == ordinal &&
                                 previous.device_key == device_key &&
                                 previous.context_generation != 0;
    bool observe = Armed() && context_matches;
    if (observe)
    {
        const auto loop_context = loop::CurrentObservationContext();
        if (!loop_context.main_thread_id || !loop_context.loop_iteration)
            observe = false;
        else if (loop_context.thread_id != loop_context.main_thread_id)
        {
            ReportUnsafe(Error::WrongThread);
            observe = false;
        }
    }
    if (!identity && Armed())
        Redact();
    if (observe)
    {
        g_tls.consumer_transaction = static_cast<std::uint64_t>(
            InterlockedIncrement64(&g_next_consumer_transaction));
        g_tls.consumer_site = kConsumerSiteRvas[index];
        auto enter = Event(EventKind::PlayerConsumerEnter);
        enter.hook_site_rva = kConsumerSiteRvas[index];
        enter.return_rva = kConsumerSiteRvas[index] + 5;
        enter.classification = static_cast<std::uint32_t>(
            Classification::PlayerConsumerObserved);
        FillContext(enter);
        Publish(enter);
        SnapshotWitness(self, index, EventKind::WorldWitnessBefore);
    }
    invoke();
    if (observe)
    {
        SnapshotWitness(self, index, EventKind::WorldWitnessAfter);
        auto exit = Event(EventKind::PlayerConsumerExit);
        exit.hook_site_rva = kConsumerSiteRvas[index];
        exit.return_rva = kConsumerSiteRvas[index] + 5;
        exit.original_calls = 1;
        exit.flags |= kFlagOriginalCalled;
        exit.classification = static_cast<std::uint32_t>(
            Classification::PlayerConsumerObserved);
        FillContext(exit);
        Publish(exit);
    }
    g_tls = previous;
}

extern "C" void __fastcall PlayerUpdateHook(void* self, void*)
{
    HookScope scope;
    const ThreadContext previous = g_tls;
    std::uint32_t ordinal = kNoValue;
    std::uint32_t device_key = kNoValue;
    DeviceKind sample_kind = DeviceKind::Unknown;
    std::uint64_t matched_sample = 0;
    bool observe = Armed() && ReadPlayerIdentity(self, ordinal, device_key);
    if (observe)
    {
        matched_sample = MatchingSample(device_key, &sample_kind);
        observe = matched_sample != 0;
    }
    NativeRawApplyEvent owner_enter{};
    if (observe)
    {
        owner_enter = Event(EventKind::ContextOwner);
        const auto context = loop::CurrentObservationContext();
        if (!context.main_thread_id || !context.loop_iteration)
            observe = false;
        else if (context.thread_id != context.main_thread_id)
        {
            ReportUnsafe(Error::WrongThread);
            observe = false;
        }
        if (observe)
        {
            g_tls.player = self;
            g_tls.player_ordinal = ordinal;
            g_tls.device_key = device_key;
            g_tls.sample_generation = matched_sample;
            g_tls.sample_device_kind = sample_kind;
            g_tls.context_generation = static_cast<std::uint64_t>(
                InterlockedIncrement64(&g_next_context_generation));
            g_tls.consumer_transaction = 0;
            owner_enter.hook_site_rva = kPlayerVtableSlotRva;
            owner_enter.return_rva = ReturnRva(_ReturnAddress());
            owner_enter.classification = static_cast<std::uint32_t>(
                Classification::PlayerConsumerObserved);
            if (!owner_enter.return_rva)
            {
                Redact();
                observe = false;
            }
            else
            {
                FillContext(owner_enter);
                Publish(owner_enter);
            }
        }
    }
    else if (Armed())
        Redact();
    g_player_update(self);
    if (observe)
    {
        auto owner_exit = owner_enter;
        owner_exit.qpc = Qpc();
        owner_exit.original_calls = 1;
        owner_exit.flags |= kFlagOriginalCalled;
        Publish(owner_exit);
    }
    g_tls = previous;
}

extern "C" void __fastcall MovementConsumerHook(void* self, void*)
{ InvokeConsumer(self, 0, [&]() noexcept { g_consumers[0](self); }); }
extern "C" void __fastcall BombConsumerHook(void* self, void*)
{ InvokeConsumer(self, 1, [&]() noexcept { g_consumers[1](self); }); }
extern "C" void __fastcall ItemConsumerHook(void* self, void*, std::int32_t slot)
{ InvokeConsumer(self, 2, [&]() noexcept { g_item_consumer(self, slot); }); }
extern "C" void __fastcall PillConsumerHook(void* self, void*)
{ InvokeConsumer(self, 3, [&]() noexcept { g_consumers[3](self); }); }
extern "C" void __fastcall ShootingConsumerHook(void* self, void*)
{ InvokeConsumer(self, 4, [&]() noexcept { g_consumers[4](self); }); }
extern "C" void __fastcall DropConsumerHook(void* self, void*)
{ InvokeConsumer(self, 5, [&]() noexcept { g_consumers[5](self); }); }

template <typename Function, typename Result>
Result InvokeLogical(Function original, void* self, std::uint32_t action,
                     EventKind enter_kind, EventKind exit_kind, Result false_value,
                     void* return_address) noexcept
{
    HookScope scope;
    DeviceKind device_kind = DeviceKind::Unknown;
    std::uint32_t key = kNoValue;
    NativeRawApplyEvent enter{};
    bool entered = false;
    if (action <= 11 && Device(self, device_kind, key) &&
        Eligible(enter, enter_kind, return_address))
    {
        enter.action = action;
        enter.device_key = key;
        enter.classification = static_cast<std::uint32_t>(Classification::ExplicitGameplay);
        FillDeviceFlags(enter, device_kind);
        FillContext(enter);
        if (g_tls.consumer_transaction && g_tls.device_key != key)
        {
            Redact();
        }
        else
        {
            Publish(enter);
            entered = true;
        }
    }
    else if (Armed() && action > 11)
        Redact();
    const Result result = original(self, action);
    if (entered)
    {
        auto exit = enter;
        exit.qpc = Qpc();
        exit.kind = static_cast<std::uint32_t>(exit_kind);
        exit.original_calls = 1;
        exit.flags |= kFlagOriginalCalled | kFlagValueValid;
        if constexpr (std::is_same_v<Result, bool>)
        {
            exit.value_bits = result ? 1u : 0u;
            if (result)
                exit.flags |= kFlagResultTrue;
        }
        else
        {
            exit.value_bits = FloatBits(result);
            if (result != false_value)
                exit.flags |= kFlagResultTrue;
        }
        Publish(exit);
    }
    return result;
}

extern "C" bool __fastcall RawKeyboardPressedHook(void* self, void*, std::uint32_t action)
{ return InvokeLogical(g_keyboard_pressed, self, action, EventKind::LogicalPressedEnter,
                       EventKind::LogicalPressedExit, false, _ReturnAddress()); }
extern "C" bool __fastcall RawKeyboardTriggeredHook(void* self, void*, std::uint32_t action)
{ return InvokeLogical(g_keyboard_triggered, self, action, EventKind::LogicalTriggeredEnter,
                       EventKind::LogicalTriggeredExit, false, _ReturnAddress()); }
extern "C" float __fastcall RawKeyboardScalarHook(void* self, void*, std::uint32_t action)
{ return InvokeLogical(g_keyboard_scalar, self, action, EventKind::LogicalScalarEnter,
                       EventKind::LogicalScalarExit, 0.0F, _ReturnAddress()); }
extern "C" bool __fastcall RawGamepadPressedHook(void* self, void*, std::uint32_t action)
{ return InvokeLogical(g_gamepad_pressed, self, action, EventKind::LogicalPressedEnter,
                       EventKind::LogicalPressedExit, false, _ReturnAddress()); }
extern "C" bool __fastcall RawGamepadTriggeredHook(void* self, void*, std::uint32_t action)
{ return InvokeLogical(g_gamepad_triggered, self, action, EventKind::LogicalTriggeredEnter,
                       EventKind::LogicalTriggeredExit, false, _ReturnAddress()); }
extern "C" float __fastcall RawGamepadScalarHook(void* self, void*, std::uint32_t action)
{ return InvokeLogical(g_gamepad_scalar, self, action, EventKind::LogicalScalarEnter,
                       EventKind::LogicalScalarExit, 0.0F, _ReturnAddress()); }

template <typename Function>
void InvokeSetter(Function original, void* self, std::uint32_t action, float value,
                  void* return_address) noexcept
{
    HookScope scope;
    DeviceKind device_kind = DeviceKind::Unknown;
    std::uint32_t key = kNoValue;
    NativeRawApplyEvent enter{};
    bool entered = false;
    if (action <= 11 && Device(self, device_kind, key) &&
        Eligible(enter, EventKind::LogicalSetterEnter, return_address))
    {
        enter.action = action;
        enter.device_key = key;
        enter.value_bits = FloatBits(value);
        enter.classification = static_cast<std::uint32_t>(
            Classification::LogicalOverrideDiagnostic);
        enter.flags |= kFlagValueValid;
        FillDeviceFlags(enter, device_kind);
        FillContext(enter);
        if (!g_tls.consumer_transaction || g_tls.device_key == key)
        {
            Publish(enter);
            entered = true;
        }
        else
            Redact();
    }
    else if (Armed())
        Redact();
    original(self, action, value);
    if (entered)
    {
        auto exit = enter;
        exit.qpc = Qpc();
        exit.kind = static_cast<std::uint32_t>(EventKind::LogicalSetterExit);
        exit.original_calls = 1;
        exit.flags |= kFlagOriginalCalled;
        Publish(exit);
    }
}
extern "C" void __fastcall RawKeyboardSetterHook(void* self, void*, std::uint32_t action,
                                                float value)
{ InvokeSetter(g_keyboard_setter, self, action, value, _ReturnAddress()); }
extern "C" void __fastcall RawGamepadSetterHook(void* self, void*, std::uint32_t action,
                                               float value)
{ InvokeSetter(g_gamepad_setter, self, action, value, _ReturnAddress()); }

extern "C" bool __fastcall RawManagerPressedHook(void* self, void*, std::uint32_t primitive,
                                                std::int32_t controller,
                                                std::int32_t* selected)
{
    HookScope scope;
    if (Armed()) Redact();
    return g_manager_pressed(self, primitive, controller, selected);
}
extern "C" bool __fastcall RawManagerTriggeredHook(void* self, void*, std::uint32_t primitive,
                                                  std::int32_t controller,
                                                  std::int32_t* selected)
{
    HookScope scope;
    if (Armed()) Redact();
    return g_manager_triggered(self, primitive, controller, selected);
}
extern "C" float __fastcall RawManagerScalarHook(void* self, void*, std::uint32_t primitive,
                                                std::int32_t controller,
                                                std::int32_t* selected)
{
    HookScope scope;
    if (Armed()) Redact();
    return g_manager_scalar(self, primitive, controller, selected);
}

void PhysicalPoll(bool entering, void* self, bool result, void* return_address,
                  void*) noexcept
{
    if (!Armed())
        return;
    HookScope scope;
    DeviceKind kind = DeviceKind::Unknown;
    std::uint32_t key = kNoValue;
    NativeRawApplyEvent event{};
    const auto event_kind = entering ? EventKind::RawPollEnter : EventKind::RawPollExit;
    if (!Device(self, kind, key) || !Eligible(event, event_kind, return_address))
    {
        Redact();
        return;
    }
    if (!detail::PhysicalProjectionContextEligible(event.return_rva, event.phase))
    {
        Redact();
        return;
    }
    const std::size_t index = kind == DeviceKind::Keyboard ? 0u : 1u;
    if (entering)
    {
        g_tls.poll_generation = static_cast<std::uint64_t>(
            InterlockedIncrement64(&g_next_sample_generation));
        InterlockedExchange64(&g_samples[index].generation, 0);
    }
    event.sample_generation = g_tls.poll_generation;
    event.hook_site_rva = index == 0 ? 0x007A16F0u : 0x00782CF8u;
    event.device_key = key;
    event.classification = static_cast<std::uint32_t>(Classification::RawDeviceOnly);
    event.raw_channel_kind = static_cast<std::uint32_t>(RawChannelKind::DevicePacketU32);
    event.flags |= kFlagSampleKnown;
    FillDeviceFlags(event, kind);
    if (!entering)
    {
        const std::uint64_t generation = g_tls.poll_generation;
        event.original_calls = 1;
        event.value_bits = result ? 1u : 0u;
        event.flags |= kFlagOriginalCalled | kFlagValueValid;
        if (result)
            event.flags |= kFlagResultTrue;
        InterlockedOr64(&g_shared->observed_raw_source_mask,
                        static_cast<LONG64>(1ull << index));
        Publish(event);
        if (result && generation != 0 &&
            PublishConcreteProjection(self, kind, key, generation,
                                      event.hook_site_rva, event.return_rva, event.phase))
        {
            InterlockedExchange(&g_samples[index].key, static_cast<LONG>(key));
            InterlockedExchange64(&g_samples[index].loop_iteration,
                                  static_cast<LONG64>(event.loop_iteration));
            InterlockedExchange(&g_samples[index].thread_id,
                                static_cast<LONG>(event.thread_id));
            InterlockedExchange64(&g_samples[index].generation,
                                  static_cast<LONG64>(generation));
        }
        else
        {
            g_projection_history[index].valid = false;
            Redact();
        }
        g_tls.poll_generation = 0;
        return;
    }
    Publish(event);
}

struct BindingSnapshot final {
    std::array<input::PrimitiveBinding, input::kMaximumLogicalActionCount> bindings{};
    std::size_t action_count = 0;
};

bool TakeBindingSnapshot(const std::uint8_t* bytes, BindingSnapshot& snapshot) noexcept
{
    const auto* begin = *reinterpret_cast<const std::uint8_t* const*>(bytes + 0x10);
    const auto* end = *reinterpret_cast<const std::uint8_t* const*>(bytes + 0x14);
    const std::uintptr_t begin_address = reinterpret_cast<std::uintptr_t>(begin);
    const std::uintptr_t end_address = reinterpret_cast<std::uintptr_t>(end);
    if (!begin || !end || end_address < begin_address)
        return false;
    const std::size_t binding_bytes = static_cast<std::size_t>(end_address - begin_address);
    if (binding_bytes < input::kGameplayActionCount * sizeof(input::PrimitiveBinding) ||
        binding_bytes % sizeof(input::PrimitiveBinding) != 0 ||
        binding_bytes > snapshot.bindings.size() * sizeof(input::PrimitiveBinding))
        return false;
    snapshot.action_count = binding_bytes / sizeof(input::PrimitiveBinding);
    // Schema-3 v3 projects explicit gameplay slots only.  The end pointer is used to
    // validate the fixed vector layout, but action12..31 bindings are neither read nor
    // classified: an outside-gameplay alias cannot change an action-indexed projection.
    std::memcpy(snapshot.bindings.data(), begin,
                input::kGameplayActionCount * sizeof(input::PrimitiveBinding));
    return true;
}

bool SameBindings(const BindingSnapshot& first, const BindingSnapshot& second) noexcept
{
    return detail::StableBindingsEqual(first.bindings, first.action_count,
                                       second.bindings, second.action_count);
}

bool TakeKeyboardSnapshot(
    const std::uint8_t* bytes, BindingSnapshot& bindings,
    std::array<std::uint8_t, detail::kKeyboardPrimitiveCount>& current,
    bool& active) noexcept
{
    if (!TakeBindingSnapshot(bytes, bindings))
        return false;
    active = (bytes[0x08] & 0x02u) != 0;
    const auto* source = g_main_base + kKeyboardCurrentStateRva;
    for (std::size_t action = 0; action < input::kGameplayActionCount; ++action)
    {
        for (std::uint32_t primitive : bindings.bindings[action])
        {
            if (primitive != input::kUnboundPrimitive && primitive < current.size())
                current[primitive] = source[primitive];
        }
    }
    return true;
}

bool ReadKeyboardProjection(
    void* self, std::array<float, input::kGameplayActionCount>& output,
    std::uint16_t& valid_actions) noexcept
{
    __try
    {
        const auto* bytes = static_cast<const std::uint8_t*>(self);
        BindingSnapshot bindings_first{};
        BindingSnapshot bindings_second{};
        std::array<std::uint8_t, detail::kKeyboardPrimitiveCount> current_first{};
        std::array<std::uint8_t, detail::kKeyboardPrimitiveCount> current_second{};
        bool active_first = false;
        bool active_second = false;
        if (!TakeKeyboardSnapshot(bytes, bindings_first, current_first, active_first))
            return false;
        MemoryBarrier();
        if (!TakeKeyboardSnapshot(bytes, bindings_second, current_second, active_second))
            return false;
        MemoryBarrier();
        if (!SameBindings(bindings_first, bindings_second) ||
            !detail::StableKeyboardStateEqual(active_first, current_first,
                                              active_second, current_second))
            return false;
        return detail::ProjectKeyboard(bindings_second.bindings.data(),
                                       bindings_second.action_count, active_second,
                                       current_second, output, valid_actions) ==
               detail::ProjectionStatus::Ok;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool TakeGamepadSnapshot(const std::uint8_t* bytes, BindingSnapshot& bindings,
                         detail::GamepadConcreteState& state) noexcept
{
    if (!TakeBindingSnapshot(bytes, bindings))
        return false;
    state.flags = *reinterpret_cast<const std::uint16_t*>(bytes + 0x08);
    state.button_low = *reinterpret_cast<const std::uint32_t*>(bytes + 0x20);
    state.button_high = *reinterpret_cast<const std::uint32_t*>(bytes + 0x24);
    std::memcpy(state.directional_axes.data(), bytes + 0x40,
                sizeof(state.directional_axes));
    state.deadzone = *reinterpret_cast<const float*>(bytes + 0xD0);
    std::memcpy(state.mappings.data(), bytes + 0x388, sizeof(state.mappings));
    return true;
}

bool ReadGamepadProjection(
    void* self, std::array<float, input::kGameplayActionCount>& output,
    std::uint16_t& valid_actions) noexcept
{
    static_assert(sizeof(detail::GamepadMapping) == 8);
    __try
    {
        const auto* bytes = static_cast<const std::uint8_t*>(self);
        BindingSnapshot bindings_first{};
        BindingSnapshot bindings_second{};
        detail::GamepadConcreteState state_first{};
        detail::GamepadConcreteState state_second{};
        if (!TakeGamepadSnapshot(bytes, bindings_first, state_first))
            return false;
        MemoryBarrier();
        if (!TakeGamepadSnapshot(bytes, bindings_second, state_second))
            return false;
        MemoryBarrier();
        if (!SameBindings(bindings_first, bindings_second) ||
            !detail::StableGamepadStateEqual(state_first, state_second))
            return false;
        return detail::ProjectGamepad(bindings_second.bindings.data(),
                                      bindings_second.action_count, state_second,
                                      output, valid_actions) ==
               detail::ProjectionStatus::Ok;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool PublishConcreteProjection(void* self, DeviceKind kind, std::uint32_t key,
                               std::uint64_t generation, std::uint32_t hook_site_rva,
                               std::uint32_t return_rva, std::uint32_t phase) noexcept
{
    if (!detail::PhysicalProjectionContextEligible(return_rva, phase) || !generation)
        return false;
    std::array<float, input::kGameplayActionCount> current{};
    std::uint16_t valid_actions = 0;
    const bool stable = kind == DeviceKind::Keyboard
                            ? ReadKeyboardProjection(self, current, valid_actions)
                            : kind == DeviceKind::Gamepad
                                  ? ReadGamepadProjection(self, current, valid_actions)
                                  : false;
    if (!stable || valid_actions == 0)
        return false;
    if (valid_actions != 0x0FFFu)
        Redact();
    const std::size_t index = kind == DeviceKind::Keyboard ? 0u : 1u;
    const detail::ProjectionEdgeMasks edges = detail::UpdateProjectionHistory(
        g_projection_history[index], key, current, valid_actions);
    for (std::uint32_t action = 0; action < input::kGameplayActionCount; ++action)
    {
        const std::uint16_t action_bit = static_cast<std::uint16_t>(1u << action);
        if ((valid_actions & action_bit) == 0)
            continue;
        auto event = Event(EventKind::RawValue);
        event.sample_generation = generation;
        event.hook_site_rva = hook_site_rva;
        event.return_rva = return_rva;
        event.original_calls = 1;
        event.device_key = key;
        event.action = action;
        event.raw_channel_kind = static_cast<std::uint32_t>(RawChannelKind::DevicePacketU32);
        event.raw_channel_id = action;
        event.value_bits = FloatBits(current[action]);
        event.classification = static_cast<std::uint32_t>(Classification::ExplicitGameplay);
        event.flags |= kFlagOriginalCalled | kFlagValueValid | kFlagSampleKnown;
        FillDeviceFlags(event, kind);
        if ((edges.rising & action_bit) != 0)
            event.flags |= kFlagRawRising;
        else if ((edges.falling & action_bit) != 0)
            event.flags |= kFlagRawFalling;
        if (!detail::ProjectionPayloadScopeValid(event.action, event.primitive))
        {
            Redact();
            continue;
        }
        Publish(event);
    }
    return true;
}

bool ValidMapping() noexcept
{
    if (!g_shared || g_shared->magic != kMagic ||
        g_shared->schema_version != kSchemaVersion ||
        g_shared->struct_size != sizeof(SharedRawApplyBlock) ||
        g_shared->target_pid != GetCurrentProcessId() || g_shared->probe != 1 ||
        g_shared->scenario == 0 || g_shared->generation == 0 ||
        !Nonzero(g_shared->nonce) || g_shared->target_sha256 != kTargetSha256 ||
        g_shared->loop_gate_sha256 != kLoopGateSha256 ||
        g_shared->catalog_sha256 != kCatalogSha256 ||
        g_shared->capture_plan_sha256 != kCapturePlanSha256 ||
        g_shared->loop_mapping_name[0] == L'\0' ||
        std::find(g_shared->loop_mapping_name.begin(), g_shared->loop_mapping_name.end(),
                  L'\0') == g_shared->loop_mapping_name.end())
        return false;
    return g_shared->required_raw_source_mask == kRawSourceMask &&
           g_shared->required_consumer_mask == kConsumerHookMask &&
           g_shared->legacy_installed_hooks_mask == 0 &&
           g_shared->legacy_restored_hooks_mask == 0 &&
           g_shared->observed_raw_source_mask == 0 &&
           g_shared->installed_consumer_mask == 0 &&
           g_shared->restored_consumer_mask == 0 && g_shared->next_sequence == 0 &&
           g_shared->controller_sequence == 0 && g_shared->overwritten_events == 0 &&
           g_shared->redacted_events == 0 && g_shared->active_hooks == 0 &&
           g_shared->producer_closed == 0 &&
           g_shared->state == static_cast<LONG>(NativeState::Empty);
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
        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(g_main_base);
        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(
            g_main_base + dos->e_lfanew);
        return dos->e_magic == IMAGE_DOS_SIGNATURE && nt->Signature == IMAGE_NT_SIGNATURE &&
               nt->FileHeader.Machine == IMAGE_FILE_MACHINE_I386 &&
               nt->FileHeader.TimeDateStamp == kExpectedTimestamp &&
               nt->OptionalHeader.SizeOfImage == kExpectedImageSize;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool PrepareTargets() noexcept
{
    __try
    {
        auto** manager = reinterpret_cast<void**>(g_main_base + kInputManagerVtableRva);
        auto** keyboard = reinterpret_cast<void**>(g_main_base + kKeyboardVtableRva);
        auto** gamepad = reinterpret_cast<void**>(g_main_base + kGamepadVtableRva);
        auto** player_slot = reinterpret_cast<void**>(g_main_base + kPlayerVtableSlotRva);
        if (*reinterpret_cast<void***>(g_main_base + kInputManagerObjectRva) != manager ||
            manager[-1] != g_main_base + kInputManagerColRva ||
            keyboard[-1] != g_main_base + kKeyboardColRva ||
            gamepad[-1] != g_main_base + kGamepadColRva ||
            *player_slot != g_main_base + kPlayerUpdateRva ||
            manager[0x40 / 4] != g_main_base + kManagerPressedRva ||
            manager[0x44 / 4] != g_main_base + kManagerTriggeredRva ||
            manager[0x48 / 4] != g_main_base + kManagerScalarRva)
            return false;
        for (auto** device : std::array<void**, 2>{{keyboard, gamepad}})
        {
            if (device[0x30 / 4] != g_main_base + kLogicalPressedRva ||
                device[0x34 / 4] != g_main_base + kLogicalTriggeredRva ||
                device[0x38 / 4] != g_main_base + kLogicalScalarRva ||
                device[0x4C / 4] != g_main_base + kLogicalSetterRva)
                return false;
        }
        for (std::size_t index = 0; index < kConsumerSiteRvas.size(); ++index)
        {
            const auto* call = g_main_base + kConsumerSiteRvas[index];
            if (std::memcmp(call, kConsumerCallBytes[index].data(), 5) != 0)
                return false;
            const auto displacement = *reinterpret_cast<const std::int32_t*>(call + 1);
            if (call + 5 + displacement != g_main_base + kConsumerTargetRvas[index])
                return false;
        }

        g_player_update = reinterpret_cast<ThisVoidFn>(g_main_base + kPlayerUpdateRva);
        for (std::size_t index = 0; index < g_consumers.size(); ++index)
            g_consumers[index] = reinterpret_cast<ThisVoidFn>(
                g_main_base + kConsumerTargetRvas[index]);
        g_item_consumer = reinterpret_cast<ThisArgVoidFn>(g_main_base + kConsumerTargetRvas[2]);
        g_manager_pressed = reinterpret_cast<ManagerBoolFn>(g_main_base + kManagerPressedRva);
        g_manager_triggered = reinterpret_cast<ManagerBoolFn>(g_main_base + kManagerTriggeredRva);
        g_manager_scalar = reinterpret_cast<ManagerScalarFn>(g_main_base + kManagerScalarRva);
        g_keyboard_pressed = reinterpret_cast<LogicalBoolFn>(g_main_base + kLogicalPressedRva);
        g_keyboard_triggered = reinterpret_cast<LogicalBoolFn>(g_main_base + kLogicalTriggeredRva);
        g_keyboard_scalar = reinterpret_cast<LogicalScalarFn>(g_main_base + kLogicalScalarRva);
        g_gamepad_pressed = g_keyboard_pressed;
        g_gamepad_triggered = g_keyboard_triggered;
        g_gamepad_scalar = g_keyboard_scalar;
        g_keyboard_setter = reinterpret_cast<LogicalSetterFn>(g_main_base + kLogicalSetterRva);
        g_gamepad_setter = g_keyboard_setter;

        const std::array<void*, 6> relative_replacements{{
            reinterpret_cast<void*>(&MovementConsumerHook),
            reinterpret_cast<void*>(&BombConsumerHook),
            reinterpret_cast<void*>(&ItemConsumerHook),
            reinterpret_cast<void*>(&PillConsumerHook),
            reinterpret_cast<void*>(&ShootingConsumerHook),
            reinterpret_cast<void*>(&DropConsumerHook),
        }};
        for (std::size_t index = 0; index < g_relative_patches.size(); ++index)
        {
            auto& patch = g_relative_patches[index];
            patch.call = g_main_base + kConsumerSiteRvas[index];
            std::copy(kConsumerCallBytes[index].begin(), kConsumerCallBytes[index].end(),
                      patch.expected);
            patch.replacement = relative_replacements[index];
        }
        const std::array<void**, 12> slots{{
            manager + 0x40 / 4, manager + 0x44 / 4, manager + 0x48 / 4,
            keyboard + 0x30 / 4, keyboard + 0x34 / 4, keyboard + 0x38 / 4,
            gamepad + 0x30 / 4, gamepad + 0x34 / 4, gamepad + 0x38 / 4,
            keyboard + 0x4C / 4, gamepad + 0x4C / 4, player_slot,
        }};
        const std::array<void*, 12> pointer_replacements{{
            reinterpret_cast<void*>(&RawManagerPressedHook),
            reinterpret_cast<void*>(&RawManagerTriggeredHook),
            reinterpret_cast<void*>(&RawManagerScalarHook),
            reinterpret_cast<void*>(&RawKeyboardPressedHook),
            reinterpret_cast<void*>(&RawKeyboardTriggeredHook),
            reinterpret_cast<void*>(&RawKeyboardScalarHook),
            reinterpret_cast<void*>(&RawGamepadPressedHook),
            reinterpret_cast<void*>(&RawGamepadTriggeredHook),
            reinterpret_cast<void*>(&RawGamepadScalarHook),
            reinterpret_cast<void*>(&RawKeyboardSetterHook),
            reinterpret_cast<void*>(&RawGamepadSetterHook),
            reinterpret_cast<void*>(&PlayerUpdateHook),
        }};
        for (std::size_t index = 0; index < g_pointer_patches.size(); ++index)
            g_pointer_patches[index] = {slots[index], pointer_replacements[index], nullptr,
                                        L"isaac-ng.exe", false};
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

void CloseMapping() noexcept
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
}

} // namespace

void SetModule(HMODULE module) noexcept { g_module = module; }

void ProcessDetach() noexcept
{
    CloseMapping();
    g_main_base = nullptr;
    g_module = nullptr;
}

bool Active() noexcept { return g_shared != nullptr; }

DWORD Initialize(const wchar_t* mapping_name) noexcept
{
    if (!g_module || !mapping_name || !*mapping_name || g_mapping || g_shared)
        return static_cast<DWORD>(Error::MappingInvalid);
    g_mapping = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, mapping_name);
    if (!g_mapping)
        return static_cast<DWORD>(Error::MappingInvalid);
    g_shared = static_cast<SharedRawApplyBlock*>(MapViewOfFile(
        g_mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedRawApplyBlock)));
    if (!g_shared || g_shared->magic != kMagic ||
        g_shared->schema_version != kSchemaVersion ||
        g_shared->struct_size != sizeof(SharedRawApplyBlock))
    {
        SetState(Error::MappingInvalid, NativeState::Failed);
        return static_cast<DWORD>(Error::MappingInvalid);
    }
    if (g_shared->catalog_sha256 != kCatalogSha256)
    {
        SetState(Error::CatalogMismatch, NativeState::Failed);
        return static_cast<DWORD>(Error::CatalogMismatch);
    }
    if (g_shared->capture_plan_sha256 != kCapturePlanSha256)
    {
        SetState(Error::CapturePlanMismatch, NativeState::Failed);
        return static_cast<DWORD>(Error::CapturePlanMismatch);
    }
    if (g_shared->target_sha256 != kTargetSha256)
    {
        SetState(Error::TargetMismatch, NativeState::Failed);
        return static_cast<DWORD>(Error::TargetMismatch);
    }
    if (g_shared->loop_gate_sha256 != kLoopGateSha256)
    {
        SetState(Error::CompanionLoopInvalid, NativeState::Failed);
        return static_cast<DWORD>(Error::CompanionLoopInvalid);
    }
    if (!ValidMapping())
    {
        SetState(Error::MappingInvalid, NativeState::Failed);
        return static_cast<DWORD>(Error::MappingInvalid);
    }
    InterlockedExchange(&g_unloading, 0);
    InterlockedExchange(&g_armed, 0);
    InterlockedExchange(&g_active_hooks, 0);
    InterlockedExchange(&g_unsafe_reported, 0);
    InterlockedExchange64(&g_next_sample_generation, 0);
    InterlockedExchange64(&g_next_consumer_transaction, 0);
    InterlockedExchange64(&g_next_context_generation, 0);
    for (auto& sample : g_samples)
    {
        InterlockedExchange(&sample.key, static_cast<LONG>(kNoValue));
        InterlockedExchange64(&sample.generation, 0);
        InterlockedExchange64(&sample.loop_iteration, 0);
        InterlockedExchange(&sample.thread_id, 0);
    }
    g_projection_history = {};
    InterlockedExchange(&g_shared->state, static_cast<LONG>(NativeState::Initializing));
    g_main_base = reinterpret_cast<std::uint8_t*>(GetModuleHandleW(nullptr));
    Publish(Event(EventKind::NativeInitialized, true));
    if (!IsTargetImage())
    {
        ReportUnsafe(Error::TargetMismatch);
        SetState(Error::TargetMismatch, NativeState::Failed);
        return static_cast<DWORD>(Error::TargetMismatch);
    }
    if (IsDebuggerPresent() != FALSE)
    {
        ReportUnsafe(Error::DebuggerPresent);
        SetState(Error::DebuggerPresent, NativeState::Failed);
        return static_cast<DWORD>(Error::DebuggerPresent);
    }
    if (!PrepareTargets())
    {
        ReportUnsafe(Error::HookTargetMismatch);
        SetState(Error::HookTargetMismatch, NativeState::Failed);
        return static_cast<DWORD>(Error::HookTargetMismatch);
    }
    loop::EmbeddedObserverIdentity identity{};
    identity.target_pid = g_shared->target_pid;
    identity.scenario = g_shared->scenario;
    identity.nonce = g_shared->nonce;
    loop::EmbeddedObserverHooks hooks{};
    hooks.relative = g_relative_patches.data();
    hooks.relative_count = g_relative_patches.size();
    hooks.pointers = g_pointer_patches.data();
    hooks.pointer_count = g_pointer_patches.size();
    hooks.physical_poll = &PhysicalPoll;
    hooks.device_update_complete = nullptr;
    const DWORD loop_result = loop::StartEmbeddedObserver(
        g_shared->loop_mapping_name.data(), hooks, identity);
    if (loop_result != 0)
    {
        const Error error = loop_result == kUnsafeResult ? Error::RestoreFailed
                                                         : Error::CompanionLoopInvalid;
        SetState(error, loop_result == kUnsafeResult ? NativeState::Unsafe
                                                     : NativeState::Failed);
        return loop_result == kUnsafeResult ? kUnsafeResult : static_cast<DWORD>(error);
    }
    InterlockedExchange64(&g_shared->legacy_installed_hooks_mask,
                          static_cast<LONG64>(kLegacyInputHookMask));
    InterlockedExchange64(&g_shared->installed_consumer_mask,
                          static_cast<LONG64>(kConsumerHookMask));
    Publish(Event(EventKind::HooksInstalled, true));
    InterlockedExchange(&g_armed, 1);
    InterlockedExchange(&g_shared->state, static_cast<LONG>(NativeState::Ready));
    return 0;
}

DWORD Shutdown() noexcept
{
    if (!g_shared || InterlockedExchange(&g_unloading, 1) != 0)
        return static_cast<DWORD>(Error::RestoreFailed);
    InterlockedExchange(&g_shared->state, static_cast<LONG>(NativeState::ShuttingDown));
    InterlockedExchange(&g_armed, 0);
    if (loop::RestoreEmbeddedInputHooks() != 0)
    {
        SetState(Error::RestoreFailed, NativeState::Unsafe);
        return static_cast<DWORD>(Error::RestoreFailed);
    }
    InterlockedExchange64(&g_shared->legacy_restored_hooks_mask,
                          static_cast<LONG64>(kLegacyInputHookMask));
    InterlockedExchange64(&g_shared->restored_consumer_mask,
                          static_cast<LONG64>(kConsumerHookMask));
    const ULONGLONG deadline = GetTickCount64() + kDrainTimeoutMs;
    while (InterlockedCompareExchange(&g_active_hooks, 0, 0) != 0 &&
           GetTickCount64() < deadline)
        Sleep(1);
    if (InterlockedCompareExchange(&g_active_hooks, 0, 0) != 0)
    {
        SetState(Error::RestoreFailed, NativeState::Unsafe);
        return static_cast<DWORD>(Error::RestoreFailed);
    }
    if (loop::StopEmbeddedObserver() != 0)
    {
        SetState(Error::RestoreFailed, NativeState::Unsafe);
        return static_cast<DWORD>(Error::RestoreFailed);
    }
    Publish(Event(EventKind::HooksRestored, true));
    Publish(Event(EventKind::NativeRetired, true));
    InterlockedExchange(&g_shared->producer_closed, 1);
    InterlockedExchange(&g_shared->state, static_cast<LONG>(NativeState::Stopped));
    return 0;
}

} // namespace isaac_cs_t0::raw_apply
