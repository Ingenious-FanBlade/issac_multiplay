// tx-04-08 world 存储层实现：三槽所有权、通知、有界值队列、gap 控制记录
// 与 DLL 自有 raw reader。
//
// 本单元是 win32 相关的，但只做线程、事件与内存所有权，不读取游戏对象、
// 不调用原生函数、不写文件（文件在 writer.cpp）。

// clang-format off
#include <windows.h>
// clang-format on

#include "world/storage.h"

#include <algorithm>
#include <chrono>
#include <new>
#include <utility>

namespace cs::world {

const char *StorageStateName(StorageState value) noexcept {
  switch (value) {
  case StorageState::Idle:
    return "Idle";
  case StorageState::Preparing:
    return "Preparing";
  case StorageState::Ready:
    return "Ready";
  case StorageState::Faulted:
    return "Faulted";
  case StorageState::Closing:
    return "Closing";
  case StorageState::Closed:
    return "Closed";
  }
  return "UNKNOWN";
}
const char *ThreadStateName(ThreadState value) noexcept {
  switch (value) {
  case ThreadState::NotStarted:
    return "NotStarted";
  case ThreadState::Running:
    return "Running";
  case ThreadState::Draining:
    return "Draining";
  case ThreadState::Done:
    return "Done";
  case ThreadState::Failed:
    return "Failed";
  }
  return "UNKNOWN";
}
const char *FailSourceName(FailSource value) noexcept {
  switch (value) {
  case FailSource::None:
    return "NONE";
  case FailSource::Producer:
    return "PRODUCER";
  case FailSource::Reader:
    return "READER";
  case FailSource::Writer:
    return "WRITER";
  case FailSource::Queue:
    return "QUEUE";
  case FailSource::Prepare:
    return "PREPARE";
  }
  return "UNKNOWN";
}

// ===========================================================================
// Diagnostics
// ===========================================================================

bool Diagnostics::Fail(Reason reason, FailSource source, std::uint32_t code,
                       const char *stage) noexcept {
  if (reason == Reason::None)
    return false;
  auto expected = static_cast<std::uint32_t>(Reason::None);
  if (!fault_reason_.compare_exchange_strong(expected,
                                             static_cast<std::uint32_t>(reason),
                                             std::memory_order_acq_rel,
                                             std::memory_order_relaxed))
    return false;
  // 首因已确定，其余字段只是描述；读取方以 reason 为准。
  fault_source_.store(static_cast<std::uint32_t>(source),
                      std::memory_order_relaxed);
  fault_code_.store(code, std::memory_order_relaxed);
  fault_stage_.store(stage ? stage : "", std::memory_order_relaxed);
  fault_at_ms_.store(GetTickCount64(), std::memory_order_relaxed);
  return true;
}
FaultInfo Diagnostics::Fault() const noexcept {
  FaultInfo info{};
  info.reason = static_cast<Reason>(fault_reason_.load(std::memory_order_acquire));
  info.source = static_cast<FailSource>(
      fault_source_.load(std::memory_order_relaxed));
  info.code = fault_code_.load(std::memory_order_relaxed);
  info.stage = fault_stage_.load(std::memory_order_relaxed);
  info.at_ms = fault_at_ms_.load(std::memory_order_relaxed);
  return info;
}

// ===========================================================================
// ValueQueue
// ===========================================================================

ValueQueue::ValueQueue(std::uint64_t bytes, std::uint32_t items) noexcept
    : byte_limit_(bytes), item_limit_(items) {}
ValueQueue::~ValueQueue() = default;

bool ValueQueue::Reserve(std::uint64_t bytes,
                         std::uint32_t wait_ms) noexcept {
  std::unique_lock<std::mutex> lock(mutex_);
  // 两个上限同时成立：字节与项数任一不足都不能预占。
  const auto available = [&]() noexcept {
    return held_items_.load(std::memory_order_relaxed) < item_limit_ &&
           bytes <= byte_limit_ - held_bytes_.load(std::memory_order_relaxed);
  };
  if (!available()) {
    budget_waits_.fetch_add(1, std::memory_order_relaxed);
    if (!space_.wait_for(lock, std::chrono::milliseconds(wait_ms), available))
      return false;
  }
  const auto items = held_items_.fetch_add(1, std::memory_order_relaxed) + 1;
  const auto held = held_bytes_.fetch_add(bytes, std::memory_order_relaxed) + bytes;
  auto peak_items = peak_items_.load(std::memory_order_relaxed);
  while (peak_items < items &&
         !peak_items_.compare_exchange_weak(peak_items, items,
                                            std::memory_order_relaxed,
                                            std::memory_order_relaxed)) {
  }
  auto peak_bytes = peak_bytes_.load(std::memory_order_relaxed);
  while (peak_bytes < held &&
         !peak_bytes_.compare_exchange_weak(peak_bytes, held,
                                            std::memory_order_relaxed,
                                            std::memory_order_relaxed)) {
  }
  return true;
}

bool ValueQueue::Push(QueueItem &&item) noexcept {
  bool pushed = false;
  {
    // deque 节点分配失败是可控的预算/内存错误：返回 false，由调用方归还
    // 预留并把 world 置为故障，不能在这里终止线程。
    std::lock_guard<std::mutex> lock(mutex_);
    try {
      items_.push_back(std::move(item));
      pushed = true;
    } catch (...) {
      pushed = false;
    }
  }
  if (pushed) {
    pushes_.fetch_add(1, std::memory_order_relaxed);
    ready_.notify_one();
  }
  return pushed;
}

void ValueQueue::Release(std::uint64_t bytes) noexcept {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto items = held_items_.load(std::memory_order_relaxed);
    while (items &&
           !held_items_.compare_exchange_weak(items, items - 1,
                                              std::memory_order_relaxed,
                                              std::memory_order_relaxed)) {
    }
    auto held = held_bytes_.load(std::memory_order_relaxed);
    while (held) {
      const auto next = held >= bytes ? held - bytes : 0;
      if (held_bytes_.compare_exchange_weak(held, next,
                                            std::memory_order_relaxed,
                                            std::memory_order_relaxed))
        break;
    }
  }
  // 归还的预算唤醒等待中的 reader，即使期间没有新的 native 发布。
  space_.notify_one();
}

