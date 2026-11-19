#pragma once
#include <cstdint>
#include <string>
namespace cs::capture {
enum class CostPart : unsigned { Native, Lua, Copy, Control, Handoff, ScenePrepare, TopologyResources, EntityValues, SceneValues, FastBody, Review, Commit,
  TopologyCommon, TopologyGroups, TopologyLease, AnchorInstall, FieldExecution, JointPrepare, BodyClone, Count };
enum class CostOutcome : unsigned { Published, Failed, JointOnly };
// 只在已激活的Host主线程记录固定聚合；嵌套计时做排他归属，总计不重复相加。
void FrameBegin(std::uint64_t outer,bool active) noexcept;
void FrameEnd() noexcept;
void Candidate(bool ordinary,CostOutcome) noexcept;
// 执行器在本根累加整数，退出时一次并入现有主线程诊断。
struct ExecutionCounts {std::uint32_t group_checks=0,group_reuses=0,bound_writes=0,checked_writes=0;};
void CaptureExecutionCounts(const ExecutionCounts &) noexcept;
inline void Candidate(bool ordinary,bool published) noexcept {Candidate(ordinary,published?CostOutcome::Published:CostOutcome::Failed);}
// 每批仅上报一次；无逐字段计时和日志。生命周期入口与 source_native 内的批次分别累计。
struct TopologyCounts {
  std::uint64_t batches=0,groups=0,common_reads=0,roster_reads=0,device_reads=0;
  std::uint64_t region_queries=0,region_hits=0,region_fallbacks=0,fact_hits=0;
  std::uint64_t lease_acquires=0,hologram_calls=0,read_failures=0,revoked=0;
};
void CountTopology(const TopologyCounts &) noexcept;
void CountAnchorChanges(unsigned count) noexcept;
// 固定的快槽复用结果，只在每个真实 PlayerPost 选择一次。
enum class BodyReuse : unsigned { Match, NoOrdinary, NoSharedBody, Subject, Window, TooLarge, Stale, Unqualified, NoLane, Count };
struct BodyCloneCounts {std::uint32_t cells=0,paths=0,bytes=0,references=0;};
void CountBodyReuse(BodyReuse) noexcept;
void CountBodyClone(bool success,const BodyCloneCounts &) noexcept;
bool CostBegin(CostPart) noexcept;
void CostEnd() noexcept;
// Host 主线程采集窗口的累计计数与耗时，供 HUD 计算最近窗口均值。
struct CaptureTiming {std::uint64_t count=0,total_us=0;};
CaptureTiming ReadCaptureTiming() noexcept;
std::string CostFields(unsigned page);
class Cost final {
public:
  explicit Cost(CostPart part,bool enabled=true) noexcept:active_(enabled&&CostBegin(part)){}
  ~Cost(){if(active_)CostEnd();}
private:bool active_;
};
}
