#pragma once

#include <Windows.h>

#include "isaac_cs_t0/loop_ipc.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace isaac_cs_t0::mode {
struct PointerPatch;
struct RelativeCallPatch;
struct EntryPatch;
}

namespace isaac_cs_t0::loop {

struct ObservationContext {
    std::uint64_t loop_iteration = 0;
    std::uint32_t thread_id = 0;
    std::uint32_t main_thread_id = 0;
    Phase phase = Phase::OutsideEnvelope;
    std::uint32_t phase_depth = 0;
};

struct EmbeddedObserverIdentity {
    std::uint32_t target_pid = 0;
    std::uint32_t scenario = 0;
    std::array<std::uint8_t, 32> nonce{};
};

using PhysicalPollObservation = void (*)(bool entering, void* device, bool result,
                                         void* return_address, void* context) noexcept;
using DeviceUpdateCompleteObservation = void (*)(void* device, bool result,
                                                 void* return_address,
                                                 void* context) noexcept;
using PhysicalExitObservation = void (*)(void* device, bool result,
                                         void* return_address,
                                         void* context) noexcept;
using AuthorityObservation = void (*)(bool entering, void* room,
                                      void* return_address,
                                      void* context) noexcept;
using TopUpdateExitObservation = void (*)(void* return_address,
                                          void* context) noexcept;

enum class EmbeddedLifecycleSignal : std::uint32_t {
    NewRoom = 1,
    AlternateEnvelope = 2,
    RoomLifecycle = 3,
    RunTeardown = 4,
    RoomChange = 5,
};

using LifecycleObservation = void (*)(EmbeddedLifecycleSignal signal, bool entering,
                                      void* owner, void* return_address,
                                      void* context) noexcept;

struct EmbeddedObserverHooks {
    isaac_cs_t0::mode::RelativeCallPatch* relative = nullptr;
    std::size_t relative_count = 0;
    // Required for strict_ordered_transaction. Values are catalog-global ordinals.
    const std::uint32_t* relative_ordinals = nullptr;
    isaac_cs_t0::mode::PointerPatch* pointers = nullptr;
    std::size_t pointer_count = 0;
    const std::uint32_t* pointer_ordinals = nullptr;
    // Install V5 0..31 and the caller-owned suffix in one exact ordinal sequence.
    // Embedded restore removes the suffix in reverse; StopEmbeddedObserver then
    // removes the V5 prefix in reverse.
    bool strict_ordered_transaction = false;
    std::array<std::uint8_t, 32>* strict_slot_snapshot_before = nullptr;
    PhysicalPollObservation physical_poll = nullptr;
    void* physical_poll_context = nullptr;
    DeviceUpdateCompleteObservation device_update_complete = nullptr;
    void* device_update_complete_context = nullptr;
    PhysicalExitObservation physical_exit = nullptr;
    void* physical_exit_context = nullptr;
    AuthorityObservation authority = nullptr;
    void* authority_context = nullptr;
    TopUpdateExitObservation top_update_exit = nullptr;
    void* top_update_exit_context = nullptr;
    LifecycleObservation lifecycle = nullptr;
    void* lifecycle_context = nullptr;
    isaac_cs_t0::mode::EntryPatch* entries = nullptr;
    std::size_t entry_count = 0;
    const std::uint32_t* entry_ordinals = nullptr;
};

namespace detail {

inline constexpr std::uint32_t kStandaloneScenarioMaximum = 10;
inline constexpr std::uint32_t kEmbeddedActiveScenarioMaximum = 16;

inline bool CompanionScenarioAllowed(std::uint32_t scenario,
                                     bool embedded) noexcept
{
    const std::uint32_t maximum =
        embedded ? kEmbeddedActiveScenarioMaximum : kStandaloneScenarioMaximum;
    return scenario >= 1 && scenario <= maximum;
}

enum class AlternateEnvelopeTransition : std::uint32_t {
    None = 0,
    Open = 1,
    Close = 2,
    Invalid = 3,
};

// The ordinary frame path calls the alternate owner even when no loading transaction
// exists.  Only a nested state-apply proves that the reverse-confirmed alternate
// owner/state/room/tail envelope is active.
struct AlternateEnvelopeTracker {
    bool owner_active = false;
    bool envelope_open = false;

