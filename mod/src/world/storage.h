#pragma once
// tx-04-08 world 存储层：槽所有权、通知、有界值队列、gap 控制记录与
// 原始复制线程。
//
// 线程与所有权（迁移设计第 3.1、7.1 节）：
//   主线程   只写自己持有的 WRITING 槽，随后 PREPARED，core 提交后 READY；
//            只做原子操作与 SetEvent，从不等待 writer 或磁盘。
//   reader   取得最早 attempt 的 READY 槽，预占队列预算，复制有效前缀后
//            立即归还槽；只把 owned values 交给队列。
//   writer   由 writer.h 提供，只处理队列中的 owned values。
//
// 槽状态：FREE → WRITING → PREPARED → READY → READING → FREE。
// 主线程在 core 提交前停在 PREPARED，后台不可见；未提交的批次回到 FREE，
// 不留下带新 Authority tick 的“完整”世界样本。
//
// 队列预算同时计入 reader 预留、排队、writer 在途与事件；两个上限（字节与
// 项数）必须同时满足。队列不足时 reader 有界等待自有 writer 释放预算，
// 主线程不等待，只记录背压 gap。

#include "world/model.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <vector>

namespace cs::world {

// ===========================================================================
// 1. 基础枚举与诊断
// ===========================================================================

enum class StorageState : std::uint32_t {
  Idle,
  Preparing,
  Ready,
  Faulted,
  Closing,
  Closed
};
enum class ThreadState : std::uint32_t {
  NotStarted,
  Running,
  Draining,
  Done,
  Failed
};
// world 自有故障的来源；与真实 Authority/原生故障分开，不冒称游戏故障。
enum class FailSource : std::uint32_t {
  None,
  Producer,
  Reader,
  Writer,
  Queue,
  Prepare
};
const char *StorageStateName(StorageState value) noexcept;
const char *ThreadStateName(ThreadState value) noexcept;
const char *FailSourceName(FailSource value) noexcept;

// 故障只保留首因：后续错误不覆盖已经记录的失败事实。
struct FaultInfo final {
  Reason reason = Reason::None;
  FailSource source = FailSource::None;
  std::uint32_t code = 0;
  const char *stage = "";
  std::uint64_t at_ms = 0;
};

// 跨线程只经原子读写。Producer=主线程，Reader=原始复制线程，Writer=编码线程。
// callback 计数由主线程桥登记，本层只保存值。
struct Diagnostics final {
  std::atomic<std::uint32_t> reader_state{static_cast<std::uint32_t>(
      ThreadState::NotStarted)};
  std::atomic<std::uint32_t> writer_state{static_cast<std::uint32_t>(
      ThreadState::NotStarted)};
  std::atomic<std::uint64_t> attempts{0}, captures{0}, complete_captures{0},
      gaps{0};
  std::atomic<std::uint64_t> events_produced{0}, events_consumed{0},
      events_written{0};
  std::atomic<std::uint64_t> gaps_produced{0}, gaps_consumed{0},
      gaps_written{0};
  std::atomic<std::uint64_t> batches_copied{0}, batches_written{0};
  std::atomic<std::uint64_t> checkpoints_produced{0}, checkpoints_written{0};
  std::atomic<std::uint64_t> backpressure{0}, budget_waits{0};
  std::atomic<std::uint64_t> callbacks{0}, callback_faults{0};
  std::atomic<std::uint64_t> copy_max_ns{0};
  std::atomic<std::uint32_t> producer_thread{0};
  std::atomic<std::uint64_t> last_attempt{0}, last_capture{0};

