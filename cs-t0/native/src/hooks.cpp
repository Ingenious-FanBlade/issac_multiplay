#include "isaac_cs_t0/hooks.hpp"

#include <TlHelp32.h>

#include <array>
#include <algorithm>
#include <cstring>
#include <limits>

namespace isaac_cs_t0::mode {
namespace {

#if defined(ISAAC_CS_T0_TESTING)
PatchTestFaultStage g_test_fault_stage = PatchTestFaultStage::None;
std::uint32_t g_test_fault_index = UINT32_MAX;
PatchTestFaultStage g_test_fault_stage2 = PatchTestFaultStage::None;
std::uint32_t g_test_fault_index2 = UINT32_MAX;
OrderedPatchTestObserver g_ordered_test_observer = nullptr;

bool TestFault(PatchTestFaultStage stage, std::uint32_t index) noexcept
{
    return (g_test_fault_stage == stage && g_test_fault_index == index) ||
           (g_test_fault_stage2 == stage && g_test_fault_index2 == index);
}
#endif

class SuspendedThreads final {
public:
    bool SuspendAll(PatchTransactionFailure* failure, bool contexts = false) noexcept
    {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (snapshot == INVALID_HANDLE_VALUE)
        {
            SetFailure(failure, PatchFailureStage::ThreadSnapshot, UINT32_MAX,
                       GetLastError());
            return false;
        }
        THREADENTRY32 entry{sizeof(entry)};
        bool success = Thread32First(snapshot, &entry) != FALSE;
        if (!success)
            SetFailure(failure, PatchFailureStage::ThreadFirst, UINT32_MAX,
                       GetLastError());
        while (success)
        {
            if (entry.th32OwnerProcessID == GetCurrentProcessId() &&
                entry.th32ThreadID != GetCurrentThreadId())
            {
                HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME |
                    (contexts ? THREAD_GET_CONTEXT : 0), FALSE, entry.th32ThreadID);
                if (!thread)
                {
                    SetFailure(failure, PatchFailureStage::ThreadOpen,
                               entry.th32ThreadID, GetLastError());
                    success = false;
                    break;
                }
                if (SuspendThread(thread) == static_cast<DWORD>(-1))
                {
                    const DWORD error = GetLastError();
                    CloseHandle(thread);
                    SetFailure(failure, PatchFailureStage::ThreadSuspend,
                               entry.th32ThreadID, error);
                    success = false;
                    break;
                }
                if (thread_count_ == threads_.size())
                {
                    ResumeThread(thread);
                    CloseHandle(thread);
                    SetFailure(failure, PatchFailureStage::ThreadCapacity,
                               entry.th32ThreadID, ERROR_INSUFFICIENT_BUFFER);
                    success = false;
                    break;
                }
                threads_[thread_count_++] = thread;
            }
            if (!Thread32Next(snapshot, &entry))
                break;
        }
        CloseHandle(snapshot);
        if (!success)
            ResumeAll();
        return success;
    }

    ~SuspendedThreads() { ResumeAll(); }

    bool OutsideEntries(const OrderedPatchDescriptor* patches, std::size_t count,
                        PatchTransactionFailure* failure) noexcept
    {
        for (std::size_t i=0;i<thread_count_;++i) {
            CONTEXT context{};context.ContextFlags=CONTEXT_CONTROL;
            if (!GetThreadContext(threads_[i],&context)) {
                SetFailure(failure,PatchFailureStage::ThreadContext,
                    static_cast<std::uint32_t>(i),GetLastError());return false;
            }
            for (std::size_t j=0;j<count;++j) if (patches[j].entry) {
                const auto& patch=*patches[j].entry;
                const auto address=reinterpret_cast<std::uintptr_t>(patch.entry);
                if (context.Eip>=address && context.Eip-address<patch.length) {
                    SetFailure(failure,PatchFailureStage::InstructionInEntry,
                        patches[j].ordinal,ERROR_BUSY);return false;
                }
                const auto replacement=reinterpret_cast<std::uintptr_t>(patch.replacement);
                if(patch.installed && patch.replacement_guard_length && context.Eip>=replacement &&
                   context.Eip-replacement<patch.replacement_guard_length) {
                    SetFailure(failure,PatchFailureStage::InstructionInEntry,
                        patches[j].ordinal,ERROR_BUSY);return false;
                }
            }
        }
        return true;
    }

private:
    static void SetFailure(PatchTransactionFailure* failure, PatchFailureStage stage,
                           std::uint32_t index, DWORD error) noexcept
    {
        if (failure)
            *failure = {stage, index, error};
    }

