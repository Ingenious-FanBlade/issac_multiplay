#include "isaac_cs_t0/offline_attestation_runtime.hpp"

#include "isaac_cs_t0/hooks.hpp"
#include "isaac_cs_t0/offline_attestation_ipc.hpp"
#include "isaac_cs_t0/offline_attestation_policy.hpp"
#include "isaac_cs_t0/sha256.hpp"
#include "isaac_cs_t0/target_image.hpp"

#include <Windows.h>
#include <intrin.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace isaac_cs_t0::offline_attestation {
namespace {

constexpr std::uint32_t kManagerSlotRva = 0x0087169Cu;
constexpr std::uint32_t kNetworkBackendSlotRva = 0x00879A58u;
constexpr std::uint32_t kSteamContextDescriptorRva = 0x0085C51Cu;
constexpr std::uint32_t kSteamContextInitializerRva = 0x00676090u;
constexpr std::uint32_t kLuaCloseCallRva = 0x0046E5C0u;
constexpr std::uint32_t kLuaPcallCallRva = 0x004729CBu;
constexpr std::uint32_t kLuaPcallReturnRva = 0x004729D1u;
constexpr std::uint32_t kGenericPumpRva = 0x0065E660u;
constexpr std::uint32_t kSecurityCookieRva = 0x007F93B4u;
constexpr std::uint32_t kPeekMessageIatRva = 0x00718714u;
constexpr std::uint32_t kMenuPeekCallRva = 0x0065E68Au;
constexpr std::uint32_t kMenuPeekReturnRva = 0x0065E68Cu;
constexpr std::uint32_t kGameSlotRva = 0x00871678u;
constexpr std::uint32_t kRunActiveOffset = 0x0002658Au;
constexpr std::uint32_t kSyncManagerOffset = 0x0004B3D8u;
constexpr DWORD kHookDrainTimeoutMs = 5000;

using LuaCloseFn = void(__cdecl*)(void*);
using LuaPcallFn = int(__cdecl*)(void*, int, int, int, std::intptr_t, void*);
using SteamContextInitFn = void*(__cdecl*)(void*);
using PeekMessageFn = BOOL(WINAPI*)(LPMSG, HWND, UINT, UINT, UINT);
using SteamSendFn = bool(__thiscall*)(void*, std::uint64_t, const void*, std::uint32_t, int, int);
using SteamAvailableFn = bool(__thiscall*)(void*, std::uint32_t*, int);
using SteamReadFn = bool(__thiscall*)(void*, void*, std::uint32_t, std::uint32_t*,
                                      std::uint64_t*, int);

HMODULE g_module = nullptr;
HMODULE g_main_module = nullptr;
std::uint8_t* g_main_base = nullptr;
HANDLE g_mapping = nullptr;
SharedBlock* g_shared = nullptr;

volatile LONG g_unloading = 0;
volatile LONG g_active_hooks = 0;
volatile LONG g_context_state = 0;
volatile LONG64 g_generation_counter = 0;
void* g_lua_root = nullptr;
void* g_game_root = nullptr;
ManagerNegativeSample g_manager_baseline{};
bool g_manager_bound = false;

LuaCloseFn g_lua_close = nullptr;
LuaPcallFn g_lua_pcall = nullptr;
SteamContextInitFn g_steam_context = nullptr;
PeekMessageFn g_peek_message = nullptr;
SteamSendFn g_steam_send = nullptr;
SteamAvailableFn g_steam_available = nullptr;
SteamReadFn g_steam_read = nullptr;

std::array<mode::PointerPatch, 4> g_iat_patches{};
std::size_t g_iat_patch_count = 0;
std::array<mode::PointerPatch, 3> g_steam_patches{};

void __cdecl LuaCloseHook(void* state);
int __cdecl LuaPcallHook(void* state, int nargs, int nresults, int errfunc,
                         std::intptr_t context, void* continuation);
void* __cdecl SteamContextHook(void* descriptor);
BOOL WINAPI PeekMessageHook(LPMSG message, HWND window, UINT minimum, UINT maximum,
                            UINT remove_message);
extern "C" bool __fastcall OfflineSteamSendHook(void* self, void*, std::uint64_t remote,
                                         const void* data, std::uint32_t size, int send_mode,
                                         int channel);
extern "C" bool __fastcall OfflineSteamAvailableHook(void* self, void*, std::uint32_t* size,
                                              int channel);
extern "C" bool __fastcall OfflineSteamReadHook(void* self, void*, void* data,
                                         std::uint32_t capacity, std::uint32_t* size,
                                         std::uint64_t* remote, int channel);

class HookScope final {
public:
    HookScope() noexcept
    {
        InterlockedIncrement(&g_active_hooks);
        if (g_shared)
            InterlockedIncrement(&g_shared->active_hooks);
    }

