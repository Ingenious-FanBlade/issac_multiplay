#pragma once
#include "game/bridge.h"

namespace cs::game {
struct SessionRuntime;
// 每次稳定观察后收口持久窗口（死亡/复活/换层/Continue 的完成与超时）。
// 只读当前 runtime 的已复制值，不调用原生函数、不写游戏对象。
void ObservePersistenceWindows(SessionRuntime &runtime) noexcept;
// 退休前关闭所有未收口窗口并记录残留原因，避免把未观测当成“无副作用”。
void FinishPersistenceWindows(const char *reason) noexcept;
} // namespace cs::game
