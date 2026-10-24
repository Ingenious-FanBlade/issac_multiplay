#pragma once

#include <Windows.h>

#include "isaac_cs_t0/active_ipc.hpp"
#include "isaac_cs_t0/loop_runtime.hpp"

#include <cstdint>

namespace isaac_cs_t0::active {

namespace detail {

constexpr std::uint32_t kPauseMissingAuthorityThreshold = 2;

enum class PauseGapTransition : std::uint32_t {
    None = 0,
    Enter = 1,
    Exit = 2,
};

// One ordinary top-level update without StandardRoom authority is expected on the
// alternating live frame shape.  A pause starts only after a consecutive gap that is
// longer than that normal cadence.
struct PauseGapTracker {
    std::uint32_t consecutive_missing = 0;
    bool paused = false;

    PauseGapTransition AuthorityObserved() noexcept
    {
        consecutive_missing = 0;
        if (!paused)
            return PauseGapTransition::None;
        paused = false;
        return PauseGapTransition::Exit;
    }

    PauseGapTransition TopUpdateCompleted(bool authority_seen) noexcept
    {
        if (authority_seen)
        {
            consecutive_missing = 0;
            return PauseGapTransition::None;
        }
        if (paused)
            return PauseGapTransition::None;
        if (consecutive_missing < kPauseMissingAuthorityThreshold)
            ++consecutive_missing;
        if (consecutive_missing != kPauseMissingAuthorityThreshold)
            return PauseGapTransition::None;
        paused = true;
        return PauseGapTransition::Enter;
    }

    void Reset() noexcept
    {
        consecutive_missing = 0;
        paused = false;
    }

    bool BlocksInput() const noexcept
    {
        return paused;
    }
};

// The reverse-confirmed loading transaction ends with AlternateOwnerExit, before
// the first StandardRoom authority callback is guaranteed to resume.  Keep that
// exact lifecycle boundary for evidence, but quarantine physical submission until
// authority is observed again.  The authority-less recovery interval is part of
// loading recovery, not a pause gap: treating it as PauseEnter would retire the
// first post-loading command before its target tick.
struct PostLoadingAuthorityGate {
    bool awaiting_authority = false;

    void LoadingEntered() noexcept
    {
        awaiting_authority = false;
    }

    void LoadingExited() noexcept
    {
        awaiting_authority = true;
    }

    bool AuthorityObserved() noexcept
    {
        if (!awaiting_authority)
            return false;
        awaiting_authority = false;
        return true;
    }

    bool BlocksInput() const noexcept
    {
        return awaiting_authority;
    }

    void Reset() noexcept
    {
        awaiting_authority = false;
    }
};

inline PauseGapTransition AuthorityObservedWithLoadingRecovery(
    PauseGapTracker& pause,
    PostLoadingAuthorityGate& loading_recovery) noexcept
{
    if (loading_recovery.AuthorityObserved())
    {
        pause.Reset();
        return PauseGapTransition::None;
    }
    return pause.AuthorityObserved();
}

inline PauseGapTransition TopUpdateCompletedWithLoadingRecovery(
    PauseGapTracker& pause,
    const PostLoadingAuthorityGate& loading_recovery,
    bool authority_seen) noexcept
{
    return loading_recovery.BlocksInput()
               ? PauseGapTransition::None
               : pause.TopUpdateCompleted(authority_seen);
}

enum class LifecycleInputDomain : std::uint32_t {
    Loading = 1,
    Room = 2,
};

enum class LifecycleGateTransition : std::uint32_t {
    Entered = 1,
    Nested = 2,
    Exited = 3,
    RemainsNested = 4,
    Quarantined = 5,
};

// RoomChange/RoomLifecycle wrappers can execute for multiple owner-only attempts
// around one actual transition. They must still clear the owner generation, but
// only the nested NewRoom callback is a unique, evidence-bearing room lifecycle
// label. Lifecycle::None is the existing schema-4 generic clear code and keeps
// the numeric codebook and shared-memory ABI unchanged.
constexpr active_abi::Lifecycle PublishedRoomLifecycle(
    loop::EmbeddedLifecycleSignal signal, bool entering) noexcept
{
    return signal == loop::EmbeddedLifecycleSignal::NewRoom
               ? (entering ? active_abi::Lifecycle::RoomEnter
                           : active_abi::Lifecycle::RoomExit)
               : active_abi::Lifecycle::None;
}

// Physical polling can occur while a V5 lifecycle envelope is still open.  Keep
// those envelopes independent and depth-counted: room transitions may nest (for
// example owner -> NewRoom -> room update), and a loading envelope can overlap a
// room callback.  An unmatched exit or counter overflow is not promoted to Unsafe
// by itself, but permanently quarantines input submission for this load.  That is
// the fail-closed response for a capture-mid-transition ambiguity while still
// allowing the ordinary shutdown/restore path to run.
struct LifecycleSubmissionGate {
    std::uint32_t loading_depth = 0;
    std::uint32_t room_depth = 0;
    bool quarantined = false;
    bool retired = false;

