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
const std::string &WorldCommandStatus() noexcept;
// 仅本地控制口主线程调用；token/代次/模式均在内部校验。
std::string TestWorldCommand(const char *source, const std::string &token,
    std::uint32_t generation, const std::string &mode, std::uint32_t value);
} // namespace cs
