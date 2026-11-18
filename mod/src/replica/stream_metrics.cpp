#include "replica/stream_metrics.h"
#include "replica/stream_e2e.h"
#include <algorithm>
#include <cstring>
#include <iterator>
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
void Duration::Observe(std::uint64_t us) noexcept {++count;total_us+=us;max_us=(std::max)(max_us,us);over_2000_us+=us>2000;over_4000_us+=us>4000;unsigned b=0;for(auto n=us;n>1&&b<31;n>>=1)++b;++buckets[b];}
// 以桶上界报告保守近似值，精确最大值用于收窄最后一个桶。
std::uint64_t Duration::Percentile(unsigned percent) const noexcept {
  const auto rank=(count/100)*percent+((count%100)*percent+99)/100;
  std::uint64_t seen=0;
  for(unsigned i=0;i<buckets.size();++i){seen+=buckets[i];if(seen>=rank)return i==31?max_us:std::min(max_us,(std::uint64_t{2}<<i)-1);}
  return max_us;
}
void Metrics::Bind(WorldScope world,std::uint64_t epoch) noexcept {if(scope!=world||route!=epoch){scope=world;route=epoch;first_prepare_ms=first_receive_ms=display_ready_ms=0;}}
const char *CpuStageName(CpuStage stage) noexcept {
  constexpr const char *names[]={"capture","encode","compress","queue","decode","native_prepare","native_apply","draw","network","ipc","native_retire","build","main_capture","frame_interval","source_lua","source_native","source_copy","source_control","source_handoff",
    "scene_prepare","topology_resources","entity_values","scene_values","fast_body","source_review","source_commit","ordinary_outer","body_outer","capture_index","capture_queue_age",
    "topology_common","topology_groups","topology_lease","anchor_install","field_execution","joint_prepare","body_clone",
    "ordinary_published_outer","ordinary_failed_outer","body_published_outer","body_failed_outer","body_mixed_outer","body_joint_only_outer","capture_idle_outer"};
  static_assert(std::size(names)==unsigned(CpuStage::Count));
  return names[unsigned(stage)];
}
std::string Metrics::Fields(const std::string &prefix,unsigned page) const {
  std::string out;const auto add=[&](const std::string &key,std::uint64_t value){out+=prefix+key+"="+std::to_string(value)+"\n";};
  add("schema",1);add("session",scope.session);add("run",scope.run);add("room",scope.room);add("scene",scope.scene);add("route",route);add("missing",missing);
  add("packets",packets);add("udp_payload_bytes",payload_bytes);add("wire_bytes",wire_bytes);add("bundle_messages",bundle_messages);for(unsigned i=0;i<4;++i){add("lane."+std::to_string(i)+".wire_bytes",lane_bytes[i]);add("lane."+std::to_string(i)+".packet_incidence",lane_packets[i]);add("lane."+std::to_string(i)+".encoded_bytes",lane_encoded[i]);add("lane."+std::to_string(i)+".compressed_bytes",lane_compressed[i]);}
  add("queue",queue_items);add("queue_high",queue_high);add("candidates",candidates);add("retained",retained);add("replacements",replacements);add("stale_rejections",stale_rejections);
  add("first_prepare_local_ms",first_prepare_ms);add("first_receive_local_ms",first_receive_ms);add("display_ready_local_ms",display_ready_ms);add("encoded_bytes",encoded_bytes);add("compressed_bytes",compressed_bytes);
  add("page",page);add("pages",duration.size());
  for(unsigned i=0;i<duration.size();++i){const auto &d=duration[i];const auto p="duration."+std::string(CpuStageName(CpuStage(i)))+".";add(p+"wall_count",d.count);add(p+"wall_total_us",d.total_us);add(p+"wall_max_us",d.max_us);const auto &c=cpu[i];add(p+"thread_cpu_count",c.count);add(p+"thread_cpu_total_us",c.total_us);add(p+"thread_cpu_max_us",c.max_us);
    if(i==page%duration.size()){
      add(p+"over_2000_us",d.over_2000_us);add(p+"over_4000_us",d.over_4000_us);
      for(unsigned j=0;j<d.buckets.size();++j){
      if(d.buckets[j])add(p+"bucket_log2_us."+std::to_string(j),d.buckets[j]);
      if(c.buckets[j])add(p+"thread_cpu_bucket_log2_us."+std::to_string(j),c.buckets[j]);
      }
    }
  }
  add("histogram_stage",page%duration.size());
  out+=prefix+"clock=local_monotonic_per_owner_no_cross_device_subtraction\n";
  out+=prefix+"missing_stages=zero_count_means_not_captured\n";return out;
}
} // namespace cs::replica::stream
