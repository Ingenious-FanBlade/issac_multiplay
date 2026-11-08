#include "core/capture_cost.h"
#include "replica/stream_metrics.h"
#include "replica/stream_e2e.h"

namespace cs::capture {
namespace {
using namespace replica::stream;
// 数据只由Manager主线程操作；后台构建成本由自己的Metrics单列。
struct Costs final {
  struct Span {CostPart part;std::uint64_t wall,cpu,child_wall=0,child_cpu=0;};
  Metrics metrics;bool active=false,cpu_known=true;unsigned depth=0;
  std::uint64_t outer=0,previous=0,total=0,cpu=0;
  std::array<Span,64> stack{};std::array<std::uint64_t,5> parts{},part_cpu{};
} costs;
// 来源Hook也可能报告错误线程；只有FrameBegin所在的主线程才可写此帧的聚合器。
thread_local bool capture_thread=false;
void Observe(CpuStage stage,std::uint64_t wall,std::uint64_t cpu) noexcept {
  costs.metrics.duration[unsigned(stage)].Observe(wall);if(cpu!=UINT64_MAX)costs.metrics.cpu[unsigned(stage)].Observe(cpu);else ++costs.metrics.missing;
#ifdef ISAACCS_E2E
  E2ETime(&costs.metrics,stage,wall,cpu);
#endif
}
}
void FrameBegin(std::uint64_t outer,bool active) noexcept {
  capture_thread=true;
  const auto now=MetricMicros();
  if(costs.active&&costs.previous&&outer>costs.outer){
    Observe(CpuStage::FrameInterval,now-costs.previous,0);
    Observe(CpuStage::MainCapture,costs.total,costs.cpu_known?costs.cpu:UINT64_MAX);
    constexpr CpuStage parts[]={CpuStage::SourceNative,CpuStage::SourceLua,CpuStage::SourceCopy,CpuStage::SourceControl,CpuStage::SourceHandoff};
    for(unsigned i=0;i<5;++i)if(costs.parts[i])Observe(parts[i],costs.parts[i],costs.cpu_known?costs.part_cpu[i]:UINT64_MAX);
  }
  if(costs.depth){++costs.metrics.missing;costs.depth=0;}
  costs.active=active;costs.outer=outer;costs.previous=active?now:0;costs.total=costs.cpu=0;costs.parts={};costs.part_cpu={};costs.cpu_known=true;
}
bool CostBegin(CostPart part) noexcept {
  if(!capture_thread||!costs.active)return false;
  // 同类嵌套由外层一次计时覆盖，避免每个getter产生重复计时系统调用。
  if(costs.depth&&costs.stack[costs.depth-1].part==part)return false;
  if(costs.depth==costs.stack.size()){++costs.metrics.missing;return false;}
  costs.stack[costs.depth++]={part,MetricMicros(),ThreadCpuMicros()};return true;
}
void CostEnd() noexcept {
  if(!costs.depth)return;const auto end=MetricMicros(),cpu=ThreadCpuMicros();const auto span=costs.stack[--costs.depth];const auto wall=end-span.wall;
  const bool known=cpu!=UINT64_MAX&&span.cpu!=UINT64_MAX&&cpu>=span.cpu;const auto used=known?cpu-span.cpu:0;
  costs.parts[unsigned(span.part)]+=wall-span.child_wall;if(known)costs.part_cpu[unsigned(span.part)]+=used>=span.child_cpu?used-span.child_cpu:0;else costs.cpu_known=false;
  if(costs.depth){auto &parent=costs.stack[costs.depth-1];parent.child_wall+=wall;parent.child_cpu+=used;}
  else {costs.total+=wall;costs.cpu+=used;}
}
void FrameEnd() noexcept {
  if(!costs.active)return;if(costs.depth){++costs.metrics.missing;return;}
  // 在下一真实Manager入口归约，包含本尾部之后的渲染/资源失效工作，不漏计帧间回调。
}
std::string CostFields(unsigned page){return costs.metrics.Fields("stream.main.",page);}
}
