#pragma once
#include "game/common/bridge.h"
namespace cs::game {
ConsumerScope *CurrentConsumer() noexcept;
bool ControlledPlayer(void *player) noexcept;
int ControlledOrdinal(void *player) noexcept;
void RecordQuery(unsigned ordinal, ConsumerKind consumer, QueryKind query,
                 int action, float result) noexcept;
} // namespace cs::game
