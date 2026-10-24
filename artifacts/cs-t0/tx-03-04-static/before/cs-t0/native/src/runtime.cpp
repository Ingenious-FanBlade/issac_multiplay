#include "isaac_cs_t0/runtime.hpp"

#include "isaac_cs_t0/hooks.hpp"
#include "isaac_cs_t0/lifecycle.hpp"
#include "isaac_cs_t0/mode_ipc.hpp"
#include "isaac_cs_t0/mode_latch.hpp"
#include "isaac_cs_t0/sha256.hpp"

#include <Windows.h>
#include <intrin.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>

namespace isaac_cs_t0::mode {
namespace {

constexpr std::uint32_t kExpectedTimestamp = 0x69E6E3A7u;
constexpr std::uint32_t kExpectedImageSize = 0x0092C000u;
constexpr std::uint32_t kFrameCallRva = 0x0050B8E5u;
constexpr std::uint32_t kDispatchCallRva = 0x005110ABu;
constexpr std::uint32_t kManagerSlotRva = 0x0087169Cu;
constexpr std::uint32_t kNetworkBackendSlotRva = 0x00879A58u;
constexpr std::uint32_t kSteamBackendVtableRva = 0x007A2440u;
constexpr std::uint32_t kSteamContextDescriptorRva = 0x0085C51Cu;
constexpr std::uint32_t kSteamContextInitializerRva = 0x00676090u;
constexpr std::uint32_t kLuaNewstateCallRva = 0x0046057Bu;
constexpr std::uint32_t kLuaLNewstateCallRva = 0x004605A3u;
constexpr std::uint32_t kLuaCloseCallRva = 0x0046E5C0u;
constexpr std::uint32_t kLuaPcallCallRva = 0x004729CBu;
constexpr std::uint32_t kLuaPcallReturnRva = 0x004729D1u;
constexpr std::uint32_t kGameSlotRva = 0x00871678u;
constexpr std::uint32_t kRunActiveOffset = 0x0002658Au;
constexpr std::uint32_t kPreGameExitCallRva = 0x002FA0E2u;
constexpr std::uint32_t kPreGameExitDispatcherRva = 0x004650A0u;
constexpr std::uint32_t kSyncManagerOffset = 0x0004B3D8u;
constexpr std::uint32_t kOnlineFrameOffset = 0x0Cu;
constexpr std::size_t kMaximumPayloadHash = 1024u * 1024u;
constexpr DWORD kHookDrainTimeoutMs = 5000;

constexpr std::array<std::uint8_t, 5> kFrameBytes{{0xE8, 0x96, 0x50, 0x00, 0x00}};
constexpr std::array<std::uint8_t, 5> kDispatchBytes{{0xE8, 0x90, 0xC5, 0xFF, 0xFF}};
constexpr std::array<std::uint32_t, 6> kPostGameStartCallRvas{{
    0x002F580Bu, 0x002F61FFu, 0x002F72C9u,
    0x002F7721u, 0x002F7A10u, 0x00559B82u,
}};
constexpr std::array<std::array<std::uint8_t, 5>, 6> kPostGameStartCallBytes{{
    {{0xE8, 0xD0, 0xF6, 0x16, 0x00}},
    {{0xE8, 0xDC, 0xEC, 0x16, 0x00}},
    {{0xE8, 0x12, 0xDC, 0x16, 0x00}},
    {{0xE8, 0xBA, 0xD7, 0x16, 0x00}},
    {{0xE8, 0xCB, 0xD4, 0x16, 0x00}},
    {{0xE8, 0x59, 0xB3, 0xF0, 0xFF}},
}};
constexpr std::array<std::uint32_t, 2> kRunTeardownCallRvas{{
    0x00559696u, 0x005AB84Au,
}};
constexpr std::array<std::array<std::uint8_t, 5>, 2> kRunTeardownCallBytes{{
    {{0xE8, 0x25, 0x0A, 0xDA, 0xFF}},
    {{0xE8, 0x71, 0xE8, 0xD4, 0xFF}},
}};
constexpr std::array<std::uint8_t, 5> kPreGameExitCallBytes{{
    0xE8, 0xB9, 0xAF, 0x16, 0x00,
}};

enum class ErrorCode : LONG {
    None = 0,
    MappingInvalid = 1,
    TargetImageMismatch = 2,
    ConflictingModuleOrDebugger = 3,
    FixedHookTransaction = 4,
    SteamHookTransaction = 5,
    EventRingOverflow = 6,
    ModeConflict = 7,
    UnsafeShutdown = 8,
    CustomModeDisarmed = 9,
    HookDrainTimeout = 10,
};

using FrameFn = bool(__thiscall*)(void*);
using DispatchFn = std::uint32_t(__thiscall*)(void*, std::uint32_t);
using LifecycleFn = std::uint32_t(__stdcall*)(std::uint32_t);
using LuaNewstateFn = void*(__cdecl*)(void*, void*);
using LuaLNewstateFn = void*(__cdecl*)();
using LuaCloseFn = void(__cdecl*)(void*);
using LuaPcallFn = int(__cdecl*)(void*, int, int, int, std::intptr_t, void*);
using SteamContextInitFn = void*(__cdecl*)(void*);
using SteamSendFn = bool(__thiscall*)(void*, std::uint64_t, const void*, std::uint32_t, int, int);
using SteamAvailableFn = bool(__thiscall*)(void*, std::uint32_t*, int);
using SteamReadFn = bool(__thiscall*)(void*, void*, std::uint32_t, std::uint32_t*,
                                      std::uint64_t*, int);

HMODULE g_module = nullptr;
HMODULE g_main_module = nullptr;
std::uint8_t* g_main_base = nullptr;
HANDLE g_mapping = nullptr;
SharedModeBlock* g_shared = nullptr;

std::optional<ModeLatch> g_latch;
void* g_lua_root = nullptr;
void* g_session_manager = nullptr;
void* g_generation_manager = nullptr;
std::uint32_t g_last_frame = UINT32_MAX;
std::array<std::uint64_t, 4> g_roster_candidate{};
std::size_t g_roster_candidate_count = 0;
std::uint32_t g_roster_stable_observations = 0;
std::uint32_t g_missing_manager_callbacks = 0;

volatile LONG g_unloading = 0;
volatile LONG g_active_hooks = 0;
volatile LONG g_channel_zero_since_evaluate = 0;
volatile LONG g_nonzero_channel_since_evaluate = 0;
volatile LONG g_context_state = 0;
volatile LONG64 g_attempt_counter = 0;
volatile LONG64 g_generation_token_counter = 0;
volatile LONG64 g_lifecycle_generation_counter = 0;
volatile LONG64 g_current_lifecycle_generation = 0;
thread_local std::uint64_t g_current_attempt = 0;
thread_local std::uint32_t g_lua_create_depth = 0;

FrameFn g_frame = nullptr;
DispatchFn g_dispatch = nullptr;
LifecycleFn g_post_game_start = nullptr;
LifecycleFn g_run_teardown = nullptr;
LuaNewstateFn g_lua_newstate = nullptr;
LuaLNewstateFn g_lua_lnewstate = nullptr;
LuaCloseFn g_lua_close = nullptr;
LuaPcallFn g_lua_pcall = nullptr;
SteamContextInitFn g_steam_context = nullptr;
SteamSendFn g_steam_send = nullptr;
SteamAvailableFn g_steam_available = nullptr;
SteamReadFn g_steam_read = nullptr;
void* g_steam_interface = nullptr;
void** g_steam_vtable = nullptr;

std::array<RelativeCallPatch, 10> g_relative_patches{};
std::array<PointerPatch, 5> g_iat_patches{};
std::array<PointerPatch, 3> g_steam_patches{};

extern "C" bool __fastcall FrameHook(void* self, void* unused);
extern "C" std::uint32_t __fastcall DispatchHook(void* self, void* unused,
                                                  std::uint32_t argument);
std::uint32_t __stdcall PostGameStartHook(std::uint32_t continued);
std::uint32_t __stdcall RunTeardownHook(std::uint32_t argument);
void* __cdecl LuaNewstateHook(void* allocator, void* user_data);
void* __cdecl LuaLNewstateHook();
void __cdecl LuaCloseHook(void* state);
int __cdecl LuaPcallHook(void* state, int nargs, int nresults, int errfunc,
                         std::intptr_t context, void* continuation);
void* __cdecl SteamContextHook(void* init_data);
extern "C" bool __fastcall SteamSendHook(void* self, void* unused, std::uint64_t remote,
                                         const void* data, std::uint32_t size, int send_mode,
                                         int channel);
extern "C" bool __fastcall SteamAvailableHook(void* self, void* unused,
                                              std::uint32_t* size, int channel);
extern "C" bool __fastcall SteamReadHook(void* self, void* unused, void* data,
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

void SetError(ErrorCode error, NativeState state) noexcept
{
    if (!g_shared)
        return;
    InterlockedExchange(&g_shared->error_code, static_cast<LONG>(error));
    InterlockedExchange(&g_shared->state, static_cast<LONG>(state));
}

std::int64_t Qpc() noexcept
{
    LARGE_INTEGER value{};
    return QueryPerformanceCounter(&value) ? value.QuadPart : 0;
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
        SetError(ErrorCode::EventRingOverflow, NativeState::Failed);
    }
    NativeEvent& slot = g_shared->events[static_cast<std::size_t>(sequence - 1) % kEventCapacity];
    InterlockedExchange64(&slot.published_sequence, 0);
    constexpr std::size_t sequence_size = sizeof(NativeEvent::published_sequence);
    std::memcpy(reinterpret_cast<std::uint8_t*>(&slot) + sequence_size,
                reinterpret_cast<const std::uint8_t*>(&event) + sequence_size,
                sizeof(NativeEvent) - sequence_size);
    MemoryBarrier();
    InterlockedExchange64(&slot.published_sequence, sequence);
}

NativeEvent Event(NativeEventKind kind) noexcept
{
    NativeEvent event{};
    event.qpc = Qpc();
    event.thread_id = GetCurrentThreadId();
    event.kind = static_cast<std::uint32_t>(kind);
    event.mode = g_latch ? static_cast<std::uint32_t>(g_latch->State())
                         : static_cast<std::uint32_t>(NetworkMode::Unresolved);
    event.generation = g_latch ? g_latch->Generation() : 0;
    return event;
}

bool Nonzero(const std::array<std::uint8_t, 32>& bytes) noexcept
{
    for (const auto byte : bytes)
    {
        if (byte)
            return true;
    }
    return false;
}

bool ValidateMapping() noexcept
{
    if (!g_shared || g_shared->magic != kIpcMagic || g_shared->version != kIpcVersion ||
        g_shared->size != sizeof(SharedModeBlock) ||
        g_shared->target_pid != GetCurrentProcessId() || !Nonzero(g_shared->nonce) ||
        !Nonzero(g_shared->pairing_key))
        return false;
    const auto scenario = static_cast<Scenario>(g_shared->scenario);
    const auto requested = static_cast<RequestedMode>(g_shared->requested_mode);
    if (scenario == Scenario::OfflineDisarmed && requested != RequestedMode::Offline)
        return false;
    if (scenario == Scenario::OriginalOnline && requested != RequestedMode::OriginalOnline)
        return false;
    if (scenario == Scenario::ControlNoNative ||
        (scenario == Scenario::OfflineDisarmed &&
         g_shared->endpoint != static_cast<std::uint32_t>(EndpointRole::Solo)) ||
        (scenario == Scenario::OriginalOnline &&
         g_shared->endpoint != static_cast<std::uint32_t>(EndpointRole::A) &&
         g_shared->endpoint != static_cast<std::uint32_t>(EndpointRole::B)))
        return false;
    return requested == RequestedMode::Offline || requested == RequestedMode::OriginalOnline;
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

std::uint32_t ReadOnlineFrame(void* self) noexcept
{
    __try
    {
        return self ? *reinterpret_cast<std::uint32_t*>(
                          static_cast<std::uint8_t*>(self) + kOnlineFrameOffset)
                    : UINT32_MAX;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return UINT32_MAX;
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

bool ManagerIdentity(void* self) noexcept
{
    __try
    {
        void* manager = *reinterpret_cast<void**>(g_main_base + kManagerSlotRva);
        return manager && self == static_cast<std::uint8_t*>(manager) + kSyncManagerOffset &&
               IsSteamBackend();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

void* ReadManagerSlot() noexcept
{
    __try
    {
        return *reinterpret_cast<void**>(g_main_base + kManagerSlotRva);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return nullptr;
    }
}

struct RunActiveSnapshot {
    void* game = nullptr;
    std::uint32_t value = kLifecycleUnavailable;
};

RunActiveSnapshot ReadRunActive() noexcept
{
    RunActiveSnapshot snapshot{};
    __try
    {
        snapshot.game = *reinterpret_cast<void**>(g_main_base + kGameSlotRva);
        if (snapshot.game)
        {
            snapshot.value = *reinterpret_cast<std::uint16_t*>(
                static_cast<std::uint8_t*>(snapshot.game) + kRunActiveOffset);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        snapshot = {};
    }
    return snapshot;
}

bool ValidateCallTarget(std::uint32_t call_rva,
                        const std::array<std::uint8_t, 5>& expected,
                        std::uint32_t target_rva) noexcept
{
    __try
    {
        const auto* call = g_main_base + call_rva;
        if (std::memcmp(call, expected.data(), expected.size()) != 0 || call[0] != 0xE8)
            return false;
        const auto displacement = *reinterpret_cast<const std::int32_t*>(call + 1);
        return call + 5 + displacement == g_main_base + target_rva;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool SteamDescriptorMatches(void* init_data) noexcept
{
    __try
    {
        return init_data == g_main_base + kSteamContextDescriptorRva &&
               *reinterpret_cast<void**>(init_data) ==
                   g_main_base + kSteamContextInitializerRva;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

std::uint32_t ReadOptionalSize(const std::uint32_t* size) noexcept
{
    __try
    {
        return size ? *size : UINT32_MAX;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return UINT32_MAX;
    }
}

void ReadPacketOutputs(const std::uint32_t* size, const std::uint64_t* remote,
                       std::uint32_t& output_size, std::uint64_t& peer) noexcept
{
    output_size = UINT32_MAX;
    peer = 0;
    __try
    {
        output_size = size ? *size : UINT32_MAX;
        peer = remote ? *remote : 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        output_size = UINT32_MAX;
        peer = 0;
    }
}

bool ObserveStableRoster(void* self) noexcept
{
    std::array<std::uint64_t, 4> identities{};
    std::size_t count = 0;
    __try
    {
        auto** first = self ? *reinterpret_cast<void***>(self) : nullptr;
        auto** last = self ? *reinterpret_cast<void***>(static_cast<std::uint8_t*>(self) + 4)
                           : nullptr;
        auto** capacity = self ? *reinterpret_cast<void***>(static_cast<std::uint8_t*>(self) + 8)
                               : nullptr;
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
    for (std::size_t index = 1; index < count; ++index)
    {
        if (identities[index] == identities[index - 1])
            return false;
    }
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

bool WrongSimulationThread() noexcept
{
    if (!g_shared)
        return true;
    const LONG thread_id = static_cast<LONG>(GetCurrentThreadId());
    const LONG current = InterlockedCompareExchange(&g_shared->main_thread_id, thread_id, 0);
    return current != 0 && current != thread_id;
}

std::uint32_t ObservationFlags(const SignalObservation& signal) noexcept
{
    return (signal.manager_identity ? kEventManagerIdentity : 0u) |
           (signal.stable_roster ? kEventStableRoster : 0u) |
           (signal.channel_zero_activity ? kEventChannelZero : 0u) |
           (signal.wrong_thread ? kEventWrongThread : 0u) |
           (signal.frame_regressed ? kEventFrameRegressed : 0u) |
           (signal.manager_changed ? kEventManagerChanged : 0u) |
           (signal.lua_closed ? kEventLuaClosed : 0u) |
           (signal.returned_to_menu ? kEventMenuReturn : 0u) |
           (signal.nonzero_channel ? kEventNonzeroChannel : 0u);
}

void PublishTransition(const ModeTransition& transition, std::uint32_t flags) noexcept
{
    if (transition.kind == TransitionKind::None || !g_shared || !g_latch)
        return;
    InterlockedExchange(&g_shared->network_mode, static_cast<LONG>(g_latch->State()));
    InterlockedExchange64(&g_shared->generation, static_cast<LONG64>(g_latch->Generation()));
    NativeEvent event = Event(transition.kind == TransitionKind::Frozen
                                  ? NativeEventKind::ModeFrozen
                                  : NativeEventKind::ModeRetired);
    event.mode = static_cast<std::uint32_t>(g_latch->State());
    event.generation = g_latch->Generation();
    event.flags = flags;
    event.signal_mask = g_latch->OnlineSignalCount();
    Publish(event);
}

void Evaluate(SignalObservation signal) noexcept
{
    if (!g_latch || !g_latch->Token())
        return;
    signal.channel_zero_activity =
        InterlockedExchange(&g_channel_zero_since_evaluate, 0) > 0;
    signal.nonzero_channel = InterlockedExchange(&g_nonzero_channel_since_evaluate, 0) > 0;
    const std::uint32_t flags = ObservationFlags(signal);
    PublishTransition(g_latch->Observe(signal), flags);
}

void BeginLuaGeneration(void* state) noexcept
{
    if (!g_latch || g_latch->Token())
        return;
    if (!g_lua_root)
        g_lua_root = state;
    const auto counter = static_cast<std::uint64_t>(
        InterlockedIncrement64(&g_generation_token_counter));
    std::uint64_t token = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(g_lua_root));
    token ^= counter * 0x9E3779B97F4A7C15ull;
    if (!token)
        token = counter ? counter : 1;
    if (!g_latch->BeginGeneration(token))
    {
        SetError(ErrorCode::CustomModeDisarmed, NativeState::Failed);
        NativeEvent event = Event(NativeEventKind::Failure);
        event.error_code = static_cast<std::uint32_t>(ErrorCode::CustomModeDisarmed);
        Publish(event);
        return;
    }
    g_session_manager = nullptr;
    g_generation_manager = ReadManagerSlot();
    g_last_frame = UINT32_MAX;
    g_roster_candidate = {};
    g_roster_candidate_count = 0;
    g_roster_stable_observations = 0;
    g_missing_manager_callbacks = 0;
    InterlockedExchange64(&g_shared->generation, static_cast<LONG64>(g_latch->Generation()));
}

bool SafeHash(const void* data, std::uint32_t size, Sha256Digest& digest) noexcept
{
    digest = {};
    if (!data || !size || size > kMaximumPayloadHash)
        return false;
    __try
    {
        digest = Sha256(data, size);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        digest = {};
        return false;
    }
}

std::uint64_t PeerToken(std::uint64_t steam_id) noexcept
{
    if (!g_shared || !steam_id)
        return 0;
    const auto digest = Sha256Pair(g_shared->pairing_key.data(), g_shared->pairing_key.size(),
                                   &steam_id, sizeof(steam_id));
    std::uint64_t token = 0;
    std::memcpy(&token, digest.data(), sizeof(token));
    return token ? token : 1;
}

std::uint64_t ResolveLocalSteamToken() noexcept
{
    using GetUserFn = std::int32_t(__cdecl*)();
    using FindInterfaceFn = void*(__cdecl*)(std::int32_t, const char*);
    using GetSteamIdFn = std::uint64_t(__cdecl*)(void*);
    HMODULE steam_api = GetModuleHandleW(L"steam_api.dll");
    if (!steam_api)
        return 0;
    const auto get_user = reinterpret_cast<GetUserFn>(
        GetProcAddress(steam_api, "SteamAPI_GetHSteamUser"));
    const auto find_interface = reinterpret_cast<FindInterfaceFn>(
        GetProcAddress(steam_api, "SteamInternal_FindOrCreateUserInterface"));
    const auto get_id = reinterpret_cast<GetSteamIdFn>(
        GetProcAddress(steam_api, "SteamAPI_ISteamUser_GetSteamID"));
    if (!get_user || !find_interface || !get_id)
        return 0;
    const auto user = get_user();
    if (!user)
        return 0;
    constexpr std::array<const char*, 5> versions{{
        "SteamUser023", "SteamUser022", "SteamUser021", "SteamUser020", "SteamUser019",
    }};
    for (const char* version : versions)
    {
        void* interface_pointer = find_interface(user, version);
        if (interface_pointer)
        {
            const auto id = get_id(interface_pointer);
            if (id)
                return PeerToken(id);
        }
    }
    return 0;
}

bool ValidateFixedTargets() noexcept
{
    g_relative_patches[0].call = g_main_base + kFrameCallRva;
    std::copy(kFrameBytes.begin(), kFrameBytes.end(), g_relative_patches[0].expected);
    g_relative_patches[0].replacement = reinterpret_cast<void*>(&FrameHook);
    g_relative_patches[1].call = g_main_base + kDispatchCallRva;
    std::copy(kDispatchBytes.begin(), kDispatchBytes.end(), g_relative_patches[1].expected);
    g_relative_patches[1].replacement = reinterpret_cast<void*>(&DispatchHook);
    for (std::size_t index = 0; index < kPostGameStartCallRvas.size(); ++index)
    {
        auto& patch = g_relative_patches[index + 2];
        patch.call = g_main_base + kPostGameStartCallRvas[index];
        std::copy(kPostGameStartCallBytes[index].begin(), kPostGameStartCallBytes[index].end(),
                  patch.expected);
        patch.replacement = reinterpret_cast<void*>(&PostGameStartHook);
    }
    for (std::size_t index = 0; index < kRunTeardownCallRvas.size(); ++index)
    {
        auto& patch = g_relative_patches[index + 8];
        patch.call = g_main_base + kRunTeardownCallRvas[index];
        std::copy(kRunTeardownCallBytes[index].begin(), kRunTeardownCallBytes[index].end(),
                  patch.expected);
        patch.replacement = reinterpret_cast<void*>(&RunTeardownHook);
    }

    if (!ValidateCallTarget(kPreGameExitCallRva, kPreGameExitCallBytes,
                            kPreGameExitDispatcherRva))
        return false;

    void** newstate = FindIatSlot(g_main_module, "lua_newstate");
    void** lnewstate = FindIatSlot(g_main_module, "luaL_newstate");
    void** close = FindIatSlot(g_main_module, "lua_close");
    void** pcall = FindIatSlot(g_main_module, "lua_pcallk");
    void** context = FindIatSlot(g_main_module, "SteamInternal_ContextInit");
    if (!newstate || !lnewstate || !close || !pcall || !context ||
        !ValidateIatCallsite(g_main_base + kLuaNewstateCallRva, newstate) ||
        !ValidateIatCallsite(g_main_base + kLuaLNewstateCallRva, lnewstate) ||
        !ValidateIatCallsite(g_main_base + kLuaCloseCallRva, close) ||
        !ValidateIatCallsite(g_main_base + kLuaPcallCallRva, pcall))
        return false;
    g_iat_patches = {{
        {newstate, reinterpret_cast<void*>(&LuaNewstateHook), nullptr, L"Lua5.3.3r.dll", false},
        {lnewstate, reinterpret_cast<void*>(&LuaLNewstateHook), nullptr, L"Lua5.3.3r.dll", false},
        {close, reinterpret_cast<void*>(&LuaCloseHook), nullptr, L"Lua5.3.3r.dll", false},
        {pcall, reinterpret_cast<void*>(&LuaPcallHook), nullptr, L"Lua5.3.3r.dll", false},
        {context, reinterpret_cast<void*>(&SteamContextHook), nullptr, L"steam_api.dll", false},
    }};
    return true;
}

bool InstallFixedHooks() noexcept
{
    if (!ValidateFixedTargets() ||
        !InstallPatchTransaction(g_relative_patches.data(), g_relative_patches.size(),
                                 g_iat_patches.data(), g_iat_patches.size()))
        return false;
    g_frame = reinterpret_cast<FrameFn>(g_relative_patches[0].original);
    g_dispatch = reinterpret_cast<DispatchFn>(g_relative_patches[1].original);
    g_post_game_start = reinterpret_cast<LifecycleFn>(g_relative_patches[2].original);
    g_run_teardown = reinterpret_cast<LifecycleFn>(g_relative_patches[8].original);
    const bool lifecycle_owners_match =
        std::all_of(g_relative_patches.begin() + 2, g_relative_patches.begin() + 8,
                    [](const RelativeCallPatch& patch) {
                        return patch.original == reinterpret_cast<void*>(g_post_game_start);
                    }) &&
        std::all_of(g_relative_patches.begin() + 8, g_relative_patches.end(),
                    [](const RelativeCallPatch& patch) {
                        return patch.original == reinterpret_cast<void*>(g_run_teardown);
                    });
    if (!lifecycle_owners_match)
    {
        RestorePatchTransaction(g_relative_patches.data(), g_relative_patches.size(),
                                g_iat_patches.data(), g_iat_patches.size());
        return false;
    }
    g_lua_newstate = reinterpret_cast<LuaNewstateFn>(g_iat_patches[0].original);
    g_lua_lnewstate = reinterpret_cast<LuaLNewstateFn>(g_iat_patches[1].original);
    g_lua_close = reinterpret_cast<LuaCloseFn>(g_iat_patches[2].original);
    g_lua_pcall = reinterpret_cast<LuaPcallFn>(g_iat_patches[3].original);
    g_steam_context = reinterpret_cast<SteamContextInitFn>(g_iat_patches[4].original);
    InterlockedExchange(&g_shared->hooks_mask, 0x37Fu);
    return true;
}

bool AnyPatchInstalled() noexcept
{
    return std::any_of(g_relative_patches.begin(), g_relative_patches.end(),
                       [](const RelativeCallPatch& patch) { return patch.installed; }) ||
           std::any_of(g_iat_patches.begin(), g_iat_patches.end(),
                       [](const PointerPatch& patch) { return patch.installed; }) ||
           std::any_of(g_steam_patches.begin(), g_steam_patches.end(),
                       [](const PointerPatch& patch) { return patch.installed; });
}

bool AnySteamPatchInstalled() noexcept
{
    return std::any_of(g_steam_patches.begin(), g_steam_patches.end(),
                       [](const PointerPatch& patch) { return patch.installed; });
}

bool InstallSteamHooks(void* context) noexcept
{
    if (!context || InterlockedCompareExchange(&g_context_state, 1, 0) != 0)
        return InterlockedCompareExchange(&g_context_state, 0, 0) == 2;
    bool valid = false;
    __try
    {
        void* interface_pointer = *reinterpret_cast<void**>(context);
        void** vtable = interface_pointer ? *reinterpret_cast<void***>(interface_pointer) : nullptr;
        if (vtable && IsAddressOwnedBy(vtable[0], L"steamclient.dll") &&
            IsAddressOwnedBy(vtable[1], L"steamclient.dll") &&
            IsAddressOwnedBy(vtable[2], L"steamclient.dll"))
        {
            g_steam_interface = interface_pointer;
            g_steam_vtable = vtable;
            g_steam_patches = {{
                {&vtable[0], reinterpret_cast<void*>(&SteamSendHook), nullptr, L"steamclient.dll", false},
                {&vtable[1], reinterpret_cast<void*>(&SteamAvailableHook), nullptr, L"steamclient.dll", false},
                {&vtable[2], reinterpret_cast<void*>(&SteamReadHook), nullptr, L"steamclient.dll", false},
            }};
            valid = InstallPatchTransaction(nullptr, 0, g_steam_patches.data(),
                                            g_steam_patches.size());
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        valid = false;
    }
    if (!valid)
    {
        InterlockedExchange(&g_context_state, -1);
        const bool unsafe = AnySteamPatchInstalled();
        SetError(unsafe ? ErrorCode::UnsafeShutdown : ErrorCode::SteamHookTransaction,
                 unsafe ? NativeState::Unsafe : NativeState::Failed);
        NativeEvent event = Event(NativeEventKind::Failure);
        event.error_code = static_cast<std::uint32_t>(ErrorCode::SteamHookTransaction);
        Publish(event);
        return false;
    }
    g_steam_send = reinterpret_cast<SteamSendFn>(g_steam_patches[0].original);
    g_steam_available = reinterpret_cast<SteamAvailableFn>(g_steam_patches[1].original);
    g_steam_read = reinterpret_cast<SteamReadFn>(g_steam_patches[2].original);
    InterlockedOr(&g_shared->hooks_mask, static_cast<LONG>(1u << 7));
    InterlockedExchange64(&g_shared->local_peer_token,
                          static_cast<LONG64>(ResolveLocalSteamToken()));
    InterlockedExchange(&g_context_state, 2);
    Publish(Event(NativeEventKind::SteamHooksInstalled));
    return true;
}

bool RestoreAllHooks() noexcept
{
    bool restored = true;
    if (InterlockedCompareExchange(&g_context_state, 0, 0) == 2)
    {
        restored = RestorePatchTransaction(nullptr, 0, g_steam_patches.data(),
                                           g_steam_patches.size()) && restored;
    }
    restored = RestorePatchTransaction(g_relative_patches.data(), g_relative_patches.size(),
                                       g_iat_patches.data(), g_iat_patches.size()) && restored;
    if (restored)
        InterlockedExchange(&g_shared->hooks_mask, 0);
    return restored;
}

extern "C" bool __fastcall FrameHook(void* self, void*)
{
    HookScope scope;
    const std::uint64_t attempt = static_cast<std::uint64_t>(
        InterlockedIncrement64(&g_attempt_counter));
    g_current_attempt = attempt;
    const std::uint32_t before = ReadOnlineFrame(self);
    const bool manager_identity = ManagerIdentity(self);
    const bool stable_roster = manager_identity && ObserveStableRoster(self);
    const bool wrong_thread = WrongSimulationThread();
    const bool manager_changed = g_session_manager && self != g_session_manager;
    const bool frame_regressed = g_last_frame != UINT32_MAX && before != UINT32_MAX &&
                                 before < g_last_frame;
    if (!g_session_manager && manager_identity)
        g_session_manager = self;
    Evaluate({true, false, manager_identity, stable_roster, false, false, wrong_thread,
              manager_changed, frame_regressed, false, false});
    const bool result = g_frame(self);
    const std::uint32_t after = ReadOnlineFrame(self);
    g_last_frame = after;
    NativeEvent event = Event(NativeEventKind::Frame);
    event.attempt_id = attempt;
    event.online_before = before;
    event.online_after = after;
    event.original_calls = 1;
    event.result_value = result ? 1 : 0;
    event.flags = kEventResultBool |
                  (manager_identity ? kEventManagerIdentity : 0u) |
                  (stable_roster ? kEventStableRoster : 0u) |
                  (wrong_thread ? kEventWrongThread : 0u) |
                  (frame_regressed ? kEventFrameRegressed : 0u) |
                  (manager_changed ? kEventManagerChanged : 0u);
    Publish(event);
    g_current_attempt = 0;
    return result;
}

extern "C" std::uint32_t __fastcall DispatchHook(void* self, void*, std::uint32_t argument)
{
    HookScope scope;
    const std::uint32_t result = g_dispatch(self, argument);
    NativeEvent event = Event(NativeEventKind::Dispatch);
    event.attempt_id = g_current_attempt;
    event.original_calls = 1;
    event.result_value = static_cast<std::int32_t>(result);
    Publish(event);
    return result;
}

std::uint32_t __stdcall PostGameStartHook(std::uint32_t continued)
{
    HookScope scope;
    const auto lifecycle_generation = static_cast<std::uint64_t>(
        InterlockedIncrement64(&g_lifecycle_generation_counter));
    InterlockedExchange64(&g_current_lifecycle_generation,
                          static_cast<LONG64>(lifecycle_generation));
    const RunActiveSnapshot before = ReadRunActive();
    const bool wrong_thread = WrongSimulationThread();
    const std::uint32_t result = InvokeLifecycleOnce(g_post_game_start, continued);
    const RunActiveSnapshot after = ReadRunActive();
    const bool same_game = before.game && before.game == after.game;
    const std::uint32_t active_after = same_game ? after.value : kLifecycleUnavailable;

    NativeEvent event = Event(NativeEventKind::PostGameStart);
    event.attempt_id = lifecycle_generation;
    event.original_calls = 1;
    event.result_value = static_cast<std::int32_t>(continued);
    event.active_before = before.value;
    event.active_after = active_after;
    event.flags = (IsRunActive(before.value) ? kEventRunActiveBefore : 0u) |
                  (continued != 0 ? kEventContinued : 0u) |
                  (wrong_thread ? kEventWrongThread : 0u);
    Publish(event);
    return result;
}

std::uint32_t __stdcall RunTeardownHook(std::uint32_t argument)
{
    HookScope scope;
    const auto lifecycle_generation = static_cast<std::uint64_t>(
        InterlockedCompareExchange64(&g_current_lifecycle_generation, 0, 0));
    const RunActiveSnapshot before = ReadRunActive();
    const bool wrong_thread = WrongSimulationThread();
    const std::uint32_t result = InvokeLifecycleOnce(g_run_teardown, argument);
    const RunActiveSnapshot after = ReadRunActive();
    const bool same_game = before.game && before.game == after.game;
    const std::uint32_t active_after = same_game ? after.value : kLifecycleUnavailable;
    const bool lifecycle_transition =
        IsTeardownObservation(before.value, active_after, same_game);

    if (g_latch && g_latch->Token() && (lifecycle_transition || wrong_thread))
    {
        const SignalObservation signal{false, false, false, false, false, false,
                                       wrong_thread, false, false, false,
                                       lifecycle_transition};
        PublishTransition(g_latch->Observe(signal), ObservationFlags(signal));
    }

    NativeEvent event = Event(NativeEventKind::RunTeardown);
    event.attempt_id = lifecycle_generation;
    event.original_calls = 1;
    event.result_value = static_cast<std::int32_t>(argument);
    event.active_before = before.value;
    event.active_after = active_after;
    event.flags = (IsRunActive(before.value) ? kEventRunActiveBefore : 0u) |
                  (lifecycle_transition ? kEventRunActiveAfterZero : 0u) |
                  (wrong_thread ? kEventWrongThread : 0u);
    Publish(event);

    if (lifecycle_transition)
    {
        g_session_manager = nullptr;
        g_generation_manager = nullptr;
        g_missing_manager_callbacks = 0;
        InterlockedExchange64(&g_current_lifecycle_generation, 0);
    }
    return result;
}

void* __cdecl LuaNewstateHook(void* allocator, void* user_data)
{
    HookScope scope;
    const bool outermost = g_lua_create_depth++ == 0;
    void* state = g_lua_newstate(allocator, user_data);
    --g_lua_create_depth;
    if (outermost && state)
    {
        g_lua_root = state;
        Publish(Event(NativeEventKind::LuaCreated));
    }
    return state;
}

void* __cdecl LuaLNewstateHook()
{
    HookScope scope;
    const bool outermost = g_lua_create_depth++ == 0;
    void* state = g_lua_lnewstate();
    --g_lua_create_depth;
    if (outermost && state)
    {
        g_lua_root = state;
        Publish(Event(NativeEventKind::LuaCreated));
    }
    return state;
}

void __cdecl LuaCloseHook(void* state)
{
    HookScope scope;
    const bool root_match = state && state == g_lua_root;
    if (root_match && g_latch && g_latch->Token())
    {
        const SignalObservation signal{false, false, false, false, false, false, false,
                                       false, false, true, false};
        PublishTransition(g_latch->Observe(signal), ObservationFlags(signal));
    }
    g_lua_close(state);
    NativeEvent event = Event(NativeEventKind::LuaClose);
    event.original_calls = 1;
    event.flags = root_match ? kEventRootMatch : 0u;
    Publish(event);
    if (root_match)
    {
        g_lua_root = nullptr;
        g_session_manager = nullptr;
        g_generation_manager = nullptr;
    }
}

int __cdecl LuaPcallHook(void* state, int nargs, int nresults, int errfunc,
                         std::intptr_t context, void* continuation)
{
    void* return_address = _ReturnAddress();
    HookScope scope;
    const bool simulation = return_address == g_main_base + kLuaPcallReturnRva;
    if (simulation)
    {
        BeginLuaGeneration(state);
        const bool wrong_thread = WrongSimulationThread();
        void* manager = ReadManagerSlot();
        const bool manager_changed = g_latch && g_latch->Token() && g_generation_manager &&
                                     manager && manager != g_generation_manager;
        bool returned_to_menu = false;
        if (g_latch && g_latch->Token())
        {
            if (!manager)
                returned_to_menu = ++g_missing_manager_callbacks >= 2;
            else
                g_missing_manager_callbacks = 0;
        }
        Evaluate({true, true, false, false, false, false, wrong_thread, manager_changed,
                  false, false, returned_to_menu});
    }
    const int result = g_lua_pcall(state, nargs, nresults, errfunc, context, continuation);
    if (simulation)
    {
        NativeEvent event = Event(NativeEventKind::LuaSimulationPcall);
        event.original_calls = 1;
        event.result_value = result;
        event.flags = (state == g_lua_root ? kEventRootMatch : 0u);
        Publish(event);
    }
    return result;
}

void* __cdecl SteamContextHook(void* init_data)
{
    HookScope scope;
    void* context = g_steam_context(init_data);
    const bool descriptor = SteamDescriptorMatches(init_data);
    if (descriptor && InterlockedCompareExchange(&g_unloading, 0, 0) == 0)
        InstallSteamHooks(context);
    return context;
}

extern "C" bool __fastcall SteamSendHook(void* self, void*, std::uint64_t remote,
                                         const void* data, std::uint32_t size, int send_mode,
                                         int channel)
{
    HookScope scope;
    Sha256Digest before{};
    Sha256Digest after{};
    const bool hashed_before = SafeHash(data, size, before);
    if (channel == 0)
        InterlockedIncrement(&g_channel_zero_since_evaluate);
    else
        InterlockedIncrement(&g_nonzero_channel_since_evaluate);
    const bool result = g_steam_send(self, remote, data, size, send_mode, channel);
    const bool hashed_after = SafeHash(data, size, after);
    NativeEvent event = Event(NativeEventKind::SteamSend);
    event.channel = channel;
    event.send_mode = send_mode;
    event.payload_size = size;
    event.original_calls = 1;
    event.result_value = result ? 1 : 0;
    event.peer_token = PeerToken(remote);
    event.flags = kEventResultBool | (channel == 0 ? kEventChannelZero : 0u) |
                  (hashed_after ? kEventPayloadHashAfter : 0u);
    if (hashed_before)
        event.payload_hash = before;
    if (hashed_after)
        event.payload_hash_after = after;
    Publish(event);
    return result;
}

extern "C" bool __fastcall SteamAvailableHook(void* self, void*, std::uint32_t* size,
                                              int channel)
{
    HookScope scope;
    if (channel == 0)
        InterlockedIncrement(&g_channel_zero_since_evaluate);
    else
        InterlockedIncrement(&g_nonzero_channel_since_evaluate);
    const bool result = g_steam_available(self, size, channel);
    const std::uint32_t output = ReadOptionalSize(size);
    NativeEvent event = Event(NativeEventKind::SteamAvailable);
    event.channel = channel;
    event.output_size = output;
    event.original_calls = 1;
    event.result_value = result ? 1 : 0;
    event.flags = kEventResultBool | (channel == 0 ? kEventChannelZero : 0u);
    Publish(event);
    return result;
}

extern "C" bool __fastcall SteamReadHook(void* self, void*, void* data,
                                         std::uint32_t capacity, std::uint32_t* size,
                                         std::uint64_t* remote, int channel)
{
    HookScope scope;
    if (channel == 0)
        InterlockedIncrement(&g_channel_zero_since_evaluate);
    else
        InterlockedIncrement(&g_nonzero_channel_since_evaluate);
    const bool result = g_steam_read(self, data, capacity, size, remote, channel);
    std::uint32_t output = UINT32_MAX;
    std::uint64_t peer = 0;
    ReadPacketOutputs(size, remote, output, peer);
    Sha256Digest digest{};
    const bool hashed = result && output <= capacity && SafeHash(data, output, digest);
    NativeEvent event = Event(NativeEventKind::SteamRead);
    event.channel = channel;
    event.payload_size = capacity;
    event.output_size = output;
    event.original_calls = 1;
    event.result_value = result ? 1 : 0;
    event.peer_token = PeerToken(peer);
    event.flags = kEventResultBool | (channel == 0 ? kEventChannelZero : 0u);
    if (hashed)
        event.payload_hash = digest;
    Publish(event);
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

extern "C" DWORD WINAPI IsaacCsT0Initialize(const wchar_t* mapping_name)
{
    if (!mapping_name || !*mapping_name || g_shared)
        return static_cast<DWORD>(ErrorCode::MappingInvalid);
    g_mapping = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, mapping_name);
    if (!g_mapping)
        return static_cast<DWORD>(ErrorCode::MappingInvalid);
    g_shared = static_cast<SharedModeBlock*>(MapViewOfFile(
        g_mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedModeBlock)));
    if (!ValidateMapping())
    {
        SetError(ErrorCode::MappingInvalid, NativeState::Failed);
        return static_cast<DWORD>(ErrorCode::MappingInvalid);
    }
    InterlockedExchange(&g_shared->state, static_cast<LONG>(NativeState::Initializing));
    g_main_module = GetModuleHandleW(nullptr);
    g_main_base = reinterpret_cast<std::uint8_t*>(g_main_module);
    if (!IsTargetImage())
    {
        SetError(ErrorCode::TargetImageMismatch, NativeState::Failed);
        return static_cast<DWORD>(ErrorCode::TargetImageMismatch);
    }
    if (HasConflictingRuntime())
    {
        SetError(ErrorCode::ConflictingModuleOrDebugger, NativeState::Failed);
        return static_cast<DWORD>(ErrorCode::ConflictingModuleOrDebugger);
    }
    const auto requested = static_cast<RequestedMode>(g_shared->requested_mode);
    g_latch.emplace(requested);
    Publish(Event(NativeEventKind::NativeInitialized));
    if (!InstallFixedHooks())
    {
        const bool unsafe = AnyPatchInstalled();
        SetError(unsafe ? ErrorCode::UnsafeShutdown : ErrorCode::FixedHookTransaction,
                 unsafe ? NativeState::Unsafe : NativeState::Failed);
        NativeEvent event = Event(NativeEventKind::Failure);
        event.error_code = static_cast<std::uint32_t>(
            unsafe ? ErrorCode::UnsafeShutdown : ErrorCode::FixedHookTransaction);
        Publish(event);
        return static_cast<DWORD>(unsafe ? ErrorCode::UnsafeShutdown
                                         : ErrorCode::FixedHookTransaction);
    }
    Publish(Event(NativeEventKind::HooksInstalled));
    InterlockedExchange(&g_shared->state, static_cast<LONG>(NativeState::Ready));
    return 0;
}

extern "C" DWORD WINAPI IsaacCsT0Shutdown(void*)
{
    if (!g_shared || InterlockedExchange(&g_unloading, 1) != 0)
        return static_cast<DWORD>(ErrorCode::UnsafeShutdown);
    InterlockedExchange(&g_shared->state, static_cast<LONG>(NativeState::ShuttingDown));
    if (!RestoreAllHooks())
    {
        SetError(ErrorCode::UnsafeShutdown, NativeState::Unsafe);
        InterlockedExchange(&g_unloading, 0);
        return static_cast<DWORD>(ErrorCode::UnsafeShutdown);
    }
    const ULONGLONG deadline = GetTickCount64() + kHookDrainTimeoutMs;
    while (InterlockedCompareExchange(&g_active_hooks, 0, 0) != 0 &&
           GetTickCount64() < deadline)
        Sleep(1);
    if (InterlockedCompareExchange(&g_active_hooks, 0, 0) != 0)
    {
        SetError(ErrorCode::HookDrainTimeout, NativeState::Unsafe);
        return static_cast<DWORD>(ErrorCode::HookDrainTimeout);
    }
    Publish(Event(NativeEventKind::HooksRestored));
    InterlockedExchange(&g_shared->producer_closed, 1);
    InterlockedExchange(&g_shared->state, static_cast<LONG>(NativeState::Stopped));
    HMODULE module = g_module;
    FreeLibraryAndExitThread(module, 0);
}

} // namespace isaac_cs_t0::mode
