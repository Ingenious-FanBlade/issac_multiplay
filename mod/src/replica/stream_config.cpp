#include "replica/stream_config.h"
#include "state/budget.h"
#include <cstring>
namespace cs::replica::stream {
// 一次性检查数值及组合约束，返回具体首因；不降档或修改配置。
const char *RuntimeConfig::Validate() const noexcept {
#define CS_STREAM_CHECK(name,value,min,max) if(name<min||name>max)return "stream_config_" #name;
  CS_STREAM_SETTINGS(CS_STREAM_CHECK)
#undef CS_STREAM_CHECK
  if(60%player_hz||30%world_hz||60%timing_hz)return "stream_config_frequency_divisor";
  if(control_bytes_per_second+player_bytes_per_second+world_bytes_per_second+reliable_bytes_per_second>peer_bytes_per_second)return "stream_config_lane_sum";
  if(peer_bytes_per_second>total_bytes_per_second/3)return "stream_config_peer_sum";
  if(total_burst_bytes<3*peer_burst_bytes||control_burst_bytes>peer_burst_bytes||player_burst_bytes>peer_burst_bytes||world_burst_bytes>peer_burst_bytes||reliable_burst_bytes>peer_burst_bytes)return "stream_config_burst";
  if(heartbeat_ms>=peer_timeout_ms||rto_min_ms>rto_initial_ms||rto_initial_ms>rto_max_ms||rto_max_ms>=progress_timeout_ms)return "stream_config_timing";
  if(1000/world_hz>=stale_ms)return "stream_config_world_frequency_vs_stale";
  if(final_retry_ms>=stale_ms||stale_ms>=recovery_ms||progress_timeout_ms>absolute_timeout_ms)return "stream_config_recovery";
  if(fragment_payload_bytes+64!=datagram_bytes||fragments*fragment_payload_bytes<world_message_bytes||fast_message_bytes>world_message_bytes||fast_nodes>world_nodes)return "stream_config_fragment_capacity";
  const auto &budget=state::StateBudgets();
  if(acceptance_profile&&(peer_bytes_per_second!=budget.peer_bytes_per_second||total_bytes_per_second!=budget.host_bytes_per_second||control_bytes_per_second!=budget.control_bytes_per_second||player_bytes_per_second!=budget.body_bytes_per_second||world_bytes_per_second!=budget.world_bytes_per_second||reliable_bytes_per_second!=budget.reliable_bytes_per_second))return "stream_config_acceptance_budget";
  return "";
}
std::string RuntimeConfig::Fields(const std::string &prefix) const {
  std::string out;
#define CS_STREAM_PRINT(name,value,min,max) out+=prefix+#name+"="+std::to_string(name)+"\n";
  CS_STREAM_SETTINGS(CS_STREAM_PRINT)
#undef CS_STREAM_PRINT
  out+=prefix+"profile="+(acceptance_profile?"acceptance-fixed-peer":"product-fixed-peer")+"\n";
  out+=prefix+"borrow_initial_lanes="+(borrow_initial_lanes?std::string("1"):std::string("0"))+"\n";
  out+=prefix+"reserved_peer_slots=3\n";
  out+=prefix+"cross_peer_borrow=0\n"+prefix+"frozen=1\n";
  return out;
}
// 未解释owner优先停止，已知写后前缀只能先退休；写前暂缺可保留A等待。
FailurePolicy ClassifyFailure(const char *reason,bool written,bool unsafe) noexcept {
  // 缺成员的诊断后缀只提供定位，不让字段名里的resource/context等词改变稳定首因分类。
  constexpr char missing[]="stream_materialize_missing:";
  const auto stable=reason&&!std::strncmp(reason,missing,sizeof(missing)-1)?"stream_materialize_missing":reason;
  const auto has=[&](const char *s){return stable&&std::strstr(stable,s);};
  if(unsafe||has("owner_unknown")||has("unexplained")||has("exception")||has("thread")||has("context")||has("retire")||has("unbalanced"))return {FailureClass::Owner,"owner_uncertain","场景无法安全退休",false,false};
  if(has("recovery")||has("retry_failed"))return {FailureClass::Recovery,"recovery_failed","场景恢复失败",false,false};
  if(has("capability")||has("unsupported")||has("schema")||has("conflict"))return {FailureClass::Capability,"capability_or_contract","版本或能力不兼容",false,false};
  if(written)return {FailureClass::Native,"native_partial","场景应用失败，正在恢复",true,false};
  if(has("budget")||has("capacity")||has("pool")||has("allocation"))return {FailureClass::Budget,"budget_exhausted","复制预算不足",true,true};
  if(has("resource")||has("anm")||has("image"))return {FailureClass::Resource,"resource_error","场景资源不可用",true,false};
  if(has("dependency")||has("missing"))return {FailureClass::Dependency,"dependency_missing","等待场景依赖",true,true};
  if(has("stale")||has("converge")||has("progress")||has("deadline"))return {FailureClass::Stale,"state_stale","状态陈旧，正在恢复",true,false};
  return {FailureClass::Prewrite,"prewrite_rejected","状态应用前被拒绝",true,false};
}
} // namespace cs::replica::stream
