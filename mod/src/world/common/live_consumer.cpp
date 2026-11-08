#include "world/common/live_consumer.h"
#include <stdexcept>
#include <cstring>
#include <cstdio>
#include "core/deferred_values.h"

namespace cs::world::observation {
Consumer LiveConsumer::Bind(CapturePlan plan){plan_=plan;return {kLiveConsumer,this,Publish,std::move(plan),Invalidated};}
void LiveConsumer::Attached(bool active) noexcept {status_.attached=active;if(!active)Invalidate("live_detached");}
void LiveConsumer::Publish(void *owner,const ObservationEnvelope &e) noexcept {static_cast<LiveConsumer *>(owner)->Receive(e);}
void LiveConsumer::Invalidated(void *owner,const char *reason) noexcept {static_cast<LiveConsumer *>(owner)->Invalidate(reason);}
void LiveConsumer::Invalidate(const char *reason) noexcept {pending_.clear();latest_.reset();++status_.invalidations;std::strncpy(status_.reason,reason?reason:"live_invalidated",sizeof(status_.reason)-1);status_.reason[sizeof(status_.reason)-1]=0;}
std::shared_ptr<const ObservationEnvelope> LiveConsumer::Own(const ObservationEnvelope &e){
  const auto payload_bytes=e.owned_source?0:e.payload.size()+e.stream_payload.size();
  std::uint64_t bytes=sizeof(ObservationEnvelope)+256+2*(payload_bytes+e.schema.size()+e.result.range.size());
  for(const auto &r:e.result.reasons)bytes+=sizeof(std::string)+32+2*r.size();
  auto lease=memory::Reserve(memory::Pool::Values,bytes,"live_envelope");
  auto used=ledger_->bytes.load(std::memory_order_acquire);
  do {if(bytes>ledger_->limit||used>ledger_->limit-bytes)throw std::bad_alloc();}while(!ledger_->bytes.compare_exchange_weak(used,used+bytes,std::memory_order_acq_rel));
  const auto ledger=ledger_;
  ObservationEnvelope *copy=nullptr;
  std::uint64_t retained=0;
  try {
    copy=new ObservationEnvelope();copy->context=e.context;copy->domain=e.domain;copy->kind=e.kind;copy->schema=e.schema;copy->version=e.version;copy->result=e.result;
    copy->owned_source=e.owned_source;copy->owned_bytes=e.owned_bytes;
    if(!e.owned_source){copy->payload=e.payload;copy->stream_payload=e.stream_payload;}
    // 两倍长度只保护复制峰值；不可变封套长期按实际字符串/容器容量持有，不永久占两份预算。
    // 内联字符串的存储已包含于Envelope或reasons元素本身，只另计真正的堆容量和终止符。
    const auto inline_capacity=std::string{}.capacity();
    const auto heap=[&](const std::string &s)->std::uint64_t{return s.capacity()>inline_capacity?s.capacity()+1:0;};
    retained=sizeof(ObservationEnvelope)+128+heap(copy->payload)+heap(copy->stream_payload)+heap(copy->schema)+heap(copy->result.range)+copy->result.reasons.capacity()*sizeof(std::string);
    for(const auto &reason:copy->result.reasons)retained+=heap(reason);
    if(retained>bytes)throw std::runtime_error("live_value_footprint");
    lease->Resize(retained);
  }catch(...){delete copy;ledger->bytes.fetch_sub(bytes,std::memory_order_acq_rel);throw;}
  ledger->bytes.fetch_sub(bytes-retained,std::memory_order_acq_rel);
  // 来源树本身使用有界延后回收；小封套退还的只是自身元数据租约。
  return {copy,[ledger,retained,lease=std::move(lease)](const ObservationEnvelope *p){delete p;ledger->bytes.fetch_sub(retained,std::memory_order_acq_rel);}};
}
void LiveConsumer::Receive(const ObservationEnvelope &e) noexcept {
  if(!status_.attached)return;
  // PlayerPost由正式身体owner独立收束，不撤销普通整房样本，也不从子帧生成删除。
  if(e.context.phase==Phase::PlayerPost)return;
  if(e.domain==DomainId::Random&&e.context.phase==Phase::ActualCall)return;
  // 诊断的逐Grid正常更新不代表身份/资源失效，当前值仍由完整ordinary独立取得。
  if(e.domain==DomainId::Grid&&e.kind==RecordKind::State&&e.schema=="isaac.cs.grid.update")return;
  if(e.kind!=RecordKind::State||e.context.phase!=Phase::OrdinaryPost){
    // 独立Render姿态有自己的scope，不把每次绘制当成普通世界变更或新普通样本。
    if(e.kind==RecordKind::State&&e.context.phase==Phase::Render)return;
    // 事件只是撤销缓存的事实，不能当成完整最终值；odd gap不撤销已经提交的普通后态。
    if(e.kind==RecordKind::Gap){++status_.gaps;for(const auto &reason:e.result.reasons)if(reason=="ODD_PLAYER_ONLY")return;}
    // 保留真实失效来源，避免后台构建首因被后续笼统的changed覆盖后无法定位。
    char reason[128];std::snprintf(reason,sizeof(reason),"%s:%u:%u:%.80s",e.kind==RecordKind::Gap?"live_source_gap":"live_source_changed",unsigned(e.kind),unsigned(e.context.phase),e.schema.c_str());
    Invalidate(reason);return;
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
void LiveConsumer::Finish(bool committed,bool stable,std::uint64_t loss,std::uint64_t watermark) noexcept {
  if(!status_.attached)return;
  if(!committed||!stable){Invalidate("live_uncommitted_or_mutated");return;}
  if(pending_.empty())return;
  try {
    const auto &c=pending_.begin()->second->context;
    // 缺任意当次要求域时整批不发布；未调度、Partial及真实空集合各自保留语义。
    for(const auto &d:plan_.domains)if((d.phases&1u)&&c.capture_index%d.interval==0&&!pending_.contains(d.domain)){++status_.gaps;Invalidate("live_required_domain_missing");return;}
    // 批次元数据与域引用同样预付；被合并/停止时只移交到有界回收线程。
    auto batch_memory=memory::Reserve(memory::Pool::Values,sizeof(LiveBatch)+128+pending_.size()*sizeof(std::shared_ptr<const ObservationEnvelope>),"live_batch");
    auto batch=memory::DeferredValue<LiveBatch>();batch->memory=std::move(batch_memory);batch->domains.reserve(pending_.size());
    batch->memory->Resize(sizeof(LiveBatch)+128+batch->domains.capacity()*sizeof(std::shared_ptr<const ObservationEnvelope>));
    batch->session=c.session;batch->epoch=c.epoch;batch->capture=c.capture_index;batch->tick=c.authority_tick.value;batch->outer=c.outer;batch->owner=c.owner;batch->mutation=c.mutation_after;batch->loss=loss;batch->invalidation=status_.invalidations;batch->plan_revision=c.plan_revision;
    batch->event_watermark=watermark;
    for(auto &[domain,e]:pending_){if(e->context.authority_tick.value!=batch->tick||e->context.plan_revision!=batch->plan_revision)throw std::runtime_error("live_context_mismatch");batch->domains.push_back(std::move(e));}
    pending_.clear();latest_=std::move(batch);++status_.delivered;std::strcpy(status_.reason,"live_committed_values");
  }catch(...){++status_.gaps;Invalidate("live_batch_budget_or_context");}
}
LiveStatus LiveConsumer::Status() const noexcept {auto s=status_;s.bytes=ledger_->bytes.load(std::memory_order_acquire);return s;}
} // namespace cs::world::observation
