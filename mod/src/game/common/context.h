#pragma once
#include "runtime.h"

namespace cs::game {
// 固定容量主线程 owner 栈；token 不复用，不将原生地址当成调用身份。
uint64_t PushOwner(Owner owner) noexcept;
void PopOwner(uint64_t token) noexcept;
Owner TopOwner() noexcept;
uint64_t OwnerToken(Owner owner) noexcept;
// 包括已退出的嵌套 owner，供同步只读批次识别调用期间的重入。
uint64_t OwnerSequence() noexcept;
bool HasOwner(Owner owner) noexcept;
bool ContextHealthy() noexcept;
bool InBlockedContext() noexcept;
uint64_t ContextErrors() noexcept;
void ContextFault(const char *reason) noexcept;
// Manager 的唯一正常入口只能清理上一轮已经完全弹出的错误状态。
void BeginContextFrame() noexcept;
void ClearOddMarkers(uint64_t odd_token) noexcept;
bool TakeOddNormal(void *player) noexcept;
// 仅计数普通 shim 中的外层 Player 入口；特殊调用点保持单独的观测记录。
void RecordOddPlayerEntry() noexcept;
void __stdcall EnterOddNormal(void *player) noexcept;
void __stdcall LeaveOddNormal() noexcept;
} // namespace cs::game
