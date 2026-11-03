#pragma once
// tx-04-08
// world_record观察策略；tx-04-09为清单显式标记的shared_control入口增加独立输入资格通知。
// 下方原world-only说明继续适用于未标记入口；共享生命周期由值回调交给主线程桥，不写游戏对象。
//
// 迁移设计 4.1/4.2 与失败方式 F4 要求：新增的 world 观察入口不能套用
// `cs::CallScope` 的会话失败策略（非主线程 `game::ForeignCall()`、异常时
// `game::ContextFault()` 都会进入会话失败路径）。本层因此只做三件事：
//
//   1. 递增该 Hook 自己的调用/非主线程/正常返回计数（指针来自生成代码里
//      的 Hook 记录，本层不持有运行时类型）；
//   2. 记录 world 自有故障位 —— 非主线程进入、原生异常展开；
//   3. 保存最近一次观察的只读诊断快照（对象地址、实参 word、返回 word）。
//
// 它不读写身份表、不分配内存、不抛异常、不调用任何游戏/Lua/getter，也不
// 判断"原生 body 是否到达"（那属于主线程桥与清单声明）。快照用于状态诊断，
// 不是 lifecycle 事件记录：事件、call/parent 编号与 epoch 由主线程桥负责。
//
// 本头按项目约定只用 C++17 可表达的类型，因为生成的 SDK-facing wrapper
// （hooks_generated.cpp）以 C++17 编译。
#include <atomic>
#include <cstdint>
#include <exception>
#include <initializer_list>
#include <type_traits>

#include "world/observation.h"

namespace cs::world {

// 单次观察最多保存的实参 word 数。19 项观察入口的显式参数全部是 32 位
// 整数、bool 或指针，返回值同样是 32 位整数/bool/指针（见 Hook 清单的 ABI
// 登记），因此按 4 字节 word 保存即可，不做浮点解释。
inline constexpr std::uint32_t kObservationArgWords = 8;

// 计数来源：直接指向 Hook 记录中的原子量。允许为空，便于离线调用。
struct HookCounters final {
  std::atomic<std::uint64_t> *calls = nullptr;
  std::atomic<std::uint64_t> *foreign = nullptr;
  std::atomic<std::uint64_t> *completed = nullptr;
};

// world 自有故障位。它不是 C/S 会话故障，也不使 Host、B/K 或 Hook 安装失败；
// 由主线程桥在下一个收束点读取并收口（停用本次录制或记录明确 gap）。
enum class ObservationFault : std::uint32_t {
  None = 0,
  // 非主线程进入了 world 观察入口：只计数并置位，不触碰身份表。
  WrongThread = 1u << 0,
  // 原生异常展开经过 world 观察入口：只撤销本次观察资格，不伪造返回值。
  NativeException = 1u << 1
};
inline constexpr std::uint32_t kObservationFaultCount = 2;

// 永久的值通知接线。只在主线程调用 sink；非主线程只递增故障序号。
// 回调地址常驻 DLL，不携带可被后台释放的录制上下文。
struct ObservationSink final {
  std::uint32_t (*enter)(Observation, std::uint32_t, const std::uint32_t *,
                         std::uint32_t, bool) noexcept;
  void (*leave)(std::uint32_t, std::uint32_t, bool, bool) noexcept;
  // tx-04-09：共享生命周期的输入资格与world录制互相独立，回调仍只传值。
  std::uint32_t (*control_enter)(Observation, std::uint32_t,
                                 const std::uint32_t *,
                                 std::uint32_t) noexcept = nullptr;
  void (*control_leave)(std::uint32_t, bool) noexcept = nullptr;
  void (*control_foreign)() noexcept = nullptr;
  // Replace成功返回的值通知保留替换来源，不依赖原生随后清空的临时关系。
  void (*control_replace)(std::uint32_t, std::uint32_t) noexcept = nullptr;
};
void SetObservationSink(const ObservationSink *sink) noexcept;
std::uint64_t ObservationFailureSequence() noexcept;
// 分类别的累计次数用于相邻录制/帧之间取增量；历史故障位不污染新录制。
struct ObservationFailureCounts final {
  std::uint64_t wrong_thread = 0, native_exception = 0;
};
ObservationFailureCounts ObservationFailures() noexcept;
// 专属重配owner沿用world故障隔离；错误线程只更新原子状态，不调用Authority失败路径。
void ReportEntityReconfigurationFault(ObservationFault fault) noexcept;

const char *ObservationFaultName(std::uint32_t bit) noexcept;

// 单个观察语义的只读快照。地址与实参只用于诊断，不进入比较 key。
struct ObservationSnapshot final {
  std::uint64_t entered = 0, returned = 0, exceptional = 0, foreign = 0;
  std::uint64_t last_object = 0; // 仅诊断：最后一次主线程进入的对象地址
  std::uint64_t last_value = 0;  // 最近一次正常返回的 raw32
  std::uint32_t last_arg_count = 0;
  std::uint32_t last_args[kObservationArgWords]{};
  std::uint32_t faults = 0; // ObservationFault 位掩码
};

// 一个原生入口的进入/返回观察作用域。
//
// 由 gen_binding.py 生成成：
//   cs::world::ObservationScope observation(
//       cs::world::Observation::Factory, cs::IsMain(), this,
//       {&record.calls, &record.foreign, &record.completed},
//       {uint32_t(type), uint32_t(force)});
//   auto result = super(type, force);
//   observation.Returned(cs::world::PackWord(result));
//
// 构造与析构都不抛异常；原生异常沿原有栈展开，作用域只记录 exceptional。
class ObservationScope final {
public:
  ObservationScope(Observation observation, bool main_thread,
                   const void *object, HookCounters counters,
                   std::initializer_list<std::uint32_t> args,
                   bool native_witness = false,
                   bool shared_control = false) noexcept;
  ~ObservationScope() noexcept;
  ObservationScope(const ObservationScope &) = delete;
  ObservationScope &operator=(const ObservationScope &) = delete;

