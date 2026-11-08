#include "replica/stream_metrics.h"
#include "replica/stream_e2e.h"
#include <algorithm>
#include <cstring>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
namespace cs::replica::stream {
std::uint64_t ThreadCpuMicros() noexcept {FILETIME creation{},exit{},kernel{},user{};if(!GetThreadTimes(GetCurrentThread(),&creation,&exit,&kernel,&user))return UINT64_MAX;ULARGE_INTEGER k{},u{};k.LowPart=kernel.dwLowDateTime;k.HighPart=kernel.dwHighDateTime;u.LowPart=user.dwLowDateTime;u.HighPart=user.dwHighDateTime;return (k.QuadPart+u.QuadPart)/10;}
std::string LimitsFields(const Limits &l,const std::string &p){std::string out;const auto add=[&](const char *key,std::uint64_t value){out+=p+key+"="+std::to_string(value)+"\n";};add("bytes",l.bytes);add("nodes",l.nodes);add("depth",l.depth);add("string_bytes",l.string_bytes);add("datagram",l.datagram);add("candidates",l.candidates);add("fragments",l.fragments);add("value_bytes",l.value_bytes);out+=p+"limit_source=minimum_of_local_config_and_negotiated_capabilities\n";return out;}
void Duration::Observe(std::uint64_t us) noexcept {++count;total_us+=us;max_us=(std::max)(max_us,us);unsigned b=0;for(auto n=us;n>1&&b<31;n>>=1)++b;++buckets[b];}
void Metrics::Bind(WorldScope world,std::uint64_t epoch) noexcept {if(scope!=world||route!=epoch){scope=world;route=epoch;flows={};first_prepare_ms=first_receive_ms=display_ready_ms=0;rendered_local_ms=rendered_revision=0;}}
// 新序号才增加范围阶段次数；重发与重复确认只影响物理网络计数。
bool Metrics::Note(const RangeId &id,std::uint64_t revision,const Sample &sample,FlowStage stage,std::uint64_t now) noexcept {
  if(id.name.size()>=128){++missing;return false;}
  Flow *slot=nullptr;
  for(auto &f:flows){if(!f.used){if(!slot)slot=&f;continue;}if(f.kind==unsigned(id.kind)&&f.body==id.body&&id.name==f.name.data()){slot=&f;break;}}
  if(!slot){++missing;return false;}
  if(!slot->used){slot->used=true;slot->kind=unsigned(id.kind);slot->body=id.body;std::memcpy(slot->name.data(),id.name.data(),id.name.size());}
  const auto s=unsigned(stage);if(revision<=slot->sequence[s])return false;
  ++slot->counts[s];++stage_totals[s];slot->sequence[s]=revision;slot->tick[s]=sample.tick;slot->phase[s]=unsigned(id.kind==RangeKind::PlayerFast?world::observation::Phase::PlayerPost:world::observation::Phase::OrdinaryPost);slot->authority_kind[s]=unsigned(sample.kind);slot->at_ms[s]=now;
#ifdef ISAACCS_E2E
  E2EFlow(this,id,revision,sample,stage);
#endif
  return true;
}
std::string Metrics::Fields(const std::string &prefix,unsigned page) const {
  std::string out;const auto add=[&](const std::string &key,std::uint64_t value){out+=prefix+key+"="+std::to_string(value)+"\n";};
  add("schema",1);add("session",scope.session);add("run",scope.run);add("room",scope.room);add("scene",scope.scene);add("route",route);add("missing",missing);
  add("packets",packets);add("udp_payload_bytes",payload_bytes);add("wire_bytes",wire_bytes);add("bundle_messages",bundle_messages);for(unsigned i=0;i<4;++i){add("lane."+std::to_string(i)+".wire_bytes",lane_bytes[i]);add("lane."+std::to_string(i)+".packet_incidence",lane_packets[i]);add("lane."+std::to_string(i)+".encoded_bytes",lane_encoded[i]);add("lane."+std::to_string(i)+".compressed_bytes",lane_compressed[i]);}
  add("queue",queue_items);add("queue_high",queue_high);add("candidates",candidates);add("retained",retained);add("replacements",replacements);add("stale_rejections",stale_rejections);
  add("first_prepare_local_ms",first_prepare_ms);add("first_receive_local_ms",first_receive_ms);add("display_ready_local_ms",display_ready_ms);add("encoded_bytes",encoded_bytes);add("compressed_bytes",compressed_bytes);
  const char *stages[]={"witness","sample","publish","encode","send","receive","commit","ack"};
  add("repeated_range_transfers",repeated_range_transfers);
  add("rendered_local_ms",rendered_local_ms);add("rendered_revision",rendered_revision);
  for(unsigned i=0;i<unsigned(FlowStage::Count);++i)add("total."+std::string(stages[i]),stage_totals[i]);
  add("page",page);add("pages",32);
  for(unsigned i=(std::min)(page,32u)*4;i<(std::min)(page*4+4,128u);++i){const auto &f=flows[i];if(!f.used)continue;const auto p="range."+std::to_string(i)+".";out+=prefix+p+"name="+f.name.data()+"\n";add(p+"kind",f.kind);add(p+"body_run",f.body.run);add(p+"body_id",f.body.id);add(p+"body_space",f.body.space);
    for(unsigned j=0;j<unsigned(FlowStage::Count);++j){const auto q=p+stages[j];add(q+".count",f.counts[j]);add(q+".sequence",f.sequence[j]);add(q+".tick",f.tick[j]);add(q+".phase",f.phase[j]);add(q+".authority_kind",f.authority_kind[j]);add(q+".local_ms",f.at_ms[j]);}}
  const char *cpu_names[]={"capture","encode","compress","queue","decode","native_prepare","native_apply","draw","network","ipc","native_retire","build","main_capture","frame_interval","source_lua","source_native","source_copy","source_control","source_handoff"};
  for(unsigned i=0;i<duration.size();++i){const auto &d=duration[i];const auto p="duration."+std::string(cpu_names[i])+".";add(p+"wall_count",d.count);add(p+"wall_total_us",d.total_us);add(p+"wall_max_us",d.max_us);const auto &c=cpu[i];add(p+"thread_cpu_count",c.count);add(p+"thread_cpu_total_us",c.total_us);add(p+"thread_cpu_max_us",c.max_us);for(unsigned j=0;j<32;++j)if(i==page%duration.size()&&d.buckets[j])add(p+"bucket_log2_us."+std::to_string(j),d.buckets[j]);}
  add("histogram_stage",page%duration.size());
  out+=prefix+"clock=local_monotonic_per_owner_no_cross_device_subtraction\n";
  out+=prefix+"missing_stages=zero_count_means_not_captured\n";return out;
}
} // namespace cs::replica::stream
