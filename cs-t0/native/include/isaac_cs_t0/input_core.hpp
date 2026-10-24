#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace isaac_cs_t0::input {

inline constexpr std::uint16_t kInputStateVersion = 1;
inline constexpr std::uint16_t kInputStateEncodedSize = 24;
inline constexpr std::size_t kInputCommandEncodedSize = 64;
inline constexpr std::int32_t kAxisMaximum = 32767;
inline constexpr std::int32_t kRadialDeadZone = 4096;

inline constexpr std::uint32_t kHeldShoot = 1u << 0;
inline constexpr std::uint32_t kValidHeldMask = kHeldShoot;
inline constexpr std::uint32_t kEdgeBomb = 1u << 0;
inline constexpr std::uint32_t kEdgeActiveItem = 1u << 1;
inline constexpr std::uint32_t kEdgeCardOrPill = 1u << 2;
inline constexpr std::uint32_t kEdgeDrop = 1u << 3;
inline constexpr std::uint32_t kBaseValidEdgeMask =
    kEdgeBomb | kEdgeActiveItem | kEdgeCardOrPill | kEdgeDrop;

inline constexpr std::uint64_t kInputLeadTicks = 2;
inline constexpr std::uint64_t kMaximumFutureTicks = 64;
inline constexpr std::size_t kLoopbackTransportCapacity = 256;
inline constexpr std::size_t kInputQueueCapacity = 256;
inline constexpr std::uint32_t kDrainBudgetPerTick = 64;
inline constexpr std::size_t kDedupeHistoryCapacity = 4096;
inline constexpr std::size_t kGameplayActionCount = 12;
inline constexpr std::size_t kMaximumLogicalActionCount = 32;
inline constexpr std::size_t kPrimitivesPerAction = 4;
inline constexpr std::uint32_t kUnboundPrimitive = UINT32_MAX;

// T0-07 experiment configuration. These are supported ranges, not product
// constants. Missing-input neutrality itself remains fixed at max age 0.
inline constexpr std::uint64_t kRuntimeInputLeadMinimum = 1;
inline constexpr std::uint64_t kRuntimeInputLeadMaximum = 256;
inline constexpr std::uint64_t kRuntimeFutureWindowMinimum = 1;
inline constexpr std::uint64_t kRuntimeFutureWindowMaximum = 256;
inline constexpr std::uint32_t kRuntimeScriptDrainMinimum = 1;
inline constexpr std::uint32_t kRuntimeScriptDrainMaximum = 256;
inline constexpr std::uint32_t kRuntimeAdmissionAttemptMaximum = 4096;

enum class InputConfigDecision : std::uint32_t {
    Accepted = 0,
    InvalidMappingVersion,
    InvalidMappingSize,
    InvalidConfigVersion,
    InvalidConfigSize,
    ConfigReservedNonzero,
    LeadOutOfRange,
    FutureWindowOutOfRange,
    LeadExceedsFutureWindow,
    MaxInputAgeUnsupported,
    ScriptBudgetOutOfRange,
    AdmissionBudgetOutOfRange,
    AdmissionBudgetBelowMinimum,
};
const char* InputConfigDecisionName(InputConfigDecision decision) noexcept;

struct alignas(8) InputRuntimeConfig {
    std::uint32_t version = 1;
    std::uint32_t size = 48;
    std::uint64_t input_lead_ticks = kInputLeadTicks;
    std::uint64_t future_window_ticks = kMaximumFutureTicks;
    std::uint64_t max_input_age_ticks = 0;
    std::uint32_t script_drain_budget = kDrainBudgetPerTick;
    std::uint32_t admission_attempt_budget = 2 + 4 * kDrainBudgetPerTick;
    std::uint64_t reserved = 0;
};
static_assert(sizeof(InputRuntimeConfig) == 48);
static_assert(offsetof(InputRuntimeConfig, input_lead_ticks) == 8);
static_assert(offsetof(InputRuntimeConfig, future_window_ticks) == 16);
static_assert(offsetof(InputRuntimeConfig, max_input_age_ticks) == 24);
static_assert(offsetof(InputRuntimeConfig, script_drain_budget) == 32);
static_assert(offsetof(InputRuntimeConfig, admission_attempt_budget) == 36);
static_assert(offsetof(InputRuntimeConfig, reserved) == 40);

InputConfigDecision ValidateInputRuntimeConfig(const InputRuntimeConfig& config) noexcept;
// Productions helpers used by Native source planning and bounded script drain.
bool NextInputTarget(std::uint64_t completed_tick, const InputRuntimeConfig& config,
                     std::uint64_t& target_tick) noexcept;