bool ValueQueue::Pop(QueueItem &item, std::uint32_t wait_ms) noexcept {
  std::unique_lock<std::mutex> lock(mutex_);
  if (items_.empty() && !producer_done_.load(std::memory_order_relaxed))
    ready_.wait_for(lock, std::chrono::milliseconds(wait_ms), [&]() noexcept {
      return !items_.empty() || producer_done_.load(std::memory_order_relaxed);
    });
  if (items_.empty())
    return false;
  item = std::move(items_.front());
  items_.pop_front();
  pops_.fetch_add(1, std::memory_order_relaxed);
  return true;
}

void ValueQueue::CloseProducer() noexcept {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    producer_done_.store(true, std::memory_order_release);
  }
  // 唤醒可能正等预算的 reader 与等数据的 writer。
  space_.notify_all();
  ready_.notify_all();
}

bool ValueQueue::ProducerDone() const noexcept {
  return producer_done_.load(std::memory_order_acquire);
}
bool ValueQueue::Empty() const noexcept {
  return held_items_.load(std::memory_order_acquire) == 0;
}
QueueStats ValueQueue::Read() const noexcept {
  QueueStats stats{};
  stats.held_items = held_items_.load(std::memory_order_acquire);
  stats.held_bytes = held_bytes_.load(std::memory_order_acquire);
  stats.peak_items = peak_items_.load(std::memory_order_relaxed);
  stats.peak_bytes = peak_bytes_.load(std::memory_order_relaxed);
  stats.pushes = pushes_.load(std::memory_order_relaxed);
  stats.pops = pops_.load(std::memory_order_relaxed);
  stats.budget_waits = budget_waits_.load(std::memory_order_relaxed);
  stats.discarded_items = discarded_.load(std::memory_order_relaxed);
  stats.producer_done = producer_done_.load(std::memory_order_acquire);
  stats.empty = stats.held_items == 0;
  return stats;
}
void ValueQueue::NoteDiscarded(std::uint64_t items) noexcept {
  discarded_.fetch_add(items, std::memory_order_relaxed);
}

// ===========================================================================
// 存储内部
// ===========================================================================

namespace {

// 槽状态机。主线程只写 WRITING→PREPARED；reader 只写 READY→READING→FREE；
// 撤销由主线程写 PREPARED→FREE。
enum class SlotState : std::uint32_t {
  Free = 0,
  Writing = 1,
  Prepared = 2,
  Ready = 3,
  Reading = 4
};

// reader 单次有界预算等待；主线程从不进入这里。
constexpr std::uint32_t kReserveWaitMs = 50;
// 空闲轮询上限：主线程发布/停止时会 SetEvent，超时只是兜底。
constexpr std::uint32_t kIdleWaitMs = 25;
// 停止后允许的排空期限；超时保留 incomplete，不无限等待 writer。
constexpr std::uint32_t kDrainMs = 2000;
// 每轮最多处理的批次/事件块/控制记录块，避免生命周期事件被新批次饿死。
constexpr std::uint32_t kBatchPass = 2;
constexpr std::uint32_t kEventPass = 4;
constexpr std::uint32_t kGapPass = 2;
constexpr std::uint32_t kEventChunk = 1024;
constexpr std::uint32_t kGapChunk = 256;

std::uint64_t NowNanoseconds() noexcept {
  static const std::uint64_t frequency = []() noexcept {
    LARGE_INTEGER value{};
    QueryPerformanceFrequency(&value);
    return value.QuadPart > 0 ? static_cast<std::uint64_t>(value.QuadPart) : 1;
  }();
  LARGE_INTEGER counter{};
  QueryPerformanceCounter(&counter);
  return static_cast<std::uint64_t>(counter.QuadPart) * 1000000000ull /
         frequency;
}

// 固定容量、单生产者/单消费者环。生产者是主线程，消费者是 reader。
// 满时返回 false：事件/控制记录丢失必须让录制失效，不能静默覆盖。
template <class T> class SpscRing final {
public:
  SpscRing(std::unique_ptr<T[]> values, std::uint32_t capacity) noexcept
      : values_(std::move(values)), capacity_(capacity) {}
  bool TryPush(const T &value) noexcept {
    const auto write = write_.load(std::memory_order_relaxed);
    const auto read = read_.load(std::memory_order_acquire);
    if (write - read >= capacity_)
      return false;
    values_[static_cast<std::size_t>(write % capacity_)] = value;
    write_.store(write + 1, std::memory_order_release);
    return true;
  }
  bool TryPop(T &value) noexcept {
    const auto read = read_.load(std::memory_order_relaxed);
    if (read == write_.load(std::memory_order_acquire))
      return false;
    value = values_[static_cast<std::size_t>(read % capacity_)];
    read_.store(read + 1, std::memory_order_release);
    return true;
  }
  std::uint64_t Distance() const noexcept {
    return write_.load(std::memory_order_acquire) -
           read_.load(std::memory_order_acquire);
  }
  std::uint32_t Capacity() const noexcept { return capacity_; }

private:
  std::unique_ptr<T[]> values_;
  std::uint32_t capacity_;
  std::atomic<std::uint64_t> write_{0}, read_{0};
};

struct Slot final {
  std::atomic<std::uint32_t> state{static_cast<std::uint32_t>(SlotState::Free)};
  std::atomic<std::uint64_t> attempt{0};
  // 该槽本次尝试的候选值：撤销/悬挂时据此生成 gap。
  BatchBegin begin{};
  // 43.5 MiB 一次分配；默认初始化不触碰对象/成员数组。
  std::unique_ptr<RawBatch> batch;
};

} // namespace

