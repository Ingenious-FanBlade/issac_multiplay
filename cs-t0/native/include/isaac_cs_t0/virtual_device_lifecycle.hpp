#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace isaac_cs_t0::virtual_device {

enum class LifecycleState : std::uint8_t {
    Empty,
    ObjectAllocated,
    ControlReady,
    Registered,
    Retiring,
    Unregistered,
    Destroyed,
};

enum class LifecycleError : std::uint8_t {
    None,
    WrongState,
    WrongThread,
    ObjectAllocationFailure,
    ObjectConstructionFailure,
    ControlBlockAllocationNull,
    ControlBlockConstructionFailure,
    VectorInsertFailure,
    VectorGrowOrCopyFailure,
    InsertNotificationFailure,
    CallbackReentryOrEscape,
    PublicationFailure,
    RemoveCallbackFailure,
    ReaderDrainFailure,
    StopOrShutdownEarlyReturn,
    FinalReleaseFailureOrWrongThread,
    ReferenceLimitExceeded,
    ReferenceUnderflow,
    DuplicateRelease,
    AdmissionClosed,
    VectorMembershipMismatch,
    PortContractViolation,
};

enum class ThreadClass : std::uint8_t {
    AuthorityMainLifecycle,
    ManagerCallback,
    Reader,
};

struct ThreadToken final {
    ThreadClass threadClass{ThreadClass::AuthorityMainLifecycle};
    std::uint32_t identity{0};
};

enum class StrongRefClass : std::uint8_t {
    FactoryLocalStrong,
    ManagerVectorStrong,
    InsertCallbackTransientStrong,
    RemoveCallbackTransientStrong,
    ReaderTransientStrong,
    RollbackTransientStrong,
};

inline constexpr std::size_t kStrongRefClassCount = 6;
inline constexpr std::uint16_t kMaximumSimultaneousStrong = 6;
inline constexpr std::uint16_t kMaximumExternalWeak = 0;

enum class CallbackKind : std::uint8_t {
    Insert,
    Remove,
};

struct StrongReleaseResult final {
    LifecycleError error{LifecycleError::None};
    bool objectDeleted{false};
    bool implicitWeakReleased{false};
    bool controlBlockFreed{false};
};

struct LifecycleSnapshot final {
    LifecycleState state{LifecycleState::Empty};
    LifecycleError firstError{LifecycleError::None};
    bool admissionOpen{false};
    bool publicationConfirmed{false};
    bool drainConfirmed{false};
    bool vectorPresent{false};
    bool objectPresent{false};
    bool controlPresent{false};
    bool finalReleaseStarted{false};
    std::uint16_t liveReaders{0};
    std::uint16_t liveCallbacks{0};
    std::array<std::uint16_t, kStrongRefClassCount> strongByClass{};
    std::uint16_t totalStrong{0};
    std::uint16_t implicitWeak{0};
    std::uint16_t externalWeak{0};
    std::uint16_t objectDeleteCount{0};
    std::uint16_t controlFreeCount{0};
    std::uint16_t rawObjectFreeCount{0};
    std::uint16_t rawControlFreeCount{0};
    std::array<LifecycleState, 7> transitionTrace{};
    std::uint8_t transitionCount{0};
};

class VirtualDeviceLifecycleCoordinator;

class EngineLifecyclePort {
public:
    EngineLifecyclePort() = default;
    virtual ~EngineLifecyclePort() = default;
    EngineLifecyclePort(const EngineLifecyclePort&) = delete;
    EngineLifecyclePort& operator=(const EngineLifecyclePort&) = delete;

    virtual bool IsAuthorityThread(ThreadToken token) noexcept = 0;

    virtual void* AllocateObject() noexcept = 0;
    virtual LifecycleError ConstructObject(void* object) noexcept = 0;
    virtual void DestroyRawObject(void* object, bool constructed) noexcept = 0;

    virtual void* AllocateControlBlock() noexcept = 0;
    virtual LifecycleError ConstructControlBlock(
        void* control,
        void* object) noexcept = 0;
    virtual void DestroyRawControlBlock(
        void* control,
        bool constructed) noexcept = 0;

    virtual LifecycleError AddStrong(
        void* control,
        StrongRefClass referenceClass,
        ThreadToken token) noexcept = 0;
    virtual StrongReleaseResult ReleaseStrong(
        void* control,
        StrongRefClass referenceClass,
        bool expectFinal,
        ThreadToken token) noexcept = 0;

    virtual LifecycleError ManagerInsert(
        VirtualDeviceLifecycleCoordinator& coordinator,
        void* object,
        void* control,
        ThreadToken token) noexcept = 0;
    virtual LifecycleError ManagerRemove(
        VirtualDeviceLifecycleCoordinator& coordinator,
        void* object,
        void* control,
        ThreadToken token) noexcept = 0;
    virtual bool ManagerContains(void* object, void* control) noexcept = 0;

    virtual LifecycleError ConfirmPublication(
        void* object,
        void* control,
        ThreadToken token) noexcept = 0;
    virtual LifecycleError CloseAdmissionAndPublishNeutral(
        void* object,
        ThreadToken token) noexcept = 0;
    virtual LifecycleError CompleteDrain(ThreadToken token) noexcept = 0;
};

class ReaderLease final {
public:
    ReaderLease(
        VirtualDeviceLifecycleCoordinator& coordinator,
        EngineLifecyclePort& port,
        ThreadToken token) noexcept;
    ~ReaderLease() noexcept;

