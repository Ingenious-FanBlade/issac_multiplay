#include "state/layout.h"
#include <array>
#include <iterator>

namespace cs::state {
namespace {
#include "state_wire.inc"
#include "state_capture.inc"
#include "state_client.inc"
#include "state_members.inc"
}
std::uint32_t LayoutVersion() noexcept { return kStateLayoutVersion; }
std::span<const Layout> Layouts() noexcept { return kStateLayouts; }
std::span<const Field> Fields() noexcept { return kStateFields; }
std::span<const CopyRange> CopyRanges() noexcept { return kStateCopyRanges; }
std::span<const Block> Blocks() noexcept { return kStateBlocks; }
std::span<const Validator> Validators() noexcept { return kStateValidators; }
std::span<const NativeRange> NativeRanges() noexcept { return kStateNativeRanges; }
std::span<const Member> Members() noexcept { return kStateMembers; }
std::span<const ClientField> ClientFields() noexcept { return kStateClientFields; }
std::span<const Variant> Variants() noexcept { return kStateVariants; }
std::span<const ContextField> ContextFields() noexcept { return kStateContexts; }
const Layout *FindLayout(std::uint16_t id) noexcept {
  for (const auto &layout : kStateLayouts) if (layout.id == id) return &layout;
  return nullptr;
}
const Field *FindField(std::uint16_t id, std::uint16_t index) noexcept {
  const auto *layout = FindLayout(id);
  if (!layout || index >= layout->field_count) return nullptr;
  return &kStateFields[layout->first_field + index];
}
// Client分派表与字段表在同一生成轮次保持一一对应，不逐字段比较字符串。
const ClientField *FindClientField(std::uint16_t id, std::uint16_t index) noexcept {
  const auto *layout = FindLayout(id);
  if (!layout || index >= layout->field_count) return nullptr;
  return &kStateClientFields[layout->first_field + index];
}
}