// 存储核心。由 WorldStorage 与各线程以 shared_ptr 共同持有：线程退出前
// 不会释放槽/环/队列，符合“超时或崩溃不提前释放后台仍持有的内存”。
struct StorageCore final {
  explicit StorageCore(std::uint64_t budget_limit) noexcept
      : internal_ledger(budget_limit) {}
  ~StorageCore() { Release(); }

  StorageCore(const StorageCore &) = delete;
  StorageCore &operator=(const StorageCore &) = delete;

  bool Prepare(const StorageConfig &options) noexcept;
  void Release() noexcept;
  bool Notify() noexcept;
  void Fail(Reason reason, FailSource source, std::uint32_t code,
            const char *stage) noexcept;
  Reason FaultReason() const noexcept { return diag.FaultReason(); }
  GapRecord BuildGap(const BatchBegin &begin, Reason reason) const noexcept;
  std::uint32_t AcquireSlot() noexcept;
  bool PrepareSlot(std::uint32_t index) noexcept;
  bool ReleaseSlot(std::uint32_t index, bool committed) noexcept;
  void AbandonSlot(std::uint32_t index) noexcept;
  void RequestStop(Reason reason) noexcept;
  bool LedgerCharged(std::uint64_t bytes) noexcept;

  StorageConfig config{};
  StorageLimits limits{};
  BudgetLedger internal_ledger;
  BudgetLedger *ledger = &internal_ledger;
  Diagnostics diag;
  std::unique_ptr<ValueQueue> queue;
  std::vector<std::unique_ptr<Slot>> slots;
  std::unique_ptr<SpscRing<LifecycleEvent>> events;
  std::unique_ptr<SpscRing<GapRecord>> gaps;
  HANDLE wake = nullptr;
  HANDLE reader_thread = nullptr;
  std::atomic<bool> stop{false};
  std::atomic<std::uint32_t> stop_reason{static_cast<std::uint32_t>(Reason::None)};
  std::atomic<std::uint32_t> state{static_cast<std::uint32_t>(StorageState::Idle)};
  std::atomic<std::uint64_t> event_sequence{0};
  std::atomic<std::uint64_t> last_event{0};
  std::atomic<bool> reader_done{false};
  std::atomic<bool> drain_incomplete{false};
  std::uint64_t charged = 0;
};

bool StorageCore::LedgerCharged(std::uint64_t bytes) noexcept {
  if (!ledger->Charge(bytes)) {
    Fail(Reason::Budget, FailSource::Prepare, 0, "budget_charge");
    return false;
  }
  charged += bytes;
  return true;
}

bool StorageCore::Prepare(const StorageConfig &options) noexcept {
  config = options;
  limits = options.limits;
  if (options.ledger) {
    ledger = options.ledger;
  }
  state.store(static_cast<std::uint32_t>(StorageState::Preparing),
              std::memory_order_release);
  // 只接受小于等于设计上限的值：E2E 可以降低自有上限，但不能抬高。
  if (limits.slots == 0 || limits.slots > kBatchSlots ||
      limits.event_capacity == 0 || limits.event_capacity > kEventCapacity ||
      limits.gap_capacity == 0 || limits.gap_capacity > kGapCapacity ||
      limits.queue_items == 0 || limits.queue_items > kQueueItems ||
      limits.queue_bytes == 0 || limits.queue_bytes > kQueueBytes) {
    Fail(Reason::PrepareFailed, FailSource::Prepare, 0, "limits");
    return false;
  }
  try {
    queue = std::make_unique<ValueQueue>(limits.queue_bytes, limits.queue_items);
  } catch (...) {
    Fail(Reason::Budget, FailSource::Prepare, 0, "queue");
    return false;
  }
  // 队列上限是预占预算：计入总量后才是真实上限，而不是“最多还能用”。
  if (!LedgerCharged(limits.queue_bytes))
    return false;
  try {
    slots.reserve(limits.slots);
  } catch (...) {
    Fail(Reason::Budget, FailSource::Prepare, 0, "slot_directory");
    return false;
  }
  for (std::uint32_t i = 0; i < limits.slots; ++i) {
    if (!LedgerCharged(sizeof(RawBatch)))
      return false;
    // 大块分配用默认初始化：不触碰对象/成员数组的 42.5 MiB 页。
    auto slot = std::unique_ptr<Slot>(new (std::nothrow) Slot());
    if (!slot) {
      Fail(Reason::Budget, FailSource::Prepare, 0, "slot");
      return false;
    }
    slot->batch = std::unique_ptr<RawBatch>(new (std::nothrow) RawBatch());
    if (!slot->batch) {
      Fail(Reason::Budget, FailSource::Prepare, 0, "batch_allocation");
      return false;
    }
    slots.push_back(std::move(slot));
  }
  const auto event_bytes =
      std::uint64_t(limits.event_capacity) * sizeof(LifecycleEvent);
  const auto gap_bytes =
      std::uint64_t(limits.gap_capacity) * sizeof(GapRecord);
  if (!LedgerCharged(event_bytes) || !LedgerCharged(gap_bytes))
    return false;
  try {
    events = std::make_unique<SpscRing<LifecycleEvent>>(
        std::make_unique<LifecycleEvent[]>(limits.event_capacity),
        limits.event_capacity);
    gaps = std::make_unique<SpscRing<GapRecord>>(
        std::make_unique<GapRecord[]>(limits.gap_capacity),
        limits.gap_capacity);
  } catch (...) {
    Fail(Reason::Budget, FailSource::Prepare, 0, "rings");
    return false;
  }
  // 自动复位事件：主线程 SetEvent 唤醒 reader，失败即 world 故障。
  wake = CreateEventW(nullptr, FALSE, FALSE, nullptr);
  if (!wake) {
    Fail(Reason::NotificationFailed, FailSource::Prepare, GetLastError(),
         "create_event");
    return false;
  }
  state.store(static_cast<std::uint32_t>(StorageState::Ready),
              std::memory_order_release);
  return true;
}

