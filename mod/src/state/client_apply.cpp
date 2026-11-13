#include "state/client_apply.h"

namespace cs::state {
ClientStateApplyCoordinator::ClientStateApplyCoordinator(ClientStateNativeOwner &owner,ClientStateApplyResults &results)
  :owner_(owner),results_(results) {}
bool ClientStateApplyCoordinator::Offer(ClientStateApplyBatchRef batch) noexcept {
  if(thread_!=std::this_thread::get_id()||batch_||!batch||!batch->native||!batch->target.candidate||!batch->ticket.serial||batch->target.events.size()>512)return false;
  batch_=std::move(batch);completion_={};completion_.ticket=batch_->ticket;written_=cancelled_=false;event_=0;phase_=Phase::Prepare;reason_="binary_preparing";return true;
}
void ClientStateApplyCoordinator::Cancel() noexcept {if(thread_==std::this_thread::get_id())cancelled_=true;}
void ClientStateApplyCoordinator::Fail(const char *reason) noexcept {
  reason_=reason;completion_.outcome=written_?StateApplyOutcome::FailedAfterWrites:StateApplyOutcome::FailedBeforeWrites;
  // 正式 owner 发现部分写入时撤整个原生场景，共享 Player 根使两条流都失去差分基线。
  if(written_){completion_.affected_streams=3;owner_.Invalidate(*batch_,reason);}
  phase_=written_?Phase::Report:Phase::DrainBeforeReport;
}
bool ClientStateApplyCoordinator::Step(ClientStateNativeStep step,Phase next) noexcept {
  written_|=step.written;
  if(step.failed||(!step.ready&&written_)){Fail(step.reason);return false;}
  if(!step.ready){reason_=step.reason;return false;}
  phase_=next;return true;
}
void ClientStateApplyCoordinator::Pump() noexcept {
  if(thread_!=std::this_thread::get_id()||!batch_)return;
  // 完成/排空反馈与取消检查分开。取消后的真实Success也必须保留已发生的回放前缀。
  if(phase_<Phase::DrainBeforeReport&&(cancelled_||batch_->cancelled.load(std::memory_order_acquire)||!owner_.Current(*batch_)))Fail("binary_native_cancelled_or_generation_changed");
  // 一个Manager边界内完成整个写入/显示/回放/发布区间；准备允许跨帧等待但不暴露候选。
  for(unsigned transitions=0;batch_&&transitions<536;++transitions) {
    switch(phase_) {
      case Phase::Prepare:if(!Step(owner_.Prepare(*batch_),Phase::Stage))return;break;
      case Phase::Stage:if(!Step(owner_.Stage(*batch_),Phase::Install))return;break;
      case Phase::Install:if(!Step(owner_.Install(*batch_),Phase::Display))return;break;
      case Phase::Display:if(!Step(owner_.Display(*batch_),Phase::Replay))return;break;
      case Phase::Replay:
        if(event_==batch_->target.events.size()){phase_=Phase::Publish;break;}
        {const auto replay=owner_.Replay(*batch_,event_);
          if(replay.failed&&replay.written)completion_.uncertain_event=batch_->target.events[event_].value.sequence;
          if(!Step(replay,Phase::Replay))return;}
        completion_.replayed[completion_.replayed_count++]=batch_->target.events[event_++].value.sequence;break;
      case Phase::Publish:
        if(!Step(owner_.Publish(*batch_),Phase::Report))return;
        completion_.outcome=StateApplyOutcome::Success;reason_="binary_native_committed";break;
      case Phase::DrainBeforeReport:
        if(!owner_.Drain(*batch_))return;phase_=Phase::Report;break;
      case Phase::Report:
        if(completion_.outcome==StateApplyOutcome::Success&&(cancelled_||batch_->cancelled.load(std::memory_order_acquire)||!owner_.Current(*batch_))) {Fail("binary_publish_revoked_before_report");break;}
        if(!results_.Complete(completion_))return;
        if(completion_.outcome==StateApplyOutcome::FailedBeforeWrites){batch_.reset();return;}
        phase_=Phase::AwaitDecision;break;
      case Phase::AwaitDecision: {
        const auto resolution=batch_->resolution.load(std::memory_order_acquire);
        if(resolution==ClientStateApplyResolution::Pending)return;
        if(resolution==ClientStateApplyResolution::Committed){batch_.reset();return;}
        owner_.Invalidate(*batch_,"binary_completion_revoked");phase_=Phase::Drain;break;
      }
      case Phase::Drain:
        if(!owner_.Drain(*batch_))return;
        completion_.drained=true;phase_=Phase::ReportDrain;break;
      case Phase::ReportDrain:
        if(!results_.Complete(completion_))return;
        batch_.reset();return;
    }
    // 任何子调用/回调都可能改变生命周期；下一次写入/事件之前重新核本地资格。
    if(batch_&&phase_<Phase::DrainBeforeReport&&(cancelled_||batch_->cancelled.load(std::memory_order_acquire)||!owner_.Current(*batch_)))Fail("binary_native_lifecycle_during_apply");
  }
}
} // namespace cs::state