    LifecycleGateTransition Enter(LifecycleInputDomain domain) noexcept
    {
        if (quarantined || retired)
            return LifecycleGateTransition::Quarantined;
        std::uint32_t& depth = Depth(domain);
        if (depth == UINT32_MAX)
        {
            quarantined = true;
            return LifecycleGateTransition::Quarantined;
        }
        ++depth;
        return depth == 1 ? LifecycleGateTransition::Entered
                          : LifecycleGateTransition::Nested;
    }

    LifecycleGateTransition Exit(LifecycleInputDomain domain) noexcept
    {
        if (quarantined || retired)
            return LifecycleGateTransition::Quarantined;
        std::uint32_t& depth = Depth(domain);
        if (depth == 0)
        {
            quarantined = true;
            return LifecycleGateTransition::Quarantined;
        }
        --depth;
        return depth == 0 ? LifecycleGateTransition::Exited
                          : LifecycleGateTransition::RemainsNested;
    }

    bool BlocksInput(bool pause_blocked = false) const noexcept
    {
        return pause_blocked || quarantined || retired || loading_depth != 0 ||
               room_depth != 0;
    }

    bool Healthy() const noexcept
    {
        return !quarantined;
    }

    void Retire() noexcept
    {
        retired = true;
    }

    void Reset() noexcept
    {
        loading_depth = 0;
        room_depth = 0;
        quarantined = false;
        retired = false;
    }

private:
    std::uint32_t& Depth(LifecycleInputDomain domain) noexcept
    {
        return domain == LifecycleInputDomain::Loading ? loading_depth
                                                       : room_depth;
    }
};

inline LifecycleGateTransition ObserveLoadingEnvelope(
    LifecycleSubmissionGate& lifecycle,
    PostLoadingAuthorityGate& loading_recovery,
    bool entering) noexcept
{
    const auto transition = entering
                                ? lifecycle.Enter(LifecycleInputDomain::Loading)
                                : lifecycle.Exit(LifecycleInputDomain::Loading);
    if (transition == LifecycleGateTransition::Entered)
        loading_recovery.LoadingEntered();
    else if (transition == LifecycleGateTransition::Exited)
        loading_recovery.LoadingExited();
    return transition;
}

inline bool PhysicalSubmissionAllowedAfterBinding(
    const PauseGapTracker& pause,
    const LifecycleSubmissionGate& lifecycle,
    const PostLoadingAuthorityGate& loading_recovery) noexcept
{
    return !lifecycle.BlocksInput(pause.BlocksInput()) &&
           !loading_recovery.BlocksInput();
}

inline bool PhysicalOwnerReadyForSubmission(active_abi::Scenario,
                                            bool player_bound) noexcept
{
    // Rust preflight proves a unique owner before injection, but Native must bind
    // that same Player through the hooked PlayerUpdate path before it may read or
    // submit a physical projection.  The first physical poll can precede that
    // wrapper in a newly installed loop; treating an unbound projection as a
    // protocol failure makes startup timing decide whether the client survives.
    return player_bound;
}

inline bool LoadingAuthorityPassthrough(active_abi::Scenario scenario,
                                        bool owner_ready) noexcept
{
    return scenario == active_abi::Scenario::Loading && !owner_ready;
}

inline bool PreserveLoadingPlayerBootstrap(active_abi::Scenario scenario,
                                           bool player_stable,
                                           bool device_bound) noexcept
{
    return scenario == active_abi::Scenario::Loading && player_stable &&
           !device_bound;
}

enum class MainThreadBinding : std::uint32_t {
    Bound = 0,
    AwaitingAnchor = 1,
    WrongThread = 2,
    Conflict = 3,
};

inline MainThreadBinding BindTrustedMainThread(
    volatile LONG& published_thread, std::uint32_t trusted_main_thread,
    std::uint32_t current_thread) noexcept
{
    if (trusted_main_thread == 0) {
        const LONG observed = InterlockedCompareExchange(&published_thread, 0, 0);
        return observed == 0 ? MainThreadBinding::AwaitingAnchor
                             : MainThreadBinding::Conflict;
    }
    if (current_thread != trusted_main_thread)
        return MainThreadBinding::WrongThread;
    const LONG observed = InterlockedCompareExchange(
        &published_thread, static_cast<LONG>(trusted_main_thread), 0);
    return observed == 0 || observed == static_cast<LONG>(trusted_main_thread)
               ? MainThreadBinding::Bound
               : MainThreadBinding::Conflict;
}

template <typename Invoke>
bool PassthroughConsumerWithoutSyntheticAuthority(bool override_enabled,
                                                  bool authority_scope,
                                                  bool command_valid,
                                                  Invoke&& invoke) noexcept
{
    if (override_enabled && authority_scope && command_valid)
        return false;
    invoke();
    return true;
}

enum class ShutdownStageResult : std::uint32_t {
    Complete = 0,
    CallbackDrainFailed = 1,
    ActiveRestoreFailed = 2,
    ActiveDrainFailed = 3,
    CompanionStopFailed = 4,
    SlotVerificationFailed = 5,
};

enum class ClientRetirementDisposition : std::uint32_t {
    Continue = 0,
    Graceful = 1,
    Unsafe = 2,
};

inline ClientRetirementDisposition ClassifyClientRetirement(
    bool client_retired, active_abi::ErrorCode error) noexcept
{
    if (error != active_abi::ErrorCode::None)
        return ClientRetirementDisposition::Unsafe;
    return client_retired ? ClientRetirementDisposition::Graceful
                          : ClientRetirementDisposition::Continue;
}

template <typename GracefulRetire, typename UnsafeRetire>
inline ClientRetirementDisposition HandleClientRetirement(
    bool client_retired, active_abi::ErrorCode error,
    GracefulRetire&& graceful_retire, UnsafeRetire&& unsafe_retire) noexcept
{
    const auto disposition = ClassifyClientRetirement(client_retired, error);
    if (disposition == ClientRetirementDisposition::Graceful)
        graceful_retire();
    else if (disposition == ClientRetirementDisposition::Unsafe)
        unsafe_retire(error);
    return disposition;
}

template <typename DrainCallbacks, typename RetireClient>
inline bool DrainCallbacksBeforeRetirement(
    DrainCallbacks&& drain_callbacks, RetireClient&& retire_client) noexcept
{
    if (!drain_callbacks())
        return false;
    retire_client();
    return true;
}

template <typename DrainCallbacks, typename RestoreActive, typename DrainActive,
          typename StopCompanion, typename VerifySlots>
inline ShutdownStageResult RunShutdownStages(
    DrainCallbacks&& drain_callbacks, RestoreActive&& restore_active,
    DrainActive&& drain_active, StopCompanion&& stop_companion,
    VerifySlots&& verify_slots) noexcept
{
    // Disable admission before entering this helper.  The first stage drains every
    // callback that could already have observed admission; only then may the
    // dispatcher/adapter be retired or any patch be restored.
    if (!drain_callbacks())
        return ShutdownStageResult::CallbackDrainFailed;
    if (!restore_active())
        return ShutdownStageResult::ActiveRestoreFailed;
    if (!drain_active())
        return ShutdownStageResult::ActiveDrainFailed;
    if (!stop_companion())
        return ShutdownStageResult::CompanionStopFailed;
    if (!verify_slots())
        return ShutdownStageResult::SlotVerificationFailed;
    return ShutdownStageResult::Complete;
}

#ifdef ISAAC_CS_T0_TESTING
struct AbiOriginalOnceProbe {
    std::uint32_t logical_calls[6]{};
    std::uint32_t player_calls{};
    std::uint32_t consumer_calls[6]{};
    bool keyboard_pressed_result{};
    bool keyboard_triggered_result{};
    bool gamepad_pressed_result{};
    bool gamepad_triggered_result{};
    float keyboard_scalar_result{};
    float gamepad_scalar_result{};
    std::int32_t item_slot{};
};

bool RunAbiOriginalOnceProbe(AbiOriginalOnceProbe& probe) noexcept;

struct ConflictRetirementRuntimeProbe {
    std::uint32_t conflict_event_count{};
    std::uint32_t protocol_event_count{};
    bool conflict_event_order{};
    bool conflict_witness_unchanged{};
    bool conflict_has_no_apply_or_failure{};
    bool conflict_state_remains_ready{};
    bool conflict_override_disabled{};
    bool conflict_adapter_passthrough{};
    bool conflict_dispatcher_retired{};
    bool post_retirement_callbacks_are_silent{};
    bool protocol_state_is_unsafe{};
    bool protocol_error_preserved{};
    bool protocol_failure_published{};
};

bool RunConflictRetirementRuntimeProbe(
    ConflictRetirementRuntimeProbe& probe) noexcept;
#endif

} // namespace detail

void SetModule(HMODULE module) noexcept;
void ProcessDetach() noexcept;
bool Active() noexcept;

extern "C" DWORD WINAPI IsaacCsT0InputActiveInitialize(const wchar_t* mapping_name);
extern "C" DWORD WINAPI IsaacCsT0InputActiveShutdown(void* unused);

} // namespace isaac_cs_t0::active