  // 记录首个故障；返回是否由本次调用写入。
  bool Fail(Reason reason, FailSource source, std::uint32_t code,
            const char *stage) noexcept;
  FaultInfo Fault() const noexcept;
  Reason FaultReason() const noexcept {
    return static_cast<Reason>(fault_reason_.load(std::memory_order_acquire));
  }

private:
  std::atomic<std::uint32_t> fault_reason_{
      static_cast<std::uint32_t>(Reason::None)};
  std::atomic<std::uint32_t> fault_source_{
      static_cast<std::uint32_t>(FailSource::None)};
  std::atomic<std::uint32_t> fault_code_{0};
  std::atomic<const char *> fault_stage_{""};
  std::atomic<std::uint64_t> fault_at_ms_{0};
};

// ===========================================================================
// 2. 有界值队列
// ===========================================================================

// 直接从 READY 槽复制出来的 owned batch：不再持有任何游戏地址。
struct OwnedBatch final {
  BatchHeader header{};
  std::unique_ptr<RawObject[]> objects;
  std::unique_ptr<RawMembership[]> memberships;
  std::uint32_t object_count = 0;
  std::uint32_t membership_count = 0;

  OwnedBatch() noexcept = default;
  OwnedBatch(OwnedBatch &&) noexcept = default;
  OwnedBatch &operator=(OwnedBatch &&) noexcept = default;
  OwnedBatch(const OwnedBatch &) = delete;
  OwnedBatch &operator=(const OwnedBatch &) = delete;
};

enum class ItemKind : std::uint32_t { Batch, Events, Gaps, Checkpoints };

// 队列项：一次复制或一段事件/控制记录，计费字节含其自身管理开销。
struct QueueItem final {
  ItemKind kind = ItemKind::Batch;
  std::uint64_t bytes = 0;
  OwnedBatch batch{};
  std::vector<LifecycleEvent> events{};
  std::vector<GapRecord> gaps{};
  std::vector<CheckpointRecord> checkpoints{};

  QueueItem() noexcept = default;
  QueueItem(QueueItem &&) noexcept = default;
  QueueItem &operator=(QueueItem &&) noexcept = default;
  QueueItem(const QueueItem &) = delete;
  QueueItem &operator=(const QueueItem &) = delete;
};

struct QueueStats final {
  std::uint64_t held_items = 0, held_bytes = 0, peak_items = 0, peak_bytes = 0,
                pushes = 0, pops = 0, budget_waits = 0, discarded_items = 0;
  bool producer_done = false;
  bool empty = true;
};

// reader 是唯一生产者，writer 是唯一消费者；主线程从不接触队列。
class ValueQueue final {
public:
  ValueQueue(std::uint64_t bytes, std::uint32_t items) noexcept;
  ~ValueQueue();
  ValueQueue(const ValueQueue &) = delete;
  ValueQueue &operator=(const ValueQueue &) = delete;

