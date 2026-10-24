#include "isaac_cs_t0/active_apply.hpp"
#include "isaac_cs_t0/active_dispatcher.hpp"
#include "isaac_cs_t0/active_identity.hpp"
#include "isaac_cs_t0/active_ipc.hpp"
#include "isaac_cs_t0/active_runtime.hpp"
#include "isaac_cs_t0/active_witness.hpp"
#include "isaac_cs_t0/helper_operations.hpp"
#include "isaac_cs_t0/hooks.hpp"
#include "isaac_cs_t0/loop_runtime.hpp"
#include "isaac_cs_t0/target_image.hpp"

#include <Windows.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>

namespace {

int g_failures = 0;

#define CHECK(expression)                                                        \
    do {                                                                         \
        if (!(expression)) {                                                     \
            std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__,       \
                         #expression);                                            \
            ++g_failures;                                                        \
            return false;                                                        \
        }                                                                        \
    } while (false)

void Suite(const char* name, std::uint32_t cases, bool passed)
{
    std::printf(
        "{\"identity\":\"IsaacCsT0-ActiveNativeSuite-v1\",\"suite\":"
        "\"%s\",\"cases\":%u,\"status\":\"%s\"}\n",
        name, cases, passed ? "PASS" : "FAIL");
}

template <std::size_t Size>
bool AllBytes(const std::array<std::uint8_t, Size>& value,
              std::uint8_t expected)
{
    for (const auto byte : value)
        if (byte != expected)
            return false;
    return true;
}

bool SignalAndWait(HANDLE ready, HANDLE acknowledge)
{
    return SetEvent(ready) != FALSE &&
           WaitForSingleObject(acknowledge, 30'000) == WAIT_OBJECT_0;
}

int PublishCrossLanguageFixture(const wchar_t* mapping_name,
                                const wchar_t* ready_event_name,
                                const wchar_t* acknowledge_event_name)
{
    using namespace isaac_cs_t0;
    HANDLE mapping = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, mapping_name);
    HANDLE ready = OpenEventW(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE,
                              ready_event_name);
    HANDLE acknowledge = OpenEventW(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE,
                                    acknowledge_event_name);
    if (!mapping || !ready || !acknowledge)
    {
        if (acknowledge)
            CloseHandle(acknowledge);
        if (ready)
            CloseHandle(ready);
        if (mapping)
            CloseHandle(mapping);
        return 2;
    }
    auto* block = static_cast<active_abi::SharedActiveBlock*>(
        MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0,
                      sizeof(active_abi::SharedActiveBlock)));
    if (!block || block->magic != active_abi::kIpcMagic ||
        block->schema_version != active_abi::kSchemaVersion ||
        block->struct_size != sizeof(active_abi::SharedActiveBlock) ||
        block->process_creation_time_100ns != 1 ||
        !AllBytes(block->nonce, 1) || !AllBytes(block->target_sha256, 2) ||
        !AllBytes(block->observed_target_sha256, 3) ||
        !AllBytes(block->target_equivalence_sha256, 4) ||
        !AllBytes(block->mode_gate_sha256, 5) ||
        !AllBytes(block->loop_gate_sha256, 6) ||
        !AllBytes(block->raw_prereq_sha256, 7) ||
        !AllBytes(block->active_catalog_sha256, 8) ||
        !AllBytes(block->active_plan_sha256, 9) ||
        !AllBytes(block->input_script_sha256, 10) ||
        !AllBytes(block->arm_token_sha256, 11) ||
        std::wcscmp(block->loop_mapping_name.data(),
                    L"Local\\IsaacCsT0.Loop.NativePublisher") != 0 ||
        !AllBytes(block->session_id, 12) || block->client_id != 13)
    {
        if (block)
            UnmapViewOfFile(block);
        CloseHandle(acknowledge);
        CloseHandle(ready);
        CloseHandle(mapping);
        return 3;
    }

    input::InputStateDraftV1 state{};
    state.move_x = input::kAxisMaximum;
    input::EncodedInputStateV1 canonical{};
    if (input::EncodeInputState(state, input::kBaseValidEdgeMask, canonical) !=
        input::CodecError::None)
    {
        UnmapViewOfFile(block);
        CloseHandle(acknowledge);
        CloseHandle(ready);
        CloseHandle(mapping);
        return 4;
    }
    active_abi::NativeActiveEvent sentinel{};
    sentinel.qpc = 11;
    sentinel.authority_tick = 12;
    sentinel.loop_iteration = 13;
    sentinel.thread_id = 14;
    sentinel.kind = static_cast<std::uint32_t>(active_abi::EventKind::Canonical);
    sentinel.phase = static_cast<std::uint32_t>(active_abi::Phase::PhysicalSample);
    sentinel.flags = active_abi::kFlagMainThread;
    sentinel.attempt = 15;
    sentinel.source_device =
        static_cast<std::uint32_t>(active_abi::SourceDevice::Keyboard);
    sentinel.client_id = 10;
    sentinel.command_sequence = 16;
    sentinel.target_tick = 17;
    sentinel.queue_depth = 19;
    sentinel.apply_generation = 20;
    sentinel.owner_generation = 21;
    sentinel.action = UINT32_MAX;
    sentinel.canonical_state = canonical;
    sentinel.witness_before.fill(0xB6);
    sentinel.witness_after.fill(0xC7);

    auto& first = block->events[0];
    active_abi::CopyEventPayload(first, sentinel);
    MemoryBarrier();
    InterlockedExchange64(&first.published_sequence, 1);
    InterlockedExchange64(&block->next_sequence, 1);
    bool complete = SignalAndWait(ready, acknowledge);

    sentinel.qpc = 22;
    auto& second = block->events[1];
    active_abi::CopyEventPayload(second, sentinel);
    InterlockedExchange64(&second.published_sequence, 0);
    InterlockedExchange64(&block->next_sequence, 2);
    complete = complete && SignalAndWait(ready, acknowledge);

    MemoryBarrier();
    InterlockedExchange64(&second.published_sequence, 2);
    complete = complete && SignalAndWait(ready, acknowledge);

    InterlockedExchange64(&block->overwritten_events, 1);
    complete = complete && SignalAndWait(ready, acknowledge);

    UnmapViewOfFile(block);
    CloseHandle(acknowledge);
    CloseHandle(ready);
    CloseHandle(mapping);
    return complete ? 0 : 5;
}

