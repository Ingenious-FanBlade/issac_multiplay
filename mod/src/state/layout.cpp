#include "state/layout.h"
#include <algorithm>
#include <array>
#include <iterator>

namespace cs::state {
namespace {
#include "state_wire.inc"
#include "state_capture.inc"
#include "state_client.inc"
#include "state_members.inc"
}
std::uint16_t LayoutVersion() noexcept { return kStateLayoutVersion; }
std::span<const Layout> Layouts() noexcept { return kStateLayouts; }
std::span<const Field> Fields() noexcept { return kStateFields; }
std::span<const CopyRange> CopyRanges() noexcept { return kStateCopyRanges; }
std::span<const Block> Blocks() noexcept { return kStateBlocks; }
std::span<const Validator> Validators() noexcept { return kStateValidators; }
std::span<const NativeRange> NativeRanges() noexcept { return kStateNativeRanges; }
std::span<const Member> Members() noexcept { return kStateMembers; }
std::span<const ClientField> ClientFields() noexcept { return kStateClientFields; }
std::span<const CountConstraint> CountConstraints() noexcept { return kStateCountConstraints; }
std::span<const Variant> Variants() noexcept { return kStateVariants; }
std::span<const ContextField> ContextFields() noexcept { return kStateContexts; }
std::span<const FieldCondition> FieldConditions() noexcept { return kStateConditions; }
std::span<const LayoutChoice> LayoutChoices() noexcept { return kStateChoices; }
std::span<const CatalogRow> CatalogRows() noexcept { return kStateCatalogRows; }
std::span<const SupplementaryMember> SupplementaryMembers() noexcept { return kStateSupplementaryMembers; }
std::span<const CatalogAlias> CatalogAliases() noexcept { return kStateCatalogAliases; }
std::span<const LayoutBudget> LayoutBudgets() noexcept { return kStateBudgets; }
std::span<const PresentationContract> PresentationContracts() noexcept { return kStatePresentationContracts; }
std::span<const PresentationDependency> PresentationDependencies() noexcept { return kStatePresentationDependencies; }
std::span<const ProtocolConstant> ProtocolConstants() noexcept { return kStateProtocolConstants; }
// 布局号可能来自线上数据：先对生成的直接索引做边界检查再取表项，常数时间且不分配。
const Layout *FindLayout(std::uint16_t id) noexcept {
  if (id >= kStateLayoutSlots.size()) return nullptr;
  const auto slot = kStateLayoutSlots[id];
  return slot == kNullIndex ? nullptr : &kStateLayouts[slot];
}
const Field *FindField(std::uint16_t id, std::uint16_t index) noexcept {
  const auto *layout = FindLayout(id);
  if (!layout || index >= layout->field_count) return nullptr;
  return &kStateFields[layout->first_field + index];
}
bool ValidContainerCountGroups(std::uint16_t id, std::span<const std::uint16_t> counts) noexcept {
  const auto *layout = FindLayout(id);
  if (!layout || counts.size() != layout->field_count) return false;
  auto it = std::lower_bound(kStateCountConstraints.begin(), kStateCountConstraints.end(), id,
    [](const CountConstraint &row, std::uint16_t key) { return row.layout < key; });
  while (it != kStateCountConstraints.end() && it->layout == id) {
    const auto group = it->group;
    const auto limit = it->limit;
    std::uint32_t total = 0;
    do {
      const auto slot = it->slot;
      const auto value = counts[it->field];
      do {
        // 元素本身的limit与跨容器合计limit分别核对，不把重复投影重复计数。
        if (counts[it->field] != value ||
            value > kStateFields[layout->first_field + it->field].limit) return false;
        ++it;
      } while (it != kStateCountConstraints.end() && it->layout == id && it->group == group && it->slot == slot);
      total += value;
      if (total > limit) return false;
    } while (it != kStateCountConstraints.end() && it->layout == id && it->group == group);
  }
  return true;
}
// Client分派表与字段表在同一生成轮次保持一一对应，不逐字段比较字符串。
const ClientField *FindClientField(std::uint16_t id, std::uint16_t index) noexcept {
  const auto *layout = FindLayout(id);
  if (!layout || index >= layout->field_count) return nullptr;
  return &kStateClientFields[layout->first_field + index];
}
}