  // 复制前预占；预算不足时有界等待 writer 释放。返回 false 表示超时，
  // 调用方记录背压并稍后重试，不能绕过预算继续复制。
  bool Reserve(std::uint64_t bytes, std::uint32_t wait_ms) noexcept;
  // 并入队（预占已包含本次字节），唤醒 writer。分配失败返回 false，
  // 调用方必须归还预留并把本次录制置为故障。
  bool Push(QueueItem &&item) noexcept;
  // writer 完成编码/写入后归还预算，唤醒等待预算的 reader。
  void Release(std::uint64_t bytes) noexcept;
  // writer 取出一项；超时或空返回 false，由调用方检查 ProducerDone()。
  bool Pop(QueueItem &item, std::uint32_t wait_ms) noexcept;
  // reader 退出：writer 依据它决定何时排空结束。
  void CloseProducer() noexcept;
  bool ProducerDone() const noexcept;
  bool Empty() const noexcept;
  std::uint64_t ByteLimit() const noexcept { return byte_limit_; }
  std::uint32_t ItemLimit() const noexcept { return item_limit_; }
  QueueStats Read() const noexcept;
  void NoteDiscarded(std::uint64_t items) noexcept;

private:
  std::uint64_t byte_limit_;
  std::uint32_t item_limit_;
  mutable std::mutex mutex_;
  std::condition_variable ready_;
  std::condition_variable space_;
  std::deque<QueueItem> items_;
  // 计数使用原子：状态查询（主线程/控制口）不得为了读统计而取得队列锁。
  std::atomic<std::uint64_t> held_items_{0}, held_bytes_{0}, peak_items_{0},
      peak_bytes_{0}, pushes_{0}, pops_{0}, budget_waits_{0}, discarded_{0};
  std::atomic<bool> producer_done_{false};
};

// ===========================================================================
// 3. 存储配置与主线程写槽
// ===========================================================================

struct StorageLimits final {
  std::uint32_t slots = kBatchSlots;
  std::uint32_t event_capacity = kEventCapacity;
  std::uint32_t gap_capacity = kGapCapacity;
  std::uint64_t queue_bytes = kQueueBytes;
  std::uint32_t queue_items = kQueueItems;
};

// 仅 E2E 使用的可控故障开关：只在显式 E2E 配置与匹配 token 下由控制口设置，
// 默认全关，远端输入协议无法传入。所有效果都标为 TEST_INJECTED。
struct FaultSwitches final {
  // 暂停 DLL 自有 raw reader，触发真实槽位耗尽（不暂停游戏线程）。
  std::atomic<bool> pause_reader{false};
  // 作家关闭自有 captures 句柄，使下一次真实 WriteFile 失败。
  std::atomic<bool> drop_capture_file{false};
  // 自动恢复期限；stop/fault 路径忽略暂停，绝不暂停游戏线程。
  std::atomic<std::uint64_t> pause_until{0};
  std::atomic<std::uint32_t> mode{0}, value{0}, applied{0}, before{0}, after{0};
  std::atomic<std::uint32_t> rejection{0}; // 实际 Incomplete 分支，不被较早的首因遮蔽。
  std::atomic<std::uint64_t> attempt{0}, watermark{0};
  std::atomic<bool> worker_done{true};
  std::atomic<bool> reader_paused{false};
  // 以下字段只有主线程访问，录制 owner 在后台准备与释放不可读页。
  std::uint32_t object_limit = kObjectBudget, membership_limit = kMembershipBudget,
                relation_limit = kRelationNodes, unreadable = 0;
  std::uint32_t pending = 0;
};

struct StorageConfig final {
  StorageLimits limits{};
  // 可空；为空表示不做 E2E 故障注入。开关对象必须比 reader 线程活得久。
  FaultSwitches *faults = nullptr; // 可空；不拥有
  // 可空；为空时使用本模块内部的 256 MiB 预算账本。身份表与 writer 共用
  // 同一账本时由调用方提供，保证总量仍是 256 MiB。
  BudgetLedger *ledger = nullptr;
};

// 一轮收束尝试的候选值；Begin 失败时用于生成 gap 控制记录。
struct BatchBegin final {
  std::uint64_t attempt = 0;
  std::uint64_t run = 0, room = 0, mutation = 0, outer = 0, tick = 0, scope = 0;
  std::uint64_t event_watermark = 0;
  std::uint32_t recording_generation = 0;
  AuthorityKind authority_kind = AuthorityKind::None;
  bool authority = false;
};

struct StorageStatus final {
  StorageState state = StorageState::Idle;
  ThreadState reader = ThreadState::NotStarted;
  FaultInfo fault{};
  // 实际上限（E2E 可能显式降低），manifest 必须登记而不是隐藏。
  StorageLimits limits{};
  std::uint64_t attempts = 0, captures = 0, complete_captures = 0, gaps = 0;
  std::uint64_t events_produced = 0, events_consumed = 0, events_written = 0;
  std::uint64_t gaps_produced = 0, gaps_consumed = 0, gaps_written = 0;
  std::uint64_t batches_copied = 0, batches_written = 0;
  std::uint64_t backpressure = 0, budget_waits = 0;
  std::uint64_t callbacks = 0, callback_faults = 0;
  std::uint64_t copy_max_ns = 0;
  std::uint64_t last_attempt = 0, last_capture = 0, last_event = 0;
  std::uint32_t producer_thread = 0;
  std::uint64_t budget_used = 0, budget_peak = 0, budget_limit = 0;
};

struct StorageCore; // storage.cpp

// 主线程写槽租约。析构时若仍停留在 WRITING/PREPARED，槽退回 FREE 并记录
// 一个 UNPAIRED_SCOPE gap，避免悬挂槽把后续 attempt 静默吞掉。
class BatchWriter final {
public:
  BatchWriter() noexcept = default;
  BatchWriter(BatchWriter &&other) noexcept;
  BatchWriter &operator=(BatchWriter &&other) noexcept;
  ~BatchWriter();
  BatchWriter(const BatchWriter &) = delete;
  BatchWriter &operator=(const BatchWriter &) = delete;