bool TestContract()
{
    using namespace isaac_cs_t0::active_abi;
    static_assert(kIpcMagic == 0x3156343049534349ull);
    static_assert(sizeof(NativeActiveEvent) == 288);
    static_assert(offsetof(SharedActiveBlock, process_creation_time_100ns) == 24);

    volatile LONG published_thread = 0;
    using isaac_cs_t0::active::detail::BindTrustedMainThread;
    using isaac_cs_t0::active::detail::MainThreadBinding;
    CHECK(BindTrustedMainThread(published_thread, 0, 2024) ==
          MainThreadBinding::AwaitingAnchor);
    CHECK(published_thread == 0);
    CHECK(BindTrustedMainThread(published_thread, 2024, 2025) ==
          MainThreadBinding::WrongThread);
    CHECK(published_thread == 0);
    CHECK(BindTrustedMainThread(published_thread, 2024, 2024) ==
          MainThreadBinding::Bound);
    CHECK(published_thread == 2024);
    CHECK(BindTrustedMainThread(published_thread, 0, 2024) ==
          MainThreadBinding::Conflict);
    CHECK(BindTrustedMainThread(published_thread, 2024, 2024) ==
          MainThreadBinding::Bound);
    CHECK(BindTrustedMainThread(published_thread, 2025, 2025) ==
          MainThreadBinding::Conflict);
    using isaac_cs_t0::active::detail::PauseGapTracker;
    using isaac_cs_t0::active::detail::PauseGapTransition;
    PauseGapTracker pause{};
    CHECK(pause.TopUpdateCompleted(false) == PauseGapTransition::None);
    CHECK(pause.AuthorityObserved() == PauseGapTransition::None);
    CHECK(!pause.BlocksInput());
    CHECK(pause.TopUpdateCompleted(false) == PauseGapTransition::None);
    CHECK(pause.TopUpdateCompleted(false) == PauseGapTransition::Enter);
    CHECK(pause.BlocksInput());
    CHECK(pause.TopUpdateCompleted(false) == PauseGapTransition::None);
    CHECK(pause.AuthorityObserved() == PauseGapTransition::Exit);
    CHECK(!pause.BlocksInput());
    CHECK(pause.TopUpdateCompleted(true) == PauseGapTransition::None);

    using isaac_cs_t0::active::detail::LifecycleGateTransition;
    using isaac_cs_t0::active::detail::LifecycleInputDomain;
    using isaac_cs_t0::active::detail::LifecycleSubmissionGate;
    using isaac_cs_t0::active::detail::PublishedRoomLifecycle;
    using isaac_cs_t0::active::detail::ObserveLoadingEnvelope;
    using isaac_cs_t0::active::detail::PhysicalSubmissionAllowedAfterBinding;
    using isaac_cs_t0::active::detail::PostLoadingAuthorityGate;
    using isaac_cs_t0::active::detail::AuthorityObservedWithLoadingRecovery;
    using isaac_cs_t0::active::detail::TopUpdateCompletedWithLoadingRecovery;
    LifecycleSubmissionGate lifecycle_gate{};
    PostLoadingAuthorityGate loading_recovery{};
    CHECK(PhysicalSubmissionAllowedAfterBinding(pause, lifecycle_gate,
                                                loading_recovery));
    CHECK(ObserveLoadingEnvelope(lifecycle_gate, loading_recovery, true) ==
          LifecycleGateTransition::Entered);
    CHECK(!PhysicalSubmissionAllowedAfterBinding(pause, lifecycle_gate,
                                                 loading_recovery));
    CHECK(ObserveLoadingEnvelope(lifecycle_gate, loading_recovery, true) ==
          LifecycleGateTransition::Nested);
    CHECK(!loading_recovery.BlocksInput());
    CHECK(ObserveLoadingEnvelope(lifecycle_gate, loading_recovery, false) ==
          LifecycleGateTransition::RemainsNested);
    CHECK(!loading_recovery.BlocksInput());
    CHECK(lifecycle_gate.Enter(LifecycleInputDomain::Room) ==
          LifecycleGateTransition::Entered);
    CHECK(lifecycle_gate.Enter(LifecycleInputDomain::Room) ==
          LifecycleGateTransition::Nested);
    CHECK(lifecycle_gate.Exit(LifecycleInputDomain::Room) ==
          LifecycleGateTransition::RemainsNested);
    CHECK(!PhysicalSubmissionAllowedAfterBinding(pause, lifecycle_gate,
                                                 loading_recovery));
    CHECK(lifecycle_gate.Exit(LifecycleInputDomain::Room) ==
          LifecycleGateTransition::Exited);
    CHECK(ObserveLoadingEnvelope(lifecycle_gate, loading_recovery, false) ==
          LifecycleGateTransition::Exited);
    CHECK(!PhysicalSubmissionAllowedAfterBinding(pause, lifecycle_gate,
                                                 loading_recovery));
    CHECK(TopUpdateCompletedWithLoadingRecovery(pause, loading_recovery,
                                                false) ==
          PauseGapTransition::None);
    CHECK(TopUpdateCompletedWithLoadingRecovery(pause, loading_recovery,
                                                false) ==
          PauseGapTransition::None);
    CHECK(!pause.BlocksInput());
    CHECK(AuthorityObservedWithLoadingRecovery(pause, loading_recovery) ==
          PauseGapTransition::None);
    CHECK(PhysicalSubmissionAllowedAfterBinding(pause, lifecycle_gate,
                                                loading_recovery));
    CHECK(TopUpdateCompletedWithLoadingRecovery(pause, loading_recovery,
                                                false) ==
          PauseGapTransition::None);
    CHECK(TopUpdateCompletedWithLoadingRecovery(pause, loading_recovery,
                                                false) ==
          PauseGapTransition::Enter);
    CHECK(pause.BlocksInput());
    CHECK(AuthorityObservedWithLoadingRecovery(pause, loading_recovery) ==
          PauseGapTransition::Exit);
    CHECK(!pause.BlocksInput());
    CHECK(lifecycle_gate.Exit(LifecycleInputDomain::Room) ==
          LifecycleGateTransition::Quarantined);
    CHECK(!lifecycle_gate.Healthy());
    CHECK(!PhysicalSubmissionAllowedAfterBinding(pause, lifecycle_gate,
                                                 loading_recovery));
    lifecycle_gate.Reset();
    CHECK(lifecycle_gate.Healthy());
    lifecycle_gate.Retire();
    CHECK(!PhysicalSubmissionAllowedAfterBinding(pause, lifecycle_gate,
                                                 loading_recovery));
    lifecycle_gate.Reset();
    loading_recovery.Reset();
    CHECK(PhysicalSubmissionAllowedAfterBinding(pause, lifecycle_gate,
                                                loading_recovery));
    using isaac_cs_t0::loop::EmbeddedLifecycleSignal;
    CHECK(PublishedRoomLifecycle(EmbeddedLifecycleSignal::RoomChange, true) ==
          Lifecycle::None);
    CHECK(PublishedRoomLifecycle(EmbeddedLifecycleSignal::RoomChange, false) ==
          Lifecycle::None);
    CHECK(PublishedRoomLifecycle(EmbeddedLifecycleSignal::RoomLifecycle, true) ==
          Lifecycle::None);
    CHECK(PublishedRoomLifecycle(EmbeddedLifecycleSignal::RoomLifecycle, false) ==
          Lifecycle::None);
    CHECK(PublishedRoomLifecycle(EmbeddedLifecycleSignal::NewRoom, true) ==
          Lifecycle::RoomEnter);
    CHECK(PublishedRoomLifecycle(EmbeddedLifecycleSignal::NewRoom, false) ==
          Lifecycle::RoomExit);
    using isaac_cs_t0::active::detail::LoadingAuthorityPassthrough;
    using isaac_cs_t0::active::detail::PhysicalOwnerReadyForSubmission;
    using isaac_cs_t0::active::detail::PreserveLoadingPlayerBootstrap;
    constexpr std::array all_active_scenarios{
        Scenario::Keyboard,   Scenario::Controller, Scenario::Simultaneous,
        Scenario::RapidTap,   Scenario::DeviceSwitch, Scenario::Pause,
        Scenario::Loading,    Scenario::Room,       Scenario::Menu,
        Scenario::QueueFull,  Scenario::Expired,    Scenario::TooFar,
        Scenario::Stale,      Scenario::Conflict,   Scenario::Duplicate,
        Scenario::Loopback,
    };
    for (const auto scenario : all_active_scenarios)
    {
        CHECK(!PhysicalOwnerReadyForSubmission(scenario, false));
        CHECK(PhysicalOwnerReadyForSubmission(scenario, true));
    }
    CHECK(LoadingAuthorityPassthrough(Scenario::Loading, false));
    CHECK(!LoadingAuthorityPassthrough(Scenario::Loading, true));
    CHECK(!LoadingAuthorityPassthrough(Scenario::Keyboard, false));
    CHECK(PreserveLoadingPlayerBootstrap(Scenario::Loading, true, false));
    CHECK(!PreserveLoadingPlayerBootstrap(Scenario::Loading, false, false));
    CHECK(!PreserveLoadingPlayerBootstrap(Scenario::Loading, true, true));
    CHECK(!PreserveLoadingPlayerBootstrap(Scenario::Keyboard, true, false));

    using isaac_cs_t0::loop::detail::AlternateEnvelopeTracker;
    using isaac_cs_t0::loop::detail::AlternateEnvelopeTransition;
    AlternateEnvelopeTracker alternate{};
    CHECK(alternate.BeginOwner() == AlternateEnvelopeTransition::None);
    CHECK(alternate.EndOwner() == AlternateEnvelopeTransition::None);
    CHECK(alternate.BeginOwner() == AlternateEnvelopeTransition::None);
    CHECK(alternate.ObserveStateApply() == AlternateEnvelopeTransition::Open);
    CHECK(alternate.ObserveStateApply() == AlternateEnvelopeTransition::None);
    CHECK(alternate.EndOwner() == AlternateEnvelopeTransition::Close);
    CHECK(alternate.EndOwner() == AlternateEnvelopeTransition::Invalid);
    CHECK(alternate.BeginOwner() == AlternateEnvelopeTransition::None);
    CHECK(alternate.BeginOwner() == AlternateEnvelopeTransition::Invalid);
    alternate.Reset();
    static_assert(offsetof(SharedActiveBlock, nonce) == 32);
    static_assert(offsetof(SharedActiveBlock, target_sha256) == 64);
    static_assert(offsetof(SharedActiveBlock, observed_target_sha256) == 96);
    static_assert(offsetof(SharedActiveBlock, target_equivalence_sha256) == 128);
    static_assert(offsetof(SharedActiveBlock, mode_gate_sha256) == 160);
    static_assert(offsetof(SharedActiveBlock, loop_gate_sha256) == 192);
    static_assert(offsetof(SharedActiveBlock, raw_prereq_sha256) == 224);
    static_assert(offsetof(SharedActiveBlock, arm_token_sha256) == 352);
    static_assert(offsetof(SharedActiveBlock, loop_mapping_name) == 384);
    static_assert(offsetof(SharedActiveBlock, session_id) == 576);
    static_assert(offsetof(SharedActiveBlock, events) == 720);
    static_assert(sizeof(SharedActiveBlock) == 18'875'088);
    static_assert(static_cast<std::uint32_t>(Scenario::Keyboard) == 1);
    static_assert(static_cast<std::uint32_t>(Scenario::Loopback) == 16);
    using isaac_cs_t0::loop::detail::CompanionScenarioAllowed;
    CHECK(!CompanionScenarioAllowed(0, false));
    CHECK(!CompanionScenarioAllowed(0, true));
    for (std::uint32_t scenario = 1; scenario <= 10; ++scenario)
    {
        CHECK(CompanionScenarioAllowed(scenario, false));
        CHECK(CompanionScenarioAllowed(scenario, true));
    }
    for (std::uint32_t scenario = 11; scenario <= 16; ++scenario)
    {
        CHECK(!CompanionScenarioAllowed(scenario, false));
        CHECK(CompanionScenarioAllowed(scenario, true));
    }
    CHECK(!CompanionScenarioAllowed(17, false));
    CHECK(!CompanionScenarioAllowed(17, true));
    using isaac_cs_t0::loop::EmbeddedObserverIdentity;
    using isaac_cs_t0::loop::detail::CompanionMappingIdentityAllowed;
    EmbeddedObserverIdentity embedded_identity{};
    embedded_identity.target_pid = 1234;
    embedded_identity.scenario = 11;
    embedded_identity.nonce.fill(0xA5u);
    CHECK(CompanionMappingIdentityAllowed(
        1234, 11, embedded_identity.nonce, true, &embedded_identity));
    CHECK(!CompanionMappingIdentityAllowed(
        1234, 11, embedded_identity.nonce, false, nullptr));
    embedded_identity.scenario = 16;
    CHECK(CompanionMappingIdentityAllowed(
        1234, 16, embedded_identity.nonce, true, &embedded_identity));
    CHECK(!CompanionMappingIdentityAllowed(
        1234, 17, embedded_identity.nonce, true, &embedded_identity));
    static_assert(static_cast<std::uint32_t>(EventKind::Initialized) == 1);
    static_assert(static_cast<std::uint32_t>(EventKind::Failure) == 20);
    static_assert(static_cast<std::uint32_t>(Phase::Witness) == 6);
    static_assert(static_cast<std::uint32_t>(Decision::Duplicate) == 3);
    static_assert(static_cast<std::uint32_t>(RejectReason::QueueFull) == 1);
    static_assert(static_cast<std::uint32_t>(RejectReason::IdentityConflict) == 5);
    static_assert(static_cast<std::uint32_t>(QueryKind::Scalar) == 3);
    static_assert(static_cast<std::uint32_t>(Consumer::Drop) == 6);
    static_assert(static_cast<std::uint32_t>(Lifecycle::OwnerSwitch) == 7);
    static_assert(static_cast<std::uint32_t>(Lifecycle::RunTeardown) == 8);
    static_assert(static_cast<std::uint32_t>(WitnessKind::WorldState) == 1);
    static_assert(static_cast<std::uint32_t>(WitnessKind::RejectedUnchanged) == 2);
    static_assert(static_cast<LONG>(ErrorCode::UnsafeShutdown) == 8);
    static_assert(static_cast<LONG>(ErrorCode::AuthorityOverflow) == 15);
    static_assert(kCombinedHookCount == 45);
    std::array<std::array<std::uint8_t, 32>, 9> identities{{
        isaac_cs_t0::active_identity::kCanonicalTargetSha256,
        isaac_cs_t0::active_identity::kObservedUnpackedTargetSha256,
        isaac_cs_t0::active_identity::kTargetEquivalenceSha256,
        isaac_cs_t0::active_identity::kModeGateSha256,
        isaac_cs_t0::active_identity::kLoopGateSha256,
        isaac_cs_t0::active_identity::kRawPrerequisiteSha256,
        isaac_cs_t0::active_identity::kCatalogSha256,
        isaac_cs_t0::active_identity::kPlanSha256,
        isaac_cs_t0::active_identity::kScriptSha256,
    }};
    const auto approved_identity = [](const auto& values) {
        return isaac_cs_t0::active_identity::ApprovedIdentity(
            values[0], values[1], values[2], values[3], values[4],
            values[5], values[6], values[7], values[8]);
    };
    CHECK(approved_identity(identities));
    for (std::size_t index = 0; index < identities.size(); ++index)
    {
        identities[index][index] ^= 0x01u;
        CHECK(!approved_identity(identities));
        identities[index][index] ^= 0x01u;
    }
    CHECK(isaac_cs_t0::active_identity::ApprovedProcessIdentity(7, 7, 11, 11));
    CHECK(!isaac_cs_t0::active_identity::ApprovedProcessIdentity(0, 7, 11, 11));
    CHECK(!isaac_cs_t0::active_identity::ApprovedProcessIdentity(7, 8, 11, 11));
    CHECK(!isaac_cs_t0::active_identity::ApprovedProcessIdentity(7, 7, 0, 11));
    CHECK(!isaac_cs_t0::active_identity::ApprovedProcessIdentity(7, 7, 11, 12));
    std::array<std::uint8_t, 0x400> header{};
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(header.data());
    dos->e_magic = IMAGE_DOS_SIGNATURE;
    dos->e_lfanew = 0x80;
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS32*>(header.data() + dos->e_lfanew);
    nt->Signature = IMAGE_NT_SIGNATURE;
    nt->FileHeader.Machine = IMAGE_FILE_MACHINE_I386;
    nt->FileHeader.NumberOfSections = 5;
    nt->FileHeader.TimeDateStamp = isaac_cs_t0::target_image::kExpectedTimestamp;
    nt->FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER32);
    nt->FileHeader.Characteristics = 0x0122;
    nt->OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR32_MAGIC;
    nt->OptionalHeader.AddressOfEntryPoint =
        isaac_cs_t0::target_image::kUnpackedEntryPointRva;
    nt->OptionalHeader.ImageBase = isaac_cs_t0::target_image::kPreferredImageBase;
    nt->OptionalHeader.SectionAlignment = 0x1000;
    nt->OptionalHeader.FileAlignment = 0x0200;
    nt->OptionalHeader.SizeOfImage = isaac_cs_t0::target_image::kUnpackedImageSize;
    nt->OptionalHeader.SizeOfHeaders = 0x0400;
    nt->OptionalHeader.DllCharacteristics = 0x8140;
    auto* sections = IMAGE_FIRST_SECTION(nt);
    for (std::size_t index = 0; index < 5; ++index)
    {
        const auto& expected = isaac_cs_t0::target_image::kSections[index];
        std::memcpy(sections[index].Name, expected.name.data(), expected.name.size());
        sections[index].Misc.VirtualSize = expected.virtual_size;
        sections[index].VirtualAddress = expected.rva;
        sections[index].SizeOfRawData = expected.raw_size;
        sections[index].PointerToRawData = expected.raw_offset;
        sections[index].Characteristics = expected.characteristics;
    }
    CHECK(!isaac_cs_t0::target_image::IsApproved(header.data()));
    nt->OptionalHeader.ImageBase =
        static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(header.data()));
    CHECK(isaac_cs_t0::target_image::IsApproved(header.data()));
    CHECK(isaac_cs_t0::target_image::IsApproved(
        header.data(), isaac_cs_t0::target_image::Profile::ApprovedUnpacked));
    CHECK(!isaac_cs_t0::target_image::IsApproved(
        header.data(), isaac_cs_t0::target_image::Profile::Canonical));
    nt->OptionalHeader.ImageBase = 0x00500000u;
    if (nt->OptionalHeader.ImageBase ==
        static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(header.data())))
        nt->OptionalHeader.ImageBase = 0x00600000u;
    CHECK(!isaac_cs_t0::target_image::IsApproved(header.data()));
    nt->OptionalHeader.ImageBase = isaac_cs_t0::target_image::kPreferredImageBase;
    nt->FileHeader.NumberOfSections = 6;
    nt->OptionalHeader.AddressOfEntryPoint =
        isaac_cs_t0::target_image::kCanonicalEntryPointRva;
    nt->OptionalHeader.SizeOfImage = isaac_cs_t0::target_image::kCanonicalImageSize;
    const auto& bind = isaac_cs_t0::target_image::kSections[5];
    std::memcpy(sections[5].Name, bind.name.data(), bind.name.size());
    sections[5].Misc.VirtualSize = bind.virtual_size;
    sections[5].VirtualAddress = bind.rva;
    sections[5].SizeOfRawData = bind.raw_size;
    sections[5].PointerToRawData = bind.raw_offset;
    sections[5].Characteristics = bind.characteristics;
    CHECK(isaac_cs_t0::target_image::IsApproved(header.data()));
    CHECK(isaac_cs_t0::target_image::IsApproved(
        header.data(), isaac_cs_t0::target_image::Profile::Canonical));
    CHECK(!isaac_cs_t0::target_image::IsApproved(
        header.data(), isaac_cs_t0::target_image::Profile::ApprovedUnpacked));
    nt->OptionalHeader.SizeOfImage += 0x1000;
    CHECK(!isaac_cs_t0::target_image::IsApproved(header.data()));
    return true;
}

