#include "isaac_cs_t0/coop_input.hpp"
#include "isaac_cs_t0/authority_session.hpp"
#include <algorithm>
#include <cstring>
#include <limits>
#include <bit>
#include <cmath>

namespace isaac_cs_t0::coop_active {
namespace {
input::InputStateDraftV1 LegacyAxes(const InputStateDraftV2& state) noexcept
{
    input::InputStateDraftV1 value{};
    value.held_flags = state.held_flags & input::kHeldShoot;
    value.edge_flags = state.edge_flags;
    value.move_x = state.move_x; value.move_y = state.move_y;
    value.aim_x = state.aim_x; value.aim_y = state.aim_y;
    return value;
}
bool Same(const InputCmdDraftV2& a, const InputCmdDraftV2& b) noexcept
{
    return a.session_id == b.session_id && a.client_id == b.client_id &&
           a.sequence == b.sequence && a.target_tick == b.target_tick &&
           std::memcmp(&a.state, &b.state, sizeof(a.state)) == 0;
}
}
const char* MissingInputReasonName(MissingInputReason reason) noexcept
{
    switch(reason) {
    case MissingInputReason::None:return "None";
    case MissingInputReason::NoTargetCommand:return "NoTargetCommand";
    case MissingInputReason::FutureOnly:return "FutureOnly";
    case MissingInputReason::Unbound:return "Unbound";
    case MissingInputReason::Retired:return "Retired";
    case MissingInputReason::SessionInactive:return "SessionInactive";
    }
    return "UNKNOWN";
}
const char* SourceActivityName(SourceActivity activity) noexcept
{
    switch(activity) {
    case SourceActivity::Unknown:return "Unknown";
    case SourceActivity::Active:return "Active";
    case SourceActivity::Silent:return "Silent";
    case SourceActivity::TimedOut:return "TimedOut";
    case SourceActivity::Unbound:return "Unbound";
    case SourceActivity::Retired:return "Retired";
    case SourceActivity::SessionInactive:return "SessionInactive";
    }
    return "UNKNOWN";
}
bool Valid(const InputStateDraftV2& state) noexcept
{
    return state.version == 2 && state.encoded_length == 24 &&
           state.reserved == 0 && (state.held_flags & ~15u) == 0 &&
           input::ValidateInputState(LegacyAxes(state),15) == input::CodecError::None;
}
bool Canonicalize(const input::RawInputSample& raw, InputStateDraftV2& state) noexcept
{
    if (raw.held_flags & ~15u) return false;
    auto axes = raw; axes.held_flags &= input::kHeldShoot;
    input::InputStateDraftV1 value{};
    auto error=input::CanonicalizeRaw(axes,15,value);
    // The radial deadzone may turn a nonzero physical aim into canonical zero.
    if(error==input::CodecError::ShootAimMismatch && axes.held_flags==input::kHeldShoot) {
        axes.held_flags=0;error=input::CanonicalizeRaw(axes,15,value);
    }
    if(error!=input::CodecError::None) return false;
    state = {};
    state.held_flags = (raw.held_flags & ~input::kHeldShoot)|value.held_flags;
    state.edge_flags = value.edge_flags;
    state.move_x = value.move_x; state.move_y = value.move_y;
    state.aim_x = value.aim_x; state.aim_y = value.aim_y;
    return Valid(state);
}
bool ShootingDirections::Project(const std::array<float,4>& directions,float& x,float& y) noexcept
{
    std::uint32_t held=0,selected=0;
    for(std::uint32_t i=0;i<4;++i) {
        if(!std::isfinite(directions[i]) || directions[i]<0 || directions[i]>1) return false;
        if(directions[i]>0) held|=1u<<i;
    }
    for(std::uint32_t axis=0;axis<2;++axis) {
        const auto negative=1u<<(axis*2),positive=negative<<1;
        const auto pair=negative|positive,down=held&pair;
        if(down!=pair) {selected|=down;continue;}
        const auto rising=down&~held_;
        if(rising==negative || rising==positive) selected|=rising;
        else {
            // No distinguishable new press: keep the prior held winner; a first
            // same-sample tie uses right/down, as explicitly accepted by the user.
            const auto previous=selected_&down;
            selected|=previous?previous:positive;
        }
    }
    x=(selected&2?directions[1]:0)-(selected&1?directions[0]:0);
    y=(selected&8?directions[3]:0)-(selected&4?directions[2]:0);
    held_=held;selected_=selected;return true;
}
bool CanonicalizeShooting(input::RawInputSample& raw,const std::array<float,4>& directions,
    ShootingDirections& history,InputStateDraftV2& state) noexcept
{
    auto next=history;
    if(!next.Project(directions,raw.aim_x,raw.aim_y)) return false;
    raw.held_flags&=~input::kHeldShoot;
    if(raw.aim_x || raw.aim_y) raw.held_flags|=input::kHeldShoot;
    if(!Canonicalize(raw,state)) return false;
    history=next;return true;
}
bool Encode(const InputStateDraftV2& state,EncodedInputStateV2& bytes) noexcept
{
    static_assert(std::endian::native==std::endian::little);
    if(!Valid(state)) return false;
    std::memcpy(bytes.data(),&state,bytes.size());return true;
}
bool Decode(const EncodedInputStateV2& bytes,InputStateDraftV2& state) noexcept
{
    InputStateDraftV2 value{};std::memcpy(&value,bytes.data(),bytes.size());
    if(!Valid(value)) return false;state=value;return true;
}
bool Encode(const InputCmdDraftV2& command,EncodedInputCmdV2& bytes) noexcept
{
    if(!Valid(command.state)) return false;
    std::memcpy(bytes.data(),&command,bytes.size());return true;
}
bool Decode(const EncodedInputCmdV2& bytes,InputCmdDraftV2& command) noexcept
{
    InputCmdDraftV2 value{};std::memcpy(&value,bytes.data(),bytes.size());
    if(!Valid(value.state)) return false;command=value;return true;
}
bool ConsumerAllows(Consumer consumer, std::uint32_t action) noexcept
{
    switch (consumer) {
    case Consumer::Movement: return action < 4 || action == 11;
    case Consumer::Shooting: return action < 8 || action == 11;
    case Consumer::Bomb: return action == 8;
    case Consumer::Item: return action >= 9 && action <= 11;
    case Consumer::Pill: return action >= 9 && action <= 11;
    case Consumer::Drop: return action == 11;
    default: return false;
    }
}
float Magnitude(const InputStateDraftV2& state, std::uint32_t action) noexcept
{
    if (action < 8) {
        const std::int32_t axes[4]{state.move_x,state.move_y,state.aim_x,state.aim_y};
        const auto component = axes[action/2];
        return static_cast<float>((std::max)((action & 1) ? component : -component,0)) / 32767.0F;
    }
    if (action >= 9 && action <= 11)
        return (state.held_flags & (1u << (action-8))) ? 1.0F : 0.0F;
    return 0.0F;
}
void Participant::StartGeneration(std::uint64_t tick) noexcept
{
    generation_started_=true;generation_started_generation_=generation_;
    generation_started_tick_=tick;
    current_generation_arrival_=false;source_silent_=false;client_timed_out_=false;
    silent_ticks_=0;last_effective_input_tick_=0;last_effective_input_sequence_=0;
    last_valid_arrival_tick_=0;last_valid_arrival_sequence_=0;
    last_valid_arrival_target_tick_=0;last_valid_arrival_generation_=0;
}
void Participant::UpdateSourceSilence(std::uint64_t tick) noexcept
{
    if(!initialized_ || retired_ || !bound_ || !SessionInputActive()) return;
    if(!generation_started_ || generation_started_generation_!=generation_) StartGeneration(tick);
    const auto base=current_generation_arrival_?last_valid_arrival_tick_:generation_started_tick_;
    silent_ticks_=tick>base?tick-base:0;
    if(silent_ticks_ && !source_silent_) {
        source_silent_=true;Count(silent_entries_);
    }
    if(silent_ticks_>=kClientSilenceTimeoutTicks && !client_timed_out_) {
        client_timed_out_=true;Count(timeout_entries_);
    }
}
void Participant::NoteValidArrival(const InputCmdDraftV2& command,std::uint64_t completed_tick) noexcept
{
    const auto arrival_tick=completed_tick==(std::numeric_limits<std::uint64_t>::max)()?
        completed_tick:completed_tick+1;
    if(!generation_started_ || generation_started_generation_!=generation_)
        StartGeneration(arrival_tick);
    if(source_silent_) {Count(silence_recoveries_);source_silent_=false;}
    if(client_timed_out_) {Count(timeout_recoveries_);client_timed_out_=false;}
    silent_ticks_=0;current_generation_arrival_=true;
    Count(valid_arrival_count_);
    last_valid_arrival_tick_=arrival_tick;
    last_valid_arrival_sequence_=command.sequence;
    last_valid_arrival_target_tick_=command.target_tick;
    last_valid_arrival_generation_=generation_;
}
MissingInputReason Participant::CurrentMissingReason() const noexcept
{
    if(retired_) return MissingInputReason::Retired;
    if(!bound_) return MissingInputReason::Unbound;
    if(!SessionInputActive()) return MissingInputReason::SessionInactive;
    if(current_sequence_) return MissingInputReason::None;
    for(std::size_t i=0;i<depth_;++i)
        if(pending_[i].generation==generation_ && pending_[i].cmd.target_tick>visible_tick_)
            return MissingInputReason::FutureOnly;
    return MissingInputReason::NoTargetCommand;
}
SourceActivity Participant::CurrentSourceActivity() const noexcept
{
    if(!initialized_) return SourceActivity::Unknown;
    if(retired_) return SourceActivity::Retired;
    if(!bound_) return SourceActivity::Unbound;
    if(!SessionInputActive()) return SourceActivity::SessionInactive;
    if(!generation_started_) return SourceActivity::Unknown;
    if(client_timed_out_) return SourceActivity::TimedOut;
    if(source_silent_) return SourceActivity::Silent;
    return SourceActivity::Active;
}
void Participant::FillAvailability(std::uint64_t tick,ClientInputAvailability& value) const noexcept
{
    value={};
    value.observed_tick=tick;
    value.effective_available=current_sequence_!=0;
    value.neutral_applied=!value.effective_available && BoundValid() && SessionInputActive();
    value.missing_reason=CurrentMissingReason();
    value.source=CurrentSourceActivity();
    if(value.effective_available) {
        value.effective_source_tick=tick;value.effective_source_sequence=current_sequence_;
    } else {
        value.effective_source_tick=last_effective_input_tick_;
        value.effective_source_sequence=last_effective_input_sequence_;
        const auto base=last_effective_input_tick_?last_effective_input_tick_:
            (generation_started_?generation_started_tick_:tick);
        value.effective_age_ticks=tick>base?tick-base:0;
    }
    if(current_generation_arrival_) {
        value.last_valid_arrival_tick=last_valid_arrival_tick_;
        value.last_valid_arrival_sequence=last_valid_arrival_sequence_;
        value.last_valid_arrival_target_tick=last_valid_arrival_target_tick_;
        value.last_valid_arrival_generation=last_valid_arrival_generation_;
    }
    value.has_valid_arrival=current_generation_arrival_;
    value.valid_arrival_count=valid_arrival_count_;
    value.silent_ticks=silent_ticks_;
    value.silent_entries=silent_entries_;
    value.silence_recoveries=silence_recoveries_;
    value.timeout_entries=timeout_entries_;
    value.timeout_recoveries=timeout_recoveries_;
}
bool Participant::Configure(const InputRuntimeConfig& config) noexcept
{
    if(initialized_ || runtime_config_frozen_) return false;
    if(ValidateInputRuntimeConfig(config)!=InputConfigDecision::Accepted) return false;
    runtime_config_=config;runtime_config_frozen_=true;return true;
}
void Participant::Initialize(const std::array<std::uint8_t,16>& session,
                            std::uint64_t client, std::uint32_t ordinal,
                            const AuthoritySession* authority_session,
                            std::uint64_t source_sequence_start) noexcept
{
    if(initialized_ && authority_session_) return;
    runtime_config_frozen_=true;
    authority_session_=authority_session;initialized_=true;
    session_ = session; client_ = client; ordinal_ = ordinal;
    retired_ = !client || ordinal > 1 ||
        std::all_of(session.begin(),session.end(),[](auto b){return b == 0;}) ||
        source_sequence_start == 0 ||
        source_sequence_start == (std::numeric_limits<std::uint64_t>::max)();
    sequence_ = retired_ ? 1 : source_sequence_start;
}
void Participant::Invalidate() noexcept
{
    if (generation_ == (std::numeric_limits<std::uint64_t>::max)()) retired_ = true;
    else ++generation_;
    Count(progress_.discarded_on_invalidate,static_cast<std::uint64_t>(depth_));
    bound_ = false; binding_ = {}; visible_ = false; depth_ = 0; scope_pending_ = false;
    current_ = {}; current_sequence_ = 0; current_edge_sequences_.fill(0);
    progress_.last_state_valid = false;
    // Old tagged previous remains stored but is never comparable after transition.
    // Session sequence and dedupe high-water marks must not reset on a rebind.
    previous_valid_ = false;
    startup_pending_ = true;
    edge_targets_.fill(0);
    // Arrival/recovery counters are historical diagnostics; current-generation
    // availability, source-silence baseline and effective source are invalidated.
    generation_started_ = false;current_generation_arrival_ = false;
    source_silent_ = false;client_timed_out_ = false;silent_ticks_ = 0;
    last_effective_input_tick_ = 0;last_effective_input_sequence_ = 0;
    last_valid_arrival_tick_ = 0;last_valid_arrival_sequence_ = 0;
    last_valid_arrival_target_tick_ = 0;last_valid_arrival_generation_ = 0;
    scope_availability_ = {};
    progress_.availability.effective_available = false;
    progress_.availability.neutral_applied = false;
    progress_.availability.missing_reason = MissingInputReason::Unbound;
    progress_.availability.source = SourceActivity::Unbound;
    progress_.availability.effective_source_tick = 0;
    progress_.availability.effective_source_sequence = 0;
    progress_.availability.effective_age_ticks = 0;
    progress_.availability.last_valid_arrival_tick = 0;
    progress_.availability.last_valid_arrival_sequence = 0;
    progress_.availability.last_valid_arrival_target_tick = 0;
    progress_.availability.last_valid_arrival_generation = 0;
    progress_.availability.has_valid_arrival = false;
    progress_.availability.valid_arrival_count = valid_arrival_count_;
    progress_.availability.silent_entries = silent_entries_;
    progress_.availability.silence_recoveries = silence_recoveries_;
    progress_.availability.timeout_entries = timeout_entries_;
    progress_.availability.timeout_recoveries = timeout_recoveries_;
    progress_.availability.silent_ticks = 0;
}
void Participant::Retire() noexcept { Invalidate(); retired_ = true; }
bool Participant::Bind(const Binding& value) noexcept
{
    if(authority_session_ && !authority_session_->MatchesIdentity(session_,client_,ordinal_)) return false;
    if (retired_ || !value.player || !value.device || !value.control || value.key < 0) return false;
    if (!bound_ || !(value == binding_)) {
        if (bound_) Invalidate();
        binding_ = value; bound_ = true;
        if(!generation_started_) {
            progress_.availability.missing_reason=MissingInputReason::None;
            progress_.availability.source=SourceActivity::Unknown;
        }
    }
    return true;
}
InputCmdDraftV2 Participant::MakeCommand(const InputStateDraftV2& state,
                                       std::uint64_t target) noexcept
{
    InputCmdDraftV2 command{};
    if(retired_ || !SessionInputActive()) return command;
    if (sequence_ == (std::numeric_limits<std::uint64_t>::max)()) { Retire(); return command; }
    for (std::uint32_t i=0;i<4;++i) if (state.edge_flags & (1u<<i)) {
        if(edge_targets_[i]==UINT64_MAX) {Retire();return command;}
        target=(std::max)(target,edge_targets_[i]+1);
    }
    command.session_id = session_; command.client_id = client_;
    command.sequence = sequence_++;
    command.target_tick = target; command.state = state;
    return command;
}
void Participant::Count(std::uint64_t& value,std::uint64_t amount) noexcept
{
    if(amount>UINT64_MAX-value) {value=UINT64_MAX;progress_.counters_saturated=true;}
    else value+=amount;
}
bool Participant::ObserveArrival(const InputCmdDraftV2& command,std::uint64_t generation,
                                 std::uint64_t completed_tick) noexcept
{
    Count(progress_.submit_count);
    const bool attributable=command.session_id==session_ && command.client_id==client_;
    if(attributable) {
        Count(progress_.received_count);progress_.has_received=true;
        progress_.last_received_tick=completed_tick;progress_.last_received_sequence=command.sequence;
        progress_.last_received_target_tick=command.target_tick;progress_.last_received_generation=generation;
    }
    return attributable;
}
Decision Participant::FinishSubmit(Decision decision,bool attributable) noexcept
{
    if(attributable)progress_.last_received_decision=static_cast<std::uint32_t>(decision);
    if(decision==Decision::Accepted)Count(progress_.accepted_count);
    else if(decision==Decision::Duplicate)Count(progress_.duplicate_count);
    else {Count(progress_.rejected_count);Count(progress_.rejections[static_cast<std::size_t>(decision)]);}
    return decision;
}
Decision Participant::RejectInvalidEncoding(const InputCmdDraftV2& command,std::uint64_t generation,
                                            std::uint64_t completed_tick) noexcept
{return FinishSubmit(Decision::Invalid,ObserveArrival(command,generation,completed_tick));}
Decision Participant::AcceptInput(const InputCmdDraftV2& command,std::uint64_t generation,
                                 std::uint64_t completed_tick) noexcept
{
    // Serialize without repairing or pre-rejecting invalid source data. Both
    // transports reach the same byte decoder and exactly one arrival accounting.
    static_assert(std::endian::native==std::endian::little);
    EncodedInputCmdV2 bytes{};std::memcpy(bytes.data(),&command,bytes.size());
    return AcceptInput(std::span<const std::uint8_t>(bytes),generation,completed_tick);
}
Decision Participant::AcceptInput(std::span<const std::uint8_t> bytes,std::uint64_t generation,
                                 std::uint64_t completed_tick) noexcept
{
    InputCmdDraftV2 received{};
    // A complete 40-byte header is necessary to attribute even a malformed
    // arrival. Missing header bytes never become invented sequence/target data.
    constexpr auto header_size=offsetof(InputCmdDraftV2,state);
    if(bytes.size()<header_size) {
        Count(progress_.submit_count);
        return FinishSubmit(Decision::Invalid,false);
    }
    std::memcpy(&received,bytes.data(),header_size);
    if(bytes.size()==sizeof(InputCmdDraftV2)) {
        EncodedInputCmdV2 encoded{};std::memcpy(encoded.data(),bytes.data(),encoded.size());
        if(Decode(encoded,received))return Submit(received,generation,completed_tick);
    }
    return RejectInvalidEncoding(received,generation,completed_tick);
}
Decision Participant::Submit(const InputCmdDraftV2& command, std::uint64_t generation,
                             std::uint64_t completed_tick) noexcept
{
    const bool attributable=ObserveArrival(command,generation,completed_tick);
    if (retired_) return FinishSubmit(Decision::Retired,attributable);
    if(!SessionInputActive()) {
        const auto state=authority_session_->Status().state;
        return FinishSubmit(state==SessionState::Retiring || state==SessionState::Retired || state==SessionState::Failed?
            Decision::Retired:Decision::SessionInactive,attributable);
    }
    if (!bound_ || generation != generation_) return FinishSubmit(Decision::WrongGeneration,attributable);
    if (command.session_id != session_) return FinishSubmit(Decision::WrongSession,attributable);
    if (command.client_id != client_) return FinishSubmit(Decision::WrongClient,attributable);
    for (std::size_t i=0;i<history_count_;++i) if (history_[i].sequence == command.sequence) {
        if (Same(history_[i],command)) return FinishSubmit(Decision::Duplicate,attributable);
        FinishSubmit(Decision::SequenceConflict,attributable);Retire();return Decision::SequenceConflict;
    }
    if (!command.sequence || command.sequence == UINT64_MAX || !Valid(command.state)) return FinishSubmit(Decision::Invalid,attributable);
    if (command.sequence <= last_accepted_) return FinishSubmit(Decision::Stale,attributable);
    if (command.target_tick <= completed_tick || command.target_tick <= last_resolved_) return FinishSubmit(Decision::Expired,attributable);
    if (command.target_tick-completed_tick > runtime_config_.future_window_ticks)
        return FinishSubmit(Decision::TooFar,attributable);
    for (std::uint32_t i=0;i<4;++i)
        if ((command.state.edge_flags & (1u<<i)) && command.target_tick <= edge_targets_[i]) return FinishSubmit(Decision::EdgeConflict,attributable);
    if (depth_ == pending_.size()) return FinishSubmit(Decision::Full,attributable);
    pending_[depth_++] = {command,generation};
    history_[history_cursor_] = command;
    history_cursor_ = (history_cursor_+1)%history_.size();
    history_count_ = (std::min)(history_count_+1,history_.size());
    last_accepted_ = command.sequence;
    progress_.last_accepted_target_tick=command.target_tick;
    if(command.target_tick==completed_tick+1) startup_pending_=false;
    for (std::uint32_t i=0;i<4;++i) if (command.state.edge_flags & (1u<<i)) {
        edge_targets_[i] = command.target_tick;
        auto& edge=progress_.edges[i];edge.accepted_sequence=command.sequence;
        edge.accepted_tick=command.target_tick;edge.accepted_generation=generation_;
    }
    NoteValidArrival(command,completed_tick);
    return FinishSubmit(Decision::Accepted,attributable);
}
bool Participant::Begin(std::uint64_t tick) noexcept
{
    if(!SessionInputActive()) return false;
    if (scope_pending_ || tick <= last_resolved_ || retired_) return false;
    last_resolved_ = tick; visible_tick_ = tick; current_ = {}; current_sequence_ = 0;
    current_generation_ = generation_;scope_pending_=true;current_edge_sequences_.fill(0);
    UpdateSourceSilence(tick);
    progress_.resolved_contributors=0;
    std::size_t write = 0; std::uint32_t edges = 0;
    for (std::size_t i=0;i<depth_;++i) {
        const auto& queued = pending_[i];
        if (queued.generation != generation_) continue;
        if (queued.cmd.target_tick < tick) {Count(progress_.expired_before_resolve);continue;}
        if (queued.cmd.target_tick == tick) {
            ++progress_.resolved_contributors; // Bounded by pending_.size().
            edges |= queued.cmd.state.edge_flags;
            for(std::uint32_t action=0;action<4;++action) if(queued.cmd.state.edge_flags & (1u<<action)) {
                current_edge_sequences_[action]=queued.cmd.sequence;
                auto& edge=progress_.edges[action];edge.resolved_sequence=queued.cmd.sequence;
                edge.resolved_tick=tick;edge.resolved_generation=generation_;
            }
            if (queued.cmd.sequence > current_sequence_) { current_ = queued.cmd.state; current_sequence_ = queued.cmd.sequence; }
        } else pending_[write++] = queued;
    }
    depth_ = write; current_.edge_flags = edges;
    progress_.last_resolved_sequence=current_sequence_;
    if(current_sequence_) startup_pending_=false;
    // An explicit neutral current is eligible; absence never retains previous edge/held.
    // A fresh binding with no resolved command has invalid current, rather than
    // fabricating a comparable previous. Its queries are still neutral. After a
    // first valid command, ordinary missing-input neutral markers can be committed.
    visible_ = BoundValid() && (current_sequence_!=0 || PreviousComparable());
    FillAvailability(tick,scope_availability_);
    return true;
}
void Participant::End(bool normal_return) noexcept
{
    if(!scope_pending_)return;
    const bool persist_availability=normal_return && SessionInputActive() &&
        current_generation_==generation_ && BoundValid();
    if(persist_availability) {
        if(current_sequence_) {
            last_effective_input_tick_=visible_tick_;
            last_effective_input_sequence_=current_sequence_;
        }
        progress_.availability=scope_availability_;
    }
    if (SessionInputActive() && visible_ && normal_return && current_generation_ == generation_ && BoundValid()) {
        previous_ = current_; previous_generation_ = generation_; previous_valid_ = true;
        progress_.last_processed_tick=visible_tick_;
        progress_.last_processed_sequence=(std::max)(progress_.last_processed_sequence,current_sequence_);
        progress_.last_processed_source_sequence=current_sequence_;
        progress_.processed_contributors=progress_.resolved_contributors;
        progress_.last_state=current_;progress_.last_state.edge_flags=0;
        progress_.last_state_tick=visible_tick_;progress_.last_state_generation=generation_;progress_.last_state_valid=true;
        for(std::uint32_t action=0;action<4;++action) if(current_edge_sequences_[action]) {
            auto& edge=progress_.edges[action];edge.processed_sequence=current_edge_sequences_[action];
            edge.processed_tick=visible_tick_;edge.processed_generation=generation_;Count(edge.processed_count);
        }
    } else {
        progress_.last_state_valid=false;
        current_={};current_sequence_=0;current_edge_sequences_.fill(0);
    }
    visible_ = false;scope_pending_=false;scope_availability_={};
}
ClientInputProgress Participant::Progress(std::uint64_t completed_tick) const noexcept
{
    auto value=progress_;
    value.session=session_;value.client=client_;value.ordinal=ordinal_;value.generation=generation_;
    value.next_sequence=sequence_;value.last_accepted_sequence=last_accepted_;value.last_resolved_tick=last_resolved_;
    value.scope_pending=scope_pending_;value.retired=retired_;
    value.current_valid=CurrentEligible(binding_,visible_tick_);
    value.current_state=value.current_valid?current_:InputStateDraftV2{};
    value.availability=scope_pending_?scope_availability_:progress_.availability;
    value.availability.valid_arrival_count=valid_arrival_count_;
    value.availability.silent_ticks=silent_ticks_;
    value.availability.silent_entries=silent_entries_;
    value.availability.silence_recoveries=silence_recoveries_;
    value.availability.timeout_entries=timeout_entries_;
    value.availability.timeout_recoveries=timeout_recoveries_;
    value.availability.has_valid_arrival=current_generation_arrival_;
    if(current_generation_arrival_) {
        value.availability.last_valid_arrival_tick=last_valid_arrival_tick_;
        value.availability.last_valid_arrival_sequence=last_valid_arrival_sequence_;
        value.availability.last_valid_arrival_target_tick=last_valid_arrival_target_tick_;
        value.availability.last_valid_arrival_generation=last_valid_arrival_generation_;
    } else {
        value.availability.last_valid_arrival_tick=0;
        value.availability.last_valid_arrival_sequence=0;
        value.availability.last_valid_arrival_target_tick=0;
        value.availability.last_valid_arrival_generation=0;
    }
    if(!scope_pending_) value.availability.source=CurrentSourceActivity();
    if(retired_) {
        value.availability.effective_available=false;value.availability.neutral_applied=false;
        value.availability.missing_reason=MissingInputReason::Retired;
        value.availability.source=SourceActivity::Retired;
    } else if(!bound_) {
        value.availability.effective_available=false;value.availability.neutral_applied=false;
        value.availability.missing_reason=MissingInputReason::Unbound;
        value.availability.source=SourceActivity::Unbound;
    } else if(!SessionInputActive()) {
        value.availability.effective_available=false;value.availability.neutral_applied=false;
        value.availability.missing_reason=MissingInputReason::SessionInactive;
        value.availability.source=SourceActivity::SessionInactive;
    }
    value.last_state_valid=value.last_state_valid && SessionInputActive() && BoundValid() &&
        value.last_state_generation==generation_;
    if(!value.last_state_valid)value.last_state={};
    // A mid-scope invalidation cancels this client's snapshot, not the fact that
    // Begin already resolved that tick. A rebind cannot enqueue into it again.
    const auto floor=(std::max)(completed_tick,last_resolved_);
    value.future_last_tick=completed_tick>UINT64_MAX-runtime_config_.future_window_ticks?
        UINT64_MAX:completed_tick+runtime_config_.future_window_ticks;
    value.future_first_tick=floor<value.future_last_tick?floor+1:0;
    value.future_window_valid=SessionInputActive() && BoundValid() && value.future_first_tick!=0;
    return value;
}
float Participant::Read(std::uint32_t action, Query query, Consumer consumer,
                        const Binding& exact, std::uint64_t tick) const noexcept
{
    if (!CurrentEligible(exact,tick) || !ConsumerAllows(consumer,action)) return 0.0F;
    if (query == Query::Scalar) return Magnitude(current_,action);
    if (query == Query::Pressed) return Magnitude(current_,action) != 0 ? 1.0F : 0.0F;
    if (query != Query::Triggered) return 0.0F;
    if (action >= 8) return (current_.edge_flags & (1u<<(action-8))) ? 1.0F : 0.0F;
    return PreviousComparable() &&
        Magnitude(previous_,action) == 0 && Magnitude(current_,action) != 0 ? 1.0F : 0.0F;
}
bool Participant::CurrentEligible(const Binding& exact,std::uint64_t tick) const noexcept
{
    return SessionInputActive() && visible_ && BoundValid() && exact==binding_ && tick==visible_tick_ && generation_==current_generation_;
}
bool Participant::SessionInputActive() const noexcept
{
    return !authority_session_ || (authority_session_->AcceptsInput() &&
        authority_session_->MatchesIdentity(session_,client_,ordinal_));
}
bool Participant::PreviousComparable() const noexcept
{return SessionInputActive() && previous_valid_ && previous_generation_==generation_;}
} // namespace isaac_cs_t0::coop_active
