#include "isaac_cs_t0/input_runtime.hpp"

#include "isaac_cs_t0/hooks.hpp"
#include "isaac_cs_t0/input_core.hpp"
#include "isaac_cs_t0/input_ipc.hpp"
#include "isaac_cs_t0/loop_runtime.hpp"
#include "isaac_cs_t0/raw_apply_runtime.hpp"

#include <Windows.h>
#include <intrin.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace isaac_cs_t0::input {
namespace {

constexpr std::uint32_t kInputManagerObjectRva = 0x00857B18u;
constexpr std::uint32_t kExpectedTimestamp = 0x69E6E3A7u;
constexpr std::uint32_t kExpectedImageSize = 0x0092C000u;
constexpr std::uint32_t kInputManagerVtableRva = 0x00782950u;
constexpr std::uint32_t kInputManagerColRva = 0x007B0424u;
constexpr std::uint32_t kManagerPressedRva = 0x00620710u;
constexpr std::uint32_t kManagerTriggeredRva = 0x006207B0u;
constexpr std::uint32_t kManagerScalarRva = 0x00620850u;
constexpr std::uint32_t kKeyboardVtableRva = 0x007A1678u;
constexpr std::uint32_t kKeyboardColRva = 0x007B191Cu;
constexpr std::uint32_t kGamepadVtableRva = 0x00782C80u;
constexpr std::uint32_t kGamepadColRva = 0x007B0460u;
constexpr std::uint32_t kLogicalPressedRva = 0x0061E8D0u;
constexpr std::uint32_t kLogicalTriggeredRva = 0x0061E970u;
constexpr std::uint32_t kLogicalScalarRva = 0x0061EA20u;
constexpr std::uint32_t kLogicalSetterRva = 0x0061ED50u;
constexpr std::size_t kMaximumInputDevices = 32;
constexpr DWORD kHookDrainTimeoutMs = 5000;
constexpr DWORD kUnsafeInitializerResult = 8;

using LogicalBoolFn = bool(__thiscall*)(void*, std::uint32_t);
using LogicalScalarFn = float(__thiscall*)(void*, std::uint32_t);
using LogicalSetterFn = void(__thiscall*)(void*, std::uint32_t, float);
using ManagerBoolFn = bool(__thiscall*)(void*, std::uint32_t, std::int32_t, std::int32_t*);
using ManagerScalarFn = float(__thiscall*)(void*, std::uint32_t, std::int32_t,
                                           std::int32_t*);

enum class SnapshotStatus : std::uint32_t {
    Stable,
    Invalid,
    Unstable,
};

enum class DeviceKind : std::uint32_t {
    Unknown,
    Keyboard,
    Gamepad,
};

struct DeviceIdentity {
    void* object = nullptr;
    std::uint32_t key = kInputNoDeviceKey;
    DeviceKind kind = DeviceKind::Unknown;
};

struct SharedDeviceEntry {
    void* object = nullptr;
    void* owner = nullptr;
};

static_assert(sizeof(void*) == 4);
static_assert(sizeof(SharedDeviceEntry) == 8);

struct BindingSnapshot {
    std::array<PrimitiveBinding, kMaximumLogicalActionCount> bindings{};
    std::size_t action_count = 0;
};

HMODULE g_module = nullptr;
HANDLE g_mapping = nullptr;
SharedInputBlock* g_shared = nullptr;
std::uint8_t* g_main_base = nullptr;
volatile LONG g_armed = 0;
volatile LONG g_unloading = 0;
volatile LONG g_active_hooks = 0;
volatile LONG g_unsafe_reported = 0;

std::array<isaac_cs_t0::mode::PointerPatch, kInputHookCount> g_input_patches{};

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

extern "C" bool __fastcall ManagerPressedHook(void* self, void* unused,
                                                std::uint32_t primitive,
                                                std::int32_t controller_id,
                                                std::int32_t* selected_device_key);
extern "C" bool __fastcall ManagerTriggeredHook(void* self, void* unused,
                                                  std::uint32_t primitive,
                                                  std::int32_t controller_id,
                                                  std::int32_t* selected_device_key);
extern "C" float __fastcall ManagerScalarHook(void* self, void* unused,
                                                std::uint32_t primitive,
                                                std::int32_t controller_id,
                                                std::int32_t* selected_device_key);
extern "C" bool __fastcall KeyboardPressedHook(void* self, void* unused,
                                                 std::uint32_t action);
extern "C" bool __fastcall KeyboardTriggeredHook(void* self, void* unused,
                                                   std::uint32_t action);
extern "C" float __fastcall KeyboardScalarHook(void* self, void* unused,
                                                 std::uint32_t action);
extern "C" bool __fastcall GamepadPressedHook(void* self, void* unused,
                                                std::uint32_t action);
extern "C" bool __fastcall GamepadTriggeredHook(void* self, void* unused,
                                                  std::uint32_t action);
extern "C" float __fastcall GamepadScalarHook(void* self, void* unused,
                                                std::uint32_t action);
extern "C" void __fastcall KeyboardSetterHook(void* self, void* unused,
                                                std::uint32_t action, float value);
extern "C" void __fastcall GamepadSetterHook(void* self, void* unused,
                                               std::uint32_t action, float value);

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
    std::uint32_t bits = 0;
    static_assert(sizeof(bits) == sizeof(value));
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

template <std::size_t Size>
bool Nonzero(const std::array<std::uint8_t, Size>& bytes) noexcept
{
    return std::any_of(bytes.begin(), bytes.end(), [](std::uint8_t value) {
        return value != 0;
    });
}

void SetState(InputError error, InputNativeState state) noexcept
{
    if (!g_shared)
        return;
    InterlockedExchange(&g_shared->error_code, static_cast<LONG>(error));
    InterlockedExchange(&g_shared->state, static_cast<LONG>(state));
}

bool Armed() noexcept
{
    return g_shared && InterlockedCompareExchange(&g_armed, 0, 0) != 0 &&
           InterlockedCompareExchange(&g_unloading, 0, 0) == 0 &&
           InterlockedCompareExchange(&g_unsafe_reported, 0, 0) == 0;
}

std::uint32_t ReturnRva(void* address) noexcept
{
    return detail::BoundedTargetRva(g_main_base, kExpectedImageSize, address);
}

NativeInputEvent Event(InputEventKind kind, void* return_address = nullptr) noexcept
{
    NativeInputEvent event{};
    event.qpc = Qpc();
    event.thread_id = GetCurrentThreadId();
    event.kind = static_cast<std::uint32_t>(kind);
    event.return_rva = ReturnRva(return_address);
    const bool lifecycle = kind == InputEventKind::NativeInitialized ||
                           kind == InputEventKind::HooksInstalled ||
                           kind == InputEventKind::HooksRestored ||
                           kind == InputEventKind::NativeRetired ||
                           kind == InputEventKind::Failure;
    if (lifecycle)
    {
        event.phase = static_cast<std::uint32_t>(
            isaac_cs_t0::loop::Phase::OutsideEnvelope);
        return event;
    }
    const auto context = isaac_cs_t0::loop::CurrentObservationContext();
    event.loop_iteration = context.loop_iteration;
    event.phase = static_cast<std::uint32_t>(context.phase);
    if (context.loop_iteration != 0)
        event.flags |= kInputFlagLoopKnown;
    if (context.main_thread_id != 0 && context.main_thread_id == context.thread_id)
        event.flags |= kInputFlagMainThread;
    else if (context.main_thread_id != 0)
        event.flags |= kInputFlagWrongThread;
    return event;
}

void Publish(const NativeInputEvent& event) noexcept
{
    if (!g_shared)
        return;
    const LONG64 sequence = InterlockedIncrement64(&g_shared->next_sequence);
    const LONG64 consumed = InterlockedCompareExchange64(&g_shared->controller_sequence, 0, 0);
    if (sequence - consumed > static_cast<LONG64>(kInputEventCapacity))
    {
        InterlockedIncrement64(&g_shared->overwritten_events);
        InterlockedExchange(&g_armed, 0);
        InterlockedExchange(&g_unsafe_reported, 1);
        SetState(InputError::RingOverflow, InputNativeState::Unsafe);
    }
    NativeInputEvent& slot =
        g_shared->events[static_cast<std::size_t>(sequence - 1) % kInputEventCapacity];
    InterlockedExchange64(&slot.published_sequence, 0);
    CopyInputEventPayload(slot, event);
    MemoryBarrier();
    InterlockedExchange64(&slot.published_sequence, sequence);
}

void PublishFailure(InputError error) noexcept
{
    NativeInputEvent event = Event(InputEventKind::Failure);
    event.error_code = static_cast<std::uint32_t>(error);
    Publish(event);
}

void ReportUnsafe(InputError error) noexcept
{
    if (!g_shared || InterlockedExchange(&g_unsafe_reported, 1) != 0)
        return;
    InterlockedExchange(&g_armed, 0);
    PublishFailure(error);
    SetState(error, InputNativeState::Unsafe);
}

void Redact() noexcept
{
    if (g_shared)
        InterlockedIncrement64(&g_shared->redacted_events);
}

bool BeginObservation(NativeInputEvent& event, InputEventKind kind,
                      void* return_address) noexcept
{
    if (!Armed())
        return false;
    event = Event(kind, return_address);
    const auto context = isaac_cs_t0::loop::CurrentObservationContext();
    if (context.main_thread_id == 0 || context.loop_iteration == 0)
        return false;
    if (context.thread_id != context.main_thread_id)
    {
        ReportUnsafe(InputError::WrongThread);
        return false;
    }
    if (context.phase == isaac_cs_t0::loop::Phase::OutsideEnvelope)
    {
        Redact();
        return false;
    }
    if (event.return_rva == 0)
    {
        Redact();
        return false;
    }
    InterlockedExchange(&g_shared->main_thread_id, static_cast<LONG>(context.main_thread_id));
    InterlockedExchange64(&g_shared->current_loop_iteration,
                          static_cast<LONG64>(context.loop_iteration));
    return Armed();
}

bool ReadDeviceIdentityRaw(void* object, DeviceIdentity& identity) noexcept
{
    __try
    {
        if (!object)
            return false;
        void* const vtable = *reinterpret_cast<void**>(object);
        DeviceKind kind = DeviceKind::Unknown;
        if (vtable == g_main_base + kKeyboardVtableRva)
            kind = DeviceKind::Keyboard;
        else if (vtable == g_main_base + kGamepadVtableRva)
            kind = DeviceKind::Gamepad;
        else
            return false;
        identity.object = object;
        identity.key = *reinterpret_cast<const std::uint32_t*>(
            static_cast<const std::uint8_t*>(object) + 0x0C);
        identity.kind = kind;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool ReadPointerRangeRaw(void* object, std::size_t begin_offset, std::size_t end_offset,
                         const std::uint8_t*& begin, const std::uint8_t*& end) noexcept
{
    __try
    {
        const auto* bytes = static_cast<const std::uint8_t*>(object);
        begin = *reinterpret_cast<const std::uint8_t* const*>(bytes + begin_offset);
        end = *reinterpret_cast<const std::uint8_t* const*>(bytes + end_offset);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        begin = nullptr;
        end = nullptr;
        return false;
    }
}

bool CopyRaw(const void* source, void* destination, std::size_t size) noexcept
{
    __try
    {
        std::memcpy(destination, source, size);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool ReadSelectedKeyRaw(const std::int32_t* selected, std::int32_t& value) noexcept
{
    __try
    {
        value = *selected;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

SnapshotStatus ReadBindingSnapshot(void* object, BindingSnapshot& output) noexcept
{
    const std::uint8_t* begin1 = nullptr;
    const std::uint8_t* end1 = nullptr;
    if (!ReadPointerRangeRaw(object, 0x10, 0x14, begin1, end1) || !begin1 || !end1 ||
        reinterpret_cast<std::uintptr_t>(end1) <
            reinterpret_cast<std::uintptr_t>(begin1))
        return SnapshotStatus::Invalid;
    const std::size_t bytes = static_cast<std::size_t>(
        reinterpret_cast<std::uintptr_t>(end1) -
        reinterpret_cast<std::uintptr_t>(begin1));
    if (bytes < kGameplayActionCount * sizeof(PrimitiveBinding) ||
        bytes % sizeof(PrimitiveBinding) != 0 ||
        bytes > kMaximumLogicalActionCount * sizeof(PrimitiveBinding))
        return SnapshotStatus::Invalid;
    const std::size_t action_count = bytes / sizeof(PrimitiveBinding);
    BindingSnapshot first{};
    BindingSnapshot second{};
    first.action_count = action_count;
    second.action_count = action_count;
    if (!CopyRaw(begin1, first.bindings.data(), bytes))
        return SnapshotStatus::Invalid;
    MemoryBarrier();
    const std::uint8_t* begin2 = nullptr;
    const std::uint8_t* end2 = nullptr;
    if (!ReadPointerRangeRaw(object, 0x10, 0x14, begin2, end2) || begin2 != begin1 ||
        end2 != end1)
        return SnapshotStatus::Unstable;
    if (!CopyRaw(begin2, second.bindings.data(), bytes))
        return SnapshotStatus::Invalid;
    MemoryBarrier();
    const std::uint8_t* begin3 = nullptr;
    const std::uint8_t* end3 = nullptr;
    if (!ReadPointerRangeRaw(object, 0x10, 0x14, begin3, end3) || begin3 != begin1 ||
        end3 != end1 || std::memcmp(first.bindings.data(), second.bindings.data(), bytes) != 0)
        return SnapshotStatus::Unstable;
    output = second;
    return SnapshotStatus::Stable;
}

SnapshotStatus ReadDeviceVector(void* manager,
                                std::array<SharedDeviceEntry, kMaximumInputDevices>& entries,
                                std::size_t& count) noexcept
{
    count = 0;
    if (manager != g_main_base + kInputManagerObjectRva)
        return SnapshotStatus::Invalid;
    const std::uint8_t* begin1 = nullptr;
    const std::uint8_t* end1 = nullptr;
    if (!ReadPointerRangeRaw(manager, 0x08, 0x0C, begin1, end1) || !begin1 || !end1 ||
        reinterpret_cast<std::uintptr_t>(end1) <
            reinterpret_cast<std::uintptr_t>(begin1))
        return SnapshotStatus::Invalid;
    const std::size_t bytes = static_cast<std::size_t>(
        reinterpret_cast<std::uintptr_t>(end1) -
        reinterpret_cast<std::uintptr_t>(begin1));
    if (bytes == 0 || bytes % sizeof(SharedDeviceEntry) != 0 ||
        bytes > entries.size() * sizeof(SharedDeviceEntry))
        return SnapshotStatus::Invalid;
    std::array<SharedDeviceEntry, kMaximumInputDevices> first{};
    std::array<SharedDeviceEntry, kMaximumInputDevices> second{};
    if (!CopyRaw(begin1, first.data(), bytes))
        return SnapshotStatus::Invalid;
    MemoryBarrier();
    const std::uint8_t* begin2 = nullptr;
    const std::uint8_t* end2 = nullptr;
    if (!ReadPointerRangeRaw(manager, 0x08, 0x0C, begin2, end2) || begin2 != begin1 ||
        end2 != end1)
        return SnapshotStatus::Unstable;
    if (!CopyRaw(begin2, second.data(), bytes))
        return SnapshotStatus::Invalid;
    MemoryBarrier();
    const std::uint8_t* begin3 = nullptr;
    const std::uint8_t* end3 = nullptr;
    if (!ReadPointerRangeRaw(manager, 0x08, 0x0C, begin3, end3) || begin3 != begin1 ||
        end3 != end1 || std::memcmp(first.data(), second.data(), bytes) != 0)
        return SnapshotStatus::Unstable;
    entries = second;
    count = bytes / sizeof(SharedDeviceEntry);
    return SnapshotStatus::Stable;
}

SnapshotStatus FindDevice(void* manager, bool has_requested_key, std::uint32_t requested_key,
                          DeviceIdentity& output) noexcept
{
    std::array<SharedDeviceEntry, kMaximumInputDevices> entries{};
    std::size_t count = 0;
    const SnapshotStatus vector_status = ReadDeviceVector(manager, entries, count);
    if (vector_status != SnapshotStatus::Stable)
        return vector_status;
    std::size_t matches = 0;
    for (std::size_t index = 0; index < count; ++index)
    {
        DeviceIdentity candidate{};
        if (!ReadDeviceIdentityRaw(entries[index].object, candidate))
            return SnapshotStatus::Invalid;
        if (!has_requested_key || candidate.key == requested_key)
        {
            output = candidate;
            ++matches;
        }
    }
    return matches == 1 ? SnapshotStatus::Stable : SnapshotStatus::Invalid;
}

void FillDevice(NativeInputEvent& event, const DeviceIdentity& identity) noexcept
{
    event.device_key = identity.key;
    if (identity.kind == DeviceKind::Keyboard)
        event.flags |= kInputFlagKeyboardDevice;
    else if (identity.kind == DeviceKind::Gamepad)
        event.flags |= kInputFlagGamepadDevice;
}

InputClassification ClassifyManagerPrimitive(void* device, std::uint32_t primitive,
                                             std::uint32_t& action) noexcept
{
    BindingSnapshot snapshot{};
    const SnapshotStatus status = ReadBindingSnapshot(device, snapshot);
    if (status == SnapshotStatus::Invalid)
        return InputClassification::InvalidLayout;
    if (status == SnapshotStatus::Unstable)
        return InputClassification::UnstableSnapshot;
    const PrimitiveActionClassification classification = ClassifyPrimitiveAction(
        primitive, snapshot.bindings.data(), snapshot.action_count);
    switch (classification.classification)
    {
    case PrimitiveActionClass::UniqueGameplay:
        action = classification.action;
        return InputClassification::UniqueGameplay;
    case PrimitiveActionClass::NoMatch:
        return InputClassification::NoMatch;
    case PrimitiveActionClass::AmbiguousOrOutsideGameplay:
        return InputClassification::AmbiguousOrOutsideGameplay;
    case PrimitiveActionClass::InvalidLayout:
        return InputClassification::InvalidLayout;
    }
    return InputClassification::InvalidLayout;
}

template <typename Function>
bool InvokeLogicalBool(Function original, void* self, std::uint32_t action,
                       InputEventKind enter_kind, InputEventKind exit_kind,
                       void* return_address) noexcept
{
    HookScope scope;
    NativeInputEvent enter{};
    const bool observe = BeginObservation(enter, enter_kind, return_address);
    DeviceIdentity identity{};
    const bool eligible = observe && action < kGameplayActionCount &&
                          ReadDeviceIdentityRaw(self, identity);
    if (observe && !eligible)
        Redact();
    if (eligible)
    {
        FillDevice(enter, identity);
        enter.action = action;
        enter.classification =
            static_cast<std::uint32_t>(InputClassification::ExplicitGameplay);
        Publish(enter);
    }
    const bool result = original(self, action);
    if (eligible && Armed())
    {
        NativeInputEvent exit = Event(exit_kind, return_address);
        FillDevice(exit, identity);
        exit.action = action;
        exit.classification =
            static_cast<std::uint32_t>(InputClassification::ExplicitGameplay);
        exit.original_calls = 1;
        exit.flags |= kInputFlagOriginalCalled | kInputFlagValueValid;
        exit.value_bits = result ? 1u : 0u;
        if (result)
            exit.flags |= kInputFlagResultTrue;
        Publish(exit);
    }
    return result;
}

template <typename Function>
float InvokeLogicalScalar(Function original, void* self, std::uint32_t action,
                          InputEventKind enter_kind, InputEventKind exit_kind,
                          void* return_address) noexcept
{
    HookScope scope;
    NativeInputEvent enter{};
    const bool observe = BeginObservation(enter, enter_kind, return_address);
    DeviceIdentity identity{};
    const bool eligible = observe && action < kGameplayActionCount &&
                          ReadDeviceIdentityRaw(self, identity);
    if (observe && !eligible)
        Redact();
    if (eligible)
    {
        FillDevice(enter, identity);
        enter.action = action;
        enter.classification =
            static_cast<std::uint32_t>(InputClassification::ExplicitGameplay);
        Publish(enter);
    }
    const float result = original(self, action);
    if (eligible && Armed())
    {
        NativeInputEvent exit = Event(exit_kind, return_address);
        FillDevice(exit, identity);
        exit.action = action;
        exit.classification =
            static_cast<std::uint32_t>(InputClassification::ExplicitGameplay);
        exit.original_calls = 1;
        exit.flags |= kInputFlagOriginalCalled | kInputFlagValueValid;
        exit.value_bits = FloatBits(result);
        if (result != 0.0F)
            exit.flags |= kInputFlagResultTrue;
        Publish(exit);
    }
    return result;
}

template <typename Result>
void PublishManagerAfterOriginal(Result& result, void* self,
                                 std::uint32_t primitive, std::int32_t controller_id,
                                 std::int32_t* selected_device_key,
                                 InputEventKind exit_kind, void* return_address,
                                 NativeInputEvent& enter, bool observe) noexcept
{
    if (!observe || !Armed())
        return;
    bool result_matched = false;
    if constexpr (std::is_same_v<Result, bool>)
        result_matched = result;
    else
        result_matched = result != 0.0F;
    std::int32_t selected_raw = kInputNoControllerId;
    const bool selected_readable =
        !result_matched || !selected_device_key ||
        ReadSelectedKeyRaw(selected_device_key, selected_raw);
    if (!selected_readable)
    {
        Redact();
        return;
    }
    const bool selected_valid = result_matched && selected_device_key && selected_raw >= 0;
    const bool controller_valid = controller_id >= 0;
    const bool has_requested_key = selected_valid || controller_valid;
    const std::uint32_t requested_key = selected_valid
                                            ? static_cast<std::uint32_t>(selected_raw)
                                            : static_cast<std::uint32_t>(controller_id);
    DeviceIdentity identity{};
    const SnapshotStatus device_status =
        FindDevice(self, has_requested_key, requested_key, identity);
    if (device_status != SnapshotStatus::Stable)
    {
        Redact();
        return;
    }
    std::uint32_t action = kInputNoAction;
    const InputClassification classification =
        ClassifyManagerPrimitive(identity.object, primitive, action);
    if (classification != InputClassification::UniqueGameplay)
    {
        Redact();
        return;
    }
    FillDevice(enter, identity);
    enter.controller_id = controller_id;
    enter.selected_device_key = selected_valid ? static_cast<std::uint32_t>(selected_raw)
                                                : kInputNoDeviceKey;
    enter.action = action;
    enter.primitive = primitive;
    enter.classification = static_cast<std::uint32_t>(classification);
    Publish(enter);

    NativeInputEvent exit = Event(exit_kind, return_address);
    FillDevice(exit, identity);
    exit.controller_id = controller_id;
    exit.selected_device_key = enter.selected_device_key;
    exit.action = action;
    exit.primitive = primitive;
    exit.classification = static_cast<std::uint32_t>(classification);
    exit.original_calls = 1;
    exit.flags |= kInputFlagOriginalCalled | kInputFlagValueValid;
    if constexpr (std::is_same_v<Result, bool>)
    {
        exit.value_bits = result ? 1u : 0u;
        if (result)
            exit.flags |= kInputFlagResultTrue;
    }
    else
    {
        exit.value_bits = FloatBits(result);
        if (result != 0.0F)
            exit.flags |= kInputFlagResultTrue;
    }
    Publish(exit);
}

bool InvokeManagerBool(ManagerBoolFn original, void* self, std::uint32_t primitive,
                       std::int32_t controller_id, std::int32_t* selected_device_key,
                       InputEventKind enter_kind, InputEventKind exit_kind,
                       void* return_address) noexcept
{
    HookScope scope;
    NativeInputEvent enter{};
    const bool observe = BeginObservation(enter, enter_kind, return_address);
    const bool result = original(self, primitive, controller_id, selected_device_key);
    PublishManagerAfterOriginal(result, self, primitive, controller_id, selected_device_key,
                                exit_kind, return_address, enter, observe);
    return result;
}

float InvokeManagerScalar(ManagerScalarFn original, void* self, std::uint32_t primitive,
                          std::int32_t controller_id, std::int32_t* selected_device_key,
                          InputEventKind enter_kind, InputEventKind exit_kind,
                          void* return_address) noexcept
{
    HookScope scope;
    NativeInputEvent enter{};
    const bool observe = BeginObservation(enter, enter_kind, return_address);
    const float result = original(self, primitive, controller_id, selected_device_key);
    PublishManagerAfterOriginal(result, self, primitive, controller_id, selected_device_key,
                                exit_kind, return_address, enter, observe);
    return result;
}

void InvokeSetter(LogicalSetterFn original, void* self, std::uint32_t action, float value,
                  void* return_address) noexcept
{
    HookScope scope;
    NativeInputEvent enter{};
    const bool observe =
        BeginObservation(enter, InputEventKind::LogicalSetterEnter, return_address);
    DeviceIdentity identity{};
    const bool eligible = observe && action < kGameplayActionCount &&
                          ReadDeviceIdentityRaw(self, identity);
    if (observe && !eligible)
        Redact();
    if (eligible)
    {
        FillDevice(enter, identity);
        enter.action = action;
        enter.value_bits = FloatBits(value);
        enter.flags |= kInputFlagValueValid;
        enter.classification =
            static_cast<std::uint32_t>(InputClassification::LogicalOverrideDiagnostic);
        Publish(enter);
    }
    original(self, action, value);
    if (eligible && Armed())
    {
        NativeInputEvent exit = Event(InputEventKind::LogicalSetterExit, return_address);
        FillDevice(exit, identity);
        exit.action = action;
        exit.value_bits = FloatBits(value);
        exit.flags |= kInputFlagOriginalCalled | kInputFlagValueValid;
        exit.original_calls = 1;
        exit.classification =
            static_cast<std::uint32_t>(InputClassification::LogicalOverrideDiagnostic);
        Publish(exit);
    }
}

extern "C" bool __fastcall ManagerPressedHook(void* self, void*, std::uint32_t primitive,
                                                std::int32_t controller_id,
                                                std::int32_t* selected_device_key)
{
    return InvokeManagerBool(g_manager_pressed, self, primitive, controller_id,
                             selected_device_key, InputEventKind::ManagerPressedEnter,
                             InputEventKind::ManagerPressedExit, _ReturnAddress());
}

extern "C" bool __fastcall ManagerTriggeredHook(void* self, void*, std::uint32_t primitive,
                                                  std::int32_t controller_id,
                                                  std::int32_t* selected_device_key)
{
    return InvokeManagerBool(g_manager_triggered, self, primitive, controller_id,
                             selected_device_key, InputEventKind::ManagerTriggeredEnter,
                             InputEventKind::ManagerTriggeredExit, _ReturnAddress());
}

extern "C" float __fastcall ManagerScalarHook(void* self, void*, std::uint32_t primitive,
                                                std::int32_t controller_id,
                                                std::int32_t* selected_device_key)
{
    return InvokeManagerScalar(g_manager_scalar, self, primitive, controller_id,
                               selected_device_key, InputEventKind::ManagerScalarEnter,
                               InputEventKind::ManagerScalarExit, _ReturnAddress());
}

extern "C" bool __fastcall KeyboardPressedHook(void* self, void*, std::uint32_t action)
{
    return InvokeLogicalBool(g_keyboard_pressed, self, action,
                             InputEventKind::LogicalPressedEnter,
                             InputEventKind::LogicalPressedExit, _ReturnAddress());
}

extern "C" bool __fastcall KeyboardTriggeredHook(void* self, void*, std::uint32_t action)
{
    return InvokeLogicalBool(g_keyboard_triggered, self, action,
                             InputEventKind::LogicalTriggeredEnter,
                             InputEventKind::LogicalTriggeredExit, _ReturnAddress());
}

extern "C" float __fastcall KeyboardScalarHook(void* self, void*, std::uint32_t action)
{
    return InvokeLogicalScalar(g_keyboard_scalar, self, action,
                               InputEventKind::LogicalScalarEnter,
                               InputEventKind::LogicalScalarExit, _ReturnAddress());
}

extern "C" bool __fastcall GamepadPressedHook(void* self, void*, std::uint32_t action)
{
    return InvokeLogicalBool(g_gamepad_pressed, self, action,
                             InputEventKind::LogicalPressedEnter,
                             InputEventKind::LogicalPressedExit, _ReturnAddress());
}

extern "C" bool __fastcall GamepadTriggeredHook(void* self, void*, std::uint32_t action)
{
    return InvokeLogicalBool(g_gamepad_triggered, self, action,
                             InputEventKind::LogicalTriggeredEnter,
                             InputEventKind::LogicalTriggeredExit, _ReturnAddress());
}

extern "C" float __fastcall GamepadScalarHook(void* self, void*, std::uint32_t action)
{
    return InvokeLogicalScalar(g_gamepad_scalar, self, action,
                               InputEventKind::LogicalScalarEnter,
                               InputEventKind::LogicalScalarExit, _ReturnAddress());
}

extern "C" void __fastcall KeyboardSetterHook(void* self, void*, std::uint32_t action,
                                                float value)
{
    InvokeSetter(g_keyboard_setter, self, action, value, _ReturnAddress());
}

extern "C" void __fastcall GamepadSetterHook(void* self, void*, std::uint32_t action,
                                               float value)
{
    InvokeSetter(g_gamepad_setter, self, action, value, _ReturnAddress());
}

bool ValidMapping() noexcept
{
    if (!g_shared || g_shared->magic != kInputIpcMagic ||
        g_shared->schema_version != kInputIpcSchemaVersion ||
        g_shared->struct_size != sizeof(SharedInputBlock) ||
        g_shared->target_pid != GetCurrentProcessId() || g_shared->scenario < 1 ||
        g_shared->scenario > 10 || g_shared->generation == 0 ||
        !Nonzero(g_shared->nonce) || !Nonzero(g_shared->target_sha256) ||
        !Nonzero(g_shared->loop_gate_sha256) || g_shared->loop_mapping_name[0] == L'\0')
        return false;
    if (std::find(g_shared->loop_mapping_name.begin(), g_shared->loop_mapping_name.end(),
                  L'\0') == g_shared->loop_mapping_name.end())
        return false;
    return InterlockedCompareExchange(&g_shared->state, 0, 0) ==
               static_cast<LONG>(InputNativeState::Empty) &&
           InterlockedCompareExchange64(&g_shared->installed_hooks_mask, 0, 0) == 0 &&
           InterlockedCompareExchange64(&g_shared->restored_hooks_mask, 0, 0) == 0 &&
           InterlockedCompareExchange64(&g_shared->next_sequence, 0, 0) == 0 &&
           InterlockedCompareExchange64(&g_shared->controller_sequence, 0, 0) == 0 &&
           InterlockedCompareExchange64(&g_shared->overwritten_events, 0, 0) == 0 &&
           InterlockedCompareExchange64(&g_shared->redacted_events, 0, 0) == 0 &&
           InterlockedCompareExchange(&g_shared->active_hooks, 0, 0) == 0 &&
           InterlockedCompareExchange(&g_shared->producer_closed, 0, 0) == 0;
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

bool ValidateAndPrepareTargets() noexcept
{
    __try
    {
        auto** const manager = reinterpret_cast<void**>(g_main_base + kInputManagerVtableRva);
        auto** const keyboard = reinterpret_cast<void**>(g_main_base + kKeyboardVtableRva);
        auto** const gamepad = reinterpret_cast<void**>(g_main_base + kGamepadVtableRva);
        if (*reinterpret_cast<void***>(g_main_base + kInputManagerObjectRva) != manager ||
            manager[-1] != g_main_base + kInputManagerColRva ||
            keyboard[-1] != g_main_base + kKeyboardColRva ||
            gamepad[-1] != g_main_base + kGamepadColRva ||
            manager[0x40 / sizeof(void*)] != g_main_base + kManagerPressedRva ||
            manager[0x44 / sizeof(void*)] != g_main_base + kManagerTriggeredRva ||
            manager[0x48 / sizeof(void*)] != g_main_base + kManagerScalarRva)
            return false;
        const std::array<void**, 2> devices{{keyboard, gamepad}};
        for (void** device : devices)
        {
            if (device[0x30 / sizeof(void*)] != g_main_base + kLogicalPressedRva ||
                device[0x34 / sizeof(void*)] != g_main_base + kLogicalTriggeredRva ||
                device[0x38 / sizeof(void*)] != g_main_base + kLogicalScalarRva ||
                device[0x4C / sizeof(void*)] != g_main_base + kLogicalSetterRva)
                return false;
        }

        g_manager_pressed = reinterpret_cast<ManagerBoolFn>(g_main_base + kManagerPressedRva);
        g_manager_triggered =
            reinterpret_cast<ManagerBoolFn>(g_main_base + kManagerTriggeredRva);
        g_manager_scalar = reinterpret_cast<ManagerScalarFn>(g_main_base + kManagerScalarRva);
        g_keyboard_pressed = reinterpret_cast<LogicalBoolFn>(g_main_base + kLogicalPressedRva);
        g_keyboard_triggered =
            reinterpret_cast<LogicalBoolFn>(g_main_base + kLogicalTriggeredRva);
        g_keyboard_scalar =
            reinterpret_cast<LogicalScalarFn>(g_main_base + kLogicalScalarRva);
        g_gamepad_pressed = g_keyboard_pressed;
        g_gamepad_triggered = g_keyboard_triggered;
        g_gamepad_scalar = g_keyboard_scalar;
        g_keyboard_setter =
            reinterpret_cast<LogicalSetterFn>(g_main_base + kLogicalSetterRva);
        g_gamepad_setter = g_keyboard_setter;

        const std::array<void**, kInputHookCount> slots{{
            manager + 0x40 / sizeof(void*), manager + 0x44 / sizeof(void*),
            manager + 0x48 / sizeof(void*), keyboard + 0x30 / sizeof(void*),
            keyboard + 0x34 / sizeof(void*), keyboard + 0x38 / sizeof(void*),
            gamepad + 0x30 / sizeof(void*), gamepad + 0x34 / sizeof(void*),
            gamepad + 0x38 / sizeof(void*), keyboard + 0x4C / sizeof(void*),
            gamepad + 0x4C / sizeof(void*),
        }};
        const std::array<void*, kInputHookCount> replacements{{
            reinterpret_cast<void*>(&ManagerPressedHook),
            reinterpret_cast<void*>(&ManagerTriggeredHook),
            reinterpret_cast<void*>(&ManagerScalarHook),
            reinterpret_cast<void*>(&KeyboardPressedHook),
            reinterpret_cast<void*>(&KeyboardTriggeredHook),
            reinterpret_cast<void*>(&KeyboardScalarHook),
            reinterpret_cast<void*>(&GamepadPressedHook),
            reinterpret_cast<void*>(&GamepadTriggeredHook),
            reinterpret_cast<void*>(&GamepadScalarHook),
            reinterpret_cast<void*>(&KeyboardSetterHook),
            reinterpret_cast<void*>(&GamepadSetterHook),
        }};
        for (std::size_t index = 0; index < g_input_patches.size(); ++index)
            g_input_patches[index] = {slots[index], replacements[index], nullptr,
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

InputError MapCompanionError(DWORD error) noexcept
{
    switch (error)
    {
    case 2:
        return InputError::TargetMismatch;
    case 3:
        return InputError::DebuggerPresent;
    case 4:
        return InputError::HookInstallFailed;
    default:
        return InputError::CompanionLoopInvalid;
    }
}

} // namespace

void SetModule(HMODULE module) noexcept
{
    g_module = module;
    isaac_cs_t0::raw_apply::SetModule(module);
}

void ProcessDetach() noexcept
{
    CloseMapping();
    isaac_cs_t0::raw_apply::ProcessDetach();
    g_main_base = nullptr;
    g_module = nullptr;
}

extern "C" DWORD WINAPI IsaacCsT0InputInitialize(const wchar_t* mapping_name)
{
    struct MappingPrefix {
        std::uint64_t magic;
        std::uint32_t schema;
        std::uint32_t size;
    };
    if (mapping_name && *mapping_name)
    {
        HANDLE inspect_mapping = OpenFileMappingW(FILE_MAP_READ, FALSE, mapping_name);
        if (inspect_mapping)
        {
            const auto* prefix = static_cast<const MappingPrefix*>(
                MapViewOfFile(inspect_mapping, FILE_MAP_READ, 0, 0, sizeof(MappingPrefix)));
            std::uint64_t magic = 0;
            std::uint32_t schema = 0;
            if (prefix)
            {
                __try
                {
                    magic = prefix->magic;
                    schema = prefix->schema;
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    magic = 0;
                    schema = 0;
                }
                UnmapViewOfFile(prefix);
            }
            CloseHandle(inspect_mapping);
            if (magic == kInputIpcMagic && schema == 3)
                return isaac_cs_t0::raw_apply::Initialize(mapping_name);
        }
    }
    if (!g_module || !mapping_name || !*mapping_name || g_mapping || g_shared)
        return static_cast<DWORD>(InputError::MappingInvalid);
    g_mapping = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, mapping_name);
    if (!g_mapping)
        return static_cast<DWORD>(InputError::MappingInvalid);
    g_shared = static_cast<SharedInputBlock*>(
        MapViewOfFile(g_mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedInputBlock)));
    if (!ValidMapping())
    {
        SetState(InputError::MappingInvalid, InputNativeState::Failed);
        return static_cast<DWORD>(InputError::MappingInvalid);
    }
    const auto probe = static_cast<InputProbe>(g_shared->probe);
    if (probe != InputProbe::Passive)
    {
        SetState(InputError::InvalidProbe, InputNativeState::Failed);
        PublishFailure(InputError::InvalidProbe);
        return static_cast<DWORD>(InputError::InvalidProbe);
    }

    InterlockedExchange(&g_shared->state, static_cast<LONG>(InputNativeState::Initializing));
    g_main_base = reinterpret_cast<std::uint8_t*>(GetModuleHandleW(nullptr));
    Publish(Event(InputEventKind::NativeInitialized));
    if (!IsTargetImage())
    {
        PublishFailure(InputError::TargetMismatch);
        SetState(InputError::TargetMismatch, InputNativeState::Failed);
        return static_cast<DWORD>(InputError::TargetMismatch);
    }
    if (IsDebuggerPresent() != FALSE)
    {
        PublishFailure(InputError::DebuggerPresent);
        SetState(InputError::DebuggerPresent, InputNativeState::Failed);
        return static_cast<DWORD>(InputError::DebuggerPresent);
    }
    if (!ValidateAndPrepareTargets())
    {
        PublishFailure(InputError::HookTargetMismatch);
        SetState(InputError::HookTargetMismatch, InputNativeState::Failed);
        return static_cast<DWORD>(InputError::HookTargetMismatch);
    }

    isaac_cs_t0::loop::EmbeddedObserverIdentity companion_identity{};
    companion_identity.target_pid = g_shared->target_pid;
    companion_identity.scenario = g_shared->scenario;
    companion_identity.nonce = g_shared->nonce;
    const DWORD loop_result = isaac_cs_t0::loop::StartEmbeddedObserver(
        g_shared->loop_mapping_name.data(), g_input_patches.data(), g_input_patches.size(),
        companion_identity);
    if (loop_result != 0)
    {
        const InputError error = MapCompanionError(loop_result);
        PublishFailure(error);
        if (loop_result == kUnsafeInitializerResult)
        {
            SetState(InputError::RestoreFailed, InputNativeState::Unsafe);
            return kUnsafeInitializerResult;
        }
        SetState(error, InputNativeState::Failed);
        return static_cast<DWORD>(error);
    }

    InterlockedExchange64(&g_shared->installed_hooks_mask,
                          static_cast<LONG64>(kInputHookMask));
    Publish(Event(InputEventKind::HooksInstalled));
    InterlockedExchange(&g_armed, 1);
    InterlockedExchange(&g_shared->state, static_cast<LONG>(InputNativeState::Ready));
    return 0;
}

extern "C" DWORD WINAPI IsaacCsT0InputShutdown(void*)
{
    if (isaac_cs_t0::raw_apply::Active())
    {
        const DWORD result = isaac_cs_t0::raw_apply::Shutdown();
        if (result != 0)
            return result;
        HMODULE module = g_module;
        FreeLibraryAndExitThread(module, 0);
    }
    if (!g_shared || InterlockedExchange(&g_unloading, 1) != 0)
        return static_cast<DWORD>(InputError::RestoreFailed);
    InterlockedExchange(&g_shared->state, static_cast<LONG>(InputNativeState::ShuttingDown));
    InterlockedExchange(&g_armed, 0);
    const auto shutdown_result = detail::RunEmbeddedShutdownStages(
        []() noexcept {
            if (isaac_cs_t0::loop::RestoreEmbeddedInputHooks() != 0)
                return false;
            InterlockedExchange64(&g_shared->restored_hooks_mask,
                                  static_cast<LONG64>(kInputHookMask));
            return true;
        },
        []() noexcept {
            const ULONGLONG deadline = GetTickCount64() + kHookDrainTimeoutMs;
            while (InterlockedCompareExchange(&g_active_hooks, 0, 0) != 0 &&
                   GetTickCount64() < deadline)
                Sleep(1);
            return InterlockedCompareExchange(&g_active_hooks, 0, 0) == 0;
        },
        []() noexcept { return isaac_cs_t0::loop::StopEmbeddedObserver() == 0; });
    if (shutdown_result != detail::EmbeddedShutdownResult::Complete)
    {
        PublishFailure(InputError::RestoreFailed);
        SetState(InputError::RestoreFailed, InputNativeState::Unsafe);
        return static_cast<DWORD>(InputError::RestoreFailed);
    }

    Publish(Event(InputEventKind::HooksRestored));
    Publish(Event(InputEventKind::NativeRetired));
    InterlockedExchange(&g_shared->producer_closed, 1);
    InterlockedExchange(&g_shared->state, static_cast<LONG>(InputNativeState::Stopped));
    HMODULE module = g_module;
    FreeLibraryAndExitThread(module, 0);
}

} // namespace isaac_cs_t0::input