bool HasKind(const isaac_cs_t0::active::DispatchBatch& batch,
             isaac_cs_t0::active_abi::EventKind kind,
             isaac_cs_t0::active_abi::RejectReason reason =
                 isaac_cs_t0::active_abi::RejectReason::None)
{
    for (std::size_t index = 0; index < batch.count; ++index)
        if (batch.traces[index].kind == kind &&
            (reason == isaac_cs_t0::active_abi::RejectReason::None ||
             batch.traces[index].reject == reason))
            return true;
    return false;
}

bool TestPipeline()
{
    using namespace isaac_cs_t0;
    const std::array<std::uint8_t, 16> session{{
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
    }};
    input::RawInputSample right{};
    right.move_x = 1.0F;
    active::ActiveDispatcher normal(session, 7, active_abi::Scenario::Keyboard);
    const auto batch =
        normal.SubmitPhysical(right, active_abi::SourceDevice::Keyboard, 10);
    CHECK(HasKind(batch, active_abi::EventKind::Canonical));
    CHECK(HasKind(batch, active_abi::EventKind::Send));
    CHECK(HasKind(batch, active_abi::EventKind::Receive));
    CHECK(HasKind(batch, active_abi::EventKind::Accept));
    active::DispatchBatch resolve_trace{};
    const auto early = normal.Resolve(11, resolve_trace);
    CHECK(early.resolved && !early.has_input);
    const auto resolved = normal.Resolve(12, resolve_trace);
    CHECK(resolved.resolved && resolved.has_input);
    CHECK(resolved.state.move_x == input::kAxisMaximum);
    CHECK(resolved.provenance_count == 1);

    active::ActiveDispatcher sampled(session, 8,
                                     active_abi::Scenario::Keyboard);
    const auto first_same_tick =
        sampled.SubmitPhysical({}, active_abi::SourceDevice::Keyboard, 20);
    CHECK(HasKind(first_same_tick, active_abi::EventKind::Accept));
    const auto duplicate_same_tick =
        sampled.SubmitPhysical({}, active_abi::SourceDevice::Keyboard, 20);
    CHECK(duplicate_same_tick.count == 0);
    input::RawInputSample changed{};
    changed.move_x = -1.0F;
    const auto changed_same_tick =
        sampled.SubmitPhysical(changed, active_abi::SourceDevice::Keyboard, 20);
    CHECK(HasKind(changed_same_tick, active_abi::EventKind::Accept));
    input::RawInputSample edged = changed;
    edged.edge_flags = input::kEdgeBomb;
    const auto edge_same_tick =
        sampled.SubmitPhysical(edged, active_abi::SourceDevice::Keyboard, 20);
    CHECK(HasKind(edge_same_tick, active_abi::EventKind::Accept));
    const auto post_edge_same_tick =
        sampled.SubmitPhysical(changed, active_abi::SourceDevice::Keyboard, 20);
    CHECK(post_edge_same_tick.count == 0);
    const auto next_tick =
        sampled.SubmitPhysical(changed, active_abi::SourceDevice::Keyboard, 21);
    CHECK(HasKind(next_tick, active_abi::EventKind::Accept));
    sampled.ClearForGeneration();
    const auto after_generation_clear =
        sampled.SubmitPhysical(changed, active_abi::SourceDevice::Keyboard, 21);
    CHECK(HasKind(after_generation_clear, active_abi::EventKind::Accept));

    struct NegativeCase {
        active_abi::Scenario scenario;
        active_abi::RejectReason reason;
    };
    constexpr std::array<NegativeCase, 4> negative{{
        {active_abi::Scenario::QueueFull, active_abi::RejectReason::QueueFull},
        {active_abi::Scenario::Expired, active_abi::RejectReason::Expired},
        {active_abi::Scenario::TooFar, active_abi::RejectReason::TooFar},
        {active_abi::Scenario::Stale, active_abi::RejectReason::Stale},
    }};
    active::ActiveDispatcher expired_at_zero(session, 7,
                                             active_abi::Scenario::Expired);
    CHECK(expired_at_zero
              .SubmitPhysical({}, active_abi::SourceDevice::Keyboard, 0)
              .count == 0);
    const auto expired_after_zero =
        expired_at_zero.SubmitPhysical({}, active_abi::SourceDevice::Keyboard, 1);
    CHECK(HasKind(expired_after_zero, active_abi::EventKind::Reject,
                  active_abi::RejectReason::Expired));
    CHECK(expired_at_zero
              .SubmitPhysical({}, active_abi::SourceDevice::Keyboard, 2)
              .count == 0);
    expired_at_zero.ClearForGeneration();
    CHECK(expired_at_zero
              .SubmitPhysical({}, active_abi::SourceDevice::Keyboard, 3)
              .count == 0);
    for (const auto& item : negative)
    {
        active::ActiveDispatcher dispatcher(session, 7, item.scenario);
        const std::uint64_t first_tick =
            item.scenario == active_abi::Scenario::Expired ? 1u : 0u;
        const auto rejected = dispatcher.SubmitPhysical(
            {}, active_abi::SourceDevice::Keyboard, first_tick);
        CHECK(HasKind(rejected, active_abi::EventKind::Reject, item.reason));
        active::DispatchBatch trace{};
        CHECK(!dispatcher.Resolve(first_tick + 2, trace).has_input);
        CHECK(dispatcher
                  .SubmitPhysical({}, active_abi::SourceDevice::Keyboard,
                                  first_tick + 1)
                  .count == 0);
        dispatcher.ClearForGeneration();
        CHECK(dispatcher
                  .SubmitPhysical({}, active_abi::SourceDevice::Keyboard,
                                  first_tick + 2)
                  .count == 0);
    }
    active::ActiveDispatcher duplicate(session, 7,
                                       active_abi::Scenario::Duplicate);
    CHECK(HasKind(
        duplicate.SubmitPhysical({}, active_abi::SourceDevice::Keyboard, 10),
        active_abi::EventKind::Duplicate));
    active::ActiveDispatcher conflict(session, 7,
                                      active_abi::Scenario::Conflict);
    const auto conflict_batch =
        conflict.SubmitPhysical({}, active_abi::SourceDevice::Keyboard, 10);
    CHECK(HasKind(conflict_batch, active_abi::EventKind::Reject,
                  active_abi::RejectReason::IdentityConflict));
    CHECK(conflict_batch.client_retired);
    CHECK(conflict_batch.error == active_abi::ErrorCode::None);
    using active::detail::ClientRetirementDisposition;
    using active::detail::HandleClientRetirement;
    std::uint32_t graceful_retirements = 0;
    std::uint32_t unsafe_retirements = 0;
    active_abi::ErrorCode observed_unsafe_error = active_abi::ErrorCode::None;
    CHECK(HandleClientRetirement(
              conflict_batch.client_retired, conflict_batch.error,
              [&]() noexcept { ++graceful_retirements; },
              [&](active_abi::ErrorCode) noexcept { ++unsafe_retirements; }) ==
          ClientRetirementDisposition::Graceful);
    CHECK(graceful_retirements == 1);
    CHECK(unsafe_retirements == 0);
    CHECK(HandleClientRetirement(
              true, active_abi::ErrorCode::ProtocolViolation,
              [&]() noexcept { ++graceful_retirements; },
              [&](active_abi::ErrorCode error) noexcept {
                  observed_unsafe_error = error;
                  ++unsafe_retirements;
              }) == ClientRetirementDisposition::Unsafe);
    CHECK(graceful_retirements == 1);
    CHECK(unsafe_retirements == 1);
    CHECK(observed_unsafe_error == active_abi::ErrorCode::ProtocolViolation);
    CHECK(HandleClientRetirement(
              false, active_abi::ErrorCode::ProtocolViolation,
              [&]() noexcept { ++graceful_retirements; },
              [&](active_abi::ErrorCode error) noexcept {
                  observed_unsafe_error = error;
                  ++unsafe_retirements;
              }) == ClientRetirementDisposition::Unsafe);
    CHECK(graceful_retirements == 1);
    CHECK(unsafe_retirements == 2);
    CHECK(observed_unsafe_error == active_abi::ErrorCode::ProtocolViolation);
    CHECK(HandleClientRetirement(
              false, active_abi::ErrorCode::None,
              [&]() noexcept { ++graceful_retirements; },
              [&](active_abi::ErrorCode) noexcept { ++unsafe_retirements; }) ==
          ClientRetirementDisposition::Continue);
    CHECK(graceful_retirements == 1);
    CHECK(unsafe_retirements == 2);
    for (std::size_t index = 0; index < conflict_batch.count; ++index)
    {
        const auto& trace = conflict_batch.traces[index];
        if (trace.attempt == 1)
        {
            CHECK(trace.has_conflicting_state);
            CHECK(trace.command.state.move_x == 0);
            CHECK(trace.conflicting_state.move_x == 1024);
        }
    }
    CHECK(conflict.Retired());
    active::detail::ConflictRetirementRuntimeProbe runtime_probe{};
    CHECK(active::detail::RunConflictRetirementRuntimeProbe(runtime_probe));
    CHECK(runtime_probe.conflict_event_count == 9);
    CHECK(runtime_probe.protocol_event_count == 2);
    return true;
}

