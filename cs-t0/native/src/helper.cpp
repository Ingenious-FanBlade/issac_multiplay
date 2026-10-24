#include <Windows.h>
#include <TlHelp32.h>
#include <Shellapi.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <array>
#include <string>

#include "isaac_cs_t0/helper_operations.hpp"

namespace {

constexpr wchar_t kTargetName[] = L"isaac-ng.exe";
constexpr wchar_t kDllName[] = L"IsaacCsT0.Native.dll";

class Handle final {
public:
    explicit Handle(HANDLE handle = nullptr) noexcept : handle_(handle) {}
    ~Handle()
    {
        if (handle_ && handle_ != INVALID_HANDLE_VALUE)
            CloseHandle(handle_);
    }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    [[nodiscard]] HANDLE Get() const noexcept { return handle_; }
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return handle_ && handle_ != INVALID_HANDLE_VALUE;
    }

private:
    HANDLE handle_;
};

class Module final {
public:
    explicit Module(HMODULE module = nullptr) noexcept : module_(module) {}
    ~Module()
    {
        if (module_)
            FreeLibrary(module_);
    }
    [[nodiscard]] HMODULE Get() const noexcept { return module_; }

private:
    HMODULE module_;
};

struct RemoteModule {
    std::uintptr_t base = 0;
    std::wstring path;
};

enum class FindResult {
    Error,
    Missing,
    Found,
};

std::wstring BaseName(const std::wstring& path)
{
    const auto position = path.find_last_of(L"\\/");
    return position == std::wstring::npos ? path : path.substr(position + 1);
}

bool AbsolutePath(const wchar_t* input, std::wstring& output)
{
    if (!input || !*input)
        return false;
    const DWORD required = GetFullPathNameW(input, 0, nullptr, nullptr);
    if (!required)
        return false;
    std::wstring buffer(required, L'\0');
    const DWORD written = GetFullPathNameW(input, required, buffer.data(), nullptr);
    if (!written || written >= required)
        return false;
    buffer.resize(written);
    output = buffer;
    return true;
}

FindResult FindRemoteModule(DWORD pid, const wchar_t* name, RemoteModule& output)
{
    output = {};
    Handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid));
    if (!snapshot)
        return FindResult::Error;
    MODULEENTRY32W entry{sizeof(entry)};
    if (!Module32FirstW(snapshot.Get(), &entry))
        return GetLastError() == ERROR_NO_MORE_FILES ? FindResult::Missing : FindResult::Error;
    do
    {
        if (_wcsicmp(entry.szModule, name) == 0)
        {
            output.base = reinterpret_cast<std::uintptr_t>(entry.modBaseAddr);
            output.path = entry.szExePath;
            return FindResult::Found;
        }
    } while (Module32NextW(snapshot.Get(), &entry));
    return GetLastError() == ERROR_NO_MORE_FILES ? FindResult::Missing : FindResult::Error;
}

bool ValidateProcess(HANDLE process)
{
    wchar_t path[32768]{};
    DWORD length = _countof(path);
    if (!QueryFullProcessImageNameW(process, 0, path, &length) ||
        _wcsicmp(BaseName(path).c_str(), kTargetName) != 0)
        return false;
    USHORT process_machine = 0;
    USHORT native_machine = 0;
    if (!IsWow64Process2(process, &process_machine, &native_machine))
        return false;
    return process_machine == IMAGE_FILE_MACHINE_I386 ||
           (process_machine == IMAGE_FILE_MACHINE_UNKNOWN &&
            native_machine == IMAGE_FILE_MACHINE_I386);
}

std::uintptr_t RemoteKernelExport(DWORD pid, const char* name)
{
    HMODULE local = GetModuleHandleW(L"kernel32.dll");
    FARPROC address = local ? GetProcAddress(local, name) : nullptr;
    RemoteModule remote{};
    if (!local || !address ||
        FindRemoteModule(pid, L"kernel32.dll", remote) != FindResult::Found)
        return 0;
    return remote.base + reinterpret_cast<std::uintptr_t>(address) -
           reinterpret_cast<std::uintptr_t>(local);
}

bool RunRemote(HANDLE process, std::uintptr_t address, void* parameter, DWORD& result)
{
    result = 0;
    if (!address)
        return false;
    const auto start = reinterpret_cast<LPTHREAD_START_ROUTINE>(address);
    Handle thread(CreateRemoteThread(process, nullptr, 0, start, parameter, 0, nullptr));
    if (!thread || WaitForSingleObject(thread.Get(), 15000) != WAIT_OBJECT_0)
        return false;
    return GetExitCodeThread(thread.Get(), &result) != FALSE;
}

