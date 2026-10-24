#pragma once
#include "core/session.h"
#include "core/tick_clock.h"
#include "game/bridge.h"
#include <string>
namespace cs::game {
struct SoloRuntime;
SoloRuntime *Runtime() noexcept;
uint64_t ManagerLoop() noexcept;
bool SoloBusy() noexcept;
bool Armed() noexcept;
bool RuntimeHealthy() noexcept;
const char *StartSolo() noexcept;
void StopSolo(core::SessionCause cause = core::SessionCause::Stop) noexcept;
void BeginManager(uint64_t loop) noexcept;
void EndManager(bool normal) noexcept;
void FailRuntime(
    const char *reason,
    core::TickDecision decision = core::TickDecision::Cancelled) noexcept;
std::string StatusFields();
const char *StateName() noexcept;
} // namespace cs::game