bool TestAdapter()
{
    using namespace isaac_cs_t0;
    input::InputStateDraftV1 state{};
    state.move_x = input::kAxisMaximum;
    state.move_y = -input::kAxisMaximum;
    state.aim_x = -input::kAxisMaximum;
    state.held_flags = input::kHeldShoot;
    state.edge_flags = input::kEdgeBomb | input::kEdgeDrop;
    CHECK(active::ActionMagnitude(state, 0) == 0.0F);
    CHECK(active::ActionMagnitude(state, 1) == 1.0F);
    CHECK(active::ActionMagnitude(state, 2) == 1.0F);
    CHECK(active::ActionMagnitude(state, 4) == 1.0F);
    CHECK(active::ActionMagnitude(state, 8) == 1.0F);
    CHECK(active::ActionMagnitude(state, 11) == 1.0F);
    CHECK(active::ActionTriggered({}, state, 1));
    CHECK(active::ActionTriggered({}, state, 8));
    CHECK(active::ConsumerForAction(10) == active_abi::Consumer::PillCard);
    CHECK(active::ConsumerForAction(12) == active_abi::Consumer::None);

    active::QueryEligibility eligible{};
    eligible.main_thread = true;
    eligible.steady_standard = true;
    eligible.unique_player = true;
    eligible.unique_device = true;
    eligible.natural_consumer = true;
    eligible.apply_generation = 4;
    eligible.owner_generation = 5;
    eligible.consumer = active_abi::Consumer::Movement;
    active::SyntheticAdapter adapter{};
    CHECK(adapter.BeginAuthority(12, 4, 5, &state));
    const auto scalar =
        adapter.Evaluate(1, active_abi::QueryKind::Scalar, eligible);
    CHECK(scalar.disposition == active::OverrideDisposition::Snapshot);
    CHECK(scalar.scalar == 1.0F);
    CHECK(adapter.EndAuthority(true));
    adapter.SetPendingTarget(14);
    CHECK(adapter.BeginAuthority(13, 4, 5, nullptr));
    CHECK(adapter.Evaluate(1, active_abi::QueryKind::Pressed, eligible).disposition ==
          active::OverrideDisposition::SuppressNeutral);
    CHECK(adapter.EndAuthority(true));
    CHECK(adapter.BeginAuthority(14, 4, 5, &state));
    const auto rising_after_suppression =
        adapter.Evaluate(1, active_abi::QueryKind::Triggered, eligible);
    CHECK(rising_after_suppression.disposition ==
          active::OverrideDisposition::Snapshot);
    CHECK(rising_after_suppression.boolean);
    CHECK(adapter.EndAuthority(true));
    adapter.Retire();
    CHECK(adapter.Evaluate(1, active_abi::QueryKind::Pressed, eligible).disposition ==
          active::OverrideDisposition::TerminalPassthrough);
    return true;
}

