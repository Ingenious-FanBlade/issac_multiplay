#pragma once
#include "state/nested.h"
#include <optional>

namespace cs::state {
// 同一个具体共享owner分投影编码；仅首块带present，尾块依赖同目标版本首块。
// 缺失时返回空视图，调用者不得把全零占位递归解释成有效资源或动画状态。
struct SharedOwnerPayload {
  bool present;
  std::span<const std::byte> bytes;
};
// projected只含已递归投影的payload；owner缺失时必须为空，不读取原生空指针。
std::span<const std::byte> EncodeSharedOwnerProjectionInto(
    const Field &, bool present, std::span<const std::byte> projected,
    std::span<std::byte> storage, std::string_view field);
// 首块可以不传presence；其它块必须传同版本首块已经校验的存在性。
SharedOwnerPayload DecodeSharedOwnerProjection(
    const Field &, std::span<const std::byte> encoded, std::optional<bool> presence,
    std::string_view field);
}
