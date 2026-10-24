#pragma once

#include <Windows.h>

#include <cstddef>
#include <cstdint>
#include <array>

namespace isaac_cs_t0::mode {

struct RelativeCallPatch {
    std::uint8_t* call = nullptr;
    std::uint8_t expected[5]{};
    void* replacement = nullptr;
    void* original = nullptr;
    std::int32_t original_displacement = 0;
    std::int32_t replacement_displacement = 0;
    bool installed = false;
};

struct PointerPatch {
    void** slot = nullptr;
    void* replacement = nullptr;
    void* original = nullptr;
    const wchar_t* expected_owner = nullptr;
    bool installed = false;
    // Optional frozen-target value.  Prepare validates it before suspension and Apply
    // validates the captured value again while every peer thread is suspended.
    void* expected_original = nullptr;
};

// Fixed, previously decoded x86 prologues only. No instruction decoder or
// relocation guess is made here. Published trampolines remain process resident.
struct EntryPatch {
    std::uint8_t* entry = nullptr;
    std::array<std::uint8_t, 16> expected{};
    std::array<std::uint8_t, 16> replacement_bytes{};
    std::uint32_t length = 0;
    void* replacement = nullptr;
    void* trampoline = nullptr;
    void** original_slot = nullptr;
    std::uint32_t replacement_guard_length = 0;
    bool installed = false;
};

// A type-erased view used when a catalog assigns one global ordinal space to
// relative-call and pointer patches. Exactly one patch pointer must be non-null.
// The descriptor does not own the patch; the caller must keep both alive for the
// complete install/restore lifetime.
struct OrderedPatchDescriptor {
    std::uint32_t ordinal = UINT32_MAX;
    RelativeCallPatch* relative = nullptr;
    PointerPatch* pointer = nullptr;
    EntryPatch* entry = nullptr;
};

enum class PatchFailureStage : std::uint32_t {
    None = 0,
    InvalidArguments = 1,
    PrepareRelative = 2,
    PreparePointer = 3,
    ThreadSnapshot = 4,
    ThreadFirst = 5,
    ThreadOpen = 6,
    ThreadSuspend = 7,
    ThreadCapacity = 8,
    ApplyRelative = 9,
    ApplyPointer = 10,
    RollbackPointer = 11,
    RollbackRelative = 12,
    PrepareEntry = 13,
    ApplyEntry = 14,
    RollbackEntry = 15,
    ThreadContext = 16,
    InstructionInEntry = 17,
};

struct PatchTransactionFailure {
    PatchFailureStage stage = PatchFailureStage::None;
    std::uint32_t index = UINT32_MAX;
    DWORD win32_error = ERROR_SUCCESS;
};

#if defined(ISAAC_CS_T0_TESTING)
enum class PatchTestFaultStage : std::uint32_t {
    None = 0,
    PrepareRelative,
    PreparePointer,
    ThreadSnapshot,
    ThreadSuspend,
    Protect,
    ApplyRelative,
    ApplyPointer,
    RollbackRelative,
    RollbackPointer,
    RestoreRelative,
    RestorePointer,
    ApplyAfterWrite,
};

void SetPatchTestFault(PatchTestFaultStage stage, std::uint32_t index) noexcept;
void SetPatchTestFaultPair(PatchTestFaultStage first_stage,
                           std::uint32_t first_index,
                           PatchTestFaultStage second_stage,
                           std::uint32_t second_index) noexcept;
enum class OrderedPatchTestOperation : std::uint32_t { Apply, Rollback, Restore };
using OrderedPatchTestObserver = void (*)(OrderedPatchTestOperation operation,
                                          std::uint32_t ordinal, bool succeeded) noexcept;
// Test-only, after each actual operation (including injected failures). The
// observer may inspect target bytes; it cannot authorize or alter the operation.
void SetOrderedPatchTestObserver(OrderedPatchTestObserver observer) noexcept;
#endif

void** FindIatSlot(HMODULE module, const char* import_name) noexcept;
bool ValidateIatCallsite(const std::uint8_t* call, void** slot) noexcept;
bool IsAddressOwnedBy(void* address, const wchar_t* expected_module) noexcept;

bool InstallPatchTransaction(RelativeCallPatch* relative, std::size_t relative_count,
                             PointerPatch* pointers, std::size_t pointer_count,
                             PatchTransactionFailure* failure = nullptr) noexcept;
bool RestorePatchTransaction(RelativeCallPatch* relative, std::size_t relative_count,
                             PointerPatch* pointers, std::size_t pointer_count) noexcept;

// Prepares every target before suspending peer threads, then applies descriptors
// in strictly increasing ordinal order. A failed apply is rolled back in the exact
// reverse of the successfully applied prefix. Restore likewise walks the supplied
// sequence in reverse ordinal order. The legacy grouped APIs above intentionally
// keep their existing relative-then-pointer behavior.
bool InstallOrderedPatchTransaction(OrderedPatchDescriptor* patches,
                                    std::size_t patch_count,
                                    PatchTransactionFailure* failure = nullptr) noexcept;
bool RestoreOrderedPatchTransaction(OrderedPatchDescriptor* patches,
                                    std::size_t patch_count) noexcept;

} // namespace isaac_cs_t0::mode