bool TestWitness()
{
    using namespace isaac_cs_t0;
    std::array<std::uint8_t, 0x2000> player{};
    const active::PlayerIdentity identity{0, 1, 3};
    CHECK(active::EnterPlayerScope(player.data(), identity));
    CHECK(active::EnterConsumerScope(player.data(),
                                     active_abi::Consumer::Movement, 3));
    mode::Sha256Digest before{};
    mode::Sha256Digest after{};
    CHECK(active::CaptureWitness(player.data(),
                                 active_abi::Consumer::Movement,
                                 before) == active::WitnessStatus::Ok);
    player[0x360] = 1;
    CHECK(active::CaptureWitness(player.data(),
                                 active_abi::Consumer::Movement,
                                 after) == active::WitnessStatus::Ok);
    CHECK(before != after);
    CHECK(active::CaptureWitness(player.data(), active_abi::Consumer::Bomb,
                                 after) == active::WitnessStatus::CrossFamily);
    CHECK(active::LeaveConsumerScope(player.data(),
                                     active_abi::Consumer::Movement, 3));
    CHECK(active::LeavePlayerScope(player.data(), identity));

    CHECK(active::EnterPlayerScope(player.data(), identity));
    CHECK(active::EnterConsumerScope(player.data(), active_abi::Consumer::Item, 3));
    CHECK(active::CaptureWitness(player.data(), active_abi::Consumer::Item,
                                 before) == active::WitnessStatus::Ok);
    CHECK(active::CaptureWitness(player.data(), active_abi::Consumer::Item,
                                 after) == active::WitnessStatus::Ok);
    CHECK(before == after);
    CHECK(active::LeaveConsumerScope(player.data(), active_abi::Consumer::Item, 3));
    CHECK(active::LeavePlayerScope(player.data(), identity));
    return true;
}

