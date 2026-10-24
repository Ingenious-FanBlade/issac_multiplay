#pragma once

#include <Windows.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace isaac_cs_t0::loop {

inline constexpr std::uint32_t kIpcMagic = 0x52333054u; // "T03R"
inline constexpr std::uint32_t kIpcVersion = 5;
inline constexpr std::size_t kEventCapacity = 65536;
inline constexpr std::uint32_t kNoAction = UINT32_MAX;
inline constexpr std::size_t kRelativeHookCount = 27;
inline constexpr std::size_t kPointerHookCount = 5;
inline constexpr std::size_t kHookCount = kRelativeHookCount + kPointerHookCount;
static_assert(kHookCount < 64);
inline constexpr std::uint64_t kCompleteHookMask = (1ull << kHookCount) - 1ull;

enum class Probe : std::uint32_t {
    Passive = 1,
};

enum class NativeState : LONG {
    Empty = 0,
    Initializing = 1,
    Ready = 2,
    Failed = 3,
    ShuttingDown = 4,
    Stopped = 5,
    Unsafe = 6,
};

enum class Phase : std::uint32_t {
    OutsideEnvelope = 0,
    LoopBoundary = 1,
    MessagePump = 2,
    InputQuery = 3,
    GameplayUpdate = 4,
    EntityMutation = 5,
    PostUpdate = 6,
    Render = 7,
    RoomLifecycle = 8,
    RunLifecycle = 9,
};

enum class EventKind : std::uint32_t {
    NativeInitialized = 1,
    HooksInstalled = 2,
    LoopBegin = 3,
    LoopClockExit = 4,
    UpdateEnter = 5,
    UpdateExit = 6,
    GameplayEnter = 7,
    GameplayExit = 8,
    EntityEnter = 9,
    EntityExit = 10,
    RemoveEnter = 11,
    RemoveExit = 12,
    PostUpdateEnter = 13,
    PostUpdateExit = 14,
    SpawnEnter = 15,
    SpawnExit = 16,
    NewRoomEnter = 17,
    NewRoomExit = 18,
    RunTeardownEnter = 19,
    RunTeardownExit = 20,
    RenderEnter = 21,
    RenderExit = 22,
    PeekMessage = 23,
    TranslateMessage = 24,
    DispatchMessage = 25,
    InputPressed = 26,
    InputTriggered = 27,
    InputScalar = 28,
    HooksRestored = 29,
    Failure = 30,
    InputManagerSampleEnter = 31,
    InputManagerSampleExit = 32,
    InputDeviceUpdateEnter = 33,
    InputDeviceUpdateExit = 34,
    InputPhysicalPollEnter = 35,
    InputPhysicalPollExit = 36,
    ConditionalAttemptEnter = 37,
    ConditionalAttemptExit = 38,
    AlternateOwnerEnter = 39,
    AlternateOwnerExit = 40,
    AlternateRoomEnter = 41,
    AlternateRoomExit = 42,
    AlternateTailEnter = 43,
    AlternateTailExit = 44,
    StandardPredicateEnter = 45,
    StandardPredicateExit = 46,
    StandardBodyEnter = 47,
    StandardBodyExit = 48,
    StandardRoomEnter = 49,
    StandardRoomExit = 50,
    StandardTailEnter = 51,
    StandardTailExit = 52,
    EntityListEnter = 53,
    EntityListExit = 54,
    AlternateStateApplyEnter = 55,
    AlternateStateApplyExit = 56,
    AlternateOutcomeEnter = 57,
    AlternateOutcomeExit = 58,
    StandardRoomLifecycleOwnerEnter = 59,
    StandardRoomLifecycleOwnerExit = 60,
    StandardRoomLifecycleSimulationEnter = 61,
    StandardRoomLifecycleSimulationExit = 62,
    RoomChangeOwnerEnter = 63,
    RoomChangeOwnerExit = 64,
    RoomChangeSimulationEnter = 65,
    RoomChangeSimulationExit = 66,
};

inline constexpr std::uint32_t kFlagLoopKnown = 1u << 0;
inline constexpr std::uint32_t kFlagMainThread = 1u << 1;
inline constexpr std::uint32_t kFlagWrongThread = 1u << 2;
inline constexpr std::uint32_t kFlagOriginalCalled = 1u << 3;
inline constexpr std::uint32_t kFlagResultTrue = 1u << 4;
inline constexpr std::uint32_t kFlagActionInGameplayRange = 1u << 5;
inline constexpr std::uint32_t kFlagKeyboardDevice = 1u << 6;
inline constexpr std::uint32_t kFlagGamepadDevice = 1u << 7;

struct alignas(8) NativeEvent {
    volatile LONG64 published_sequence = 0;
    std::int64_t qpc = 0;
    std::uint64_t loop_iteration = 0;
    std::uint32_t thread_id = 0;
    std::uint32_t kind = 0;
    std::uint32_t phase = 0;
    std::uint32_t action = kNoAction;
    std::uint32_t value_bits = 0;
    std::uint32_t flags = 0;
    std::uint32_t return_rva = 0;
    std::uint32_t original_calls = 0;
    std::uint32_t error_code = 0;
    std::uint32_t device_key = 0;
};

struct alignas(8) SharedLoopBlock {
    std::uint32_t magic = 0;
    std::uint32_t version = 0;
    std::uint32_t size = 0;
    std::uint32_t target_pid = 0;
    std::uint32_t probe = 0;
    std::uint32_t scenario = 0;
    std::uint32_t flags = 0;
    std::uint32_t reserved0 = 0;
    std::array<std::uint8_t, 32> nonce{};
    volatile LONG state = 0;
    volatile LONG error_code = 0;
    volatile LONG64 hooks_mask = 0;
    volatile LONG main_thread_id = 0;
    volatile LONG64 current_loop_iteration = 0;
    volatile LONG64 next_sequence = 0;
    volatile LONG64 controller_sequence = 0;
    volatile LONG64 overwritten_events = 0;
    volatile LONG active_hooks = 0;
    volatile LONG producer_closed = 0;
    std::array<NativeEvent, kEventCapacity> events{};
};

static_assert(sizeof(NativeEvent) == 64);
static_assert(alignof(NativeEvent) == 8);
static_assert(sizeof(SharedLoopBlock) == 4194432);
static_assert(alignof(SharedLoopBlock) == 8);

inline void CopyEventPayload(NativeEvent& destination, const NativeEvent& source) noexcept
{
    constexpr std::size_t sequence_size = sizeof(NativeEvent::published_sequence);
    std::memcpy(reinterpret_cast<std::uint8_t*>(&destination) + sequence_size,
                reinterpret_cast<const std::uint8_t*>(&source) + sequence_size,
                sizeof(NativeEvent) - sequence_size);
}

} // namespace isaac_cs_t0::loop
