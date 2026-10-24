#pragma once

#include <Windows.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace isaac_cs_t0::raw_apply {

inline constexpr std::uint64_t kMagic = 0x3156343049534349ull; // "ICSI04V1"
inline constexpr std::uint32_t kSchemaVersion = 3;
inline constexpr std::size_t kEventCapacity = 65536;
inline constexpr std::size_t kLoopMappingNameCapacity = 128;
inline constexpr std::uint64_t kLegacyInputHookMask = 0x7FFull;
inline constexpr std::uint64_t kRawSourceMask = 0x3ull;
inline constexpr std::uint64_t kConsumerHookMask = 0x7Full;
inline constexpr std::uint32_t kNoValue = UINT32_MAX;

inline constexpr std::array<std::uint8_t, 32> kCatalogSha256{{
    0xE2, 0x23, 0x23, 0x04, 0x70, 0x82, 0xE6, 0x28,
    0x98, 0x91, 0x39, 0x3D, 0xE0, 0xB4, 0x67, 0xCA,
    0x50, 0x38, 0xE1, 0x5E, 0xBD, 0x45, 0x31, 0x0F,
    0x39, 0xB5, 0x05, 0x1F, 0xD0, 0xEB, 0x13, 0xD6,
}};
inline constexpr std::array<std::uint8_t, 32> kCapturePlanSha256{{
    0x20, 0x3F, 0x64, 0x82, 0x5E, 0xA2, 0x4E, 0xE4,
    0xEF, 0xA2, 0x1B, 0x73, 0x1F, 0xD2, 0xCB, 0x8E,
    0x15, 0xAB, 0x11, 0xDA, 0x87, 0x14, 0x5F, 0x76,
    0x54, 0xC5, 0x45, 0xB6, 0x76, 0xA4, 0x55, 0x62,
}};

enum class NativeState : LONG {
    Empty = 0, Initializing = 1, Ready = 2, Failed = 3,
    ShuttingDown = 4, Stopped = 5, Unsafe = 6,
};

enum class Error : LONG {
    None = 0, MappingInvalid = 1, TargetMismatch = 2, DebuggerPresent = 3,
    InvalidProbe = 4, CompanionLoopInvalid = 5, HookTargetMismatch = 6,
    HookInstallFailed = 7, WrongThread = 8, RingOverflow = 9,
    RestoreFailed = 10, UnknownAction = 11, ScopeViolation = 12,
    CatalogMismatch = 13, CapturePlanMismatch = 14, OwnerUnstable = 15,
    OfflineProofMissing = 16,
};

enum class EventKind : std::uint32_t {
    NativeInitialized = 1, HooksInstalled = 2,
    LogicalPressedEnter = 3, LogicalPressedExit = 4,
    LogicalTriggeredEnter = 5, LogicalTriggeredExit = 6,
    LogicalScalarEnter = 7, LogicalScalarExit = 8,
    ManagerPressedEnter = 9, ManagerPressedExit = 10,
    ManagerTriggeredEnter = 11, ManagerTriggeredExit = 12,
    ManagerScalarEnter = 13, ManagerScalarExit = 14,
    LogicalSetterEnter = 15, LogicalSetterExit = 16,
    HooksRestored = 17, NativeRetired = 18, Failure = 19,
    RawPollEnter = 20, RawValue = 21, RawPollExit = 22,
    PlayerConsumerEnter = 23, PlayerConsumerExit = 24,
    WorldWitnessBefore = 25, WorldWitnessAfter = 26,
    ContextOwner = 27, ContextBinding = 28,
};

enum class Classification : std::uint32_t {
    Unknown = 0, UniqueGameplay = 1, NoMatch = 2,
    AmbiguousOrOutsideGameplay = 3, InvalidLayout = 4,
    UnstableSnapshot = 5, ExplicitGameplay = 6,
    LogicalOverrideDiagnostic = 7, RawDeviceOnly = 8,
    PlayerConsumerObserved = 9, PassiveWorldWitness = 10,
    ExplicitGameplayWithOutsideAlias = 11,
};

enum class RawChannelKind : std::uint32_t {
    None = 0, KeyboardKeyDigital = 1, GamepadButtonDigital = 2,
    GamepadAxisF32 = 3, GamepadTriggerF32 = 4, DevicePacketU32 = 5,
};

enum class WitnessKind : std::uint32_t {
    None = 0, PositionF32x2 = 1, VelocityF32x2 = 2,
    OwnedEntityCountU32x2 = 3, InventorySlotU32x2 = 4,
    PlayerCounterU32x2 = 5, MovementDirection = 6,
    ShootingDirection = 7, AimVector = 8, AimState = 9,
    BombIntentBit = 10, DropHoldCounter = 11,
};