class PatchFixture {
public:
    static constexpr std::size_t kRelative = 33;
    static constexpr std::size_t kPointers = 12;

    PatchFixture()
    {
        memory = static_cast<std::uint8_t*>(
            VirtualAlloc(nullptr, 0x3000, MEM_COMMIT | MEM_RESERVE,
                         PAGE_EXECUTE_READWRITE));
        Reset();
    }
    ~PatchFixture()
    {
        if (memory)
            VirtualFree(memory, 0, MEM_RELEASE);
    }
    void Reset()
    {
        if (!memory)
            return;
        std::memset(memory, 0x90, 0x3000);
        for (std::size_t index = 0; index < relative.size(); ++index)
        {
            auto* call = memory + index * 8;
            auto* original = memory + 0x800 + index;
            auto* replacement = memory + 0xA00 + index;
            call[0] = 0xE8;
            const auto displacement =
                static_cast<std::int32_t>(original - (call + 5));
            std::memcpy(call + 1, &displacement, sizeof(displacement));
            relative[index] = {};
            relative[index].call = call;
            std::memcpy(relative[index].expected, call, 5);
            relative[index].replacement = replacement;
        }
        auto** slots = reinterpret_cast<void**>(memory + 0x1000);
        for (std::size_t index = 0; index < pointers.size(); ++index)
        {
            slots[index] = memory + 0xC00 + index;
            pointers[index] = {&slots[index], memory + 0xE00 + index,
                               nullptr, nullptr, false, slots[index]};
        }
    }
    bool Restored() const
    {
        for (const auto& patch : relative)
            if (patch.installed ||
                std::memcmp(patch.call, patch.expected, sizeof(patch.expected)) != 0)
                return false;
        for (const auto& patch : pointers)
            if (patch.installed || *patch.slot != patch.expected_original)
                return false;
        return true;
    }
    std::uint8_t* memory = nullptr;
    std::array<isaac_cs_t0::mode::RelativeCallPatch, kRelative> relative{};
    std::array<isaac_cs_t0::mode::PointerPatch, kPointers> pointers{};
};