void StorageCore::Release() noexcept {
  // 释放顺序：先断开 reader 依赖的队列/环，再归还预算。
  slots.clear();
  events.reset();
  gaps.reset();
  queue.reset();
  if (wake) {
    CloseHandle(wake);
    wake = nullptr;
  }
  if (charged) {
    ledger->Release(charged);
    charged = 0;
  }
}

bool StorageCore::Notify() noexcept {
  if (!wake)
    return false;
  if (!SetEvent(wake)) {
    Fail(Reason::NotificationFailed, FailSource::Producer, GetLastError(),
         "set_event");
    return false;
  }
  return true;
}

void StorageCore::Fail(Reason reason, FailSource source, std::uint32_t code,
                       const char *stage) noexcept {
  diag.Fail(reason, source, code, stage);
  const auto expected = static_cast<std::uint32_t>(StorageState::Ready);
  auto current = state.load(std::memory_order_acquire);
  if (current == expected ||
      current == static_cast<std::uint32_t>(StorageState::Preparing)) {
    state.compare_exchange_strong(current,
                                  static_cast<std::uint32_t>(StorageState::Faulted),
                                  std::memory_order_acq_rel);
  }
  // 故障也要唤醒 reader：它仍需排空已发布内容。
  if (wake)
    SetEvent(wake);
}

GapRecord StorageCore::BuildGap(const BatchBegin &begin,
                                Reason reason) const noexcept {
  GapRecord gap{};
  gap.attempt = begin.attempt;
  gap.run = begin.run;
  gap.room = begin.room;
  gap.mutation = begin.mutation;
  gap.outer = begin.outer;
  gap.tick = begin.tick;
  gap.scope = begin.scope;
  gap.event_watermark = begin.event_watermark;
  gap.reason = reason;
  gap.recording_generation = begin.recording_generation;
  gap.authority_kind = static_cast<std::uint32_t>(begin.authority_kind);
  return gap;
}

std::uint32_t StorageCore::AcquireSlot() noexcept {
  for (std::uint32_t i = 0; i < slots.size(); ++i) {
    auto expected = static_cast<std::uint32_t>(SlotState::Free);
    if (slots[i]->state.compare_exchange_strong(
            expected, static_cast<std::uint32_t>(SlotState::Writing),
            std::memory_order_acq_rel, std::memory_order_relaxed))
      return i;
  }
  return UINT32_MAX;
}

bool StorageCore::PrepareSlot(std::uint32_t index) noexcept {
  if (index >= slots.size())
    return false;
  auto &slot = *slots[index];
  auto expected = static_cast<std::uint32_t>(SlotState::Writing);
  if (!slot.state.compare_exchange_strong(
          expected, static_cast<std::uint32_t>(SlotState::Prepared),
          std::memory_order_acq_rel, std::memory_order_relaxed))
    return false;
  return true;
}

bool StorageCore::ReleaseSlot(std::uint32_t index, bool committed) noexcept {
  if (index >= slots.size())
    return false;
  auto &slot = *slots[index];
  auto expected = static_cast<std::uint32_t>(SlotState::Prepared);
  if (!slot.state.compare_exchange_strong(
          expected,
          static_cast<std::uint32_t>(committed ? SlotState::Ready
                                              : SlotState::Free),
          std::memory_order_acq_rel, std::memory_order_relaxed))
    return false;
  if (committed)
    Notify();
  return true;
}

void StorageCore::AbandonSlot(std::uint32_t index) noexcept {
  if (index >= slots.size())
    return;
  auto &slot = *slots[index];
  const auto current = slot.state.load(std::memory_order_acquire);
  if (current != static_cast<std::uint32_t>(SlotState::Writing) &&
      current != static_cast<std::uint32_t>(SlotState::Prepared))
    return;
  // 未配对的收束：取消 pending 批次并记录明确 gap，不留下带 tick 的样本。
  const auto gap = BuildGap(slot.begin, Reason::UnpairedScope);
  slot.state.store(static_cast<std::uint32_t>(SlotState::Free),
                   std::memory_order_release);
  diag.gaps.fetch_add(1, std::memory_order_relaxed);
  if (gaps && gaps->TryPush(gap))
    diag.gaps_produced.fetch_add(1, std::memory_order_relaxed);
  else
    Fail(Reason::EventOverflow, FailSource::Producer, 0, "gap_ring");
  Notify();
}

void StorageCore::RequestStop(Reason reason) noexcept {
  auto expected = static_cast<std::uint32_t>(Reason::None);
  stop_reason.compare_exchange_strong(expected,
                                      static_cast<std::uint32_t>(reason),
                                      std::memory_order_acq_rel);
  stop.store(true, std::memory_order_release);
  // Faulted 也进入 Closing：终态仍由 FaultInfo 保留失败原因，不被 Closed 覆盖。
  const auto current = state.load(std::memory_order_acquire);
  const auto reachable =
      current == static_cast<std::uint32_t>(StorageState::Ready) ||
      current == static_cast<std::uint32_t>(StorageState::Preparing) ||
      current == static_cast<std::uint32_t>(StorageState::Faulted);
  if (reachable)
    state.store(static_cast<std::uint32_t>(StorageState::Closing),
                std::memory_order_release);
  if (wake)
    SetEvent(wake);
}

// ===========================================================================
// reader
// ===========================================================================

