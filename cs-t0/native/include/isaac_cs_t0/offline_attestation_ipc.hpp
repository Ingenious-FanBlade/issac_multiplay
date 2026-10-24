#pragma once

#include <Windows.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace isaac_cs_t0::offline_attestation {

inline constexpr std::uint64_t kMagic = 0x315631414F545349ull;
inline constexpr std::uint32_t kVersion = 2;
inline constexpr std::size_t kEventCapacity = 4096;
inline constexpr std::array<std::uint8_t, 32> kCanonicalTargetSha256{{
    0xC4u, 0x11u, 0x85u, 0x19u, 0xBAu, 0x5Cu, 0x05u, 0x18u,
    0x26u, 0xFEu, 0x16u, 0x50u, 0x77u, 0x15u, 0xDEu, 0x60u,
    0x2Bu, 0x51u, 0xBEu, 0x4Fu, 0x2Cu, 0xD7u, 0x15u, 0x34u,
    0xEAu, 0x13u, 0x03u, 0x7Au, 0x0Fu, 0x9Bu, 0xAEu, 0x30u,
}};
inline constexpr std::array<std::uint8_t, 32> kObservedUnpackedTargetSha256{{
    0x1Fu, 0xBAu, 0x9Au, 0x37u, 0xBCu, 0xEBu, 0xF9u, 0x27u,
    0x6Cu, 0x9Au, 0x80u, 0x37u, 0x2Eu, 0x8Cu, 0x4Du, 0xCAu,
    0xD5u, 0x9Cu, 0x8Fu, 0x65u, 0x2Du, 0x2Fu, 0xD4u, 0x17u,
    0x27u, 0xB3u, 0x96u, 0xDCu, 0xDCu, 0xD6u, 0xB8u, 0x9Cu,
}};
inline constexpr std::array<std::uint8_t, 32> kTargetEquivalenceSha256{{
    0x8Bu, 0x3Fu, 0xB0u, 0x97u, 0xEDu, 0x32u, 0xB1u, 0x8Du,
    0xE6u, 0xA7u, 0x8Fu, 0x53u, 0x50u, 0x45u, 0xACu, 0xD5u,
    0x25u, 0x18u, 0x55u, 0x69u, 0xACu, 0xA0u, 0x29u, 0xA6u,
    0x37u, 0x04u, 0x94u, 0x51u, 0x18u, 0x51u, 0xF1u, 0x38u,
}};

inline constexpr bool ApprovedIdentity(
    const std::array<std::uint8_t, 32>& canonical,
    const std::array<std::uint8_t, 32>& observed,
    const std::array<std::uint8_t, 32>& equivalence) noexcept
{
    return canonical == kCanonicalTargetSha256 &&
           observed == kObservedUnpackedTargetSha256 &&
           equivalence == kTargetEquivalenceSha256;
}

enum class NativeState : LONG {
    Empty = 0,
    Initializing = 1,
    Ready = 2,
    Failed = 3,
    ShuttingDown = 4,
    Stopped = 5,
    Unsafe = 6,
};

enum class Error : LONG {
    None = 0,
    MappingInvalid = 1,
    TargetMismatch = 2,
    DebuggerOrConflict = 3,
    HookTargetMismatch = 4,
    HookInstallFailed = 5,
    SteamContextUnavailable = 6,
    SteamOwnerMismatch = 7,
    WrongThread = 8,
    GenerationChanged = 9,
    OnlineSignal = 10,
    RingOverflow = 11,
    RestoreFailed = 12,
    DrainTimeout = 13,
};

enum class EventKind : std::uint32_t {
    NativeInitialized = 1,
    HooksInstalled = 2,
    SteamObserverArmed = 3,
    SimulationCallback = 4,
    LuaClose = 5,
    SteamSend = 6,
    SteamAvailable = 7,
    SteamRead = 8,
    HooksRestored = 9,
    ProducerClosed = 10,
    Failure = 11,
    MenuHeartbeatPeek = 12,
    GenericLuaPcallDiagnostic = 13,
};

