#pragma once

#include <Windows.h>

#include <array>
#include <cstddef>
#include <cstdint>

namespace isaac_cs_t0::coop_passive {

struct SharedCoopPassiveBlock;

void SetModule(HMODULE module) noexcept;
void ProcessDetach() noexcept;
bool Active() noexcept;

extern "C" DWORD WINAPI IsaacCsT0CoopPassiveInitialize(const wchar_t* mapping_name);
extern "C" DWORD WINAPI IsaacCsT0CoopPassiveShutdown(void* unused);

namespace detail {

// A stable, non-address-bearing reason code for topology rejection. Rejected
// manager passes never become positive evidence or terminate the observer; a
// fixed digest of this code is carried by the existing manager-end event.
enum class TopologyDiagnostic : std::uint32_t {
    None = 0,
    RunNotActive = 1,
    ManagerOwnerMismatch = 2,
    OnlineBackendPresent = 3,
    OnlineRosterHeaderInvalid = 4,
    OnlineRosterPresent = 5,
    DeviceVectorInvalid = 6,
    InvalidDeviceEntry = 7,
    UnknownDeviceVtable = 8,
    PlayerVectorInvalid = 9,
    PlayerVectorEmpty = 10,
    UnexpectedThirdPlayer = 11,
    InvalidPlayerEntry = 12,
    UnknownPlayerVtable = 13,
    UnknownPlayerCol = 14,
    TwinOrPossessor = 15,
    PlayerOrdinalMismatch = 16,
    PlayerAssignmentPending = 17,
    AssignmentNotUnique = 18,
    DuplicatePlayer = 19,
    DuplicateAssignedDevice = 20,
    HudMismatch = 21,
    StructuralDoubleReadDrift = 22,
    AccessViolation = 23,
    InvalidOnlineManager = 24,
    OfflineOnlineFrameNonzero = 25,
};

// Returns a stable, unkeyed, domain-separated digest for a rejected topology
// reason. None has no diagnostic representation and returns the all-zero
// digest. The digest contains no address or other runtime identity and can be
// decoded offline by comparing it with the finite TopologyDiagnostic codebook.
std::array<std::uint8_t, 32> TopologyDiagnosticDigest(
    TopologyDiagnostic diagnostic) noexcept;

#if defined(ISAAC_CS_T0_TESTING)
// Raw integration seam: the independent harness owns the memory image, the
// mapping and original functions. No assertion result is produced by runtime.
enum class DeviceRebindHarnessFault : std::uint32_t {
    None,
    ObservationDisabled,
    WrongThread,
    ReadException,
    RingFull,
    PossessorStop,
    Unsafe,
    LifecycleEnter,
    LifecycleExit,
    MenuTeardown,
    AdvanceGeneration,
    SkipManagerPass,
};
bool BeginDeviceRebindHarness(std::uint8_t* main_base,
                              SharedCoopPassiveBlock* shared,
                              void* assignment_original,
                              void* manager_original) noexcept;
void InvokeDeviceRebindAssignment(std::uint32_t slot, void* self,
                                  std::int32_t controller_id,
                                  std::uint32_t propagate) noexcept;
void* DeviceRebindAssignmentWrapperAddress(std::uint32_t slot) noexcept;
bool SetDeviceRebindHarnessOrdinaryOriginals(void* spawn_original,
                                            void* hud_original) noexcept;
void* DeviceRebindOrdinaryWrapperAddress(std::uint32_t slot) noexcept;
std::uint32_t InvokeDeviceRebindManager(void* manager) noexcept;
void SetDeviceRebindHarnessFault(DeviceRebindHarnessFault fault) noexcept;
bool DrainDeviceRebindHarness() noexcept;
void EndDeviceRebindHarness() noexcept;

// Only the low-level shutdown dependencies are replaceable. The harness still
// executes the production Shutdown control flow, private cleanup, Event and
// Publish. A null callback uses the real dependency (including callback drain).
struct DeviceRebindShutdownDependencies {
    void* context = nullptr;
    bool (*wait_for_callbacks_drained)(void*) noexcept = nullptr;
    DWORD (*restore_embedded_input_hooks)(void*) noexcept = nullptr;
    bool (*verify_additional_targets_restored)(void*) noexcept = nullptr;
    DWORD (*stop_embedded_observer)(void*) noexcept = nullptr;
    bool (*verify_embedded_observer_restored)(void*, std::uint32_t&) noexcept = nullptr;
    bool (*snapshot_embedded_observer_slots)(
        void*, std::array<std::uint8_t, 32>&) noexcept = nullptr;
};
DWORD InvokeDeviceRebindShutdownHarness(
    const DeviceRebindShutdownDependencies& dependencies) noexcept;
bool SetDeviceRebindHarnessGeneration(std::uint64_t generation) noexcept;
bool PublishDeviceRebindHarnessInitialization() noexcept;

// Raw test-only identity values, including unused array elements. These are
// never serialized or exported by the production DLL; assertions belong to the
// independent harness, not to the snapshot implementation.
struct DeviceRebindTopologySnapshot {
    std::array<std::uintptr_t, 4> roots{};
    std::uint32_t online_frame = 0;
    std::size_t device_count = 0;
    std::array<std::uintptr_t, 32> device_objects{};
    std::array<std::uintptr_t, 32> device_reference_owners{};
    std::array<std::int32_t, 32> device_assignments{};
    std::array<std::uint32_t, 32> device_kinds{};
    std::size_t player_count = 0;
    std::array<std::uintptr_t, 4> player_objects{};
    std::array<std::uintptr_t, 4> player_devices{};
    std::array<std::int32_t, 4> player_assignments{};
    std::array<std::uint32_t, 4> player_ordinals{};
    std::array<std::uint32_t, 4> player_device_kinds{};
    std::array<std::uintptr_t, 8> hud{};
};
struct DeviceRebindPrivateStateSnapshot {
    std::uint64_t generation = 0;
    std::uint64_t candidate_generation = 0;
    std::uint64_t candidate_pass = 0;
    bool candidate_valid = false;
    bool bound_valid = false;
    bool retired = false;
    std::array<std::uint32_t, 2> binding_states{};
    DeviceRebindTopologySnapshot candidate{};
    DeviceRebindTopologySnapshot bound{};
    DeviceRebindTopologySnapshot device_baseline{};
    std::uint32_t device_stage = 0;
    std::uint32_t device_ordinal = 0;
    std::uint32_t device_primary_slot = 0;
    std::uint64_t device_generation = 0;
    std::uint64_t device_last_manager_pass = 0;
    std::uint64_t device_candidate_pass = 0;
    std::int32_t device_new_assignment = 0;
    bool device_ordinary_backend_authorized = false;
    bool ordinary_transaction_active = false;
    std::uintptr_t ordinary_manager = 0;
    std::uint64_t ordinary_manager_pass = 0;
    std::uint32_t ordinary_stage = 0;
    std::uint64_t ordinary_route_generation = 0;
    std::uint64_t ordinary_authorized_generation = 0;
    std::array<std::uint8_t, 32> token_secret{};
};
DeviceRebindPrivateStateSnapshot SnapshotDeviceRebindPrivateState() noexcept;

struct AbiOriginalOnceProbe {
    std::array<std::uint32_t, 21> original_calls{};
    bool arguments_preserved = false;
    bool returns_preserved = false;
    bool observation_failure_passthrough = false;
    bool possessor_passthrough = false;