bool NextStartupTarget(std::uint64_t completed_tick, std::uint64_t& target_tick) noexcept;
bool ScriptDrainAllowed(std::uint64_t drained_steps, const InputRuntimeConfig& config) noexcept;

using PrimitiveBinding = std::array<std::uint32_t, kPrimitivesPerAction>;

enum class PrimitiveActionClass : std::uint32_t {
    NoMatch = 0,
    UniqueGameplay,
    AmbiguousOrOutsideGameplay,
    InvalidLayout,
};

struct PrimitiveActionClassification {
    PrimitiveActionClass classification = PrimitiveActionClass::NoMatch;
    std::uint32_t action = UINT32_MAX;
};

PrimitiveActionClassification ClassifyPrimitiveAction(
    std::uint32_t primitive, const PrimitiveBinding* bindings,
    std::size_t action_count) noexcept;

struct InputStateDraftV1 {
    std::uint16_t version = kInputStateVersion;
    std::uint16_t encoded_length = kInputStateEncodedSize;
    std::uint32_t held_flags = 0;
    std::uint32_t edge_flags = 0;
    std::int16_t move_x = 0;
    std::int16_t move_y = 0;
    std::int16_t aim_x = 0;
    std::int16_t aim_y = 0;
    std::uint32_t reserved = 0;
};

struct alignas(8) InputCmdDraftV1 {
    std::array<std::uint8_t, 16> session_id{};
    std::uint64_t client_id = 0;
    std::uint64_t sequence = 0;
    std::uint64_t target_tick = 0;
    InputStateDraftV1 state{};
};

static_assert(sizeof(InputStateDraftV1) == kInputStateEncodedSize);
static_assert(offsetof(InputStateDraftV1, held_flags) == 4);
static_assert(offsetof(InputStateDraftV1, edge_flags) == 8);
static_assert(offsetof(InputStateDraftV1, move_x) == 12);
static_assert(offsetof(InputStateDraftV1, reserved) == 20);
static_assert(sizeof(InputCmdDraftV1) == kInputCommandEncodedSize);
static_assert(alignof(InputCmdDraftV1) == 8);
static_assert(offsetof(InputCmdDraftV1, client_id) == 16);
static_assert(offsetof(InputCmdDraftV1, sequence) == 24);
static_assert(offsetof(InputCmdDraftV1, target_tick) == 32);
static_assert(offsetof(InputCmdDraftV1, state) == 40);

using EncodedInputStateV1 = std::array<std::uint8_t, kInputStateEncodedSize>;
using EncodedInputCmdV1 = std::array<std::uint8_t, kInputCommandEncodedSize>;

struct RawInputSample {
    std::uint32_t held_flags = 0;
    std::uint32_t edge_flags = 0;
    float move_x = 0.0F;
    float move_y = 0.0F;
    float aim_x = 0.0F;
    float aim_y = 0.0F;
};

enum class CodecError : std::uint32_t {
    None = 0,
    InvalidSize,
    InvalidVersion,
    InvalidLength,
    UnknownHeldFlags,
    UnknownEdgeFlags,
    ReservedNonzero,
    AxisOutOfRange,
    VectorOutOfRange,
    ShootAimMismatch,
    NonFiniteRaw,
};

CodecError CanonicalizeRaw(const RawInputSample& raw, std::uint32_t valid_edge_mask,
                           InputStateDraftV1& output) noexcept;
CodecError ValidateInputState(const InputStateDraftV1& state,
                              std::uint32_t valid_edge_mask) noexcept;
CodecError EncodeInputState(const InputStateDraftV1& state, std::uint32_t valid_edge_mask,
                            EncodedInputStateV1& output) noexcept;
CodecError DecodeInputState(const void* bytes, std::size_t size, std::uint32_t valid_edge_mask,
                            InputStateDraftV1& output) noexcept;
CodecError EncodeInputCommand(const InputCmdDraftV1& command, std::uint32_t valid_edge_mask,
                              EncodedInputCmdV1& output) noexcept;
CodecError DecodeInputCommand(const void* bytes, std::size_t size, std::uint32_t valid_edge_mask,
                              InputCmdDraftV1& output) noexcept;

enum class TransportSendResult : std::uint32_t {
    Sent = 0,
    Full,
    InvalidSize,
};

