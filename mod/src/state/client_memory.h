#pragma once
#include "core/replication_memory.h"
#include "state/budget.h"
#include <new>
#include <limits>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>

namespace cs::state {
enum class ClientApplyStage : unsigned {
  Receive,Decode,Inspect,Plan,Describe,Admit,MainPrepare,Install,Readback,Publish,Feedback,Accept,Ack,Retire,EndToEnd,Manager,
  MainObjects,MainFields,AtomicDescribe,NativeWrite,NativePublish,MainQueue,
  AtomicInputs,AtomicIndex,AtomicFields,AtomicLeaves,AtomicOrder,AtomicRanges,
  NativeInitialize,NativeOwners,NativeResolve,NativeFinish,
  PlanImport,PlanWalk,PlanPaths,PlanCounts,PlanRanges,PlanIndex,PlanRules,PlanCommands,
  DecodeRingBase,DecodeNormalize,BackgroundGap,WorldBusyPacket,
  // 快速路径（设计 10.6 节）：整批资格判定与写程序执行。
  FastCheck,FastWrite,Count
};
enum class ClientApplyWait : unsigned {OwnStream,Joint,Dependency,Resources,Budget,Expired,Stale,Mailbox,Cancelled,OldBase,Duplicate,
  BodyQueued,BodyReplaced,BodyQueueExpired,BodyRebased,ResourceYieldFrame,TableWaitExpired,
  WorldQueueReplaced,WorldQueueExpired,CpuQueueBorrow,ManagerOver8ms,
  Count};
enum class ClientApplyVolume : unsigned {Values,Structure,Dependencies,Reused,Drift,Checks,ReadBytes,Constructed,Retained,
  RawOperations,NativeOperations,LateOperations,DelegatedOperations,
  ReadPageQueries,ReadPageFull,WritePageQueries,WritePageFull,SingleFrames,JointFrames,
  PureWriteBytes,PublishReadBytes,UnchangedWorldBytes,PlanReusedRecords,NativeReusedOrder,WorldRingBaseDecoded,
  PlanReusedWorldBlocks,
  WorldWriteOperations,
  WriteOperations,WriteSegments,
  PlanSharedRecords,
  PoolMembersDeferred,
  // 每批按变化量安装时整体跳过纯值叶与可跳过委托的静止记录数。
  QuietRecords,
  // 快速路径（设计 10.6 节）：整批快速帧命中／未命中、事务内被吸收记录、写程序表条目、写程序实际写入的叶数。
  FastHits,FastMisses,AbsorbedRecords,StoreRecords,FastLeaves,Count};
// 每个连接固定两流的聚合桶。主线程和 worker 都可写，详细字段只在显式诊断读取。
class ClientApplyMetrics final {
public:
  static std::uint64_t Now() noexcept {
    return std::uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count());
  }
  void Mark(std::uint8_t streams,ClientApplyStage,std::uint64_t elapsed_us=0) noexcept;
  void Wait(std::uint8_t streams,ClientApplyWait) noexcept;
  void Volume(std::uint8_t streams,ClientApplyVolume,std::uint64_t=1) noexcept;
  struct Issue {
    std::uint64_t route=0,ticket=0,member=0,instance=0,expected=0,actual=0;
    std::uint32_t resource_epoch=0,resource_key=UINT32_MAX,target_version=0,field=UINT32_MAX;
    std::uint16_t layout=0;
    std::uint8_t block=UINT8_MAX;
    std::array<char,96> phase{},reason{};
  };
  // 首个可定位失败随连接保留，显示资格单独记录以免被其它通道的前置失败淹没。
  void FirstIssue(std::uint8_t,Issue,const char *phase,const char *reason,bool display=false) noexcept;
  // 暂态等待保留最近原因，与首个实际失败分开，便于定位准备期限耗尽。
  void LastWait(std::uint8_t,Issue,const char *phase,const char *reason) noexcept;
#ifdef ISAACCS_E2E
  // M0 首因只累计到固定桶。D1 首字段独立保留，避免占用真实失败记录。
  void CadenceReason(bool recipe,const char *reason) noexcept;
  void FirstD1(Issue) noexcept;
#endif
  void Holds(std::uint64_t published,std::uint64_t pending,std::uint64_t retired_native) noexcept;
  void BackgroundHolds(std::uint64_t waiting,std::uint64_t in_flight,std::uint64_t retired) noexcept;
  void RequestBlockSample()const noexcept{sample_requested_.store(true,std::memory_order_release);}
  bool TakeBlockSampleRequest()noexcept{return sample_requested_.exchange(false,std::memory_order_acq_rel);}
  void SetBlockSample(const std::array<std::uint64_t,3> &)noexcept;
  // World、Body 分别统计到实际原生发布为止的安装样本。
  struct InstallTiming {std::uint64_t count=0,total_us=0;};
  std::array<InstallTiming,2> ReadInstallTiming() const noexcept;
  std::string Fields(unsigned page)const;
private:
  struct Stage {
    std::atomic<std::uint64_t> count{0},total{0},maximum{0},last{0},interval{0};
    std::atomic<std::uint64_t> over_2ms{0},over_4ms{0};
    std::array<std::atomic<std::uint64_t>,32> buckets{};
  };
  struct Stream {
    std::array<Stage,unsigned(ClientApplyStage::Count)> stages{};
    std::array<std::atomic<std::uint64_t>,unsigned(ClientApplyWait::Count)> waits{};
    std::array<std::atomic<std::uint64_t>,unsigned(ClientApplyVolume::Count)> volumes{};
  };
  std::array<Stream,2> streams_{};
  std::array<std::atomic<std::uint64_t>,6> holds_{};
  mutable std::atomic<bool> sample_requested_{false};
  std::atomic<std::uint64_t> sample_time_{0};
  std::array<std::atomic<std::uint64_t>,3> block_bytes_{};
  mutable std::mutex issue_mutex_;
  std::array<Issue,3> issues_{};
  std::array<Issue,2> last_wait_{};
#ifdef ISAACCS_E2E
  std::array<std::atomic<std::uint64_t>,8> recipe_misses_{};
  std::array<std::atomic<std::uint64_t>,25> skip_misses_{};
  Issue first_d1_{};
#endif
};
// 作用域只量阶段墙钟，暂停期间的等待另由反馈及端到端指标表达。
class ClientApplyTimer final {
public:
  ClientApplyTimer(ClientApplyMetrics &m,std::uint8_t streams,ClientApplyStage stage) noexcept:
    metrics_(m),streams_(streams),stage_(stage),start_(ClientApplyMetrics::Now()) {}
  ~ClientApplyTimer(){metrics_.Mark(streams_,stage_,ClientApplyMetrics::Now()-start_);}
private:
  ClientApplyMetrics &metrics_;
  std::uint8_t streams_;
  ClientApplyStage stage_;
  std::uint64_t start_;
};
#ifdef ISAACCS_E2E
// 片内先按原时钟精度累加，避免每条小记录分别截断为微秒或写入原子计数。
struct ClientApplyDetailTimer {
  using Clock=std::chrono::steady_clock;
  Clock::duration *total;
  Clock::time_point start;
  explicit ClientApplyDetailTimer(Clock::duration *elapsed) noexcept:total(elapsed),start(elapsed?Clock::now():Clock::time_point{}) {}
  ~ClientApplyDetailTimer(){if(total)*total+=Clock::now()-start;}
  static std::uint64_t Microseconds(Clock::duration elapsed) noexcept {
    return std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
  }
};
#endif
// 类型化本地失败只用于后台调度；不新增线上枚举，也不通过Gap.what()猜等待原因。
enum class ClientTableFailure : std::uint8_t {
  Budget, WorkspaceBusy, WrongThread, WrongRoute, InvalidEncoding, InvalidReference,
  EpochChain, EpochConflict, SnapshotLimit,
  MissingTable, MissingLocalResource, ResourceFailure, Busy, Generation, Internal
};
class ClientTableError final : public std::runtime_error {
public:
  ClientTableError(ClientTableFailure failure,const char *reason,std::uint64_t requested=0,std::uint64_t available=0,
      memory::Pool pool=memory::Pool::Values):std::runtime_error(reason),failure_(failure),pool_(pool),requested_(requested),available_(available) {}
  ClientTableFailure Failure() const noexcept {return failure_;}
  memory::Pool Pool() const noexcept {return pool_;}
  std::uint64_t Requested() const noexcept {return requested_;}
  std::uint64_t Available() const noexcept {return available_;}
private:
  ClientTableFailure failure_;
  memory::Pool pool_;
  std::uint64_t requested_=0,available_=0;
};

