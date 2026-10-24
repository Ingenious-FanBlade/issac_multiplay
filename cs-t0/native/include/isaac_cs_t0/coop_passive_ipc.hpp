#pragma once

#include <Windows.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace isaac_cs_t0::coop_passive {

inline constexpr std::uint64_t kIpcMagic = 0x315653504F4F4349ull;
inline constexpr std::uint32_t kSchemaVersion = 1;
inline constexpr std::uint32_t kDeviceSchemaVersion = 2;
inline constexpr std::size_t kEventCapacity = 16'384;
inline constexpr std::size_t kDigestSize = 32;
inline constexpr std::size_t kLoopMappingNameCapacity = 96;
inline constexpr std::size_t kInheritedSlotCount = 45;
inline constexpr std::size_t kCoopSlotCount = 8;
inline constexpr std::size_t kCombinedSlotCount =
    kInheritedSlotCount + kCoopSlotCount;
inline constexpr std::uint64_t kCompleteHookMask =
    (1ull << kCombinedSlotCount) - 1ull;
inline constexpr std::size_t kDeviceAdditionalSlotCount = 3;
inline constexpr std::size_t kDeviceCombinedSlotCount =
    kCombinedSlotCount + kDeviceAdditionalSlotCount;
inline constexpr std::uint64_t kDeviceCompleteHookMask =
    (1ull << kDeviceCombinedSlotCount) - 1ull;
inline constexpr std::uint32_t kNoOrdinal = UINT32_MAX;
inline constexpr std::uint32_t kNoSlot = UINT32_MAX;

enum class NativeState : LONG {
    Empty = 0,
    Initializing = 1,
    Ready = 2,
    Failed = 3,
    ShuttingDown = 4,
    Stopped = 5,
    Unsafe = 6,
};

enum class Scenario : std::uint32_t {
    NativeJoin = 1,
    NativeExit = 2,
    DeathRespawn = 3,
    RoomRebind = 4,
    DeviceRebind = 5,
    MenuTeardown = 6,
};

enum class EventKind : std::uint32_t {
    NativeInitialized = 1,
    HooksInstalled = 2,
    ManagerPassBegin = 3,
    ManagerPassEnd = 4,
    OrdinarySpawnBegin = 5,
    OrdinarySpawnEnd = 6,
    PossessorSpawnStop = 7,
    ExitBegin = 8,
    ExitReturnStable = 9,
    OrdinaryAssignmentBegin = 10,
    OrdinaryAssignmentEnd = 11,
    PossessorAssignmentStop = 12,
    JoinHudBegin = 13,
    JoinHudEnd = 14,
    ExitHudBegin = 15,
    ExitHudEnd = 16,
    BindingUnbound = 17,
    BindingCandidate = 18,
    BindingBound = 19,
    BindingRetired = 20,
    LuaVectorEquivalent = 21,
    LifecycleEnter = 22,
    LifecycleExit = 23,
    RelationChange = 24,
    HooksRestored = 25,
    ProducerClosed = 26,
    Failure = 27,
    DeviceUnassignBegin = 28,
    DeviceUnassignEnd = 29,
    DeviceReassignBegin = 30,
    DeviceReassignEnd = 31,
    UnclassifiedRelationChange = 32,
};

enum class BindingState : std::uint32_t {
    Unbound = 0,
    Candidate = 1,
    Bound = 2,
    Retired = 3,
};

enum class LifecycleReason : std::uint32_t {
    None = 0,
    NativeJoin = 1,
    NativeExit = 2,
    OrdinaryAssignment = 3,
    NewRoom = 4,
    AlternateEnvelope = 5,
    RoomLifecycle = 6,
    RoomChange = 7,
    RunTeardown = 8,
    ObservedRelationChange = 9,
    DeviceRebind = 10,
};

enum class ErrorCode : LONG {
    None = 0,
    MappingInvalid = 1,
    TargetImageMismatch = 2,
    ConflictingModuleOrDebugger = 3,
    TargetMismatch = 4,
    HookInstallFailed = 5,
    RingOverflow = 6,
    WrongThread = 7,
    TopologyReadFailed = 8,
    TopologyAmbiguous = 9,
    ReentrantManager = 10,
    PossessorPath = 11,
    GenerationOverflow = 12,
    LifecycleMismatch = 13,
    CompanionUnsafe = 14,
    DrainTimeout = 15,
    RestoreMismatch = 16,
    UnsafeShutdown = 17,
};

inline constexpr std::uint32_t kFlagMainThread = 1u << 0;
inline constexpr std::uint32_t kFlagOriginalCalled = 1u << 1;
inline constexpr std::uint32_t kFlagTopologyCoherent = 1u << 2;
inline constexpr std::uint32_t kFlagEndpointA = 1u << 3;
inline constexpr std::uint32_t kFlagEndpointB = 1u << 4;
inline constexpr std::uint32_t kFlagBindingUnique = 1u << 5;
inline constexpr std::uint32_t kFlagHudMatch = 1u << 6;
inline constexpr std::uint32_t kFlagAssignmentMatch = 1u << 7;
inline constexpr std::uint32_t kFlagLuaVectorEquivalent = 1u << 8;
inline constexpr std::uint32_t kFlagRelationChanged = 1u << 9;
inline constexpr std::uint32_t kFlagStableRegistryRetired = 1u << 10;
inline constexpr std::uint32_t kFlagFailure = 1u << 11;
inline constexpr std::uint32_t kKnownFlags = (1u << 12) - 1u;

