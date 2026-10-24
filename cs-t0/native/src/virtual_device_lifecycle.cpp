#include "isaac_cs_t0/virtual_device_lifecycle.hpp"

#include <utility>

namespace isaac_cs_t0::virtual_device {

namespace {

constexpr ThreadClass ExpectedThread(StrongRefClass referenceClass) noexcept {
    switch (referenceClass) {
        case StrongRefClass::InsertCallbackTransientStrong:
        case StrongRefClass::RemoveCallbackTransientStrong:
            return ThreadClass::ManagerCallback;
        case StrongRefClass::ReaderTransientStrong:
            return ThreadClass::Reader;
        case StrongRefClass::FactoryLocalStrong:
        case StrongRefClass::ManagerVectorStrong:
        case StrongRefClass::RollbackTransientStrong:
            return ThreadClass::AuthorityMainLifecycle;
    }
    return ThreadClass::AuthorityMainLifecycle;
}

}  // namespace

VirtualDeviceLifecycleCoordinator::VirtualDeviceLifecycleCoordinator() noexcept {
    transitionTrace_[0] = LifecycleState::Empty;
    transitionCount_ = 1;
}

std::size_t VirtualDeviceLifecycleCoordinator::RefIndex(
    StrongRefClass referenceClass) noexcept {
    switch (referenceClass) {
        case StrongRefClass::FactoryLocalStrong:
            return 0;
        case StrongRefClass::ManagerVectorStrong:
            return 1;
        case StrongRefClass::InsertCallbackTransientStrong:
            return 2;
        case StrongRefClass::RemoveCallbackTransientStrong:
            return 3;
        case StrongRefClass::ReaderTransientStrong:
            return 4;
        case StrongRefClass::RollbackTransientStrong:
            return 5;
    }
    return 0;
}

std::uint16_t VirtualDeviceLifecycleCoordinator::TotalStrong() const noexcept {
    std::uint16_t total = 0;
    for (const auto count : strongByClass_) {
        total = static_cast<std::uint16_t>(total + count);
    }
    return total;
}

void VirtualDeviceLifecycleCoordinator::Remember(
    LifecycleError error) noexcept {
    if (error != LifecycleError::None && firstError_ == LifecycleError::None) {
        firstError_ = error;
    }
}

void VirtualDeviceLifecycleCoordinator::SetState(
    LifecycleState state) noexcept {
    state_ = state;
    if (static_cast<std::size_t>(transitionCount_) < transitionTrace_.size()) {
        transitionTrace_[transitionCount_] = state;
        ++transitionCount_;
    }
}

void VirtualDeviceLifecycleCoordinator::CleanupRawObject(
    EngineLifecyclePort& port,
    bool constructed) noexcept {
    if (object_ == nullptr) {
        return;
    }
    port.DestroyRawObject(object_, constructed);
    object_ = nullptr;
    ++rawObjectFreeCount_;
}

void VirtualDeviceLifecycleCoordinator::CleanupRawControl(
    EngineLifecyclePort& port,
    bool constructed) noexcept {
    if (control_ == nullptr) {
        return;
    }
    port.DestroyRawControlBlock(control_, constructed);
    control_ = nullptr;
    ++rawControlFreeCount_;
}

LifecycleError VirtualDeviceLifecycleCoordinator::AcquireClass(
    EngineLifecyclePort& port,
    StrongRefClass referenceClass,
    ThreadToken token) noexcept {
    if (control_ == nullptr || token.threadClass != ExpectedThread(referenceClass)) {
        const auto error = control_ == nullptr
            ? LifecycleError::PortContractViolation
            : LifecycleError::WrongThread;
        Remember(error);
        return error;
    }

    const auto index = RefIndex(referenceClass);
    if (strongByClass_[index] != 0 || TotalStrong() >= kMaximumSimultaneousStrong) {
        Remember(LifecycleError::ReferenceLimitExceeded);
        return LifecycleError::ReferenceLimitExceeded;
    }

    const auto error = port.AddStrong(control_, referenceClass, token);
    if (error != LifecycleError::None) {
        Remember(error);
        return error;
    }
    strongByClass_[index] = 1;
    return LifecycleError::None;
}

LifecycleError VirtualDeviceLifecycleCoordinator::ReleaseClass(
    EngineLifecyclePort& port,
    StrongRefClass referenceClass,
    bool expectFinal,
    ThreadToken token) noexcept {
    const auto index = RefIndex(referenceClass);
    if (strongByClass_[index] == 0) {
        Remember(LifecycleError::ReferenceUnderflow);
        return LifecycleError::ReferenceUnderflow;
    }
    if (token.threadClass != ExpectedThread(referenceClass)) {
        Remember(LifecycleError::WrongThread);
        return LifecycleError::WrongThread;
    }

    const auto result = port.ReleaseStrong(
        control_, referenceClass, expectFinal, token);
    if (result.error != LifecycleError::None) {
        Remember(result.error);
        return result.error;
    }

    strongByClass_[index] = 0;
    if (expectFinal) {
        if (TotalStrong() != 0 || !result.objectDeleted ||
            !result.implicitWeakReleased || !result.controlBlockFreed) {
            Remember(LifecycleError::PortContractViolation);
            return LifecycleError::PortContractViolation;
        }
        object_ = nullptr;
        control_ = nullptr;
        implicitWeak_ = 0;
        ++objectDeleteCount_;
        ++controlFreeCount_;
    } else if (result.objectDeleted || result.implicitWeakReleased ||
               result.controlBlockFreed) {
        Remember(LifecycleError::PortContractViolation);
        return LifecycleError::PortContractViolation;
    }
    return LifecycleError::None;
}

LifecycleError VirtualDeviceLifecycleCoordinator::FinalRelease(
    EngineLifecyclePort& port,
    ThreadToken token) noexcept {
    finalReleaseStarted_ = true;
    const auto error = ReleaseClass(
        port, StrongRefClass::FactoryLocalStrong, true, token);
    if (error != LifecycleError::None) {
        finalReleaseStarted_ = false;
        return error;
    }
    finalReleaseStarted_ = false;
    SetState(LifecycleState::Destroyed);
    return LifecycleError::None;
}

LifecycleError VirtualDeviceLifecycleCoordinator::RollBackRegistration(
    EngineLifecyclePort& port,
    ThreadToken token,
    LifecycleError cause) noexcept {
    Remember(cause);
    admissionOpen_ = false;
    publicationConfirmed_ = false;

    const auto rollbackAcquire = AcquireClass(
        port, StrongRefClass::RollbackTransientStrong, token);
    if (rollbackAcquire != LifecycleError::None) {
        return cause;
    }

    if (vectorPresent_) {
        callbackWindowKind_ = CallbackKind::Remove;
        callbackWindowOpen_ = true;
        const auto removeError = port.ManagerRemove(
            *this, object_, control_, token);
        callbackWindowOpen_ = false;
        vectorPresent_ = port.ManagerContains(object_, control_);
        if (removeError != LifecycleError::None || vectorPresent_ ||
            liveCallbacks_ != 0) {
            SetState(LifecycleState::Retiring);
            (void)ReleaseClass(
                port, StrongRefClass::RollbackTransientStrong, false, token);
            return cause;
        }
    }

    if (strongByClass_[RefIndex(StrongRefClass::ManagerVectorStrong)] != 0) {
        const auto releaseVector = ReleaseClass(
            port, StrongRefClass::ManagerVectorStrong, false, token);
        if (releaseVector != LifecycleError::None) {
            (void)ReleaseClass(
                port, StrongRefClass::RollbackTransientStrong, false, token);
            return cause;
        }
    }

    const auto releaseRollback = ReleaseClass(
        port, StrongRefClass::RollbackTransientStrong, false, token);
    if (releaseRollback != LifecycleError::None) {
        return cause;
    }
    (void)FinalRelease(port, token);
    return cause;
}

LifecycleError VirtualDeviceLifecycleCoordinator::CreateAndRegister(
    EngineLifecyclePort& port,
    ThreadToken token) noexcept {
    if (state_ != LifecycleState::Empty) {
        Remember(LifecycleError::WrongState);
        return LifecycleError::WrongState;
    }
    if (!port.IsAuthorityThread(token)) {
        Remember(LifecycleError::WrongThread);
        return LifecycleError::WrongThread;
    }

    object_ = port.AllocateObject();
    if (object_ == nullptr) {
        Remember(LifecycleError::ObjectAllocationFailure);
        return LifecycleError::ObjectAllocationFailure;
    }
    SetState(LifecycleState::ObjectAllocated);

    const auto objectError = port.ConstructObject(object_);
    if (objectError != LifecycleError::None) {
        CleanupRawObject(port, false);
        SetState(LifecycleState::Empty);
        Remember(LifecycleError::ObjectConstructionFailure);
        return LifecycleError::ObjectConstructionFailure;
    }

    control_ = port.AllocateControlBlock();
    if (control_ == nullptr) {
        CleanupRawObject(port, true);
        SetState(LifecycleState::Empty);
        Remember(LifecycleError::ControlBlockAllocationNull);
        return LifecycleError::ControlBlockAllocationNull;
    }

    const auto controlError = port.ConstructControlBlock(control_, object_);
    if (controlError != LifecycleError::None) {
        CleanupRawControl(port, false);
        CleanupRawObject(port, true);
        SetState(LifecycleState::Empty);
        Remember(LifecycleError::ControlBlockConstructionFailure);
        return LifecycleError::ControlBlockConstructionFailure;
    }

    strongByClass_[RefIndex(StrongRefClass::FactoryLocalStrong)] = 1;
    implicitWeak_ = 1;
    SetState(LifecycleState::ControlReady);

    auto error = AcquireClass(
        port, StrongRefClass::ManagerVectorStrong, token);
    if (error != LifecycleError::None) {
        return RollBackRegistration(port, token, error);
    }

    callbackWindowKind_ = CallbackKind::Insert;
    callbackWindowOpen_ = true;
    error = port.ManagerInsert(*this, object_, control_, token);
    callbackWindowOpen_ = false;
    vectorPresent_ = port.ManagerContains(object_, control_);

    if (error == LifecycleError::CallbackReentryOrEscape ||
        liveCallbacks_ != 0) {
        Remember(LifecycleError::CallbackReentryOrEscape);
        if (vectorPresent_) {
            admissionOpen_ = false;
            SetState(LifecycleState::Retiring);
        } else {
            (void)ReleaseClass(
                port, StrongRefClass::ManagerVectorStrong, false, token);
        }
        return LifecycleError::CallbackReentryOrEscape;
    }
    if (error != LifecycleError::None) {
        return RollBackRegistration(port, token, error);
    }
    if (!vectorPresent_) {
        return RollBackRegistration(
            port, token, LifecycleError::PublicationFailure);
    }

    error = port.ConfirmPublication(object_, control_, token);
    if (error != LifecycleError::None) {
        return RollBackRegistration(
            port, token, LifecycleError::PublicationFailure);
    }

    publicationConfirmed_ = true;
    admissionOpen_ = true;
    drainConfirmed_ = false;
    SetState(LifecycleState::Registered);
    return LifecycleError::None;
}

LifecycleError VirtualDeviceLifecycleCoordinator::BeginRetire(
    EngineLifecyclePort& port,
    ThreadToken token) noexcept {
    if (state_ != LifecycleState::Registered &&
        state_ != LifecycleState::Retiring) {
        Remember(LifecycleError::WrongState);
        return LifecycleError::WrongState;
    }
    if (!port.IsAuthorityThread(token)) {
        Remember(LifecycleError::WrongThread);
        return LifecycleError::WrongThread;
    }

    if (state_ == LifecycleState::Registered) {
        admissionOpen_ = false;
        publicationConfirmed_ = false;
        SetState(LifecycleState::Retiring);
    }

    auto error = port.CloseAdmissionAndPublishNeutral(object_, token);
    if (error != LifecycleError::None) {
        Remember(LifecycleError::StopOrShutdownEarlyReturn);
        return LifecycleError::StopOrShutdownEarlyReturn;
    }
    if (liveReaders_ != 0 || liveCallbacks_ != 0) {
        Remember(LifecycleError::ReaderDrainFailure);
        return LifecycleError::ReaderDrainFailure;
    }

    error = port.CompleteDrain(token);
    if (error != LifecycleError::None) {
        Remember(LifecycleError::ReaderDrainFailure);
        return LifecycleError::ReaderDrainFailure;
    }
    drainConfirmed_ = true;
    return LifecycleError::None;
}

LifecycleError VirtualDeviceLifecycleCoordinator::RemoveAndDestroy(
    EngineLifecyclePort& port,
    ThreadToken token) noexcept {
    if (state_ != LifecycleState::Retiring || !drainConfirmed_ ||
        admissionOpen_ || liveReaders_ != 0 || liveCallbacks_ != 0) {
        const auto error = state_ == LifecycleState::Retiring
            ? LifecycleError::ReaderDrainFailure
            : LifecycleError::WrongState;
        Remember(error);
        return error;
    }
    if (!port.IsAuthorityThread(token)) {
        Remember(LifecycleError::WrongThread);
        return LifecycleError::WrongThread;
    }

    callbackWindowKind_ = CallbackKind::Remove;
    callbackWindowOpen_ = true;
    const auto removeError = port.ManagerRemove(
        *this, object_, control_, token);
    callbackWindowOpen_ = false;
    vectorPresent_ = port.ManagerContains(object_, control_);
    if (removeError == LifecycleError::CallbackReentryOrEscape ||
        liveCallbacks_ != 0) {
        Remember(LifecycleError::CallbackReentryOrEscape);
        return LifecycleError::CallbackReentryOrEscape;
    }
    if (removeError != LifecycleError::None || vectorPresent_) {
        Remember(LifecycleError::RemoveCallbackFailure);
        return LifecycleError::RemoveCallbackFailure;
    }

    const auto vectorRelease = ReleaseClass(
        port, StrongRefClass::ManagerVectorStrong, false, token);
    if (vectorRelease != LifecycleError::None) {
        return vectorRelease;
    }
    SetState(LifecycleState::Unregistered);

    if (!port.IsAuthorityThread(token)) {
        Remember(LifecycleError::FinalReleaseFailureOrWrongThread);
        return LifecycleError::FinalReleaseFailureOrWrongThread;
    }
    const auto finalError = FinalRelease(port, token);
    if (finalError != LifecycleError::None) {
        Remember(LifecycleError::FinalReleaseFailureOrWrongThread);
        return LifecycleError::FinalReleaseFailureOrWrongThread;
    }
    return LifecycleError::None;
}

LifecycleSnapshot VirtualDeviceLifecycleCoordinator::Snapshot() noexcept {
    LifecycleSnapshot snapshot{};
    snapshot.state = state_;
    snapshot.firstError = firstError_;
    snapshot.admissionOpen = admissionOpen_;
    snapshot.publicationConfirmed = publicationConfirmed_;
    snapshot.drainConfirmed = drainConfirmed_;
    snapshot.vectorPresent = vectorPresent_;
    snapshot.objectPresent = object_ != nullptr;
    snapshot.controlPresent = control_ != nullptr;
    snapshot.finalReleaseStarted = finalReleaseStarted_;
    snapshot.liveReaders = liveReaders_;
    snapshot.liveCallbacks = liveCallbacks_;
    snapshot.strongByClass = strongByClass_;
    snapshot.totalStrong = TotalStrong();
    snapshot.implicitWeak = implicitWeak_;
    snapshot.externalWeak = externalWeak_;
    snapshot.objectDeleteCount = objectDeleteCount_;
    snapshot.controlFreeCount = controlFreeCount_;
    snapshot.rawObjectFreeCount = rawObjectFreeCount_;
    snapshot.rawControlFreeCount = rawControlFreeCount_;
    snapshot.transitionTrace = transitionTrace_;
    snapshot.transitionCount = transitionCount_;
    return snapshot;
}

LifecycleError VirtualDeviceLifecycleCoordinator::AcquireReader(
    EngineLifecyclePort& port,
    ThreadToken token) noexcept {
    if (state_ != LifecycleState::Registered || !admissionOpen_) {
        Remember(LifecycleError::AdmissionClosed);
        return LifecycleError::AdmissionClosed;
    }
    const auto error = AcquireClass(
        port, StrongRefClass::ReaderTransientStrong, token);
    if (error == LifecycleError::None) {
        ++liveReaders_;
    }
    return error;
}

LifecycleError VirtualDeviceLifecycleCoordinator::AcquireCallback(
    EngineLifecyclePort& port,
    ThreadToken token,
    CallbackKind kind) noexcept {
    if (!callbackWindowOpen_ || kind != callbackWindowKind_) {
        Remember(LifecycleError::AdmissionClosed);
        return LifecycleError::AdmissionClosed;
    }
    const auto referenceClass = kind == CallbackKind::Insert
        ? StrongRefClass::InsertCallbackTransientStrong
        : StrongRefClass::RemoveCallbackTransientStrong;
    const auto error = AcquireClass(port, referenceClass, token);
    if (error == LifecycleError::None) {
        ++liveCallbacks_;
    }
    return error;
}

void VirtualDeviceLifecycleCoordinator::ReleaseReader(
    EngineLifecyclePort& port,
    ThreadToken token) noexcept {
    if (liveReaders_ == 0) {
        Remember(LifecycleError::DuplicateRelease);
        return;
    }
    const auto error = ReleaseClass(
        port, StrongRefClass::ReaderTransientStrong, false, token);
    if (error == LifecycleError::None) {
        --liveReaders_;
    }
}

void VirtualDeviceLifecycleCoordinator::ReleaseCallback(
    EngineLifecyclePort& port,
    ThreadToken token,
    CallbackKind kind) noexcept {
    if (liveCallbacks_ == 0) {
        Remember(LifecycleError::DuplicateRelease);
        return;
    }
    const auto referenceClass = kind == CallbackKind::Insert
        ? StrongRefClass::InsertCallbackTransientStrong
        : StrongRefClass::RemoveCallbackTransientStrong;
    const auto error = ReleaseClass(port, referenceClass, false, token);
    if (error == LifecycleError::None) {
        --liveCallbacks_;
    }
}

ReaderLease::ReaderLease(
    VirtualDeviceLifecycleCoordinator& coordinator,
    EngineLifecyclePort& port,
    ThreadToken token) noexcept
    : coordinator_(&coordinator), port_(&port), token_(token) {
    error_ = coordinator_->AcquireReader(port, token);
    valid_ = error_ == LifecycleError::None;
}

ReaderLease::~ReaderLease() noexcept {
    Reset();
}

ReaderLease::ReaderLease(ReaderLease&& other) noexcept
    : coordinator_(std::exchange(other.coordinator_, nullptr)),
      port_(std::exchange(other.port_, nullptr)),
      token_(other.token_),
      error_(other.error_),
      valid_(std::exchange(other.valid_, false)) {}

ReaderLease& ReaderLease::operator=(ReaderLease&& other) noexcept {
    if (this != &other) {
        Reset();
        coordinator_ = std::exchange(other.coordinator_, nullptr);
        port_ = std::exchange(other.port_, nullptr);
        token_ = other.token_;
        error_ = other.error_;
        valid_ = std::exchange(other.valid_, false);
    }
    return *this;
}

bool ReaderLease::valid() const noexcept {
    return valid_;
}

LifecycleError ReaderLease::error() const noexcept {
    return error_;
}

void ReaderLease::Reset() noexcept {
    if (valid_ && coordinator_ != nullptr && port_ != nullptr) {
        coordinator_->ReleaseReader(*port_, token_);
    }
    valid_ = false;
    coordinator_ = nullptr;
    port_ = nullptr;
}

CallbackLease::CallbackLease(
    VirtualDeviceLifecycleCoordinator& coordinator,
    EngineLifecyclePort& port,
    ThreadToken token,
    CallbackKind kind) noexcept
    : coordinator_(&coordinator),
      port_(&port),
      token_(token),
      kind_(kind) {
    error_ = coordinator_->AcquireCallback(port, token, kind);
    valid_ = error_ == LifecycleError::None;
}

CallbackLease::~CallbackLease() noexcept {
    Reset();
}

CallbackLease::CallbackLease(CallbackLease&& other) noexcept
    : coordinator_(std::exchange(other.coordinator_, nullptr)),
      port_(std::exchange(other.port_, nullptr)),
      token_(other.token_),
      kind_(other.kind_),
      error_(other.error_),
      valid_(std::exchange(other.valid_, false)) {}

CallbackLease& CallbackLease::operator=(CallbackLease&& other) noexcept {
    if (this != &other) {
        Reset();
        coordinator_ = std::exchange(other.coordinator_, nullptr);
        port_ = std::exchange(other.port_, nullptr);
        token_ = other.token_;
        kind_ = other.kind_;
        error_ = other.error_;
        valid_ = std::exchange(other.valid_, false);
    }
    return *this;
}

bool CallbackLease::valid() const noexcept {
    return valid_;
}

LifecycleError CallbackLease::error() const noexcept {
    return error_;
}

CallbackKind CallbackLease::kind() const noexcept {
    return kind_;
}

void CallbackLease::Reset() noexcept {
    if (valid_ && coordinator_ != nullptr && port_ != nullptr) {
        coordinator_->ReleaseCallback(*port_, token_, kind_);
    }
    valid_ = false;
    coordinator_ = nullptr;
    port_ = nullptr;
}

}  // namespace isaac_cs_t0::virtual_device