// 同一个Peer的表、后续影子、缓存与正文必须共享本账户，实际额度仍取自进程三池。
// 分配器按实际allocate数量收费，包含vector capacity及allocate_shared控制块；释放归还。
class ClientStateMemory final {
public:
  static std::shared_ptr<ClientStateMemory> Create();
  void *Allocate(memory::Pool,std::size_t,std::size_t alignment);
  void Deallocate(memory::Pool,void *,std::size_t,std::size_t alignment) noexcept;
  // 原生 CRT 分配仍计入同一 Peer；仅记账，不接管其指针或跨线程调用原生释放。
  void ChargeNative(std::size_t bytes);
  void UnchargeNative(std::size_t bytes) noexcept;
  std::array<std::uint64_t,3> Usage() const noexcept;
  ClientApplyMetrics &ApplyMetrics() noexcept{return apply_metrics_;}
  const ClientApplyMetrics &ApplyMetrics()const noexcept{return apply_metrics_;}
private:
  ClientStateMemory();
  mutable std::mutex mutex_;
  std::array<memory::Lease,3> charges_;
  std::array<std::uint64_t,3> limits_{};
  ClientApplyMetrics apply_metrics_;
};
// Client表与影子共用大型后台工作位置；竞争立即报告，不阻塞主线程。
class ClientStateWorkspace final {
public:
  ClientStateWorkspace();
  ~ClientStateWorkspace();
  ClientStateWorkspace(const ClientStateWorkspace &)=delete;
  ClientStateWorkspace &operator=(const ClientStateWorkspace &)=delete;
  // 子步骤显式借用同一租约，不重新抢占全局工作区；跨线程借用仍拒绝。
  void RequireCurrentThread() const;
private:
  const std::thread::id thread_=std::this_thread::get_id();
};
template<class T> class ClientStateAllocator {
public:
  using value_type=T;
  using propagate_on_container_move_assignment=std::true_type;
  using propagate_on_container_swap=std::true_type;
  ClientStateAllocator(std::shared_ptr<ClientStateMemory> memory,memory::Pool pool=memory::Pool::Values) noexcept
      :memory_(std::move(memory)),pool_(pool) {}
  template<class U> ClientStateAllocator(const ClientStateAllocator<U> &other) noexcept:memory_(other.Memory()),pool_(other.Pool()) {}
  T *allocate(std::size_t count) {
    if(!memory_ || count>std::numeric_limits<std::size_t>::max()/sizeof(T))
      throw ClientTableError(ClientTableFailure::Budget,"client_allocation_overflow");
    return static_cast<T *>(memory_->Allocate(pool_,count*sizeof(T),alignof(T)));
  }
  void deallocate(T *value,std::size_t count) noexcept {memory_->Deallocate(pool_,value,count*sizeof(T),alignof(T));}
  const std::shared_ptr<ClientStateMemory> &Memory() const noexcept {return memory_;}
  memory::Pool Pool() const noexcept {return pool_;}
  template<class U> bool operator==(const ClientStateAllocator<U> &other) const noexcept {
    return memory_==other.Memory() && pool_==other.Pool();
  }
private:
  std::shared_ptr<ClientStateMemory> memory_;
  memory::Pool pool_;
};
} // namespace cs::state