  bool Main() const noexcept { return main_; }
  // 正常返回：调用方在 super 之后立即调用；无返回值入口用无参重载。
  void Returned(std::uint32_t value) noexcept;
  void Returned() noexcept;

private:
  Observation observation_;
  HookCounters counters_{};
  std::uint32_t args_[kObservationArgWords]{};
  std::uint32_t arg_count_ = 0;
  const void *object_ = nullptr;
  bool main_ = false, returned_ = false;
  int exceptions_ = 0;
  const ObservationSink *sink_ = nullptr;
  std::uint32_t call_ = 0;
  bool native_ = false, shared_control_ = false;
  std::uint32_t control_mask_ = 0;
};

// 读取一个观察语义的累计快照；越界编号返回全零，不猜测最近值。
ObservationSnapshot ObservationStatus(Observation observation) noexcept;
// 该观察语义的故障位掩码（累计并集，只增）。
std::uint32_t ObservationFaults(Observation observation) noexcept;
// 全部 19 项观察的故障并集；主线程桥据此决定是否停用本次录制。
std::uint32_t ObservationFaultUnion() noexcept;
// 把观察枚举规范化为可用的索引；越界返回 kObservationCount。
std::uint32_t ObservationIndex(Observation observation) noexcept;

// 把 19 项入口的返回值打包成 raw32：指针取低 32 位，bool 归一化为 0/1，
// 其余整数直接截断。只用于诊断与统计，不解释语义。
template <class T> inline std::uint32_t PackWord(T value) noexcept {
  if constexpr (std::is_pointer<T>::value)
    return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(value));
  else if constexpr (std::is_same<T, bool>::value)
    return value ? 1u : 0u;
  else
    return static_cast<std::uint32_t>(value);
}

} // namespace cs::world
