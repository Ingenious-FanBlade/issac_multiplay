#define NOMINMAX
#include <Windows.h>

#include <limits>
#include <new>

#include "isaac_cs_t0/manager_identity_reader.hpp"

namespace isaac_cs_t0::manager_identity {

bool ProcessIdentity::valid() const noexcept
{
    return process_id != 0u && creation_time != 0u;
}

bool ProcessIdentity::operator==(const ProcessIdentity& other) const noexcept
{
    return process_id == other.process_id && creation_time == other.creation_time;
}

bool Reader::Initialize(ProcessIdentity process,
                        std::uint32_t main_thread_id) noexcept
{
    if (value_.state != State::Uninitialized || !process.valid() ||
        main_thread_id == 0u)
    {
        Permanent(Invalidation::WrongOrder);
        return false;
    }

    value_ = {};
    value_.state = State::Ready;
    value_.process = process;
    value_.main_thread_id = main_thread_id;
    value_.generation = 1u;
    return true;
}

bool Reader::ValidateInputManager(ProcessIdentity process,
                                  std::uint32_t thread_id,
                                  SlotSample vptr,
                                  std::uintptr_t expected_vptr) noexcept
{
    if (value_.state != State::Ready || value_.input_manager_validated)
    {
        Permanent(Invalidation::WrongOrder);
        return false;
    }
    if (!AcceptProcessAndThread(process, thread_id))
        return false;
    if (value_.read_operations > kMaximumReadOperations - 2u ||
        value_.read_bytes > kMaximumReadBytes - 8u)
    {
        Permanent(Invalidation::BudgetExceeded);
        return false;
    }
    value_.read_operations += 2u;
    value_.read_bytes += 8u;
    if (!vptr.first_read_ok || !vptr.second_read_ok)
    {
        Permanent(Invalidation::ReadFailure);
        return false;
    }
    if (vptr.first != vptr.second)
    {
        Permanent(Invalidation::DoubleReadDrift);
        return false;
    }
    if (expected_vptr == 0u || vptr.first != expected_vptr)
    {
        Permanent(Invalidation::Lifecycle);
        return false;
    }
    value_.input_manager_vptr = vptr.first;
    value_.input_manager_validated = true;
    return true;
}

bool Reader::ObserveCaller(ProcessIdentity process,
                           std::uint32_t thread_id,
                           SlotSample slot,
                           std::uintptr_t manager_this) noexcept
{
    return Observe(State::Ready, State::CallerSampled, process, thread_id,
                   slot, manager_this);
}

bool Reader::ObserveManagerEntry(ProcessIdentity process,
                                 std::uint32_t thread_id,
                                 SlotSample slot,
                                 std::uintptr_t manager_this) noexcept
{
    return Observe(State::CallerSampled, State::ManagerEntered, process,
                   thread_id, slot, manager_this);
}

bool Reader::ObserveOrdinaryReload(ProcessIdentity process,
                                   std::uint32_t thread_id,
                                   SlotSample slot,
                                   std::uintptr_t spawn_receiver) noexcept
{
    return Observe(State::ManagerEntered, State::OrdinarySampled, process,
                   thread_id, slot, spawn_receiver);
}

bool Reader::ConfirmSpawnEligibility(ProcessIdentity process,
                                     std::uint32_t thread_id,
                                     std::uintptr_t spawn_receiver) noexcept
{
    if (value_.state != State::OrdinarySampled)
    {
        Permanent(Invalidation::WrongOrder);
        return false;
    }
    if (!AcceptProcessAndThread(process, thread_id))
        return false;
    if (spawn_receiver == 0u || spawn_receiver != value_.player_manager ||
        value_.epoch_generation == 0u ||
        value_.epoch_generation != value_.generation)
    {
        Permanent(Invalidation::ReceiverMismatch);
        return false;
    }

    value_.state = State::SpawnValidated;
    return true;
}

bool Reader::ObserveManagerNormalExit(ProcessIdentity process,
                                      std::uint32_t thread_id) noexcept
{
    if (value_.state != State::SpawnValidated)
    {
        Permanent(Invalidation::WrongOrder);
        return false;
    }
    if (!AcceptProcessAndThread(process, thread_id))
        return false;
    value_.state = State::ManagerExited;
    return true;
}

bool Reader::ObserveCallerNormalExit(ProcessIdentity process,
                                     std::uint32_t thread_id) noexcept
{
    if (value_.state != State::ManagerExited)
    {
        Permanent(Invalidation::WrongOrder);
        return false;
    }
    if (!AcceptProcessAndThread(process, thread_id))
        return false;
    value_.state = State::Eligible;
    value_.publication_eligible = true;
    return true;
}

void Reader::Invalidate(Invalidation reason) noexcept
{
    if (reason == Invalidation::None)
        reason = Invalidation::ComputedAliasWriter;
    Permanent(reason);
}

void Reader::MarkUncertainStop() noexcept
{
    if (value_.state == State::UnsafeRetainModule)
        return;
    Permanent(Invalidation::UncertainStop);
    value_.state = State::UnsafeRetainModule;
    value_.unsafe_retain_module = true;
}

void Reader::Retire() noexcept
{
    if (value_.state == State::UnsafeRetainModule ||
        value_.state == State::Retired)
        return;

    if (!value_.permanently_disarmed)
        Permanent(Invalidation::Lifecycle);
    value_.state = State::Retired;
    value_.publication_eligible = false;
}

Snapshot Reader::snapshot() const noexcept
{
    return value_;
}

#if defined(ISAAC_CS_T0_MANAGER_IDENTITY_TESTING)
void Reader::SetCountersForTest(std::uint64_t generation,
                                std::uint32_t read_operations,
                                std::uint32_t read_bytes) noexcept
{
    value_.generation = generation;
    value_.read_operations = read_operations;
    value_.read_bytes = read_bytes;
}
#endif

bool Reader::Observe(State expected,
                     State next,
                     ProcessIdentity process,
                     std::uint32_t thread_id,
                     SlotSample slot,
                     std::uintptr_t receiver) noexcept
{
    if (value_.state != expected)
    {
        Permanent(Invalidation::WrongOrder);
        return false;
    }
    if (expected == State::Ready && !value_.input_manager_validated)
    {
        Permanent(Invalidation::Lifecycle);
        return false;
    }
    if (!AcceptProcessAndThread(process, thread_id) ||
        !AcceptStableSample(slot, receiver))
        return false;

    value_.state = next;
    return true;
}

bool Reader::AcceptProcessAndThread(ProcessIdentity process,
                                    std::uint32_t thread_id) noexcept
{
    if (!process.valid() || !(process == value_.process))
    {
        Permanent(Invalidation::ProcessReused);
        return false;
    }
    if (thread_id == 0u || thread_id != value_.main_thread_id)
    {
        Permanent(Invalidation::WrongThread);
        return false;
    }
    return true;
}

bool Reader::AcceptStableSample(SlotSample slot,
                                std::uintptr_t receiver) noexcept
{
    if (value_.read_operations > kMaximumReadOperations - 2u ||
        value_.read_bytes > kMaximumReadBytes - 8u ||
        value_.stable_samples >= kMaximumStableSamples)
    {
        Permanent(Invalidation::BudgetExceeded);
        return false;
    }
    value_.read_operations += 2u;
    value_.read_bytes += 8u;

    if (!slot.first_read_ok || !slot.second_read_ok)
    {
        Permanent(Invalidation::ReadFailure);
        return false;
    }
    if (slot.first != slot.second)
    {
        Permanent(Invalidation::DoubleReadDrift);
        return false;
    }
    if (slot.first == 0u ||
        slot.first > std::numeric_limits<std::uintptr_t>::max() -
                         kPlayerManagerOffset)
    {
        Permanent(Invalidation::ReceiverMismatch);
        return false;
    }

    const auto derived = slot.first + kPlayerManagerOffset;
    if (receiver == 0u || receiver != derived)
    {
        Permanent(Invalidation::ReceiverMismatch);
        return false;
    }

    if (value_.stable_samples == 0u)
    {
        value_.game_root = slot.first;
        value_.player_manager = derived;
        value_.epoch_generation = value_.generation;
    }
    else if (slot.first != value_.game_root ||
             derived != value_.player_manager ||
             value_.epoch_generation != value_.generation)
    {
        Permanent(Invalidation::OwnerReplaced);
        return false;
    }

    ++value_.stable_samples;
    return true;
}

void Reader::Permanent(Invalidation reason) noexcept
{
    if (value_.permanently_disarmed || value_.state == State::Retired)
        return;

    value_.publication_eligible = false;
    value_.permanently_disarmed = true;
    if (value_.generation == std::numeric_limits<std::uint64_t>::max())
        value_.invalidation = Invalidation::GenerationOverflow;
    else
    {
        ++value_.generation;
        value_.invalidation = reason;
    }
    value_.state = State::PermanentlyDisarmed;
}

} // namespace isaac_cs_t0::manager_identity

