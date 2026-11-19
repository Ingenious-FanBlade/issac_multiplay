#include "core/capture_cost.h"
#include "replica/stream_metrics.h"
#include "replica/stream_e2e.h"
#include <iterator>

namespace cs::capture {
namespace {
using namespace replica::stream;
// 奇数周期覆盖普通帧与仅 Body 的帧。采中的整棵调用树使用同一口径扣除子段。
constexpr std::uint64_t kCpuSampleFrames=31;
bool IsDetail(CostPart part) noexcept {return part>=CostPart::TopologyCommon;}
// 细分时段排除自己的子时段。旧统计只排除原有子阶段，保留可比口径。
struct Elapsed {
  std::uint64_t wall=0,cpu=0;
  bool cpu_known=true;
  void Add(const Elapsed &value) noexcept {wall+=value.wall;cpu+=value.cpu;cpu_known&=value.cpu_known;}
  std::uint64_t Cpu() const noexcept {return cpu_known?cpu:UINT64_MAX;}
};
// 数据只由Manager主线程操作；后台构建成本由自己的Metrics单列。
struct Costs final {
  struct Span {CostPart part;std::uint64_t wall,cpu;Elapsed children,legacy_children;};
  Metrics metrics;bool active=false,sample_cpu=false;unsigned depth=0;
  std::uint64_t cpu_sampled_frames=0;
  std::uint64_t outer=0,previous=0;
  Elapsed total;
  std::array<Span,64> stack{};
  std::array<Elapsed,unsigned(CostPart::Count)> parts{};
  std::array<bool,unsigned(CostPart::Count)> sampled{};
  std::array<std::uint64_t,4> outcomes{};
  std::array<std::uint64_t,unsigned(BodyReuse::Count)> reuse{};
  struct Clone {std::uint64_t count=0,cells=0,paths=0,bytes=0,references=0;};
  std::array<Clone,2> clones{};
  std::uint64_t joint_only=0;
  std::uint64_t group_checks=0,group_reuses=0,bound_writes=0,checked_writes=0;
  std::array<TopologyCounts,2> topology{};
  std::uint64_t anchor_changes=0;
  // 按 outer 存在的发布／失败／仅联合结果分类，Body 多候选可为混合结果。
  bool ordinary_published=false,ordinary_failed=false,body_published=false,body_failed=false,body_joint=false;
} costs;
// 来源Hook也可能报告错误线程；只有FrameBegin所在的主线程才可写此帧的聚合器。
thread_local bool capture_thread=false;
void Observe(CpuStage stage,std::uint64_t wall,std::uint64_t cpu) noexcept {
  costs.metrics.duration[unsigned(stage)].Observe(wall);
  if(cpu!=UINT64_MAX)costs.metrics.cpu[unsigned(stage)].Observe(cpu);
  else if(costs.sample_cpu&&stage!=CpuStage::FrameInterval)++costs.metrics.missing;
#ifdef ISAACCS_E2E
  E2ETime(&costs.metrics,stage,wall,cpu);
#endif
}
}
void FrameBegin(std::uint64_t outer,bool active) noexcept {
  capture_thread=true;
  const auto now=MetricMicros();
  FrameEnd();
  if(active&&costs.previous&&outer>costs.outer)Observe(CpuStage::FrameInterval,now-costs.previous,UINT64_MAX);
  if(costs.depth){++costs.metrics.missing;costs.depth=0;}
  costs.active=active;costs.outer=outer;costs.previous=active?now:0;costs.total={};
  costs.sample_cpu=active&&outer%kCpuSampleFrames==0;
  if(costs.sample_cpu)++costs.cpu_sampled_frames;
  costs.parts={};costs.sampled={};
  costs.ordinary_published=costs.ordinary_failed=costs.body_published=costs.body_failed=costs.body_joint=false;
}
void Candidate(bool ordinary,CostOutcome outcome) noexcept {
  if(!capture_thread||!costs.active)return;
  if(outcome==CostOutcome::JointOnly){++costs.joint_only;costs.body_joint=true;return;}
  const bool published=outcome==CostOutcome::Published;
  ++costs.outcomes[(ordinary?0u:2u)+(published?0u:1u)];
  if(ordinary)(published?costs.ordinary_published:costs.ordinary_failed)=true;
  else (published?costs.body_published:costs.body_failed)=true;
}
void CaptureExecutionCounts(const ExecutionCounts &value) noexcept {
  if(!capture_thread||!costs.active)return;
  costs.group_checks+=value.group_checks;costs.group_reuses+=value.group_reuses;
  costs.bound_writes+=value.bound_writes;costs.checked_writes+=value.checked_writes;
}
void CountTopology(const TopologyCounts &sample) noexcept {
  if(!capture_thread||!costs.active)return;
  const bool native=costs.depth&&costs.stack[costs.depth-1].part==CostPart::Native;
  auto &total=costs.topology[native?0:1];
  total.batches+=sample.batches;total.groups+=sample.groups;
  total.common_reads+=sample.common_reads;total.roster_reads+=sample.roster_reads;
  total.device_reads+=sample.device_reads;total.region_queries+=sample.region_queries;
  total.region_hits+=sample.region_hits;total.region_fallbacks+=sample.region_fallbacks;
  total.fact_hits+=sample.fact_hits;total.lease_acquires+=sample.lease_acquires;
  total.hologram_calls+=sample.hologram_calls;total.read_failures+=sample.read_failures;
  total.revoked+=sample.revoked;
}
void CountAnchorChanges(unsigned count) noexcept {
  if(capture_thread&&costs.active)costs.anchor_changes+=count;
}
void CountBodyReuse(BodyReuse result) noexcept {
  if(capture_thread&&costs.active)++costs.reuse[unsigned(result)];
}
void CountBodyClone(bool success,const BodyCloneCounts &sample) noexcept {
  if(!capture_thread||!costs.active)return;
  auto &total=costs.clones[success?0:1];++total.count;
  total.cells+=sample.cells;total.paths+=sample.paths;total.bytes+=sample.bytes;total.references+=sample.references;
}
bool CostBegin(CostPart part) noexcept {
  if(!capture_thread||!costs.active)return false;
  // 原阶段的同类嵌套判断忽略新增细分，细分阶段仍按实际栈计算子成本。
  auto parent=costs.depth;
  if(!IsDetail(part))while(parent&&IsDetail(costs.stack[parent-1].part))--parent;
  if(parent&&costs.stack[parent-1].part==part)return false;
  if(costs.depth==costs.stack.size()){++costs.metrics.missing;return false;}
  costs.stack[costs.depth++]={part,MetricMicros(),costs.sample_cpu?ThreadCpuMicros():UINT64_MAX};return true;
}
void CostEnd() noexcept {
  if(!costs.depth)return;const auto end=MetricMicros(),cpu=costs.sample_cpu?ThreadCpuMicros():UINT64_MAX;const auto span=costs.stack[--costs.depth];const auto wall=end-span.wall;
  const bool known=cpu!=UINT64_MAX&&span.cpu!=UINT64_MAX&&cpu>=span.cpu;
  const Elapsed inclusive{wall,known?cpu-span.cpu:0,known};
  const bool detail=IsDetail(span.part);
  const auto &children=detail?span.children:span.legacy_children;
  const Elapsed exclusive{wall-children.wall,inclusive.cpu>=children.cpu?inclusive.cpu-children.cpu:0,known&&children.cpu_known};
  costs.sampled[unsigned(span.part)]=true;
  costs.parts[unsigned(span.part)].Add(exclusive);
  const auto &legacy=detail?span.legacy_children:inclusive;
  if(costs.depth){auto &parent=costs.stack[costs.depth-1];parent.children.Add(inclusive);parent.legacy_children.Add(legacy);}
  else costs.total.Add(legacy);
}
void FrameEnd() noexcept {
  if(!capture_thread||!costs.active||costs.depth)return;
  costs.active=false;
  const auto observe_outer=[](CpuStage stage){Observe(stage,costs.total.wall,costs.total.Cpu());};
  const bool ordinary=costs.ordinary_published||costs.ordinary_failed;
  const bool body=costs.body_published||costs.body_failed||costs.body_joint;
  observe_outer(ordinary||body?CpuStage::MainCapture:CpuStage::CaptureIdleOuter);
  if(ordinary){
    observe_outer(CpuStage::OrdinaryOuter);
    // 同帧若存在 ordinary 撤销或失败，纳入失败分母。
    observe_outer(costs.ordinary_failed?CpuStage::OrdinaryFailedOuter:CpuStage::OrdinaryPublishedOuter);
  }else if(body){
    observe_outer(CpuStage::BodyOuter);
    if(costs.body_failed)observe_outer(costs.body_published?CpuStage::BodyMixedOuter:CpuStage::BodyFailedOuter);
    else observe_outer(costs.body_published?CpuStage::BodyPublishedOuter:CpuStage::BodyJointOnlyOuter);
  }
  constexpr CpuStage stages[]={CpuStage::SourceNative,CpuStage::SourceLua,CpuStage::SourceCopy,CpuStage::SourceControl,CpuStage::SourceHandoff,
    CpuStage::ScenePrepare,CpuStage::TopologyResources,CpuStage::EntityValues,CpuStage::SceneValues,CpuStage::FastBody,CpuStage::SourceReview,CpuStage::SourceCommit,
    CpuStage::TopologyCommon,CpuStage::TopologyGroups,CpuStage::TopologyLease,CpuStage::AnchorInstall,CpuStage::FieldExecution,CpuStage::JointPrepare,CpuStage::BodyClone};
  static_assert(std::size(stages)==unsigned(CostPart::Count));
  for(unsigned i=0;i<std::size(stages);++i)if(costs.sampled[i])Observe(stages[i],costs.parts[i].wall,costs.parts[i].Cpu());
}
CaptureTiming ReadCaptureTiming() noexcept {
  if(!capture_thread)return {};
  const auto &wall=costs.metrics.duration[unsigned(CpuStage::MainCapture)];
  return {wall.count,wall.total_us};
}
std::string CostFields(unsigned page){
  auto out=costs.metrics.Fields("stream.main.",page);
  const auto add=[&](const std::string &key,std::uint64_t value){out+="stream.main."+key+"="+std::to_string(value)+"\n";};
  add("outer",costs.outer);
  add("cpu_sample_period_frames",kCpuSampleFrames);add("cpu_sampled_frames",costs.cpu_sampled_frames);
  add("ordinary_published",costs.outcomes[0]);add("ordinary_failed",costs.outcomes[1]);
  add("body_published",costs.outcomes[2]);add("body_failed",costs.outcomes[3]);
  add("body_joint_only",costs.joint_only);
  constexpr const char *reuse_names[]={"match","no_ordinary","no_shared_body","subject","window","too_large","stale","unqualified","no_lane"};
  static_assert(std::size(reuse_names)==unsigned(BodyReuse::Count));
  for(unsigned i=0;i<costs.reuse.size();++i)add(std::string("body_reuse.")+reuse_names[i],costs.reuse[i]);
  for(unsigned i=0;i<costs.clones.size();++i){
    const auto prefix=std::string(i==0?"body_clone.complete.":"body_clone.failed.");const auto &work=costs.clones[i];
    add(prefix+"count",work.count);add(prefix+"cells",work.cells);add(prefix+"paths",work.paths);
    add(prefix+"bytes",work.bytes);add(prefix+"reference_remaps",work.references);
  }
  add("execution.group_checks",costs.group_checks);add("execution.group_reuses",costs.group_reuses);
  add("execution.bound_writes",costs.bound_writes);add("execution.checked_writes",costs.checked_writes);
  add("topology.anchor_changes",costs.anchor_changes);
  for(unsigned i=0;i<costs.topology.size();++i){
    const auto prefix=std::string(i==0?"topology.source_native.":"topology.other.");
    const auto &t=costs.topology[i];
    add(prefix+"batches",t.batches);add(prefix+"groups",t.groups);
    add(prefix+"common_reads",t.common_reads);add(prefix+"roster_reads",t.roster_reads);
    add(prefix+"device_reads",t.device_reads);add(prefix+"region_queries",t.region_queries);
    add(prefix+"region_hits",t.region_hits);add(prefix+"region_fallbacks",t.region_fallbacks);
    add(prefix+"fact_hits",t.fact_hits);add(prefix+"lease_acquires",t.lease_acquires);
    add(prefix+"hologram_calls",t.hologram_calls);add(prefix+"read_failures",t.read_failures);
    add(prefix+"revoked",t.revoked);
  }
  constexpr CpuStage stages[]={CpuStage::MainCapture,CpuStage::OrdinaryOuter,CpuStage::BodyOuter,CpuStage::FrameInterval,
    CpuStage::OrdinaryPublishedOuter,CpuStage::OrdinaryFailedOuter,CpuStage::BodyPublishedOuter,CpuStage::BodyFailedOuter,
    CpuStage::BodyMixedOuter,CpuStage::BodyJointOnlyOuter,CpuStage::CaptureIdleOuter,CpuStage::TopologyCommon,CpuStage::TopologyGroups,
    CpuStage::TopologyLease,CpuStage::AnchorInstall,CpuStage::FieldExecution,CpuStage::JointPrepare,CpuStage::BodyClone};
  for(const auto stage:stages)for(unsigned q:{50u,95u,99u}){
    const auto key=std::string("duration.")+CpuStageName(stage)+".";
    add(key+"wall_p"+std::to_string(q)+"_us",costs.metrics.duration[unsigned(stage)].Percentile(q));
    add(key+"cpu_p"+std::to_string(q)+"_us",costs.metrics.cpu[unsigned(stage)].Percentile(q));
  }
  return out;
}
}