    bool AllOriginalsCalledExactlyOnce() const noexcept;
    bool ArgumentsAndReturnsPreserved() const noexcept;
    bool ObservationFailureStillPassedThrough() const noexcept;
    bool PossessorStoppedWithoutSuppressingOriginal() const noexcept;
};

bool RunAbiOriginalOnceProbe(AbiOriginalOnceProbe& probe) noexcept;

struct ManagerCallProbe {
    std::uint32_t original_calls = 0;
    std::uint32_t expected_return = 0;
    std::uint32_t observed_return = 0;
};

struct ManagerDiagnosticProbe {
    ManagerCallProbe normal_first{};
    ManagerCallProbe normal_second{};
    std::array<std::uint32_t, 7> normal_event_kinds{};
    std::uint32_t normal_event_count = 0;
    std::array<std::uint8_t, 32> first_end_digest{};
    std::array<std::uint8_t, 32> candidate_digest{};
    std::array<std::uint8_t, 32> second_end_digest{};
    std::array<std::uint8_t, 32> bound_digest{};

    TopologyDiagnostic rejected_diagnostic = TopologyDiagnostic::None;
    ManagerCallProbe rejected{};
    std::array<std::uint32_t, 2> rejected_event_kinds{};
    std::uint32_t rejected_event_count = 0;
    std::array<std::uint8_t, 32> rejected_digest{};
    std::array<std::uint8_t, 32> expected_rejected_digest{};