namespace {

using isaac_cs_t0::manager_identity::Invalidation;
using isaac_cs_t0::manager_identity::ProcessIdentity;
using isaac_cs_t0::manager_identity::Reader;
using isaac_cs_t0::manager_identity::SlotSample;

struct ReaderHandle {
    SRWLOCK lock = SRWLOCK_INIT;
    Reader reader{};
    std::uintptr_t main_image_base{};
};

ProcessIdentity CurrentProcessIdentity() noexcept
{
    FILETIME creation{};
    FILETIME exit{};
    FILETIME kernel{};
    FILETIME user{};
    if (!GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user))
        return {};
    ULARGE_INTEGER combined{};
    combined.LowPart = creation.dwLowDateTime;
    combined.HighPart = creation.dwHighDateTime;
    return {GetCurrentProcessId(), combined.QuadPart};
}

bool ReadPointer(std::uintptr_t address, std::uintptr_t& output) noexcept
{
    __try
    {
        output = *reinterpret_cast<volatile const std::uintptr_t*>(address);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        output = 0u;
        return false;
    }
}

SlotSample SampleSlot(const ReaderHandle& handle) noexcept
{
    SlotSample result{};
    if (handle.main_image_base == 0u ||
        handle.main_image_base >
            std::numeric_limits<std::uintptr_t>::max() -
                isaac_cs_t0::manager_identity::kGameRootSlotRva)
        return result;

    const auto slot = handle.main_image_base +
                      isaac_cs_t0::manager_identity::kGameRootSlotRva;
    result.first_read_ok = ReadPointer(slot, result.first);
    MemoryBarrier();
    result.second_read_ok = ReadPointer(slot, result.second);
    return result;
}

