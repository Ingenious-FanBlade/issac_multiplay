#include "isaac_cs_t0/boundary_runtime.hpp"

#include "isaac_cs_t0/boundary_ipc.hpp"
#include "isaac_cs_t0/hooks.hpp"

#include <Windows.h>
#include <intrin.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace isaac_cs_t0::boundary {
namespace {

constexpr std::uint32_t kExpectedTimestamp = 0x69E6E3A7u;
constexpr std::uint32_t kExpectedImageSize = 0x0092C000u;
constexpr std::uint32_t kManagerSlotRva = 0x0087169Cu;
constexpr std::uint32_t kNetworkBackendSlotRva = 0x00879A58u;
constexpr std::uint32_t kSteamBackendVtableRva = 0x007A2440u;
constexpr std::uint32_t kGameSlotRva = 0x00871678u;
constexpr std::uint32_t kSyncManagerOffset = 0x0004B3D8u;
constexpr std::uint32_t kOnlineFrameOffset = 0x0Cu;
constexpr std::uint32_t kEntityContainerOffset = 0x00018300u;
constexpr std::uint32_t kEntityListOffset = 0x00001218u;
constexpr std::uint32_t kEntityCountOffset = 0x4Cu;
constexpr std::uint32_t kMainLoopStateOffset = 0x08u;
constexpr std::uint32_t kGameplayDispatchFlagsOffset = 0x0004ABBCu;
constexpr std::uint32_t kLuaCloseCallRva = 0x0046E5C0u;
constexpr std::uint32_t kLuaCloseReturnRva = 0x0046E5C6u;
constexpr std::uint32_t kLuaNewstateCallRva = 0x0046057Bu;
constexpr std::uint32_t kLuaNewstateReturnRva = 0x00460581u;
constexpr std::uint32_t kLuaLNewstateCallRva = 0x004605A3u;
constexpr std::uint32_t kLuaLNewstateReturnRva = 0x004605A9u;
constexpr std::uint32_t kLuaPcallCallRva = 0x004729CBu;
constexpr std::uint32_t kLuaPcallReturnRva = 0x004729D1u;
constexpr std::uint32_t kLuaRebuildScenario = 8u;
constexpr DWORD kHookDrainTimeoutMs = 5000;

constexpr std::array<std::uint32_t, 12> kCallRvas{{
    0x00554F84u, 0x00555231u, 0x002FB51Cu, 0x002FBB66u, 0x002FBBA5u,
    0x005559C1u, 0x002FC0A6u, 0x005110ABu, 0x00028B3Du, 0x00559696u,
    0x005AB84Au, 0x0033F5B9u,
}};
constexpr std::array<std::uint32_t, 12> kTargetRvas{{
    0x0055B310u, 0x002FADC0u, 0x004318A0u, 0x000147F0u, 0x004607A0u,
    0x002FBC10u, 0x00463680u, 0x0050D640u, 0x002FE410u, 0x002FA0C0u,
    0x002FA0C0u, 0x00465180u,
}};
constexpr std::array<std::array<std::uint8_t, 5>, 12> kCallBytes{{
    {{0xE8, 0x87, 0x63, 0x00, 0x00}},
    {{0xE8, 0x8A, 0x5B, 0xDA, 0xFF}},
    {{0xE8, 0x7F, 0x63, 0x13, 0x00}},
    {{0xE8, 0x85, 0x8C, 0xD1, 0xFF}},
    {{0xE8, 0xF6, 0x4B, 0x16, 0x00}},
    {{0xE8, 0x4A, 0x62, 0xDA, 0xFF}},
    {{0xE8, 0xD5, 0x75, 0x16, 0x00}},
    {{0xE8, 0x90, 0xC5, 0xFF, 0xFF}},
    {{0xE8, 0xCE, 0x58, 0x2D, 0x00}},
    {{0xE8, 0x25, 0x0A, 0xDA, 0xFF}},
    {{0xE8, 0x71, 0xE8, 0xD4, 0xFF}},
    {{0xE8, 0xC2, 0x5B, 0x12, 0x00}},
}};

enum class ErrorCode : LONG {
    None = 0,
    MappingInvalid = 1,
    TargetImageMismatch = 2,
    ConflictingModuleOrDebugger = 3,
    HookTransaction = 4,
    EventRingOverflow = 5,
    WrongThread = 6,
    OriginalOnline = 7,
    UnsafeShutdown = 8,
    HookDrainTimeout = 9,
    LuaLifecycleMismatch = 10,
    MainLoopObservation = 11,
};

using ThisVoidFn = void(__thiscall*)(void*);
using CallbackFn = void(__cdecl*)();
using DispatchFn = std::uint32_t(__thiscall*)(void*, std::uint32_t);
using SpawnFn = void*(__thiscall*)(void*, std::uintptr_t, std::uintptr_t, std::uintptr_t,
                                  std::uintptr_t, std::uintptr_t, std::uintptr_t,
                                  std::uintptr_t, std::uintptr_t);
using LifecycleFn = std::uint32_t(__stdcall*)(std::uint32_t);
using LuaNewstateFn = void*(__cdecl*)(void*, void*);
using LuaLNewstateFn = void*(__cdecl*)();
using LuaCloseFn = void(__cdecl*)(void*);
using LuaPcallFn = int(__cdecl*)(void*, int, int, int, std::intptr_t, void*);

struct Snapshot {
    std::uint64_t summary = 0;
    std::uint32_t flags = 0;
    std::uint32_t game_frame = kUnavailable;
    std::uint32_t online_frame = kUnavailable;
    std::uint32_t entity_count = kUnavailable;
};

HMODULE g_module = nullptr;
HMODULE g_main_module = nullptr;
std::uint8_t* g_main_base = nullptr;
HANDLE g_mapping = nullptr;
SharedBoundaryBlock* g_shared = nullptr;
volatile LONG g_unloading = 0;
volatile LONG g_armed = 0;
volatile LONG g_active_hooks = 0;
volatile LONG g_retired = 0;
volatile LONG g_lua_rebuild_pending = 0;
volatile LONG g_unsafe_reported = 0;
std::array<std::uint64_t, 4> g_roster_candidate{};
std::size_t g_roster_candidate_count = 0;
std::uint32_t g_roster_stable_observations = 0;
thread_local std::uint32_t g_depth = 0;

ThisVoidFn g_input_sample = nullptr;
ThisVoidFn g_gameplay = nullptr;
ThisVoidFn g_entity = nullptr;
ThisVoidFn g_remove = nullptr;
CallbackFn g_post_update = nullptr;
ThisVoidFn g_render = nullptr;
CallbackFn g_post_render = nullptr;
CallbackFn g_new_room = nullptr;
DispatchFn g_online_dispatch = nullptr;
SpawnFn g_spawn = nullptr;
LifecycleFn g_run_teardown = nullptr;
LuaNewstateFn g_lua_newstate = nullptr;
LuaLNewstateFn g_lua_lnewstate = nullptr;
LuaCloseFn g_lua_close = nullptr;
LuaPcallFn g_lua_pcall = nullptr;
void* g_lua_root = nullptr;
void* g_closed_lua_root = nullptr;
thread_local std::uint32_t g_lua_create_depth = 0;
std::array<isaac_cs_t0::mode::RelativeCallPatch, 12> g_relative_patches{};
std::array<isaac_cs_t0::mode::PointerPatch, 4> g_iat_patches{};

extern "C" void __fastcall InputSampleHook(void* self, void* unused);
extern "C" void __fastcall GameplayHook(void* self, void* unused);
extern "C" void __fastcall EntityHook(void* self, void* unused);
extern "C" void __fastcall RemoveHook(void* self, void* unused);
void __cdecl PostUpdateHook();
extern "C" void __fastcall RenderHook(void* self, void* unused);
void __cdecl PostRenderHook();
void __cdecl NewRoomHook();
extern "C" std::uint32_t __fastcall OnlineDispatchHook(void* self, void* unused,
                                                        std::uint32_t argument);
extern "C" void* __fastcall SpawnHook(void* self, void* unused, std::uintptr_t type,
                                      std::uintptr_t variant, std::uintptr_t subtype,
                                      std::uintptr_t position, std::uintptr_t velocity,
                                      std::uintptr_t spawner, std::uintptr_t seed,
                                      std::uintptr_t zero_sentinel);
std::uint32_t __stdcall RunTeardownHook(std::uint32_t argument);
void* __cdecl LuaNewstateHook(void* allocator, void* user_data);
void* __cdecl LuaLNewstateHook();
void __cdecl LuaCloseHook(void* state);
int __cdecl LuaPcallHook(void* state, int nargs, int nresults, int errfunc,
                         std::intptr_t context, void* continuation);

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

bool Nonzero(const std::array<std::uint8_t, 32>& bytes) noexcept
{
    return std::any_of(bytes.begin(), bytes.end(), [](std::uint8_t value) { return value != 0; });
}

void SetState(ErrorCode error, NativeState state) noexcept
{
    if (!g_shared)
        return;
    InterlockedExchange(&g_shared->error_code, static_cast<LONG>(error));
    InterlockedExchange(&g_shared->state, static_cast<LONG>(state));
}

void Publish(const NativeEvent& event) noexcept
{
    if (!g_shared)
        return;
    const LONG64 sequence = InterlockedIncrement64(&g_shared->next_sequence);
    const LONG64 consumed = InterlockedCompareExchange64(&g_shared->controller_sequence, 0, 0);
    if (sequence - consumed > static_cast<LONG64>(kEventCapacity))
    {
        InterlockedIncrement64(&g_shared->overwritten_events);
        SetState(ErrorCode::EventRingOverflow, NativeState::Unsafe);
    }
    NativeEvent& slot = g_shared->events[static_cast<std::size_t>(sequence - 1) % kEventCapacity];
    InterlockedExchange64(&slot.published_sequence, 0);
    CopyEventPayload(slot, event);
    MemoryBarrier();
    InterlockedExchange64(&slot.published_sequence, sequence);
}

NativeEvent Event(EventKind kind, std::uint32_t depth = 0) noexcept
{
    NativeEvent event{};
    event.qpc = Qpc();
    event.generation = g_shared
                           ? static_cast<std::uint64_t>(InterlockedCompareExchange64(
                                 &g_shared->generation, 0, 0))
                           : 0;
    event.thread_id = GetCurrentThreadId();
    event.kind = static_cast<std::uint32_t>(kind);
    event.depth = depth;
    return event;
}

std::uint32_t ReturnRva(void* address) noexcept
{
    if (!address || !g_main_base)
        return 0;
    const auto raw = reinterpret_cast<std::uintptr_t>(address);
    const auto base = reinterpret_cast<std::uintptr_t>(g_main_base);
    if (raw < base || raw - base >= kExpectedImageSize)
        return 0;
    return static_cast<std::uint32_t>(raw - base);
}

std::uint64_t Mix(std::uint64_t value, std::uint32_t word) noexcept
{
    value ^= word;
    return value * 1099511628211ull;
}

Snapshot ReadSnapshot() noexcept
{
    Snapshot snapshot{};
    __try
    {
        void* manager = *reinterpret_cast<void**>(g_main_base + kManagerSlotRva);
        if (manager)
        {
            auto* sync = static_cast<std::uint8_t*>(manager) + kSyncManagerOffset;
            snapshot.online_frame =
                *reinterpret_cast<std::uint32_t*>(sync + kOnlineFrameOffset);
            snapshot.flags |= kFlagOnlineFrameValid;
        }
        void* game = *reinterpret_cast<void**>(g_main_base + kGameSlotRva);
        void* entity_owner = game ? *reinterpret_cast<void**>(
                                        static_cast<std::uint8_t*>(game) + kEntityContainerOffset)
                                  : nullptr;
        void* entity_list = entity_owner
                                ? *reinterpret_cast<void**>(
                                      static_cast<std::uint8_t*>(entity_owner) + kEntityListOffset)
                                : nullptr;
        if (entity_list)
        {
            const std::uint32_t count = *reinterpret_cast<std::uint32_t*>(
                static_cast<std::uint8_t*>(entity_list) + kEntityCountOffset);
            if (count < 1000000u)
            {
                snapshot.entity_count = count;
                snapshot.flags |= kFlagEntityCountValid;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        snapshot.online_frame = kUnavailable;
        snapshot.entity_count = kUnavailable;
        snapshot.flags = 0;
    }
    std::uint64_t summary = 1469598103934665603ull;
    summary = Mix(summary, snapshot.game_frame);
    summary = Mix(summary, snapshot.online_frame);
    summary = Mix(summary, snapshot.entity_count);
    snapshot.summary = summary;
    snapshot.flags |= kFlagSummaryValid;
    return snapshot;
}

void FillSnapshot(NativeEvent& event, const Snapshot& before, const Snapshot& after) noexcept
{
    event.summary_before = before.summary;
    event.summary_after = after.summary;
    event.game_frame_before = before.game_frame;
    event.game_frame_after = after.game_frame;
    event.online_frame_before = before.online_frame;
    event.online_frame_after = after.online_frame;
    event.entity_count_before = before.entity_count;
    event.entity_count_after = after.entity_count;
    event.flags |= before.flags & after.flags;
}

bool ReadMainLoopObservation(void* self, std::uint32_t& flags,
                             std::uint32_t& state) noexcept
{
    flags = 0;
    state = kUnavailable;
    if (!self)
        return false;
    __try
    {
        const auto* root = static_cast<const std::uint8_t*>(self);
        state = *reinterpret_cast<const std::uint32_t*>(root + kMainLoopStateOffset);
        flags = kFlagMainLoopStateValid;
        if ((*reinterpret_cast<const std::uint8_t*>(root + kGameplayDispatchFlagsOffset) & 1u) !=
            0)
            flags |= kFlagGameplayDispatchSuppressed;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        flags = 0;
        state = kUnavailable;
        return false;
    }
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
        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(g_main_base);
        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(g_main_base + dos->e_lfanew);
        return dos->e_magic == IMAGE_DOS_SIGNATURE && nt->Signature == IMAGE_NT_SIGNATURE &&
               nt->FileHeader.Machine == IMAGE_FILE_MACHINE_I386 &&
               nt->FileHeader.TimeDateStamp == kExpectedTimestamp &&
               nt->OptionalHeader.SizeOfImage == kExpectedImageSize;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool HasConflictingRuntime() noexcept
{
    return IsDebuggerPresent() != FALSE || GetModuleHandleW(L"IsaacSync.Native.dll") != nullptr ||
           GetModuleHandleW(L"tractor_beam_native_hook.dll") != nullptr ||
           GetModuleHandleW(L"x32dbg.dll") != nullptr || GetModuleHandleW(L"x64dbg.dll") != nullptr;
}

bool ValidateMapping() noexcept
{
    return g_shared && g_shared->magic == kIpcMagic && g_shared->version == kIpcVersion &&
           g_shared->size == sizeof(SharedBoundaryBlock) &&
           g_shared->target_pid == GetCurrentProcessId() &&
           g_shared->probe == static_cast<std::uint32_t>(Probe::Passive) &&
           g_shared->scenario >= 1 && g_shared->scenario <= 8 && Nonzero(g_shared->nonce);
}

bool ValidateCallTarget(std::size_t index) noexcept
{
    __try
    {
        const auto* call = g_main_base + kCallRvas[index];
        if (std::memcmp(call, kCallBytes[index].data(), 5) != 0 || call[0] != 0xE8)
            return false;
        const auto displacement = *reinterpret_cast<const std::int32_t*>(call + 1);
        return call + 5 + displacement == g_main_base + kTargetRvas[index];
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool IsSteamBackend() noexcept
{
    __try
    {
        void* backend = *reinterpret_cast<void**>(g_main_base + kNetworkBackendSlotRva);
        void** vtable = backend ? *reinterpret_cast<void***>(backend) : nullptr;
        return vtable == reinterpret_cast<void**>(g_main_base + kSteamBackendVtableRva);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool ObserveStableRoster() noexcept
{
    std::array<std::uint64_t, 4> identities{};
    std::size_t count = 0;
    __try
    {
        void* manager = *reinterpret_cast<void**>(g_main_base + kManagerSlotRva);
        auto* sync = manager ? static_cast<std::uint8_t*>(manager) + kSyncManagerOffset : nullptr;
        auto** first = sync ? *reinterpret_cast<void***>(sync) : nullptr;
        auto** last = sync ? *reinterpret_cast<void***>(sync + 4) : nullptr;
        auto** capacity = sync ? *reinterpret_cast<void***>(sync + 8) : nullptr;
        if (!first || !last || !capacity || last < first || capacity < last)
            return false;
        const std::ptrdiff_t raw_count = last - first;
        if (raw_count < 2 || raw_count > 4)
            return false;
        count = static_cast<std::size_t>(raw_count);
        for (std::size_t index = 0; index < count; ++index)
        {
            auto* device = static_cast<std::uint8_t*>(first[index]);
            auto* identity = device ? *reinterpret_cast<std::uint8_t**>(device + 0x374) : nullptr;
            if (!identity)
                return false;
            identities[index] = *reinterpret_cast<std::uint64_t*>(identity + 8);
            if (!identities[index])
                return false;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
    std::sort(identities.begin(), identities.begin() + count);
    if (std::adjacent_find(identities.begin(), identities.begin() + count) !=
        identities.begin() + count)
        return false;
    if (count == g_roster_candidate_count &&
        std::equal(identities.begin(), identities.begin() + count, g_roster_candidate.begin()))
    {
        if (g_roster_stable_observations < 2)
            ++g_roster_stable_observations;
    }
    else
    {
        g_roster_candidate = identities;
        g_roster_candidate_count = count;
        g_roster_stable_observations = 1;
    }
    return g_roster_stable_observations >= 2;
}

void ReportUnsafe(ErrorCode error, std::uint32_t flag) noexcept
{
    if (InterlockedExchange(&g_unsafe_reported, 1) != 0)
        return;
    NativeEvent event = Event(EventKind::Failure, g_depth);
    event.flags = flag;
    event.error_code = static_cast<std::uint32_t>(error);
    Publish(event);
    SetState(error, NativeState::Unsafe);
}

bool PermitGameEvent(bool establish_main_thread) noexcept
{
    if (InterlockedCompareExchange(&g_armed, 0, 0) == 0 ||
        InterlockedCompareExchange(&g_retired, 0, 0) != 0 ||
        InterlockedCompareExchange(&g_unloading, 0, 0) != 0 ||
        InterlockedCompareExchange(&g_unsafe_reported, 0, 0) != 0)
        return false;
    const LONG thread = static_cast<LONG>(GetCurrentThreadId());
    LONG main_thread = InterlockedCompareExchange(&g_shared->main_thread_id, 0, 0);
    if (main_thread == 0)
    {
        if (!establish_main_thread)
            return false;
        main_thread = InterlockedCompareExchange(&g_shared->main_thread_id, thread, 0);
    }
    if (main_thread != 0 && main_thread != thread)
    {
        ReportUnsafe(ErrorCode::WrongThread, kFlagWrongThread);
        return false;
    }
    if (IsSteamBackend() && ObserveStableRoster())
    {
        ReportUnsafe(ErrorCode::OriginalOnline, kFlagOriginalOnline);
        return false;
    }
    return true;
}

void PublishEnter(EventKind kind, const Snapshot& before, void* return_address) noexcept
{
    NativeEvent event = Event(kind, ++g_depth);
    event.return_rva = ReturnRva(return_address);
    FillSnapshot(event, before, before);
    Publish(event);
}

void PublishExit(EventKind kind, const Snapshot& before, const Snapshot& after,
                 void* return_address) noexcept
{
    NativeEvent event = Event(kind, g_depth);
    event.return_rva = ReturnRva(return_address);
    event.original_calls = 1;
    FillSnapshot(event, before, after);
    Publish(event);
    if (g_depth)
        --g_depth;
}

template <typename Function>
void InvokeThisVoid(Function original, void* self, EventKind enter, EventKind exit,
                    void* return_address, bool establish_main_thread = false) noexcept
{
    HookScope scope;
    if (!PermitGameEvent(establish_main_thread))
    {
        original(self);
        return;
    }
    const Snapshot before = ReadSnapshot();
    PublishEnter(enter, before, return_address);
    original(self);
    const Snapshot after = ReadSnapshot();
    PublishExit(exit, before, after, return_address);
}

template <typename Function>
void InvokeCallback(Function original, EventKind enter, EventKind exit,
                    void* return_address) noexcept
{
    HookScope scope;
    if (!PermitGameEvent(false))
    {
        original();
        return;
    }
    const Snapshot before = ReadSnapshot();
    PublishEnter(enter, before, return_address);
    original();
    const Snapshot after = ReadSnapshot();
    PublishExit(exit, before, after, return_address);
}

bool InstallHooks() noexcept
{
    const std::array<void*, 12> replacements{{
        reinterpret_cast<void*>(&InputSampleHook), reinterpret_cast<void*>(&GameplayHook),
        reinterpret_cast<void*>(&EntityHook), reinterpret_cast<void*>(&RemoveHook),
        reinterpret_cast<void*>(&PostUpdateHook), reinterpret_cast<void*>(&RenderHook),
        reinterpret_cast<void*>(&PostRenderHook), reinterpret_cast<void*>(&OnlineDispatchHook),
        reinterpret_cast<void*>(&SpawnHook), reinterpret_cast<void*>(&RunTeardownHook),
        reinterpret_cast<void*>(&RunTeardownHook), reinterpret_cast<void*>(&NewRoomHook),
    }};
    for (std::size_t index = 0; index < g_relative_patches.size(); ++index)
    {
        if (!ValidateCallTarget(index))
            return false;
        auto& patch = g_relative_patches[index];
        patch.call = g_main_base + kCallRvas[index];
        std::copy(kCallBytes[index].begin(), kCallBytes[index].end(), patch.expected);
        patch.replacement = replacements[index];
    }
    void** lua_newstate = isaac_cs_t0::mode::FindIatSlot(g_main_module, "lua_newstate");
    void** lua_lnewstate = isaac_cs_t0::mode::FindIatSlot(g_main_module, "luaL_newstate");
    void** lua_close = isaac_cs_t0::mode::FindIatSlot(g_main_module, "lua_close");
    void** lua_pcall = isaac_cs_t0::mode::FindIatSlot(g_main_module, "lua_pcallk");
    if (!lua_newstate || !lua_lnewstate || !lua_close || !lua_pcall ||
        !isaac_cs_t0::mode::ValidateIatCallsite(g_main_base + kLuaNewstateCallRva,
                                                lua_newstate) ||
        !isaac_cs_t0::mode::ValidateIatCallsite(g_main_base + kLuaLNewstateCallRva,
                                                lua_lnewstate) ||
        !isaac_cs_t0::mode::ValidateIatCallsite(g_main_base + kLuaCloseCallRva, lua_close) ||
        !isaac_cs_t0::mode::ValidateIatCallsite(g_main_base + kLuaPcallCallRva, lua_pcall) ||
        !isaac_cs_t0::mode::IsAddressOwnedBy(*lua_newstate, L"Lua5.3.3r.dll") ||
        !isaac_cs_t0::mode::IsAddressOwnedBy(*lua_lnewstate, L"Lua5.3.3r.dll") ||
        !isaac_cs_t0::mode::IsAddressOwnedBy(*lua_close, L"Lua5.3.3r.dll") ||
        !isaac_cs_t0::mode::IsAddressOwnedBy(*lua_pcall, L"Lua5.3.3r.dll"))
        return false;

    // Bind every original before any call displacement or IAT slot can become visible.
    // InstallPatchTransaction resumes the game threads before it returns, so assigning these
    // afterward leaves a real window where a high-frequency wrapper can call a null pointer.
    g_input_sample = reinterpret_cast<ThisVoidFn>(g_main_base + kTargetRvas[0]);
    g_gameplay = reinterpret_cast<ThisVoidFn>(g_main_base + kTargetRvas[1]);
    g_entity = reinterpret_cast<ThisVoidFn>(g_main_base + kTargetRvas[2]);
    g_remove = reinterpret_cast<ThisVoidFn>(g_main_base + kTargetRvas[3]);
    g_post_update = reinterpret_cast<CallbackFn>(g_main_base + kTargetRvas[4]);
    g_render = reinterpret_cast<ThisVoidFn>(g_main_base + kTargetRvas[5]);
    g_post_render = reinterpret_cast<CallbackFn>(g_main_base + kTargetRvas[6]);
    g_online_dispatch = reinterpret_cast<DispatchFn>(g_main_base + kTargetRvas[7]);
    g_spawn = reinterpret_cast<SpawnFn>(g_main_base + kTargetRvas[8]);
    g_run_teardown = reinterpret_cast<LifecycleFn>(g_main_base + kTargetRvas[9]);
    g_new_room = reinterpret_cast<CallbackFn>(g_main_base + kTargetRvas[11]);
    g_lua_newstate = reinterpret_cast<LuaNewstateFn>(*lua_newstate);
    g_lua_lnewstate = reinterpret_cast<LuaLNewstateFn>(*lua_lnewstate);
    g_lua_close = reinterpret_cast<LuaCloseFn>(*lua_close);
    g_lua_pcall = reinterpret_cast<LuaPcallFn>(*lua_pcall);
    g_iat_patches = {{
        {lua_newstate, reinterpret_cast<void*>(&LuaNewstateHook), nullptr, L"Lua5.3.3r.dll", false},
        {lua_lnewstate, reinterpret_cast<void*>(&LuaLNewstateHook), nullptr, L"Lua5.3.3r.dll", false},
        {lua_close, reinterpret_cast<void*>(&LuaCloseHook), nullptr, L"Lua5.3.3r.dll", false},
        {lua_pcall, reinterpret_cast<void*>(&LuaPcallHook), nullptr, L"Lua5.3.3r.dll", false},
    }};
    if (!isaac_cs_t0::mode::InstallPatchTransaction(
            g_relative_patches.data(), g_relative_patches.size(), g_iat_patches.data(),
            g_iat_patches.size()))
        return false;
    const bool relative_owners_match =
        std::equal(g_relative_patches.begin(), g_relative_patches.end(), kTargetRvas.begin(),
                   [](const auto& patch, std::uint32_t target_rva) {
                       return patch.original == g_main_base + target_rva;
                   });
    const bool iat_owners_match =
        g_iat_patches[0].original == reinterpret_cast<void*>(g_lua_newstate) &&
        g_iat_patches[1].original == reinterpret_cast<void*>(g_lua_lnewstate) &&
        g_iat_patches[2].original == reinterpret_cast<void*>(g_lua_close) &&
        g_iat_patches[3].original == reinterpret_cast<void*>(g_lua_pcall);
    if (!relative_owners_match || !iat_owners_match)
    {
        isaac_cs_t0::mode::RestorePatchTransaction(
            g_relative_patches.data(), g_relative_patches.size(), g_iat_patches.data(),
            g_iat_patches.size());
        return false;
    }
    InterlockedExchange(&g_shared->hooks_mask, 0xFFFF);
    return true;
}

bool AnyHookInstalled() noexcept
{
    return std::any_of(g_relative_patches.begin(), g_relative_patches.end(),
                       [](const auto& patch) { return patch.installed; }) ||
           std::any_of(g_iat_patches.begin(), g_iat_patches.end(),
                       [](const auto& patch) { return patch.installed; });
}

bool RestoreHooks() noexcept
{
    const bool restored = isaac_cs_t0::mode::RestorePatchTransaction(
        g_relative_patches.data(), g_relative_patches.size(), g_iat_patches.data(),
        g_iat_patches.size());
    if (restored && g_shared)
        InterlockedExchange(&g_shared->hooks_mask, 0);
    return restored;
}

extern "C" void __fastcall InputSampleHook(void* self, void*)
{
    HookScope scope;
    if (!PermitGameEvent(true))
    {
        g_input_sample(self);
        return;
    }
    std::uint32_t before_flags = 0;
    std::uint32_t before_state = kUnavailable;
    if (!ReadMainLoopObservation(self, before_flags, before_state))
    {
        ReportUnsafe(ErrorCode::MainLoopObservation, 0);
        g_input_sample(self);
        return;
    }
    void* const return_address = _ReturnAddress();
    const Snapshot before = ReadSnapshot();
    NativeEvent enter = Event(EventKind::InputSampleEnter, ++g_depth);
    enter.return_rva = ReturnRva(return_address);
    enter.flags |= before_flags;
    enter.result_value = before_state;
    FillSnapshot(enter, before, before);
    Publish(enter);

    g_input_sample(self);

    std::uint32_t after_flags = 0;
    std::uint32_t after_state = kUnavailable;
    const bool after_valid = ReadMainLoopObservation(self, after_flags, after_state);
    const Snapshot after = ReadSnapshot();
    NativeEvent exit = Event(EventKind::InputSampleExit, g_depth);
    exit.return_rva = ReturnRva(return_address);
    exit.original_calls = 1;
    if (after_valid)
    {
        exit.flags |= after_flags;
        exit.result_value = after_state;
    }
    FillSnapshot(exit, before, after);
    Publish(exit);
    if (g_depth)
        --g_depth;
    if (!after_valid)
        ReportUnsafe(ErrorCode::MainLoopObservation, 0);
}

extern "C" void __fastcall GameplayHook(void* self, void*)
{
    HookScope scope;
    if (!PermitGameEvent(true))
    {
        g_gameplay(self);
        return;
    }
    NativeEvent apply = Event(EventKind::InputApply, g_depth);
    apply.return_rva = ReturnRva(_ReturnAddress());
    Publish(apply);
    const Snapshot before = ReadSnapshot();
    PublishEnter(EventKind::GameplayEnter, before, _ReturnAddress());
    g_gameplay(self);
    const Snapshot after = ReadSnapshot();
    PublishExit(EventKind::GameplayExit, before, after, _ReturnAddress());
}

extern "C" void __fastcall EntityHook(void* self, void*)
{
    InvokeThisVoid(g_entity, self, EventKind::EntityEnter, EventKind::EntityExit,
                   _ReturnAddress());
}

extern "C" void __fastcall RemoveHook(void* self, void*)
{
    InvokeThisVoid(g_remove, self, EventKind::RemoveEnter, EventKind::RemoveExit,
                   _ReturnAddress());
}

void __cdecl PostUpdateHook()
{
    InvokeCallback(g_post_update, EventKind::PostUpdateEnter, EventKind::PostUpdateExit,
                   _ReturnAddress());
}

extern "C" void __fastcall RenderHook(void* self, void*)
{
    InvokeThisVoid(g_render, self, EventKind::RenderEnter, EventKind::RenderExit,
                   _ReturnAddress());
}

void __cdecl PostRenderHook()
{
    InvokeCallback(g_post_render, EventKind::PostRenderEnter, EventKind::PostRenderExit,
                   _ReturnAddress());
}

void __cdecl NewRoomHook()
{
    InvokeCallback(g_new_room, EventKind::NewRoomEnter, EventKind::NewRoomExit,
                   _ReturnAddress());
}

extern "C" std::uint32_t __fastcall OnlineDispatchHook(void* self, void*,
                                                        std::uint32_t argument)
{
    HookScope scope;
    if (!PermitGameEvent(false))
        return g_online_dispatch(self, argument);
    const Snapshot before = ReadSnapshot();
    PublishEnter(EventKind::OnlineDispatchEnter, before, _ReturnAddress());
    const std::uint32_t result = g_online_dispatch(self, argument);
    const Snapshot after = ReadSnapshot();
    NativeEvent event = Event(EventKind::OnlineDispatchExit, g_depth);
    event.return_rva = ReturnRva(_ReturnAddress());
    event.original_calls = 1;
    event.result_value = result;
    FillSnapshot(event, before, after);
    Publish(event);
    if (g_depth)
        --g_depth;
    return result;
}

extern "C" void* __fastcall SpawnHook(void* self, void*, std::uintptr_t type,
                                      std::uintptr_t variant, std::uintptr_t subtype,
                                      std::uintptr_t position, std::uintptr_t velocity,
                                      std::uintptr_t spawner, std::uintptr_t seed,
                                      std::uintptr_t zero_sentinel)
{
    HookScope scope;
    if (!PermitGameEvent(false))
        return g_spawn(self, type, variant, subtype, position, velocity, spawner, seed,
                       zero_sentinel);
    const Snapshot before = ReadSnapshot();
    PublishEnter(EventKind::SpawnEnter, before, _ReturnAddress());
    void* const result = g_spawn(self, type, variant, subtype, position, velocity, spawner, seed,
                                 zero_sentinel);
    const Snapshot after = ReadSnapshot();
    PublishExit(EventKind::SpawnExit, before, after, _ReturnAddress());
    return result;
}

std::uint32_t __stdcall RunTeardownHook(std::uint32_t argument)
{
    HookScope scope;
    if (InterlockedCompareExchange(&g_armed, 0, 0) == 0)
        return g_run_teardown(argument);
    static_cast<void>(PermitGameEvent(false));
    const LONG first = InterlockedExchange(&g_retired, 1);
    const std::uint32_t result = g_run_teardown(argument);
    if (first == 0)
    {
        InterlockedExchange64(&g_shared->retired_generation, g_shared->generation);
        NativeEvent event = Event(EventKind::Retired, g_depth);
        event.flags = kFlagLifecycleRetired | kFlagRetirementMenuReturn;
        event.original_calls = 1;
        event.result_value = result;
        Publish(event);
    }
    return result;
}

void PublishLuaCreated(void* state, std::uint32_t return_rva,
                       std::uint32_t creator_kind) noexcept
{
    if (InterlockedCompareExchange(&g_lua_rebuild_pending, 0, 0) == 0)
        return;
    const LONG main_thread = InterlockedCompareExchange(&g_shared->main_thread_id, 0, 0);
    if (!state || main_thread == 0 || main_thread != static_cast<LONG>(GetCurrentThreadId()))
    {
        ReportUnsafe(ErrorCode::LuaLifecycleMismatch,
                     main_thread != 0 ? kFlagWrongThread : 0);
        return;
    }
    const auto old_generation = static_cast<std::uint64_t>(
        InterlockedCompareExchange64(&g_shared->retired_generation, 0, 0));
    if (old_generation == 0 || !g_closed_lua_root)
    {
        ReportUnsafe(ErrorCode::LuaLifecycleMismatch, 0);
        return;
    }
    const auto new_generation = static_cast<std::uint64_t>(
        InterlockedIncrement64(&g_shared->generation));
    g_lua_root = state;
    NativeEvent created = Event(EventKind::LuaCreated, g_depth);
    created.generation = new_generation;
    created.flags = kFlagLuaNewRoot;
    created.return_rva = return_rva;
    created.original_calls = 1;
    created.result_value = creator_kind;
    Publish(created);

    NativeEvent retired = Event(EventKind::Retired, g_depth);
    retired.generation = old_generation;
    retired.flags = kFlagLifecycleRetired | kFlagRetirementLuaRebuild;
    retired.result_value = static_cast<std::uint32_t>(new_generation);
    Publish(retired);
    InterlockedExchange(&g_lua_rebuild_pending, 0);
}

void* __cdecl LuaNewstateHook(void* allocator, void* user_data)
{
    HookScope scope;
    const std::uint32_t return_rva = ReturnRva(_ReturnAddress());
    const bool outermost = g_lua_create_depth++ == 0;
    void* const state = g_lua_newstate(allocator, user_data);
    --g_lua_create_depth;
    if (outermost && return_rva == kLuaNewstateReturnRva)
        PublishLuaCreated(state, return_rva, 1);
    else if (outermost && InterlockedCompareExchange(&g_lua_rebuild_pending, 0, 0) != 0)
        ReportUnsafe(ErrorCode::LuaLifecycleMismatch, 0);
    return state;
}

void* __cdecl LuaLNewstateHook()
{
    HookScope scope;
    const std::uint32_t return_rva = ReturnRva(_ReturnAddress());
    const bool outermost = g_lua_create_depth++ == 0;
    void* const state = g_lua_lnewstate();
    --g_lua_create_depth;
    if (outermost && return_rva == kLuaLNewstateReturnRva)
        PublishLuaCreated(state, return_rva, 2);
    else if (outermost && InterlockedCompareExchange(&g_lua_rebuild_pending, 0, 0) != 0)
        ReportUnsafe(ErrorCode::LuaLifecycleMismatch, 0);
    return state;
}

void __cdecl LuaCloseHook(void* state)
{
    HookScope scope;
    if (InterlockedCompareExchange(&g_armed, 0, 0) == 0)
    {
        g_lua_close(state);
        return;
    }
    const std::uint32_t return_rva = ReturnRva(_ReturnAddress());
    const bool permitted = PermitGameEvent(false);
    const bool root_match = state && return_rva == kLuaCloseReturnRva &&
                            (!g_lua_root || state == g_lua_root);
    if (!permitted || !root_match)
    {
        g_lua_close(state);
        ReportUnsafe(ErrorCode::LuaLifecycleMismatch, 0);
        return;
    }
    const LONG first = InterlockedExchange(&g_retired, 1);
    const auto old_generation = static_cast<std::uint64_t>(
        InterlockedCompareExchange64(&g_shared->generation, 0, 0));
    g_closed_lua_root = state;
    InterlockedExchange64(&g_shared->retired_generation,
                          static_cast<LONG64>(old_generation));
    if (g_shared->scenario == kLuaRebuildScenario)
    {
        if (InterlockedExchange(&g_lua_rebuild_pending, 1) != 0)
        {
            g_lua_close(state);
            ReportUnsafe(ErrorCode::LuaLifecycleMismatch, 0);
            return;
        }
        InterlockedExchange(&g_armed, 0);
    }
    g_lua_close(state);
    if (first == 0)
    {
        NativeEvent close = Event(EventKind::LuaClose, g_depth);
        close.generation = old_generation;
        close.flags = kFlagLifecycleRetired | kFlagRetirementLuaClose |
                      kFlagLuaRootMatch;
        close.return_rva = return_rva;
        close.original_calls = 1;
        Publish(close);
        g_lua_root = nullptr;
        if (g_shared->scenario == kLuaRebuildScenario)
            return;
        NativeEvent retired = Event(EventKind::Retired, g_depth);
        retired.generation = old_generation;
        retired.flags = kFlagLifecycleRetired | kFlagRetirementLuaClose;
        Publish(retired);
    }
}

int __cdecl LuaPcallHook(void* state, int nargs, int nresults, int errfunc,
                         std::intptr_t context, void* continuation)
{
    HookScope scope;
    const std::uint32_t return_rva = ReturnRva(_ReturnAddress());
    if (return_rva == kLuaPcallReturnRva && state && !g_lua_root)
        g_lua_root = state;
    if (return_rva != kLuaPcallReturnRva || !PermitGameEvent(false))
        return g_lua_pcall(state, nargs, nresults, errfunc, context, continuation);
    const Snapshot before = ReadSnapshot();
    PublishEnter(EventKind::LuaCallbackEnter, before, _ReturnAddress());
    const int result = g_lua_pcall(state, nargs, nresults, errfunc, context, continuation);
    const Snapshot after = ReadSnapshot();
    NativeEvent event = Event(EventKind::LuaCallbackExit, g_depth);
    event.return_rva = return_rva;
    event.original_calls = 1;
    event.result_value = static_cast<std::uint32_t>(result);
    FillSnapshot(event, before, after);
    Publish(event);
    if (g_depth)
        --g_depth;
    return result;
}

} // namespace

void SetModule(HMODULE module) noexcept
{
    g_module = module;
}

void ProcessDetach() noexcept
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

extern "C" DWORD WINAPI IsaacCsT0BoundaryInitialize(const wchar_t* mapping_name)
{
    if (!mapping_name || !*mapping_name || g_shared)
        return static_cast<DWORD>(ErrorCode::MappingInvalid);
    g_mapping = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, mapping_name);
    if (!g_mapping)
        return static_cast<DWORD>(ErrorCode::MappingInvalid);
    g_shared = static_cast<SharedBoundaryBlock*>(MapViewOfFile(
        g_mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedBoundaryBlock)));
    if (!ValidateMapping())
    {
        SetState(ErrorCode::MappingInvalid, NativeState::Failed);
        return static_cast<DWORD>(ErrorCode::MappingInvalid);
    }
    InterlockedExchange(&g_shared->state, static_cast<LONG>(NativeState::Initializing));
    InterlockedExchange64(&g_shared->generation, 1);
    g_main_module = GetModuleHandleW(nullptr);
    g_main_base = reinterpret_cast<std::uint8_t*>(g_main_module);
    if (!IsTargetImage())
    {
        SetState(ErrorCode::TargetImageMismatch, NativeState::Failed);
        return static_cast<DWORD>(ErrorCode::TargetImageMismatch);
    }
    if (HasConflictingRuntime())
    {
        SetState(ErrorCode::ConflictingModuleOrDebugger, NativeState::Failed);
        return static_cast<DWORD>(ErrorCode::ConflictingModuleOrDebugger);
    }
    Publish(Event(EventKind::NativeInitialized));
    if (!InstallHooks())
    {
        const bool unsafe = AnyHookInstalled();
        const ErrorCode code = unsafe ? ErrorCode::UnsafeShutdown : ErrorCode::HookTransaction;
        SetState(code, unsafe ? NativeState::Unsafe : NativeState::Failed);
        NativeEvent event = Event(EventKind::Failure);
        event.error_code = static_cast<std::uint32_t>(code);
        Publish(event);
        return static_cast<DWORD>(code);
    }
    Publish(Event(EventKind::HooksInstalled));
    InterlockedExchange(&g_armed, 1);
    InterlockedExchange(&g_shared->state, static_cast<LONG>(NativeState::Ready));
    return 0;
}

extern "C" DWORD WINAPI IsaacCsT0BoundaryShutdown(void*)
{
    if (!g_shared || InterlockedExchange(&g_unloading, 1) != 0)
        return static_cast<DWORD>(ErrorCode::UnsafeShutdown);
    InterlockedExchange(&g_shared->state, static_cast<LONG>(NativeState::ShuttingDown));
    InterlockedExchange(&g_armed, 0);
    if (!RestoreHooks())
    {
        SetState(ErrorCode::UnsafeShutdown, NativeState::Unsafe);
        InterlockedExchange(&g_unloading, 0);
        return static_cast<DWORD>(ErrorCode::UnsafeShutdown);
    }
    const ULONGLONG deadline = GetTickCount64() + kHookDrainTimeoutMs;
    while (InterlockedCompareExchange(&g_active_hooks, 0, 0) != 0 &&
           GetTickCount64() < deadline)
        Sleep(1);
    if (InterlockedCompareExchange(&g_active_hooks, 0, 0) != 0)
    {
        SetState(ErrorCode::HookDrainTimeout, NativeState::Unsafe);
        return static_cast<DWORD>(ErrorCode::HookDrainTimeout);
    }
    Publish(Event(EventKind::HooksRestored));
    InterlockedExchange(&g_shared->producer_closed, 1);
    InterlockedExchange(&g_shared->state, static_cast<LONG>(NativeState::Stopped));
    HMODULE module = g_module;
    FreeLibraryAndExitThread(module, 0);
}

} // namespace isaac_cs_t0::boundary
