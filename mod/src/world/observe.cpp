// tx-04-08 world_record 迁移：world 观察入口的计数与故障隔离。
//
// 本单元不读取游戏内存、不调用 SDK、不打开文件、不创建线程，也不访问
// 身份表。它只维护 20 项观察语义的原子计数、故障位与最近一次诊断快照，
// 供主线程桥与状态命令读取。

#include "world/observe.h"

#include <array>
#include <atomic>

namespace cs::world {
namespace {

// 每个观察语义一份诊断状态。entered/returned/... 由作用域原子更新，
// last_* 只在主线程观察路径写入；离线调用（非主线程）不写快照。
struct ObservationCell final {
  std::atomic<std::uint64_t> entered{0}, returned{0}, exceptional{0},
      foreign{0};
  std::atomic<std::uint64_t> last_object{0}, last_value{0};
  std::atomic<std::uint32_t> last_arg_count{0};
  std::atomic<std::uint32_t> last_args[kObservationArgWords]{};
  std::atomic<std::uint32_t> faults{0};
};

// 19 项固定容量；不做动态分配，索引越界一律忽略。
std::array<ObservationCell, kObservationCount> cells{};
std::atomic<const ObservationSink *> sink{nullptr};
std::atomic<std::uint64_t> failure_sequence{0};
std::atomic<std::uint64_t> wrong_thread_failures{0},
    native_exception_failures{0};

void RaiseFault(std::uint32_t index, ObservationFault fault) noexcept {
  if (index >= kObservationCount)
    return;
  cells[index].faults.fetch_or(static_cast<std::uint32_t>(fault),
                               std::memory_order_relaxed);
  if (fault == ObservationFault::WrongThread)
    wrong_thread_failures.fetch_add(1, std::memory_order_release);
  else if (fault == ObservationFault::NativeException)
    native_exception_failures.fetch_add(1, std::memory_order_release);
  failure_sequence.fetch_add(1, std::memory_order_release);
}

} // namespace

void SetObservationSink(const ObservationSink *value) noexcept {
  sink.store(value, std::memory_order_release);
}
void ReportEntityReconfigurationFault(ObservationFault fault) noexcept {
  RaiseFault(ObservationIndex(Observation::NpcMorph),fault);
}
std::uint64_t ObservationFailureSequence() noexcept {
  return failure_sequence.load(std::memory_order_acquire);
}
ObservationFailureCounts ObservationFailures() noexcept {
  return {wrong_thread_failures.load(std::memory_order_acquire),
          native_exception_failures.load(std::memory_order_acquire)};
}

const char *ObservationFaultName(std::uint32_t bit) noexcept {
  switch (bit) {
  case static_cast<std::uint32_t>(ObservationFault::None):
    return "NONE";
  case static_cast<std::uint32_t>(ObservationFault::WrongThread):
    return "WRONG_THREAD";
  case static_cast<std::uint32_t>(ObservationFault::NativeException):
    return "NATIVE_EXCEPTION";
  default:
    return "UNKNOWN_FAULT";
  }
}

std::uint32_t ObservationIndex(Observation observation) noexcept {
  const auto index = static_cast<std::uint32_t>(observation);
  return index < kObservationCount ? index : kObservationCount;
}

ObservationScope::ObservationScope(Observation observation, bool main_thread,
                                   const void *object, HookCounters counters,
                                   std::initializer_list<std::uint32_t> args,
                                   bool native_witness,
                                   bool shared_control) noexcept
    : observation_(observation), counters_(counters), object_(object),
      main_(main_thread), exceptions_(std::uncaught_exceptions()) {
  native_ = native_witness;
  shared_control_ = shared_control;
  sink_ = sink.load(std::memory_order_acquire);
  const auto index = ObservationIndex(observation);
  if (index >= kObservationCount)
    return;
  auto &cell = cells[index];
  if (counters_.calls)
    counters_.calls->fetch_add(1, std::memory_order_relaxed);
  cell.entered.fetch_add(1, std::memory_order_relaxed);
  // 实参只在进入时按调用方给出的顺序复制；超出上限的部分忽略而不是截断语义。
  std::uint32_t used = 0;
  for (const auto value : args) {
    if (used >= kObservationArgWords)
      break;
    args_[used++] = value;
  }
  arg_count_ = used;
  if (!main_) {
    // 非主线程：只计数并置位。绝不读写身份表、绝不调用会话失败路径。
    if (counters_.foreign)
      counters_.foreign->fetch_add(1, std::memory_order_relaxed);
    cell.foreign.fetch_add(1, std::memory_order_relaxed);
    RaiseFault(index, ObservationFault::WrongThread);
    if (shared_control_ && sink_ && sink_->control_foreign)
      sink_->control_foreign();
    return;
  }
  // 主线程路径只写诊断快照；这些值不是身份，也不是比较 key。
  cell.last_object.store(
      static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(object_)),
      std::memory_order_relaxed);
  cell.last_arg_count.store(arg_count_, std::memory_order_relaxed);
  for (std::uint32_t i = 0; i < arg_count_; ++i)
    cell.last_args[i].store(args_[i], std::memory_order_relaxed);
  if (shared_control_ && !native_ && sink_ && sink_->control_enter)
    control_mask_ = sink_->control_enter(observation_, PackWord(object_), args_,
                                         arg_count_);
  if (sink_ && sink_->enter)
    call_ = sink_->enter(observation_, PackWord(object_), args_, arg_count_,
                         native_);
}