    ~HookScope()
    {
        if (g_shared)
            InterlockedDecrement(&g_shared->active_hooks);
        InterlockedDecrement(&g_active_hooks);
    }
};

std::int64_t Qpc() noexcept
{
    LARGE_INTEGER value{};
    return QueryPerformanceCounter(&value) ? value.QuadPart : 0;
}

bool Nonzero(const std::array<std::uint8_t, 32>& value) noexcept
{
    return std::any_of(value.begin(), value.end(), [](std::uint8_t byte) { return byte != 0; });
}

void SetState(Error error, NativeState state) noexcept
{
    if (!g_shared)
        return;
    InterlockedExchange(&g_shared->error_code, static_cast<LONG>(error));
    InterlockedExchange(&g_shared->state, static_cast<LONG>(state));
}

NativeEvent Event(EventKind kind) noexcept
{
    NativeEvent event{};
    event.qpc = Qpc();
    event.thread_id = GetCurrentThreadId();
    event.kind = static_cast<std::uint32_t>(kind);
    event.generation = g_shared ? static_cast<std::uint64_t>(
                                     InterlockedCompareExchange64(&g_shared->generation, 0, 0))
                               : 0;
    return event;
}

void Publish(const NativeEvent& event, bool force = false) noexcept
{
    if (!g_shared || InterlockedCompareExchange(&g_shared->producer_closed, 0, 0) != 0)
        return;
    if (!force && InterlockedCompareExchange(&g_unloading, 0, 0) != 0)
        return;
    const LONG64 sequence = InterlockedIncrement64(&g_shared->next_sequence);
    const LONG64 consumed = InterlockedCompareExchange64(&g_shared->controller_sequence, 0, 0);
    if (sequence - consumed > static_cast<LONG64>(kEventCapacity))
    {
        InterlockedIncrement64(&g_shared->overwritten_events);
        SetState(Error::RingOverflow, NativeState::Failed);
    }
    NativeEvent& slot = g_shared->events[static_cast<std::size_t>(sequence - 1) % kEventCapacity];
    InterlockedExchange64(&slot.published_sequence, 0);
    CopyEventPayload(slot, event);
    MemoryBarrier();
    InterlockedExchange64(&slot.published_sequence, sequence);
}

void Fail(Error error) noexcept
{
    NativeEvent event = Event(EventKind::Failure);
    event.error_code = static_cast<std::uint32_t>(error);
    Publish(event);
    SetState(error, NativeState::Failed);
}

bool CurrentCreationTime(std::uint64_t& value) noexcept
{
    FILETIME creation{}, exit{}, kernel{}, user{};
    if (!GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user))
        return false;
    value = (static_cast<std::uint64_t>(creation.dwHighDateTime) << 32) |
            creation.dwLowDateTime;
    return value != 0;
}

bool ValidateMapping() noexcept
{
    std::uint64_t creation = 0;
    if (!g_shared || g_shared->magic != kMagic || g_shared->version != kVersion ||
        g_shared->struct_size != sizeof(SharedBlock) ||
        g_shared->target_pid != GetCurrentProcessId() ||
        !ValidObservationPolicy(g_shared->reserved0) || g_shared->reserved1 != 0 ||
        !CurrentCreationTime(creation) ||
        creation != g_shared->process_creation_time_100ns || !Nonzero(g_shared->nonce) ||
        !ApprovedIdentity(g_shared->canonical_target_sha256,
                          g_shared->observed_target_sha256,
                          g_shared->target_equivalence_sha256))
        return false;
    const auto policy = static_cast<ObservationPolicy>(g_shared->reserved0);
    return policy == ObservationPolicy::LoadingMenuInactive
               ? g_shared->expected_window_thread_id != 0
               : g_shared->expected_window_thread_id == 0;
}