inline constexpr std::uint32_t kFlagLoopKnown = 1u << 0;
inline constexpr std::uint32_t kFlagMainThread = 1u << 1;
inline constexpr std::uint32_t kFlagWrongThread = 1u << 2;
inline constexpr std::uint32_t kFlagOriginalCalled = 1u << 3;
inline constexpr std::uint32_t kFlagResultTrue = 1u << 4;
inline constexpr std::uint32_t kFlagKeyboard = 1u << 5;
inline constexpr std::uint32_t kFlagGamepad = 1u << 6;
inline constexpr std::uint32_t kFlagValueValid = 1u << 7;
inline constexpr std::uint32_t kFlagRedacted = 1u << 8;
inline constexpr std::uint32_t kFlagSampleKnown = 1u << 9;
inline constexpr std::uint32_t kFlagConsumerKnown = 1u << 10;
inline constexpr std::uint32_t kFlagContextKnown = 1u << 11;
inline constexpr std::uint32_t kFlagPlayerOrdinalValid = 1u << 12;
inline constexpr std::uint32_t kFlagWitnessValid = 1u << 13;
inline constexpr std::uint32_t kFlagRawRising = 1u << 14;
inline constexpr std::uint32_t kFlagRawFalling = 1u << 15;
inline constexpr std::uint32_t kKnownFlags = 0xFFFFu;

struct alignas(8) NativeRawApplyEvent {
    volatile LONG64 published_sequence = 0;
    std::int64_t qpc = 0;
    std::uint64_t loop_iteration = 0;
    std::uint64_t sample_generation = 0;
    std::uint64_t consumer_transaction = 0;
    std::uint64_t context_generation = 0;
    std::uint32_t thread_id = 0;
    std::uint32_t kind = 0;
    std::uint32_t phase = 0;
    std::uint32_t flags = 0;
    std::uint32_t hook_site_rva = 0;
    std::uint32_t return_rva = 0;
    std::uint32_t original_calls = 0;
    std::uint32_t device_key = kNoValue;
    std::uint32_t controller_id_raw = kNoValue;
    std::uint32_t selected_device_key = kNoValue;
    std::uint32_t player_ordinal = kNoValue;
    std::uint32_t action = kNoValue;
    std::uint32_t primitive = kNoValue;
    std::uint32_t raw_channel_kind = 0;
    std::uint32_t raw_channel_id = 0;
    std::uint32_t value_bits = 0;
    std::uint32_t classification = 0;
    std::uint32_t witness_kind = 0;
    std::uint32_t witness_aux_bits = 0;
    std::uint32_t error_code = 0;
};

struct alignas(8) SharedRawApplyBlock {
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
    std::array<std::uint8_t, 32> catalog_sha256{};
    std::array<std::uint8_t, 32> capture_plan_sha256{};
    std::array<wchar_t, kLoopMappingNameCapacity> loop_mapping_name{};
    volatile LONG state = 0;
    volatile LONG error_code = 0;
    volatile LONG64 legacy_installed_hooks_mask = 0;
    volatile LONG64 legacy_restored_hooks_mask = 0;
    volatile LONG64 required_raw_source_mask = 0;
    volatile LONG64 observed_raw_source_mask = 0;
    volatile LONG64 required_consumer_mask = 0;
    volatile LONG64 installed_consumer_mask = 0;
    volatile LONG64 restored_consumer_mask = 0;
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
    std::array<NativeRawApplyEvent, kEventCapacity> events{};
};

inline void CopyEventPayload(NativeRawApplyEvent& destination,
                             const NativeRawApplyEvent& source) noexcept
{
    constexpr std::size_t skip = sizeof(NativeRawApplyEvent::published_sequence);
    std::memcpy(reinterpret_cast<std::uint8_t*>(&destination) + skip,
                reinterpret_cast<const std::uint8_t*>(&source) + skip,
                sizeof(NativeRawApplyEvent) - skip);
}

static_assert(sizeof(wchar_t) == 2);
static_assert(sizeof(NativeRawApplyEvent) == 128);
static_assert(alignof(NativeRawApplyEvent) == 8);
static_assert(offsetof(NativeRawApplyEvent, thread_id) == 48);
static_assert(offsetof(NativeRawApplyEvent, error_code) == 124);
static_assert(sizeof(SharedRawApplyBlock) == 8389184);
static_assert(alignof(SharedRawApplyBlock) == 8);
static_assert(offsetof(SharedRawApplyBlock, state) == 448);
static_assert(offsetof(SharedRawApplyBlock, events) == 576);

} // namespace isaac_cs_t0::raw_apply
