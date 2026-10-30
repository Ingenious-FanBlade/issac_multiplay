#pragma once
// 主线程接收后台准备好的录制上下文。本头不供 SDK wrapper 使用。
#include "world/collector.h"
#include "world/writer.h"
#include "world/common/service.h"
#include "world/common/publication.h"
#include <memory>

namespace cs::game {
// 采集 owner 不拥有文件录制器；即使没有 Recorder，也保持同一身份 epoch 和值消费。
struct WorldCapture final {
  // 测试状态先构造、最后释放，reader/writer 的非拥有指针不会悬空。
  world::FaultSwitches faults;
  ~WorldCapture();
  world::BudgetLedger ledger;
  world::WorldStorage storage;
  world::IdentityTable identities;
  std::unique_ptr<world::Collector> collector;
  world::observation::Service observation;
  world::observation::PublicationQueue publications{ledger};
  std::uint64_t session_key=0,epoch=0;
  std::array<std::uint64_t,4> pending_scopes{};
  world::observation::ObservationEnvelope latest_entity{},latest_grid{};
  bool formal_consumer=false,recorder_consumer=false;
  std::uint64_t lua_attempts=0,lua_complete=0,lua_partial=0;
  std::uint64_t formal_loss=0;
  bool reserve_charged=false;
  world::MemoryReader source_memory{};
  // 只在同步 Lua 调用期间有效，均为本批自有 RawObject 值，不交给后台。
  const world::RawObject *lua_objects=nullptr;
  const world::RawMembership *lua_members=nullptr;
  std::uint32_t lua_members_count=0;
  std::uint32_t lua_object_count=0;
  std::uint64_t lua_scope=0;
  std::array<std::uint64_t,world::kIdentityBudget+1> lua_subject_scopes{};
  std::array<std::uint64_t,4> control_sessions{};
  std::array<world::observation::SubjectKey,4> participant_keys{},control_group_keys{};
  void ConnectSources() noexcept;
  void SetConsumers(bool formal,bool recorder) noexcept;
};
// 旧 C++ 桥名字只作源码适配；实际身份寿命属于采集服务，不再属于录制器。
using WorldRecording=WorldCapture;
void InitializeWorldBridge() noexcept;
// 主线程只读 PE 元数据，供后续后台准备构造 Collector；copy 本身带 SEH 保护。
world::MemoryReader WorldMemoryReader() noexcept;
bool AttachPreparedWorld(std::shared_ptr<WorldRecording> recording) noexcept;
// callback/collector 与后台都结束后撤销桥引用；最终大块释放留在后台。
bool DetachClosedWorld(const std::shared_ptr<WorldRecording> &recording) noexcept;
void StopWorldRecording() noexcept;
// 调用方可保留 shared_ptr 读值状态；仅主线程访问上下文和 collector。
std::shared_ptr<WorldRecording> CurrentWorldRecording() noexcept;
bool WorldCaptureActive() noexcept;
void WorldFrameBegin(std::uint64_t loop) noexcept;
void WorldPreparePostState(bool normal) noexcept;
core::TickPostState WorldPostState(std::uint64_t tick) noexcept;
void WorldPublishPostState(bool committed, const core::TickPostState &post) noexcept;
void WorldFrameEnd() noexcept;
void WorldNativeReturned(const core::TickBoundary &boundary, std::uint64_t tick) noexcept;
void WorldRefreshAnchors() noexcept;
std::uint64_t WorldEntityLocalId(void *player) noexcept;
world::PlayerAnchor WorldPlayerAnchor(unsigned ordinal, void *player = nullptr) noexcept;
void WorldPlayerReturned(unsigned ordinal, const world::PlayerAnchor &before,
                         std::uint64_t tick, bool normal, bool body) noexcept;
void WorldRevokePlayer(unsigned ordinal) noexcept;
} // namespace cs::game
