#pragma once
#include "game/bridge.h"
namespace cs::game {
ConsumerScope *CurrentConsumer() noexcept;
bool ControlledPlayer(void *player) noexcept;
void RecordQuery(ConsumerKind consumer, QueryKind query, int action, float result) noexcept;
} // namespace cs::game