SlotSample SampleInputManagerVptr(const ReaderHandle& handle) noexcept
{
    SlotSample result{};
    if (handle.main_image_base == 0u ||
        handle.main_image_base >
            std::numeric_limits<std::uintptr_t>::max() -
                isaac_cs_t0::manager_identity::kInputManagerObjectRva)
        return result;

    const auto address = handle.main_image_base +
                         isaac_cs_t0::manager_identity::kInputManagerObjectRva;
    result.first_read_ok = ReadPointer(address, result.first);
    MemoryBarrier();
    result.second_read_ok = ReadPointer(address, result.second);
    return result;
}

Invalidation NormalizeInvalidation(std::uint32_t reason) noexcept
{
    constexpr auto highest =
        static_cast<std::uint32_t>(Invalidation::UncertainStop);
    if (reason == 0u || reason > highest)
        return Invalidation::ComputedAliasWriter;
    return static_cast<Invalidation>(reason);
}

void CopySnapshot(const isaac_cs_t0::manager_identity::Snapshot& source,
                  IsaacCsT0ManagerIdentitySnapshotV1& target) noexcept
{
    target = {};
    target.state = static_cast<std::uint32_t>(source.state);
    target.invalidation = static_cast<std::uint32_t>(source.invalidation);
    target.process_id = source.process.process_id;
    target.main_thread_id = source.main_thread_id;
    target.process_creation_time = source.process.creation_time;
    target.generation = source.generation;
    target.epoch_generation = source.epoch_generation;
    target.game_root = source.game_root;
    target.player_manager = source.player_manager;
    target.input_manager_vptr = source.input_manager_vptr;
    target.stable_samples = source.stable_samples;
    target.read_operations = source.read_operations;
    target.read_bytes = source.read_bytes;
    target.publication_eligible = source.publication_eligible ? 1u : 0u;
    target.input_manager_validated = source.input_manager_validated ? 1u : 0u;
    target.permanently_disarmed = source.permanently_disarmed ? 1u : 0u;
    target.unsafe_retain_module = source.unsafe_retain_module ? 1u : 0u;
}

