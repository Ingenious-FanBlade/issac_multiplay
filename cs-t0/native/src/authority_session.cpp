#include "isaac_cs_t0/authority_session.hpp"
#include <algorithm>

namespace isaac_cs_t0::coop_active {
const char* SessionStateName(SessionState state) noexcept
{
    switch(state) {
    case SessionState::Dormant:return "Dormant";
    case SessionState::Negotiating:return "Negotiating";
    case SessionState::Warmup:return "Warmup";
    case SessionState::Active:return "Active";
    case SessionState::Retiring:return "Retiring";
    case SessionState::Retired:return "Retired";
    case SessionState::Failed:return "Failed";
    }
    return "UNKNOWN";
}
const char* SessionCauseName(SessionCause cause) noexcept
{
    switch(cause) {
    case SessionCause::None:return "None";
    case SessionCause::Configuration:return "Configuration";
    case SessionCause::NegotiationStarted:return "NegotiationStarted";
    case SessionCause::Prepared:return "Prepared";
    case SessionCause::BindingsReady:return "BindingsReady";
    case SessionCause::Stop:return "Stop";
    case SessionCause::RunTeardown:return "RunTeardown";
    case SessionCause::RuntimeFailure:return "RuntimeFailure";
    case SessionCause::ControllerLost:return "ControllerLost";
    case SessionCause::RestoreFailure:return "RestoreFailure";
    case SessionCause::PublicationFailure:return "PublicationFailure";
    case SessionCause::HookFailure:return "HookFailure";
    case SessionCause::CleanupComplete:return "CleanupComplete";
    }
    return "UNKNOWN";
}
SessionStatus AuthoritySession::Status() const noexcept
{
    const auto status=status_.load(std::memory_order_acquire);
    return {static_cast<SessionState>(status&0xFFu),static_cast<SessionCause>(status>>8)};
}
bool AuthoritySession::Move(SessionState from,SessionState to,SessionCause cause) noexcept
{
    auto status=status_.load(std::memory_order_acquire);
    for(;;) {
        const auto state=static_cast<SessionState>(status&0xFFu);
        if(state==to) return true;
        if(state!=from) return false;
        if(status_.compare_exchange_weak(status,Pack(to,cause),std::memory_order_acq_rel)) return true;
    }
}
bool AuthoritySession::Start(const SessionIdentity& identity) noexcept
{
    // Single initializer, before any hook publication. There is deliberately no reset.
    if(Status().state!=SessionState::Dormant) return false;
    if(!identity.clients[0] || !identity.clients[1] || identity.clients[0]==identity.clients[1] ||
       std::all_of(identity.session.begin(),identity.session.end(),[](auto byte){return byte==0;})) {
        Fail(SessionCause::Configuration);return false;
    }
    identity_=identity;
    return Move(SessionState::Dormant,SessionState::Negotiating,SessionCause::NegotiationStarted);
}
bool AuthoritySession::Prepared() noexcept
{return Move(SessionState::Negotiating,SessionState::Warmup,SessionCause::Prepared);}
bool AuthoritySession::Activate(bool host_bound,bool remote_bound) noexcept
{
    if(AcceptsInput()) return true; // Initial readiness is not a per-tick barrier.
    return host_bound && remote_bound && Move(SessionState::Warmup,SessionState::Active,SessionCause::BindingsReady);
}
bool AuthoritySession::BeginRetirement(SessionCause cause) noexcept
{
    if(cause!=SessionCause::Stop && cause!=SessionCause::RunTeardown) return false;
    auto status=status_.load(std::memory_order_acquire);
    for(;;) {
        const auto state=static_cast<SessionState>(status&0xFFu);
        if(state==SessionState::Failed) return false;
        if(state==SessionState::Retiring || state==SessionState::Retired) return true;
        if(status_.compare_exchange_weak(status,Pack(SessionState::Retiring,cause),std::memory_order_acq_rel)) return true;
    }
}
bool AuthoritySession::CompleteRetirement(bool restored_and_drained) noexcept
{
    if(!restored_and_drained) return false;
    if(Status().state==SessionState::Failed) return true; // Cleanup never erases failure.
    return Move(SessionState::Retiring,SessionState::Retired,SessionCause::CleanupComplete);
}
bool AuthoritySession::Fail(SessionCause cause) noexcept
{
    if(cause!=SessionCause::Configuration && cause!=SessionCause::RuntimeFailure &&
       cause!=SessionCause::ControllerLost && cause!=SessionCause::RestoreFailure &&
       cause!=SessionCause::PublicationFailure && cause!=SessionCause::HookFailure) return false;
    auto status=status_.load(std::memory_order_acquire);
    for(;;) {
        const auto state=static_cast<SessionState>(status&0xFFu);
        if(state==SessionState::Failed || state==SessionState::Retired) return false;
        if(status_.compare_exchange_weak(status,Pack(SessionState::Failed,cause),std::memory_order_acq_rel)) return true;
    }
}
bool AuthoritySession::AllowsBinding() const noexcept
{
    const auto state=Status().state;
    return state==SessionState::Warmup || state==SessionState::Active;
}
bool AuthoritySession::MatchesIdentity(const std::array<std::uint8_t,16>& session,
    std::uint64_t client,std::uint32_t ordinal) const noexcept
{
    return AllowsBinding() && ordinal<2 && session==identity_.session && client==identity_.clients[ordinal];
}
} // namespace isaac_cs_t0::coop_active
