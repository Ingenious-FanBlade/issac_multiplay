#pragma once
// 主线程接收后台准备好的录制上下文。本头不供 SDK wrapper 使用。
#include "world/collector.h"
#include "world/writer.h"
#include <memory>

namespace cs::game {
// owner 保证 storage/ledger 在 reader/writer 真实结束前常驻；禁止放到游戏栈上。
// 命令层在后台完成 storage/identity/writer 准备，再于 Manager 边界 Attach。
struct WorldRecording final {
  // 测试状态先构造、最后释放，reader/writer 的非拥有指针不会悬空。
  world::FaultSwitches faults;
  ~WorldRecording();
  world::BudgetLedger ledger;
  world::WorldStorage storage;
  world::IdentityTable identities;
  world::WorldWriter writer;
  std::unique_ptr<world::Collector> collector;
};
void InitializeWorldBridge() noexcept;
// 主线程只读 PE 元数据，供后续后台准备构造 Collector；copy 本身带 SEH 保护。
world::MemoryReader WorldMemoryReader() noexcept;
bool AttachPreparedWorld(std::shared_ptr<WorldRecording> recording) noexcept;
// callback/collector 与后台都结束后撤销桥引用；最终大块释放留在后台。
bool DetachClosedWorld(const std::shared_ptr<WorldRecording> &recording) noexcept;
void StopWorldRecording() noexcept;
// 调用方可保留 shared_ptr 读值状态；仅主线程访问上下文和 collector。
std::shared_ptr<WorldRecording> CurrentWorldRecording() noexcept;
void WorldFrameBegin(std::uint64_t loop) noexcept;
void WorldPreparePostState(bool normal) noexcept;
core::TickPostState WorldPostState(std::uint64_t tick) noexcept;
void WorldPublishPostState(bool committed, const core::TickPostState &post) noexcept;
void WorldFrameEnd() noexcept;
void WorldNativeReturned(const core::TickBoundary &boundary, std::uint64_t tick) noexcept;
void WorldRefreshAnchors() noexcept;
world::PlayerAnchor WorldPlayerAnchor(unsigned ordinal) noexcept;
void WorldPlayerReturned(unsigned ordinal, const world::PlayerAnchor &before,
                         std::uint64_t tick, bool normal, bool body) noexcept;
void WorldRevokePlayer(unsigned ordinal) noexcept;
} // namespace cs::game
