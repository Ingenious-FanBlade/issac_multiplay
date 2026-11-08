#include "state/owned_payload.h"
#include <algorithm>
#include <cstring>

namespace cs::state {
namespace {
// 只能使用可信生成表的间接owner字段；原生holder大小与payload大小分开解释。
std::size_t Header(const Field &plan, std::string_view field) {
  if (plan.operation != Operation::Nested || !plan.storage ||
      std::string_view(plan.storage) != "shared-owner" || !plan.nested_layout ||
      plan.native_size != 8 || plan.min_size > plan.max_size)
    throw Gap(field, "shared_owner_plan");
  const std::size_t header = plan.nested_block == 0 || plan.nested_block == 255 ? 1 : 0;
  if (plan.min_size < header) throw Gap(field, "shared_owner_header_size");
  return header;
}
}

std::span<const std::byte> EncodeSharedOwnerProjectionInto(
    const Field &plan, bool present, std::span<const std::byte> projected,
    std::span<std::byte> storage, std::string_view field) {
  const auto header = Header(plan, field);
  if (!present) {
    if (!projected.empty() || storage.size() < plan.min_size)
      throw Gap(field, "shared_owner_absent_payload");
    // 不存在的动态字符串/图层/节点只写最短零计数；固定部分逐字节归零。
    const auto bytes = storage.first(plan.min_size);
    std::fill(bytes.begin(), bytes.end(), std::byte{});
    return bytes;
  }
  if (projected.size() < plan.min_size - header || projected.size() > plan.max_size - header ||
      storage.size() < header || projected.size() > storage.size() - header)
    throw Gap(field, "shared_owner_payload_bounds");
  if (!projected.empty()) std::memmove(storage.data() + header, projected.data(), projected.size());
  if (header) storage[0] = std::byte{1};
  return storage.first(header + projected.size());
}

SharedOwnerPayload DecodeSharedOwnerProjection(
    const Field &plan, std::span<const std::byte> encoded, std::optional<bool> presence,
    std::string_view field) {
  const auto header = Header(plan, field);
  if (encoded.size() < plan.min_size || encoded.size() > plan.max_size)
    throw Gap(field, "shared_owner_payload_bounds");
  if (header) {
    const auto marker = std::to_integer<unsigned>(encoded[0]);
    if (marker > 1 || (presence && *presence != (marker != 0)))
      throw Gap(field, "shared_owner_presence");
    presence = marker != 0;
  } else if (!presence) {
    throw Gap(field, "shared_owner_presence_dependency");
  }
  if (!*presence) {
    if (encoded.size() != plan.min_size ||
        !std::all_of(encoded.begin(), encoded.end(), [](std::byte value) { return value == std::byte{}; }))
      throw Gap(field, "shared_owner_absent_not_zero");
    return {false, {}};
  }
  return {true, encoded.subspan(header)};
}
}