    ReaderLease(const ReaderLease&) = delete;
    ReaderLease& operator=(const ReaderLease&) = delete;
    ReaderLease(ReaderLease&& other) noexcept;
    ReaderLease& operator=(ReaderLease&& other) noexcept;

    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] LifecycleError error() const noexcept;

private:
    void Reset() noexcept;

    VirtualDeviceLifecycleCoordinator* coordinator_{nullptr};
    EngineLifecyclePort* port_{nullptr};
    ThreadToken token_{};
    LifecycleError error_{LifecycleError::AdmissionClosed};
    bool valid_{false};
};

class CallbackLease final {
public:
    CallbackLease(
        VirtualDeviceLifecycleCoordinator& coordinator,
        EngineLifecyclePort& port,
        ThreadToken token,
        CallbackKind kind) noexcept;
    ~CallbackLease() noexcept;

    CallbackLease(const CallbackLease&) = delete;
    CallbackLease& operator=(const CallbackLease&) = delete;
    CallbackLease(CallbackLease&& other) noexcept;
    CallbackLease& operator=(CallbackLease&& other) noexcept;

    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] LifecycleError error() const noexcept;
    [[nodiscard]] CallbackKind kind() const noexcept;

private:
    void Reset() noexcept;

    VirtualDeviceLifecycleCoordinator* coordinator_{nullptr};
    EngineLifecyclePort* port_{nullptr};
    ThreadToken token_{};
    CallbackKind kind_{CallbackKind::Insert};
    LifecycleError error_{LifecycleError::AdmissionClosed};
    bool valid_{false};
};

class VirtualDeviceLifecycleCoordinator final {
public:
    VirtualDeviceLifecycleCoordinator() noexcept;
    ~VirtualDeviceLifecycleCoordinator() = default;

    VirtualDeviceLifecycleCoordinator(
        const VirtualDeviceLifecycleCoordinator&) = delete;
    VirtualDeviceLifecycleCoordinator& operator=(
        const VirtualDeviceLifecycleCoordinator&) = delete;
    VirtualDeviceLifecycleCoordinator(
        VirtualDeviceLifecycleCoordinator&&) = delete;
    VirtualDeviceLifecycleCoordinator& operator=(
        VirtualDeviceLifecycleCoordinator&&) = delete;

    LifecycleError CreateAndRegister(
        EngineLifecyclePort& port,
        ThreadToken token) noexcept;
    LifecycleError BeginRetire(
        EngineLifecyclePort& port,
        ThreadToken token) noexcept;
    LifecycleError RemoveAndDestroy(
        EngineLifecyclePort& port,
        ThreadToken token) noexcept;
    LifecycleSnapshot Snapshot() noexcept;

private:
    friend class ReaderLease;
    friend class CallbackLease;

    LifecycleError AcquireReader(
        EngineLifecyclePort& port,
        ThreadToken token) noexcept;
    LifecycleError AcquireCallback(
        EngineLifecyclePort& port,
        ThreadToken token,
        CallbackKind kind) noexcept;
    void ReleaseReader(
        EngineLifecyclePort& port,
        ThreadToken token) noexcept;
    void ReleaseCallback(
        EngineLifecyclePort& port,
        ThreadToken token,
        CallbackKind kind) noexcept;

    LifecycleError AcquireClass(
        EngineLifecyclePort& port,
        StrongRefClass referenceClass,
        ThreadToken token) noexcept;
    LifecycleError ReleaseClass(
        EngineLifecyclePort& port,
        StrongRefClass referenceClass,
        bool expectFinal,
        ThreadToken token) noexcept;
    LifecycleError RollBackRegistration(
        EngineLifecyclePort& port,
        ThreadToken token,
        LifecycleError cause) noexcept;
    LifecycleError FinalRelease(
        EngineLifecyclePort& port,
        ThreadToken token) noexcept;
    void CleanupRawObject(
        EngineLifecyclePort& port,
        bool constructed) noexcept;
    void CleanupRawControl(
        EngineLifecyclePort& port,
        bool constructed) noexcept;
    void SetState(LifecycleState state) noexcept;
    void Remember(LifecycleError error) noexcept;
    [[nodiscard]] std::uint16_t TotalStrong() const noexcept;
    [[nodiscard]] static std::size_t RefIndex(
        StrongRefClass referenceClass) noexcept;

    LifecycleState state_{LifecycleState::Empty};
    LifecycleError firstError_{LifecycleError::None};
    void* object_{nullptr};
    void* control_{nullptr};
    bool admissionOpen_{false};
    bool publicationConfirmed_{false};
    bool drainConfirmed_{false};
    bool vectorPresent_{false};
    bool finalReleaseStarted_{false};
    bool callbackWindowOpen_{false};
    CallbackKind callbackWindowKind_{CallbackKind::Insert};
    std::uint16_t liveReaders_{0};
    std::uint16_t liveCallbacks_{0};
    std::array<std::uint16_t, kStrongRefClassCount> strongByClass_{};
    std::uint16_t implicitWeak_{0};
    std::uint16_t externalWeak_{0};
    std::uint16_t objectDeleteCount_{0};
    std::uint16_t controlFreeCount_{0};
    std::uint16_t rawObjectFreeCount_{0};
    std::uint16_t rawControlFreeCount_{0};
    std::array<LifecycleState, 7> transitionTrace_{};
    std::uint8_t transitionCount_{0};
};

}  // namespace isaac_cs_t0::virtual_device
