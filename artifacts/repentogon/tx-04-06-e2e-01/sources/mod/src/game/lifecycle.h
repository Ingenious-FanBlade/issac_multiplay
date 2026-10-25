#pragma once
#include <cstdint>
namespace cs::game {
struct SessionRuntime;
enum Block : unsigned { Pause = 1, Loading = 2, Lifecycle = 4 };
void RefreshLifecycle(SessionRuntime &runtime, bool authority) noexcept;
void InvalidateBinding(SessionRuntime &runtime, const char *reason,
                       bool defer) noexcept;
bool ReadyForAuthority(SessionRuntime &runtime) noexcept;
} // namespace cs::game