ObservationScope::~ObservationScope() noexcept {
  const auto index = ObservationIndex(observation_);
  if (index >= kObservationCount)
    return;
  if (exceptions_ != std::uncaught_exceptions()) {
    // 原生异常展开：只撤销本次观察资格并保留失败事实，不伪造正常返回。
    cells[index].exceptional.fetch_add(1, std::memory_order_relaxed);
    RaiseFault(index, ObservationFault::NativeException);
    if (call_ && sink_ && sink_->leave)
      sink_->leave(call_, 0, false, native_);
    if (control_mask_ && sink_ && sink_->control_leave)
      sink_->control_leave(control_mask_, false);
    return;
  }
  // 正常返回却没有显式调用 Returned 时按"无值正常返回"收口，避免漏计；
  // 生成的 wrapper 一定会调用 Returned，这里只作兜底。
  if (!returned_ && main_)
    Returned();
}

void ObservationScope::Returned(std::uint32_t value) noexcept {
  if (returned_)
    return;
  returned_ = true;
  const auto index = ObservationIndex(observation_);
  if (index >= kObservationCount)
    return;
  auto &cell = cells[index];
  cell.returned.fetch_add(1, std::memory_order_relaxed);
  if (main_)
    cell.last_value.store(value, std::memory_order_relaxed);
  if (counters_.completed)
    counters_.completed->fetch_add(1, std::memory_order_relaxed);
  if (main_ && control_mask_ && observation_ == Observation::Replace && value &&
      arg_count_ >= 2 && sink_ && sink_->control_replace)
    sink_->control_replace(args_[0], args_[1]);
  if (control_mask_ && sink_ && sink_->control_leave) {
    sink_->control_leave(control_mask_, true);
    control_mask_ = 0;
  }
  if (call_ && sink_ && sink_->leave) {
    sink_->leave(call_, value, true, native_);
    call_ = 0;
  }
}

void ObservationScope::Returned() noexcept { Returned(0); }

ObservationSnapshot ObservationStatus(Observation observation) noexcept {
  ObservationSnapshot out{};
  const auto index = ObservationIndex(observation);
  if (index >= kObservationCount)
    return out;
  const auto &cell = cells[index];
  out.entered = cell.entered.load(std::memory_order_relaxed);
  out.returned = cell.returned.load(std::memory_order_relaxed);
  out.exceptional = cell.exceptional.load(std::memory_order_relaxed);
  out.foreign = cell.foreign.load(std::memory_order_relaxed);
  out.last_object = cell.last_object.load(std::memory_order_relaxed);
  out.last_value = cell.last_value.load(std::memory_order_relaxed);
  out.last_arg_count = cell.last_arg_count.load(std::memory_order_relaxed);
  if (out.last_arg_count > kObservationArgWords)
    out.last_arg_count = kObservationArgWords;
  for (std::uint32_t i = 0; i < out.last_arg_count; ++i)
    out.last_args[i] = cell.last_args[i].load(std::memory_order_relaxed);
  out.faults = cell.faults.load(std::memory_order_relaxed);
  return out;
}

std::uint32_t ObservationFaults(Observation observation) noexcept {
  const auto index = ObservationIndex(observation);
  return index < kObservationCount
             ? cells[index].faults.load(std::memory_order_relaxed)
             : 0;
}

std::uint32_t ObservationFaultUnion() noexcept {
  std::uint32_t out = 0;
  for (auto &cell : cells)
    out |= cell.faults.load(std::memory_order_relaxed);
  return out;
}

} // namespace cs::world
