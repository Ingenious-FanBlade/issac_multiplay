#pragma once
#include "state/client_receive.h"

namespace cs::state {
// 各子步骤只返回真实原生事实；Waiting必须没有本次写入，否则转入统一失败排空。
struct ClientStateNativeStep {
  bool ready=false,written=false,failed=false;
  const char *reason="binary_native_waiting";
};
// 一个实际Scene/Manager owner实现全部操作。Publish之前不得向Render暴露候选，
// Drain必须按真实用途借用/资源/对象退休结果回报，不能把取消或逻辑inflight清零当作排空。
class ClientStateNativeOwner {
public:
  virtual ~ClientStateNativeOwner()=default;
  virtual bool Current(const ClientStateApplyBatch &) const noexcept=0;
  virtual ClientStateNativeStep Prepare(const ClientStateApplyBatch &) noexcept=0;
  virtual ClientStateNativeStep Stage(const ClientStateApplyBatch &) noexcept=0;
  virtual ClientStateNativeStep Install(const ClientStateApplyBatch &) noexcept=0;
  virtual ClientStateNativeStep Display(const ClientStateApplyBatch &) noexcept=0;
  virtual ClientStateNativeStep Replay(const ClientStateApplyBatch &,std::size_t event) noexcept=0;
  virtual ClientStateNativeStep Publish(const ClientStateApplyBatch &) noexcept=0;
  virtual void Invalidate(const ClientStateApplyBatch &,const char *) noexcept=0;
  virtual bool Drain(const ClientStateApplyBatch &) noexcept=0;
};
class ClientStateApplyResults {
public:
  virtual ~ClientStateApplyResults()=default;
  virtual bool Complete(const ClientStateApplyCompletion &) noexcept=0;
};
// 唯一主线程writer：一个ticket从准备到发布/排空都不重叠；不解析报文，不运行后台状态机。
class ClientStateApplyCoordinator final {
public:
  ClientStateApplyCoordinator(ClientStateNativeOwner &,ClientStateApplyResults &);
  bool Offer(ClientStateApplyBatchRef) noexcept;
  void Pump() noexcept;
  void Cancel() noexcept;
  bool Busy() const noexcept{return bool(batch_);}
  const char *Reason()const noexcept{return reason_;}
private:
  enum class Phase:std::uint8_t {Prepare,Stage,Install,Display,Replay,Publish,DrainBeforeReport,Report,AwaitDecision,Drain,ReportDrain};
  void Fail(const char *) noexcept;
  bool Step(ClientStateNativeStep,Phase) noexcept;
  ClientStateNativeOwner &owner_;
  ClientStateApplyResults &results_;
  const std::thread::id thread_=std::this_thread::get_id();
  ClientStateApplyBatchRef batch_;
  ClientStateApplyCompletion completion_{};
  Phase phase_=Phase::Prepare;
  bool written_=false,cancelled_=false;
  std::size_t event_=0;
  const char *reason_="binary_idle";
};
} // namespace cs::state
