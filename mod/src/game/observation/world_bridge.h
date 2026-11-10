#pragma once
// 主线程接收后台准备好的录制上下文。本头不供 SDK wrapper 使用。
#include "world/collector.h"
#include "world/writer.h"
#include "world/common/service.h"
#include "world/common/publication.h"
#include <memory>
#include <deque>
#include <map>
#include "world/domains/level/model.h"
#include "game/observation/sources/global_random.h"
#include "game/observation/sources/random_sources.h"
#include "world/common/live_consumer.h"
#include "replica/stream_capture_worker.h"
#include "replica/stream_config.h"

namespace cs::game {
// Text缓冲资格来自本use真实writer，不由Init/内存可读或旧池内容推断。
struct EntityTextWriteWitness final { std::uint64_t local=0,use=0,revision=0; };
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
  world::observation::LiveConsumer live;
  replica::stream::CaptureWorker stream;
  replica::stream::RuntimeConfig stream_config;
  // 候选只跨同一Manager的prepare→clock提交，正常收束后再次逐槽复核。
  struct PendingBody final {world::PlayerPostWitness witness;capture::Source source;std::uint64_t invalidation=0;};
  std::array<PendingBody,world::kControlAnchorCapacity> pending_bodies{};
  std::uint64_t pending_body_bytes=0;
  // 槽仅作当前绑定的本地索引；对外诊断同时给local/use/shape/binding，不将槽充当网络身份。
  struct BodyStatus final {
    std::uint64_t local=0,use=0,shape=0,binding=0,last_native_update_tick=0,last_capture_tick=0,ordinary=0,odd=0,not_updated=0,actual_updates=0,qualified_samples=0,throttled=0;
    const char *reason="player_post_never_updated";
  };
  std::array<BodyStatus,world::kControlAnchorCapacity> body_status{};
  std::uint64_t stream_invalidation=0;
  // 资源代次只作为纯值身份；只读getter临时副本继承来源资源代次，不冒充世界owner。
  struct StreamResourceIdentity final {
    std::uint64_t generation=0;bool observed=false,temporary=false;
    // Lua按值返回的ANM保留真实copy来源；嵌套Reset完成后才能确认该别名。
    std::uint64_t copy_generation=0;std::uint32_t copy_address=0;bool constructing=false;
  };
  std::map<std::uint32_t,StreamResourceIdentity> stream_resources;
  std::uint64_t next_stream_resource=0;
  std::uint64_t stream_resource_mutation=0,stream_read_resource_mutation=0;
  unsigned stream_resource_depth=0;
  world::observation::PublicationQueue publications{ledger};
  std::uint64_t session_key=0,epoch=0;
  std::array<std::uint64_t,13> pending_scopes{};
  world::observation::ObservationEnvelope latest_entity{},latest_grid{},latest_room{},latest_level{};
  // 正式消费者保存Text独立Render最新值，关闭Recorder仍可取用；不覆盖Ordinary Entity。
  std::map<std::uint64_t,world::observation::ObservationEnvelope> entity_render_samples;
  std::uint64_t entity_render_room=0;
  std::size_t entity_render_bytes=0;
  // 跨房三域及新增四域只保存自有值；Player本体引用同批Entity scope，不重复采集。
  std::array<world::observation::ObservationEnvelope,7> latest_continuity{};
  // 每个Lua来源的最近实际Partial原因，仅供诊断；不改变完整采集或发布资格。
  std::array<std::string,11> lua_partial_reasons{};
  GlobalRandomMirror global_random{};
  std::shared_ptr<RandomRegistry> random_sources=MakeRandomRegistry();
  world::observation::ObservationEnvelope latest_global_random{};
  std::uint64_t current_entity_scope=0;
  world::level::Hierarchy hierarchy{};
  std::uint64_t lua_run=0,lua_room=0;
  bool location_window=false;
  bool formal_consumer=false,recorder_consumer=false;
  bool live_consumer=false;
  const char *live_request_reason="live_off";
  std::uint64_t lua_attempts=0,lua_complete=0,lua_partial=0;
  std::uint64_t formal_loss=0;
  // 显式E2E仅保留最近一批不可用Grid身份诊断；不进入正式目标或生产日志。
  std::string e2e_grid_faults;
#ifdef ISAACCS_E2E
  // 正式consumer旁的测试转储缓冲；绝不被正式目标/调度读取。
  std::deque<world::observation::ObservationEnvelope> e2e_entity_independent;
  std::size_t e2e_entity_bytes=0;
  bool e2e_entity_loss=false;
#endif
  // 故障续测保留有界的实际创建/退休边，便于区分漏记取得与错误退休。
  std::deque<world::LifecycleEvent> e2e_grid_events;
  bool reserve_charged=false;
  world::MemoryReader source_memory{};
  // 只在主线程查找地址关联；至多16384项，scope退休时随采集owner释放。
  std::map<std::uint32_t,EntityTextWriteWitness> text_writes;
  // 只在同步 Lua 调用期间有效，均为本批自有 RawObject 值，不交给后台。
  const world::RawObject *lua_objects=nullptr;
  const world::RawMembership *lua_members=nullptr;
  std::uint32_t lua_members_count=0;
  std::uint32_t lua_object_count=0;
  std::uint64_t lua_scope=0;
  std::array<std::uint64_t,world::kIdentityBudget+1> lua_subject_scopes{};
  std::array<std::uint64_t,4> control_sessions{};
  std::array<std::uint64_t,4> device_relation_revisions{};
  std::array<world::observation::SubjectKey,4> participant_keys{},control_group_keys{};
  void ConnectSources() noexcept;
  void SetConsumers(bool formal,bool recorder) noexcept;
  world::observation::RequestResult SetLiveConsumer(bool active) noexcept;
  world::observation::RequestResult UpdateLiveCapturePlan(world::observation::CapturePlan plan) noexcept;
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
// 仅主线程取共享自有值；后台持有返回值不延长任何游戏对象/Lua借用。
std::shared_ptr<const world::observation::LiveBatch> CurrentLiveCapture() noexcept;
std::array<std::uint64_t,4> CurrentLiveSource() noexcept;
std::shared_ptr<const replica::stream::Publication> CurrentLiveWorld() noexcept;
std::shared_ptr<const replica::stream::Publication> CurrentLiveBaseline() noexcept;
bool RequestLiveBaseline() noexcept;
std::vector<std::shared_ptr<const replica::stream::Publication>> CurrentLiveBodies();
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
void ObservationControlChange(unsigned slot,const world::PlayerAnchor &before,const world::PlayerAnchor &after,const char *reason) noexcept;
} // namespace cs::game