bool Install(PatchFixture& fixture)
{
    return isaac_cs_t0::mode::InstallPatchTransaction(
        fixture.relative.data(), fixture.relative.size(), fixture.pointers.data(),
        fixture.pointers.size());
}

bool Restore(PatchFixture& fixture)
{
    return isaac_cs_t0::mode::RestorePatchTransaction(
        fixture.relative.data(), fixture.relative.size(), fixture.pointers.data(),
        fixture.pointers.size());
}

bool TestTransaction()
{
    using namespace isaac_cs_t0::mode;
    PatchFixture fixture;
    CHECK(fixture.memory != nullptr);
    SetPatchTestFault(PatchTestFaultStage::None, UINT32_MAX);
    CHECK(Install(fixture));
    CHECK(Restore(fixture));
    CHECK(fixture.Restored());

    for (std::uint32_t index = 0; index < PatchFixture::kRelative; ++index)
    {
        fixture.Reset();
        SetPatchTestFault(PatchTestFaultStage::PrepareRelative, index);
        CHECK(!Install(fixture) && fixture.Restored());
        fixture.Reset();
        SetPatchTestFault(PatchTestFaultStage::ApplyRelative, index);
        CHECK(!Install(fixture) && fixture.Restored());
        fixture.Reset();
        SetPatchTestFault(PatchTestFaultStage::Protect, index);
        CHECK(!Install(fixture) && fixture.Restored());
        fixture.Reset();
        SetPatchTestFault(PatchTestFaultStage::None, UINT32_MAX);
        CHECK(Install(fixture));
        SetPatchTestFault(PatchTestFaultStage::RestoreRelative, index);
        CHECK(!Restore(fixture));
        SetPatchTestFault(PatchTestFaultStage::None, UINT32_MAX);
        CHECK(Restore(fixture) && fixture.Restored());
    }
    for (std::uint32_t index = 0; index < PatchFixture::kPointers; ++index)
    {
        fixture.Reset();
        SetPatchTestFault(PatchTestFaultStage::PreparePointer, index);
        CHECK(!Install(fixture) && fixture.Restored());
        fixture.Reset();
        SetPatchTestFault(PatchTestFaultStage::ApplyPointer, index);
        CHECK(!Install(fixture) && fixture.Restored());
        fixture.Reset();
        SetPatchTestFault(PatchTestFaultStage::Protect,
                          PatchFixture::kRelative + index);
        CHECK(!Install(fixture) && fixture.Restored());
        fixture.Reset();
        SetPatchTestFault(PatchTestFaultStage::None, UINT32_MAX);
        CHECK(Install(fixture));
        SetPatchTestFault(PatchTestFaultStage::RestorePointer, index);
        CHECK(!Restore(fixture));
        SetPatchTestFault(PatchTestFaultStage::None, UINT32_MAX);
        CHECK(Restore(fixture) && fixture.Restored());
    }
    fixture.Reset();
    SetPatchTestFault(PatchTestFaultStage::ThreadSnapshot, 0);
    CHECK(!Install(fixture) && fixture.Restored());
    fixture.Reset();
    SetPatchTestFault(PatchTestFaultStage::ThreadSuspend, 0);
    CHECK(!Install(fixture) && fixture.Restored());

    fixture.Reset();
    SetPatchTestFaultPair(PatchTestFaultStage::ApplyRelative, 32,
                          PatchTestFaultStage::RollbackRelative, 10);
    CHECK(!Install(fixture));
    SetPatchTestFault(PatchTestFaultStage::None, UINT32_MAX);
    CHECK(Restore(fixture) && fixture.Restored());
    fixture.Reset();
    SetPatchTestFaultPair(PatchTestFaultStage::ApplyPointer, 11,
                          PatchTestFaultStage::RollbackPointer, 5);
    CHECK(!Install(fixture));
    SetPatchTestFault(PatchTestFaultStage::None, UINT32_MAX);
    CHECK(Restore(fixture) && fixture.Restored());
    return true;
}

struct AbiObject {
    bool Bool(std::uint32_t action) noexcept { return action == 11; }
    float Scalar(std::uint32_t action) noexcept {
        return static_cast<float>(action) + 0.5F;
    }
};

extern "C" bool __fastcall ActiveAbiBoolWrapper(void* self, void*,
                                                std::uint32_t action)
{
    return static_cast<AbiObject*>(self)->Bool(action);
}

extern "C" float __fastcall ActiveAbiFloatWrapper(void* self, void*,
                                                  std::uint32_t action)
{
    return static_cast<AbiObject*>(self)->Scalar(action);
}

bool TestAbi()
{
    std::uintptr_t before = 0;
    std::uintptr_t after = 0;
    std::uintptr_t saved_ebx = 0;
    std::uintptr_t saved_esi = 0;
    std::uintptr_t saved_edi = 0;
    std::uintptr_t observed_ebx = 0;
    std::uintptr_t observed_esi = 0;
    std::uintptr_t observed_edi = 0;
    __asm mov before, esp
    AbiObject object{};
    const bool boolean = ActiveAbiBoolWrapper(&object, nullptr, 11);
    const float scalar = ActiveAbiFloatWrapper(&object, nullptr, 7);
    __asm mov saved_ebx, ebx
    __asm mov saved_esi, esi
    __asm mov saved_edi, edi
    __asm mov ebx, 13579BDFh
    __asm mov esi, 2468ACE0h
    __asm mov edi, 55AA55AAh
    isaac_cs_t0::active::detail::AbiOriginalOnceProbe probe{};
    const bool runtime_wrappers =
        isaac_cs_t0::active::detail::RunAbiOriginalOnceProbe(probe);
    std::uint32_t bootstrap_calls = 0;
    auto bootstrap_original = [&bootstrap_calls]() noexcept { ++bootstrap_calls; };
    CHECK(isaac_cs_t0::active::detail::PassthroughConsumerWithoutSyntheticAuthority(
        false, false, false, bootstrap_original));
    CHECK(bootstrap_calls == 1);
    CHECK(isaac_cs_t0::active::detail::PassthroughConsumerWithoutSyntheticAuthority(
        true, false, true, bootstrap_original));
    CHECK(bootstrap_calls == 2);
    CHECK(isaac_cs_t0::active::detail::PassthroughConsumerWithoutSyntheticAuthority(
        true, true, false, bootstrap_original));
    CHECK(bootstrap_calls == 3);
    CHECK(!isaac_cs_t0::active::detail::PassthroughConsumerWithoutSyntheticAuthority(
        true, true, true, bootstrap_original));
    CHECK(bootstrap_calls == 3);
    __asm mov observed_ebx, ebx
    __asm mov observed_esi, esi
    __asm mov observed_edi, edi
    __asm mov ebx, saved_ebx
    __asm mov esi, saved_esi
    __asm mov edi, saved_edi
    __asm mov after, esp
    CHECK(before == after);
    CHECK(observed_ebx == 0x13579BDFu);
    CHECK(observed_esi == 0x2468ACE0u);
    CHECK(observed_edi == 0x55AA55AAu);
    CHECK(boolean);
    CHECK(std::fabs(scalar - 7.5F) < 0.0001F);
    CHECK(runtime_wrappers);
    CHECK(probe.keyboard_pressed_result);
    CHECK(!probe.keyboard_triggered_result);
    CHECK(probe.gamepad_pressed_result);
    CHECK(!probe.gamepad_triggered_result);
    CHECK(std::fabs(probe.keyboard_scalar_result - 7.5F) < 0.0001F);
    CHECK(std::fabs(probe.gamepad_scalar_result - 8.5F) < 0.0001F);
    CHECK(probe.item_slot == 17);
    return true;
}

