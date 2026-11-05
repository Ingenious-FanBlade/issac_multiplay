#include "world/common/live_consumer.h"
#include <stdexcept>
#include <cstring>

namespace cs::world::observation {
Consumer LiveConsumer::Bind(CapturePlan plan){plan_=plan;return {kLiveConsumer,this,Publish,std::move(plan),Invalidated};}
void LiveConsumer::Attached(bool active) noexcept {status_.attached=active;if(!active)Invalidate("live_detached");}
void LiveConsumer::Publish(void *owner,const ObservationEnvelope &e) noexcept {static_cast<LiveConsumer *>(owner)->Receive(e);}
void LiveConsumer::Invalidated(void *owner,const char *reason) noexcept {static_cast<LiveConsumer *>(owner)->Invalidate(reason);}
void LiveConsumer::Invalidate(const char *reason) noexcept {pending_.clear();latest_.reset();++status_.invalidations;std::strncpy(status_.reason,reason?reason:"live_invalidated",sizeof(status_.reason)-1);status_.reason[sizeof(status_.reason)-1]=0;}
std::shared_ptr<const ObservationEnvelope> LiveConsumer::Own(const ObservationEnvelope &e){
  std::uint64_t bytes=sizeof(ObservationEnvelope)+128+2*(e.payload.size()+e.schema.size()+e.result.range.size());
  for(const auto &r:e.result.reasons)bytes+=sizeof(std::string)+32+2*r.size();
  auto used=ledger_->bytes.load(std::memory_order_acquire);
  do {if(bytes>ledger_->limit||used>ledger_->limit-bytes)throw std::bad_alloc();}while(!ledger_->bytes.compare_exchange_weak(used,used+bytes,std::memory_order_acq_rel));
  const auto ledger=ledger_;
  ObservationEnvelope *copy=nullptr;
  try {copy=new ObservationEnvelope(e);}catch(...){ledger->bytes.fetch_sub(bytes,std::memory_order_acq_rel);throw;}
  // shared_ptr控制块分配失败会调用deleter，账本也恰好退还一次。
  return {copy,[ledger,bytes](const ObservationEnvelope *p){delete p;ledger->bytes.fetch_sub(bytes,std::memory_order_acq_rel);}};
}
void LiveConsumer::Receive(const ObservationEnvelope &e) noexcept {
  if(!status_.attached)return;
  if(e.kind!=RecordKind::State||e.context.phase!=Phase::OrdinaryPost){
    // 独立Render姿态有自己的scope，不把每次绘制当成普通世界变更或新普通样本。
    if(e.kind==RecordKind::State&&e.context.phase==Phase::Render)return;
    // 事件只是撤销缓存的事实，不能当成完整最终值；odd gap不撤销已经提交的普通后态。
    if(e.kind==RecordKind::Gap){++status_.gaps;for(const auto &reason:e.result.reasons)if(reason=="ODD_PLAYER_ONLY")return;}
    Invalidate(e.kind==RecordKind::Gap?"live_source_gap":"live_source_changed");return;
  }
  if(e.schema.find(".native-v1")!=std::string::npos||e.schema.find(".legacy")!=std::string::npos)return;
  if(e.result.coverage!=Coverage::Complete||e.context.mutation_before!=e.context.mutation_after||!e.context.capture_index||e.context.run.presence!=Presence::Known||e.context.room.presence!=Presence::Known){++status_.gaps;Invalidate("live_source_incomplete");return;}
  try {
    if(!pending_.empty()){
      const auto &c=pending_.begin()->second->context;
      if(c.capture_index!=e.context.capture_index||c.session!=e.context.session||c.epoch!=e.context.epoch||c.outer!=e.context.outer||c.owner!=e.context.owner||c.mutation_after!=e.context.mutation_after){++status_.gaps;Invalidate("live_mixed_capture");}
    }
    if(pending_.contains(e.domain)){++status_.gaps;Invalidate("live_duplicate_domain");return;}
    pending_.emplace(e.domain,Own(e));
  }catch(...){++status_.gaps;Invalidate("live_value_budget");}
}
void LiveConsumer::Finish(bool committed,bool stable,std::uint64_t loss) noexcept {
  if(!status_.attached)return;
  if(!committed||!stable){Invalidate("live_uncommitted_or_mutated");return;}
  if(pending_.empty())return;
  try {
    const auto &c=pending_.begin()->second->context;
    // 缺任意当次要求域时整批不发布；未调度、Partial及真实空集合各自保留语义。
    for(const auto &d:plan_.domains)if((d.phases&1u)&&c.capture_index%d.interval==0&&!pending_.contains(d.domain)){++status_.gaps;Invalidate("live_required_domain_missing");return;}
    auto batch=std::make_shared<LiveBatch>();batch->session=c.session;batch->epoch=c.epoch;batch->capture=c.capture_index;batch->tick=c.authority_tick.value;batch->outer=c.outer;batch->owner=c.owner;batch->mutation=c.mutation_after;batch->loss=loss;batch->invalidation=status_.invalidations;batch->plan_revision=c.plan_revision;
    for(auto &[domain,e]:pending_){if(e->context.authority_tick.value!=batch->tick||e->context.plan_revision!=batch->plan_revision)throw std::runtime_error("live_context_mismatch");batch->domains.push_back(std::move(e));}
    pending_.clear();latest_=std::move(batch);++status_.delivered;std::strcpy(status_.reason,"live_committed_values");
  }catch(...){++status_.gaps;Invalidate("live_batch_budget_or_context");}
}
LiveStatus LiveConsumer::Status() const noexcept {auto s=status_;s.bytes=ledger_->bytes.load(std::memory_order_acquire);return s;}
} // namespace cs::world::observation
