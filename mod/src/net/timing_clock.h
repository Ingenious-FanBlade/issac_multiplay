#pragma once
// 输入时序链专用的本机单调毫秒钟（2026-11-20，降低输入提前量）。
// 覆盖范围：Host 的 tick 提交时刻（server_sample_ms）、Roster／TickProgress 发送时刻、收到客机控制消息的时刻（echo_server_ms），
// 客机控制消息的发送时刻（sent_ms → echo_client_ms）、收到 Roster／TickProgress 的时刻，以及计算输入目标时的“现在”。
// 这些值只在同一进程内相减（RTT、发布年龄、本地经过），从不跨机器比较绝对值，因此换钟不改线格式。
// 原先用 GetTickCount64（10—16ms 量化），输入投影须加 4×16=64ms 余量，约占提前量 4 tick；
// steady_clock 在 MSVC 上基于 QueryPerformanceCounter，截断到毫秒后每个差值误差小于 1ms。
// 要求 Host 与客机同为本版本：若一端仍用 GetTickCount64，余量不足会使输入迟到。
#include <chrono>
#include <cstdint>

namespace cs::net {
// 每个毫秒差值的最大截断误差。
inline constexpr std::uint64_t kTimingQuantumMs = 1;
// 本机单调毫秒；进程内单调不减，开机后恒非零。
inline std::uint64_t TimingClockMs() noexcept {
  return std::uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now().time_since_epoch())
                           .count());
}
} // namespace cs::net
