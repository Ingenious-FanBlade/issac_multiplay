#pragma once
// hooks.json派生的动态入口与原生入口使用同一个协调owner，补丁终身常驻。
#include <cstdint>
namespace cs {
struct RuntimeRandomHookSpec final { const char *id,*module,*symbol; unsigned source; };
void InstallRuntimeRandomHook(unsigned source,void *target) noexcept;
void InitializeRuntimeRandomHooks() noexcept;
void *RuntimeRandomOriginal(unsigned source) noexcept;
bool RuntimeRandomHookInstalled(unsigned source) noexcept;
unsigned RuntimeRandomImportMask(unsigned source) noexcept;
}
