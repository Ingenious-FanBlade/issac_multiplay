#pragma once

#include <Windows.h>

#include <array>
#include <cstddef>
#include <cstdint>

namespace isaac_cs_t0::mode {

inline constexpr std::uint32_t kIpcMagic = 0x324D5343u;
inline constexpr std::uint32_t kIpcVersion = 2;
inline constexpr std::size_t kEventCapacity = 4096;

enum class Scenario : std::uint32_t {
    ControlNoNative = 0,
    OfflineDisarmed = 1,
    OriginalOnline = 2,
};

enum class EndpointRole : std::uint32_t {
    Solo = 0,
    A = 1,
    B = 2,
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

enum class NetworkMode : std::uint32_t {
    Unresolved = 0,
    Offline = 1,
    OriginalOnline = 2,
    CustomAuthorityServer = 3,
    CustomAuthorityClient = 4,
    Retired = 5,
};

enum class NativeEventKind : std::uint32_t {
    NativeInitialized = 1,
    HooksInstalled = 2,
    SteamHooksInstalled = 3,
    LuaCreated = 4,
    LuaSimulationPcall = 5,
    LuaClose = 6,
    Frame = 7,
    Dispatch = 8,
    SteamSend = 9,
    SteamAvailable = 10,
    SteamRead = 11,
    ModeFrozen = 12,
    ModeRetired = 13,
    HooksRestored = 14,
    Failure = 15,
    PostGameStart = 16,
    RunTeardown = 17,
};

inline constexpr std::uint32_t kEventResultBool = 1u << 0;
inline constexpr std::uint32_t kEventManagerIdentity = 1u << 1;
inline constexpr std::uint32_t kEventStableRoster = 1u << 2;
inline constexpr std::uint32_t kEventChannelZero = 1u << 3;
inline constexpr std::uint32_t kEventPayloadHashAfter = 1u << 4;
inline constexpr std::uint32_t kEventRootMatch = 1u << 5;
inline constexpr std::uint32_t kEventWrongThread = 1u << 6;
inline constexpr std::uint32_t kEventFrameRegressed = 1u << 7;
inline constexpr std::uint32_t kEventManagerChanged = 1u << 8;
inline constexpr std::uint32_t kEventLuaClosed = 1u << 9;
inline constexpr std::uint32_t kEventMenuReturn = 1u << 10;
inline constexpr std::uint32_t kEventNonzeroChannel = 1u << 11;
inline constexpr std::uint32_t kEventRunActiveBefore = 1u << 12;
inline constexpr std::uint32_t kEventRunActiveAfterZero = 1u << 13;
inline constexpr std::uint32_t kEventContinued = 1u << 14;

struct alignas(8) NativeEvent {
    LONG64 published_sequence = 0;
    std::int64_t qpc = 0;
    std::uint64_t generation = 0;
    std::uint64_t attempt_id = 0;
    std::uint32_t thread_id = 0;
    std::uint32_t kind = 0;
    std::uint32_t mode = 0;
    std::uint32_t flags = 0;
    std::uint32_t online_before = UINT32_MAX;
    std::uint32_t online_after = UINT32_MAX;
    std::int32_t channel = -1;
    std::int32_t send_mode = -1;
    std::uint32_t payload_size = 0;
    std::uint32_t output_size = UINT32_MAX;
    std::uint32_t original_calls = 0;
    std::int32_t result_value = 0;
    std::uint32_t signal_mask = 0;
    std::uint32_t error_code = 0;
    std::uint32_t active_before = UINT32_MAX;
    std::uint32_t active_after = UINT32_MAX;
    std::uint64_t peer_token = 0;
    std::array<std::uint8_t, 32> payload_hash{};
    std::array<std::uint8_t, 32> payload_hash_after{};
};

struct alignas(8) SharedModeBlock {
    std::uint32_t magic = 0;
    std::uint32_t version = 0;
    std::uint32_t size = 0;
    std::uint32_t target_pid = 0;
    std::uint32_t scenario = 0;
    std::uint32_t endpoint = 0;
    std::uint32_t requested_mode = 0;
    std::uint32_t flags = 0;
    std::array<std::uint8_t, 32> nonce{};
    std::array<std::uint8_t, 32> pairing_key{};
    volatile LONG state = 0;
    volatile LONG error_code = 0;
    volatile LONG network_mode = 0;
    volatile LONG hooks_mask = 0;
    volatile LONG64 generation = 0;
    volatile LONG64 next_sequence = 0;
    volatile LONG64 controller_sequence = 0;
    volatile LONG64 overwritten_events = 0;
    volatile LONG active_hooks = 0;
    volatile LONG producer_closed = 0;
    volatile LONG main_thread_id = 0;
    std::uint32_t reserved = 0;
    volatile LONG64 local_peer_token = 0;
    std::array<NativeEvent, kEventCapacity> events{};
};

static_assert(sizeof(NativeEvent) == 168);
static_assert(alignof(NativeEvent) == 8);
static_assert(sizeof(SharedModeBlock) == 688296);
static_assert(alignof(SharedModeBlock) == 8);

} // namespace isaac_cs_t0::mode
