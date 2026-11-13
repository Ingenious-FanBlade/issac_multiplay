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
void Metrics::Bind(WorldScope world,std::uint64_t epoch) noexcept {if(scope!=world||route!=epoch){scope=world;route=epoch;first_prepare_ms=first_receive_ms=display_ready_ms=0;}}
std::string Metrics::Fields(const std::string &prefix,unsigned page) const {
  std::string out;const auto add=[&](const std::string &key,std::uint64_t value){out+=prefix+key+"="+std::to_string(value)+"\n";};
  add("schema",1);add("session",scope.session);add("run",scope.run);add("room",scope.room);add("scene",scope.scene);add("route",route);add("missing",missing);
  add("packets",packets);add("udp_payload_bytes",payload_bytes);add("wire_bytes",wire_bytes);add("bundle_messages",bundle_messages);for(unsigned i=0;i<4;++i){add("lane."+std::to_string(i)+".wire_bytes",lane_bytes[i]);add("lane."+std::to_string(i)+".packet_incidence",lane_packets[i]);add("lane."+std::to_string(i)+".encoded_bytes",lane_encoded[i]);add("lane."+std::to_string(i)+".compressed_bytes",lane_compressed[i]);}
  add("queue",queue_items);add("queue_high",queue_high);add("candidates",candidates);add("retained",retained);add("replacements",replacements);add("stale_rejections",stale_rejections);
  add("first_prepare_local_ms",first_prepare_ms);add("first_receive_local_ms",first_receive_ms);add("display_ready_local_ms",display_ready_ms);add("encoded_bytes",encoded_bytes);add("compressed_bytes",compressed_bytes);
  add("page",page);add("pages",duration.size());
  const char *cpu_names[]={"capture","encode","compress","queue","decode","native_prepare","native_apply","draw","network","ipc","native_retire","build","main_capture","frame_interval","source_lua","source_native","source_copy","source_control","source_handoff"};
  for(unsigned i=0;i<duration.size();++i){const auto &d=duration[i];const auto p="duration."+std::string(cpu_names[i])+".";add(p+"wall_count",d.count);add(p+"wall_total_us",d.total_us);add(p+"wall_max_us",d.max_us);const auto &c=cpu[i];add(p+"thread_cpu_count",c.count);add(p+"thread_cpu_total_us",c.total_us);add(p+"thread_cpu_max_us",c.max_us);for(unsigned j=0;j<32;++j)if(i==page%duration.size()&&d.buckets[j])add(p+"bucket_log2_us."+std::to_string(j),d.buckets[j]);}
  add("histogram_stage",page%duration.size());
  out+=prefix+"clock=local_monotonic_per_owner_no_cross_device_subtraction\n";
  out+=prefix+"missing_stages=zero_count_means_not_captured\n";return out;
}
} // namespace cs::replica::stream
