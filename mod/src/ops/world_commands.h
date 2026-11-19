#pragma once
// 命令队列的主线程执行端；后台只准备和收口自有上下文，不调用游戏或 Lua。
#include <cstdint>
#include <string>
namespace cs {
bool WorldCommandBusy() noexcept;
std::string WorldRecordingId(std::uint32_t generation);
const char *WorldStartError() noexcept;
bool WorldCanCheckpoint() noexcept;
void StartWorldCommand(std::uint64_t request, std::uint32_t generation, const char *label) noexcept;
void StopWorldCommand(std::uint64_t request, std::uint32_t generation) noexcept;
void CheckpointWorldCommand(std::uint64_t request, std::uint32_t generation, const char *label) noexcept;
void PumpWorldCommands() noexcept;
// 原生开局前查询本会话的正式采集是否已进入当前 Manager 窗口。
bool WorldLiveStartReady(std::uint64_t session, const char *&error) noexcept;
const std::string &WorldCommandStatus() noexcept;
// 正式纯值消费需求独立于可选录制；实际激活／停用由 Manager Pump 执行。
std::string ObservationCommand(const std::string &action);
// main 的原始退出判断点调用：零等待，只决定本轮继续原循环或进入原 Shutdown。
bool WorldProcessExitDecision() noexcept;
bool WorldProcessExitPending() noexcept;
void InstallWorldExitPatch() noexcept;
bool WorldExitPatchInstalled() noexcept;
// 仅本地控制口主线程调用；token/代次/模式均在内部校验。
std::string TestWorldCommand(const char *source, const std::string &token,
    std::uint32_t generation, const std::string &mode, std::uint32_t value);
} // namespace cs