namespace {

enum class ReadOutcome : std::uint32_t { Empty, Copied, Blocked, Stop };

// 复制最早 attempt 的 READY 槽：先预占队列预算，再取得槽所有权，
// 复制有效前缀后立即归还槽。任何分配失败都归还预算并置故障。
ReadOutcome ReadOneBatch(StorageCore &core) noexcept {
  std::uint32_t selected = UINT32_MAX;
  std::uint64_t selected_attempt = 0;
  for (std::uint32_t i = 0; i < core.slots.size(); ++i) {
    if (core.slots[i]->state.load(std::memory_order_acquire) !=
        static_cast<std::uint32_t>(SlotState::Ready))
      continue;
    const auto attempt = core.slots[i]->attempt.load(std::memory_order_relaxed);
    if (selected == UINT32_MAX || attempt < selected_attempt) {
      selected = i;
      selected_attempt = attempt;
    }
  }
  if (selected == UINT32_MAX)
    return ReadOutcome::Empty;
  auto &slot = *core.slots[selected];
  const auto &header = slot.batch->header;
  const auto objects = header.object_count;
  const auto memberships = header.membership_count;
  if (objects > kObjectBudget || memberships > kMembershipBudget) {
    core.Fail(Reason::Budget, FailSource::Reader, objects, "batch_bounds");
    return ReadOutcome::Stop;
  }
  const auto bytes = std::uint64_t(sizeof(QueueItem)) +
                     std::uint64_t(objects) * sizeof(RawObject) +
                     std::uint64_t(memberships) * sizeof(RawMembership);
  if (!core.queue->Reserve(bytes, kReserveWaitMs)) {
    core.diag.budget_waits.fetch_add(1, std::memory_order_relaxed);
    return ReadOutcome::Blocked;
  }
  auto expected = static_cast<std::uint32_t>(SlotState::Ready);
  if (!slot.state.compare_exchange_strong(
          expected, static_cast<std::uint32_t>(SlotState::Reading),
          std::memory_order_acq_rel, std::memory_order_relaxed)) {
    core.queue->Release(bytes);
    return ReadOutcome::Empty;
  }
  const auto started = NowNanoseconds();
  QueueItem item;
  item.kind = ItemKind::Batch;
  item.bytes = bytes;
  item.batch.header = header;
  item.batch.object_count = objects;
  item.batch.membership_count = memberships;
  if (objects) {
    item.batch.objects =
        std::unique_ptr<RawObject[]>(new (std::nothrow) RawObject[objects]);
    if (!item.batch.objects) {
      slot.state.store(static_cast<std::uint32_t>(SlotState::Free),
                       std::memory_order_release);
      core.queue->Release(bytes);
      core.Fail(Reason::Budget, FailSource::Reader, objects, "object_copy");
      return ReadOutcome::Stop;
    }
  }
  if (memberships) {
    item.batch.memberships = std::unique_ptr<RawMembership[]>(
        new (std::nothrow) RawMembership[memberships]);
    if (!item.batch.memberships) {
      slot.state.store(static_cast<std::uint32_t>(SlotState::Free),
                       std::memory_order_release);
      core.queue->Release(bytes);
      core.Fail(Reason::Budget, FailSource::Reader, memberships,
                "membership_copy");
      return ReadOutcome::Stop;
    }
  }
  // 只复制有效前缀；槽在复制完成后立即归还，编码/写盘不持有槽。
  if (objects)
    std::copy_n(slot.batch->objects.data(), objects, item.batch.objects.get());
  if (memberships)
    std::copy_n(slot.batch->memberships.data(), memberships,
                item.batch.memberships.get());
  slot.state.store(static_cast<std::uint32_t>(SlotState::Free),
                   std::memory_order_release);
  const auto elapsed = NowNanoseconds() - started;
  const auto previous = core.diag.copy_max_ns.load(std::memory_order_relaxed);
  if (elapsed > previous)
    core.diag.copy_max_ns.store(elapsed, std::memory_order_relaxed);
  if (!core.queue->Push(std::move(item))) {
    core.queue->Release(bytes);
    core.Fail(Reason::Budget, FailSource::Queue, 0, "queue_push");
    return ReadOutcome::Stop;
  }
  core.diag.batches_copied.fetch_add(1, std::memory_order_relaxed);
  return ReadOutcome::Copied;
}

// 事件块与控制记录块使用同一预算；事件被弹出后必须有人负责交付。
ReadOutcome ReadEventChunk(StorageCore &core) noexcept {
  const auto available = core.events->Distance();
  if (!available)
    return ReadOutcome::Empty;
  const auto count =
      static_cast<std::uint32_t>((std::min)(available,
                                           std::uint64_t(kEventChunk)));
  const auto bytes = std::uint64_t(sizeof(QueueItem)) +
                     std::uint64_t(count) * sizeof(LifecycleEvent);
  if (!core.queue->Reserve(bytes, kReserveWaitMs)) {
    core.diag.budget_waits.fetch_add(1, std::memory_order_relaxed);
    return ReadOutcome::Blocked;
  }
  QueueItem item;
  item.kind = ItemKind::Events;
  item.bytes = bytes;
  try {
    // 先预留容量：push_back 不会在已经弹出的记录上抛异常。
    item.events.reserve(count);
  } catch (...) {
    core.queue->Release(bytes);
    core.Fail(Reason::Budget, FailSource::Reader, count, "event_copy");
    return ReadOutcome::Stop;
  }
  for (std::uint32_t i = 0; i < count; ++i) {
    LifecycleEvent event{};
    if (!core.events->TryPop(event))
      break;
    item.events.push_back(event);
  }
  if (item.events.empty()) {
    core.queue->Release(bytes);
    return ReadOutcome::Empty;
  }
  core.diag.events_consumed.fetch_add(item.events.size(),
                                      std::memory_order_relaxed);
  if (!core.queue->Push(std::move(item))) {
    core.queue->Release(bytes);
    core.Fail(Reason::Budget, FailSource::Queue, 0, "queue_push");
    return ReadOutcome::Stop;
  }
  return ReadOutcome::Copied;
}

ReadOutcome ReadGapChunk(StorageCore &core) noexcept {
  const auto available = core.gaps->Distance();
  if (!available)
    return ReadOutcome::Empty;
  const auto count =
      static_cast<std::uint32_t>((std::min)(available, std::uint64_t(kGapChunk)));
  const auto bytes = std::uint64_t(sizeof(QueueItem)) +
                     std::uint64_t(count) * sizeof(GapRecord);
  if (!core.queue->Reserve(bytes, kReserveWaitMs)) {
    core.diag.budget_waits.fetch_add(1, std::memory_order_relaxed);
    return ReadOutcome::Blocked;
  }
  QueueItem item;
  item.kind = ItemKind::Gaps;
  item.bytes = bytes;
  try {
    item.gaps.reserve(count);
  } catch (...) {
    core.queue->Release(bytes);
    core.Fail(Reason::Budget, FailSource::Reader, count, "gap_copy");
    return ReadOutcome::Stop;
  }
  for (std::uint32_t i = 0; i < count; ++i) {
    GapRecord gap{};
    if (!core.gaps->TryPop(gap))
      break;
    item.gaps.push_back(gap);
  }
  if (item.gaps.empty()) {
    core.queue->Release(bytes);
    return ReadOutcome::Empty;
  }
  core.diag.gaps_consumed.fetch_add(item.gaps.size(),
                                    std::memory_order_relaxed);
  if (!core.queue->Push(std::move(item))) {
    core.queue->Release(bytes);
    core.Fail(Reason::Budget, FailSource::Queue, 0, "queue_push");
    return ReadOutcome::Stop;
  }
  return ReadOutcome::Copied;
}

bool AnyReadySlot(const StorageCore &core) noexcept {
  for (const auto &slot : core.slots)
    if (slot->state.load(std::memory_order_acquire) ==
        static_cast<std::uint32_t>(SlotState::Ready))
      return true;
  return false;
}

void ReaderLoop(const std::shared_ptr<StorageCore> &core) noexcept {
  core->diag.reader_state.store(static_cast<std::uint32_t>(ThreadState::Running),
                                std::memory_order_release);
  bool draining = false;
  std::uint64_t drain_deadline = 0;
  for (;;) {
    const auto faults = core->config.faults;
    // E2E 背压：只暂停本 DLL 的 raw reader，不暂停游戏线程。
    if (!draining && faults && faults->pause_reader.load(std::memory_order_acquire)) {
      WaitForSingleObject(core->wake, 10);
      continue;
    }
    if (!draining && core->stop.load(std::memory_order_acquire)) {
      draining = true;
      core->diag.reader_state.store(
          static_cast<std::uint32_t>(ThreadState::Draining),
          std::memory_order_release);
      drain_deadline = GetTickCount64() + kDrainMs;
    }
    bool progress = false, blocked = false;
    for (std::uint32_t i = 0; i < kBatchPass; ++i) {
      const auto outcome = ReadOneBatch(*core);
      if (outcome == ReadOutcome::Copied) {
        progress = true;
      } else if (outcome == ReadOutcome::Blocked) {
        blocked = true;
        break;
      } else if (outcome == ReadOutcome::Stop) {
        goto finish;
      } else {
        break;
      }
    }
    if (!blocked) {
      for (std::uint32_t i = 0; i < kEventPass; ++i) {
        const auto outcome = ReadEventChunk(*core);
        if (outcome == ReadOutcome::Copied) {
          progress = true;
        } else if (outcome == ReadOutcome::Blocked) {
          blocked = true;
          break;
        } else if (outcome == ReadOutcome::Stop) {
          goto finish;
        } else {
          break;
        }
      }
    }
    if (!blocked) {
      for (std::uint32_t i = 0; i < kGapPass; ++i) {
        const auto outcome = ReadGapChunk(*core);
        if (outcome == ReadOutcome::Copied) {
          progress = true;
        } else if (outcome == ReadOutcome::Blocked) {
          blocked = true;
          break;
        } else if (outcome == ReadOutcome::Stop) {
          goto finish;
        } else {
          break;
        }
      }
    }
    if (progress)
      continue;
    if (draining) {
      // 排空完成条件：没有 READY 槽、事件环与 gap 环都空。
      if (!blocked && !AnyReadySlot(*core) && core->events->Distance() == 0 &&
          core->gaps->Distance() == 0)
        break;
      if (GetTickCount64() >= drain_deadline) {
        // 超时不是“已完整交付”，终态保留 incomplete。
        core->drain_incomplete.store(true, std::memory_order_release);
        break;
      }
    }
    // 队列满时等预算归还；空闲时等主线程发布或停止。事件是自动复位，
    // 任何一次 SetEvent 都会立即返回，超时只是兜底轮询。
    WaitForSingleObject(core->wake, blocked ? 10 : kIdleWaitMs);
  }
finish:
  core->queue->CloseProducer();
  const auto faulted = core->FaultReason() != Reason::None;
  core->diag.reader_state.store(
      static_cast<std::uint32_t>(faulted ? ThreadState::Failed
                                        : ThreadState::Done),
      std::memory_order_release);
  core->reader_done.store(true, std::memory_order_release);
}

struct ReaderStart final {
  std::shared_ptr<StorageCore> core;
};

DWORD WINAPI ReaderEntry(LPVOID parameter) noexcept {
  std::unique_ptr<ReaderStart> start(static_cast<ReaderStart *>(parameter));
  auto core = std::move(start->core);
  try {
    ReaderLoop(core);
  } catch (...) {
    core->Fail(Reason::ThreadFailed, FailSource::Reader, 0, "reader_exception");
    core->queue->CloseProducer();
    core->reader_done.store(true, std::memory_order_release);
  }
  return 0;
}

} // namespace

