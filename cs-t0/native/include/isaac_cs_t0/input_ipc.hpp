#pragma once

#include <Windows.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace isaac_cs_t0::input {

inline constexpr std::uint64_t kInputIpcMagic = 0x3156343049534349ull; // "ICSI04V1"
inline constexpr std::uint32_t kInputIpcSchemaVersion = 2;
inline constexpr std::size_t kInputEventCapacity = 65536;
inline constexpr std::size_t kInputHookCount = 11;
inline constexpr std::uint64_t kInputHookMask = (1ull << kInputHookCount) - 1ull;
inline constexpr std::uint32_t kInputNoAction = UINT32_MAX;
inline constexpr std::uint32_t kInputNoPrimitive = UINT32_MAX;
inline constexpr std::uint32_t kInputNoDeviceKey = UINT32_MAX;
inline constexpr std::int32_t kInputNoControllerId = -1;
inline constexpr std::size_t kInputLoopMappingNameCapacity = 128;

enum class InputProbe : std::uint32_t {
    Passive = 1,
    Loopback = 2,
    OriginalOnlineDisarmed = 3,
};

enum class InputNativeState : LONG {
    Empty = 0,
    Initializing = 1,
    Ready = 2,
    Failed = 3,
    ShuttingDown = 4,
    Stopped = 5,
    Unsafe = 6,
};

enum class InputError : LONG {
    None = 0,
    MappingInvalid = 1,
    TargetMismatch = 2,
    DebuggerPresent = 3,
    InvalidProbe = 4,
    CompanionLoopInvalid = 5,
    HookTargetMismatch = 6,
    HookInstallFailed = 7,
    WrongThread = 8,
    RingOverflow = 9,
    RestoreFailed = 10,
    UnknownAction = 11,
    ScopeViolation = 12,
};

enum class InputEventKind : std::uint32_t {
    NativeInitialized = 1,
    HooksInstalled = 2,
    LogicalPressedEnter = 3,
    LogicalPressedExit = 4,
    LogicalTriggeredEnter = 5,
    LogicalTriggeredExit = 6,
    LogicalScalarEnter = 7,
    LogicalScalarExit = 8,
    ManagerPressedEnter = 9,
    ManagerPressedExit = 10,
    ManagerTriggeredEnter = 11,
    ManagerTriggeredExit = 12,
    ManagerScalarEnter = 13,
    ManagerScalarExit = 14,
    LogicalSetterEnter = 15,
    LogicalSetterExit = 16,
    HooksRestored = 17,
    NativeRetired = 18,
    Failure = 19,
};

enum class InputClassification : std::uint32_t {
    Unknown = 0,
    UniqueGameplay = 1,
    NoMatch = 2,
    AmbiguousOrOutsideGameplay = 3,
    InvalidLayout = 4,
    UnstableSnapshot = 5,
    ExplicitGameplay = 6,
    LogicalOverrideDiagnostic = 7,
};

inline constexpr std::uint32_t kInputFlagLoopKnown = 1u << 0;
inline constexpr std::uint32_t kInputFlagMainThread = 1u << 1;
inline constexpr std::uint32_t kInputFlagWrongThread = 1u << 2;
inline constexpr std::uint32_t kInputFlagOriginalCalled = 1u << 3;
inline constexpr std::uint32_t kInputFlagResultTrue = 1u << 4;
inline constexpr std::uint32_t kInputFlagKeyboardDevice = 1u << 5;
inline constexpr std::uint32_t kInputFlagGamepadDevice = 1u << 6;
inline constexpr std::uint32_t kInputFlagValueValid = 1u << 7;
inline constexpr std::uint32_t kInputFlagRedacted = 1u << 8;
inline constexpr std::uint32_t kInputKnownFlags = (1u << 9) - 1u;

struct alignas(8) NativeInputEvent {
    volatile LONG64 published_sequence = 0;
    std::int64_t qpc = 0;
    std::uint64_t loop_iteration = 0;
    std::uint32_t thread_id = 0;
    std::uint32_t kind = 0;
    std::uint32_t phase = 0;
    std::uint32_t flags = 0;
    std::uint32_t return_rva = 0;
    std::uint32_t original_calls = 0;
    std::uint32_t device_key = kInputNoDeviceKey;
    std::int32_t controller_id = kInputNoControllerId;
    std::uint32_t selected_device_key = kInputNoDeviceKey;
    std::uint32_t action = kInputNoAction;
    std::uint32_t primitive = kInputNoPrimitive;
    std::uint32_t value_bits = 0;
    std::uint32_t classification = static_cast<std::uint32_t>(InputClassification::Unknown);
    std::uint32_t error_code = 0;
};

struct alignas(8) SharedInputBlock {
    std::uint64_t magic = 0;
    std::uint32_t schema_version = 0;
    std::uint32_t struct_size = 0;
    std::uint32_t target_pid = 0;
    std::uint32_t probe = 0;
    std::uint32_t scenario = 0;
    std::uint32_t flags = 0;
    std::array<std::uint8_t, 32> nonce{};
    std::array<std::uint8_t, 32> target_sha256{};
    std::array<std::uint8_t, 32> loop_gate_sha256{};
    std::array<wchar_t, kInputLoopMappingNameCapacity> loop_mapping_name{};
    volatile LONG state = 0;
    volatile LONG error_code = 0;
    volatile LONG64 installed_hooks_mask = 0;
    volatile LONG64 restored_hooks_mask = 0;
    volatile LONG main_thread_id = 0;
    std::uint32_t reserved0 = 0;
    std::uint64_t generation = 0;
    volatile LONG64 current_loop_iteration = 0;
    volatile LONG64 next_sequence = 0;
    volatile LONG64 controller_sequence = 0;
    volatile LONG64 overwritten_events = 0;
    volatile LONG64 redacted_events = 0;
    volatile LONG active_hooks = 0;
    volatile LONG producer_closed = 0;
    std::array<NativeInputEvent, kInputEventCapacity> events{};
};

inline void CopyInputEventPayload(NativeInputEvent& destination,
                                  const NativeInputEvent& source) noexcept
{
    constexpr std::size_t sequence_size = sizeof(NativeInputEvent::published_sequence);
    std::memcpy(reinterpret_cast<std::uint8_t*>(&destination) + sequence_size,
                reinterpret_cast<const std::uint8_t*>(&source) + sequence_size,
                sizeof(NativeInputEvent) - sequence_size);
}

static_assert(sizeof(wchar_t) == 2);
static_assert(sizeof(NativeInputEvent) == 80);
static_assert(alignof(NativeInputEvent) == 8);
static_assert(offsetof(NativeInputEvent, flags) == 36);
static_assert(offsetof(NativeInputEvent, controller_id) == 52);
static_assert(offsetof(NativeInputEvent, error_code) == 76);
static_assert(sizeof(SharedInputBlock) == 5243352);
static_assert(alignof(SharedInputBlock) == 8);
static_assert(offsetof(SharedInputBlock, installed_hooks_mask) == 392);
static_assert(offsetof(SharedInputBlock, redacted_events) == 456);
static_assert(offsetof(SharedInputBlock, events) == 472);
static_assert(kInputHookMask == 0x7FFull);

} // namespace isaac_cs_t0::input