template <typename Callback>
std::uint32_t WithObservation(void* opaque, Callback&& callback) noexcept
{
    if (!opaque)
        return 0u;
    auto& handle = *static_cast<ReaderHandle*>(opaque);
    AcquireSRWLockExclusive(&handle.lock);
    const auto process = CurrentProcessIdentity();
    const auto thread = GetCurrentThreadId();
    const auto slot = SampleSlot(handle);
    const bool accepted = callback(handle.reader, process, thread, slot);
    ReleaseSRWLockExclusive(&handle.lock);
    return accepted ? 1u : 0u;
}

} // namespace

extern "C" {

void* __stdcall IsaacCsT0ManagerIdentityCreate(std::uintptr_t main_image_base)
{
    constexpr auto largest_rva =
        isaac_cs_t0::manager_identity::kGameRootSlotRva;
    if (main_image_base == 0u ||
        main_image_base >
            std::numeric_limits<std::uintptr_t>::max() - largest_rva)
        return nullptr;
    auto* const handle = new (std::nothrow) ReaderHandle{};
    if (!handle)
        return nullptr;
    handle->main_image_base = main_image_base;
    if (!handle->reader.Initialize(CurrentProcessIdentity(), GetCurrentThreadId()))
    {
        delete handle;
        return nullptr;
    }
    return handle;
}

void __stdcall IsaacCsT0ManagerIdentityDestroy(void* opaque)
{
    if (!opaque)
        return;
    auto* const handle = static_cast<ReaderHandle*>(opaque);
    AcquireSRWLockExclusive(&handle->lock);
    const bool unsafe = handle->reader.snapshot().unsafe_retain_module;
    ReleaseSRWLockExclusive(&handle->lock);
    if (!unsafe)
        delete handle;
}

std::uint32_t __stdcall IsaacCsT0ManagerIdentityObserveCaller(
    void* opaque, std::uintptr_t manager_this)
{
    if (!opaque)
        return 0u;
    auto& handle = *static_cast<ReaderHandle*>(opaque);
    AcquireSRWLockExclusive(&handle.lock);
    const auto process = CurrentProcessIdentity();
    const auto thread = GetCurrentThreadId();
    const auto expected_vptr =
        handle.main_image_base +
        isaac_cs_t0::manager_identity::kInputManagerDerivedVtableRva;
    const bool input_valid = handle.reader.ValidateInputManager(
        process, thread, SampleInputManagerVptr(handle), expected_vptr);
    const bool accepted = input_valid && handle.reader.ObserveCaller(
        process, thread, SampleSlot(handle), manager_this);
    ReleaseSRWLockExclusive(&handle.lock);
    return accepted ? 1u : 0u;
}

std::uint32_t __stdcall IsaacCsT0ManagerIdentityObserveManagerEntry(
    void* opaque, std::uintptr_t manager_this)
{
    return WithObservation(
        opaque, [manager_this](Reader& reader, ProcessIdentity process,
                               std::uint32_t thread, SlotSample slot) noexcept {
            return reader.ObserveManagerEntry(process, thread, slot,
                                              manager_this);
        });
}

std::uint32_t __stdcall IsaacCsT0ManagerIdentityObserveOrdinaryReload(
    void* opaque, std::uintptr_t spawn_receiver)
{
    return WithObservation(
        opaque, [spawn_receiver](Reader& reader, ProcessIdentity process,
                                 std::uint32_t thread, SlotSample slot) noexcept {
            return reader.ObserveOrdinaryReload(process, thread, slot,
                                                spawn_receiver);
        });
}

std::uint32_t __stdcall IsaacCsT0ManagerIdentityConfirmSpawn(
    void* opaque, std::uintptr_t spawn_receiver)
{
    if (!opaque)
        return 0u;
    auto& handle = *static_cast<ReaderHandle*>(opaque);
    AcquireSRWLockExclusive(&handle.lock);
    const bool accepted = handle.reader.ConfirmSpawnEligibility(
        CurrentProcessIdentity(), GetCurrentThreadId(), spawn_receiver);
    ReleaseSRWLockExclusive(&handle.lock);
    return accepted ? 1u : 0u;
}

std::uint32_t __stdcall IsaacCsT0ManagerIdentityObserveManagerNormalExit(
    void* opaque)
{
    if (!opaque)
        return 0u;
    auto& handle = *static_cast<ReaderHandle*>(opaque);
    AcquireSRWLockExclusive(&handle.lock);
    const bool accepted = handle.reader.ObserveManagerNormalExit(
        CurrentProcessIdentity(), GetCurrentThreadId());
    ReleaseSRWLockExclusive(&handle.lock);
    return accepted ? 1u : 0u;
}

std::uint32_t __stdcall IsaacCsT0ManagerIdentityObserveCallerNormalExit(
    void* opaque)
{
    if (!opaque)
        return 0u;
    auto& handle = *static_cast<ReaderHandle*>(opaque);
    AcquireSRWLockExclusive(&handle.lock);
    const bool accepted = handle.reader.ObserveCallerNormalExit(
        CurrentProcessIdentity(), GetCurrentThreadId());
    ReleaseSRWLockExclusive(&handle.lock);
    return accepted ? 1u : 0u;
}

void __stdcall IsaacCsT0ManagerIdentityInvalidate(void* opaque,
                                                   std::uint32_t reason)
{
    if (!opaque)
        return;
    auto& handle = *static_cast<ReaderHandle*>(opaque);
    AcquireSRWLockExclusive(&handle.lock);
    handle.reader.Invalidate(NormalizeInvalidation(reason));
    ReleaseSRWLockExclusive(&handle.lock);
}

void __stdcall IsaacCsT0ManagerIdentityUncertainStop(void* opaque)
{
    if (!opaque)
        return;
    auto& handle = *static_cast<ReaderHandle*>(opaque);
    AcquireSRWLockExclusive(&handle.lock);
    handle.reader.MarkUncertainStop();
    ReleaseSRWLockExclusive(&handle.lock);
}

void __stdcall IsaacCsT0ManagerIdentityRetire(void* opaque)
{
    if (!opaque)
        return;
    auto& handle = *static_cast<ReaderHandle*>(opaque);
    AcquireSRWLockExclusive(&handle.lock);
    handle.reader.Retire();
    ReleaseSRWLockExclusive(&handle.lock);
}

std::uint32_t __stdcall IsaacCsT0ManagerIdentityReadSnapshot(
    const void* opaque, IsaacCsT0ManagerIdentitySnapshotV1* output)
{
    if (!opaque || !output)
        return 0u;
    auto& handle = *const_cast<ReaderHandle*>(
        static_cast<const ReaderHandle*>(opaque));
    AcquireSRWLockShared(&handle.lock);
    CopySnapshot(handle.reader.snapshot(), *output);
    ReleaseSRWLockShared(&handle.lock);
    return 1u;
}

} // extern "C"
