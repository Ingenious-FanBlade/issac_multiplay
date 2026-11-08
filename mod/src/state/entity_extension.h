#pragma once
#include "state/nested.h"
#include <optional>

namespace cs::state {
// EntityPlus 的线上纯值：-1 为未覆盖飞行，水面值用第 32 位区分 Null 与有效零。
// 这不是原生 optional 的内存镜像，不包含 vptr、holder 或未设置的负载。
struct EntityExtensionState {
  std::int8_t flying = -1;
  std::uint64_t water = 0;
};
EntityExtensionState ReadEntityExtension(const ReadWindow &, std::uint32_t entity,
                                         std::string_view field);
// 校验线上Nullable编码后返回自有值；原生赋值必须由Client当前生命周期owner执行。
std::optional<bool> DecodeFlyingOverride(std::int8_t value, std::string_view field);
std::optional<std::uint32_t> DecodeWaterOverride(std::uint64_t value, std::string_view field);
}