void TryFreeRemoteLibrary(DWORD pid, HANDLE process, std::uintptr_t module) noexcept
{
    if (!module)
        return;
    DWORD ignored = 0;
    RunRemote(process, RemoteKernelExport(pid, "FreeLibrary"),
              reinterpret_cast<void*>(module), ignored);
}

bool WriteRemoteWide(HANDLE process, const std::wstring& value, void*& remote)
{
    remote = nullptr;
    const std::size_t size = (value.size() + 1) * sizeof(wchar_t);
    remote = VirtualAllocEx(process, nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote)
        return false;
    SIZE_T written = 0;
    if (!WriteProcessMemory(process, remote, value.c_str(), size, &written) || written != size)
    {
        VirtualFreeEx(process, remote, 0, MEM_RELEASE);
        remote = nullptr;
        return false;
    }
    return true;
}

std::uintptr_t RemoteDllExport(const std::wstring& dll_path, std::uintptr_t remote_base,
                               const char* export_name)
{
    Module local(LoadLibraryExW(dll_path.c_str(), nullptr, DONT_RESOLVE_DLL_REFERENCES));
    FARPROC address = local.Get() ? GetProcAddress(local.Get(), export_name) : nullptr;
    if (!address)
        return 0;
    return remote_base + reinterpret_cast<std::uintptr_t>(address) -
           reinterpret_cast<std::uintptr_t>(local.Get());
}

bool ValidateDll(const std::wstring& path)
{
    if (_wcsicmp(BaseName(path).c_str(), kDllName) != 0)
        return false;
    Module module(LoadLibraryExW(path.c_str(), nullptr, DONT_RESOLVE_DLL_REFERENCES));
    if (!module.Get() || !GetProcAddress(module.Get(), "IsaacCsT0Initialize") ||
        !GetProcAddress(module.Get(), "IsaacCsT0Shutdown") ||
        !GetProcAddress(module.Get(), "IsaacCsT0BoundaryInitialize") ||
        !GetProcAddress(module.Get(), "IsaacCsT0BoundaryShutdown") ||
        !GetProcAddress(module.Get(), "IsaacCsT0InputInitialize") ||
        !GetProcAddress(module.Get(), "IsaacCsT0InputShutdown") ||
        !GetProcAddress(module.Get(), "IsaacCsT0LoopInitialize") ||
        !GetProcAddress(module.Get(), "IsaacCsT0LoopShutdown") ||
        !GetProcAddress(module.Get(), "IsaacCsT0OfflineAttestationInitialize") ||
        !GetProcAddress(module.Get(), "IsaacCsT0OfflineAttestationShutdown") ||
        !GetProcAddress(module.Get(), "IsaacCsT0InputActiveInitialize") ||
        !GetProcAddress(module.Get(), "IsaacCsT0InputActiveShutdown") ||
        !GetProcAddress(module.Get(), "IsaacCsT0CoopActiveInitialize") ||
        !GetProcAddress(module.Get(), "IsaacCsT0CoopActiveShutdown") ||
        !GetProcAddress(module.Get(), "IsaacCsT0T09RemoteInitialize") ||
        !GetProcAddress(module.Get(), "IsaacCsT0T09RemoteShutdown"))
        return false;
    const auto* base = reinterpret_cast<const std::uint8_t*>(module.Get());
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
    return dos->e_magic == IMAGE_DOS_SIGNATURE && nt->Signature == IMAGE_NT_SIGNATURE &&
           nt->FileHeader.Machine == IMAGE_FILE_MACHINE_I386 &&
           (nt->FileHeader.Characteristics & IMAGE_FILE_DLL) != 0;
}

