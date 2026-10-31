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
inline constexpr char Version[] = "0.4.0-tx-04-17";
// Group 决定能力组：某组 binding 不符时只关闭该组。
// Observe 是 tx-04-07 新增的只读观察组；World 是 tx-04-08 的 world_record
// 观察组（19 项观察语义），它有独立的 world 故障策略，不进入会话失败路径。
enum class Group {
  Loop,
  Input,
  Player,
  Join,
  Control,
  Device,
  Observe,
  World,
  Count
};
inline constexpr const char *GroupNames[] = {"loop",    "input", "player",
                                             "join",    "control", "device",
                                             "observe", "world"};
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
  Console,
  Input,
  Device,
  OddNormal
};
// schema=1配置；高频测试诊断须显式启用，生产默认只保留低频状态/错误日志。
struct Config {
  bool enabled = false, control = false;
  bool diagnostics = false, repeat_topology_reads = false;
  unsigned control_port = 27091, host_port = 27090;
  // Host 网络参数只在会话创建前复制，不提供运行中热修改。
  std::string host_bind_address = "0.0.0.0";
  unsigned peer_timeout_ms = 3000;
  std::string trace_dir = "isaac-cs/traces", log_level = "info", reason;
  // 允许注册独立 world 能力；不自动开始录制，目录和期限供命令层准备上下文。
  bool world_record_enabled = false;
  std::string world_output_dir = "isaac-cs/world";
  unsigned world_close_timeout_ms = 10000;
  std::string world_e2e_token; // 专用本地 E2E 配置；空串关闭所有测试入口。
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
// 格式化超长单列计数，同时计入 dropped；不把截断文本当作完整证据。
uint64_t TraceTruncated() noexcept;
// 区分队列、写入和唤醒错误，供状态与唯一 E2E 的失败归因。
std::string TraceFields();
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

// 单个原生参数的 ABI 描述（tx-04-08）。
//
// ZHL 的 ArgData 用 `_size == 0` 表示"值在寄存器里"，非零表示占用的栈
// word 数。旧生成器按逗号数推断参数个数并一律给 `{NoRegister, 1}`，无法
// 表达 GridFactory 的 ECX/EDX。本结构把两种形态分开表达，寄存器编号与
// HookSystem::GPRegisters / XMMRegisters 的枚举值一致。
struct HookArg {
  enum class Kind : std::uint8_t { Stack = 0, GpRegister = 1, XmmRegister = 2 };
  Kind kind = Kind::Stack;
  std::uint8_t reg = 0;    // 寄存器编号；Stack 时不使用
  std::uint16_t size = 1;  // Stack 时的栈 word 数；寄存器参数为 0
};

// 非 world 观察入口的 sentinel。
inline constexpr std::uint32_t kHookObservationNone = 0xFFFFFFFFu;

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
  // tx-04-08：world 观察入口的显式 ABI 表与语义编号；其他记录保持默认。
  // 自有定义按 arg_spec 构造真实 ArgData；官方定义仍复用 REPENTOGON 的
  // ArgData，这里的 arg_spec 只作清单/校验用途。
  const HookArg *arg_spec = nullptr;
  std::uint32_t observation = kHookObservationNone;
  bool world = false;
  bool shared_control = false; // 关闭world录制时仍常驻的C/S生命周期观察。
  std::atomic<uint64_t> calls{}, foreign{}, completed{};
  bool registered = false;
};

// 待注册表的容量报告。容量与行数来自生成清单（binding/hook_capacity.h），
// 注册阶段必须显式报告，不能像旧实现那样静默丢弃超限记录。
struct HookCapacity final {
  std::size_t capacity = 0, used = 0, rejected = 0;
};
HookCapacity PendingHookCapacity() noexcept;
// 被容量拒绝的记录（最多保留 8 项用于停用对应能力组）。
Hook *const *RejectedHooks(std::size_t &count) noexcept;
// tx-04-08：由独立 world_record.enabled 决定是否登记 world 常驻 Hook。
bool WorldObservationArmed() noexcept;
void AddPending(Hook *) noexcept;
Hook *const *Hooks(size_t &count) noexcept;
bool PrepareBindings();
std::string MenuStatus(bool run_active);
void RegisterHooks();
void InstallPointers() noexcept;
void InstallRuntimePatches() noexcept;
void InstallClientPatches() noexcept;
bool ClientPatchesInstalled() noexcept;
bool OddPatchInstalled() noexcept;
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
// 只清掉旧 run 的排队变更与 pending 标记，不改变已经生效的会话。
// 供 Restore 的保留分支与真实退出共用（tx-04-07）。
void ClearPendingRunMutations() noexcept;
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
  char text[65536]; // 四参与者及逐身体状态仍为有界主线程值快照。
};
bool QueuePort(const PortRequest &) noexcept;
bool TakeReply(PortReply &) noexcept;
// 透传调用的计数与上下文由 RAII 维护，原生异常经过时也会弹栈。
struct CallScope {
  Hook &hook;
  bool main, pushed = false;
  int exceptions;
  uint64_t token = 0;
  explicit CallScope(Hook &) noexcept;
  ~CallScope() noexcept;
};
Owner CurrentOwner() noexcept;
uint64_t ContextFaults() noexcept;
} // namespace cs
