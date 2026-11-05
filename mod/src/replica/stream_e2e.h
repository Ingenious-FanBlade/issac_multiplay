#pragma once
#include "replica/stream_metrics.h"
#ifdef ISAACCS_E2E
namespace cs::replica::stream {
// 仅显式token启用的短期诊断；后台只写自有值，溢出/缺测不影响产品接纳。
void E2ETrace(bool enabled) noexcept;
void E2EFlow(const Metrics *,const RangeId &,std::uint64_t,const Sample &,FlowStage) noexcept;
void E2ETime(const Metrics *,CpuStage,std::uint64_t wall,std::uint64_t cpu) noexcept;
void E2EPacket(std::uint64_t connection,std::uint64_t route,std::uint64_t bytes,const std::array<std::uint64_t,4> &,const char *direction) noexcept;
std::string E2ETracePage(std::uint64_t cursor);
void E2EArm(const std::string &role,unsigned kind,std::uint64_t revision);
bool E2EWants(const char *role,const RangeData &) noexcept;
void E2ESnapshot(const char *role,const Range &,const std::vector<Range> &,std::uint64_t connection,std::uint64_t route,const std::string &native={},const std::vector<Version> &commit_vector={}) noexcept;
void E2ERender(std::uint64_t connection,std::uint64_t route,const std::vector<Version> &,std::uint64_t revision) noexcept;
void E2EBody(std::uint64_t local,std::uint64_t use,std::uint64_t shape,std::uint64_t binding,std::uint64_t tick,unsigned stage) noexcept;
std::string E2ESnapshotPage(unsigned slot,unsigned page);
}
#endif
