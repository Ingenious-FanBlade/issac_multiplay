#pragma once
#include "state/client_events.h"
#include <atomic>

namespace cs::state {
// 主线程安装计划由具体域owner派生，不能用空计划／成功占位承认未接通的原生字段。
struct ClientStateNativePlan {
  virtual ~ClientStateNativePlan()=default;
  // 后台退休批次的最后一份引用时检查所有派生描述借用，避免主线程释放大缓冲／池锁。
  virtual bool Releasable()const noexcept=0;
};
class ClientStateInstallPlan;
struct ClientStatePreparedTarget {
  ClientStateCandidateRef candidate;
  std::array<ClientShadowStateRef,2> effective;
  // 比较稳定控制分配与结构变化只使用真正成功安装根，不能拿等待候选作基线。
  std::array<ClientShadowStateRef,2> previous;
  ClientStateVector<ClientReceivedEvent> events;
  // 后台完成公共／跨域检查后封存的数字安装计划；存在不代表原生owner已经实现。
  std::shared_ptr<const ClientStateInstallPlan> install;
  // 具体域描述同样在后台只构建一次；其后Inspect仍重查真实资源／原生代次资格。
  std::shared_ptr<const ClientStateNativePlan> described;
  explicit ClientStatePreparedTarget(const std::shared_ptr<ClientStateMemory> &m):events(ClientStateAllocator<ClientReceivedEvent>(m)) {}
};
enum class ClientStateApplyResolution:std::uint8_t {Pending,Committed,Drain};
// 只在接收线程执行：对完整影子作具名域校验，并预先准备无原生地址的安装描述。
// inspect必须检查本地资源完成、窄拓扑、字段owner覆盖、事件支持范围及所有第二批显示值。
class ClientStatePreparation {
public:
  virtual ~ClientStatePreparation()=default;
  // 只能在全部正式域／事件原生owner已经绑定时声明；接口存在本身不是能力证明。
  virtual bool Available() const noexcept=0;
  virtual StateReadiness Inspect(const ClientStatePreparedTarget &,const ClientStateTables &) const=0;
  // 只产生纯值描述，不调用原生准备；在Inspect前封存，不能用此返回值冒充ready。
  virtual std::shared_ptr<const ClientStateNativePlan> Prepare(const ClientStatePreparedTarget &) const=0;
};
// 一次交接固定同一候选、完整两流、事件和原始ticket；撤销只改资格，不销毁在途存储。
struct ClientStateApplyBatch final {
  ClientStatePreparedTarget target;
  StateApplyTicket ticket;
  std::shared_ptr<const ClientStateNativePlan> native;
  mutable std::atomic<bool> cancelled{false};
  mutable std::atomic<ClientStateApplyResolution> resolution{ClientStateApplyResolution::Pending};
  explicit ClientStateApplyBatch(const std::shared_ptr<ClientStateMemory> &m):target(m) {}
};
using ClientStateApplyBatchRef=std::shared_ptr<const ClientStateApplyBatch>;
struct ClientStateApplyCompletion {
  StateApplyTicket ticket;
  StateApplyOutcome outcome=StateApplyOutcome::FailedBeforeWrites;
  std::uint8_t affected_streams=0;
  // 只填真实正常返回的播放前缀；失败后保留，恢复不能重放已发生的声音／文字。
  std::array<std::uint64_t,512> replayed{};
  std::uint16_t replayed_count=0;
  std::uint64_t uncertain_event=0;
  bool drained=false;
};
struct ClientStateResourceJob {
  ClientResourcePreparationTicket ticket;
  ClientResourceSnapshotRef resources;
  std::shared_ptr<const ResourceLocalDescription> described;
};
// 所有接受状态仅由网络worker串行驱动。外部队列是SPSC：满时返回false并由owner保留重试。
class ClientStateReceiveDelivery {
public:
  virtual ~ClientStateReceiveDelivery()=default;
  virtual bool Apply(ClientStateApplyBatchRef)=0;
  virtual bool Resource(const ClientStateResourceJob &)=0;
  virtual bool Control(StateMessageKind,std::span<const std::byte>)=0;
};
class ClientStateReceiver final {
public:
  ClientStateReceiver(std::shared_ptr<ClientStateMemory>,std::span<const ClientShadowLayoutBinding>,
      const ClientStatePreparation &,ClientStateReceiveDelivery &);
  ~ClientStateReceiver();
  void Open(const StateRoute &,StateAgreement,std::uint64_t generation,std::uint64_t now_ms);
  void Close();
  bool Drained() const;
  void Receive(StateMessageKind,StateLane,std::span<const std::byte>,std::uint64_t now_ms);
  void Pump(std::uint64_t now_ms);
  void Complete(const ClientStateApplyCompletion &);
  void CompleteResource(const ClientResourcePreparationTicket &,ClientResourceState);
  void RequireRecovery(std::uint8_t,StateResyncReason,bool structural=false);
  std::array<std::uint32_t,2> Versions() const;
  std::uint64_t Events() const;
private:
  struct Impl;
  std::shared_ptr<Impl> impl_;
};
} // namespace cs::state
