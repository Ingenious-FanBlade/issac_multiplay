#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace cs::core::input {

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

// 原型的有界输入配置范围；只在初始化时设置，缺输入中性固定为 max age 0。
// script 字段保留为纯预算值，本模块不连接脚本、IPC 或外部控制器。
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
const char *InputConfigDecisionName(InputConfigDecision decision) noexcept;

// 只包含值的输入配置；布局沿用原型，不能在会话运行时热改。
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

InputConfigDecision
ValidateInputRuntimeConfig(const InputRuntimeConfig &config) noexcept;
// 目标 tick 的有界计算与预算判断，不读取物理设备或游戏状态。
bool NextInputTarget(std::uint64_t completed_tick,
                     const InputRuntimeConfig &config,
                     std::uint64_t &target_tick) noexcept;
bool NextStartupTarget(std::uint64_t completed_tick,
                       std::uint64_t &target_tick) noexcept;
bool ScriptDrainAllowed(std::uint64_t drained_steps,
                        const InputRuntimeConfig &config) noexcept;

using PrimitiveBinding = std::array<std::uint32_t, kPrimitivesPerAction>;

enum class PrimitiveActionClass : std::uint32_t {
  NoMatch = 0,
  UniqueGameplay,
  AmbiguousOrOutsideGameplay,
  InvalidLayout,
};

// 对调用者提供的逻辑映射数组分类，不依赖任何版本的设备内存布局。
struct PrimitiveActionClassification {
  PrimitiveActionClass classification = PrimitiveActionClass::NoMatch;
  std::uint32_t action = UINT32_MAX;
};

PrimitiveActionClassification
ClassifyPrimitiveAction(std::uint32_t primitive,
                        const PrimitiveBinding *bindings,
                        std::size_t action_count) noexcept;

// V1 是共用的轴规范化与历史 codec；实际 Participant 使用 V2 扩展 held 位。
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

// 只编码稳定身份、sequence、目标 tick 和状态，不编码临时对象指针。
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

// 采样方提供的连续量与动作位；规范化失败时不发布部分输出。
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

CodecError CanonicalizeRaw(const RawInputSample &raw,
                           std::uint32_t valid_edge_mask,
                           InputStateDraftV1 &output) noexcept;
CodecError ValidateInputState(const InputStateDraftV1 &state,
                              std::uint32_t valid_edge_mask) noexcept;
CodecError EncodeInputState(const InputStateDraftV1 &state,
                            std::uint32_t valid_edge_mask,
                            EncodedInputStateV1 &output) noexcept;
CodecError DecodeInputState(const void *bytes, std::size_t size,
                            std::uint32_t valid_edge_mask,
                            InputStateDraftV1 &output) noexcept;
CodecError EncodeInputCommand(const InputCmdDraftV1 &command,
                              std::uint32_t valid_edge_mask,
                              EncodedInputCmdV1 &output) noexcept;
CodecError DecodeInputCommand(const void *bytes, std::size_t size,
                              std::uint32_t valid_edge_mask,
                              InputCmdDraftV1 &output) noexcept;

enum class TransportSendResult : std::uint32_t {
  Sent = 0,
  Full,
  InvalidSize,
};

// 固定容量的本地字节队列，仅供同一 owner 使用，不提供跨线程同步。
class LoopbackTransport final {
public:
  TransportSendResult Send(const void *bytes, std::size_t size) noexcept;
  std::size_t
  Drain(EncodedInputCmdV1 *output, std::size_t output_capacity,
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

// admission 结果及队列变化，不把重复输入再次排队。
struct AcceptResult {
  AcceptDecision decision = AcceptDecision::Rejected;
  RejectReason reason = RejectReason::None;
  CodecError codec_error = CodecError::None;
  std::size_t depth_before = 0;
  std::size_t depth_after = 0;
  std::uint64_t sequence_gap = 0;
};

// 当前目标的合并结果：连续量取最高 sequence，边沿合并并保留来源。
struct EffectiveInput {
  InputStateDraftV1 state{};
  std::array<std::uint64_t, kInputQueueCapacity> provenance_sequences{};
  std::size_t provenance_count = 0;
};

// 当前目标无命令时返回中性，不沿用 previous 的 held 或 edge。
struct ResolveResult {
  bool resolved = false;
  bool has_input = false;
  std::size_t expired_commands = 0;
  EffectiveInput effective{};
};

// 原型 V1 有界队列；生产流水线使用 Participant 的 V2 队列与 generation 校验。
class InputQueue final {
public:
  InputQueue(const std::array<std::uint8_t, 16> &session_id,
             std::uint64_t client_id, std::uint32_t valid_edge_mask) noexcept;

  AcceptResult Accept(const InputCmdDraftV1 &command,
                      std::uint64_t current_tick, TickPhase phase) noexcept;
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

  [[nodiscard]] const HistoryEntry *
  FindHistory(std::uint64_t sequence) const noexcept;
  void Remember(const InputCmdDraftV1 &command) noexcept;

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

// 同类边沿依次归属不同目标 tick，所有加法先检查溢出。
class EdgeTargetPlanner final {
public:
  explicit EdgeTargetPlanner(std::uint32_t valid_edge_mask) noexcept
      : valid_edge_mask_(valid_edge_mask & kBaseValidEdgeMask) {}

  EdgePlanResult Plan(std::uint64_t observed_server_tick,
                      const InputStateDraftV1 &state,
                      std::uint64_t &target_tick) noexcept;
  void Reset() noexcept;

private:
  std::uint32_t valid_edge_mask_ = 0;
  std::array<std::uint64_t, 32> last_edge_target_{};
  std::uint32_t edge_target_valid_mask_ = 0;
};

} // namespace cs::core::input
