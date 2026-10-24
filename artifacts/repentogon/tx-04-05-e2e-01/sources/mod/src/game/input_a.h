#pragma once
#include "game/bridge.h"
#include "game/topology.h"

namespace cs::game {
struct SoloRuntime;
// 物理历史属于设备源，独立于 Session 与 Participant generation。
void CaptureInput(void *device, uint64_t loop, const PhysicalActions &actions);
void DiscardInputEdges() noexcept;
void ForgetInputDevice() noexcept;
bool SubmitInput(SoloRuntime &runtime) noexcept;
void InputFrameFinished() noexcept;
} // namespace cs::game
