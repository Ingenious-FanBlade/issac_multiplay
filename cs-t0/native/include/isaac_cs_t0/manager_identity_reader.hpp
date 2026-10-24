#pragma once

#include <cstddef>
#include <cstdint>

namespace isaac_cs_t0::manager_identity {

inline constexpr std::uintptr_t kGameRootSlotRva = 0x00871678u;
inline constexpr std::uintptr_t kPlayerManagerOffset = 0x0001BAA8u;
inline constexpr std::uintptr_t kInputManagerObjectRva = 0x00857B18u;
inline constexpr std::uintptr_t kInputManagerDerivedVtableRva = 0x00782950u;
inline constexpr std::uint32_t kMaximumStableSamples = 4u;
inline constexpr std::uint32_t kMaximumReadOperations = 8u;
inline constexpr std::uint32_t kMaximumReadBytes = 32u;

struct ProcessIdentity {
    std::uint32_t process_id{};
    std::uint64_t creation_time{};

    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] bool operator==(const ProcessIdentity& other) const noexcept;
};

struct SlotSample {
    bool first_read_ok{};
    bool second_read_ok{};
    std::uintptr_t first{};
    std::uintptr_t second{};
};

enum class State : std::uint32_t {
    Uninitialized = 0,
    Ready = 1,
    CallerSampled = 2,
    ManagerEntered = 3,
    OrdinarySampled = 4,
    SpawnValidated = 5,
    ManagerExited = 6,
    Eligible = 7,
    PermanentlyDisarmed = 8,
    UnsafeRetainModule = 9,
    Retired = 10,
};

enum class Invalidation : std::uint32_t {
    None = 0,
    ReadFailure = 1,
    DoubleReadDrift = 2,
    ProcessReused = 3,
    WrongThread = 4,
    WrongOrder = 5,
    ReceiverMismatch = 6,
    OwnerReplaced = 7,
    SlotWriter = 8,
    ComputedAliasWriter = 9,
    Lifecycle = 10,
    CallbackReentry = 11,
    ExceptionResume = 12,
    GenerationOverflow = 13,
    BudgetExceeded = 14,
    UncertainStop = 15,
};

struct Snapshot {
    State state{State::Uninitialized};
    Invalidation invalidation{Invalidation::None};
    ProcessIdentity process{};
    std::uint32_t main_thread_id{};
    std::uint64_t generation{};
    std::uint64_t epoch_generation{};
    std::uintptr_t game_root{};
    std::uintptr_t player_manager{};
    std::uintptr_t input_manager_vptr{};
    std::uint32_t stable_samples{};
    std::uint32_t read_operations{};
    std::uint32_t read_bytes{};
    bool publication_eligible{};
    bool input_manager_validated{};
    bool permanently_disarmed{};
    bool unsafe_retain_module{};
};

class Reader final {
public:
    Reader() noexcept = default;

    [[nodiscard]] bool Initialize(ProcessIdentity process,
                                  std::uint32_t main_thread_id) noexcept;
    [[nodiscard]] bool ValidateInputManager(ProcessIdentity process,
                                            std::uint32_t thread_id,
                                            SlotSample vptr,
                                            std::uintptr_t expected_vptr) noexcept;
    [[nodiscard]] bool ObserveCaller(ProcessIdentity process,
                                     std::uint32_t thread_id,
                                     SlotSample slot,
                                     std::uintptr_t manager_this) noexcept;
    [[nodiscard]] bool ObserveManagerEntry(ProcessIdentity process,
                                           std::uint32_t thread_id,
                                           SlotSample slot,
                                           std::uintptr_t manager_this) noexcept;
    [[nodiscard]] bool ObserveOrdinaryReload(ProcessIdentity process,
                                             std::uint32_t thread_id,
                                             SlotSample slot,
                                             std::uintptr_t spawn_receiver) noexcept;
    [[nodiscard]] bool ConfirmSpawnEligibility(ProcessIdentity process,
                                               std::uint32_t thread_id,
                                               std::uintptr_t spawn_receiver) noexcept;
    [[nodiscard]] bool ObserveManagerNormalExit(ProcessIdentity process,
                                                std::uint32_t thread_id) noexcept;
    [[nodiscard]] bool ObserveCallerNormalExit(ProcessIdentity process,
                                               std::uint32_t thread_id) noexcept;
    void Invalidate(Invalidation reason) noexcept;
    void MarkUncertainStop() noexcept;
    void Retire() noexcept;
    [[nodiscard]] Snapshot snapshot() const noexcept;
#if defined(ISAAC_CS_T0_MANAGER_IDENTITY_TESTING)
    void SetCountersForTest(std::uint64_t generation,
                            std::uint32_t read_operations,
                            std::uint32_t read_bytes) noexcept;
#endif

private:
    [[nodiscard]] bool Observe(State expected,
                               State next,
                               ProcessIdentity process,
                               std::uint32_t thread_id,
                               SlotSample slot,
                               std::uintptr_t receiver) noexcept;
    [[nodiscard]] bool AcceptProcessAndThread(ProcessIdentity process,
                                              std::uint32_t thread_id) noexcept;
    [[nodiscard]] bool AcceptStableSample(SlotSample slot,
                                          std::uintptr_t receiver) noexcept;
    void Permanent(Invalidation reason) noexcept;

    Snapshot value_{};
};

} // namespace isaac_cs_t0::manager_identity

extern "C" {

struct IsaacCsT0ManagerIdentitySnapshotV1 {
    std::uint32_t state;
    std::uint32_t invalidation;
    std::uint32_t process_id;
    std::uint32_t main_thread_id;
    std::uint64_t process_creation_time;
    std::uint64_t generation;
    std::uint64_t epoch_generation;
    std::uintptr_t game_root;
    std::uintptr_t player_manager;
    std::uintptr_t input_manager_vptr;
    std::uint32_t stable_samples;
    std::uint32_t read_operations;
    std::uint32_t read_bytes;
    std::uint32_t publication_eligible;
    std::uint32_t input_manager_validated;
    std::uint32_t permanently_disarmed;
    std::uint32_t unsafe_retain_module;
};

void* __stdcall IsaacCsT0ManagerIdentityCreate(std::uintptr_t main_image_base);
void __stdcall IsaacCsT0ManagerIdentityDestroy(void* handle);
std::uint32_t __stdcall IsaacCsT0ManagerIdentityObserveCaller(
    void* handle, std::uintptr_t manager_this);
std::uint32_t __stdcall IsaacCsT0ManagerIdentityObserveManagerEntry(
    void* handle, std::uintptr_t manager_this);
std::uint32_t __stdcall IsaacCsT0ManagerIdentityObserveOrdinaryReload(
    void* handle, std::uintptr_t spawn_receiver);
std::uint32_t __stdcall IsaacCsT0ManagerIdentityConfirmSpawn(
    void* handle, std::uintptr_t spawn_receiver);
std::uint32_t __stdcall IsaacCsT0ManagerIdentityObserveManagerNormalExit(
    void* handle);
std::uint32_t __stdcall IsaacCsT0ManagerIdentityObserveCallerNormalExit(
    void* handle);
void __stdcall IsaacCsT0ManagerIdentityInvalidate(void* handle,
                                                   std::uint32_t reason);
void __stdcall IsaacCsT0ManagerIdentityUncertainStop(void* handle);
void __stdcall IsaacCsT0ManagerIdentityRetire(void* handle);
std::uint32_t __stdcall IsaacCsT0ManagerIdentityReadSnapshot(
    const void* handle, IsaacCsT0ManagerIdentitySnapshotV1* output);

}
