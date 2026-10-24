#include "isaac_cs_t0/virtual_device_lifecycle.hpp"

#include <array>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <new>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using namespace isaac_cs_t0::virtual_device;

namespace {

static_assert(sizeof(void*) == 4, "offline lifecycle harness must be Win32/x86");
static_assert(kStrongRefClassCount == 6);
static_assert(kMaximumSimultaneousStrong == 6);
static_assert(kMaximumExternalWeak == 0);
static_assert(!std::is_copy_constructible_v<ReaderLease>);
static_assert(!std::is_copy_assignable_v<ReaderLease>);
static_assert(std::is_move_constructible_v<ReaderLease>);
static_assert(std::is_move_assignable_v<ReaderLease>);
static_assert(!std::is_copy_constructible_v<CallbackLease>);
static_assert(!std::is_copy_assignable_v<CallbackLease>);
static_assert(std::is_move_constructible_v<CallbackLease>);
static_assert(std::is_move_assignable_v<CallbackLease>);

using CreateSignature = LifecycleError (VirtualDeviceLifecycleCoordinator::*)(
    EngineLifecyclePort&, ThreadToken) noexcept;
using RetireSignature = CreateSignature;
using RemoveSignature = CreateSignature;
using SnapshotSignature = LifecycleSnapshot (
    VirtualDeviceLifecycleCoordinator::*)() noexcept;
static_assert(std::is_same_v<decltype(static_cast<CreateSignature>(
    &VirtualDeviceLifecycleCoordinator::CreateAndRegister)), CreateSignature>);
static_assert(std::is_same_v<decltype(static_cast<RetireSignature>(
    &VirtualDeviceLifecycleCoordinator::BeginRetire)), RetireSignature>);
static_assert(std::is_same_v<decltype(static_cast<RemoveSignature>(
    &VirtualDeviceLifecycleCoordinator::RemoveAndDestroy)), RemoveSignature>);
static_assert(std::is_same_v<decltype(static_cast<SnapshotSignature>(
    &VirtualDeviceLifecycleCoordinator::Snapshot)), SnapshotSignature>);

constexpr ThreadToken kAuthority{
    ThreadClass::AuthorityMainLifecycle, 0xA11CE001U};
constexpr ThreadToken kCallback{
    ThreadClass::ManagerCallback, 0xCA11BAC1U};
constexpr ThreadToken kReader{ThreadClass::Reader, 0x0EADB001U};

enum class FailurePoint : std::uint8_t {
    None,
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
};

constexpr std::array<FailurePoint, 13> kFailurePoints{
    FailurePoint::ObjectAllocationFailure,
    FailurePoint::ObjectConstructionFailure,
    FailurePoint::ControlBlockAllocationNull,
    FailurePoint::ControlBlockConstructionFailure,
    FailurePoint::VectorInsertFailure,
    FailurePoint::VectorGrowOrCopyFailure,
    FailurePoint::InsertNotificationFailure,
    FailurePoint::CallbackReentryOrEscape,
    FailurePoint::PublicationFailure,
    FailurePoint::RemoveCallbackFailure,
    FailurePoint::ReaderDrainFailure,
    FailurePoint::StopOrShutdownEarlyReturn,
    FailurePoint::FinalReleaseFailureOrWrongThread,
};

const char* FailureName(FailurePoint point) noexcept {
    switch (point) {
        case FailurePoint::None:
            return "NONE";
        case FailurePoint::ObjectAllocationFailure:
            return "OBJECT_ALLOCATION_FAILURE";
        case FailurePoint::ObjectConstructionFailure:
            return "OBJECT_CONSTRUCTION_FAILURE";
        case FailurePoint::ControlBlockAllocationNull:
            return "CONTROL_BLOCK_ALLOCATION_NULL";
        case FailurePoint::ControlBlockConstructionFailure:
            return "CONTROL_BLOCK_CONSTRUCTION_FAILURE";
        case FailurePoint::VectorInsertFailure:
            return "VECTOR_INSERT_FAILURE";
        case FailurePoint::VectorGrowOrCopyFailure:
            return "VECTOR_GROW_OR_COPY_FAILURE";
        case FailurePoint::InsertNotificationFailure:
            return "INSERT_NOTIFICATION_FAILURE";
        case FailurePoint::CallbackReentryOrEscape:
            return "CALLBACK_REENTRY_OR_ESCAPE";
        case FailurePoint::PublicationFailure:
            return "PUBLICATION_FAILURE";
        case FailurePoint::RemoveCallbackFailure:
            return "REMOVE_CALLBACK_FAILURE";
        case FailurePoint::ReaderDrainFailure:
            return "READER_DRAIN_FAILURE";
        case FailurePoint::StopOrShutdownEarlyReturn:
            return "STOP_OR_SHUTDOWN_EARLY_RETURN";
        case FailurePoint::FinalReleaseFailureOrWrongThread:
            return "FINAL_RELEASE_FAILURE_OR_WRONG_THREAD";
    }
    return "INVALID";
}

const char* StateName(LifecycleState state) noexcept {
    switch (state) {
        case LifecycleState::Empty:
            return "EMPTY";
        case LifecycleState::ObjectAllocated:
            return "OBJECT_ALLOCATED";
        case LifecycleState::ControlReady:
            return "CONTROL_READY";
        case LifecycleState::Registered:
            return "REGISTERED";
        case LifecycleState::Retiring:
            return "RETIRING";
        case LifecycleState::Unregistered:
            return "UNREGISTERED";
        case LifecycleState::Destroyed:
            return "DESTROYED";
    }
    return "INVALID";
}

const char* ThreadClassName(ThreadClass threadClass) noexcept {
    switch (threadClass) {
        case ThreadClass::AuthorityMainLifecycle:
            return "AUTHORITY_MAIN_LIFECYCLE";
        case ThreadClass::ManagerCallback:
            return "MANAGER_CALLBACK";
        case ThreadClass::Reader:
            return "READER";
    }
    return "INVALID";
}

std::string ThreadTokenName(ThreadToken token) {
    constexpr char digits[] = "0123456789ABCDEF";
    std::string output(8, '0');
    for (std::size_t index = 0; index < output.size(); ++index) {
        const auto shift = static_cast<unsigned int>((7 - index) * 4);
        output[index] = digits[(token.identity >> shift) & 0xFU];
    }
    return output;
}

const char* ErrorName(LifecycleError error) noexcept {
    switch (error) {
        case LifecycleError::None:
            return "NONE";
        case LifecycleError::WrongState:
            return "WRONG_STATE";
        case LifecycleError::WrongThread:
            return "WRONG_THREAD";
        case LifecycleError::ObjectAllocationFailure:
            return "OBJECT_ALLOCATION_FAILURE";
        case LifecycleError::ObjectConstructionFailure:
            return "OBJECT_CONSTRUCTION_FAILURE";
        case LifecycleError::ControlBlockAllocationNull:
            return "CONTROL_BLOCK_ALLOCATION_NULL";
        case LifecycleError::ControlBlockConstructionFailure:
            return "CONTROL_BLOCK_CONSTRUCTION_FAILURE";
        case LifecycleError::VectorInsertFailure:
            return "VECTOR_INSERT_FAILURE";
        case LifecycleError::VectorGrowOrCopyFailure:
            return "VECTOR_GROW_OR_COPY_FAILURE";
        case LifecycleError::InsertNotificationFailure:
            return "INSERT_NOTIFICATION_FAILURE";
        case LifecycleError::CallbackReentryOrEscape:
            return "CALLBACK_REENTRY_OR_ESCAPE";
        case LifecycleError::PublicationFailure:
            return "PUBLICATION_FAILURE";
        case LifecycleError::RemoveCallbackFailure:
            return "REMOVE_CALLBACK_FAILURE";
        case LifecycleError::ReaderDrainFailure:
            return "READER_DRAIN_FAILURE";
        case LifecycleError::StopOrShutdownEarlyReturn:
            return "STOP_OR_SHUTDOWN_EARLY_RETURN";
        case LifecycleError::FinalReleaseFailureOrWrongThread:
            return "FINAL_RELEASE_FAILURE_OR_WRONG_THREAD";
        case LifecycleError::ReferenceLimitExceeded:
            return "REFERENCE_LIMIT_EXCEEDED";
        case LifecycleError::ReferenceUnderflow:
            return "REFERENCE_UNDERFLOW";
        case LifecycleError::DuplicateRelease:
            return "DUPLICATE_RELEASE";
        case LifecycleError::AdmissionClosed:
            return "ADMISSION_CLOSED";
        case LifecycleError::VectorMembershipMismatch:
            return "VECTOR_MEMBERSHIP_MISMATCH";
        case LifecycleError::PortContractViolation:
            return "PORT_CONTRACT_VIOLATION";
    }
    return "INVALID";
}

std::size_t RefIndex(StrongRefClass referenceClass) noexcept {
    return static_cast<std::size_t>(referenceClass);
}

ThreadClass ExpectedThread(StrongRefClass referenceClass) noexcept {
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

std::uint32_t ExpectedIdentity(StrongRefClass referenceClass) noexcept {
    switch (referenceClass) {
        case StrongRefClass::InsertCallbackTransientStrong:
        case StrongRefClass::RemoveCallbackTransientStrong:
            return kCallback.identity;
        case StrongRefClass::ReaderTransientStrong:
            return kReader.identity;
        case StrongRefClass::FactoryLocalStrong:
        case StrongRefClass::ManagerVectorStrong:
        case StrongRefClass::RollbackTransientStrong:
            return kAuthority.identity;
    }
    return 0;
}

struct FakeObject final {
    std::uint32_t tag{0x0B1EC7U};
};

struct FakeControl final {
    std::uint32_t tag{0xC047201U};
};

class FakePort final : public EngineLifecyclePort {
public:
    explicit FakePort(FailurePoint failure) noexcept : failure_(failure) {}

    bool IsAuthorityThread(ThreadToken token) noexcept override {
        if (failure_ == FailurePoint::FinalReleaseFailureOrWrongThread &&
            !contains_ && controlAllocated_) {
            RecordOperation("final-release-thread-rejected-before-decrement");
            return false;
        }
        return token.threadClass == ThreadClass::AuthorityMainLifecycle &&
            token.identity == kAuthority.identity;
    }

    void* AllocateObject() noexcept override {
        RecordOperation("object-allocation-attempt");
        if (failure_ == FailurePoint::ObjectAllocationFailure) {
            RecordOperation("object-allocation-failed");
            return nullptr;
        }
        objectAllocated_ = true;
        RecordOperation("object-allocation-owned-by-coordinator");
        return &object_;
    }

    LifecycleError ConstructObject(void* object) noexcept override {
        if (object != &object_ || !objectAllocated_) {
            return LifecycleError::PortContractViolation;
        }
        if (failure_ == FailurePoint::ObjectConstructionFailure) {
            partialObjectConstructed_ = true;
            RecordOperation("object-partial-member-constructed");
            return LifecycleError::ObjectConstructionFailure;
        }
        objectConstructed_ = true;
        RecordOperation("object-construction-complete");
        return LifecycleError::None;
    }

    void DestroyRawObject(void* object, bool constructed) noexcept override {
        if (object == &object_ && objectAllocated_ &&
            constructed == objectConstructed_) {
            if (partialObjectConstructed_) {
                RecordOperation("object-partial-member-destroyed-reverse-order");
                partialObjectConstructed_ = false;
            } else if (constructed) {
                RecordOperation("constructed-object-destroyed-before-control-bind");
            }
            objectConstructed_ = false;
            objectAllocated_ = false;
            ++rawObjectFreeCount_;
            RecordOperation("raw-object-freed");
        } else {
            contractViolation_ = true;
        }
    }

    void* AllocateControlBlock() noexcept override {
        RecordOperation("control-allocation-attempt");
        if (failure_ == FailurePoint::ControlBlockAllocationNull) {
            RecordOperation("control-allocation-returned-null");
            return nullptr;
        }
        controlAllocated_ = true;
        RecordOperation("control-allocation-owned-by-coordinator");
        return &control_;
    }

    LifecycleError ConstructControlBlock(
        void* control,
        void* object) noexcept override {
        if (control != &control_ || object != &object_ ||
            !controlAllocated_ || !objectConstructed_) {
            return LifecycleError::PortContractViolation;
        }
        if (failure_ == FailurePoint::ControlBlockConstructionFailure) {
            partialControlConstructed_ = true;
            RecordOperation("control-partial-member-constructed");
            return LifecycleError::ControlBlockConstructionFailure;
        }
        controlConstructed_ = true;
        strong_[RefIndex(StrongRefClass::FactoryLocalStrong)] = 1;
        implicitWeak_ = 1;
        ObserveStrongCeiling();
        RecordOperation("control-bound-root-strong-and-implicit-weak");
        return LifecycleError::None;
    }

    void DestroyRawControlBlock(
        void* control,
        bool constructed) noexcept override {
        if (control == &control_ && controlAllocated_ &&
            constructed == controlConstructed_) {
            if (partialControlConstructed_) {
                RecordOperation("control-partial-member-destroyed-reverse-order");
                partialControlConstructed_ = false;
            }
            controlConstructed_ = false;
            controlAllocated_ = false;
            ++rawControlFreeCount_;
            RecordOperation("raw-control-freed");
        } else {
            contractViolation_ = true;
        }
    }

    LifecycleError AddStrong(
        void* control,
        StrongRefClass referenceClass,
        ThreadToken token) noexcept override {
        const auto index = RefIndex(referenceClass);
        if (control != &control_ || !controlConstructed_ ||
            token.threadClass != ExpectedThread(referenceClass) ||
            token.identity != ExpectedIdentity(referenceClass)) {
            return LifecycleError::WrongThread;
        }
        if (strong_[index] != 0 || TotalStrong() >= 6) {
            ++overflowOrDuplicateAcquireRejections_;
            RecordOperation("strong-acquire-rejected-before-abi-increment");
            return LifecycleError::ReferenceLimitExceeded;
        }
        strong_[index] = 1;
        ObserveStrongCeiling();
        RecordOperation(StrongAcquireName(referenceClass));
        return LifecycleError::None;
    }

    StrongReleaseResult ReleaseStrong(
        void* control,
        StrongRefClass referenceClass,
        bool expectFinal,
        ThreadToken token) noexcept override {
        StrongReleaseResult result{};
        const auto index = RefIndex(referenceClass);
        if (control != &control_ || !controlConstructed_ ||
            token.threadClass != ExpectedThread(referenceClass) ||
            token.identity != ExpectedIdentity(referenceClass)) {
            result.error = LifecycleError::WrongThread;
            return result;
        }
        if (strong_[index] == 0) {
            ++underflowReleaseRejections_;
            RecordOperation("strong-release-rejected-at-zero");
            result.error = LifecycleError::ReferenceUnderflow;
            return result;
        }
        if (expectFinal &&
            failure_ == FailurePoint::FinalReleaseFailureOrWrongThread) {
            result.error = LifecycleError::FinalReleaseFailureOrWrongThread;
            return result;
        }
        if (expectFinal && TotalStrong() != 1) {
            result.error = LifecycleError::PortContractViolation;
            return result;
        }

        strong_[index] = 0;
        ++releaseCalls_[index];
        RecordOperation(StrongReleaseName(referenceClass));
        if (TotalStrong() == 0) {
            if (!expectFinal || implicitWeak_ != 1) {
                result.error = LifecycleError::PortContractViolation;
                return result;
            }
            objectConstructed_ = false;
            objectAllocated_ = false;
            controlConstructed_ = false;
            controlAllocated_ = false;
            implicitWeak_ = 0;
            ++objectDeleteCount_;
            ++controlFreeCount_;
            RecordOperation("object-deleting-destructor-exactly-once");
            RecordOperation("implicit-weak-release-exactly-once");
            RecordOperation("control-block-free-exactly-once");
            result.objectDeleted = true;
            result.implicitWeakReleased = true;
            result.controlBlockFreed = true;
        }
        return result;
    }

    LifecycleError ManagerInsert(
        VirtualDeviceLifecycleCoordinator& coordinator,
        void* object,
        void* control,
        ThreadToken token) noexcept override {
        if (object != &object_ || control != &control_ ||
            token.identity != kAuthority.identity) {
            return LifecycleError::PortContractViolation;
        }
        if (failure_ == FailurePoint::VectorInsertFailure) {
            RecordOperation("vector-insert-failed-before-membership");
            return LifecycleError::VectorInsertFailure;
        }
        if (failure_ == FailurePoint::VectorGrowOrCopyFailure) {
            const auto beforeVector = existingVector_;
            const auto beforeSize = existingVectorSize_;
            RecordOperation("vector-grow-destination-acquire-begin");
            const auto acquire = AddStrong(
                control, StrongRefClass::RollbackTransientStrong, token);
            if (acquire != LifecycleError::None) {
                return LifecycleError::PortContractViolation;
            }
            RecordOperation("vector-grow-destination-acquired");
            growScratch_[0] = existingVector_[0];
            growScratchSize_ = 1;
            const auto release = ReleaseStrong(
                control,
                StrongRefClass::RollbackTransientStrong,
                false,
                token);
            if (release.error != LifecycleError::None) {
                return LifecycleError::PortContractViolation;
            }
            growScratch_[0] = 0;
            growScratchSize_ = 0;
            RecordOperation("vector-grow-destination-undone");
            vectorPreservedAfterGrowFailure_ =
                beforeVector == existingVector_ &&
                beforeSize == existingVectorSize_ &&
                growScratchSize_ == 0;
            if (vectorPreservedAfterGrowFailure_) {
                RecordOperation("preexisting-vector-preserved-by-state-compare");
            }
            return LifecycleError::VectorGrowOrCopyFailure;
        }
        contains_ = true;
        RecordOperation("manager-vector-membership-established");
        if (exerciseCallbackMoves_) {
            constexpr ThreadToken wrongCallbackIdentity{
                ThreadClass::ManagerCallback, 0xCA11BAC2U};
            CallbackLease wrongIdentity(
                coordinator,
                *this,
                wrongCallbackIdentity,
                CallbackKind::Insert);
            CallbackLease first(
                coordinator, *this, kCallback, CallbackKind::Insert);
            CallbackLease moved(std::move(first));
            CallbackLease wrongKind(
                coordinator, *this, kCallback, CallbackKind::Remove);
            const bool invalidWrongKind = !wrongKind.valid() &&
                wrongKind.error() == LifecycleError::AdmissionClosed;
            CallbackLease duplicate(
                coordinator, *this, kCallback, CallbackKind::Insert);
            if (!callbackMoveTarget_) {
                return LifecycleError::PortContractViolation;
            }
            *callbackMoveTarget_ = std::move(moved);
            const bool targetOwnsSource = callbackMoveTarget_->valid();
            callbackMoveTarget_.reset();
            callbackMoveContractPassed_ =
                !first.valid() && !moved.valid() && targetOwnsSource &&
                invalidWrongKind && !wrongIdentity.valid() &&
                wrongIdentity.error() == LifecycleError::WrongThread &&
                !duplicate.valid() &&
                duplicate.error() == LifecycleError::ReferenceLimitExceeded;
            RecordOperation("insert-callback-move-contract-exercised");
        } else {
            CallbackLease callback(
                coordinator, *this, kCallback, CallbackKind::Insert);
            if (!callback.valid()) {
                return callback.error();
            }
            RecordOperation("insert-callback-synchronous-body");
        }
        if (failure_ == FailurePoint::InsertNotificationFailure) {
            RecordOperation("insert-notification-failed-after-membership");
            return LifecycleError::InsertNotificationFailure;
        }
        RecordOperation("manager-insert-complete");
        return LifecycleError::None;
    }

    LifecycleError ManagerRemove(
        VirtualDeviceLifecycleCoordinator& coordinator,
        void* object,
        void* control,
        ThreadToken token) noexcept override {
        if (object != &object_ || control != &control_ || !contains_ ||
            token.identity != kAuthority.identity) {
            return LifecycleError::PortContractViolation;
        }
        {
            if (failure_ == FailurePoint::CallbackReentryOrEscape) {
                escapedCallback_.reset(new (std::nothrow) CallbackLease(
                    coordinator, *this, kCallback, CallbackKind::Remove));
                if (!escapedCallback_ || !escapedCallback_->valid()) {
                    return LifecycleError::PortContractViolation;
                }
                RecordOperation("remove-callback-reference-escaped");
                return LifecycleError::CallbackReentryOrEscape;
            }
            CallbackLease callback(
                coordinator, *this, kCallback, CallbackKind::Remove);
            if (!callback.valid()) {
                return callback.error();
            }
            RecordOperation("remove-callback-synchronous-body");
        }
        if (failure_ == FailurePoint::RemoveCallbackFailure) {
            RecordOperation("remove-callback-failed-before-erase");
            return LifecycleError::RemoveCallbackFailure;
        }
        contains_ = false;
        RecordOperation("manager-vector-membership-removed");
        return LifecycleError::None;
    }

    bool ManagerContains(void* object, void* control) noexcept override {
        return contains_ && object == &object_ && control == &control_;
    }

    LifecycleError ConfirmPublication(
        void* object,
        void* control,
        ThreadToken token) noexcept override {
        if (object != &object_ || control != &control_ || !contains_ ||
            token.identity != kAuthority.identity) {
            return LifecycleError::PortContractViolation;
        }
        return failure_ == FailurePoint::PublicationFailure
            ? (RecordOperation("publication-confirmation-failed"),
               LifecycleError::PublicationFailure)
            : (RecordOperation("publication-confirmed"), LifecycleError::None);
    }

    LifecycleError CloseAdmissionAndPublishNeutral(
        void* object,
        ThreadToken token) noexcept override {
        if (object != &object_ || token.identity != kAuthority.identity) {
            return LifecycleError::PortContractViolation;
        }
        return failure_ == FailurePoint::StopOrShutdownEarlyReturn
            ? (RecordOperation("shutdown-early-return-after-admission-close"),
               LifecycleError::StopOrShutdownEarlyReturn)
            : (RecordOperation("neutral-published-with-admission-closed"),
               LifecycleError::None);
    }

    LifecycleError CompleteDrain(ThreadToken token) noexcept override {
        if (token.identity != kAuthority.identity) {
            return LifecycleError::PortContractViolation;
        }
        return failure_ == FailurePoint::ReaderDrainFailure
            ? (RecordOperation("reader-drain-incomplete"),
               LifecycleError::ReaderDrainFailure)
            : (RecordOperation("reader-and-callback-drain-confirmed"),
               LifecycleError::None);
    }

    void ReleaseEscapedCallback() noexcept {
        escapedCallback_.reset();
    }

    void ClearFailure() noexcept {
        failure_ = FailurePoint::None;
    }

    void ExerciseCallbackMoves() noexcept {
        exerciseCallbackMoves_ = true;
    }

    [[nodiscard]] std::unique_ptr<CallbackLease> TakeEscapedCallback() noexcept {
        return std::move(escapedCallback_);
    }

    void AdoptCallbackMoveTarget(
        std::unique_ptr<CallbackLease> target) noexcept {
        callbackMoveTarget_ = std::move(target);
    }

    [[nodiscard]] bool callbackMoveContractPassed() const noexcept {
        return callbackMoveContractPassed_;
    }

    [[nodiscard]] std::uint16_t TotalStrong() const noexcept {
        std::uint16_t total = 0;
        for (const auto count : strong_) {
            total = static_cast<std::uint16_t>(total + count);
        }
        return total;
    }

    [[nodiscard]] bool contractViolation() const noexcept {
        return contractViolation_;
    }

    [[nodiscard]] std::uint16_t objectDeleteCount() const noexcept {
        return objectDeleteCount_;
    }

    [[nodiscard]] std::uint16_t controlFreeCount() const noexcept {
        return controlFreeCount_;
    }

    [[nodiscard]] std::uint16_t maximumObservedStrong() const noexcept {
        return maximumObservedStrong_;
    }

    [[nodiscard]] std::uint16_t releaseCalls(
        StrongRefClass referenceClass) const noexcept {
        return releaseCalls_[RefIndex(referenceClass)];
    }

    [[nodiscard]] std::uint16_t overflowRejections() const noexcept {
        return overflowOrDuplicateAcquireRejections_;
    }

    [[nodiscard]] std::uint16_t underflowRejections() const noexcept {
        return underflowReleaseRejections_;
    }

    [[nodiscard]] std::array<std::uint32_t, 4> existingVector() const noexcept {
        return existingVector_;
    }

    [[nodiscard]] std::size_t existingVectorSize() const noexcept {
        return existingVectorSize_;
    }

    [[nodiscard]] std::size_t growScratchSize() const noexcept {
        return growScratchSize_;
    }

    [[nodiscard]] bool vectorPreservedAfterGrowFailure() const noexcept {
        return vectorPreservedAfterGrowFailure_;
    }

    [[nodiscard]] std::vector<std::string> OperationLog() const {
        std::vector<std::string> result;
        result.reserve(operationCount_);
        for (std::size_t index = 0; index < operationCount_; ++index) {
            result.emplace_back(operations_[index]);
        }
        return result;
    }

private:
    static const char* StrongAcquireName(
        StrongRefClass referenceClass) noexcept {
        switch (referenceClass) {
            case StrongRefClass::FactoryLocalStrong:
                return "factory-root-strong-acquired";
            case StrongRefClass::ManagerVectorStrong:
                return "manager-vector-strong-acquired";
            case StrongRefClass::InsertCallbackTransientStrong:
                return "insert-callback-strong-acquired";
            case StrongRefClass::RemoveCallbackTransientStrong:
                return "remove-callback-strong-acquired";
            case StrongRefClass::ReaderTransientStrong:
                return "reader-strong-acquired";
            case StrongRefClass::RollbackTransientStrong:
                return "rollback-strong-acquired";
        }
        return "invalid-strong-acquire";
    }

    static const char* StrongReleaseName(
        StrongRefClass referenceClass) noexcept {
        switch (referenceClass) {
            case StrongRefClass::FactoryLocalStrong:
                return "factory-root-strong-released";
            case StrongRefClass::ManagerVectorStrong:
                return "manager-vector-strong-released";
            case StrongRefClass::InsertCallbackTransientStrong:
                return "insert-callback-strong-released";
            case StrongRefClass::RemoveCallbackTransientStrong:
                return "remove-callback-strong-released";
            case StrongRefClass::ReaderTransientStrong:
                return "reader-strong-released";
            case StrongRefClass::RollbackTransientStrong:
                return "rollback-strong-released";
        }
        return "invalid-strong-release";
    }

    void RecordOperation(const char* operation) noexcept {
        if (operationCount_ < operations_.size()) {
            operations_[operationCount_] = operation;
            ++operationCount_;
        } else {
            contractViolation_ = true;
        }
    }

    void ObserveStrongCeiling() noexcept {
        const auto total = TotalStrong();
        if (total > maximumObservedStrong_) {
            maximumObservedStrong_ = total;
        }
        if (total > kMaximumSimultaneousStrong) {
            contractViolation_ = true;
        }
    }

    FailurePoint failure_{FailurePoint::None};
    FakeObject object_{};
    FakeControl control_{};
    bool objectAllocated_{false};
    bool objectConstructed_{false};
    bool partialObjectConstructed_{false};
    bool controlAllocated_{false};
    bool controlConstructed_{false};
    bool partialControlConstructed_{false};
    bool contains_{false};
    bool contractViolation_{false};
    bool exerciseCallbackMoves_{false};
    bool callbackMoveContractPassed_{false};
    std::array<std::uint16_t, kStrongRefClassCount> strong_{};
    std::uint16_t implicitWeak_{0};
    std::uint16_t rawObjectFreeCount_{0};
    std::uint16_t rawControlFreeCount_{0};
    std::uint16_t objectDeleteCount_{0};
    std::uint16_t controlFreeCount_{0};
    std::uint16_t maximumObservedStrong_{0};
    std::uint16_t overflowOrDuplicateAcquireRejections_{0};
    std::uint16_t underflowReleaseRejections_{0};
    std::array<std::uint16_t, kStrongRefClassCount> releaseCalls_{};
    std::array<std::uint32_t, 4> existingVector_{
        0x11110001U, 0x22220002U, 0U, 0U};
    std::size_t existingVectorSize_{2};
    std::array<std::uint32_t, 4> growScratch_{};
    std::size_t growScratchSize_{0};
    bool vectorPreservedAfterGrowFailure_{false};
    std::array<const char*, 192> operations_{};
    std::size_t operationCount_{0};
    std::unique_ptr<CallbackLease> escapedCallback_{};
    std::unique_ptr<CallbackLease> callbackMoveTarget_{};
};

struct Event final {
    std::string operation;
    LifecycleState before{LifecycleState::Empty};
    LifecycleState after{LifecycleState::Empty};
    std::array<int, kStrongRefClassCount> classDelta{};
    std::string threadClass{"AUTHORITY_MAIN_LIFECYCLE"};
    std::string threadToken{"A11CE001"};
    LifecycleError result{LifecycleError::None};
    std::string failurePoint;
};

struct CaseResult final {
    std::string id;
    std::string injectedFailure;
    bool passed{true};
    LifecycleSnapshot finalSnapshot{};
    LifecycleSnapshot failureSnapshot{};
    bool hasFailureSnapshot{false};
    std::vector<Event> events;
    std::vector<std::string> operationLog;
};

std::uint32_t gAssertionCount = 0;
std::vector<std::string> gFailures;

void Check(bool condition, CaseResult& result, const char* message) {
    ++gAssertionCount;
    if (!condition) {
        result.passed = false;
        gFailures.emplace_back(result.id + ": " + message);
    }
}

using OperationLog = std::vector<std::string>;

bool ContainsOrdered(
    const OperationLog& actual,
    std::initializer_list<const char*> expected) {
    std::size_t cursor = 0;
    for (const auto* item : expected) {
        bool found = false;
        while (cursor < actual.size()) {
            if (actual[cursor] == item) {
                found = true;
                ++cursor;
                break;
            }
            ++cursor;
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

void AppendOperations(
    OperationLog& log,
    std::initializer_list<const char*> operations) {
    for (const auto* operation : operations) {
        log.emplace_back(operation);
    }
}

void AppendConstructedControl(OperationLog& log) {
    AppendOperations(log, {
        "object-allocation-attempt",
        "object-allocation-owned-by-coordinator",
        "object-construction-complete",
        "control-allocation-attempt",
        "control-allocation-owned-by-coordinator",
        "control-bound-root-strong-and-implicit-weak",
    });
}

void AppendCreateBeforeManagerCall(OperationLog& log) {
    AppendConstructedControl(log);
    log.emplace_back("manager-vector-strong-acquired");
}

void AppendNormalInsertAndPublication(OperationLog& log) {
    AppendOperations(log, {
        "manager-vector-membership-established",
        "insert-callback-strong-acquired",
        "insert-callback-synchronous-body",
        "insert-callback-strong-released",
        "manager-insert-complete",
        "publication-confirmed",
    });
}

void AppendCreateSuccess(OperationLog& log) {
    AppendCreateBeforeManagerCall(log);
    AppendNormalInsertAndPublication(log);
}

void AppendRetireSuccess(OperationLog& log) {
    AppendOperations(log, {
        "neutral-published-with-admission-closed",
        "reader-and-callback-drain-confirmed",
    });
}

void AppendRemoveAndFinalSuccess(OperationLog& log) {
    AppendOperations(log, {
        "remove-callback-strong-acquired",
        "remove-callback-synchronous-body",
        "remove-callback-strong-released",
        "manager-vector-membership-removed",
        "manager-vector-strong-released",
        "factory-root-strong-released",
        "object-deleting-destructor-exactly-once",
        "implicit-weak-release-exactly-once",
        "control-block-free-exactly-once",
    });
}

OperationLog ExpectedFailureLog(FailurePoint point) {
    OperationLog log;
    switch (point) {
        case FailurePoint::ObjectAllocationFailure:
            AppendOperations(log, {
                "object-allocation-attempt",
                "object-allocation-failed",
            });
            break;
        case FailurePoint::ObjectConstructionFailure:
            AppendOperations(log, {
                "object-allocation-attempt",
                "object-allocation-owned-by-coordinator",
                "object-partial-member-constructed",
                "object-partial-member-destroyed-reverse-order",
                "raw-object-freed",
            });
            break;
        case FailurePoint::ControlBlockAllocationNull:
            AppendOperations(log, {
                "object-allocation-attempt",
                "object-allocation-owned-by-coordinator",
                "object-construction-complete",
                "control-allocation-attempt",
                "control-allocation-returned-null",
                "constructed-object-destroyed-before-control-bind",
                "raw-object-freed",
            });
            break;
        case FailurePoint::ControlBlockConstructionFailure:
            AppendOperations(log, {
                "object-allocation-attempt",
                "object-allocation-owned-by-coordinator",
                "object-construction-complete",
                "control-allocation-attempt",
                "control-allocation-owned-by-coordinator",
                "control-partial-member-constructed",
                "control-partial-member-destroyed-reverse-order",
                "raw-control-freed",
                "constructed-object-destroyed-before-control-bind",
                "raw-object-freed",
            });
            break;
        case FailurePoint::VectorInsertFailure:
            AppendCreateBeforeManagerCall(log);
            AppendOperations(log, {
                "vector-insert-failed-before-membership",
                "rollback-strong-acquired",
                "manager-vector-strong-released",
                "rollback-strong-released",
                "factory-root-strong-released",
                "object-deleting-destructor-exactly-once",
                "implicit-weak-release-exactly-once",
                "control-block-free-exactly-once",
            });
            break;
        case FailurePoint::VectorGrowOrCopyFailure:
            AppendCreateBeforeManagerCall(log);
            AppendOperations(log, {
                "vector-grow-destination-acquire-begin",
                "rollback-strong-acquired",
                "vector-grow-destination-acquired",
                "rollback-strong-released",
                "vector-grow-destination-undone",
                "preexisting-vector-preserved-by-state-compare",
                "rollback-strong-acquired",
                "manager-vector-strong-released",
                "rollback-strong-released",
                "factory-root-strong-released",
                "object-deleting-destructor-exactly-once",
                "implicit-weak-release-exactly-once",
                "control-block-free-exactly-once",
            });
            break;
        case FailurePoint::InsertNotificationFailure:
            AppendCreateBeforeManagerCall(log);
            AppendOperations(log, {
                "manager-vector-membership-established",
                "insert-callback-strong-acquired",
                "insert-callback-synchronous-body",
                "insert-callback-strong-released",
                "insert-notification-failed-after-membership",
                "rollback-strong-acquired",
                "remove-callback-strong-acquired",
                "remove-callback-synchronous-body",
                "remove-callback-strong-released",
                "manager-vector-membership-removed",
                "manager-vector-strong-released",
                "rollback-strong-released",
                "factory-root-strong-released",
                "object-deleting-destructor-exactly-once",
                "implicit-weak-release-exactly-once",
                "control-block-free-exactly-once",
            });
            break;
        case FailurePoint::CallbackReentryOrEscape:
            AppendCreateSuccess(log);
            AppendRetireSuccess(log);
            AppendOperations(log, {
                "remove-callback-strong-acquired",
                "remove-callback-reference-escaped",
                "remove-callback-strong-released",
            });
            AppendRemoveAndFinalSuccess(log);
            break;
        case FailurePoint::PublicationFailure:
            AppendCreateBeforeManagerCall(log);
            AppendOperations(log, {
                "manager-vector-membership-established",
                "insert-callback-strong-acquired",
                "insert-callback-synchronous-body",
                "insert-callback-strong-released",
                "manager-insert-complete",
                "publication-confirmation-failed",
                "rollback-strong-acquired",
                "remove-callback-strong-acquired",
                "remove-callback-synchronous-body",
                "remove-callback-strong-released",
                "manager-vector-membership-removed",
                "manager-vector-strong-released",
                "rollback-strong-released",
                "factory-root-strong-released",
                "object-deleting-destructor-exactly-once",
                "implicit-weak-release-exactly-once",
                "control-block-free-exactly-once",
            });
            break;
        case FailurePoint::RemoveCallbackFailure:
            AppendCreateSuccess(log);
            AppendRetireSuccess(log);
            AppendOperations(log, {
                "remove-callback-strong-acquired",
                "remove-callback-synchronous-body",
                "remove-callback-strong-released",
                "remove-callback-failed-before-erase",
            });
            break;
        case FailurePoint::ReaderDrainFailure:
            AppendCreateSuccess(log);
            AppendOperations(log, {
                "neutral-published-with-admission-closed",
                "reader-drain-incomplete",
            });
            break;
        case FailurePoint::StopOrShutdownEarlyReturn:
            AppendCreateSuccess(log);
            log.emplace_back("shutdown-early-return-after-admission-close");
            AppendRetireSuccess(log);
            AppendRemoveAndFinalSuccess(log);
            break;
        case FailurePoint::FinalReleaseFailureOrWrongThread:
            AppendCreateSuccess(log);
            AppendRetireSuccess(log);
            AppendOperations(log, {
                "remove-callback-strong-acquired",
                "remove-callback-synchronous-body",
                "remove-callback-strong-released",
                "manager-vector-membership-removed",
                "manager-vector-strong-released",
                "final-release-thread-rejected-before-decrement",
            });
            break;
        case FailurePoint::None:
            break;
    }
    return log;
}

void CheckExactOperationLog(
    const OperationLog& actual,
    const OperationLog& expected,
    CaseResult& result,
    const char* message) {
    Check(actual == expected, result, message);
}

struct ExpectedFailureSnapshot final {
    LifecycleState state{LifecycleState::Empty};
    LifecycleError firstError{LifecycleError::None};
    bool drainConfirmed{false};
    bool vectorPresent{false};
    bool objectPresent{false};
    bool controlPresent{false};
    std::uint16_t liveCallbacks{0};
    std::array<std::uint16_t, kStrongRefClassCount> strongByClass{};
    std::uint16_t implicitWeak{0};
    std::uint16_t objectDeleteCount{0};
    std::uint16_t controlFreeCount{0};
    std::uint16_t rawObjectFreeCount{0};
    std::uint16_t rawControlFreeCount{0};
    std::array<LifecycleState, 7> transitionTrace{};
    std::uint8_t transitionCount{1};
};

ExpectedFailureSnapshot ExpectedSnapshotAtFailure(FailurePoint point) {
    ExpectedFailureSnapshot expected{};
    expected.firstError = LifecycleError::None;
    expected.transitionTrace.fill(LifecycleState::Empty);

    switch (point) {
        case FailurePoint::ObjectAllocationFailure:
            expected.firstError = LifecycleError::ObjectAllocationFailure;
            break;
        case FailurePoint::ObjectConstructionFailure:
            expected.firstError = LifecycleError::ObjectConstructionFailure;
            expected.rawObjectFreeCount = 1;
            expected.transitionTrace[1] = LifecycleState::ObjectAllocated;
            expected.transitionCount = 3;
            break;
        case FailurePoint::ControlBlockAllocationNull:
            expected.firstError = LifecycleError::ControlBlockAllocationNull;
            expected.rawObjectFreeCount = 1;
            expected.transitionTrace[1] = LifecycleState::ObjectAllocated;
            expected.transitionCount = 3;
            break;
        case FailurePoint::ControlBlockConstructionFailure:
            expected.firstError = LifecycleError::ControlBlockConstructionFailure;
            expected.rawObjectFreeCount = 1;
            expected.rawControlFreeCount = 1;
            expected.transitionTrace[1] = LifecycleState::ObjectAllocated;
            expected.transitionCount = 3;
            break;
        case FailurePoint::VectorInsertFailure:
        case FailurePoint::VectorGrowOrCopyFailure:
        case FailurePoint::InsertNotificationFailure:
        case FailurePoint::PublicationFailure:
            expected.state = LifecycleState::Destroyed;
            expected.firstError = point == FailurePoint::VectorInsertFailure
                ? LifecycleError::VectorInsertFailure
                : point == FailurePoint::VectorGrowOrCopyFailure
                    ? LifecycleError::VectorGrowOrCopyFailure
                    : point == FailurePoint::InsertNotificationFailure
                        ? LifecycleError::InsertNotificationFailure
                        : LifecycleError::PublicationFailure;
            expected.objectDeleteCount = 1;
            expected.controlFreeCount = 1;
            expected.transitionTrace[1] = LifecycleState::ObjectAllocated;
            expected.transitionTrace[2] = LifecycleState::ControlReady;
            expected.transitionTrace[3] = LifecycleState::Destroyed;
            expected.transitionCount = 4;
            break;
        case FailurePoint::CallbackReentryOrEscape:
            expected.state = LifecycleState::Retiring;
            expected.firstError = LifecycleError::CallbackReentryOrEscape;
            expected.drainConfirmed = true;
            expected.vectorPresent = true;
            expected.objectPresent = true;
            expected.controlPresent = true;
            expected.liveCallbacks = 1;
            expected.strongByClass = {1, 1, 0, 1, 0, 0};
            expected.implicitWeak = 1;
            expected.transitionTrace[1] = LifecycleState::ObjectAllocated;
            expected.transitionTrace[2] = LifecycleState::ControlReady;
            expected.transitionTrace[3] = LifecycleState::Registered;
            expected.transitionTrace[4] = LifecycleState::Retiring;
            expected.transitionCount = 5;
            break;
        case FailurePoint::RemoveCallbackFailure:
        case FailurePoint::ReaderDrainFailure:
        case FailurePoint::StopOrShutdownEarlyReturn:
            expected.state = LifecycleState::Retiring;
            expected.firstError = point == FailurePoint::RemoveCallbackFailure
                ? LifecycleError::RemoveCallbackFailure
                : point == FailurePoint::ReaderDrainFailure
                    ? LifecycleError::ReaderDrainFailure
                    : LifecycleError::StopOrShutdownEarlyReturn;
            expected.drainConfirmed =
                point == FailurePoint::RemoveCallbackFailure;
            expected.vectorPresent = true;
            expected.objectPresent = true;
            expected.controlPresent = true;
            expected.strongByClass = {1, 1, 0, 0, 0, 0};
            expected.implicitWeak = 1;
            expected.transitionTrace[1] = LifecycleState::ObjectAllocated;
            expected.transitionTrace[2] = LifecycleState::ControlReady;
            expected.transitionTrace[3] = LifecycleState::Registered;
            expected.transitionTrace[4] = LifecycleState::Retiring;
            expected.transitionCount = 5;
            break;
        case FailurePoint::FinalReleaseFailureOrWrongThread:
            expected.state = LifecycleState::Unregistered;
            expected.firstError =
                LifecycleError::FinalReleaseFailureOrWrongThread;
            expected.drainConfirmed = true;
            expected.objectPresent = true;
            expected.controlPresent = true;
            expected.strongByClass = {1, 0, 0, 0, 0, 0};
            expected.implicitWeak = 1;
            expected.transitionTrace[1] = LifecycleState::ObjectAllocated;
            expected.transitionTrace[2] = LifecycleState::ControlReady;
            expected.transitionTrace[3] = LifecycleState::Registered;
            expected.transitionTrace[4] = LifecycleState::Retiring;
            expected.transitionTrace[5] = LifecycleState::Unregistered;
            expected.transitionCount = 6;
            break;
        case FailurePoint::None:
            break;
    }
    return expected;
}

void CheckFailureSnapshot(
    FailurePoint point,
    const LifecycleSnapshot& actual,
    CaseResult& result) {
    const auto expected = ExpectedSnapshotAtFailure(point);
    std::uint16_t expectedTotalStrong = 0;
    for (const auto count : expected.strongByClass) {
        expectedTotalStrong =
            static_cast<std::uint16_t>(expectedTotalStrong + count);
    }

    Check(actual.state == expected.state, result,
          "failure snapshot state mismatch");
    Check(actual.firstError == expected.firstError, result,
          "failure snapshot first error mismatch");
    Check(!actual.admissionOpen && !actual.publicationConfirmed,
          result, "failure snapshot left admission/publication open");
    Check(actual.drainConfirmed == expected.drainConfirmed,
          result, "failure snapshot drain flag mismatch");
    Check(actual.vectorPresent == expected.vectorPresent &&
          actual.objectPresent == expected.objectPresent &&
          actual.controlPresent == expected.controlPresent,
          result, "failure snapshot owner-presence mismatch");
    Check(!actual.finalReleaseStarted, result,
          "failure snapshot left final release active");
    Check(actual.liveReaders == 0 &&
          actual.liveCallbacks == expected.liveCallbacks,
          result, "failure snapshot live lease count mismatch");
    Check(actual.strongByClass == expected.strongByClass &&
          actual.totalStrong == expectedTotalStrong,
          result, "failure snapshot strong-reference ledger mismatch");
    Check(actual.implicitWeak == expected.implicitWeak &&
          actual.externalWeak == 0,
          result, "failure snapshot weak-reference ledger mismatch");
    Check(actual.objectDeleteCount == expected.objectDeleteCount &&
          actual.controlFreeCount == expected.controlFreeCount &&
          actual.rawObjectFreeCount == expected.rawObjectFreeCount &&
          actual.rawControlFreeCount == expected.rawControlFreeCount,
          result, "failure snapshot delete/free count mismatch");
    Check(actual.transitionCount == expected.transitionCount &&
          actual.transitionTrace == expected.transitionTrace,
          result, "failure snapshot transition trace mismatch");
}

template <typename Operation>
LifecycleError Record(
    CaseResult& result,
    VirtualDeviceLifecycleCoordinator& coordinator,
    const char* operation,
    const char* failurePoint,
    ThreadToken token,
    Operation&& invoke) {
    const auto before = coordinator.Snapshot();
    const auto error = invoke();
    const auto after = coordinator.Snapshot();
    std::array<int, kStrongRefClassCount> deltas{};
    for (std::size_t index = 0; index < deltas.size(); ++index) {
        deltas[index] = static_cast<int>(after.strongByClass[index]) -
            static_cast<int>(before.strongByClass[index]);
    }
    result.events.push_back(Event{
        operation,
        before.state,
        after.state,
        deltas,
        ThreadClassName(token.threadClass),
        ThreadTokenName(token),
        error,
        failurePoint,
    });
    return error;
}

CaseResult RunHappyPath() {
    CaseResult result{"HAPPY_SEVEN_STATE_PATH", "NONE"};
    FakePort port(FailurePoint::None);
    VirtualDeviceLifecycleCoordinator coordinator;

    Check(Record(result, coordinator, "CreateAndRegister", "NONE", kAuthority, [&] {
        return coordinator.CreateAndRegister(port, kAuthority);
    }) == LifecycleError::None, result, "create/register failed");
    Check(Record(result, coordinator, "BeginRetire", "NONE", kAuthority, [&] {
        return coordinator.BeginRetire(port, kAuthority);
    }) == LifecycleError::None, result, "begin retire failed");
    Check(Record(result, coordinator, "RemoveAndDestroy", "NONE", kAuthority, [&] {
        return coordinator.RemoveAndDestroy(port, kAuthority);
    }) == LifecycleError::None, result, "remove/destroy failed");

    result.finalSnapshot = coordinator.Snapshot();
    const std::array<LifecycleState, 7> expected{
        LifecycleState::Empty,
        LifecycleState::ObjectAllocated,
        LifecycleState::ControlReady,
        LifecycleState::Registered,
        LifecycleState::Retiring,
        LifecycleState::Unregistered,
        LifecycleState::Destroyed,
    };
    Check(static_cast<std::size_t>(result.finalSnapshot.transitionCount) ==
              expected.size(),
          result, "transition count is not seven");
    Check(result.finalSnapshot.transitionTrace == expected,
          result, "seven-state trace mismatch");
    Check(result.finalSnapshot.state == LifecycleState::Destroyed,
          result, "final state is not destroyed");
    Check(result.finalSnapshot.objectDeleteCount == 1 &&
          result.finalSnapshot.controlFreeCount == 1,
          result, "final deletion/free is not exactly once");
    Check(result.finalSnapshot.totalStrong == 0 &&
          result.finalSnapshot.implicitWeak == 0 &&
          result.finalSnapshot.externalWeak == 0,
          result, "terminal references are not zero");
    Check(port.objectDeleteCount() == 1 && port.controlFreeCount() == 1 &&
          !port.contractViolation(), result, "port terminal accounting mismatch");
    Check(port.releaseCalls(StrongRefClass::FactoryLocalStrong) == 1 &&
          port.releaseCalls(StrongRefClass::ManagerVectorStrong) == 1 &&
          port.releaseCalls(StrongRefClass::InsertCallbackTransientStrong) == 1 &&
          port.releaseCalls(StrongRefClass::RemoveCallbackTransientStrong) == 1,
          result, "normal path did not balance every acquired class once");
    result.operationLog = port.OperationLog();
    OperationLog expectedOperations;
    AppendCreateSuccess(expectedOperations);
    AppendRetireSuccess(expectedOperations);
    AppendRemoveAndFinalSuccess(expectedOperations);
    CheckExactOperationLog(
        result.operationLog,
        expectedOperations,
        result,
        "normal lifecycle full operation log mismatch");
    Check(ContainsOrdered(result.operationLog, {
              "object-allocation-owned-by-coordinator",
              "object-construction-complete",
              "control-bound-root-strong-and-implicit-weak",
              "manager-vector-strong-acquired",
              "manager-vector-membership-established",
              "insert-callback-strong-acquired",
              "insert-callback-strong-released",
              "publication-confirmed",
              "neutral-published-with-admission-closed",
              "reader-and-callback-drain-confirmed",
              "remove-callback-strong-acquired",
              "remove-callback-strong-released",
              "manager-vector-membership-removed",
              "manager-vector-strong-released",
              "factory-root-strong-released",
              "object-deleting-destructor-exactly-once",
              "implicit-weak-release-exactly-once",
              "control-block-free-exactly-once",
          }), result, "normal lifecycle operation order mismatch");
    return result;
}

CaseResult RunLeaseAndBoundCase() {
    CaseResult result{"MOVE_ONLY_LEASE_AND_REFERENCE_BOUND", "NONE"};
    FakePort port(FailurePoint::None);
    FakePort targetPort(FailurePoint::None);
    VirtualDeviceLifecycleCoordinator coordinator;
    VirtualDeviceLifecycleCoordinator targetCoordinator;
    Check(coordinator.CreateAndRegister(port, kAuthority) == LifecycleError::None,
          result, "setup failed");
    Check(targetCoordinator.CreateAndRegister(targetPort, kAuthority) ==
              LifecycleError::None,
          result, "valid move-assignment target setup failed");
    {
        constexpr ThreadToken wrongReaderIdentity{
            ThreadClass::Reader, 0x0EADB002U};
        ReaderLease wrongIdentity(coordinator, port, wrongReaderIdentity);
        Check(!wrongIdentity.valid() &&
              wrongIdentity.error() == LifecycleError::WrongThread,
              result, "same-class wrong reader identity reached ABI acquire");
        ReaderLease first(coordinator, port, kReader);
        Check(first.valid(), result, "first reader admission failed");
        ReaderLease moved(std::move(first));
        ReaderLease assignedTarget(targetCoordinator, targetPort, kReader);
        Check(assignedTarget.valid(), result,
              "move-assignment target was not initially valid");
        assignedTarget = std::move(moved);
        Check(!first.valid() && !moved.valid() && assignedTarget.valid(), result,
              "reader move assignment did not release target and transfer source");
        Check(targetPort.releaseCalls(StrongRefClass::ReaderTransientStrong) == 1,
              result, "valid reader move target was not released exactly once");
        ReaderLease second(coordinator, port, kReader);
        Check(!second.valid() &&
              second.error() == LifecycleError::ReferenceLimitExceeded,
              result, "per-class bound did not reject second reader");
        Check(Record(result, coordinator, "BeginRetireWithLiveReader", "NONE", kAuthority, [&] {
            return coordinator.BeginRetire(port, kAuthority);
        }) == LifecycleError::ReaderDrainFailure,
              result, "live reader crossed a completed drain");
    }
    Check(Record(result, coordinator, "BeginRetireAfterReaderRelease", "NONE", kAuthority, [&] {
        return coordinator.BeginRetire(port, kAuthority);
    }) == LifecycleError::None, result, "drain retry failed");
    Check(coordinator.RemoveAndDestroy(port, kAuthority) == LifecycleError::None,
          result, "lease case teardown failed");
    Check(targetCoordinator.BeginRetire(targetPort, kAuthority) ==
              LifecycleError::None &&
          targetCoordinator.RemoveAndDestroy(targetPort, kAuthority) ==
              LifecycleError::None,
          result, "move-assignment target coordinator teardown failed");
    result.finalSnapshot = coordinator.Snapshot();
    Check(result.finalSnapshot.state == LifecycleState::Destroyed &&
          result.finalSnapshot.totalStrong == 0,
          result, "lease case did not terminate cleanly");
    Check(kMaximumSimultaneousStrong == 6 && kMaximumExternalWeak == 0,
          result, "global reference ceiling changed");
    Check(port.releaseCalls(StrongRefClass::ReaderTransientStrong) == 1 &&
          targetPort.releaseCalls(StrongRefClass::ReaderTransientStrong) == 1 &&
          port.overflowRejections() == 0 &&
          port.underflowRejections() == 0,
          result, "invalid reader reached the ABI or moved-from reader released");
    Check(port.maximumObservedStrong() <= kMaximumSimultaneousStrong,
          result, "port observed a strong-count overflow");
    result.operationLog = port.OperationLog();

    OperationLog expectedOperations;
    AppendCreateSuccess(expectedOperations);
    AppendOperations(expectedOperations, {
        "reader-strong-acquired",
        "neutral-published-with-admission-closed",
        "reader-strong-released",
    });
    AppendRetireSuccess(expectedOperations);
    AppendRemoveAndFinalSuccess(expectedOperations);
    CheckExactOperationLog(
        result.operationLog,
        expectedOperations,
        result,
        "reader source full operation log mismatch");

    OperationLog expectedTargetOperations;
    AppendCreateSuccess(expectedTargetOperations);
    AppendOperations(expectedTargetOperations, {
        "reader-strong-acquired",
        "reader-strong-released",
    });
    AppendRetireSuccess(expectedTargetOperations);
    AppendRemoveAndFinalSuccess(expectedTargetOperations);
    CheckExactOperationLog(
        targetPort.OperationLog(),
        expectedTargetOperations,
        result,
        "reader move target full operation log mismatch");
    return result;
}

CaseResult RunCallbackMoveCase() {
    CaseResult result{"CALLBACK_MOVE_ONLY_KIND_AND_RELEASE", "NONE"};
    FakePort targetPort(FailurePoint::CallbackReentryOrEscape);
    VirtualDeviceLifecycleCoordinator targetCoordinator;
    Check(targetCoordinator.CreateAndRegister(targetPort, kAuthority) ==
              LifecycleError::None &&
          targetCoordinator.BeginRetire(targetPort, kAuthority) ==
              LifecycleError::None &&
          targetCoordinator.RemoveAndDestroy(targetPort, kAuthority) ==
              LifecycleError::CallbackReentryOrEscape,
          result, "valid callback move-assignment target setup failed");
    auto validMoveTarget = targetPort.TakeEscapedCallback();
    Check(validMoveTarget && validMoveTarget->valid(), result,
          "escaped callback target was not valid");

    FakePort port(FailurePoint::None);
    port.ExerciseCallbackMoves();
    port.AdoptCallbackMoveTarget(std::move(validMoveTarget));
    VirtualDeviceLifecycleCoordinator coordinator;
    Check(coordinator.CreateAndRegister(port, kAuthority) == LifecycleError::None,
          result, "callback move setup failed");
    Check(port.callbackMoveContractPassed(), result,
          "callback kind/move contract was not exercised");
    Check(port.releaseCalls(StrongRefClass::InsertCallbackTransientStrong) == 1 &&
          targetPort.releaseCalls(
              StrongRefClass::RemoveCallbackTransientStrong) == 1 &&
          port.overflowRejections() == 0,
          result, "valid target/moved-from/invalid callback release mismatch");
    const auto targetReleasedSnapshot = targetCoordinator.Snapshot();
    Check(targetReleasedSnapshot.state == LifecycleState::Retiring &&
          targetReleasedSnapshot.liveCallbacks == 0 &&
          targetReleasedSnapshot.totalStrong == 2 &&
          targetReleasedSnapshot.vectorPresent,
          result, "callback move target lease was not safely released");
    targetPort.ClearFailure();
    Check(targetCoordinator.RemoveAndDestroy(targetPort, kAuthority) ==
              LifecycleError::None,
          result, "callback move target did not resume bounded teardown");
    const auto targetSnapshot = targetCoordinator.Snapshot();
    Check(targetSnapshot.state == LifecycleState::Destroyed &&
          targetSnapshot.totalStrong == 0 &&
          !targetSnapshot.objectPresent && !targetSnapshot.controlPresent &&
          !targetSnapshot.vectorPresent &&
          targetSnapshot.objectDeleteCount == 1 &&
          targetSnapshot.controlFreeCount == 1 &&
          targetPort.releaseCalls(
              StrongRefClass::RemoveCallbackTransientStrong) == 2,
          result, "callback move target lifecycle did not close exactly once");
    Check(coordinator.BeginRetire(port, kAuthority) == LifecycleError::None &&
          coordinator.RemoveAndDestroy(port, kAuthority) == LifecycleError::None,
          result, "callback move case teardown failed");
    result.finalSnapshot = coordinator.Snapshot();
    Check(result.finalSnapshot.state == LifecycleState::Destroyed &&
          result.finalSnapshot.totalStrong == 0,
          result, "callback move case did not terminate cleanly");
    result.operationLog = port.OperationLog();
    OperationLog expectedOperations;
    AppendCreateBeforeManagerCall(expectedOperations);
    AppendOperations(expectedOperations, {
        "manager-vector-membership-established",
        "insert-callback-strong-acquired",
        "insert-callback-strong-released",
        "insert-callback-move-contract-exercised",
        "manager-insert-complete",
        "publication-confirmed",
    });
    AppendRetireSuccess(expectedOperations);
    AppendRemoveAndFinalSuccess(expectedOperations);
    CheckExactOperationLog(
        result.operationLog,
        expectedOperations,
        result,
        "callback source full operation log mismatch");

    OperationLog expectedTargetOperations;
    AppendCreateSuccess(expectedTargetOperations);
    AppendRetireSuccess(expectedTargetOperations);
    AppendOperations(expectedTargetOperations, {
        "remove-callback-strong-acquired",
        "remove-callback-reference-escaped",
        "remove-callback-strong-released",
    });
    AppendRemoveAndFinalSuccess(expectedTargetOperations);
    CheckExactOperationLog(
        targetPort.OperationLog(),
        expectedTargetOperations,
        result,
        "callback move target full operation log mismatch");
    return result;
}

CaseResult RunFailureCase(FailurePoint point) {
    CaseResult result{FailureName(point), FailureName(point)};
    FakePort port(point);
    const auto preexistingVector = port.existingVector();
    const auto preexistingVectorSize = port.existingVectorSize();
    VirtualDeviceLifecycleCoordinator coordinator;

    const auto createError = Record(
        result, coordinator, "CreateAndRegister", FailureName(point), kAuthority, [&] {
            return coordinator.CreateAndRegister(port, kAuthority);
        });
    if (point <= FailurePoint::PublicationFailure) {
        result.failureSnapshot = coordinator.Snapshot();
        result.hasFailureSnapshot = true;
    }

    switch (point) {
        case FailurePoint::ObjectAllocationFailure:
            Check(createError == LifecycleError::ObjectAllocationFailure,
                  result, "wrong object-allocation error");
            Check(coordinator.Snapshot().state == LifecycleState::Empty,
                  result, "object-allocation failure did not stay EMPTY");
            break;
        case FailurePoint::ObjectConstructionFailure:
            Check(createError == LifecycleError::ObjectConstructionFailure,
                  result, "wrong object-construction error");
            Check(coordinator.Snapshot().state == LifecycleState::Empty &&
                  coordinator.Snapshot().rawObjectFreeCount == 1,
                  result, "object construction rollback mismatch");
            break;
        case FailurePoint::ControlBlockAllocationNull:
            Check(createError == LifecycleError::ControlBlockAllocationNull,
                  result, "wrong null-control error");
            Check(coordinator.Snapshot().state == LifecycleState::Empty &&
                  !coordinator.Snapshot().vectorPresent &&
                  coordinator.Snapshot().rawObjectFreeCount == 1,
                  result, "null control was published or leaked");
            break;
        case FailurePoint::ControlBlockConstructionFailure:
            Check(createError == LifecycleError::ControlBlockConstructionFailure,
                  result, "wrong control-construction error");
            Check(coordinator.Snapshot().state == LifecycleState::Empty &&
                  coordinator.Snapshot().rawObjectFreeCount == 1 &&
                  coordinator.Snapshot().rawControlFreeCount == 1,
                  result, "control construction rollback mismatch");
            break;
        case FailurePoint::VectorInsertFailure:
            Check(createError == LifecycleError::VectorInsertFailure,
                  result, "wrong vector-insert error");
            Check(coordinator.Snapshot().state == LifecycleState::Destroyed &&
                  coordinator.Snapshot().totalStrong == 0 &&
                  coordinator.Snapshot().objectDeleteCount == 1 &&
                  coordinator.Snapshot().controlFreeCount == 1,
                  result, "vector-insert rollback was incomplete");
            break;
        case FailurePoint::VectorGrowOrCopyFailure:
            Check(createError == LifecycleError::VectorGrowOrCopyFailure,
                  result, "wrong vector-grow error");
            Check(coordinator.Snapshot().state == LifecycleState::Destroyed &&
                  coordinator.Snapshot().totalStrong == 0 &&
                  coordinator.Snapshot().objectDeleteCount == 1 &&
                  coordinator.Snapshot().controlFreeCount == 1,
                  result, "vector-grow rollback was incomplete");
            break;
        case FailurePoint::InsertNotificationFailure:
            Check(createError == LifecycleError::InsertNotificationFailure,
                  result, "wrong insert-notification error");
            Check(coordinator.Snapshot().state == LifecycleState::Destroyed &&
                  !coordinator.Snapshot().vectorPresent,
                  result, "insert-notification rollback mismatch");
            break;
        case FailurePoint::CallbackReentryOrEscape:
            Check(createError == LifecycleError::None,
                  result, "callback escape setup failed");
            Check(coordinator.BeginRetire(port, kAuthority) == LifecycleError::None,
                  result, "callback escape retire setup failed");
            Check(Record(result, coordinator, "RemoveAndDestroy", FailureName(point), kAuthority, [&] {
                return coordinator.RemoveAndDestroy(port, kAuthority);
            }) == LifecycleError::CallbackReentryOrEscape,
                  result, "callback escape failure not reported");
            result.failureSnapshot = coordinator.Snapshot();
            result.hasFailureSnapshot = true;
            Check(result.failureSnapshot.state == LifecycleState::Retiring &&
                  result.failureSnapshot.liveCallbacks == 1 &&
                  result.failureSnapshot.vectorPresent &&
                  result.failureSnapshot.totalStrong == 3,
                  result, "remove callback escape was not quarantined");
            {
                const auto before = coordinator.Snapshot();
                port.ReleaseEscapedCallback();
                const auto after = coordinator.Snapshot();
                std::array<int, kStrongRefClassCount> deltas{};
                for (std::size_t index = 0; index < deltas.size(); ++index) {
                    deltas[index] =
                        static_cast<int>(after.strongByClass[index]) -
                        static_cast<int>(before.strongByClass[index]);
                }
                result.events.push_back(Event{
                    "ReleaseEscapedCallback",
                    before.state,
                    after.state,
                    deltas,
                    ThreadClassName(kCallback.threadClass),
                    ThreadTokenName(kCallback),
                    LifecycleError::None,
                    FailureName(point),
                });
                Check(after.liveCallbacks == 0 && after.totalStrong == 2 &&
                      after.vectorPresent,
                      result, "escaped callback did not release exactly once");
            }
            port.ClearFailure();
            Check(Record(result, coordinator, "ResumeRemoveAfterCallbackQuiescence",
                         FailureName(point), kAuthority, [&] {
                return coordinator.RemoveAndDestroy(port, kAuthority);
            }) == LifecycleError::None,
                  result, "callback quarantine had no bounded authority recovery");
            Check(coordinator.Snapshot().state == LifecycleState::Destroyed,
                  result, "callback recovery did not finish teardown");
            break;
        case FailurePoint::PublicationFailure:
            Check(createError == LifecycleError::PublicationFailure,
                  result, "wrong publication error");
            Check(coordinator.Snapshot().state == LifecycleState::Destroyed &&
                  !coordinator.Snapshot().vectorPresent,
                  result, "publication rollback mismatch");
            break;
        case FailurePoint::RemoveCallbackFailure:
            Check(createError == LifecycleError::None,
                  result, "remove-callback setup failed");
            Check(coordinator.BeginRetire(port, kAuthority) == LifecycleError::None,
                  result, "remove-callback retire setup failed");
            Check(Record(result, coordinator, "RemoveAndDestroy", FailureName(point), kAuthority, [&] {
                return coordinator.RemoveAndDestroy(port, kAuthority);
            }) == LifecycleError::RemoveCallbackFailure,
                  result, "remove-callback failure not reported");
            result.failureSnapshot = coordinator.Snapshot();
            result.hasFailureSnapshot = true;
            Check(coordinator.Snapshot().state == LifecycleState::Retiring &&
                  coordinator.Snapshot().vectorPresent &&
                  coordinator.Snapshot().totalStrong == 2,
                  result, "remove-callback failure did not retain safe owners");
            break;
        case FailurePoint::ReaderDrainFailure:
            Check(createError == LifecycleError::None,
                  result, "reader-drain setup failed");
            Check(Record(result, coordinator, "BeginRetire", FailureName(point), kAuthority, [&] {
                return coordinator.BeginRetire(port, kAuthority);
            }) == LifecycleError::ReaderDrainFailure,
                  result, "reader-drain failure not reported");
            result.failureSnapshot = coordinator.Snapshot();
            result.hasFailureSnapshot = true;
            Check(coordinator.Snapshot().state == LifecycleState::Retiring &&
                  coordinator.Snapshot().vectorPresent &&
                  coordinator.Snapshot().totalStrong == 2,
                  result, "reader-drain failure released a long-lived owner");
            break;
        case FailurePoint::StopOrShutdownEarlyReturn:
            Check(createError == LifecycleError::None,
                  result, "shutdown setup failed");
            Check(Record(result, coordinator, "BeginRetire", FailureName(point), kAuthority, [&] {
                return coordinator.BeginRetire(port, kAuthority);
            }) == LifecycleError::StopOrShutdownEarlyReturn,
                  result, "shutdown early return not reported");
            result.failureSnapshot = coordinator.Snapshot();
            result.hasFailureSnapshot = true;
            Check(coordinator.Snapshot().state == LifecycleState::Retiring &&
                  coordinator.Snapshot().totalStrong == 2,
                  result, "shutdown early return was not quarantined");
            port.ClearFailure();
            Check(Record(result, coordinator, "ResumeSameRetirePath", FailureName(point), kAuthority, [&] {
                return coordinator.BeginRetire(port, kAuthority);
            }) == LifecycleError::None,
                  result, "shutdown did not resume through retire path");
            Check(coordinator.RemoveAndDestroy(port, kAuthority) == LifecycleError::None,
                  result, "shutdown did not complete normal teardown");
            Check(coordinator.Snapshot().state == LifecycleState::Destroyed,
                  result, "shutdown completion did not destroy");
            break;
        case FailurePoint::FinalReleaseFailureOrWrongThread:
            Check(createError == LifecycleError::None,
                  result, "final-release setup failed");
            Check(coordinator.BeginRetire(port, kAuthority) == LifecycleError::None,
                  result, "final-release retire setup failed");
            Check(Record(result, coordinator, "RemoveAndDestroy", FailureName(point), kAuthority, [&] {
                return coordinator.RemoveAndDestroy(port, kAuthority);
            }) == LifecycleError::FinalReleaseFailureOrWrongThread,
                  result, "final-release failure not reported");
            result.failureSnapshot = coordinator.Snapshot();
            result.hasFailureSnapshot = true;
            Check(coordinator.Snapshot().state == LifecycleState::Unregistered &&
                  !coordinator.Snapshot().vectorPresent &&
                  coordinator.Snapshot().totalStrong == 1 &&
                  coordinator.Snapshot().objectDeleteCount == 0,
                  result, "final-release failure decremented or compensated");
            break;
        case FailurePoint::None:
            Check(false, result, "NONE entered failure runner");
            break;
    }

    result.finalSnapshot = coordinator.Snapshot();
    result.operationLog = port.OperationLog();
    Check(result.hasFailureSnapshot, result,
          "failure case omitted the point-of-failure snapshot");
    if (result.hasFailureSnapshot) {
        CheckFailureSnapshot(point, result.failureSnapshot, result);
    }
    CheckExactOperationLog(
        result.operationLog,
        ExpectedFailureLog(point),
        result,
        "failure path full operation log mismatch");
    Check(!port.contractViolation(), result, "fake port contract violation");
    Check(result.finalSnapshot.externalWeak == 0,
          result, "external weak reference appeared");
    Check(result.finalSnapshot.totalStrong <= kMaximumSimultaneousStrong,
          result, "strong reference ceiling exceeded");
    Check(port.maximumObservedStrong() <= kMaximumSimultaneousStrong,
          result, "port strong reference ceiling exceeded");
    if (result.finalSnapshot.state == LifecycleState::Destroyed) {
        Check(result.finalSnapshot.totalStrong == 0 &&
              result.finalSnapshot.implicitWeak == 0 &&
              !result.finalSnapshot.objectPresent &&
              !result.finalSnapshot.controlPresent &&
              !result.finalSnapshot.vectorPresent &&
              result.finalSnapshot.objectDeleteCount == 1 &&
              result.finalSnapshot.controlFreeCount == 1,
              result, "destroyed failure path retained or duplicated ownership");
    } else if (result.finalSnapshot.state == LifecycleState::Retiring) {
        Check(!result.finalSnapshot.admissionOpen &&
              result.finalSnapshot.objectPresent &&
              result.finalSnapshot.controlPresent &&
              result.finalSnapshot.vectorPresent &&
              result.finalSnapshot.totalStrong == 2 &&
              result.finalSnapshot.objectDeleteCount == 0 &&
              result.finalSnapshot.controlFreeCount == 0,
              result, "RETIRING quarantine did not retain exactly root+vector");
    } else if (result.finalSnapshot.state == LifecycleState::Unregistered) {
        Check(!result.finalSnapshot.admissionOpen &&
              result.finalSnapshot.objectPresent &&
              result.finalSnapshot.controlPresent &&
              !result.finalSnapshot.vectorPresent &&
              result.finalSnapshot.totalStrong == 1 &&
              result.finalSnapshot.objectDeleteCount == 0 &&
              result.finalSnapshot.controlFreeCount == 0,
              result, "UNREGISTERED quarantine did not retain exactly root");
    } else if (result.finalSnapshot.state == LifecycleState::Empty) {
        Check(result.finalSnapshot.totalStrong == 0 &&
              result.finalSnapshot.implicitWeak == 0 &&
              !result.finalSnapshot.objectPresent &&
              !result.finalSnapshot.controlPresent &&
              !result.finalSnapshot.vectorPresent,
              result, "EMPTY rollback retained ownership");
    } else {
        Check(false, result, "failure path ended in an unapproved state");
    }

    switch (point) {
        case FailurePoint::ObjectAllocationFailure:
            Check(ContainsOrdered(result.operationLog, {
                      "object-allocation-attempt",
                      "object-allocation-failed",
                  }), result, "object-allocation failure was self-reported only");
            break;
        case FailurePoint::ObjectConstructionFailure:
            Check(ContainsOrdered(result.operationLog, {
                      "object-partial-member-constructed",
                      "object-partial-member-destroyed-reverse-order",
                      "raw-object-freed",
                  }), result, "partial object cleanup order mismatch");
            break;
        case FailurePoint::ControlBlockAllocationNull:
            Check(ContainsOrdered(result.operationLog, {
                      "object-construction-complete",
                      "control-allocation-returned-null",
                      "constructed-object-destroyed-before-control-bind",
                      "raw-object-freed",
                  }), result, "null-control cleanup order mismatch");
            break;
        case FailurePoint::ControlBlockConstructionFailure:
            Check(ContainsOrdered(result.operationLog, {
                      "control-partial-member-constructed",
                      "control-partial-member-destroyed-reverse-order",
                      "raw-control-freed",
                      "constructed-object-destroyed-before-control-bind",
                      "raw-object-freed",
                  }), result, "partial control cleanup order mismatch");
            break;
        case FailurePoint::VectorInsertFailure:
            Check(ContainsOrdered(result.operationLog, {
                      "manager-vector-strong-acquired",
                      "vector-insert-failed-before-membership",
                      "rollback-strong-acquired",
                      "manager-vector-strong-released",
                      "rollback-strong-released",
                      "factory-root-strong-released",
                      "object-deleting-destructor-exactly-once",
                      "control-block-free-exactly-once",
                  }), result, "vector-insert rollback order mismatch");
            break;
        case FailurePoint::VectorGrowOrCopyFailure:
            Check(ContainsOrdered(result.operationLog, {
                      "vector-grow-destination-acquire-begin",
                      "rollback-strong-acquired",
                      "vector-grow-destination-acquired",
                      "rollback-strong-released",
                      "vector-grow-destination-undone",
                      "preexisting-vector-preserved-by-state-compare",
                      "rollback-strong-acquired",
                      "manager-vector-strong-released",
                      "rollback-strong-released",
                      "factory-root-strong-released",
                  }), result, "vector grow/copy undo order mismatch");
            Check(port.releaseCalls(StrongRefClass::RollbackTransientStrong) == 2,
                  result, "grow destination and coordinator rollback were not balanced");
            Check(port.vectorPreservedAfterGrowFailure() &&
                  port.existingVector() == preexistingVector &&
                  port.existingVectorSize() == preexistingVectorSize &&
                  port.growScratchSize() == 0,
                  result, "grow/copy failure changed the preexisting vector state");
            break;
        case FailurePoint::InsertNotificationFailure:
            Check(ContainsOrdered(result.operationLog, {
                      "manager-vector-membership-established",
                      "insert-callback-strong-acquired",
                      "insert-callback-strong-released",
                      "insert-notification-failed-after-membership",
                      "rollback-strong-acquired",
                      "remove-callback-strong-acquired",
                      "remove-callback-strong-released",
                      "manager-vector-membership-removed",
                      "manager-vector-strong-released",
                      "rollback-strong-released",
                      "factory-root-strong-released",
                  }), result, "insert-notification rollback order mismatch");
            break;
        case FailurePoint::CallbackReentryOrEscape:
            Check(ContainsOrdered(result.operationLog, {
                      "publication-confirmed",
                      "reader-and-callback-drain-confirmed",
                      "remove-callback-strong-acquired",
                      "remove-callback-reference-escaped",
                      "remove-callback-strong-released",
                      "remove-callback-strong-acquired",
                      "remove-callback-strong-released",
                      "manager-vector-membership-removed",
                      "factory-root-strong-released",
                  }), result, "remove callback escape/recovery order mismatch");
            break;
        case FailurePoint::PublicationFailure:
            Check(ContainsOrdered(result.operationLog, {
                      "manager-vector-membership-established",
                      "publication-confirmation-failed",
                      "rollback-strong-acquired",
                      "manager-vector-membership-removed",
                      "manager-vector-strong-released",
                      "rollback-strong-released",
                      "factory-root-strong-released",
                  }), result, "publication rollback order mismatch");
            break;
        case FailurePoint::RemoveCallbackFailure:
            Check(ContainsOrdered(result.operationLog, {
                      "reader-and-callback-drain-confirmed",
                      "remove-callback-strong-acquired",
                      "remove-callback-strong-released",
                      "remove-callback-failed-before-erase",
                  }), result, "remove callback quarantine order mismatch");
            break;
        case FailurePoint::ReaderDrainFailure:
            Check(ContainsOrdered(result.operationLog, {
                      "neutral-published-with-admission-closed",
                      "reader-drain-incomplete",
                  }), result, "reader drain failure was self-reported only");
            break;
        case FailurePoint::StopOrShutdownEarlyReturn:
            Check(ContainsOrdered(result.operationLog, {
                      "shutdown-early-return-after-admission-close",
                      "neutral-published-with-admission-closed",
                      "reader-and-callback-drain-confirmed",
                      "manager-vector-membership-removed",
                      "factory-root-strong-released",
                  }), result, "shutdown did not reuse the bounded retire path");
            break;
        case FailurePoint::FinalReleaseFailureOrWrongThread:
            Check(ContainsOrdered(result.operationLog, {
                      "manager-vector-membership-removed",
                      "manager-vector-strong-released",
                      "final-release-thread-rejected-before-decrement",
                  }), result, "wrong-thread final release was not stopped before decrement");
            Check(port.releaseCalls(StrongRefClass::FactoryLocalStrong) == 0,
                  result, "wrong-thread final release reached the ABI decrement");
            break;
        case FailurePoint::None:
            break;
    }
    return result;
}

void AppendQuoted(std::ostringstream& output, const std::string& value) {
    output << '"';
    for (const char character : value) {
        if (character == '"' || character == '\\') {
            output << '\\';
        }
        output << character;
    }
    output << '"';
}

void AppendSnapshot(std::ostringstream& output, const LifecycleSnapshot& snapshot) {
    output << "{\"state\":\"" << StateName(snapshot.state)
           << "\",\"firstError\":\"" << ErrorName(snapshot.firstError)
           << "\",\"admissionOpen\":" << (snapshot.admissionOpen ? "true" : "false")
           << ",\"publicationConfirmed\":"
           << (snapshot.publicationConfirmed ? "true" : "false")
           << ",\"drainConfirmed\":" << (snapshot.drainConfirmed ? "true" : "false")
           << ",\"vectorPresent\":" << (snapshot.vectorPresent ? "true" : "false")
           << ",\"objectPresent\":" << (snapshot.objectPresent ? "true" : "false")
           << ",\"controlPresent\":" << (snapshot.controlPresent ? "true" : "false")
           << ",\"finalReleaseStarted\":"
           << (snapshot.finalReleaseStarted ? "true" : "false")
           << ",\"liveReaders\":" << snapshot.liveReaders
           << ",\"liveCallbacks\":" << snapshot.liveCallbacks
           << ",\"totalStrong\":" << snapshot.totalStrong
           << ",\"implicitWeak\":" << snapshot.implicitWeak
           << ",\"externalWeak\":" << snapshot.externalWeak
           << ",\"strongByClass\":[";
    for (std::size_t index = 0; index < snapshot.strongByClass.size(); ++index) {
        if (index != 0) {
            output << ',';
        }
        output << snapshot.strongByClass[index];
    }
    output << ']'
           << ",\"objectDeleteCount\":" << snapshot.objectDeleteCount
           << ",\"controlFreeCount\":" << snapshot.controlFreeCount
           << ",\"rawObjectFreeCount\":" << snapshot.rawObjectFreeCount
           << ",\"rawControlFreeCount\":" << snapshot.rawControlFreeCount
           << ",\"transitionCount\":"
           << static_cast<unsigned int>(snapshot.transitionCount)
           << ",\"transitionTrace\":[";
    for (std::size_t index = 0;
         index < static_cast<std::size_t>(snapshot.transitionCount); ++index) {
        if (index != 0) {
            output << ',';
        }
        output << '"' << StateName(snapshot.transitionTrace[index]) << '"';
    }
    output << "]}";
}

std::string BuildReport(const std::vector<CaseResult>& cases) {
    std::ostringstream output;
    output << "{\"identity\":\"IsaacCsT0-VirtualDeviceLifecycleOfflineHarness-v1\""
           << ",\"status\":\"" << (gFailures.empty() ? "PASS" : "FAIL") << '"'
           << ",\"architecture\":\"Win32/x86\""
           << ",\"stateMachine\":[\"EMPTY\",\"OBJECT_ALLOCATED\",\"CONTROL_READY\","
              "\"REGISTERED\",\"RETIRING\",\"UNREGISTERED\",\"DESTROYED\"]"
           << ",\"strongReferenceClasses\":["
              "\"FACTORY_LOCAL_STRONG\",\"MANAGER_VECTOR_STRONG\","
              "\"INSERT_CALLBACK_TRANSIENT_STRONG\","
              "\"REMOVE_CALLBACK_TRANSIENT_STRONG\","
              "\"READER_TRANSIENT_STRONG\",\"ROLLBACK_TRANSIENT_STRONG\"]"
           << ",\"maximumSimultaneousStrong\":6,\"maximumExternalWeak\":0"
           << ",\"rollbackCoverage\":[";
    for (std::size_t index = 0; index < kFailurePoints.size(); ++index) {
        if (index != 0) {
            output << ',';
        }
        output << '"' << FailureName(kFailurePoints[index]) << '"';
    }
    output << "]"
           << ",\"caseCount\":" << cases.size()
           << ",\"assertionCount\":" << gAssertionCount
           << ",\"cases\":[";
    for (std::size_t caseIndex = 0; caseIndex < cases.size(); ++caseIndex) {
        if (caseIndex != 0) {
            output << ',';
        }
        const auto& result = cases[caseIndex];
        output << "{\"id\":";
        AppendQuoted(output, result.id);
        output << ",\"injectedFailure\":";
        AppendQuoted(output, result.injectedFailure);
        output << ",\"status\":\"" << (result.passed ? "PASS" : "FAIL")
               << "\",\"finalSnapshot\":";
        AppendSnapshot(output, result.finalSnapshot);
        output << ",\"failureSnapshot\":";
        if (result.hasFailureSnapshot) {
            AppendSnapshot(output, result.failureSnapshot);
        } else {
            output << "null";
        }
        output << ",\"events\":[";
        for (std::size_t eventIndex = 0;
             eventIndex < result.events.size(); ++eventIndex) {
            if (eventIndex != 0) {
                output << ',';
            }
            const auto& event = result.events[eventIndex];
            output << "{\"operation\":";
            AppendQuoted(output, event.operation);
            output << ",\"stateBefore\":\"" << StateName(event.before)
                   << "\",\"stateAfter\":\"" << StateName(event.after)
                   << "\",\"referenceClassDelta\":[";
            for (std::size_t deltaIndex = 0;
                 deltaIndex < event.classDelta.size(); ++deltaIndex) {
                if (deltaIndex != 0) {
                    output << ',';
                }
                output << event.classDelta[deltaIndex];
            }
            output << ']'
                   << ",\"threadClass\":";
            AppendQuoted(output, event.threadClass);
            output << ",\"threadToken\":";
            AppendQuoted(output, event.threadToken);
            output
                   << ",\"result\":\"" << ErrorName(event.result)
                   << "\",\"failurePoint\":";
            AppendQuoted(output, event.failurePoint);
            output << '}';
        }
        output << "],\"operationLog\":[";
        for (std::size_t operationIndex = 0;
             operationIndex < result.operationLog.size(); ++operationIndex) {
            if (operationIndex != 0) {
                output << ',';
            }
            AppendQuoted(output, result.operationLog[operationIndex]);
        }
        output << "]}";
    }
    output << "]"
           << ",\"failures\":[";
    for (std::size_t index = 0; index < gFailures.size(); ++index) {
        if (index != 0) {
            output << ',';
        }
        AppendQuoted(output, gFailures[index]);
    }
    output << "]}";
    return output.str();
}

}  // namespace

int main() {
    std::vector<CaseResult> cases;
    cases.reserve(16);
    cases.push_back(RunHappyPath());
    cases.push_back(RunLeaseAndBoundCase());
    cases.push_back(RunCallbackMoveCase());
    for (const auto failure : kFailurePoints) {
        cases.push_back(RunFailureCase(failure));
    }

    std::cout << BuildReport(cases) << '\n';
    return gFailures.empty() ? 0 : 1;
}