// ===========================================================================
// BatchWriter
// ===========================================================================

BatchWriter::BatchWriter(std::shared_ptr<StorageCore> core,
                         std::uint32_t slot) noexcept
    : core_(std::move(core)), slot_(slot) {}
BatchWriter::BatchWriter(BatchWriter &&other) noexcept
    : core_(std::move(other.core_)), slot_(other.slot_),
      prepared_(other.prepared_) {
  other.slot_ = 0;
  other.prepared_ = false;
}
BatchWriter &BatchWriter::operator=(BatchWriter &&other) noexcept {
  if (this != &other) {
    // 覆盖前先按未提交处理旧槽，避免槽悬挂。
    if (core_)
      core_->AbandonSlot(slot_);
    core_ = std::move(other.core_);
    slot_ = other.slot_;
    prepared_ = other.prepared_;
    other.slot_ = 0;
    other.prepared_ = false;
  }
  return *this;
}
BatchWriter::~BatchWriter() {
  if (core_)
    core_->AbandonSlot(slot_);
}
std::uint64_t BatchWriter::Attempt() const noexcept {
  if (!core_ || slot_ >= core_->slots.size())
    return 0;
  return core_->slots[slot_]->attempt.load(std::memory_order_relaxed);
}
BatchHeader &BatchWriter::Header() noexcept {
  // 租约失效时返回占位头：采集器误用不应让游戏进程因空指针崩溃。
  static BatchHeader abandoned;
  if (!core_ || slot_ >= core_->slots.size())
    return abandoned;
  return core_->slots[slot_]->batch->header;
}
RawObject *BatchWriter::Objects() noexcept {
  if (!core_ || slot_ >= core_->slots.size())
    return nullptr;
  return core_->slots[slot_]->batch->objects.data();
}
RawMembership *BatchWriter::Memberships() noexcept {
  if (!core_ || slot_ >= core_->slots.size())
    return nullptr;
  return core_->slots[slot_]->batch->memberships.data();
}
bool BatchWriter::Prepare() noexcept {
  if (!core_ || prepared_)
    return false;
  if (!core_->PrepareSlot(slot_))
    return false;
  prepared_ = true;
  return true;
}
bool BatchWriter::Release(bool committed) noexcept {
  if (!core_)
    return false;
  if (!prepared_ && !Prepare())
    return false;
  const auto released = core_->ReleaseSlot(slot_, committed);
  core_.reset();
  prepared_ = false;
  return released;
}
bool BatchWriter::Commit(bool committed) noexcept { return Release(committed); }

