#include "state/client_apply.h"
#include <algorithm>
#include <cstring>

namespace cs::state {
ClientStateApplyCoordinator::ClientStateApplyCoordinator(ClientStateNativeOwner &owner,ClientStateApplyResults &results)
  :owner_(owner),results_(results) {SetReason("binary_idle");}
bool ClientStateApplyCoordinator::Offer(ClientStateApplyBatchRef batch) noexcept {
  if(thread_!=std::this_thread::get_id()||batch_||!batch||!batch->native||!batch->target.candidate||!batch->ticket.serial||batch->target.events.size()>512)return false;
  batch_=std::move(batch);completion_={};completion_.ticket=batch_->ticket;written_=cancelled_=false;event_=0;phase_=Phase::Prepare;SetReason("binary_preparing");return true;
}
void ClientStateApplyCoordinator::Cancel() noexcept {if(thread_==std::this_thread::get_id())cancelled_=true;}
void ClientStateApplyCoordinator::SetReason(const char *reason) noexcept {
  if(!reason)reason="binary_native_reason_missing";
  const auto size=(std::min)(std::strlen(reason),reason_.size()-1);
  std::memmove(reason_.data(),reason,size);reason_[size]=0;
}
void ClientStateApplyCoordinator::Fail(const char *reason) noexcept {
  SetReason(reason);completion_.applied=false;
  // 正式 owner 发现部分写入时撤整个原生场景，共享 Player 根使两条流都失去差分基线。
  if(written_)owner_.Invalidate(*batch_,Reason());
  phase_=written_?Phase::Report:Phase::DrainBeforeReport;
}
bool ClientStateApplyCoordinator::Step(ClientStateNativeStep step,Phase next) noexcept {
  written_|=step.written;
  if(step.failed||(!step.ready&&written_)){
    Fail(step.reason);return false;
  }
  if(!step.ready){SetReason(step.reason);return false;}
  phase_=next;return true;
}
void ClientStateApplyCoordinator::Pump(bool allow_apply) noexcept {
  if(thread_!=std::this_thread::get_id()||!batch_)return;
  // 完成和排空反馈均携带已发生的事件回放前缀。
  const auto cancelled=[&]{return cancelled_||batch_->Withdrawn()||batch_->cancelled.load(std::memory_order_acquire)||!owner_.Current(*batch_);};
  if(phase_<Phase::DrainBeforeReport&&cancelled())Fail("binary_native_cancelled_or_generation_changed");
  // 一个Manager边界内完成整个写入/显示/回放/发布区间；准备允许跨帧等待但不暴露候选。
  for(unsigned transitions=0;batch_&&transitions<536;++transitions) {
    switch(phase_) {
      case Phase::Prepare:
        if(!allow_apply||!Step(owner_.Prepare(*batch_),Phase::Stage))return;
        break;
      case Phase::Stage:
        if(!allow_apply)return;
        if(!Step(owner_.Stage(*batch_),Phase::Install)){
          if(phase_==Phase::Stage)phase_=Phase::Prepare;
          return;
        }
        break;
      case Phase::Install:
        if(!allow_apply)return;
        if(!Step(owner_.Install(*batch_),Phase::Display)){
          if(phase_==Phase::Install)Fail("binary_live_install_interrupted");
          return;
        }
        break;
      case Phase::Display:
        if(!Step(owner_.Display(*batch_),Phase::Replay)){
          if(phase_==Phase::Display)Fail("binary_live_display_interrupted");
          return;
        }
        break;
      case Phase::Replay:
        if(event_==batch_->target.events.size()){phase_=Phase::Publish;break;}
        {const auto replay=owner_.Replay(*batch_,event_);
          if(replay.failed&&replay.written)completion_.uncertain_event=batch_->target.events[event_].value.sequence;
          if(!Step(replay,Phase::Replay)){
            if(phase_==Phase::Replay)Fail("binary_live_replay_interrupted");
            return;
          }}
        completion_.replayed[completion_.replayed_count++]=batch_->target.events[event_++].value.sequence;break;
      case Phase::Publish:
        if(!Step(owner_.Publish(*batch_),Phase::Report)){
          if(phase_==Phase::Publish)Fail("binary_live_publish_interrupted");
          return;
        }
        // 引用已公开。没有标量变化的批次被撤销时也须走已发布排空路径。
        written_=true;
        completion_.applied=true;SetReason("binary_native_committed");break;
      case Phase::DrainBeforeReport:
        if(!owner_.Drain(*batch_))return;completion_.drained=true;phase_=Phase::Report;break;
      case Phase::Report:
        if(completion_.applied&&cancelled()) {Fail("binary_publish_revoked_before_report");break;}
        completion_.reported_us=ClientApplyMetrics::Now();
        if(!results_.Complete(completion_))return;
        if(completion_.drained){batch_.reset();return;}
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
    if(batch_&&phase_<Phase::DrainBeforeReport&&cancelled())Fail("binary_native_lifecycle_during_apply");
  }
}
} // namespace cs::state
