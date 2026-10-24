#include "isaac_cs_t0/authority_tick_pipeline.hpp"

namespace isaac_cs_t0::coop_active {
const char* TickPipelinePhaseName(TickPipelinePhase phase) noexcept
{
    switch(phase) {
    case TickPipelinePhase::Idle:return "Idle";
    case TickPipelinePhase::Draining:return "Draining";
    case TickPipelinePhase::Simulating:return "Simulating";
    case TickPipelinePhase::AwaitPostState:return "AwaitPostState";
    case TickPipelinePhase::Committed:return "Committed";
    case TickPipelinePhase::Published:return "Published";
    case TickPipelinePhase::Failed:return "Failed";
    }
    return "UNKNOWN";
}
bool AuthorityTickPipeline::Configure(const InputRuntimeConfig& config) noexcept
{
    if(configured_ || phase_!=TickPipelinePhase::Idle) return false;
    if(ValidateInputRuntimeConfig(config)!=InputConfigDecision::Accepted) return false;
    budget_=config.admission_attempt_budget;configured_=true;return true;
}
TickDecision AuthorityTickPipeline::Abort(TickDecision reason) noexcept
{
    for(auto& player:players_)player.End(false);
    clock_.Cancel(reason);phase_=TickPipelinePhase::Failed;
    if(session_.AcceptsInput())session_.Fail(SessionCause::RuntimeFailure);
    return reason;
}
TickDecision AuthorityTickPipeline::BeginDrain(const TickBoundary& boundary,bool blocked) noexcept
{
    if(phase_==TickPipelinePhase::Failed)return TickDecision::Closed;
    if(phase_!=TickPipelinePhase::Idle && phase_!=TickPipelinePhase::Published)return Abort(TickDecision::Reentry);
    const auto decision=clock_.CheckStart(boundary,session_.AcceptsInput(),blocked);
    if(decision==TickDecision::Inactive || decision==TickDecision::Blocked || decision==TickDecision::Closed)return decision;
    if(decision!=TickDecision::Started)return Abort(decision);
    if(!budget_ || budget_>kMaximumAdmissionAttemptBudget)return Abort(TickDecision::DrainBudgetExceeded);
    boundary_=boundary;candidate_=clock_.Completed()+1;drained_=0;receipt_={};
    phase_=TickPipelinePhase::Draining;return TickDecision::Started;
}
bool AuthorityTickPipeline::Admit(std::uint32_t ordinal) noexcept
{
    if(phase_!=TickPipelinePhase::Draining || ordinal>=players_.size()) {Abort(TickDecision::Reentry);return false;}
    if(drained_>=budget_) {Abort(TickDecision::DrainBudgetExceeded);return false;}
    ++drained_;return true;
}
Decision AuthorityTickPipeline::AcceptInput(std::uint32_t ordinal,const InputCmdDraftV2& command,std::uint64_t generation) noexcept
{
    if(!Admit(ordinal))return Decision::Invalid;
    const auto decision=players_[ordinal].AcceptInput(command,generation,clock_.Completed());
    // Keep the adopted conflict policy at the common scheduler boundary so the
    // Native wrapper and offline production-chain scenarios observe one rule.
    if(decision==Decision::SequenceConflict && ordinal==0) {
        if(session_.AcceptsInput()) session_.Fail(SessionCause::RuntimeFailure);
        Abort(TickDecision::Cancelled);
    }
    return decision;
}
Decision AuthorityTickPipeline::AcceptInput(std::uint32_t ordinal,std::span<const std::uint8_t> bytes,std::uint64_t generation) noexcept
{
    if(!Admit(ordinal))return Decision::Invalid;
    const auto decision=players_[ordinal].AcceptInput(bytes,generation,clock_.Completed());
    if(decision==Decision::SequenceConflict && ordinal==0) {
        if(session_.AcceptsInput()) session_.Fail(SessionCause::RuntimeFailure);
        Abort(TickDecision::Cancelled);
    }
    return decision;
}
TickDecision AuthorityTickPipeline::BeginSimulation() noexcept
{
    if(phase_!=TickPipelinePhase::Draining)return Abort(TickDecision::Reentry);
    const auto decision=clock_.Begin(boundary_,session_.AcceptsInput(),false);
    if(decision!=TickDecision::Started)return Abort(decision);
    for(auto& player:players_)if(!player.Retired() && !player.Begin(candidate_))return Abort(TickDecision::Reentry);
    phase_=TickPipelinePhase::Simulating;return TickDecision::Started;
}
TickDecision AuthorityTickPipeline::NativeReturned(const TickBoundary& boundary,bool normal) noexcept
{
    if(phase_!=TickPipelinePhase::Simulating)return Abort(TickDecision::Reentry);
    const auto decision=clock_.CheckComplete(boundary,normal,session_.AcceptsInput());
    if(decision!=TickDecision::Committed)return Abort(decision);
    for(auto& player:players_)player.End(true);
    phase_=TickPipelinePhase::AwaitPostState;return TickDecision::Committed;
}
TickDecision AuthorityTickPipeline::FinishPostState(std::uint64_t loop,const TickPostState& observed) noexcept
{
    auto post=observed; // Keep aggregates out of by-value x86 LTCG parameters.
    if(phase_!=TickPipelinePhase::AwaitPostState)return Abort(TickDecision::Reentry);
    if(loop!=boundary_.loop)return Abort(TickDecision::WrongReturn);
    if(post.status!=PostStateStatus::Disabled && post.tick!=candidate_)
        post={PostStateStatus::Unavailable,18,candidate_,0,0};
    if(boundary_.kind==TickKind::OddPlayer && post.status==PostStateStatus::Complete)
        post={PostStateStatus::Gap,2,candidate_,post.attempt,0};
    // NativeReturned already witnessed a valid active normal simulation. Later
    // collector/lifecycle/session changes cannot erase that completed native fact.
    const auto decision=clock_.Complete(boundary_,true,true);
    if(decision!=TickDecision::Committed)return Abort(decision);
    receipt_.tick=clock_.Completed();receipt_.loop=loop;receipt_.kind=boundary_.kind;receipt_.boundary=boundary_;receipt_.post=post;
    for(std::size_t i=0;i<players_.size();++i) {
        const auto p=players_[i].Progress(clock_.Completed());
        receipt_.clients[i]={p.client,p.generation,p.last_state_generation,p.last_accepted_sequence,p.last_processed_sequence,
            p.last_processed_tick,p.last_processed_source_sequence,players_[i].BoundValid() && session_.AcceptsInput(),
            p.availability};
    }
    phase_=TickPipelinePhase::Committed;return TickDecision::Committed;
}
bool AuthorityTickPipeline::PublishCompleted(bool success) noexcept
{
    if(phase_!=TickPipelinePhase::Committed){Abort(TickDecision::Reentry);return false;}
    if(!success){session_.Fail(SessionCause::PublicationFailure);Abort(TickDecision::Cancelled);return false;}
    receipt_.published=true;phase_=TickPipelinePhase::Published;return true;
}
TickDecision AuthorityTickPipeline::EndOuter(std::uint64_t loop) noexcept
{
    (void)loop;
    if(phase_==TickPipelinePhase::Idle || phase_==TickPipelinePhase::Published)return TickDecision::Idle;
    if(phase_==TickPipelinePhase::Failed)return TickDecision::Idle;
    return Abort(TickDecision::MissingReturn);
}
} // namespace isaac_cs_t0::coop_active