  explicit operator bool() const noexcept { return core_ != nullptr; }
  std::uint64_t Attempt() const noexcept;
  // 以下访问器只在 Begin 成功后、Release 之前有效；无效租约返回安全的
  // 占位头/nullptr，不用空指针崩溃游戏进程（调用方仍应先检查 operator bool）。
  BatchHeader &Header() noexcept;
  RawObject *Objects() noexcept;
  RawMembership *Memberships() noexcept;

  // 数据写完后推进到 PREPARED：后台仍不可见。
  bool Prepare() noexcept;
  // core 的提交结果：true → READY（后台可见），false → FREE（撤销）。
  bool Release(bool committed) noexcept;
  // Prepare + Release 的便捷组合。
  bool Commit(bool committed) noexcept;

private:
  friend class WorldStorage;
  BatchWriter(std::shared_ptr<StorageCore> core, std::uint32_t slot) noexcept;
  std::shared_ptr<StorageCore> core_;
  std::uint32_t slot_ = 0;
  bool prepared_ = false;
};

// ===========================================================================
// 4. WorldStorage
// ===========================================================================

class WorldStorage final {
public:
  WorldStorage() noexcept;
  ~WorldStorage();
  WorldStorage(const WorldStorage &) = delete;
  WorldStorage &operator=(const WorldStorage &) = delete;

  // 在 Preparing 后台线程调用：分配槽/环/队列并启动 reader。
  // 失败只让 world 不可用，记录首因，不抛出、不停 C/S 会话。
  bool Prepare(const StorageConfig &config) noexcept;
  bool Ready() const noexcept;

  // —— 主线程 API：只做原子操作与 SetEvent，零等待 ——
  // 取 FREE 槽；没有可用槽时记录 BUFFER_BACKPRESSURE gap 并返回空句柄。
  // 每次调用即一次收束尝试（含拿不到槽的尝试），计数在内部登记。
  BatchWriter Begin(const BatchBegin &begin) noexcept;
  // 生命周期事件：分配 event_seq 并发布；环满即停用录制。
  bool PublishEvent(LifecycleEvent event) noexcept;
  // gap 控制记录：独立固定容量；交付失败即停用录制。
  bool PublishGap(const GapRecord &gap) noexcept;
  // 独立有界控制环；满或通知失败只使 world 故障，不能静默丢场景标记。
  bool PublishCheckpoint(const CheckpointRecord &checkpoint) noexcept;
  // 最近一次已发布的事件序号；用于批次 event_watermark。
  std::uint64_t EventWatermark() const noexcept;

  // —— 停止与收口 ——
  // 主线程边界调用：只关闭新观察/新样本，已发布内容仍会被 reader/writer 排空。
  void RequestStop(Reason reason) noexcept;
  // 只发停止信号并按需有界等待 reader；wait_ms=0 时立即返回。
  // 主线程路径必须传 0，由后续状态轮询得知 Closed。
  bool Close(std::uint32_t wait_ms) noexcept;
  bool Closed() const noexcept;
  // 停止后 reader 未能在期限内排空：终态必须报告 incomplete，而不是 Closed成功。
  bool DrainIncomplete() const noexcept;

  Diagnostics &Diag() noexcept;
  ValueQueue &Queue() noexcept;
  // 与身份表、writer 共用的预算账本；由配置或内部缺省提供。
  BudgetLedger &Ledger() noexcept;
  // E2E 故障开关（非拥有），writer 需要消费一次性开关，故提供可变访问。
  FaultSwitches *Faults() noexcept;
  const FaultSwitches *Faults() const noexcept;
  StorageStatus Status() const noexcept;

private:
  std::shared_ptr<StorageCore> core_;
};

} // namespace cs::world