class LoopbackTransport final {
public:
    TransportSendResult Send(const void* bytes, std::size_t size) noexcept;
    std::size_t Drain(EncodedInputCmdV1* output, std::size_t output_capacity,
                      std::uint32_t maximum_messages = kDrainBudgetPerTick) noexcept;
    void Clear() noexcept;

    [[nodiscard]] std::size_t Size() const noexcept { return count_; }
    [[nodiscard]] bool Empty() const noexcept { return count_ == 0; }

private:
    std::array<EncodedInputCmdV1, kLoopbackTransportCapacity> messages_{};
    std::size_t head_ = 0;
    std::size_t tail_ = 0;
    std::size_t count_ = 0;
};

enum class TickPhase : std::uint32_t {
    DrainBeforeResolve = 0,
    Resolved,
};

enum class AcceptDecision : std::uint32_t {
    Accepted = 0,
    Duplicate,
    Rejected,
    FatalConflict,
};

enum class RejectReason : std::uint32_t {
    None = 0,
    SessionRetired,
    WrongSession,
    WrongClient,
    InvalidSequence,
    InvalidCanonical,
    SequenceConflict,
    StaleSequence,
    TargetExpired,
    TargetTooFar,
    QueueFull,
    EdgeTargetConflict,
};

struct AcceptResult {
    AcceptDecision decision = AcceptDecision::Rejected;
    RejectReason reason = RejectReason::None;
    CodecError codec_error = CodecError::None;
    std::size_t depth_before = 0;
    std::size_t depth_after = 0;
    std::uint64_t sequence_gap = 0;
};

struct EffectiveInput {
    InputStateDraftV1 state{};
    std::array<std::uint64_t, kInputQueueCapacity> provenance_sequences{};
    std::size_t provenance_count = 0;
};

struct ResolveResult {
    bool resolved = false;
    bool has_input = false;
    std::size_t expired_commands = 0;
    EffectiveInput effective{};
};

class InputQueue final {
public:
    InputQueue(const std::array<std::uint8_t, 16>& session_id, std::uint64_t client_id,
               std::uint32_t valid_edge_mask) noexcept;

    AcceptResult Accept(const InputCmdDraftV1& command, std::uint64_t current_tick,
                        TickPhase phase) noexcept;
    ResolveResult Resolve(std::uint64_t tick) noexcept;
    void Retire() noexcept;
    void Clear() noexcept;

    [[nodiscard]] std::size_t Size() const noexcept { return count_; }
    [[nodiscard]] bool Retired() const noexcept { return retired_; }
    [[nodiscard]] std::uint64_t LastAcceptedSequence() const noexcept {
        return last_accepted_sequence_;
    }

private:
    struct HistoryEntry {
        bool occupied = false;
        InputCmdDraftV1 command{};
    };

    [[nodiscard]] const HistoryEntry* FindHistory(std::uint64_t sequence) const noexcept;
    void Remember(const InputCmdDraftV1& command) noexcept;

    std::array<std::uint8_t, 16> session_id_{};
    std::uint64_t client_id_ = 0;
    std::uint32_t valid_edge_mask_ = 0;
    std::array<InputCmdDraftV1, kInputQueueCapacity> commands_{};
    std::size_t count_ = 0;
    std::array<HistoryEntry, kDedupeHistoryCapacity> history_{};
    std::size_t history_count_ = 0;
    std::size_t history_cursor_ = 0;
    std::array<std::uint64_t, 32> last_edge_target_{};
    std::uint32_t edge_target_valid_mask_ = 0;
    std::uint64_t last_accepted_sequence_ = 0;
    std::uint64_t last_resolved_tick_ = 0;
    bool has_accepted_sequence_ = false;
    bool has_resolved_tick_ = false;
    bool retired_ = false;
};

enum class EdgePlanResult : std::uint32_t {
    Planned = 0,
    InvalidCanonical,
    TargetOverflow,
};

class EdgeTargetPlanner final {
public:
    explicit EdgeTargetPlanner(std::uint32_t valid_edge_mask) noexcept
        : valid_edge_mask_(valid_edge_mask & kBaseValidEdgeMask) {}

    EdgePlanResult Plan(std::uint64_t observed_server_tick, const InputStateDraftV1& state,
                        std::uint64_t& target_tick) noexcept;
    void Reset() noexcept;

private:
    std::uint32_t valid_edge_mask_ = 0;
    std::array<std::uint64_t, 32> last_edge_target_{};
    std::uint32_t edge_target_valid_mask_ = 0;
};

} // namespace isaac_cs_t0::input
