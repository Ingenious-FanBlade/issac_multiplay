#pragma once
#include <array>
#include <atomic>
#include <cstdint>

namespace isaac_cs_t0::coop_active {
enum class SessionState : std::uint32_t { Dormant,Negotiating,Warmup,Active,Retiring,Retired,Failed };
enum class SessionCause : std::uint32_t {
    None,Configuration,NegotiationStarted,Prepared,BindingsReady,Stop,RunTeardown,
    RuntimeFailure,ControllerLost,RestoreFailure,PublicationFailure,HookFailure,CleanupComplete
};
struct SessionIdentity {
    std::array<std::uint8_t,16> session{};
    std::array<std::uint64_t,2> clients{}; // Frozen ordinal 0=A, 1=B.
};
struct SessionStatus {SessionState state;SessionCause cause;};
const char* SessionStateName(SessionState state) noexcept;
const char* SessionCauseName(SessionCause cause) noexcept;

// Process-resident, one-shot session. Start/Prepared and participant initialization
// belong to the initializer before publishing hooks. Activate and normal retirement
// belong to the game main thread. Fail may close admission from any callback;
// cleanup completion is published only after the existing drain/restore sequence.
// Identity is initialized before callbacks become visible and is never rewritten.
class AuthoritySession final {
public:
    bool Start(const SessionIdentity& identity) noexcept;
    bool Prepared() noexcept;
    bool Activate(bool host_bound,bool remote_bound) noexcept;
    bool BeginRetirement(SessionCause cause) noexcept;
    bool CompleteRetirement(bool restored_and_drained) noexcept;
    bool Fail(SessionCause cause) noexcept;
    SessionStatus Status() const noexcept;
    const SessionIdentity& Identity() const noexcept {return identity_;}
    bool AcceptsInput() const noexcept {return Status().state==SessionState::Active;}
    bool AllowsBinding() const noexcept;
    bool MatchesIdentity(const std::array<std::uint8_t,16>& session,
                         std::uint64_t client,std::uint32_t ordinal) const noexcept;
private:
    static constexpr std::uint32_t Pack(SessionState state,SessionCause cause) noexcept
    {return static_cast<std::uint32_t>(state)|(static_cast<std::uint32_t>(cause)<<8);}
    bool Move(SessionState from,SessionState to,SessionCause cause) noexcept;
    std::atomic<std::uint32_t> status_{Pack(SessionState::Dormant,SessionCause::None)};
    SessionIdentity identity_{};
};
} // namespace isaac_cs_t0::coop_active
