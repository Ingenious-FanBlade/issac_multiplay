#pragma once
#include "core/session.h"
#include "core/tick_clock.h"
#include "game/bridge.h"
#include "net/worker.h"
#include <string>
namespace cs::game {
struct SessionRuntime;
SessionRuntime *Runtime() noexcept;
uint64_t ManagerLoop() noexcept;
bool SoloBusy() noexcept;
bool Busy() noexcept;
bool Armed() noexcept;
bool RuntimeHealthy() noexcept;
// 仅有效Host会话请求Authority实时采集；Client、Solo、退休及失败均返回零。
std::uint64_t LiveCaptureSession() noexcept;
const char *StartSolo() noexcept;
// 后续命令层只负责校验与排队；这里是唯一主线程 Host 创建/停止入口。
const char *StartHost(const net::WorkerConfig &config,
                      int character = 0) noexcept;
void StopSession(core::SessionCause cause = core::SessionCause::Stop) noexcept;
void StopSolo(core::SessionCause cause = core::SessionCause::Stop) noexcept;
void BeginManager(uint64_t loop) noexcept;
void EndManager(bool normal) noexcept;
void FailRuntime(
    const char *reason,
    core::TickDecision decision = core::TickDecision::Cancelled) noexcept;
std::string StatusFields();
std::string HostStreamFields(unsigned ordinal,unsigned page);
std::string SourceStreamFields(unsigned page);
const char *StateName() noexcept;
} // namespace cs::game