    AlternateEnvelopeTransition BeginOwner() noexcept
    {
        if (owner_active)
            return AlternateEnvelopeTransition::Invalid;
        owner_active = true;
        envelope_open = false;
        return AlternateEnvelopeTransition::None;
    }

    AlternateEnvelopeTransition ObserveStateApply() noexcept
    {
        if (!owner_active || envelope_open)
            return AlternateEnvelopeTransition::None;
        envelope_open = true;
        return AlternateEnvelopeTransition::Open;
    }

    AlternateEnvelopeTransition EndOwner() noexcept
    {
        if (!owner_active)
            return AlternateEnvelopeTransition::Invalid;
        const bool close = envelope_open;
        owner_active = false;
        envelope_open = false;
        return close ? AlternateEnvelopeTransition::Close
                     : AlternateEnvelopeTransition::None;
    }

    void Reset() noexcept
    {
        owner_active = false;
        envelope_open = false;
    }
};

inline bool CompanionIdentityMatches(
    std::uint32_t target_pid, std::uint32_t scenario,
    const std::array<std::uint8_t, 32>& nonce,
    const EmbeddedObserverIdentity& expected) noexcept
{
    return target_pid == expected.target_pid && scenario == expected.scenario &&
           nonce == expected.nonce;
}

inline bool CompanionMappingIdentityAllowed(
    std::uint32_t target_pid, std::uint32_t scenario,
    const std::array<std::uint8_t, 32>& nonce, bool embedded,
    const EmbeddedObserverIdentity* expected) noexcept
{
    if (!CompanionScenarioAllowed(scenario, embedded) ||
        embedded != (expected != nullptr))
        return false;
    return !expected ||
           CompanionIdentityMatches(target_pid, scenario, nonce, *expected);
}

} // namespace detail

void SetModule(HMODULE module) noexcept;
void ProcessDetach() noexcept;

// The input observer uses the same V5 companion mapping and installs its pointer slots in the
// same suspended-thread transaction as the unchanged V5 27-call/5-pointer hook set.  The
// caller-owned patches receive their captured originals on success.  Shutdown first restores
// only those additional slots, drains their wrappers, and then restores/drains the V5 set.
DWORD StartEmbeddedObserver(const wchar_t* mapping_name,
                            isaac_cs_t0::mode::PointerPatch* additional_pointers,
                            std::size_t additional_pointer_count,
                            const EmbeddedObserverIdentity& expected_identity) noexcept;
DWORD StartEmbeddedObserver(const wchar_t* mapping_name,
                            const EmbeddedObserverHooks& additional_hooks,
                            const EmbeddedObserverIdentity& expected_identity) noexcept;
DWORD RestoreEmbeddedInputHooks() noexcept;
DWORD StopEmbeddedObserver() noexcept;
bool VerifyEmbeddedObserverRestored(std::uint32_t& verified_slot_count) noexcept;
using RestorationObservation = void (*)(std::uint32_t rva,const std::uint8_t* expected,
    const std::uint8_t* actual,std::size_t size,void* context) noexcept;
// Raw restoration evidence for a resident owner; performs no digest calculation.
bool ObserveEmbeddedRestoredTargets(RestorationObservation observer,void* context) noexcept;
bool SnapshotEmbeddedObserverSlots(std::array<std::uint8_t, 32>& digest) noexcept;

// Snapshot of the nested loop phase visible to observation-only companion hooks on this thread.
// No V5 event or IPC field depends on this context, preserving the schema-5 trace contract.
ObservationContext CurrentObservationContext() noexcept;
// A typed health query for the embedded active runtime.  It is intentionally read-only and
// does not alter the V5 IPC contract or arm state.
bool CompanionHealthy() noexcept;

extern "C" DWORD WINAPI IsaacCsT0LoopInitialize(const wchar_t* mapping_name);
extern "C" DWORD WINAPI IsaacCsT0LoopShutdown(void* unused);

} // namespace isaac_cs_t0::loop
