#pragma once
#include "replica/stream_metrics.h"
#ifdef ISAACCS_E2E
namespace cs::replica::stream {
struct Message;
// 仅显式token启用的短期诊断；后台只写自有值，溢出/缺测不影响产品接纳。
void E2ETrace(bool enabled) noexcept;
void E2EObservationLoss() noexcept;
void E2EFlow(const Metrics *,const RangeId &,std::uint64_t,const Sample &,FlowStage) noexcept;
void E2ETime(const Metrics *,CpuStage,std::uint64_t wall,std::uint64_t cpu) noexcept;
void E2EPacket(std::uint64_t connection,std::uint64_t route,std::uint64_t bytes,const std::array<std::uint64_t,4> &,const char *direction) noexcept;
std::string E2ETracePage(std::uint64_t cursor);
void E2EArm(const std::string &role,unsigned kind,std::uint64_t revision);
bool E2EWants(const char *role,const RangeData &) noexcept;
void E2ESnapshot(const char *role,const Range &,const std::vector<Range> &,std::uint64_t connection,std::uint64_t route,const std::string &native={},const std::vector<Version> &commit_vector={}) noexcept;
void E2ERender(std::uint64_t connection,std::uint64_t route,const std::vector<Version> &,std::uint64_t revision) noexcept;
void E2EBody(std::uint64_t local,std::uint64_t use,std::uint64_t shape,std::uint64_t binding,std::uint64_t tick,unsigned stage) noexcept;
// 调用见证与最终文件对比分开；只复制固定名称及线程/context纯值。
void E2EIsolation(const char *kind,const char *entry,bool main,bool guarded,bool context,unsigned owner,const std::array<std::uint64_t,9> &identity) noexcept;
// 仅显式视觉中性段暂留一次已提交画面，Host/网络继续；最多120秒，不计入动态指标。
void E2EVisual(bool enabled) noexcept;
bool E2EVisualHeld() noexcept;
bool E2EVisualNeutral() noexcept;
// 物理来源、canonical接纳与真实consumer分别记值，不用processed ACK冒充动作。
struct InputWitness final {
 unsigned participant=0,body=0,authority_kind=0,edge=0;
 std::uint64_t generation=0,sequence=0,target=0,tick=0;
 std::array<int,4> axes{};
};
void E2EInput(const char *event,const InputWitness &) noexcept;
void E2EAge(std::uint64_t authority,std::uint64_t source,std::uint64_t elapsed,std::uint64_t error,bool stale) noexcept;
// 在正式sender形成真实Hello/Welcome后仅改一次负向正文；接收端仍走完整wire解码。
void E2EWireArm(const std::string &name);
std::string E2EWireStatus();
std::string E2EHelloPackets(std::uint64_t nonce);
bool E2EWirePayload(const Message &,Limits,std::vector<std::uint8_t> &);
std::string E2ESnapshotPage(unsigned slot,unsigned page);
}
#endif
