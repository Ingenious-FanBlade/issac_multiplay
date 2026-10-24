#pragma once

#include <Windows.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace isaac_cs_t0::active_abi {

inline constexpr std::uint64_t kIpcMagic = 0x3156343049534349ull; // "ICSI04V1"
inline constexpr std::uint32_t kSchemaVersion = 4;
inline constexpr std::size_t kEventCapacity = 65'536;
inline constexpr std::size_t kLoopMappingNameCapacity = 96;
inline constexpr std::size_t kCanonicalSize = 24;
inline constexpr std::size_t kSessionIdSize = 16;
inline constexpr std::size_t kActiveHookCount = 13;
inline constexpr std::size_t kCompanionHookCount = 32;
inline constexpr std::size_t kCombinedHookCount = 45;
inline constexpr std::uint64_t kCompleteActiveHookMask =
    (1ull << kActiveHookCount) - 1ull;

enum class NativeState : LONG {
    Empty = 0,
    Initializing = 1,
    Ready = 2,
    ShuttingDown = 3,
    Stopped = 4,
    Failed = 5,
    Unsafe = 6,
};

enum class Scenario : std::uint32_t {
    Keyboard = 1,
    Controller = 2,
    Simultaneous = 3,
    RapidTap = 4,
    DeviceSwitch = 5,
    Pause = 6,
    Loading = 7,
    Room = 8,
    Menu = 9,
    QueueFull = 10,
    Expired = 11,
    TooFar = 12,
    Stale = 13,
    Conflict = 14,
    Duplicate = 15,
    Loopback = 16,
};

enum class EventKind : std::uint32_t {
    Initialized = 1,
    HooksInstalled = 2,
    PhysicalSample = 3,
    Canonical = 4,
    Send = 5,
    Receive = 6,
    Accept = 7,
    Reject = 8,
    Duplicate = 9,
    Resolve = 10,
    ApplyCommit = 11,
    Suppression = 12,
    QueryOverride = 13,
    Witness = 14,
    OwnerSwitch = 15,
    LifecycleClear = 16,
    ClientRetired = 17,
    HooksRestored = 18,
    ProducerClosed = 19,
    Failure = 20,
};

enum class Phase : std::uint32_t {
    Outside = 0,
    PhysicalSample = 1,
    DrainBeforeResolve = 2,
    Resolve = 3,
    Apply = 4,
    InputQuery = 5,
    Witness = 6,
};

enum class QueryKind : std::uint32_t {
    None = 0,
    Pressed = 1,
    Triggered = 2,
    Scalar = 3,
};

enum class Consumer : std::uint32_t {
    None = 0,
    Movement = 1,
    Bomb = 2,
    Item = 3,
    PillCard = 4,
    Shooting = 5,
    Drop = 6,
};

enum class Lifecycle : std::uint32_t {
    None = 0,
    PauseEnter = 1,
    PauseExit = 2,
    LoadingEnter = 3,
    LoadingExit = 4,
    RoomEnter = 5,
    RoomExit = 6,
    OwnerSwitch = 7,
    RunTeardown = 8,
};

enum class WitnessKind : std::uint32_t {
    None = 0,
    WorldState = 1,
    RejectedUnchanged = 2,
};

enum class SourceDevice : std::uint32_t {
    None = 0,
    Keyboard = 1,
    Gamepad = 2,
};

enum class Decision : std::uint32_t {
    None = 0,
    Accepted = 1,
    Rejected = 2,
    Duplicate = 3,
};

// These are the only public, analyzer-visible reject reasons. Protocol and runtime
// failures use terminal Failure/ClientRetired events with ErrorCode instead.
enum class RejectReason : std::uint32_t {
    None = 0,
    QueueFull = 1,
    Expired = 2,
    TooFar = 3,
    Stale = 4,
    IdentityConflict = 5,
};

enum class ErrorCode : LONG {
    None = 0,
    MappingInvalid = 1,
    TargetImageMismatch = 2,
    ConflictingModuleOrDebugger = 3,
    IdentityMismatch = 4,
    HookTargetMismatch = 5,
    HookInstallFailed = 6,
    RingOverflow = 7,
    UnsafeShutdown = 8,
    WrongThread = 9,
    CompanionUnsafe = 10,
    OwnerConflict = 11,
    ProtocolViolation = 12,
    DrainTimeout = 13,
    RestoreMismatch = 14,
    AuthorityOverflow = 15,
};

inline constexpr std::uint32_t kFlagMainThread = 1u << 0;
inline constexpr std::uint32_t kFlagSteadyStandard = 1u << 1;
inline constexpr std::uint32_t kFlagOriginalCalled = 1u << 2;
inline constexpr std::uint32_t kFlagQueryOverridden = 1u << 3;
inline constexpr std::uint32_t kFlagResultTrue = 1u << 4;
inline constexpr std::uint32_t kFlagClientRetired = 1u << 5;
inline constexpr std::uint32_t kFlagWitnessValid = 1u << 6;
inline constexpr std::uint32_t kFlagConflictingState = 1u << 7;
inline constexpr std::uint32_t kKnownFlags = (1u << 8) - 1u;