int Inject(DWORD pid, HANDLE process, const std::wstring& dll_path,
           const std::wstring& mapping_name, const char* initialize_export,
           bool resident)
{
    RemoteModule existing{};
    const auto existing_result = FindRemoteModule(pid, kDllName, existing);
    if (existing_result == FindResult::Error)
        return 19;
    if (existing_result == FindResult::Found)
        return 20;
    void* remote_path = nullptr;
    if (!WriteRemoteWide(process, dll_path, remote_path))
        return 21;
    DWORD load_result = 0;
    const bool loaded = RunRemote(process, RemoteKernelExport(pid, "LoadLibraryW"),
                                  remote_path, load_result);
    VirtualFreeEx(process, remote_path, 0, MEM_RELEASE);
    if (!loaded || !load_result)
        return 22;
    RemoteModule module{};
    if (FindRemoteModule(pid, kDllName, module) != FindResult::Found ||
        _wcsicmp(module.path.c_str(), dll_path.c_str()) != 0)
    {
        TryFreeRemoteLibrary(pid, process, load_result);
        return 23;
    }
    void* remote_mapping = nullptr;
    if (!WriteRemoteWide(process, mapping_name, remote_mapping))
    {
        TryFreeRemoteLibrary(pid, process, module.base);
        return 24;
    }
    DWORD initialize_result = 0;
    const auto initialize = RemoteDllExport(dll_path, module.base, initialize_export);
    const bool initialized = RunRemote(process, initialize, remote_mapping, initialize_result);
    VirtualFreeEx(process, remote_mapping, 0, MEM_RELEASE);
    if (!initialized || initialize_result != 0)
    {
        if (initialized)
            std::fprintf(stderr,
                         "FAIL independent T0 helper stage=REMOTE_INITIALIZE native_error=%lu\n",
                         initialize_result);
        else
            std::fprintf(stderr,
                         "FAIL independent T0 helper stage=REMOTE_INITIALIZE transport_error=%lu\n",
                         GetLastError());
        if (!resident && (!initialized || initialize_result != 8))
            TryFreeRemoteLibrary(pid, process, module.base);
        return initialized ? static_cast<int>(initialize_result) : 25;
    }
    return 0;
}

int Unload(DWORD pid, HANDLE process, const std::wstring& dll_path,
           const char* shutdown_export, bool resident)
{
    RemoteModule module{};
    if (FindRemoteModule(pid, kDllName, module) != FindResult::Found ||
        _wcsicmp(module.path.c_str(), dll_path.c_str()) != 0)
        return 30;
    const auto shutdown = RemoteDllExport(dll_path, module.base, shutdown_export);
    DWORD shutdown_result = 0;
    if (!RunRemote(process, shutdown, nullptr, shutdown_result) || shutdown_result != 0)
        return shutdown_result ? static_cast<int>(shutdown_result) : 31;
    if (resident)
    {
        RemoteModule remaining{};
        return FindRemoteModule(pid, kDllName, remaining) == FindResult::Found &&
                       remaining.base == module.base ? 0 : 34;
    }
    const ULONGLONG deadline = GetTickCount64() + 5000;
    while (GetTickCount64() < deadline)
    {
        RemoteModule remaining{};
        const auto find = FindRemoteModule(pid, kDllName, remaining);
        if (find == FindResult::Missing)
            return 0;
        if (find == FindResult::Error)
            return 33;
        Sleep(25);
    }
    return 32;
}

int Run(int count, wchar_t** arguments)
{
    if (count == 2 && _wcsicmp(arguments[1], L"--self-test") == 0)
        return sizeof(void*) == 4 ? 0 : 2;
    if (count < 2)
        return 2;
    const auto* operation = isaac_cs_t0::helper::FindOperation(arguments[1]);
    if (!operation || count != (operation->inject ? 5 : 4))
        return 2;
    wchar_t* end = nullptr;
    const unsigned long parsed = wcstoul(arguments[2], &end, 10);
    if (!parsed || !end || *end || parsed > MAXDWORD)
        return 2;
    std::wstring dll_path;
    if (!AbsolutePath(arguments[3], dll_path) || !ValidateDll(dll_path))
        return 3;
    const DWORD rights = PROCESS_QUERY_INFORMATION | PROCESS_CREATE_THREAD |
                         PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ;
    Handle process(OpenProcess(rights, FALSE, static_cast<DWORD>(parsed)));
    if (!process)
    {
        std::fprintf(stderr,
                     "FAIL independent T0 helper stage=OPEN_PROCESS win32_error=%lu\n",
                     GetLastError());
        return 104;
    }
    if (!ValidateProcess(process.Get()))
    {
        std::fprintf(stderr,
                     "FAIL independent T0 helper stage=PROCESS_VALIDATION win32_error=%lu\n",
                     GetLastError());
        return 105;
    }
    return operation->inject
               ? Inject(static_cast<DWORD>(parsed), process.Get(), dll_path,
                        arguments[4], operation->entry, operation->resident)
               : Unload(static_cast<DWORD>(parsed), process.Get(), dll_path,
                        operation->entry, operation->resident);
}

} // namespace

int wmain(int count, wchar_t** arguments)
{
    const int result = Run(count, arguments);
    if (result == 0)
        std::fputs("PASS independent T0 helper operation completed\n", stdout);
    else
        std::fprintf(stderr, "FAIL independent T0 helper error=%d\n", result);
    return result;
}