// ===========================================================================
// WorldStorage
// ===========================================================================

WorldStorage::WorldStorage() noexcept = default;
WorldStorage::~WorldStorage() {
  if (!core_)
    return;
  // 只发停止信号并关闭线程句柄：reader 持有 core 的 shared_ptr，会自行收口；
  // 不在 DllMain/loader lock 中等待或释放后台仍在使用的内存。
  core_->RequestStop(Reason::Stopped);
  if (core_->reader_thread) {
    CloseHandle(core_->reader_thread);
    core_->reader_thread = nullptr;
  }
}

bool WorldStorage::Prepare(const StorageConfig &config) noexcept {
  if (core_)
    return false;
  try {
    core_ = std::make_shared<StorageCore>(kWorldBudgetBytes);
  } catch (...) {
    return false;
  }
  if (!core_->Prepare(config)) {
    // 没有 reader 可等：立即标记已收口，供调用方读取首因后关闭。
    // 未成功准备的槽/环/队列立刻归还，避免失败上下文长期占用 130 MiB。
    core_->Release();
    core_->diag.reader_state.store(
        static_cast<std::uint32_t>(ThreadState::Failed),
        std::memory_order_release);
    core_->reader_done.store(true, std::memory_order_release);
    return false;
  }
  auto start = std::unique_ptr<ReaderStart>(new (std::nothrow) ReaderStart{core_});
  if (!start) {
    core_->Fail(Reason::ThreadFailed, FailSource::Prepare, 0, "reader_start");
    core_->reader_done.store(true, std::memory_order_release);
    return false;
  }
  const auto handle = CreateThread(nullptr, 0, ReaderEntry, start.get(), 0, nullptr);
  if (!handle) {
    core_->Fail(Reason::ThreadFailed, FailSource::Prepare, GetLastError(),
                "reader_thread");
    core_->reader_done.store(true, std::memory_order_release);
    return false;
  }
  start.release();
  core_->reader_thread = handle;
  return true;
}

bool WorldStorage::Ready() const noexcept {
  return core_ &&
         core_->state.load(std::memory_order_acquire) ==
             static_cast<std::uint32_t>(StorageState::Ready);
}

BatchWriter WorldStorage::Begin(const BatchBegin &begin) noexcept {
  if (!Ready() || core_->FaultReason() != Reason::None)
    return {};
  core_->diag.attempts.fetch_add(1, std::memory_order_relaxed);
  core_->diag.last_attempt.store(begin.attempt, std::memory_order_relaxed);
  const auto index = core_->AcquireSlot();
  if (index == UINT32_MAX) {
    core_->diag.backpressure.fetch_add(1, std::memory_order_relaxed);
    // 没有数据槽时仍要交付独立 gap 控制记录，保留 attempt 与原因。
    const auto gap = core_->BuildGap(begin, Reason::Backpressure);
    PublishGap(gap);
    return {};
  }
  auto &slot = *core_->slots[index];
  slot.attempt.store(begin.attempt, std::memory_order_relaxed);
  slot.begin = begin;
  auto &batch = *slot.batch;
  batch.Reset();
  auto &header = batch.header;
  header.run = begin.run;
  header.room = begin.room;
  header.mutation = begin.mutation;
  header.attempt = begin.attempt;
  header.capture = 0;
  header.outer = begin.outer;
  header.tick = begin.tick;
  header.scope = begin.scope;
  header.event_watermark = begin.event_watermark;
  header.authority = begin.authority ? 1u : 0u;
  header.commit_kind = static_cast<std::uint32_t>(begin.authority_kind);
  header.thread = GetCurrentThreadId();
  core_->diag.producer_thread.store(header.thread, std::memory_order_relaxed);
  header.complete = 0;
  header.reason = static_cast<std::uint32_t>(Reason::None);
  header.recording_generation = begin.recording_generation;
  return BatchWriter(core_, index);
}