inline constexpr std::uint32_t kFlagMainThread = 1u << 0;
inline constexpr std::uint32_t kFlagOriginalCalled = 1u << 1;
inline constexpr std::uint32_t kFlagRootMatch = 1u << 2;
inline constexpr std::uint32_t kFlagManagerActivity = 1u << 3;
inline constexpr std::uint32_t kFlagStableRoster = 1u << 4;
inline constexpr std::uint32_t kFlagChannelZero = 1u << 5;
inline constexpr std::uint32_t kFlagNonzeroChannel = 1u << 6;
inline constexpr std::uint32_t kFlagSteamArmed = 1u << 7;
inline constexpr std::uint32_t kFlagTeardown = 1u << 8;
inline constexpr std::uint32_t kFlagLoadingMenuInactive = 1u << 9;
inline constexpr std::uint32_t kFlagExactReturn = 1u << 10;
inline constexpr std::uint32_t kFlagWindowMainThread = 1u << 11;
inline constexpr std::uint32_t kFlagRunInactiveBefore = 1u << 12;
inline constexpr std::uint32_t kFlagRunInactiveAfter = 1u << 13;
inline constexpr std::uint32_t kFlagGameRootStable = 1u << 14;
inline constexpr std::uint32_t kFlagManagerNegative = 1u << 15;
inline constexpr std::uint32_t kFlagLuaUnboundMenu = 1u << 16;

struct alignas(8) NativeEvent {
    volatile LONG64 published_sequence = 0;
    std::int64_t qpc = 0;
    std::uint64_t callback_sequence = 0;
    std::uint64_t generation = 0;
    std::uint32_t thread_id = 0;
    std::uint32_t kind = 0;
    std::uint32_t flags = 0;
    std::uint32_t error_code = 0;
    std::uint32_t original_calls = 0;
    std::int32_t channel = -1;
    std::uint32_t return_rva = 0;
    std::uint32_t result_bits = 0;
    std::array<std::uint8_t, 32> game_root_digest{};
    std::array<std::uint8_t, 32> lua_generation_digest{};
};

struct alignas(8) SharedBlock {
    std::uint64_t magic = 0;
    std::uint32_t version = 0;
    std::uint32_t struct_size = 0;
    std::uint32_t target_pid = 0;
    std::uint32_t reserved0 = 0;
    std::uint32_t expected_window_thread_id = 0;
    std::uint32_t reserved1 = 0;
    std::uint64_t process_creation_time_100ns = 0;
    std::array<std::uint8_t, 32> nonce{};
    std::array<std::uint8_t, 32> canonical_target_sha256{};
    std::array<std::uint8_t, 32> observed_target_sha256{};
    std::array<std::uint8_t, 32> target_equivalence_sha256{};
    volatile LONG state = 0;
    volatile LONG error_code = 0;
    volatile LONG64 installed_hooks_mask = 0;
    volatile LONG64 restored_hooks_mask = 0;
    volatile LONG restored_slot_count = 0;
    volatile LONG main_thread_id = 0;
    volatile LONG64 generation = 0;
    volatile LONG64 simulation_callbacks = 0;
    volatile LONG64 menu_heartbeats = 0;
    volatile LONG64 next_sequence = 0;
    volatile LONG64 controller_sequence = 0;
    volatile LONG64 overwritten_events = 0;
    volatile LONG active_hooks = 0;
    volatile LONG producer_closed = 0;
    volatile LONG steam_observer_armed = 0;
    volatile LONG teardown_observed = 0;
    volatile LONG64 manager_signal_count = 0;
    volatile LONG64 roster_signal_count = 0;
    volatile LONG64 channel_zero_count = 0;
    volatile LONG64 nonzero_channel_count = 0;
    std::array<std::uint8_t, 32> slot_snapshot_before_sha256{};
    std::array<std::uint8_t, 32> slot_snapshot_after_sha256{};
    std::array<NativeEvent, kEventCapacity> events{};
};

inline void CopyEventPayload(NativeEvent& destination, const NativeEvent& source) noexcept
{
    constexpr std::size_t skip = sizeof(NativeEvent::published_sequence);
    std::memcpy(reinterpret_cast<std::uint8_t*>(&destination) + skip,
                reinterpret_cast<const std::uint8_t*>(&source) + skip,
                sizeof(NativeEvent) - skip);
}

static_assert(sizeof(void*) == 4);
static_assert(sizeof(NativeEvent) == 128);
static_assert(alignof(NativeEvent) == 8);
static_assert(offsetof(NativeEvent, return_rva) == 56);
static_assert(offsetof(NativeEvent, game_root_digest) == 64);
static_assert(offsetof(SharedBlock, reserved0) == 20);
static_assert(offsetof(SharedBlock, expected_window_thread_id) == 24);
static_assert(sizeof(SharedBlock) == 524648);
static_assert(alignof(SharedBlock) == 8);
static_assert(offsetof(SharedBlock, events) == 360);

} // namespace isaac_cs_t0::offline_attestation
