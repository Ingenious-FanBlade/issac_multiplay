#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeinfo>
#include <windows.h>
#include <winsock2.h>

namespace cs {
inline constexpr char Version[] = "0.1.0-tx-04-04";
enum class Group { Loop, Input, Player, Join, Control, Device, Count };
inline constexpr const char *GroupNames[] = {"loop", "input",   "player",
                                             "join", "control", "device"};
enum class Owner {
  None,
  Manager,
  Game,
  Render,
  Start,
  Restore,
  Lifecycle,
  Door,
  NestedDoor,
  Odd,
  Join,
  Console
};
// schema=1 配置；log_level 在 tx-04-04 中仅记录，不过滤 trace。
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
void *GamePointer() noexcept;
Config ReadConfig();
void StartTrace(const std::string &directory);
// 生产者只格式化到固定栈缓冲，不分配内存、不等待磁盘。
void Trace(const char *ev, const char *format = "", ...) noexcept;
uint64_t TraceDropped() noexcept;
const std::string &TracePath() noexcept;
void FinishTrace() noexcept;
void FinishTraceOnProcessExit() noexcept;

// 有界 MPMC 环形队列：每槽 sequence 在完整复制后发布内容。
template <class T, size_t N> class Ring {
  struct Cell {
    std::atomic<size_t> seq{};
    T value{};
  };
  std::array<Cell, N> cells{};
  std::atomic<size_t> put{}, take{};

public:
  Ring() {
    for (size_t i = 0; i < N; ++i)
      cells[i].seq.store(i);
  }
  bool Push(const T &v) noexcept {
    size_t p = put.load(std::memory_order_relaxed);
    for (;;) {
      auto &c = cells[p % N];
      auto s = c.seq.load(std::memory_order_acquire);
      auto d = static_cast<intptr_t>(s - p);
      if (d < 0)
        return false;
      if (d == 0 &&
          put.compare_exchange_weak(p, p + 1, std::memory_order_relaxed)) {
        c.value = v;
        c.seq.store(p + 1, std::memory_order_release);
        return true;
      }
      p = put.load(std::memory_order_relaxed);
    }
  }
  bool Pop(T &v) noexcept {
    size_t p = take.load(std::memory_order_relaxed);
    for (;;) {
      auto &c = cells[p % N];
      auto s = c.seq.load(std::memory_order_acquire);
      auto d = static_cast<intptr_t>(s - (p + 1));
      if (d < 0)
        return false;
      if (d == 0 &&
          take.compare_exchange_weak(p, p + 1, std::memory_order_relaxed)) {
        v = c.value;
        c.seq.store(p + N, std::memory_order_release);
        return true;
      }
      p = take.load(std::memory_order_relaxed);
    }
  }
};

// 单个常驻 Hook 的 ABI、注册状态与只增计数。
struct Hook {
  const char *id;
  Group group;
  const char *name;
  const std::type_info *type;
  void *entry;
  void **super;
  uintptr_t rva;
  const char *pattern;
  bool own;
  int args, flags, priority;
  Owner owner;
  std::atomic<uint64_t> calls{}, foreign{}, completed{};
  bool registered = false;
};
void AddPending(Hook *) noexcept;
Hook *const *Hooks(size_t &count) noexcept;
bool PrepareBindings();
std::string MenuStatus(bool run_active);
void RegisterHooks();
void InstallPointers() noexcept;
unsigned PointerCount() noexcept;
std::string PointerCounters();
bool PatternMatches(uintptr_t rva, const char *pattern) noexcept;
void PublishStatus() noexcept;
void FrameBegin() noexcept;
void FrameEnd(bool completed = true) noexcept;
// Manager 的原函数正常返回或异常展开时都要归还嵌套深度。
struct FrameScope {
  bool main;
  int exceptions;
  explicit FrameScope(bool) noexcept;
  ~FrameScope() noexcept;
};
void RunExit() noexcept;
void StartControlPort();
void StopControlPort() noexcept;
bool OwnCommand(std::string_view text) noexcept;
std::string Command(std::string_view text, const char *source);
// 网络线程只交换固定大小消息，不访问游戏对象。
struct PortRequest {
  uint64_t id;
  uint64_t enqueued_ms;
  uint64_t deadline_ms;
  char text[513];
};
struct PortReply {
  uint64_t id;
  char text[12000];
};
bool QueuePort(const PortRequest &) noexcept;
bool TakeReply(PortReply &) noexcept;
// 透传调用的计数与上下文由 RAII 维护，原生异常经过时也会弹栈。
struct CallScope {
  Hook &hook;
  bool main, pushed = false;
  int exceptions;
  explicit CallScope(Hook &) noexcept;
  ~CallScope() noexcept;
};
Owner CurrentOwner() noexcept;
uint64_t ContextFaults() noexcept;
} // namespace cs