bool IsTargetImage() noexcept
{
    wchar_t path[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(nullptr, path, _countof(path));
    if (!length || length >= _countof(path))
        return false;
    const wchar_t* name = wcsrchr(path, L'\\');
    name = name ? name + 1 : path;
    if (_wcsicmp(name, L"isaac-ng.exe") != 0)
        return false;
    __try
    {
        return target_image::IsApproved(g_main_base,
                                        target_image::Profile::ApprovedUnpacked);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool HasConflict() noexcept
{
    return IsDebuggerPresent() != FALSE || GetModuleHandleW(L"IsaacSync.Native.dll") != nullptr ||
           GetModuleHandleW(L"tractor_beam_native_hook.dll") != nullptr ||
           GetModuleHandleW(L"x32dbg.dll") != nullptr ||
           GetModuleHandleW(L"x64dbg.dll") != nullptr;
}

bool SteamDescriptorMatches(void* descriptor) noexcept
{
    __try
    {
        return descriptor == g_main_base + kSteamContextDescriptorRva &&
               *reinterpret_cast<void**>(descriptor) ==
                   g_main_base + kSteamContextInitializerRva;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

ObservationPolicy Policy() noexcept
{
    return g_shared ? static_cast<ObservationPolicy>(g_shared->reserved0)
                    : ObservationPolicy::ActiveRun;
}

bool MenuPumpMatches(void** peek_slot) noexcept
{
    static constexpr std::array<std::uint8_t, 10> prefix{{
        0x55u, 0x8Bu, 0xECu, 0x83u, 0xE4u, 0xF8u, 0x83u, 0xECu, 0x5Cu, 0xA1u,
    }};
    static constexpr std::array<std::uint8_t, 11> middle{{
        0x33u, 0xC4u, 0x89u, 0x44u, 0x24u, 0x58u,
        0x53u, 0x56u, 0x57u, 0x8Bu, 0x3Du,
    }};
    static constexpr std::array<std::uint8_t, 2> call{{0xFFu, 0xD7u}};
    if (!peek_slot || reinterpret_cast<std::uint8_t*>(peek_slot) !=
                          g_main_base + kPeekMessageIatRva)
        return false;
    __try
    {
        const auto* pump = g_main_base + kGenericPumpRva;
        std::uint32_t cookie_address = 0;
        std::uint32_t peek_address = 0;
        std::memcpy(&cookie_address, pump + prefix.size(), sizeof(cookie_address));
        std::memcpy(&peek_address, pump + prefix.size() + sizeof(cookie_address) + middle.size(),
                    sizeof(peek_address));
        return std::memcmp(pump, prefix.data(), prefix.size()) == 0 &&
               RelocatedImageAddressMatches(reinterpret_cast<std::uintptr_t>(g_main_base),
                                            cookie_address, kSecurityCookieRva) &&
               std::memcmp(pump + prefix.size() + sizeof(cookie_address), middle.data(),
                           middle.size()) == 0 &&
               RelocatedImageAddressMatches(reinterpret_cast<std::uintptr_t>(g_main_base),
                                            peek_address, kPeekMessageIatRva) &&
               std::memcmp(g_main_base + kMenuPeekCallRva, call.data(), call.size()) == 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

struct GameRootSample {
    void* value = nullptr;
    bool readable = false;
};

GameRootSample ReadGameRoot() noexcept
{
    __try
    {
        return {*reinterpret_cast<void**>(g_main_base + kGameSlotRva), true};
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return {};
    }
}

RunStateSample ReadRunState(const GameRootSample& game) noexcept
{
    if (!game.readable || !game.value)
        return {};
    __try
    {
        return {true, true,
                *reinterpret_cast<std::uint16_t*>(
                    static_cast<std::uint8_t*>(game.value) + kRunActiveOffset)};
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return {true, false, 0};
    }
}

ManagerNegativeSample ReadManagerNegativeSample() noexcept
{
    ManagerNegativeSample sample{};
    __try
    {
        void* manager = *reinterpret_cast<void**>(g_main_base + kManagerSlotRva);
        void* backend = *reinterpret_cast<void**>(g_main_base + kNetworkBackendSlotRva);
        sample.manager = reinterpret_cast<std::uintptr_t>(manager);
        sample.network_backend = reinterpret_cast<std::uintptr_t>(backend);
        if (manager)
        {
            auto* sync = static_cast<std::uint8_t*>(manager) + kSyncManagerOffset;
            sample.roster_first = reinterpret_cast<std::uintptr_t>(
                *reinterpret_cast<void***>(sync));
            sample.roster_last = reinterpret_cast<std::uintptr_t>(
                *reinterpret_cast<void***>(sync + 4));
            sample.roster_capacity = reinterpret_cast<std::uintptr_t>(
                *reinterpret_cast<void***>(sync + 8));
            sample.online_frame = *reinterpret_cast<std::uint32_t*>(sync + 12);
        }
        sample.readable = true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        sample.readable = false;
    }
    return sample;
}

mode::Sha256Digest OpaqueDigest(void* value, std::uint32_t domain) noexcept
{
    struct Input {
        std::array<std::uint8_t, 32> nonce{};
        std::uintptr_t value = 0;
        std::uint32_t pid = 0;
        std::uint32_t domain = 0;
        std::uint64_t creation = 0;
    } input{};
    if (g_shared)
    {
        input.nonce = g_shared->nonce;
        input.pid = g_shared->target_pid;
        input.creation = g_shared->process_creation_time_100ns;
    }
    input.value = reinterpret_cast<std::uintptr_t>(value);
    input.domain = domain;
    return mode::Sha256(&input, sizeof(input));
}

bool InstallSteamHooks(void* context) noexcept
{
    if (!context)
        return InterlockedCompareExchange(&g_context_state, 0, 0) == 2;
    const LONG previous = InterlockedCompareExchange(&g_context_state, 1, 0);
    if (previous != 0)
    {
        const ULONGLONG deadline = GetTickCount64() + kHookDrainTimeoutMs;
        while (InterlockedCompareExchange(&g_context_state, 0, 0) == 1 &&
               GetTickCount64() < deadline)
            Sleep(0);
        return InterlockedCompareExchange(&g_context_state, 0, 0) == 2;
    }
    bool installed = false;
    __try
    {
        void* interface_pointer = *reinterpret_cast<void**>(context);
        void** vtable = interface_pointer ? *reinterpret_cast<void***>(interface_pointer) : nullptr;
        if (vtable && mode::IsAddressOwnedBy(vtable[0], L"steamclient.dll") &&
            mode::IsAddressOwnedBy(vtable[1], L"steamclient.dll") &&
            mode::IsAddressOwnedBy(vtable[2], L"steamclient.dll"))
        {
            // Prime every original before the transaction resumes suspended target threads.
            g_steam_send = reinterpret_cast<SteamSendFn>(vtable[0]);
            g_steam_available = reinterpret_cast<SteamAvailableFn>(vtable[1]);
            g_steam_read = reinterpret_cast<SteamReadFn>(vtable[2]);
            g_steam_patches = {{
                {&vtable[0], reinterpret_cast<void*>(&OfflineSteamSendHook), nullptr,
                 L"steamclient.dll", false},
                {&vtable[1], reinterpret_cast<void*>(&OfflineSteamAvailableHook), nullptr,
                 L"steamclient.dll", false},
                {&vtable[2], reinterpret_cast<void*>(&OfflineSteamReadHook), nullptr,
                 L"steamclient.dll", false},
            }};
            installed = mode::InstallPatchTransaction(nullptr, 0, g_steam_patches.data(),
                                                       g_steam_patches.size());
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        installed = false;
    }
    if (!installed)
    {
        InterlockedExchange(&g_context_state, -1);
        return false;
    }
    g_steam_send = reinterpret_cast<SteamSendFn>(g_steam_patches[0].original);
    g_steam_available = reinterpret_cast<SteamAvailableFn>(g_steam_patches[1].original);
    g_steam_read = reinterpret_cast<SteamReadFn>(g_steam_patches[2].original);
    InterlockedExchange(&g_context_state, 2);
    InterlockedExchange(&g_shared->steam_observer_armed, 1);
    InterlockedOr64(&g_shared->installed_hooks_mask, 0x38);
    NativeEvent event = Event(EventKind::SteamObserverArmed);
    event.flags = kFlagSteamArmed;
    Publish(event);
    return true;
}

bool ValidateAndInstallIatHooks() noexcept
{
    void** close = mode::FindIatSlot(g_main_module, "lua_close");
    void** pcall = mode::FindIatSlot(g_main_module, "lua_pcallk");
    void** steam = mode::FindIatSlot(g_main_module, "SteamInternal_ContextInit");
    void** peek = Policy() == ObservationPolicy::LoadingMenuInactive
                      ? mode::FindIatSlot(g_main_module, "PeekMessageW")
                      : nullptr;
    if (!close || !pcall || !steam ||
        !mode::ValidateIatCallsite(g_main_base + kLuaCloseCallRva, close) ||
        !mode::ValidateIatCallsite(g_main_base + kLuaPcallCallRva, pcall) ||
        !SteamDescriptorMatches(g_main_base + kSteamContextDescriptorRva) ||
        (Policy() == ObservationPolicy::LoadingMenuInactive && !MenuPumpMatches(peek)))
        return false;
    g_iat_patches[0] =
        {pcall, reinterpret_cast<void*>(&LuaPcallHook), nullptr, L"Lua5.3.3r.dll", false};
    g_iat_patches[1] =
        {close, reinterpret_cast<void*>(&LuaCloseHook), nullptr, L"Lua5.3.3r.dll", false};
    g_iat_patches[2] =
        {steam, reinterpret_cast<void*>(&SteamContextHook), nullptr, L"steam_api.dll", false};
    g_iat_patch_count = 3;
    if (peek)
    {
        g_iat_patches[3] =
            {peek, reinterpret_cast<void*>(&PeekMessageHook), nullptr, L"USER32.dll", false};
        g_iat_patch_count = 4;
    }
    // The patch transaction resumes all target threads before returning. Prime the originals so
    // a callback racing that return can only pass through, never observe a null function pointer.
    g_lua_pcall = reinterpret_cast<LuaPcallFn>(*pcall);
    g_lua_close = reinterpret_cast<LuaCloseFn>(*close);
    g_steam_context = reinterpret_cast<SteamContextInitFn>(*steam);
    g_peek_message = peek ? reinterpret_cast<PeekMessageFn>(*peek) : nullptr;
    if (!mode::InstallPatchTransaction(nullptr, 0, g_iat_patches.data(), g_iat_patch_count))
        return false;
    g_lua_pcall = reinterpret_cast<LuaPcallFn>(g_iat_patches[0].original);
    g_lua_close = reinterpret_cast<LuaCloseFn>(g_iat_patches[1].original);
    g_steam_context = reinterpret_cast<SteamContextInitFn>(g_iat_patches[2].original);
    if (peek)
        g_peek_message = reinterpret_cast<PeekMessageFn>(g_iat_patches[3].original);
    InterlockedExchange64(&g_shared->installed_hooks_mask,
                          Policy() == ObservationPolicy::LoadingMenuInactive ? 0x47 : 0x7);
    return true;
}

bool RestoreIatHooks() noexcept
{
    return mode::RestorePatchTransaction(nullptr, 0, g_iat_patches.data(),
                                         g_iat_patch_count);
}

bool RestoreSteamHooks() noexcept
{
    if (InterlockedCompareExchange(&g_context_state, 0, 0) != 2)
        return false;
    const bool restored = mode::RestorePatchTransaction(nullptr, 0, g_steam_patches.data(),
                                                        g_steam_patches.size());
    if (restored)
        InterlockedExchange64(&g_shared->restored_hooks_mask, 0x38);
    return restored;
}

mode::Sha256Digest SlotSnapshot(bool restored) noexcept
{
    struct Entry {
        std::uintptr_t slot = 0;
        std::uintptr_t value = 0;
    };
    std::array<Entry, 7> entries{};
    for (std::size_t index = 0; index < g_iat_patch_count; ++index)
    {
        entries[index].slot = reinterpret_cast<std::uintptr_t>(g_iat_patches[index].slot);
        entries[index].value = reinterpret_cast<std::uintptr_t>(
            restored ? *g_iat_patches[index].slot : g_iat_patches[index].original);
    }
    for (std::size_t index = 0; index < g_steam_patches.size(); ++index)
    {
        entries[index + g_iat_patch_count].slot =
            reinterpret_cast<std::uintptr_t>(g_steam_patches[index].slot);
        entries[index + g_iat_patch_count].value = reinterpret_cast<std::uintptr_t>(
            restored ? *g_steam_patches[index].slot : g_steam_patches[index].original);
    }
    return mode::Sha256(entries.data(), ExpectedSlotCount(Policy()) * sizeof(Entry));
}

bool RestoredOwnersValid() noexcept
{
    const bool fixed = mode::IsAddressOwnedBy(g_iat_patches[0].original, L"Lua5.3.3r.dll") &&
           mode::IsAddressOwnedBy(g_iat_patches[1].original, L"Lua5.3.3r.dll") &&
           mode::IsAddressOwnedBy(g_iat_patches[2].original, L"steam_api.dll") &&
           std::all_of(g_steam_patches.begin(), g_steam_patches.end(),
                       [](const mode::PointerPatch& patch) {
                           return mode::IsAddressOwnedBy(patch.original, L"steamclient.dll");
                       });
    return fixed &&
           (Policy() != ObservationPolicy::LoadingMenuInactive ||
            mode::IsAddressOwnedBy(g_iat_patches[3].original, L"USER32.dll"));
}

bool DrainHooks() noexcept
{
    const ULONGLONG deadline = GetTickCount64() + kHookDrainTimeoutMs;
    while (InterlockedCompareExchange(&g_active_hooks, 0, 0) != 0 &&
           GetTickCount64() < deadline)
        Sleep(1);
    return InterlockedCompareExchange(&g_active_hooks, 0, 0) == 0;
}

void* __cdecl SteamContextHook(void* descriptor)
{
    HookScope scope;
    void* context = g_steam_context(descriptor);
    if (SteamDescriptorMatches(descriptor) &&
        InterlockedCompareExchange(&g_unloading, 0, 0) == 0 &&
        InterlockedCompareExchange(&g_context_state, 0, 0) == 0)
        InstallSteamHooks(context);
    return context;
}

BOOL WINAPI PeekMessageHook(LPMSG message, HWND window, UINT minimum, UINT maximum,
                            UINT remove_message)
{
    const bool exact = _ReturnAddress() == g_main_base + kMenuPeekReturnRva;
    HookScope scope;
    if (!exact)
        return g_peek_message(message, window, minimum, maximum, remove_message);

    const DWORD thread_id = GetCurrentThreadId();
    const GameRootSample game_before = ReadGameRoot();
    const RunStateSample run_before = ReadRunState(game_before);
    const ManagerNegativeSample manager_before = ReadManagerNegativeSample();
    const BOOL result = g_peek_message(message, window, minimum, maximum, remove_message);
    const GameRootSample game_after = ReadGameRoot();
    const RunStateSample run_after = ReadRunState(game_after);
    const ManagerNegativeSample manager_after = ReadManagerNegativeSample();

    if (!ObservationWindowOpen(
            InterlockedCompareExchange(&g_shared->steam_observer_armed, 0, 0) != 0,
            InterlockedCompareExchange(&g_unloading, 0, 0) != 0))
        return result;

    const DWORD expected_thread = g_shared->expected_window_thread_id;
    const LONG frozen = InterlockedCompareExchange(
        &g_shared->main_thread_id, static_cast<LONG>(expected_thread), 0);
    const bool correct_thread = expected_thread != 0 && thread_id == expected_thread &&
                                (frozen == 0 || static_cast<DWORD>(frozen) == expected_thread);
    const bool game_stable = game_before.readable && game_after.readable &&
                             game_before.value && game_before.value == game_after.value;
    const bool before_inactive =
        RunStateAllowed(ObservationPolicy::LoadingMenuInactive, run_before);
    const bool after_inactive =
        RunStateAllowed(ObservationPolicy::LoadingMenuInactive, run_after);
    const bool manager_negative =
        !ManagerActivitySignal(manager_before) && !RosterSignal(manager_before) &&
        !ManagerActivitySignal(manager_after) && !RosterSignal(manager_after) &&
        SameManagerIdentity(manager_before, manager_after);
    const bool online_signal = !manager_negative;
    bool identity_changed = false;
    if (correct_thread && game_stable && before_inactive && after_inactive && manager_negative)
    {
        if (!g_game_root)
        {
            g_game_root = game_before.value;
            g_manager_baseline = manager_before;
            g_manager_bound = true;
            const LONG64 generation = InterlockedIncrement64(&g_generation_counter);
            InterlockedExchange64(&g_shared->generation, generation);
        }
        else
        {
            identity_changed = game_before.value != g_game_root || !g_manager_bound ||
                               !SameManagerIdentity(g_manager_baseline, manager_before);
        }
    }
    else
    {
        identity_changed = game_stable && g_game_root && game_before.value != g_game_root;
    }

    if (ManagerActivitySignal(manager_before) || ManagerActivitySignal(manager_after))
        InterlockedIncrement64(&g_shared->manager_signal_count);
    if (RosterSignal(manager_before) || RosterSignal(manager_after))
        InterlockedIncrement64(&g_shared->roster_signal_count);
    if (!before_inactive || !after_inactive)
        InterlockedExchange(&g_shared->teardown_observed, 1);

    NativeEvent event = Event(EventKind::MenuHeartbeatPeek);
    event.callback_sequence = static_cast<std::uint64_t>(
        InterlockedIncrement64(&g_shared->menu_heartbeats));
    event.original_calls = 1;
    event.return_rva = kMenuPeekReturnRva;
    event.result_bits = static_cast<std::uint32_t>(result);
    const bool root_match = game_stable && g_game_root && game_before.value == g_game_root;
    event.flags = kFlagOriginalCalled | kFlagSteamArmed | kFlagExactReturn |
                  kFlagLuaUnboundMenu |
                  (correct_thread ? kFlagMainThread | kFlagWindowMainThread : 0u) |
                  (before_inactive ? kFlagRunInactiveBefore : 0u) |
                  (after_inactive ? kFlagRunInactiveAfter : 0u) |
                  (root_match ? kFlagRootMatch : 0u) |
                  (game_stable ? kFlagGameRootStable : 0u) |
                  (manager_negative ? kFlagManagerNegative : 0u) |
                  (before_inactive && after_inactive ? kFlagLoadingMenuInactive : 0u);
    if (game_stable)
    {
        event.game_root_digest = OpaqueDigest(game_before.value, 1);
        event.lua_generation_digest = OpaqueDigest(game_before.value, 3);
    }
    Publish(event);

    if (!correct_thread)
        Fail(Error::WrongThread);
    else if (identity_changed || !game_stable || !before_inactive || !after_inactive)
        Fail(Error::GenerationChanged);
    else if (online_signal)
        Fail(Error::OnlineSignal);
    return result;
}

void __cdecl LuaCloseHook(void* state)
{
    HookScope scope;
    const bool root_match = state && state == g_lua_root;
    g_lua_close(state);
    NativeEvent event = Event(EventKind::LuaClose);
    event.original_calls = 1;
    event.flags = kFlagOriginalCalled | (root_match ? kFlagRootMatch | kFlagTeardown : 0u);
    Publish(event);
    if (root_match || Policy() == ObservationPolicy::LoadingMenuInactive)
    {
        InterlockedExchange(&g_shared->teardown_observed, 1);
        Fail(Error::GenerationChanged);
    }
}

int __cdecl LuaPcallHook(void* state, int nargs, int nresults, int errfunc,
                         std::intptr_t context, void* continuation)
{
    void* return_address = _ReturnAddress();
    const bool simulation = return_address == g_main_base + kLuaPcallReturnRva;
    HookScope scope;
    if (Policy() == ObservationPolicy::LoadingMenuInactive)
    {
        const int result = g_lua_pcall(state, nargs, nresults, errfunc, context, continuation);
        if (ObservationWindowOpen(
                InterlockedCompareExchange(&g_shared->steam_observer_armed, 0, 0) != 0,
                InterlockedCompareExchange(&g_unloading, 0, 0) != 0))
        {
            const auto address = reinterpret_cast<std::uintptr_t>(return_address);
            const auto base = reinterpret_cast<std::uintptr_t>(g_main_base);
            NativeEvent event = Event(EventKind::GenericLuaPcallDiagnostic);
            event.original_calls = 1;
            event.result_bits = static_cast<std::uint32_t>(result);
            event.return_rva = address >= base && address - base <= UINT32_MAX
                                   ? static_cast<std::uint32_t>(address - base)
                                   : 0;
            event.flags = kFlagOriginalCalled | kFlagSteamArmed | kFlagLuaUnboundMenu |
                          (simulation ? kFlagExactReturn : 0u);
            Publish(event);
            if (simulation)
                Fail(Error::GenerationChanged);
        }
        return result;
    }
    const GameRootSample game = simulation ? ReadGameRoot() : GameRootSample{};
    const auto policy = Policy();
    const RunStateSample run_state = simulation ? ReadRunState(game) : RunStateSample{};
    const bool loading_menu_inactive =
        simulation && IsLoadingMenuInactive(policy, run_state);
    const ManagerNegativeSample manager_sample =
        simulation ? ReadManagerNegativeSample() : ManagerNegativeSample{};
    const bool manager_activity = simulation && ManagerActivitySignal(manager_sample);
    const bool roster = simulation && RosterSignal(manager_sample);
    const DWORD thread_id = GetCurrentThreadId();
    bool wrong_thread = false;
    bool generation_changed = false;
    bool teardown = false;
    if (simulation)
    {
        const LONG frozen = InterlockedCompareExchange(&g_shared->main_thread_id,
                                                       static_cast<LONG>(thread_id), 0);
        wrong_thread = frozen != 0 && static_cast<DWORD>(frozen) != thread_id;
        if (!g_lua_root)
        {
            g_lua_root = state;
            g_game_root = game.value;
            g_manager_baseline = manager_sample;
            g_manager_bound = true;
            const LONG64 generation = InterlockedIncrement64(&g_generation_counter);
            InterlockedExchange64(&g_shared->generation, generation);
        }
        else
        {
            generation_changed = state != g_lua_root || game.value != g_game_root ||
                                 !g_manager_bound ||
                                 !SameManagerIdentity(g_manager_baseline, manager_sample);
        }
        teardown = !RunStateAllowed(policy, run_state);
        if (manager_activity)
            InterlockedIncrement64(&g_shared->manager_signal_count);
        if (roster)
            InterlockedIncrement64(&g_shared->roster_signal_count);
        if (teardown)
            InterlockedExchange(&g_shared->teardown_observed, 1);
    }

    const int result = g_lua_pcall(state, nargs, nresults, errfunc, context, continuation);
    if (simulation)
    {
        const bool observation_window = ObservationWindowOpen(
            InterlockedCompareExchange(&g_shared->steam_observer_armed, 0, 0) != 0,
            InterlockedCompareExchange(&g_unloading, 0, 0) != 0);
        if (observation_window)
        {
            NativeEvent event = Event(EventKind::SimulationCallback);
            event.callback_sequence = static_cast<std::uint64_t>(
                InterlockedIncrement64(&g_shared->simulation_callbacks));
            event.original_calls = 1;
            event.flags = kFlagOriginalCalled | kFlagSteamArmed |
                          (!wrong_thread ? kFlagMainThread : 0u) |
                          (state == g_lua_root ? kFlagRootMatch : 0u) |
                          (manager_activity ? kFlagManagerActivity : 0u) |
                          (roster ? kFlagStableRoster : 0u) |
                          (teardown ? kFlagTeardown : 0u) |
                          (loading_menu_inactive ? kFlagLoadingMenuInactive : 0u);
            event.game_root_digest = OpaqueDigest(game.value, 1);
            event.lua_generation_digest = OpaqueDigest(state, 2);
            Publish(event);
        }
        if (wrong_thread)
            Fail(Error::WrongThread);
        else if (generation_changed)
            Fail(Error::GenerationChanged);
        else if (manager_activity || roster)
            Fail(Error::OnlineSignal);
        else if (teardown)
            Fail(Error::GenerationChanged);
    }
    return result;
}

void RecordSteam(EventKind kind, int channel, std::uint32_t original_calls) noexcept
{
    if (channel == 0)
        InterlockedIncrement64(&g_shared->channel_zero_count);
    else
        InterlockedIncrement64(&g_shared->nonzero_channel_count);
    NativeEvent event = Event(kind);
    event.channel = channel;
    event.original_calls = original_calls;
    event.flags = kFlagOriginalCalled | kFlagSteamArmed |
                  (channel == 0 ? kFlagChannelZero : kFlagNonzeroChannel);
    Publish(event);
    Fail(Error::OnlineSignal);
}

extern "C" bool __fastcall OfflineSteamSendHook(void* self, void*, std::uint64_t remote,
                                         const void* data, std::uint32_t size, int send_mode,
                                         int channel)
{
    HookScope scope;
    const bool result = g_steam_send(self, remote, data, size, send_mode, channel);
    RecordSteam(EventKind::SteamSend, channel, 1);
    return result;
}

extern "C" bool __fastcall OfflineSteamAvailableHook(void* self, void*, std::uint32_t* size,
                                              int channel)
{
    HookScope scope;
    const bool result = g_steam_available(self, size, channel);
    RecordSteam(EventKind::SteamAvailable, channel, 1);
    return result;
}

extern "C" bool __fastcall OfflineSteamReadHook(void* self, void*, void* data,
                                         std::uint32_t capacity, std::uint32_t* size,
                                         std::uint64_t* remote, int channel)
{
    HookScope scope;
    const bool result = g_steam_read(self, data, capacity, size, remote, channel);
    RecordSteam(EventKind::SteamRead, channel, 1);
    return result;
}

void CloseMapping() noexcept
{
    if (g_shared)
    {
        UnmapViewOfFile(g_shared);
        g_shared = nullptr;
    }
    if (g_mapping)
    {
        CloseHandle(g_mapping);
        g_mapping = nullptr;
    }
}

} // namespace

void SetModule(HMODULE module) noexcept
{
    g_module = module;
}

void ProcessDetach() noexcept
{
    CloseMapping();
    g_main_module = nullptr;
    g_main_base = nullptr;
    g_module = nullptr;
}

extern "C" DWORD WINAPI IsaacCsT0OfflineAttestationInitialize(const wchar_t* mapping_name)
{
    if (!g_module || !mapping_name || !*mapping_name || g_mapping || g_shared)
        return static_cast<DWORD>(Error::MappingInvalid);
    g_mapping = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, mapping_name);
    if (!g_mapping)
        return static_cast<DWORD>(Error::MappingInvalid);
    g_shared = static_cast<SharedBlock*>(MapViewOfFile(
        g_mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedBlock)));
    if (!ValidateMapping())
    {
        SetState(Error::MappingInvalid, NativeState::Failed);
        return static_cast<DWORD>(Error::MappingInvalid);
    }
    InterlockedExchange(&g_shared->state, static_cast<LONG>(NativeState::Initializing));
    g_main_module = GetModuleHandleW(nullptr);
    g_main_base = reinterpret_cast<std::uint8_t*>(g_main_module);
    Publish(Event(EventKind::NativeInitialized));
    if (!IsTargetImage())
    {
        Fail(Error::TargetMismatch);
        return static_cast<DWORD>(Error::TargetMismatch);
    }
    if (HasConflict())
    {
        Fail(Error::DebuggerOrConflict);
        return static_cast<DWORD>(Error::DebuggerOrConflict);
    }
    if (!ValidateAndInstallIatHooks())
    {
        Fail(Error::HookTargetMismatch);
        return static_cast<DWORD>(Error::HookTargetMismatch);
    }
    Publish(Event(EventKind::HooksInstalled));

    void* context = nullptr;
    // Reverse evidence supports this only as one context-acquisition probe per observer load; it
    // is not treated as, or documented as, a proven idempotent getter.
    __try
    {
        context = g_steam_context(g_main_base + kSteamContextDescriptorRva);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        context = nullptr;
    }
    if (!SteamDescriptorMatches(g_main_base + kSteamContextDescriptorRva) ||
        !InstallSteamHooks(context))
    {
        const bool restored = RestoreIatHooks();
        Fail(context ? Error::SteamOwnerMismatch : Error::SteamContextUnavailable);
        if (!restored)
        {
            SetState(Error::RestoreFailed, NativeState::Unsafe);
            return static_cast<DWORD>(Error::RestoreFailed);
        }
        return static_cast<DWORD>(context ? Error::SteamOwnerMismatch
                                          : Error::SteamContextUnavailable);
    }
    g_shared->slot_snapshot_before_sha256 = SlotSnapshot(false);
    InterlockedExchange(&g_shared->state, static_cast<LONG>(NativeState::Ready));
    return 0;
}

extern "C" DWORD WINAPI IsaacCsT0OfflineAttestationShutdown(void*)
{
    if (!g_shared || InterlockedExchange(&g_unloading, 1) != 0)
        return static_cast<DWORD>(Error::RestoreFailed);
    InterlockedExchange(&g_shared->state, static_cast<LONG>(NativeState::ShuttingDown));
    if (!RestoreSteamHooks() || !DrainHooks())
    {
        SetState(Error::RestoreFailed, NativeState::Unsafe);
        return static_cast<DWORD>(Error::RestoreFailed);
    }
    if (!RestoreIatHooks() || !DrainHooks())
    {
        SetState(Error::RestoreFailed, NativeState::Unsafe);
        return static_cast<DWORD>(Error::RestoreFailed);
    }
    InterlockedOr64(&g_shared->restored_hooks_mask,
                    Policy() == ObservationPolicy::LoadingMenuInactive ? 0x47 : 0x7);
    g_shared->slot_snapshot_after_sha256 = SlotSnapshot(true);
    if (g_shared->slot_snapshot_before_sha256 != g_shared->slot_snapshot_after_sha256 ||
        !RestoredOwnersValid())
    {
        SetState(Error::RestoreFailed, NativeState::Unsafe);
        return static_cast<DWORD>(Error::RestoreFailed);
    }
    InterlockedExchange(&g_shared->restored_slot_count,
                        static_cast<LONG>(ExpectedSlotCount(Policy())));
    Publish(Event(EventKind::HooksRestored), true);
    Publish(Event(EventKind::ProducerClosed), true);
    InterlockedExchange(&g_shared->producer_closed, 1);
    InterlockedExchange(&g_shared->state, static_cast<LONG>(NativeState::Stopped));
    HMODULE module = g_module;
    FreeLibraryAndExitThread(module, 0);
}

} // namespace isaac_cs_t0::offline_attestation
