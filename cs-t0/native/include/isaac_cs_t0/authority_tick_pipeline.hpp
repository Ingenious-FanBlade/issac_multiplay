#pragma once
#include "isaac_cs_t0/authority_session.hpp"
#include "isaac_cs_t0/authority_tick.hpp"
#include "isaac_cs_t0/coop_input.hpp"

namespace isaac_cs_t0::coop_active {
// Current source worst case: host 2 + script 130 + diagnostic control 2 = 134.
// Keep a conservative cap of 258 including all duplicate/invalid attempts.
inline constexpr std::uint32_t kTickDrainAttemptBudget=2+4*input::kDrainBudgetPerTick;
inline constexpr std::uint32_t kMaximumAdmissionAttemptBudget=input::kRuntimeAdmissionAttemptMaximum;
enum class TickPipelinePhase : std::uint32_t { Idle,Draining,Simulating,AwaitPostState,Committed,Published,Failed };
enum class PostStateStatus : std::uint32_t { Disabled,Complete,Gap,Backpressure,Unavailable };
struct TickPostState {
    PostStateStatus status=PostStateStatus::Disabled;
    std::uint32_t reason=0;
    std::uint64_t tick=0,attempt=0,capture=0;
};
struct TickClientReceipt {
    std::uint64_t client=0,generation=0,processed_generation=0,accepted=0,processed=0,processed_tick=0,source=0;
    bool current_binding=false;
    ClientInputAvailability availability{};
};
struct TickReceipt {
    std::uint64_t tick=0,loop=0;
    TickKind kind=TickKind::StandardRoom;
    TickBoundary boundary{};
    TickPostState post{};
    std::array<TickClientReceipt,2> clients{};
    bool published=false;
};
struct TickPipelineStatus {
    TickPipelinePhase phase;
    std::uint64_t candidate;
    std::uint32_t drained,budget;
};
const char* TickPipelinePhaseName(TickPipelinePhase phase) noexcept;

// Main-thread orchestration of the actual production Session/Clock/Participants.
// This owns no game objects and calls no native body: existing wrappers remain
// the unique original-call owner. Post-state is copied before FinishPostState;
// copied buffers and receipts become public only after its committed result.
class AuthorityTickPipeline final {
public:
    AuthorityTickPipeline(AuthoritySession& session,AuthorityTickClock& clock,
                          std::array<Participant,2>& players,
                          std::uint32_t budget=kTickDrainAttemptBudget) noexcept
        :session_(session),clock_(clock),players_(players),budget_(budget) {}
    // One-shot before the first scope. The full config is validated again here
    // so a direct constructor budget cannot bypass the supported range.
    bool Configure(const InputRuntimeConfig& config) noexcept;
    TickDecision BeginDrain(const TickBoundary& boundary,bool blocked) noexcept;
    Decision AcceptInput(std::uint32_t ordinal,const InputCmdDraftV2& command,std::uint64_t generation) noexcept;
    Decision AcceptInput(std::uint32_t ordinal,std::span<const std::uint8_t> bytes,std::uint64_t generation) noexcept;
    TickDecision BeginSimulation() noexcept;
    TickDecision NativeReturned(const TickBoundary& boundary,bool normal) noexcept;
    TickDecision FinishPostState(std::uint64_t loop,const TickPostState& observed) noexcept;
    bool PublishCompleted(bool success) noexcept;
    TickDecision EndOuter(std::uint64_t loop) noexcept;
    TickDecision Abort(TickDecision reason) noexcept;
    std::uint64_t SimulationTick() const noexcept {return phase_==TickPipelinePhase::Simulating?clock_.Current():0;}
    TickPipelineStatus Status() const noexcept {return {phase_,candidate_,drained_,budget_};}
    std::uint32_t RemainingAdmissionAttempts() const noexcept
    {return phase_==TickPipelinePhase::Draining && drained_<budget_?budget_-drained_:0;}
    const TickReceipt& Receipt() const noexcept {return receipt_;}
private:
    bool Admit(std::uint32_t ordinal) noexcept;
    AuthoritySession& session_;AuthorityTickClock& clock_;
    std::array<Participant,2>& players_;
    std::uint32_t budget_,drained_=0;
    std::uint64_t candidate_=0;
    bool configured_=false;
    TickBoundary boundary_{};
    TickPipelinePhase phase_=TickPipelinePhase::Idle;
    TickReceipt receipt_{};
};
} // namespace isaac_cs_t0::coop_active