struct alignas(8) NativeActiveEvent {
    volatile LONG64 published_sequence = 0;
    std::int64_t qpc = 0;
    std::uint64_t authority_tick = 0;
    std::uint64_t loop_iteration = 0;
    std::uint32_t thread_id = 0;
    std::uint32_t kind = 0;
    std::uint32_t phase = 0;
    std::uint32_t flags = 0;
    std::uint32_t attempt = 0;
    std::uint32_t source_device = 0;
    std::uint64_t client_id = 0;
    std::uint64_t command_sequence = 0;
    std::uint64_t target_tick = 0;
    std::uint32_t edge_flags = 0;
    std::uint32_t decision = 0;
    std::uint32_t reject_reason = 0;
    std::uint32_t queue_depth = 0;
    std::uint64_t previous_command_sequence = 0;
    std::uint64_t apply_generation = 0;
    std::uint64_t owner_generation = 0;
    std::uint32_t owner_before = 0;
    std::uint32_t owner_after = 0;
    std::uint32_t action = 0;
    std::uint32_t query_kind = 0;
    std::uint32_t consumer = 0;
    std::uint32_t original_calls = 0;
    std::uint32_t original_result_bits = 0;
    std::uint32_t effective_result_bits = 0;
    std::uint32_t sample_component_count = 0;
    std::uint32_t hook_site_rva = 0;
    std::uint32_t return_rva = 0;
    std::uint32_t error_code = 0;
    std::uint32_t reserved0 = 0;
    std::array<std::uint8_t, kCanonicalSize> canonical_state{};
    std::array<std::uint8_t, kCanonicalSize> conflicting_state{};
    std::array<std::uint8_t, 32> witness_before{};
    std::array<std::uint8_t, 32> witness_after{};
};

struct alignas(8) SharedActiveBlock {
    std::uint64_t magic = 0;
    std::uint32_t schema_version = 0;
    std::uint32_t struct_size = 0;
    std::uint32_t target_pid = 0;
    std::uint32_t scenario = 0;
    std::uint64_t process_creation_time_100ns = 0;
    std::array<std::uint8_t, 32> nonce{};
    std::array<std::uint8_t, 32> target_sha256{};
    std::array<std::uint8_t, 32> observed_target_sha256{};
    std::array<std::uint8_t, 32> target_equivalence_sha256{};
    std::array<std::uint8_t, 32> mode_gate_sha256{};
    std::array<std::uint8_t, 32> loop_gate_sha256{};
    std::array<std::uint8_t, 32> raw_prereq_sha256{};
    std::array<std::uint8_t, 32> active_catalog_sha256{};
    std::array<std::uint8_t, 32> active_plan_sha256{};
    std::array<std::uint8_t, 32> input_script_sha256{};
    std::array<std::uint8_t, 32> arm_token_sha256{};
    std::array<wchar_t, kLoopMappingNameCapacity> loop_mapping_name{};
    std::array<std::uint8_t, kSessionIdSize> session_id{};
    std::uint64_t client_id = 0;
    volatile LONG state = 0;
    volatile LONG error_code = 0;
    volatile LONG64 installed_active_hooks_mask = 0;
    volatile LONG64 restored_active_hooks_mask = 0;
    volatile LONG restored_slot_count = 0;
    volatile LONG main_thread_id = 0;
    volatile LONG64 apply_generation = 0;
    volatile LONG64 authority_tick = 0;
    volatile LONG64 owner_generation = 0;
    volatile LONG64 next_sequence = 0;
    volatile LONG64 controller_sequence = 0;
    volatile LONG64 overwritten_events = 0;
    volatile LONG active_hooks = 0;
    volatile LONG producer_closed = 0;
    volatile LONG client_retired = 0;
    volatile LONG publication_enabled = 0;
    std::array<std::uint8_t, kCanonicalSize> effective_state{};
    std::array<NativeActiveEvent, kEventCapacity> events{};
};

inline void CopyEventPayload(NativeActiveEvent& destination,
                             const NativeActiveEvent& source) noexcept
{
    constexpr std::size_t sequence_size = sizeof(NativeActiveEvent::published_sequence);
    std::memcpy(reinterpret_cast<std::uint8_t*>(&destination) + sequence_size,
                reinterpret_cast<const std::uint8_t*>(&source) + sequence_size,
                sizeof(NativeActiveEvent) - sequence_size);
}

static_assert(sizeof(void*) == 4);
static_assert(sizeof(wchar_t) == 2);
static_assert(sizeof(NativeActiveEvent) == 288);
static_assert(alignof(NativeActiveEvent) == 8);
static_assert(offsetof(NativeActiveEvent, thread_id) == 32);
static_assert(offsetof(NativeActiveEvent, client_id) == 56);
static_assert(offsetof(NativeActiveEvent, canonical_state) == 172);
static_assert(offsetof(NativeActiveEvent, witness_after) == 252);
static_assert(offsetof(SharedActiveBlock, process_creation_time_100ns) == 24);
static_assert(offsetof(SharedActiveBlock, nonce) == 32);
static_assert(offsetof(SharedActiveBlock, target_sha256) == 64);
static_assert(offsetof(SharedActiveBlock, observed_target_sha256) == 96);
static_assert(offsetof(SharedActiveBlock, target_equivalence_sha256) == 128);
static_assert(offsetof(SharedActiveBlock, mode_gate_sha256) == 160);
static_assert(offsetof(SharedActiveBlock, loop_gate_sha256) == 192);
static_assert(offsetof(SharedActiveBlock, raw_prereq_sha256) == 224);
static_assert(offsetof(SharedActiveBlock, active_catalog_sha256) == 256);
static_assert(offsetof(SharedActiveBlock, active_plan_sha256) == 288);
static_assert(offsetof(SharedActiveBlock, input_script_sha256) == 320);
static_assert(offsetof(SharedActiveBlock, arm_token_sha256) == 352);
static_assert(offsetof(SharedActiveBlock, loop_mapping_name) == 384);
static_assert(offsetof(SharedActiveBlock, session_id) == 576);
static_assert(offsetof(SharedActiveBlock, events) == 720);
static_assert(sizeof(SharedActiveBlock) == 18'875'088);
static_assert(alignof(SharedActiveBlock) == 8);
static_assert(kCombinedHookCount == kCompanionHookCount + kActiveHookCount);

} // namespace isaac_cs_t0::active_abi
