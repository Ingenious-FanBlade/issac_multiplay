#pragma once
#include "game/common/bridge.h"
#include "game/participants/topology.h"

namespace cs::game {
struct SessionRuntime;
// 物理历史属于设备源，独立于 Session 与 Participant generation。
void CaptureInput(void *device, uint64_t loop, const PhysicalActions &actions);
void DiscardInputEdges() noexcept;
void ForgetInputDevice() noexcept;
bool SubmitInput(SessionRuntime &runtime) noexcept;
void InputFrameFinished() noexcept;
} // namespace cs::game
