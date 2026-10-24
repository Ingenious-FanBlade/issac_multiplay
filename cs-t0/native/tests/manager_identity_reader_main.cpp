#define NOMINMAX
#include <Windows.h>

#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <limits>

#include "isaac_cs_t0/manager_identity_reader.hpp"

namespace {

using namespace isaac_cs_t0::manager_identity;

std::uint32_t g_assertions = 0u;
std::uint32_t g_failures = 0u;

#define CHECK(expression)                                                       \
    do {                                                                        \
        ++g_assertions;                                                         \
        if (!(expression)) {                                                    \
            std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__,      \
                         #expression);                                           \
            ++g_failures;                                                       \
        }                                                                       \
    } while (false)

constexpr ProcessIdentity kProcess{0x1234u, 0x0102030405060708ull};
constexpr std::uint32_t kThread = 0x55u;
constexpr std::uintptr_t kRootA = 0x10000000u;
constexpr std::uintptr_t kRootB = 0x20000000u;
constexpr std::uintptr_t kInputVptr = 0x30000000u;

SlotSample Stable(std::uintptr_t value) noexcept
{
    return {true, true, value, value};
}

std::uintptr_t Manager(std::uintptr_t root) noexcept
{
    return root + kPlayerManagerOffset;
}

Reader ReadyReader()
{
    Reader reader;
    CHECK(reader.Initialize(kProcess, kThread));
    CHECK(reader.ValidateInputManager(kProcess, kThread, Stable(kInputVptr),
                                      kInputVptr));
    return reader;
}

void TestCompleteEpoch()
{
    auto reader = ReadyReader();
    CHECK(reader.ObserveCaller(kProcess, kThread, Stable(kRootA),
                               Manager(kRootA)));
    CHECK(reader.ObserveManagerEntry(kProcess, kThread, Stable(kRootA),
                                     Manager(kRootA)));
    CHECK(reader.ObserveOrdinaryReload(kProcess, kThread, Stable(kRootA),
                                       Manager(kRootA)));
    CHECK(reader.ConfirmSpawnEligibility(kProcess, kThread, Manager(kRootA)));
    CHECK(reader.snapshot().state == State::SpawnValidated);
    CHECK(!reader.snapshot().publication_eligible);
    CHECK(reader.ObserveManagerNormalExit(kProcess, kThread));
    CHECK(reader.snapshot().state == State::ManagerExited);
    CHECK(!reader.snapshot().publication_eligible);
    CHECK(reader.ObserveCallerNormalExit(kProcess, kThread));
    const auto snapshot = reader.snapshot();
    CHECK(snapshot.state == State::Eligible);
    CHECK(snapshot.invalidation == Invalidation::None);
    CHECK(snapshot.process == kProcess);
    CHECK(snapshot.main_thread_id == kThread);
    CHECK(snapshot.game_root == kRootA);
    CHECK(snapshot.player_manager == Manager(kRootA));
    CHECK(snapshot.input_manager_vptr == kInputVptr);
    CHECK(snapshot.generation == 1u);
    CHECK(snapshot.epoch_generation == snapshot.generation);
    CHECK(snapshot.stable_samples == 3u);
    CHECK(snapshot.read_operations == 8u);
    CHECK(snapshot.read_bytes == 32u);
    CHECK(snapshot.publication_eligible);
    CHECK(snapshot.input_manager_validated);
    CHECK(!snapshot.permanently_disarmed);
    CHECK(!snapshot.unsafe_retain_module);
}

void TestFailClosedSamples()
{
    {
        auto reader = ReadyReader();
        CHECK(!reader.ObserveCaller(kProcess, kThread,
                                    {false, true, kRootA, kRootA},
                                    Manager(kRootA)));
        CHECK(reader.snapshot().invalidation == Invalidation::ReadFailure);
        CHECK(reader.snapshot().read_operations == 4u);
    }
    {
        auto reader = ReadyReader();
        CHECK(!reader.ObserveCaller(kProcess, kThread,
                                    {true, false, kRootA, kRootA},
                                    Manager(kRootA)));
        CHECK(reader.snapshot().invalidation == Invalidation::ReadFailure);
    }
    {
        auto reader = ReadyReader();
        CHECK(!reader.ObserveCaller(kProcess, kThread,
                                    {true, true, kRootA, kRootB},
                                    Manager(kRootA)));
        CHECK(reader.snapshot().invalidation == Invalidation::DoubleReadDrift);
    }
    {
        auto reader = ReadyReader();
        CHECK(!reader.ObserveCaller(kProcess, kThread, Stable(0u),
                                    Manager(kRootA)));
        CHECK(reader.snapshot().invalidation == Invalidation::ReceiverMismatch);
    }
    {
        auto reader = ReadyReader();
        const auto near_end = std::numeric_limits<std::uintptr_t>::max() - 1u;
        CHECK(!reader.ObserveCaller(kProcess, kThread, Stable(near_end),
                                    near_end));
        CHECK(reader.snapshot().invalidation == Invalidation::ReceiverMismatch);
    }
    {
        auto reader = ReadyReader();
        CHECK(!reader.ObserveCaller(kProcess, kThread, Stable(kRootA),
                                    Manager(kRootA) + 4u));
        CHECK(reader.snapshot().invalidation == Invalidation::ReceiverMismatch);
    }
}

void TestIdentityAndOrder()
{
    {
        auto reader = ReadyReader();
        CHECK(!reader.ObserveCaller({kProcess.process_id + 1u,
                                     kProcess.creation_time},
                                    kThread, Stable(kRootA), Manager(kRootA)));
        CHECK(reader.snapshot().invalidation == Invalidation::ProcessReused);
    }
    {
        auto reader = ReadyReader();
        CHECK(!reader.ObserveCaller({kProcess.process_id,
                                     kProcess.creation_time + 1u},
                                    kThread, Stable(kRootA), Manager(kRootA)));
        CHECK(reader.snapshot().invalidation == Invalidation::ProcessReused);
    }
    {
        auto reader = ReadyReader();
        CHECK(!reader.ObserveCaller(kProcess, kThread + 1u, Stable(kRootA),
                                    Manager(kRootA)));
        CHECK(reader.snapshot().invalidation == Invalidation::WrongThread);
    }
    {
        auto reader = ReadyReader();
        CHECK(!reader.ObserveManagerEntry(kProcess, kThread, Stable(kRootA),
                                          Manager(kRootA)));
        CHECK(reader.snapshot().invalidation == Invalidation::WrongOrder);
    }
    {
        auto reader = ReadyReader();
        CHECK(reader.ObserveCaller(kProcess, kThread, Stable(kRootA),
                                   Manager(kRootA)));
        CHECK(!reader.ConfirmSpawnEligibility(kProcess, kThread,
                                              Manager(kRootA)));
        CHECK(reader.snapshot().invalidation == Invalidation::WrongOrder);
    }
}

void TestDuplicateAndInvalidIdentityEdges()
{
    {
        Reader reader;
        CHECK(!reader.Initialize({0u, kProcess.creation_time}, kThread));
        CHECK(reader.snapshot().state == State::PermanentlyDisarmed);
        CHECK(reader.snapshot().invalidation == Invalidation::WrongOrder);
    }
    {
        Reader reader;
        CHECK(!reader.Initialize({kProcess.process_id, 0u}, kThread));
        CHECK(reader.snapshot().invalidation == Invalidation::WrongOrder);
    }
    {
        Reader reader;
        CHECK(!reader.Initialize(kProcess, 0u));
        CHECK(reader.snapshot().invalidation == Invalidation::WrongOrder);
    }
    {
        auto reader = ReadyReader();
        CHECK(!reader.Initialize(kProcess, kThread));
        CHECK(reader.snapshot().invalidation == Invalidation::WrongOrder);
    }
    {
        auto reader = ReadyReader();
        CHECK(!reader.ValidateInputManager(kProcess, kThread,
                                           Stable(kInputVptr), kInputVptr));
        CHECK(reader.snapshot().invalidation == Invalidation::WrongOrder);
    }
    {
        auto reader = ReadyReader();
        CHECK(!reader.ObserveCaller({0u, kProcess.creation_time}, kThread,
                                    Stable(kRootA), Manager(kRootA)));
        CHECK(reader.snapshot().invalidation == Invalidation::ProcessReused);
    }
    {
        auto reader = ReadyReader();
        CHECK(!reader.ObserveCaller(kProcess, 0u, Stable(kRootA),
                                    Manager(kRootA)));
        CHECK(reader.snapshot().invalidation == Invalidation::WrongThread);
    }
    {
        auto reader = ReadyReader();
        CHECK(reader.ObserveCaller(kProcess, kThread, Stable(kRootA),
                                   Manager(kRootA)));
        CHECK(!reader.ObserveCaller(kProcess, kThread, Stable(kRootA),
                                    Manager(kRootA)));
        CHECK(reader.snapshot().invalidation == Invalidation::WrongOrder);
    }
    {
        auto reader = ReadyReader();
        CHECK(reader.ObserveCaller(kProcess, kThread, Stable(kRootA),
                                   Manager(kRootA)));
        CHECK(reader.ObserveManagerEntry(kProcess, kThread, Stable(kRootA),
                                         Manager(kRootA)));
        CHECK(!reader.ObserveManagerEntry(kProcess, kThread, Stable(kRootA),
                                          Manager(kRootA)));
        CHECK(reader.snapshot().invalidation == Invalidation::WrongOrder);
    }
    {
        auto reader = ReadyReader();
        CHECK(reader.ObserveCaller(kProcess, kThread, Stable(kRootA),
                                   Manager(kRootA)));
        CHECK(reader.ObserveManagerEntry(kProcess, kThread, Stable(kRootA),
                                         Manager(kRootA)));
        CHECK(reader.ObserveOrdinaryReload(kProcess, kThread, Stable(kRootA),
                                           Manager(kRootA)));
        CHECK(!reader.ObserveOrdinaryReload(kProcess, kThread, Stable(kRootA),
                                            Manager(kRootA)));
        CHECK(reader.snapshot().invalidation == Invalidation::WrongOrder);
    }
    {
        auto reader = ReadyReader();
        CHECK(reader.ObserveCaller(kProcess, kThread, Stable(kRootA),
                                   Manager(kRootA)));
        CHECK(reader.ObserveManagerEntry(kProcess, kThread, Stable(kRootA),
                                         Manager(kRootA)));
        CHECK(reader.ObserveOrdinaryReload(kProcess, kThread, Stable(kRootA),
                                           Manager(kRootA)));
        CHECK(reader.ConfirmSpawnEligibility(kProcess, kThread,
                                             Manager(kRootA)));
        CHECK(!reader.ConfirmSpawnEligibility(kProcess, kThread,
                                              Manager(kRootA)));
        CHECK(reader.snapshot().invalidation == Invalidation::WrongOrder);
    }
    {
        auto reader = ReadyReader();
        CHECK(reader.ObserveCaller(kProcess, kThread, Stable(kRootA),
                                   Manager(kRootA)));
        CHECK(reader.ObserveManagerEntry(kProcess, kThread, Stable(kRootA),
                                         Manager(kRootA)));
        CHECK(reader.ObserveOrdinaryReload(kProcess, kThread, Stable(kRootA),
                                           Manager(kRootA)));
        CHECK(reader.ConfirmSpawnEligibility(kProcess, kThread,
                                             Manager(kRootA)));
        CHECK(!reader.ObserveCallerNormalExit(kProcess, kThread));
        CHECK(reader.snapshot().invalidation == Invalidation::WrongOrder);
    }
    {
        auto reader = ReadyReader();
        CHECK(reader.ObserveCaller(kProcess, kThread, Stable(kRootA),
                                   Manager(kRootA)));
        CHECK(reader.ObserveManagerEntry(kProcess, kThread, Stable(kRootA),
                                         Manager(kRootA)));
        CHECK(reader.ObserveOrdinaryReload(kProcess, kThread, Stable(kRootA),
                                           Manager(kRootA)));
        CHECK(reader.ConfirmSpawnEligibility(kProcess, kThread,
                                             Manager(kRootA)));
        CHECK(reader.ObserveManagerNormalExit(kProcess, kThread));
        CHECK(!reader.ObserveManagerNormalExit(kProcess, kThread));
        CHECK(reader.snapshot().invalidation == Invalidation::WrongOrder);
    }
    {
        auto reader = ReadyReader();
        CHECK(reader.ObserveCaller(kProcess, kThread, Stable(kRootA),
                                   Manager(kRootA)));
        CHECK(reader.ObserveManagerEntry(kProcess, kThread, Stable(kRootA),
                                         Manager(kRootA)));
        CHECK(reader.ObserveOrdinaryReload(kProcess, kThread, Stable(kRootA),
                                           Manager(kRootA)));
        CHECK(reader.ConfirmSpawnEligibility(kProcess, kThread,
                                             Manager(kRootA)));
        CHECK(reader.ObserveManagerNormalExit(kProcess, kThread));
        CHECK(reader.ObserveCallerNormalExit(kProcess, kThread));
        CHECK(!reader.ObserveCallerNormalExit(kProcess, kThread));
        CHECK(reader.snapshot().invalidation == Invalidation::WrongOrder);
        CHECK(!reader.snapshot().publication_eligible);
    }
    {
        auto reader = ReadyReader();
        reader.Invalidate(Invalidation::CallbackReentry);
        reader.Invalidate(Invalidation::ExceptionResume);
        const auto snapshot = reader.snapshot();
        CHECK(snapshot.invalidation == Invalidation::CallbackReentry);
        CHECK(snapshot.generation == 2u);
        CHECK(snapshot.state == State::PermanentlyDisarmed);
    }
}

void TestInputManagerIdentity()
{
    {
        Reader reader;
        CHECK(reader.Initialize(kProcess, kThread));
        CHECK(!reader.ObserveCaller(kProcess, kThread, Stable(kRootA),
                                    Manager(kRootA)));
        CHECK(reader.snapshot().invalidation == Invalidation::Lifecycle);
    }
    {
        Reader reader;
        CHECK(reader.Initialize(kProcess, kThread));
        CHECK(!reader.ValidateInputManager(kProcess, kThread,
                                           {false, true, kInputVptr, kInputVptr},
                                           kInputVptr));
        CHECK(reader.snapshot().invalidation == Invalidation::ReadFailure);
    }
    {
        Reader reader;
        CHECK(reader.Initialize(kProcess, kThread));
        CHECK(!reader.ValidateInputManager(kProcess, kThread,
                                           {true, true, kInputVptr,
                                            kInputVptr + 4u},
                                           kInputVptr));
        CHECK(reader.snapshot().invalidation ==
              Invalidation::DoubleReadDrift);
    }
    {
        Reader reader;
        CHECK(reader.Initialize(kProcess, kThread));
        CHECK(!reader.ValidateInputManager(kProcess, kThread,
                                           Stable(kInputVptr - 0x15Cu),
                                           kInputVptr));
        CHECK(reader.snapshot().invalidation == Invalidation::Lifecycle);
    }
}

void TestGenerationAndInvalidation()
{
    {
        auto reader = ReadyReader();
        CHECK(reader.ObserveCaller(kProcess, kThread, Stable(kRootA),
                                   Manager(kRootA)));
        reader.Invalidate(Invalidation::SlotWriter);
        const auto invalidated = reader.snapshot();
        CHECK(invalidated.generation == 2u);
        CHECK(invalidated.epoch_generation == 1u);
        CHECK(invalidated.invalidation == Invalidation::SlotWriter);
        CHECK(invalidated.permanently_disarmed);
        CHECK(!invalidated.publication_eligible);
        CHECK(!reader.ObserveManagerEntry(kProcess, kThread, Stable(kRootA),
                                          Manager(kRootA)));
        CHECK(reader.snapshot().generation == 2u);
        CHECK(reader.snapshot().invalidation == Invalidation::SlotWriter);
    }
    {
        auto reader = ReadyReader();
        CHECK(reader.ObserveCaller(kProcess, kThread, Stable(kRootA),
                                   Manager(kRootA)));
        CHECK(!reader.ObserveManagerEntry(kProcess, kThread, Stable(kRootB),
                                          Manager(kRootB)));
        CHECK(reader.snapshot().invalidation == Invalidation::OwnerReplaced);
    }

    for (std::uint32_t raw = static_cast<std::uint32_t>(Invalidation::SlotWriter);
         raw <= static_cast<std::uint32_t>(Invalidation::ExceptionResume); ++raw)
    {
        auto reader = ReadyReader();
        reader.Invalidate(static_cast<Invalidation>(raw));
        CHECK(reader.snapshot().state == State::PermanentlyDisarmed);
        CHECK(reader.snapshot().invalidation == static_cast<Invalidation>(raw));
        CHECK(reader.snapshot().generation == 2u);
    }

    {
        auto reader = ReadyReader();
        reader.MarkUncertainStop();
        CHECK(reader.snapshot().state == State::UnsafeRetainModule);
        CHECK(reader.snapshot().invalidation == Invalidation::UncertainStop);
        CHECK(reader.snapshot().unsafe_retain_module);
        reader.Retire();
        CHECK(reader.snapshot().state == State::UnsafeRetainModule);
    }
    {
        auto reader = ReadyReader();
        reader.Retire();
        CHECK(reader.snapshot().state == State::Retired);
        CHECK(reader.snapshot().invalidation == Invalidation::Lifecycle);
        CHECK(reader.snapshot().generation == 2u);
        CHECK(reader.snapshot().permanently_disarmed);
    }
    {
        auto reader = ReadyReader();
        reader.SetCountersForTest(std::numeric_limits<std::uint64_t>::max(),
                                  reader.snapshot().read_operations,
                                  reader.snapshot().read_bytes);
        reader.Invalidate(Invalidation::SlotWriter);
        CHECK(reader.snapshot().invalidation ==
              Invalidation::GenerationOverflow);
        CHECK(reader.snapshot().generation ==
              std::numeric_limits<std::uint64_t>::max());
        CHECK(reader.snapshot().permanently_disarmed);
    }
    {
        auto reader = ReadyReader();
        reader.SetCountersForTest(reader.snapshot().generation,
                                  kMaximumReadOperations,
                                  kMaximumReadBytes);
        CHECK(!reader.ObserveCaller(kProcess, kThread, Stable(kRootA),
                                    Manager(kRootA)));
        CHECK(reader.snapshot().invalidation == Invalidation::BudgetExceeded);
    }
}

class CApiFixture {
public:
    CApiFixture()
    {
        image = static_cast<std::uint8_t*>(VirtualAlloc(
            nullptr, 0x00900000u, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
        root = static_cast<std::uint8_t*>(VirtualAlloc(
            nullptr, 0x00030000u, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
        if (image && root)
        {
            *reinterpret_cast<void**>(image + kGameRootSlotRva) = root;
            *reinterpret_cast<void**>(image + kInputManagerObjectRva) =
                image + kInputManagerDerivedVtableRva;
        }
    }

    ~CApiFixture()
    {
        if (root)
            VirtualFree(root, 0, MEM_RELEASE);
        if (image)
            VirtualFree(image, 0, MEM_RELEASE);
    }

    bool ready() const noexcept { return image && root; }
    std::uintptr_t manager() const noexcept
    {
        return reinterpret_cast<std::uintptr_t>(root) + kPlayerManagerOffset;
    }

    std::uint8_t* image{};
    std::uint8_t* root{};
};

void TestProductionInvalidationAndRawReadFailures()
{
    IsaacCsT0ManagerIdentitySnapshotV1 snapshot{};

    {
        CApiFixture fixture;
        CHECK(fixture.ready());
        void* handle = IsaacCsT0ManagerIdentityCreate(
            reinterpret_cast<std::uintptr_t>(fixture.image));
        CHECK(handle != nullptr);
        if (handle)
        {
            CHECK(IsaacCsT0ManagerIdentityObserveCaller(
                      handle, fixture.manager()) == 1u);
            IsaacCsT0ManagerIdentityInvalidate(
                handle, static_cast<std::uint32_t>(Invalidation::SlotWriter));
            *reinterpret_cast<void**>(fixture.image + kGameRootSlotRva) =
                fixture.root + 4u;
            *reinterpret_cast<void**>(fixture.image + kGameRootSlotRva) =
                fixture.root;
            CHECK(IsaacCsT0ManagerIdentityObserveManagerEntry(
                      handle, fixture.manager()) == 0u);
            CHECK(IsaacCsT0ManagerIdentityReadSnapshot(handle, &snapshot) ==
                  1u);
            CHECK(snapshot.invalidation ==
                  static_cast<std::uint32_t>(Invalidation::SlotWriter));
            CHECK(snapshot.generation == 2u);
            CHECK(snapshot.epoch_generation == 1u);
            CHECK(snapshot.permanently_disarmed == 1u);
            CHECK(snapshot.publication_eligible == 0u);
            IsaacCsT0ManagerIdentityDestroy(handle);
        }
    }

    {
        CApiFixture fixture;
        CHECK(fixture.ready());
        void* handle = IsaacCsT0ManagerIdentityCreate(
            reinterpret_cast<std::uintptr_t>(fixture.image));
        CHECK(handle != nullptr);
        if (handle)
        {
            CHECK(IsaacCsT0ManagerIdentityObserveCaller(
                      handle, fixture.manager()) == 1u);
            IsaacCsT0ManagerIdentityInvalidate(
                handle, static_cast<std::uint32_t>(
                            Invalidation::ComputedAliasWriter));
            *reinterpret_cast<void**>(fixture.image + kGameRootSlotRva) =
                fixture.root;
            CHECK(IsaacCsT0ManagerIdentityObserveManagerEntry(
                      handle, fixture.manager()) == 0u);
            CHECK(IsaacCsT0ManagerIdentityReadSnapshot(handle, &snapshot) ==
                  1u);
            CHECK(snapshot.invalidation == static_cast<std::uint32_t>(
                  Invalidation::ComputedAliasWriter));
            CHECK(snapshot.generation == 2u);
            CHECK(snapshot.publication_eligible == 0u);
            IsaacCsT0ManagerIdentityDestroy(handle);
        }
    }

    for (const auto reason : {Invalidation::Lifecycle,
                              Invalidation::CallbackReentry,
                              Invalidation::ExceptionResume})
    {
        CApiFixture fixture;
        CHECK(fixture.ready());
        void* handle = IsaacCsT0ManagerIdentityCreate(
            reinterpret_cast<std::uintptr_t>(fixture.image));
        CHECK(handle != nullptr);
        if (handle)
        {
            IsaacCsT0ManagerIdentityInvalidate(
                handle, static_cast<std::uint32_t>(reason));
            CHECK(IsaacCsT0ManagerIdentityReadSnapshot(handle, &snapshot) ==
                  1u);
            CHECK(snapshot.invalidation == static_cast<std::uint32_t>(reason));
            CHECK(snapshot.state ==
                  static_cast<std::uint32_t>(State::PermanentlyDisarmed));
            CHECK(snapshot.generation == 2u);
            CHECK(snapshot.permanently_disarmed == 1u);
            IsaacCsT0ManagerIdentityDestroy(handle);
        }
    }

    {
        CApiFixture fixture;
        CHECK(fixture.ready());
        void* handle = IsaacCsT0ManagerIdentityCreate(
            reinterpret_cast<std::uintptr_t>(fixture.image));
        CHECK(handle != nullptr);
        if (handle)
        {
            IsaacCsT0ManagerIdentityRetire(handle);
            CHECK(IsaacCsT0ManagerIdentityReadSnapshot(handle, &snapshot) ==
                  1u);
            CHECK(snapshot.state ==
                  static_cast<std::uint32_t>(State::Retired));
            CHECK(snapshot.invalidation ==
                  static_cast<std::uint32_t>(Invalidation::Lifecycle));
            CHECK(snapshot.generation == 2u);
            CHECK(snapshot.permanently_disarmed == 1u);
            CHECK(snapshot.publication_eligible == 0u);
            IsaacCsT0ManagerIdentityRetire(handle);
            CHECK(IsaacCsT0ManagerIdentityReadSnapshot(handle, &snapshot) ==
                  1u);
            CHECK(snapshot.state ==
                  static_cast<std::uint32_t>(State::Retired));
            CHECK(snapshot.generation == 2u);
            IsaacCsT0ManagerIdentityDestroy(handle);
        }
    }

    for (const auto raw : {0u, 0xFFFFFFFFu})
    {
        CApiFixture fixture;
        CHECK(fixture.ready());
        void* handle = IsaacCsT0ManagerIdentityCreate(
            reinterpret_cast<std::uintptr_t>(fixture.image));
        CHECK(handle != nullptr);
        if (handle)
        {
            IsaacCsT0ManagerIdentityInvalidate(handle, raw);
            CHECK(IsaacCsT0ManagerIdentityReadSnapshot(handle, &snapshot) ==
                  1u);
            CHECK(snapshot.invalidation == static_cast<std::uint32_t>(
                  Invalidation::ComputedAliasWriter));
            CHECK(snapshot.generation == 2u);
            IsaacCsT0ManagerIdentityDestroy(handle);
        }
    }

    for (const auto rva : {kInputManagerObjectRva, kGameRootSlotRva})
    {
        CApiFixture fixture;
        CHECK(fixture.ready());
        void* handle = IsaacCsT0ManagerIdentityCreate(
            reinterpret_cast<std::uintptr_t>(fixture.image));
        CHECK(handle != nullptr);
        if (!handle)
            continue;

        SYSTEM_INFO system{};
        GetSystemInfo(&system);
        const auto page_mask =
            static_cast<std::uintptr_t>(system.dwPageSize - 1u);
        auto* const page = fixture.image + (rva & ~page_mask);
        DWORD old_protection = 0u;
        CHECK(VirtualProtect(page, system.dwPageSize, PAGE_NOACCESS,
                             &old_protection) != 0);
        CHECK(IsaacCsT0ManagerIdentityObserveCaller(
                  handle, fixture.manager()) == 0u);
        DWORD ignored = 0u;
        CHECK(VirtualProtect(page, system.dwPageSize, old_protection,
                             &ignored) != 0);
        CHECK(IsaacCsT0ManagerIdentityReadSnapshot(handle, &snapshot) == 1u);
        CHECK(snapshot.invalidation ==
              static_cast<std::uint32_t>(Invalidation::ReadFailure));
        CHECK(snapshot.permanently_disarmed == 1u);
        CHECK(snapshot.publication_eligible == 0u);
        IsaacCsT0ManagerIdentityDestroy(handle);
    }
}

struct WrongThreadProbe {
    void* handle{};
    std::uintptr_t receiver{};
    std::uint32_t result{1u};
};

DWORD WINAPI ObserveCallerOnWrongThread(void* opaque)
{
    auto& probe = *static_cast<WrongThreadProbe*>(opaque);
    probe.result = IsaacCsT0ManagerIdentityObserveCaller(probe.handle,
                                                         probe.receiver);
    return 0u;
}

void TestReachableProductionApi()
{
    static_assert(sizeof(void*) == 4u);
    static_assert(sizeof(IsaacCsT0ManagerIdentitySnapshotV1) == 80u);

    CApiFixture fixture;
    CHECK(fixture.ready());
    void* handle = IsaacCsT0ManagerIdentityCreate(
        reinterpret_cast<std::uintptr_t>(fixture.image));
    CHECK(handle != nullptr);
    if (!handle)
        return;

    CHECK(IsaacCsT0ManagerIdentityObserveCaller(handle, fixture.manager()) == 1u);
    CHECK(IsaacCsT0ManagerIdentityObserveManagerEntry(handle,
                                                      fixture.manager()) == 1u);
    CHECK(IsaacCsT0ManagerIdentityObserveOrdinaryReload(handle,
                                                        fixture.manager()) == 1u);
    CHECK(IsaacCsT0ManagerIdentityConfirmSpawn(handle, fixture.manager()) == 1u);
    CHECK(IsaacCsT0ManagerIdentityObserveManagerNormalExit(handle) == 1u);
    CHECK(IsaacCsT0ManagerIdentityObserveCallerNormalExit(handle) == 1u);
    IsaacCsT0ManagerIdentitySnapshotV1 snapshot{};
    CHECK(IsaacCsT0ManagerIdentityReadSnapshot(handle, &snapshot) == 1u);
    CHECK(snapshot.state == static_cast<std::uint32_t>(State::Eligible));
    CHECK(snapshot.process_id == GetCurrentProcessId());
    CHECK(snapshot.main_thread_id == GetCurrentThreadId());
    CHECK(snapshot.process_creation_time != 0u);
    CHECK(snapshot.game_root == reinterpret_cast<std::uintptr_t>(fixture.root));
    CHECK(snapshot.player_manager == fixture.manager());
    CHECK(snapshot.input_manager_vptr ==
          reinterpret_cast<std::uintptr_t>(fixture.image) +
              kInputManagerDerivedVtableRva);
    CHECK(snapshot.generation == snapshot.epoch_generation);
    CHECK(snapshot.stable_samples == 3u);
    CHECK(snapshot.read_operations == 8u);
    CHECK(snapshot.read_bytes == 32u);
    CHECK(snapshot.publication_eligible == 1u);
    CHECK(snapshot.input_manager_validated == 1u);
    IsaacCsT0ManagerIdentityDestroy(handle);

    handle = IsaacCsT0ManagerIdentityCreate(
        reinterpret_cast<std::uintptr_t>(fixture.image));
    CHECK(handle != nullptr);
    if (handle)
    {
        CHECK(IsaacCsT0ManagerIdentityObserveCaller(handle,
                                                    fixture.manager() + 4u) == 0u);
        CHECK(IsaacCsT0ManagerIdentityReadSnapshot(handle, &snapshot) == 1u);
        CHECK(snapshot.invalidation ==
              static_cast<std::uint32_t>(Invalidation::ReceiverMismatch));
        CHECK(snapshot.permanently_disarmed == 1u);
        CHECK(IsaacCsT0ManagerIdentityObserveCaller(handle,
                                                    fixture.manager()) == 0u);
        IsaacCsT0ManagerIdentityDestroy(handle);
    }

    handle = IsaacCsT0ManagerIdentityCreate(
        reinterpret_cast<std::uintptr_t>(fixture.image));
    CHECK(handle != nullptr);
    if (handle)
    {
        WrongThreadProbe probe{handle, fixture.manager(), 1u};
        HANDLE thread = CreateThread(nullptr, 0, ObserveCallerOnWrongThread,
                                     &probe, 0, nullptr);
        CHECK(thread != nullptr);
        if (thread)
        {
            CHECK(WaitForSingleObject(thread, 5000u) == WAIT_OBJECT_0);
            CHECK(CloseHandle(thread) != 0);
        }
        CHECK(probe.result == 0u);
        CHECK(IsaacCsT0ManagerIdentityReadSnapshot(handle, &snapshot) == 1u);
        CHECK(snapshot.invalidation ==
              static_cast<std::uint32_t>(Invalidation::WrongThread));
        CHECK(snapshot.permanently_disarmed == 1u);
        IsaacCsT0ManagerIdentityDestroy(handle);
    }

    handle = IsaacCsT0ManagerIdentityCreate(
        reinterpret_cast<std::uintptr_t>(fixture.image));
    CHECK(handle != nullptr);
    if (handle)
    {
        IsaacCsT0ManagerIdentityInvalidate(
            handle, static_cast<std::uint32_t>(Invalidation::CallbackReentry));
        CHECK(IsaacCsT0ManagerIdentityReadSnapshot(handle, &snapshot) == 1u);
        CHECK(snapshot.invalidation ==
              static_cast<std::uint32_t>(Invalidation::CallbackReentry));
        CHECK(snapshot.generation == 2u);
        IsaacCsT0ManagerIdentityDestroy(handle);
    }

    handle = IsaacCsT0ManagerIdentityCreate(
        reinterpret_cast<std::uintptr_t>(fixture.image));
    CHECK(handle != nullptr);
    if (handle)
    {
        IsaacCsT0ManagerIdentityUncertainStop(handle);
        CHECK(IsaacCsT0ManagerIdentityReadSnapshot(handle, &snapshot) == 1u);
        CHECK(snapshot.state ==
              static_cast<std::uint32_t>(State::UnsafeRetainModule));
        CHECK(snapshot.unsafe_retain_module == 1u);
        const auto generation = snapshot.generation;
        IsaacCsT0ManagerIdentityDestroy(handle);
        CHECK(IsaacCsT0ManagerIdentityReadSnapshot(handle, &snapshot) == 1u);
        CHECK(snapshot.state ==
              static_cast<std::uint32_t>(State::UnsafeRetainModule));
        CHECK(snapshot.generation == generation);
        IsaacCsT0ManagerIdentityUncertainStop(handle);
        IsaacCsT0ManagerIdentityRetire(handle);
        CHECK(IsaacCsT0ManagerIdentityReadSnapshot(handle, &snapshot) == 1u);
        CHECK(snapshot.state ==
              static_cast<std::uint32_t>(State::UnsafeRetainModule));
        CHECK(snapshot.invalidation ==
              static_cast<std::uint32_t>(Invalidation::UncertainStop));
        // The intentionally retained handle is not dereferenced after module
        // unload; the owner must retain the module on this state.
    }

    CHECK(IsaacCsT0ManagerIdentityReadSnapshot(nullptr, &snapshot) == 0u);
    CHECK(IsaacCsT0ManagerIdentityReadSnapshot(handle, nullptr) == 0u);
}

template <typename Function>
Function Export(HMODULE module, const char* name)
{
    const auto raw = GetProcAddress(module, name);
    CHECK(raw != nullptr);
    return reinterpret_cast<Function>(raw);
}

void TestBuiltProductionDll(const char* path)
{
    CHECK(path != nullptr && path[0] != '\0');
    HMODULE module = path ? LoadLibraryA(path) : nullptr;
    CHECK(module != nullptr);
    if (!module)
        return;

    using Create = void*(__stdcall*)(std::uintptr_t);
    using Destroy = void(__stdcall*)(void*);
    using Observe = std::uint32_t(__stdcall*)(void*, std::uintptr_t);
    using Exit = std::uint32_t(__stdcall*)(void*);
    using Invalidate = void(__stdcall*)(void*, std::uint32_t);
    using Stop = void(__stdcall*)(void*);
    using SnapshotFn = std::uint32_t(__stdcall*)(
        const void*, IsaacCsT0ManagerIdentitySnapshotV1*);

    const auto create = Export<Create>(module, "IsaacCsT0ManagerIdentityCreate");
    const auto destroy = Export<Destroy>(module, "IsaacCsT0ManagerIdentityDestroy");
    const auto caller = Export<Observe>(module, "IsaacCsT0ManagerIdentityObserveCaller");
    const auto manager = Export<Observe>(module, "IsaacCsT0ManagerIdentityObserveManagerEntry");
    const auto ordinary = Export<Observe>(module, "IsaacCsT0ManagerIdentityObserveOrdinaryReload");
    const auto spawn = Export<Observe>(module, "IsaacCsT0ManagerIdentityConfirmSpawn");
    const auto manager_exit = Export<Exit>(
        module, "IsaacCsT0ManagerIdentityObserveManagerNormalExit");
    const auto caller_exit = Export<Exit>(
        module, "IsaacCsT0ManagerIdentityObserveCallerNormalExit");
    const auto invalidate = Export<Invalidate>(module,
        "IsaacCsT0ManagerIdentityInvalidate");
    const auto uncertain = Export<Stop>(module,
        "IsaacCsT0ManagerIdentityUncertainStop");
    const auto retire = Export<Stop>(module, "IsaacCsT0ManagerIdentityRetire");
    const auto snapshot_fn = Export<SnapshotFn>(module,
        "IsaacCsT0ManagerIdentityReadSnapshot");

    CApiFixture fixture;
    CHECK(fixture.ready());
    void* handle = create(reinterpret_cast<std::uintptr_t>(fixture.image));
    CHECK(handle != nullptr);
    if (handle)
    {
        CHECK(caller(handle, fixture.manager()) == 1u);
        CHECK(manager(handle, fixture.manager()) == 1u);
        CHECK(ordinary(handle, fixture.manager()) == 1u);
        CHECK(spawn(handle, fixture.manager()) == 1u);
        IsaacCsT0ManagerIdentitySnapshotV1 value{};
        CHECK(snapshot_fn(handle, &value) == 1u);
        CHECK(value.state == static_cast<std::uint32_t>(State::SpawnValidated));
        CHECK(value.publication_eligible == 0u);
        CHECK(manager_exit(handle) == 1u);
        CHECK(snapshot_fn(handle, &value) == 1u);
        CHECK(value.state == static_cast<std::uint32_t>(State::ManagerExited));
        CHECK(value.publication_eligible == 0u);
        CHECK(caller_exit(handle) == 1u);
        CHECK(snapshot_fn(handle, &value) == 1u);
        CHECK(value.state == static_cast<std::uint32_t>(State::Eligible));
        CHECK(value.publication_eligible == 1u);
        invalidate(handle,
                   static_cast<std::uint32_t>(Invalidation::SlotWriter));
        CHECK(snapshot_fn(handle, &value) == 1u);
        CHECK(value.state ==
              static_cast<std::uint32_t>(State::PermanentlyDisarmed));
        CHECK(value.invalidation ==
              static_cast<std::uint32_t>(Invalidation::SlotWriter));
        CHECK(value.generation == 2u);
        CHECK(value.epoch_generation == 1u);
        CHECK(value.publication_eligible == 0u);
        invalidate(handle,
                   static_cast<std::uint32_t>(Invalidation::ExceptionResume));
        CHECK(snapshot_fn(handle, &value) == 1u);
        CHECK(value.invalidation ==
              static_cast<std::uint32_t>(Invalidation::SlotWriter));
        CHECK(value.generation == 2u);
        uncertain(nullptr);
        retire(nullptr);
        destroy(handle);
    }
    CHECK(create(0u) == nullptr);
    CHECK(create(std::numeric_limits<std::uintptr_t>::max()) == nullptr);
    CHECK(caller(nullptr, 0u) == 0u);
    CHECK(manager(nullptr, 0u) == 0u);
    CHECK(ordinary(nullptr, 0u) == 0u);
    CHECK(spawn(nullptr, 0u) == 0u);
    CHECK(manager_exit(nullptr) == 0u);
    CHECK(caller_exit(nullptr) == 0u);
    CHECK(FreeLibrary(module) != 0);
}

} // namespace

int main(int count, char** arguments)
{
    TestCompleteEpoch();
    TestFailClosedSamples();
    TestIdentityAndOrder();
    TestDuplicateAndInvalidIdentityEdges();
    TestInputManagerIdentity();
    TestGenerationAndInvalidation();
    TestProductionInvalidationAndRawReadFailures();
    TestReachableProductionApi();
    CHECK(count == 2);
    if (count == 2)
        TestBuiltProductionDll(arguments[1]);

    std::printf(
        "{\"identity\":\"IsaacCsT0-ManagerIdentityReaderHarness-v1\","
        "\"assertions\":%u,\"status\":\"%s\"}\n",
        g_assertions, g_failures == 0u ? "PASS" : "FAIL");
    return g_failures == 0u ? 0 : 1;
}