    ManagerCallProbe observation_disabled{};
    std::uint32_t observation_disabled_event_count = 0;
    ManagerCallProbe wrong_thread{};
    std::uint32_t wrong_thread_event_count = 0;
    std::uint32_t wrong_thread_last_event_kind = 0;
    ManagerCallProbe topology_read_exception{};
    std::uint32_t topology_read_exception_event_count = 0;
    std::array<std::uint8_t, 32> topology_read_exception_digest{};
    ManagerCallProbe ring_publish_failure{};
    bool ring_publish_failed = false;
};

bool RunManagerDiagnosticProbe(TopologyDiagnostic rejected_diagnostic,
                               ManagerDiagnosticProbe& probe) noexcept;

// Integration seam for the frozen nonzero-backend screen. Each case invokes
// the production manager/callsite wrappers and reports raw observations so an
// independent harness can assert the route, ABI and fail-closed boundaries.
struct BackendScreenCaseProbe {
    // 0 Ok, 1 Transient, 2 ReadFailure, 3 Ambiguous.
    std::uint32_t classification = 0;
    TopologyDiagnostic diagnostic = TopologyDiagnostic::None;
    std::uint32_t manager_end_classification = 0;
    TopologyDiagnostic manager_end_diagnostic = TopologyDiagnostic::None;
    std::uint32_t manager_original_calls = 0;
    std::uint32_t expected_return = 0;
    std::uint32_t observed_return = 0;
    std::uint32_t spawn_original_calls = 0;
    std::uint32_t possessor_spawn_original_calls = 0;
    std::uint32_t exit_original_calls = 0;
    std::uint32_t assignment_original_calls = 0;
    std::uint32_t hud_original_calls = 0;
    std::uint32_t binding_candidate_count = 0;
    std::uint32_t binding_bound_count = 0;
    std::uint32_t terminal_failure_count = 0;
    std::uint64_t generation_before = 0;
    std::uint64_t generation_after = 0;
    bool route_authorized = false;
    bool remains_unbound = false;
};

struct BackendRouteWitnessProbe {
    BackendScreenCaseProbe null_backend_without_route{};
    BackendScreenCaseProbe known_backend_without_route{};
    BackendScreenCaseProbe known_backend_exact_route{};
    BackendScreenCaseProbe misaligned_backend{};
    BackendScreenCaseProbe unreadable_backend{};
    BackendScreenCaseProbe unknown_backend_vtable{};
    BackendScreenCaseProbe roster_begin_only{};
    BackendScreenCaseProbe roster_end_only{};
    BackendScreenCaseProbe roster_capacity_only{};
    BackendScreenCaseProbe allocated_empty_roster{};
    BackendScreenCaseProbe nonempty_roster{};
    BackendScreenCaseProbe online_frame_present{};
    BackendScreenCaseProbe partial_route{};
    BackendScreenCaseProbe out_of_order_route{};
    BackendScreenCaseProbe stale_generation{};
    BackendScreenCaseProbe cross_generation_witness{};
    BackendScreenCaseProbe possessor_route{};
    BackendScreenCaseProbe exit_route{};
    BackendScreenCaseProbe unrelated_assignment{};
    BackendScreenCaseProbe assignment_pending{};
    BackendScreenCaseProbe player_ordinal_mismatch{};
    BackendScreenCaseProbe hud_mismatch{};
    BackendScreenCaseProbe intervening_rejected_pass{};

    std::array<std::uint32_t, 24> exact_event_kinds{};
    std::array<std::uint32_t, 24> exact_event_local_ordinals{};
    std::array<std::uint32_t, 24> exact_event_endpoints{};
    std::array<std::uint64_t, 24> exact_event_generations{};
    std::uint32_t exact_event_count = 0;
    std::uint64_t exact_generation_before = 0;
    std::uint64_t exact_generation_after_spawn = 0;
    std::uint64_t exact_generation_after_assignment = 0;
};

bool RunBackendRouteWitnessProbe(BackendRouteWitnessProbe& probe) noexcept;

struct BindingMachineProbe {
    bool two_complete_passes_required = false;
    bool endpoint_ordinals_frozen = false;
    bool duplicate_and_third_ordinal_rejected = false;
    bool lifecycle_invalidates_before_rebind = false;
    bool old_generation_never_revived = false;
    bool menu_teardown_retires_stable_and_temporary_layers = false;
    bool invalid_manager_pass_unbinds_without_retirement = false;
};

bool RunBindingMachineProbe(BindingMachineProbe& probe) noexcept;

struct TopologyProbeResult {
    // 0 Ok, 1 Transient, 2 ReadFailure, 3 Ambiguous.
    std::uint32_t classification = 0;
    TopologyDiagnostic diagnostic = TopologyDiagnostic::None;
    std::size_t player_count = 0;
    std::size_t device_count = 0;
};

bool RunTopologyProbe(std::uint8_t* main_base, void* expected_manager,
                      TopologyProbeResult& result) noexcept;

struct TopologyTransitionProbe {
    bool unbound_stays_unbound = false;
    bool bound_becomes_unbound = false;
    bool first_valid_is_candidate = false;
    bool second_valid_is_bound = false;
};

bool RunInvalidTopologyTransitionProbe(
    TopologyDiagnostic diagnostic, TopologyTransitionProbe& result) noexcept;
int PublishCrossLanguageFixture(const wchar_t* mapping_name,
                                const wchar_t* ready_event_name,
                                const wchar_t* acknowledge_event_name) noexcept;

#endif

} // namespace detail

} // namespace isaac_cs_t0::coop_passive
