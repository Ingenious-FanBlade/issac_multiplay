#pragma once
#include <winsock2.h>
#include <windows.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <string>
#include <string_view>
#include <typeinfo>
#include <stdexcept>
#include <cstring>

namespace cs {
inline constexpr char Version[] = "0.1.0-tx-04-04";
enum class Group { Loop, Input, Player, Join, Control, Device, Count };
inline constexpr const char* GroupNames[] = {"loop", "input", "player", "join", "control", "device"};
enum class Owner { None, Manager, Game, Render, Start, Restore, Lifecycle, Door, NestedDoor, Odd, Join, Console };
struct Config {
    bool enabled = false, control = false;
    unsigned control_port = 27091, host_port = 27090;
    std::string trace_dir = "isaac-cs/traces", log_level = "info", reason;
};
extern DWORD MainThread;
extern uintptr_t Base;
extern std::array<bool, size_t(Group::Count)> Capabilities;
extern std::array<std::string, size_t(Group::Count)> CapabilityReasons;
extern std::string RepentogonVersion;
extern Config Settings;
bool IsMain() noexcept;
void RefreshVersion() noexcept;
void* GamePointer() noexcept;
Config ReadConfig();
void StartTrace(const std::string& directory);
// Formatting uses a fixed stack buffer. Producers never allocate or wait for disk.
void Trace(const char* ev, const char* format = "", ...) noexcept;
uint64_t TraceDropped() noexcept;
const std::string& TracePath() noexcept;
void FinishTrace() noexcept;

// Bounded MPMC ring: per-cell sequence numbers publish fully copied values.
template<class T, size_t N> class Ring {
    struct Cell { std::atomic<size_t> seq{}; T value{}; };
    std::array<Cell, N> cells{};
    std::atomic<size_t> put{}, take{};
public:
    Ring() { for (size_t i=0; i<N; ++i) cells[i].seq.store(i); }
    bool Push(const T& v) noexcept {
        size_t p=put.load(std::memory_order_relaxed);
        for (;;) {
            auto& c=cells[p%N]; auto s=c.seq.load(std::memory_order_acquire);
            auto d=static_cast<intptr_t>(s-p);
            if (d<0) return false;
            if (d==0 && put.compare_exchange_weak(p,p+1,std::memory_order_relaxed)) {
                c.value=v; c.seq.store(p+1,std::memory_order_release); return true;
            }
            p=put.load(std::memory_order_relaxed);
        }
    }
    bool Pop(T& v) noexcept {
        size_t p=take.load(std::memory_order_relaxed);
        for (;;) {
            auto& c=cells[p%N]; auto s=c.seq.load(std::memory_order_acquire);
            auto d=static_cast<intptr_t>(s-(p+1));
            if (d<0) return false;
            if (d==0 && take.compare_exchange_weak(p,p+1,std::memory_order_relaxed)) {
                v=c.value; c.seq.store(p+N,std::memory_order_release); return true;
            }
            p=take.load(std::memory_order_relaxed);
        }
    }
};

struct Hook {
    const char* id; Group group; const char* name; const std::type_info* type;
    void* entry; void** super; uintptr_t rva; const char* pattern; bool own;
    int args, flags, priority; Owner owner;
    std::atomic<uint64_t> calls{}, foreign{}, completed{};
    bool registered = false;
};
void AddPending(Hook*) noexcept;
Hook* const* Hooks(size_t& count) noexcept;
bool PrepareBindings();
std::string MenuStatus();
void RegisterHooks();
void InstallPointers() noexcept;
unsigned PointerCount() noexcept;
std::string PointerCounters();
bool PatternMatches(uintptr_t rva, const char* pattern) noexcept;
void PublishStatus() noexcept;
void FrameBegin() noexcept;
void FrameEnd() noexcept;
void RunExit() noexcept;
void StartControlPort();
void StopControlPort() noexcept;
bool OwnCommand(std::string_view text) noexcept;
std::string Command(std::string_view text, const char* source);
struct PortRequest { uint64_t id; char text[513]; };
struct PortReply { uint64_t id; char text[12000]; };
bool QueuePort(const PortRequest&) noexcept;
bool TakeReply(PortReply&) noexcept;
struct CallScope {
    Hook& hook; bool main, pushed=false; int exceptions;
    explicit CallScope(Hook&) noexcept;
    ~CallScope() noexcept;
};
Owner CurrentOwner() noexcept;
uint64_t ContextFaults() noexcept;
}
