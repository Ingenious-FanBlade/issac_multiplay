#pragma once

#include <Windows.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace isaac_cs_t0::boundary {

inline constexpr std::uint32_t kIpcMagic = 0x33425343u;
inline constexpr std::uint32_t kIpcVersion = 3;
inline constexpr std::size_t kEventCapacity = 4096;
inline constexpr std::uint32_t kUnavailable = UINT32_MAX;

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

enum class EventKind : std::uint32_t {
    NativeInitialized = 1,
    HooksInstalled = 2,
    InputSampleEnter = 3,
    InputSampleExit = 4,
    InputApply = 5,
    GameplayEnter = 6,
    GameplayExit = 7,
    EntityEnter = 8,
    EntityExit = 9,
    RemoveEnter = 10,
    RemoveExit = 11,
    PostUpdateEnter = 12,
    PostUpdateExit = 13,
    RenderEnter = 14,
    RenderExit = 15,
    PostRenderEnter = 16,
    PostRenderExit = 17,
    LuaClose = 18,
    Retired = 19,
    HooksRestored = 20,
    Failure = 21,
    SpawnEnter = 22,
    SpawnExit = 23,
    OnlineDispatchEnter = 24,
    OnlineDispatchExit = 25,
    LuaCallbackEnter = 26,
    LuaCallbackExit = 27,
    WindowStart = 28,
    WindowEnd = 29,
    LuaCreated = 30,
    NewRoomEnter = 31,
    NewRoomExit = 32,
};

inline constexpr std::uint32_t kFlagWrongThread = 1u << 0;
inline constexpr std::uint32_t kFlagOriginalOnline = 1u << 1;
inline constexpr std::uint32_t kFlagLifecycleRetired = 1u << 2;
inline constexpr std::uint32_t kFlagSummaryValid = 1u << 3;
inline constexpr std::uint32_t kFlagGameFrameValid = 1u << 4;
inline constexpr std::uint32_t kFlagOnlineFrameValid = 1u << 5;
inline constexpr std::uint32_t kFlagEntityCountValid = 1u << 6;
inline constexpr std::uint32_t kFlagRetirementMenuReturn = 1u << 7;
inline constexpr std::uint32_t kFlagRetirementLuaClose = 1u << 8;
inline constexpr std::uint32_t kFlagRetirementLuaRebuild = 1u << 9;
inline constexpr std::uint32_t kFlagLuaRootMatch = 1u << 10;
inline constexpr std::uint32_t kFlagLuaNewRoot = 1u << 11;
inline constexpr std::uint32_t kFlagGameplayDispatchSuppressed = 1u << 12;
inline constexpr std::uint32_t kFlagMainLoopStateValid = 1u << 13;

struct alignas(8) NativeEvent {
    volatile LONG64 published_sequence = 0;
    std::int64_t qpc = 0;
    std::uint64_t generation = 0;
    std::uint64_t summary_before = 0;
    std::uint64_t summary_after = 0;
    std::uint32_t thread_id = 0;
    std::uint32_t kind = 0;
    std::uint32_t depth = 0;
    std::uint32_t flags = 0;
    std::uint32_t return_rva = 0;
    std::uint32_t game_frame_before = kUnavailable;
    std::uint32_t game_frame_after = kUnavailable;
    std::uint32_t online_frame_before = kUnavailable;
    std::uint32_t online_frame_after = kUnavailable;
    std::uint32_t entity_count_before = kUnavailable;
    std::uint32_t entity_count_after = kUnavailable;
    std::uint32_t original_calls = 0;
    std::uint32_t result_value = 0;
    std::uint32_t error_code = 0;
    std::uint32_t reserved = 0;
};

struct alignas(8) SharedBoundaryBlock {
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
    volatile LONG hooks_mask = 0;
    volatile LONG main_thread_id = 0;
    volatile LONG64 generation = 0;
    volatile LONG64 retired_generation = 0;
    volatile LONG64 next_sequence = 0;
    volatile LONG64 controller_sequence = 0;
    volatile LONG64 overwritten_events = 0;
    volatile LONG active_hooks = 0;
    volatile LONG producer_closed = 0;
    std::array<NativeEvent, kEventCapacity> events{};
};

static_assert(sizeof(NativeEvent) == 104);
static_assert(alignof(NativeEvent) == 8);
static_assert(sizeof(SharedBoundaryBlock) == 426112);
static_assert(alignof(SharedBoundaryBlock) == 8);

inline void CopyEventPayload(NativeEvent& destination, const NativeEvent& source) noexcept
{
    constexpr std::size_t sequence_size = sizeof(NativeEvent::published_sequence);
    std::memcpy(reinterpret_cast<std::uint8_t*>(&destination) + sequence_size,
                reinterpret_cast<const std::uint8_t*>(&source) + sequence_size,
                sizeof(NativeEvent) - sequence_size);
}

} // namespace isaac_cs_t0::boundary