    void ResumeAll() noexcept
    {
        while (thread_count_)
        {
            HANDLE thread = threads_[--thread_count_];
            ResumeThread(thread);
            CloseHandle(thread);
        }
    }

    std::array<HANDLE, 256> threads_{};
    std::size_t thread_count_ = 0;
};

void** FindIatSlotImpl(HMODULE module, const char* import_name) noexcept
{
    if (!module || !import_name)
        return nullptr;
    auto* base = reinterpret_cast<std::uint8_t*>(module);
    __try
    {
        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE)
            return nullptr;
        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE ||
            nt->FileHeader.Machine != IMAGE_FILE_MACHINE_I386)
            return nullptr;
        const auto directory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
        if (!directory.VirtualAddress)
            return nullptr;
        auto* descriptor = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(base + directory.VirtualAddress);
        for (; descriptor->Name; ++descriptor)
        {
            auto* original = descriptor->OriginalFirstThunk
                                 ? reinterpret_cast<IMAGE_THUNK_DATA32*>(base + descriptor->OriginalFirstThunk)
                                 : reinterpret_cast<IMAGE_THUNK_DATA32*>(base + descriptor->FirstThunk);
            auto* thunk = reinterpret_cast<IMAGE_THUNK_DATA32*>(base + descriptor->FirstThunk);
            for (; original->u1.AddressOfData; ++original, ++thunk)
            {
                if (IMAGE_SNAP_BY_ORDINAL32(original->u1.Ordinal))
                    continue;
                const auto* entry =
                    reinterpret_cast<const IMAGE_IMPORT_BY_NAME*>(base + original->u1.AddressOfData);
                if (std::strcmp(reinterpret_cast<const char*>(entry->Name), import_name) == 0)
                    return reinterpret_cast<void**>(&thunk->u1.Function);
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return nullptr;
    }
    return nullptr;
}

bool ProtectWrite(void* destination, const void* source, std::size_t size,
                  DWORD requested_protection, DWORD* failure_error = nullptr) noexcept
{
    if (failure_error)
        *failure_error = ERROR_SUCCESS;
    DWORD old_protection = 0;
    if (!VirtualProtect(destination, size, requested_protection, &old_protection))
    {
        if (failure_error)
            *failure_error = GetLastError();
        return false;
    }
    std::memcpy(destination, source, size);
    DWORD ignored = 0;
    const bool protection_restored =
        VirtualProtect(destination, size, old_protection, &ignored) != FALSE;
    const DWORD protection_error = protection_restored ? ERROR_SUCCESS : GetLastError();
    FlushInstructionCache(GetCurrentProcess(), destination, size);
    const bool bytes_match = std::memcmp(destination, source, size) == 0;
    if (failure_error && (!protection_restored || !bytes_match))
        *failure_error = protection_restored ? ERROR_WRITE_FAULT : protection_error;
    return protection_restored && bytes_match;
}

bool Prepare(RelativeCallPatch& patch) noexcept
{
    if (!patch.call || !patch.replacement || std::memcmp(patch.call, patch.expected, 5) != 0 ||
        patch.installed)
        return false;
    patch.original_displacement = *reinterpret_cast<const std::int32_t*>(patch.call + 1);
    patch.original = patch.call + 5 + patch.original_displacement;
    const auto next = static_cast<std::int64_t>(reinterpret_cast<std::uintptr_t>(patch.call + 5));
    const auto replacement =
        static_cast<std::int64_t>(reinterpret_cast<std::uintptr_t>(patch.replacement));
    const auto displacement = replacement - next;
    if (displacement < (std::numeric_limits<std::int32_t>::min)() ||
        displacement > (std::numeric_limits<std::int32_t>::max)())
        return false;
    patch.replacement_displacement = static_cast<std::int32_t>(displacement);
    return true;
}

bool Prepare(PointerPatch& patch) noexcept
{
    if (!patch.slot || !patch.replacement || patch.installed)
        return false;
    patch.original = *patch.slot;
    return patch.original && patch.original != patch.replacement &&
           (!patch.expected_original || patch.original == patch.expected_original) &&
           (!patch.expected_owner || IsAddressOwnedBy(patch.original, patch.expected_owner));
}

bool Apply(RelativeCallPatch& patch, DWORD* failure_error = nullptr) noexcept
{
    if (std::memcmp(patch.call, patch.expected, sizeof(patch.expected)) != 0)
    {
        if (failure_error)
            *failure_error = ERROR_INVALID_DATA;
        return false;
    }
    const bool complete = ProtectWrite(patch.call + 1, &patch.replacement_displacement,
                                       sizeof(patch.replacement_displacement),
                                       PAGE_EXECUTE_READWRITE, failure_error);
    patch.installed =
        *reinterpret_cast<const std::int32_t*>(patch.call + 1) == patch.replacement_displacement;
    return complete && patch.installed;
}

bool Apply(PointerPatch& patch, DWORD* failure_error = nullptr) noexcept
{
    if (*patch.slot != patch.original ||
        (patch.expected_original && patch.original != patch.expected_original))
    {
        if (failure_error)
            *failure_error = ERROR_INVALID_DATA;
        return false;
    }
    const bool complete =
        ProtectWrite(patch.slot, &patch.replacement, sizeof(patch.replacement), PAGE_READWRITE,
                     failure_error);
    patch.installed = *patch.slot == patch.replacement;
    return complete && patch.installed;
}

bool Restore(RelativeCallPatch& patch, DWORD* failure_error = nullptr) noexcept
{
    if (!patch.installed)
        return true;
    if (*reinterpret_cast<const std::int32_t*>(patch.call + 1) !=
        patch.replacement_displacement)
    {
        if (failure_error)
            *failure_error = ERROR_INVALID_DATA;
        return false;
    }
    if (!ProtectWrite(patch.call + 1, &patch.original_displacement,
                      sizeof(patch.original_displacement), PAGE_EXECUTE_READWRITE,
                      failure_error))
        return false;
    patch.installed = false;
    return true;
}

bool Restore(PointerPatch& patch, DWORD* failure_error = nullptr) noexcept
{
    if (!patch.installed)
        return true;
    if (*patch.slot != patch.replacement)
    {
        if (failure_error)
            *failure_error = ERROR_INVALID_DATA;
        return false;
    }
    if (!ProtectWrite(patch.slot, &patch.original, sizeof(patch.original), PAGE_READWRITE,
                      failure_error))
        return false;
    patch.installed = false;
    return true;
}

bool RelativeJump(std::uint8_t* destination, const void* next, const void* target) noexcept
{
    const auto delta=static_cast<std::int64_t>(reinterpret_cast<std::uintptr_t>(target))-
        static_cast<std::int64_t>(reinterpret_cast<std::uintptr_t>(next));
    if (delta<INT32_MIN || delta>INT32_MAX) return false;
    const auto value=static_cast<std::int32_t>(delta);
    destination[0]=0xE9;std::memcpy(destination+1,&value,4);return true;
}
bool Prepare(EntryPatch& patch) noexcept
{
    if (!patch.entry || !patch.replacement || !patch.original_slot || patch.installed ||
        patch.length<5 || patch.length>patch.expected.size() ||
        std::memcmp(patch.entry,patch.expected.data(),patch.length)) return false;
    patch.replacement_bytes.fill(0x90);
    if (!RelativeJump(patch.replacement_bytes.data(),patch.entry+5,patch.replacement)) return false;
    if (!patch.trampoline) {
        auto* memory=static_cast<std::uint8_t*>(VirtualAlloc(nullptr,32,
            MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
        if (!memory) return false;
        std::memcpy(memory,patch.expected.data(),patch.length);
        if (!RelativeJump(memory+patch.length,memory+patch.length+5,patch.entry+patch.length)) {
            VirtualFree(memory,0,MEM_RELEASE);return false;
        }
        DWORD old=0;
        if (!VirtualProtect(memory,32,PAGE_EXECUTE_READ,&old)) {
            VirtualFree(memory,0,MEM_RELEASE);return false;
        }
        FlushInstructionCache(GetCurrentProcess(),memory,32);
        patch.trampoline=memory;
    }
    *patch.original_slot=patch.trampoline;
    return true;
}
bool Apply(EntryPatch& patch,DWORD* error=nullptr) noexcept
{
    if (std::memcmp(patch.entry,patch.expected.data(),patch.length)) {
        if(error)*error=ERROR_INVALID_DATA;return false;
    }
    const bool complete=ProtectWrite(patch.entry,patch.replacement_bytes.data(),
        patch.length,PAGE_EXECUTE_READWRITE,error);
    patch.installed=std::memcmp(patch.entry,patch.replacement_bytes.data(),patch.length)==0;
    return complete && patch.installed;
}
bool Restore(EntryPatch& patch,DWORD* error=nullptr) noexcept
{
    if(!patch.installed)return true;
    if(std::memcmp(patch.entry,patch.replacement_bytes.data(),patch.length)) {
        if(error)*error=ERROR_INVALID_DATA;return false;
    }
    if(!ProtectWrite(patch.entry,patch.expected.data(),patch.length,PAGE_EXECUTE_READWRITE,error))return false;
    patch.installed=false;return true;
}
bool ValidDescriptor(const OrderedPatchDescriptor& descriptor) noexcept
{
    return (descriptor.relative?1:0)+(descriptor.pointer?1:0)+(descriptor.entry?1:0)==1;
}
bool PrepareDescriptor(OrderedPatchDescriptor& d) noexcept
{return d.entry?Prepare(*d.entry):(d.relative?Prepare(*d.relative):Prepare(*d.pointer));}
bool ApplyDescriptor(OrderedPatchDescriptor& d,DWORD* error) noexcept
{return d.entry?Apply(*d.entry,error):(d.relative?Apply(*d.relative,error):Apply(*d.pointer,error));}
bool RestoreDescriptor(OrderedPatchDescriptor& d,DWORD* error=nullptr) noexcept
{return d.entry?Restore(*d.entry,error):(d.relative?Restore(*d.relative,error):Restore(*d.pointer,error));}

} // namespace

void** FindIatSlot(HMODULE module, const char* import_name) noexcept
{
    return FindIatSlotImpl(module, import_name);
}

bool ValidateIatCallsite(const std::uint8_t* call, void** slot) noexcept
{
    if (!call || !slot)
        return false;
    __try
    {
        return call[0] == 0xFF && call[1] == 0x15 &&
               *reinterpret_cast<void* const*>(call + 2) == slot;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool IsAddressOwnedBy(void* address, const wchar_t* expected_module) noexcept
{
    if (!address || !expected_module)
        return false;
    HMODULE owner = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCWSTR>(address), &owner))
        return false;
    wchar_t path[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(owner, path, _countof(path));
    if (!length || length >= _countof(path))
        return false;
    const wchar_t* name = wcsrchr(path, L'\\');
    name = name ? name + 1 : path;
    return _wcsicmp(name, expected_module) == 0;
}

bool InstallPatchTransaction(RelativeCallPatch* relative, std::size_t relative_count,
                             PointerPatch* pointers, std::size_t pointer_count,
                             PatchTransactionFailure* failure) noexcept
{
    if (failure)
        *failure = {};
    if ((!relative && relative_count) || (!pointers && pointer_count))
    {
        if (failure)
            *failure = {PatchFailureStage::InvalidArguments, UINT32_MAX,
                        ERROR_INVALID_PARAMETER};
        return false;
    }
    for (std::size_t index = 0; index < relative_count; ++index)
    {
#if defined(ISAAC_CS_T0_TESTING)
        if (TestFault(PatchTestFaultStage::PrepareRelative,
                      static_cast<std::uint32_t>(index)))
        {
            if (failure)
                *failure = {PatchFailureStage::PrepareRelative,
                            static_cast<std::uint32_t>(index), ERROR_ACCESS_DENIED};
            return false;
        }
#endif
        if (!Prepare(relative[index]))
        {
            if (failure)
                *failure = {PatchFailureStage::PrepareRelative,
                            static_cast<std::uint32_t>(index), ERROR_INVALID_DATA};
            return false;
        }
    }
    for (std::size_t index = 0; index < pointer_count; ++index)
    {
#if defined(ISAAC_CS_T0_TESTING)
        if (TestFault(PatchTestFaultStage::PreparePointer,
                      static_cast<std::uint32_t>(index)))
        {
            if (failure)
                *failure = {PatchFailureStage::PreparePointer,
                            static_cast<std::uint32_t>(index), ERROR_ACCESS_DENIED};
            return false;
        }
#endif
        if (!Prepare(pointers[index]))
        {
            if (failure)
                *failure = {PatchFailureStage::PreparePointer,
                            static_cast<std::uint32_t>(index), ERROR_INVALID_DATA};
            return false;
        }
    }
#if defined(ISAAC_CS_T0_TESTING)
    if (TestFault(PatchTestFaultStage::ThreadSnapshot, 0))
    {
        if (failure)
            *failure = {PatchFailureStage::ThreadSnapshot, 0, ERROR_ACCESS_DENIED};
        return false;
    }
#endif
    SuspendedThreads threads;
    if (!threads.SuspendAll(failure))
        return false;
#if defined(ISAAC_CS_T0_TESTING)
    if (TestFault(PatchTestFaultStage::ThreadSuspend, 0))
    {
        if (failure)
            *failure = {PatchFailureStage::ThreadSuspend, 0, ERROR_ACCESS_DENIED};
        return false;
    }
#endif
    std::size_t relative_installed = 0;
    std::size_t pointers_installed = 0;
    for (; relative_installed < relative_count; ++relative_installed)
    {
        DWORD apply_error = ERROR_SUCCESS;
#if defined(ISAAC_CS_T0_TESTING)
        if (TestFault(PatchTestFaultStage::ApplyRelative,
                      static_cast<std::uint32_t>(relative_installed)) ||
            TestFault(PatchTestFaultStage::Protect,
                      static_cast<std::uint32_t>(relative_installed)))
        {
            apply_error = ERROR_ACCESS_DENIED;
            if (failure)
                *failure = {PatchFailureStage::ApplyRelative,
                            static_cast<std::uint32_t>(relative_installed),
                            apply_error};
            break;
        }
#endif
        if (!Apply(relative[relative_installed], &apply_error))
        {
            if (failure)
                *failure = {PatchFailureStage::ApplyRelative,
                            static_cast<std::uint32_t>(relative_installed), apply_error};
            if (relative[relative_installed].installed)
                ++relative_installed;
            break;
        }
    }
    if (relative_installed == relative_count)
    {
        for (; pointers_installed < pointer_count; ++pointers_installed)
        {
            DWORD apply_error = ERROR_SUCCESS;
#if defined(ISAAC_CS_T0_TESTING)
            if (TestFault(PatchTestFaultStage::ApplyPointer,
                          static_cast<std::uint32_t>(pointers_installed)) ||
                TestFault(PatchTestFaultStage::Protect,
                          static_cast<std::uint32_t>(relative_count +
                                                     pointers_installed)))
            {
                apply_error = ERROR_ACCESS_DENIED;
                if (failure)
                    *failure = {PatchFailureStage::ApplyPointer,
                                static_cast<std::uint32_t>(pointers_installed),
                                apply_error};
                break;
            }
#endif
            if (!Apply(pointers[pointers_installed], &apply_error))
            {
                if (failure)
                    *failure = {PatchFailureStage::ApplyPointer,
                                static_cast<std::uint32_t>(pointers_installed), apply_error};
                if (pointers[pointers_installed].installed)
                    ++pointers_installed;
                break;
            }
        }
    }
    const bool complete = relative_installed == relative_count &&
                          pointers_installed == pointer_count;
    if (complete)
        return true;
    bool rollback_failure = false;
    while (pointers_installed)
    {
        const std::size_t index = --pointers_installed;
        DWORD restore_error = ERROR_SUCCESS;
#if defined(ISAAC_CS_T0_TESTING)
        const bool injected = TestFault(PatchTestFaultStage::RollbackPointer,
                                        static_cast<std::uint32_t>(index));
#else
        const bool injected = false;
#endif
        if ((injected || !Restore(pointers[index], &restore_error)) &&
            !rollback_failure)
        {
            rollback_failure = true;
            if (failure)
                *failure = {PatchFailureStage::RollbackPointer,
                            static_cast<std::uint32_t>(index), restore_error};
        }
    }
    while (relative_installed)
    {
        const std::size_t index = --relative_installed;
        DWORD restore_error = ERROR_SUCCESS;
#if defined(ISAAC_CS_T0_TESTING)
        const bool injected = TestFault(PatchTestFaultStage::RollbackRelative,
                                        static_cast<std::uint32_t>(index));
#else
        const bool injected = false;
#endif
        if ((injected || !Restore(relative[index], &restore_error)) &&
            !rollback_failure)
        {
            rollback_failure = true;
            if (failure)
                *failure = {PatchFailureStage::RollbackRelative,
                            static_cast<std::uint32_t>(index), restore_error};
        }
    }
    return false;
}

bool RestorePatchTransaction(RelativeCallPatch* relative, std::size_t relative_count,
                             PointerPatch* pointers, std::size_t pointer_count) noexcept
{
    if ((!relative && relative_count) || (!pointers && pointer_count))
        return false;
    SuspendedThreads threads;
    if (!threads.SuspendAll(nullptr))
        return false;
    bool restored = true;
    for (std::size_t index = pointer_count; index > 0; --index)
    {
#if defined(ISAAC_CS_T0_TESTING)
        if (TestFault(PatchTestFaultStage::RestorePointer,
                      static_cast<std::uint32_t>(index - 1)))
            restored = false;
        else
#endif
            restored = Restore(pointers[index - 1]) && restored;
    }
    for (std::size_t index = relative_count; index > 0; --index)
    {
#if defined(ISAAC_CS_T0_TESTING)
        if (TestFault(PatchTestFaultStage::RestoreRelative,
                      static_cast<std::uint32_t>(index - 1)))
            restored = false;
        else
#endif
            restored = Restore(relative[index - 1]) && restored;
    }
    return restored;
}

bool InstallOrderedPatchTransaction(OrderedPatchDescriptor* patches,
                                    std::size_t patch_count,
                                    PatchTransactionFailure* failure) noexcept
{
    if (failure)
        *failure = {};
    if ((!patches && patch_count) || patch_count > UINT32_MAX)
    {
        if (failure)
            *failure = {PatchFailureStage::InvalidArguments, UINT32_MAX,
                        ERROR_INVALID_PARAMETER};
        return false;
    }
    const std::uint32_t first_ordinal = patch_count ? patches[0].ordinal : 0;
    for (std::size_t index = 0; index < patch_count; ++index)
    {
        auto& descriptor = patches[index];
        if (descriptor.ordinal != first_ordinal + index ||
            !ValidDescriptor(descriptor))
        {
            if (failure)
                *failure = {PatchFailureStage::InvalidArguments,
                            static_cast<std::uint32_t>(index),
                            ERROR_INVALID_PARAMETER};
            return false;
        }
#if defined(ISAAC_CS_T0_TESTING)
        const PatchTestFaultStage prepare_stage = descriptor.relative
                                                     ? PatchTestFaultStage::PrepareRelative
                                                     : PatchTestFaultStage::PreparePointer;
        if (TestFault(prepare_stage, static_cast<std::uint32_t>(index)))
        {
            if (failure)
                *failure = {descriptor.relative ? PatchFailureStage::PrepareRelative
                                                : PatchFailureStage::PreparePointer,
                            static_cast<std::uint32_t>(index), ERROR_ACCESS_DENIED};
            return false;
        }
#endif
        const bool prepared = PrepareDescriptor(descriptor);
        if (!prepared)
        {
            if (failure)
                *failure = {descriptor.entry ? PatchFailureStage::PrepareEntry : descriptor.relative ? PatchFailureStage::PrepareRelative
                                                : PatchFailureStage::PreparePointer,
                            static_cast<std::uint32_t>(index), ERROR_INVALID_DATA};
            return false;
        }
    }
#if defined(ISAAC_CS_T0_TESTING)
    if (TestFault(PatchTestFaultStage::ThreadSnapshot, 0))
    {
        if (failure)
            *failure = {PatchFailureStage::ThreadSnapshot, 0, ERROR_ACCESS_DENIED};
        return false;
    }
#endif
    SuspendedThreads threads;
    const bool entries=std::any_of(patches,patches+patch_count,
        [](const auto& d){return d.entry!=nullptr;});
    if (!threads.SuspendAll(failure,entries) ||
        (entries && !threads.OutsideEntries(patches,patch_count,failure)))
        return false;
#if defined(ISAAC_CS_T0_TESTING)
    if (TestFault(PatchTestFaultStage::ThreadSuspend, 0))
    {
        if (failure)
            *failure = {PatchFailureStage::ThreadSuspend, 0, ERROR_ACCESS_DENIED};
        return false;
    }
#endif
    std::size_t applied = 0;
    bool apply_failed = false;
    for (; applied < patch_count; ++applied)
    {
        auto& descriptor = patches[applied];
        DWORD apply_error = ERROR_SUCCESS;
#if defined(ISAAC_CS_T0_TESTING)
        const PatchTestFaultStage apply_stage = descriptor.relative
                                                   ? PatchTestFaultStage::ApplyRelative
                                                   : PatchTestFaultStage::ApplyPointer;
        const bool injected =
            TestFault(apply_stage, static_cast<std::uint32_t>(applied)) ||
            TestFault(PatchTestFaultStage::Protect,
                      static_cast<std::uint32_t>(applied));
#else
        const bool injected = false;
#endif
        bool complete = !injected && ApplyDescriptor(descriptor,&apply_error);
#if defined(ISAAC_CS_T0_TESTING)
        if (complete && TestFault(PatchTestFaultStage::ApplyAfterWrite, descriptor.ordinal))
        {
            complete = false;
            apply_error = ERROR_ACCESS_DENIED;
        }
        if (g_ordered_test_observer)
            g_ordered_test_observer(OrderedPatchTestOperation::Apply, descriptor.ordinal, complete);
#endif
        if (complete)
            continue;
        apply_failed = true;
        if (injected)
            apply_error = ERROR_ACCESS_DENIED;
        const bool partially_installed = descriptor.entry ? descriptor.entry->installed : descriptor.relative
                                             ? descriptor.relative->installed
                                             : descriptor.pointer->installed;
        if (partially_installed)
            ++applied;
        if (failure)
            *failure = {descriptor.entry ? PatchFailureStage::ApplyEntry : descriptor.relative ? PatchFailureStage::ApplyRelative
                                            : PatchFailureStage::ApplyPointer,
                        static_cast<std::uint32_t>(descriptor.ordinal), apply_error};
        break;
    }
    if (!apply_failed && applied == patch_count)
        return true;

    bool rollback_failure = false;
    while (applied)
    {
        auto& descriptor = patches[--applied];
        DWORD restore_error = ERROR_SUCCESS;
#if defined(ISAAC_CS_T0_TESTING)
        const PatchTestFaultStage rollback_stage = descriptor.relative
                                                      ? PatchTestFaultStage::RollbackRelative
                                                      : PatchTestFaultStage::RollbackPointer;
        const bool injected =
            TestFault(rollback_stage, static_cast<std::uint32_t>(descriptor.ordinal));
#else
        const bool injected = false;
#endif
        const bool restored = !injected && RestoreDescriptor(descriptor,&restore_error);
#if defined(ISAAC_CS_T0_TESTING)
        if (g_ordered_test_observer)
            g_ordered_test_observer(OrderedPatchTestOperation::Rollback, descriptor.ordinal, restored);
#endif
        if (!restored && !rollback_failure)
        {
            rollback_failure = true;
            if (failure)
                *failure = {descriptor.entry ? PatchFailureStage::RollbackEntry : descriptor.relative ? PatchFailureStage::RollbackRelative
                                                : PatchFailureStage::RollbackPointer,
                            static_cast<std::uint32_t>(descriptor.ordinal), restore_error};
        }
    }
    return false;
}

bool RestoreOrderedPatchTransaction(OrderedPatchDescriptor* patches,
                                    std::size_t patch_count) noexcept
{
    if ((!patches && patch_count) || patch_count > UINT32_MAX)
        return false;
    const std::uint32_t first_ordinal = patch_count ? patches[0].ordinal : 0;
    for (std::size_t index = 0; index < patch_count; ++index)
    {
        const auto& descriptor = patches[index];
        if (descriptor.ordinal != first_ordinal + index ||
            !ValidDescriptor(descriptor))
            return false;
    }
    SuspendedThreads threads;
    const bool entries=std::any_of(patches,patches+patch_count,
        [](const auto& d){return d.entry!=nullptr;});
    if (!threads.SuspendAll(nullptr,entries) ||
        (entries && !threads.OutsideEntries(patches,patch_count,nullptr)))
        return false;
    bool restored = true;
    for (std::size_t index = patch_count; index > 0; --index)
    {
        auto& descriptor = patches[index - 1];
#if defined(ISAAC_CS_T0_TESTING)
        const PatchTestFaultStage restore_stage = descriptor.relative
                                                     ? PatchTestFaultStage::RestoreRelative
                                                     : PatchTestFaultStage::RestorePointer;
        if (TestFault(restore_stage,
                      static_cast<std::uint32_t>(descriptor.ordinal)))
        {
            restored = false;
            if (g_ordered_test_observer)
                g_ordered_test_observer(OrderedPatchTestOperation::Restore, descriptor.ordinal, false);
            continue;
        }
#endif
        const bool current_restored = RestoreDescriptor(descriptor);
#if defined(ISAAC_CS_T0_TESTING)
        if (g_ordered_test_observer)
            g_ordered_test_observer(OrderedPatchTestOperation::Restore, descriptor.ordinal, current_restored);
#endif
        restored = current_restored && restored;
    }
    return restored;
}

#if defined(ISAAC_CS_T0_TESTING)
void SetOrderedPatchTestObserver(OrderedPatchTestObserver observer) noexcept
{
    g_ordered_test_observer = observer;
}

void SetPatchTestFault(PatchTestFaultStage stage, std::uint32_t index) noexcept
{
    g_test_fault_stage = stage;
    g_test_fault_index = index;
    g_test_fault_stage2 = PatchTestFaultStage::None;
    g_test_fault_index2 = UINT32_MAX;
}

void SetPatchTestFaultPair(PatchTestFaultStage first_stage,
                           std::uint32_t first_index,
                           PatchTestFaultStage second_stage,
                           std::uint32_t second_index) noexcept
{
    g_test_fault_stage = first_stage;
    g_test_fault_index = first_index;
    g_test_fault_stage2 = second_stage;
    g_test_fault_index2 = second_index;
}
#endif

} // namespace isaac_cs_t0::mode
