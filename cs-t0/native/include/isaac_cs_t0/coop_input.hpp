#pragma once

#include "isaac_cs_t0/input_core.hpp"
#include <array>
#include <cstdint>
#include <span>

namespace isaac_cs_t0::coop_active {
class AuthoritySession;
using input::InputRuntimeConfig;
using input::InputConfigDecision;
using input::InputConfigDecisionName;
using input::ValidateInputRuntimeConfig;
using input::NextInputTarget;
using input::NextStartupTarget;
using input::ScriptDrainAllowed;

inline constexpr std::uint32_t kHeldItem = 2, kHeldPill = 4, kHeldDrop = 8;
// Missing input is immediate-neutral. The silence threshold is diagnostic only:
// it never retires a client, ends a session, or substitutes for network liveness.
inline constexpr std::uint64_t kMaximumInputAgeTicks = 0;
inline constexpr std::uint64_t kClientSilenceTimeoutTicks = 120;
enum class MissingInputReason : std::uint32_t {
    None = 0, NoTargetCommand, FutureOnly, Unbound, Retired, SessionInactive
};
enum class SourceActivity : std::uint32_t {
    Unknown = 0, Active, Silent, TimedOut, Unbound, Retired, SessionInactive
};
const char* MissingInputReasonName(MissingInputReason reason) noexcept;
const char* SourceActivityName(SourceActivity activity) noexcept;
struct ClientInputAvailability {
    std::uint64_t observed_tick=0;
    std::uint64_t effective_source_tick=0,effective_source_sequence=0,effective_age_ticks=0;
    std::uint64_t last_valid_arrival_tick=0,last_valid_arrival_sequence=0;
    std::uint64_t last_valid_arrival_target_tick=0,last_valid_arrival_generation=0;
    std::uint64_t valid_arrival_count=0,silent_ticks=0;
    std::uint64_t silent_entries=0,silence_recoveries=0,timeout_entries=0,timeout_recoveries=0;
    MissingInputReason missing_reason=MissingInputReason::None;
    SourceActivity source=SourceActivity::Unknown;
    bool effective_available=false,neutral_applied=false,has_valid_arrival=false;
};
struct InputStateDraftV2 {
    std::uint16_t version = 2, encoded_length = 24;
    std::uint32_t held_flags = 0, edge_flags = 0;
    std::int16_t move_x = 0, move_y = 0, aim_x = 0, aim_y = 0;
    std::uint32_t reserved = 0;
};
struct alignas(8) InputCmdDraftV2 {
    std::array<std::uint8_t, 16> session_id{};
    std::uint64_t client_id = 0, sequence = 0, target_tick = 0;
    InputStateDraftV2 state{};
};
static_assert(sizeof(InputStateDraftV2) == 24);
static_assert(sizeof(InputCmdDraftV2) == 64);
static_assert(offsetof(InputCmdDraftV2, state) == 40);

enum class Decision : std::uint32_t {
    Accepted = 1, Duplicate, WrongSession, WrongClient, Invalid, Stale,
    Expired, TooFar, Full, EdgeConflict, SequenceConflict, WrongGeneration, Retired,SessionInactive
};
enum class Query : std::uint32_t { Pressed = 1, Triggered = 2, Scalar = 3 };
enum class Consumer : std::uint32_t { None, Movement, Bomb, Item, Pill, Shooting, Drop };

struct Binding {
    void* player = nullptr;
    void* device = nullptr;
    void* control = nullptr;
    std::int32_t key = -1;
    bool operator==(const Binding&) const = default;
};

bool Valid(const InputStateDraftV2& state) noexcept;
bool Canonicalize(const input::RawInputSample& raw, InputStateDraftV2& state) noexcept;
// Source-side history only. Bits 0..3 are left, right, up, down. Queries and
// Authority snapshots never mutate it; each participant owns a separate instance.
class ShootingDirections final {
public:
    bool Project(const std::array<float,4>& directions, float& x, float& y) noexcept;
    std::uint32_t HeldMask() const noexcept { return held_; }
    std::uint32_t SelectedMask() const noexcept { return selected_; }
private:
    std::uint32_t held_=0,selected_=0;
};
bool CanonicalizeShooting(input::RawInputSample& raw, const std::array<float,4>& directions,
                          ShootingDirections& history, InputStateDraftV2& state) noexcept;
using EncodedInputStateV2=std::array<std::uint8_t,24>;
using EncodedInputCmdV2=std::array<std::uint8_t,64>;
bool Encode(const InputStateDraftV2& state,EncodedInputStateV2& bytes) noexcept;
bool Decode(const EncodedInputStateV2& bytes,InputStateDraftV2& state) noexcept;
bool Encode(const InputCmdDraftV2& command,EncodedInputCmdV2& bytes) noexcept;
bool Decode(const EncodedInputCmdV2& bytes,InputCmdDraftV2& command) noexcept;
bool ConsumerAllows(Consumer consumer, std::uint32_t action) noexcept;
float Magnitude(const InputStateDraftV2& state, std::uint32_t action) noexcept;

// Diagnostic history, never an ACK, contiguous prefix, or a liveness deadline.
// A sequence is a session high-water mark; target ticks can arrive out of order.
struct ClientEdgeProgress {
    std::uint64_t accepted_sequence=0,accepted_tick=0,accepted_generation=0;
    std::uint64_t resolved_sequence=0,resolved_tick=0,resolved_generation=0;
    std::uint64_t processed_sequence=0,processed_tick=0,processed_generation=0,processed_count=0;
};
struct ClientInputProgress {
    std::array<std::uint8_t,16> session{};
    std::uint64_t client=0,generation=0,next_sequence=0;
    std::uint64_t last_accepted_sequence=0,last_accepted_target_tick=0;
    std::uint64_t last_received_tick=0,last_received_sequence=0,last_received_target_tick=0,last_received_generation=0;
    std::uint64_t last_resolved_tick=0,last_resolved_sequence=0;
    std::uint64_t last_processed_tick=0,last_processed_sequence=0,last_processed_source_sequence=0;
    std::uint64_t last_state_tick=0,last_state_generation=0,future_first_tick=0,future_last_tick=0;
    std::uint64_t submit_count=0,received_count=0,accepted_count=0,duplicate_count=0,rejected_count=0;
    std::uint64_t expired_before_resolve=0,discarded_on_invalidate=0;
    std::array<std::uint64_t,15> rejections{}; // Indexed by Decision; Accepted/Duplicate stay zero.
    std::array<ClientEdgeProgress,4> edges{}; // Bomb, Item, Pill, Drop.
    InputStateDraftV2 last_state{},current_state{};
    std::uint32_t ordinal=0,last_received_decision=0,resolved_contributors=0,processed_contributors=0;
    bool current_valid=false,last_state_valid=false,future_window_valid=false,scope_pending=false;
    ClientInputAvailability availability{};
    bool has_received=false,retired=false,counters_saturated=false;
};

// Pointer/K associations are local generation metadata, never part of the codec.
// All mutation and progress snapshots belong to the existing main-thread owner.
class Participant final {
public:
    void Initialize(const std::array<std::uint8_t,16>& session,
                    std::uint64_t client, std::uint32_t ordinal,
                    const AuthoritySession* authority_session=nullptr,
                    std::uint64_t source_sequence_start=1) noexcept;
    // One-shot experiment configuration, applied before Initialize. Values are
    // copied and frozen; no runtime hot reload is exposed.
    bool Configure(const InputRuntimeConfig& config) noexcept;
    const InputRuntimeConfig& RuntimeConfig() const noexcept {return runtime_config_;}
    void Invalidate() noexcept;
    void Retire() noexcept;
    bool Bind(const Binding& value) noexcept;
    // One main-thread admission path. The caller selects the participant from its
    // trusted source binding; payload client_id cannot redirect to another queue.
    // Generation/completed tick are local metadata, never decoded from the payload.
    Decision AcceptInput(std::span<const std::uint8_t> bytes,std::uint64_t generation,
                         std::uint64_t completed_tick) noexcept;
    // Local loopback only serializes; even malformed commands reach byte admission.
    Decision AcceptInput(const InputCmdDraftV2& command,std::uint64_t generation,
                         std::uint64_t completed_tick) noexcept;
    ClientInputProgress Progress(std::uint64_t completed_tick) const noexcept;
    bool Begin(std::uint64_t tick) noexcept;
    void End(bool normal_return) noexcept;
    float Read(std::uint32_t action, Query query, Consumer consumer,
               const Binding& exact, std::uint64_t tick) const noexcept;
    InputCmdDraftV2 MakeCommand(const InputStateDraftV2& state,
                               std::uint64_t target) noexcept;
    const Binding& Bound() const noexcept { return binding_; }
    std::uint64_t Generation() const noexcept { return generation_; }
    std::uint64_t Client() const noexcept { return client_; }
    const InputStateDraftV2& Current() const noexcept { return current_; }
    std::uint64_t CurrentSequence() const noexcept { return current_sequence_; }
    std::size_t Depth() const noexcept { return depth_; }
    bool BoundValid() const noexcept { return bound_ && !retired_; }
    bool Retired() const noexcept { return retired_; }
    bool Visible() const noexcept { return visible_; }
    bool CurrentEligible(const Binding& exact,std::uint64_t tick) const noexcept;
    bool PreviousComparable() const noexcept;
    bool StartupPending() const noexcept {return BoundValid() && startup_pending_;}
private:
    Decision Submit(const InputCmdDraftV2& command,std::uint64_t generation,
                    std::uint64_t completed_tick) noexcept;
    Decision RejectInvalidEncoding(const InputCmdDraftV2& command,std::uint64_t generation,
                                   std::uint64_t completed_tick) noexcept;
    bool SessionInputActive() const noexcept;
    void Count(std::uint64_t& value,std::uint64_t amount=1) noexcept;
    bool ObserveArrival(const InputCmdDraftV2& command,std::uint64_t generation,
                        std::uint64_t completed_tick) noexcept;
    Decision FinishSubmit(Decision decision,bool attributable) noexcept;
    void StartGeneration(std::uint64_t tick) noexcept;
    void UpdateSourceSilence(std::uint64_t tick) noexcept;
    void NoteValidArrival(const InputCmdDraftV2& command,std::uint64_t completed_tick) noexcept;
    MissingInputReason CurrentMissingReason() const noexcept;
    SourceActivity CurrentSourceActivity() const noexcept;
    void FillAvailability(std::uint64_t tick,ClientInputAvailability& value) const noexcept;
    const AuthoritySession* authority_session_=nullptr;
    bool initialized_=false;
    struct Queued { InputCmdDraftV2 cmd{}; std::uint64_t generation = 0; };
    std::array<std::uint8_t,16> session_{};
    std::uint64_t client_ = 0, sequence_ = 1, generation_ = 1;
    std::uint64_t last_accepted_ = 0, last_resolved_ = 0, visible_tick_ = 0;
    std::uint64_t current_generation_ = 0, previous_generation_ = 0;
    std::uint64_t current_sequence_ = 0;
    std::uint32_t ordinal_ = 0;
    Binding binding_{};
    std::array<Queued, input::kInputQueueCapacity> pending_{};
    std::array<InputCmdDraftV2, input::kDedupeHistoryCapacity> history_{};
    std::size_t depth_ = 0, history_count_ = 0, history_cursor_ = 0;
    std::array<std::uint64_t,4> edge_targets_{};
    std::array<std::uint64_t,4> current_edge_sequences_{};
    std::uint64_t valid_arrival_count_=0,last_valid_arrival_tick_=0,last_valid_arrival_sequence_=0;
    std::uint64_t last_valid_arrival_target_tick_=0,last_valid_arrival_generation_=0;
    std::uint64_t generation_started_tick_=0,generation_started_generation_=0;
    std::uint64_t last_effective_input_tick_=0,last_effective_input_sequence_=0;
    std::uint64_t silent_ticks_=0,silent_entries_=0,silence_recoveries_=0;
    std::uint64_t timeout_entries_=0,timeout_recoveries_=0;
    ClientInputAvailability scope_availability_{};
    ClientInputProgress progress_{};
    InputStateDraftV2 current_{}, previous_{};
    bool bound_ = false, visible_ = false, previous_valid_ = false, retired_ = false;
    bool startup_pending_ = true;
    bool scope_pending_ = false;
    bool generation_started_ = false,current_generation_arrival_ = false;
    bool source_silent_ = false,client_timed_out_ = false;
    InputRuntimeConfig runtime_config_{};
    bool runtime_config_frozen_ = false;
};
} // namespace isaac_cs_t0::coop_active
