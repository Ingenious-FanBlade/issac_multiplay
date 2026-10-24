#pragma once
#include <cstdint>
namespace cs::game {
struct SoloRuntime;
enum Block : unsigned { Pause = 1, Loading = 2, Lifecycle = 4 };
void RefreshLifecycle(SoloRuntime &runtime, bool authority) noexcept;
void InvalidateBinding(SoloRuntime &runtime, const char *reason,
                       bool defer) noexcept;
bool ReadyForAuthority(SoloRuntime &runtime) noexcept;
} // namespace cs::game