struct alignas(8) NativePassiveEvent {
    volatile LONG64 published_sequence = 0;
    std::int64_t qpc = 0;
    std::uint64_t loop_iteration = 0;
    std::uint64_t owner_generation = 0;
    std::array<std::uint8_t, kDigestSize> topology_digest{};
    std::array<std::uint8_t, kDigestSize> player_digest{};
    std::array<std::uint8_t, kDigestSize> device_digest{};
    std::array<std::uint8_t, kDigestSize> assignment_digest{};
    std::uint32_t thread_id = 0;
    std::uint32_t main_thread_id = 0;
    std::uint32_t kind = 0;
    std::uint32_t slot_ordinal = kNoSlot;
    std::uint32_t phase = 0;
    std::uint32_t depth = 0;
    std::uint32_t local_ordinal = kNoOrdinal;
    std::uint32_t endpoint = 0;
    std::uint32_t original_calls = 0;
    std::uint32_t flags = 0;
};

struct alignas(8) SharedCoopPassiveBlock {
    std::uint64_t magic = 0;
    std::uint32_t version = 0;
    std::uint32_t struct_size = 0;
    std::uint32_t target_pid = 0;
    std::uint32_t scenario = 0;
    std::uint64_t process_creation_time_100ns = 0;
    std::array<std::uint8_t, kDigestSize> nonce{};
    std::array<std::uint8_t, kDigestSize> canonical_target_sha256{};
    std::array<std::uint8_t, kDigestSize> observed_target_sha256{};
    std::array<std::uint8_t, kDigestSize> target_equivalence_sha256{};
    std::array<std::uint8_t, kDigestSize> catalog_sha256{};
    std::array<std::uint8_t, kDigestSize> graph_sha256{};
    std::array<std::uint8_t, kDigestSize> plan_sha256{};
    std::array<std::uint8_t, kDigestSize> verification_sha256{};
    std::array<wchar_t, kLoopMappingNameCapacity> loop_mapping_name{};
    volatile LONG state = 0;
    volatile LONG error_code = 0;
    volatile LONG64 installed_hooks_mask = 0;
    volatile LONG64 restored_hooks_mask = 0;
    volatile LONG restored_slot_count = 0;
    volatile LONG main_thread_id = 0;
    volatile LONG64 owner_generation = 0;
    volatile LONG64 next_sequence = 0;
    volatile LONG64 controller_sequence = 0;
    volatile LONG64 overwritten_events = 0;
    volatile LONG active_hooks = 0;
    volatile LONG producer_closed = 0;
    volatile LONG publication_enabled = 0;
    volatile LONG stable_registry_retired = 0;
    volatile LONG64 override_count = 0;
    volatile LONG64 apply_count = 0;
    volatile LONG64 world_write_count = 0;
    volatile LONG64 scheduler_count = 0;
    volatile LONG64 transport_count = 0;
    volatile LONG64 mode_latch_write_count = 0;
    std::array<std::uint8_t, kDigestSize> slot_snapshot_before_sha256{};
    std::array<std::uint8_t, kDigestSize> slot_snapshot_after_sha256{};
    std::array<NativePassiveEvent, kEventCapacity> events{};
};

inline void CopyEventPayload(NativePassiveEvent& destination,
                             const NativePassiveEvent& source) noexcept
{
    constexpr std::size_t sequence_size = sizeof(NativePassiveEvent::published_sequence);
    std::memcpy(reinterpret_cast<std::uint8_t*>(&destination) + sequence_size,
                reinterpret_cast<const std::uint8_t*>(&source) + sequence_size,
                sizeof(NativePassiveEvent) - sequence_size);
}

inline constexpr std::size_t kNativeEventSize = sizeof(NativePassiveEvent);
inline constexpr std::size_t kSharedBlockSize = sizeof(SharedCoopPassiveBlock);
inline constexpr std::size_t kEventsOffset = offsetof(SharedCoopPassiveBlock, events);

static_assert(sizeof(void*) == 4);
static_assert(sizeof(wchar_t) == 2);
static_assert(kCombinedSlotCount < 64);
static_assert(kCompleteHookMask == 0x001FFFFFFFFFFFFFull);
static_assert(kDeviceCompleteHookMask == 0x00FFFFFFFFFFFFFFull);
static_assert(sizeof(NativePassiveEvent) == 200);
static_assert(alignof(NativePassiveEvent) == 8);
static_assert(offsetof(NativePassiveEvent, thread_id) == 160);
static_assert(offsetof(NativePassiveEvent, flags) == 196);
static_assert(offsetof(SharedCoopPassiveBlock, process_creation_time_100ns) == 24);
static_assert(offsetof(SharedCoopPassiveBlock, nonce) == 32);
static_assert(offsetof(SharedCoopPassiveBlock, loop_mapping_name) == 288);
static_assert(offsetof(SharedCoopPassiveBlock, state) == 480);
static_assert(offsetof(SharedCoopPassiveBlock, events) == 672);
static_assert(sizeof(SharedCoopPassiveBlock) == 3'277'472);
static_assert(alignof(SharedCoopPassiveBlock) == 8);

} // namespace isaac_cs_t0::coop_passive