bool TestShutdownAndHelper()
{
    using isaac_cs_t0::active::detail::DrainCallbacksBeforeRetirement;
    using isaac_cs_t0::active::detail::RunShutdownStages;
    using isaac_cs_t0::active::detail::ShutdownStageResult;
    bool callbacks_drained = false;
    bool client_retired = false;
    bool retirement_observed_drain = false;
    CHECK(DrainCallbacksBeforeRetirement(
        [&]() noexcept {
            if (client_retired)
                return false;
            callbacks_drained = true;
            return true;
        },
        [&]() noexcept {
            retirement_observed_drain = callbacks_drained;
            client_retired = true;
        }));
    CHECK(callbacks_drained && client_retired && retirement_observed_drain);
    bool retire_called_after_failed_drain = false;
    CHECK(!DrainCallbacksBeforeRetirement(
        []() noexcept { return false; },
        [&]() noexcept { retire_called_after_failed_drain = true; }));
    CHECK(!retire_called_after_failed_drain);
    for (std::uint32_t failure = 0; failure < 5; ++failure)
    {
        std::uint32_t calls = 0;
        const auto phase = [&calls, failure](std::uint32_t index) noexcept {
            ++calls;
            return failure != index;
        };
        const auto result = RunShutdownStages(
            [&]() noexcept { return phase(0); },
            [&]() noexcept { return phase(1); },
            [&]() noexcept { return phase(2); },
            [&]() noexcept { return phase(3); },
            [&]() noexcept { return phase(4); });
        CHECK(result == static_cast<ShutdownStageResult>(failure + 1));
        CHECK(calls == failure + 1);
    }
    CHECK(RunShutdownStages([]() noexcept { return true; },
                            []() noexcept { return true; },
                            []() noexcept { return true; },
                            []() noexcept { return true; },
                            []() noexcept { return true; }) ==
          ShutdownStageResult::Complete);
    PatchFixture segmented;
    CHECK(segmented.memory != nullptr);
    isaac_cs_t0::mode::SetPatchTestFault(
        isaac_cs_t0::mode::PatchTestFaultStage::None, UINT32_MAX);
    CHECK(Install(segmented));
    CHECK(isaac_cs_t0::mode::RestorePatchTransaction(
        segmented.relative.data() + 27, 6, segmented.pointers.data() + 5, 7));
    for (std::size_t index = 0; index < 27; ++index)
        CHECK(segmented.relative[index].installed);
    for (std::size_t index = 27; index < segmented.relative.size(); ++index)
        CHECK(!segmented.relative[index].installed);
    for (std::size_t index = 0; index < 5; ++index)
        CHECK(segmented.pointers[index].installed);
    for (std::size_t index = 5; index < segmented.pointers.size(); ++index)
        CHECK(!segmented.pointers[index].installed);
    CHECK(isaac_cs_t0::mode::RestorePatchTransaction(
        segmented.relative.data(), 27, segmented.pointers.data(), 5));
    CHECK(segmented.Restored());
    CHECK(isaac_cs_t0::helper::kOperations.size() == 14);
    const auto* inject =
        isaac_cs_t0::helper::FindOperation(L"--inject-input-active");
    const auto* unload =
        isaac_cs_t0::helper::FindOperation(L"--unload-input-active");
    CHECK(inject && inject->inject &&
          std::strcmp(inject->entry, "IsaacCsT0InputActiveInitialize") == 0);
    CHECK(unload && !unload->inject &&
          std::strcmp(unload->entry, "IsaacCsT0InputActiveShutdown") == 0);
    const auto* passive_inject =
        isaac_cs_t0::helper::FindOperation(L"--inject-coop-passive");
    const auto* passive_unload =
        isaac_cs_t0::helper::FindOperation(L"--unload-coop-passive");
    CHECK(passive_inject && passive_inject->inject &&
          std::strcmp(passive_inject->entry,
                      "IsaacCsT0CoopPassiveInitialize") == 0);
    CHECK(passive_unload && !passive_unload->inject &&
          std::strcmp(passive_unload->entry,
                      "IsaacCsT0CoopPassiveShutdown") == 0);
    CHECK(isaac_cs_t0::helper::FindOperation(L"--inject-input-active-extra") ==
          nullptr);
    return true;
}

} // namespace

int wmain(int argc, wchar_t** argv)
{
    if (argc == 5 && std::wcscmp(argv[1], L"--publish-active-cross-language") == 0)
        return PublishCrossLanguageFixture(argv[2], argv[3], argv[4]);
    const bool contract = TestContract();
    Suite("contract", 10, contract);
    const bool transaction = TestTransaction();
    Suite("transaction", 185, transaction);
    const bool abi = TestAbi();
    Suite("abi", 4, abi);
    const bool pipeline = TestPipeline();
    Suite("pipeline", 18, pipeline);
    const bool generation = TestAdapter();
    Suite("generation-and-apply", 19, generation);
    const bool witness = TestWitness();
    Suite("witness", 13, witness);
    const bool shutdown = TestShutdownAndHelper();
    Suite("shutdown-and-helper", 13, shutdown);
    const bool passed = g_failures == 0 && contract && transaction && abi &&
                        pipeline && generation && witness && shutdown;
    std::printf(
        "{\"identity\":\"IsaacCsT0-ActiveNativeHarness-v1\",\"totalCases\":"
        "262,\"status\":\"%s\"}\n",
        passed ? "PASS" : "FAIL");
    return passed ? 0 : 1;
}