bool WorldStorage::PublishEvent(LifecycleEvent event) noexcept {
  if (!core_ || !Ready() || core_->FaultReason() != Reason::None)
    return false;
  std::uint64_t sequence = 0;
  auto current = core_->event_sequence.load(std::memory_order_relaxed);
  for (;;) {
    if (current >= UINT64_MAX - 1) {
      core_->Fail(Reason::SequenceExhausted, FailSource::Producer, 0,
                  "event_sequence");
      return false;
    }
    if (core_->event_sequence.compare_exchange_weak(current, current + 1,
                                                    std::memory_order_acq_rel,
                                                    std::memory_order_relaxed)) {
      sequence = current + 1;
      break;
    }
  }
  event.sequence = sequence;
  if (!core_->events->TryPush(event)) {
    core_->Fail(Reason::EventOverflow, FailSource::Producer,
                core_->events->Capacity(), "event_ring");
    return false;
  }
  core_->diag.events_produced.fetch_add(1, std::memory_order_relaxed);
  core_->last_event.store(sequence, std::memory_order_relaxed);
  core_->Notify();
  return true;
}

bool WorldStorage::PublishGap(const GapRecord &record) noexcept {
  if (!core_ || !Ready() || core_->FaultReason() != Reason::None)
    return false;
  core_->diag.gaps.fetch_add(1, std::memory_order_relaxed);
  if (!core_->gaps->TryPush(record)) {
    core_->Fail(Reason::EventOverflow, FailSource::Producer,
                core_->gaps->Capacity(), "gap_ring");
    return false;
  }
  core_->diag.gaps_produced.fetch_add(1, std::memory_order_relaxed);
  core_->Notify();
  return true;
}

std::uint64_t WorldStorage::EventWatermark() const noexcept {
  return core_ ? core_->event_sequence.load(std::memory_order_acquire) : 0;
}

void WorldStorage::RequestStop(Reason reason) noexcept {
  if (core_)
    core_->RequestStop(reason == Reason::None ? Reason::Stopped : reason);
}

bool WorldStorage::Close(std::uint32_t wait_ms) noexcept {
  if (!core_)
    return false;
  RequestStop(Reason::Stopped);
  if (!wait_ms)
    return core_->reader_done.load(std::memory_order_acquire);
  if (core_->reader_thread) {
    const auto waited =
        WaitForSingleObject(core_->reader_thread, (std::min)(wait_ms, 5000u));
    if (waited == WAIT_OBJECT_0) {
      CloseHandle(core_->reader_thread);
      core_->reader_thread = nullptr;
    } else {
      return false;
    }
  }
  if (core_->reader_done.load(std::memory_order_acquire) &&
      core_->state.load(std::memory_order_acquire) !=
          static_cast<std::uint32_t>(StorageState::Closed))
    core_->state.store(static_cast<std::uint32_t>(StorageState::Closed),
                       std::memory_order_release);
  return core_->reader_done.load(std::memory_order_acquire);
}

bool WorldStorage::Closed() const noexcept {
  return !core_ || core_->reader_done.load(std::memory_order_acquire);
}
bool WorldStorage::DrainIncomplete() const noexcept {
  return !core_ || core_->drain_incomplete.load(std::memory_order_acquire);
}

Diagnostics &WorldStorage::Diag() noexcept { return core_->diag; }
ValueQueue &WorldStorage::Queue() noexcept { return *core_->queue; }
BudgetLedger &WorldStorage::Ledger() noexcept { return *core_->ledger; }
FaultSwitches *WorldStorage::Faults() noexcept {
  return core_ ? core_->config.faults : nullptr;
}
const FaultSwitches *WorldStorage::Faults() const noexcept {
  return core_ ? core_->config.faults : nullptr;
}

StorageStatus WorldStorage::Status() const noexcept {
  StorageStatus status{};
  if (!core_)
    return status;
  status.state = static_cast<StorageState>(core_->state.load(std::memory_order_acquire));
  status.reader =
      static_cast<ThreadState>(core_->diag.reader_state.load(std::memory_order_acquire));
  status.fault = core_->diag.Fault();
  status.limits = core_->limits;
  status.attempts = core_->diag.attempts.load(std::memory_order_relaxed);
  status.captures = core_->diag.captures.load(std::memory_order_relaxed);
  status.complete_captures =
      core_->diag.complete_captures.load(std::memory_order_relaxed);
  status.gaps = core_->diag.gaps.load(std::memory_order_relaxed);
  status.events_produced =
      core_->diag.events_produced.load(std::memory_order_relaxed);
  status.events_consumed =
      core_->diag.events_consumed.load(std::memory_order_relaxed);
  status.events_written =
      core_->diag.events_written.load(std::memory_order_relaxed);
  status.gaps_produced = core_->diag.gaps_produced.load(std::memory_order_relaxed);
  status.gaps_consumed = core_->diag.gaps_consumed.load(std::memory_order_relaxed);
  status.gaps_written = core_->diag.gaps_written.load(std::memory_order_relaxed);
  status.batches_copied =
      core_->diag.batches_copied.load(std::memory_order_relaxed);
  status.batches_written =
      core_->diag.batches_written.load(std::memory_order_relaxed);
  status.backpressure = core_->diag.backpressure.load(std::memory_order_relaxed);
  status.budget_waits = core_->diag.budget_waits.load(std::memory_order_relaxed);
  status.callbacks = core_->diag.callbacks.load(std::memory_order_relaxed);
  status.callback_faults =
      core_->diag.callback_faults.load(std::memory_order_relaxed);
  status.copy_max_ns = core_->diag.copy_max_ns.load(std::memory_order_relaxed);
  status.last_attempt = core_->diag.last_attempt.load(std::memory_order_relaxed);
  status.last_capture = core_->diag.last_capture.load(std::memory_order_relaxed);
  status.last_event = core_->last_event.load(std::memory_order_relaxed);
  status.producer_thread =
      core_->diag.producer_thread.load(std::memory_order_relaxed);
  status.budget_used = core_->ledger->Used();
  status.budget_peak = core_->ledger->Peak();
  status.budget_limit = core_->ledger->Limit();
  return status;
}

} // namespace cs::world
