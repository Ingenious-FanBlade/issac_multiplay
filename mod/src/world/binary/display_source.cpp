#include "world/binary/entity_source_internal.h"

namespace cs::world::binary {
// 当前facts固定借用一棵自有纯值树；报文、原生字段和worker都不能提供此游标。
const DisplayFieldValue *
EntitySource::Impl::DisplayValue(const CaptureFieldAccess &a,
                                 CaptureDiagnostic &d) noexcept {
  for (const auto &node : facts.display_values) {
    const auto address =
        node.native ? node.native
                    : std::uint32_t(reinterpret_cast<std::uintptr_t>(&node));
    const bool root=&node==facts.display_values.data()&&a.root.layout==facts.layout&&a.root.address==facts.address;
    if (node.layout != a.root.layout || (!root && address != a.root.address))
      continue;
    for (const auto &value : node.fields)
      if (value.field == a.field.index)
        return &value;
    break;
  }
  Reject(a, d, CaptureFailure::MissingValue);
  return nullptr;
}
bool EntitySource::Impl::DisplayLeaf(const CaptureFieldAccess &a,
                                     CaptureValueWriter &writer,
                                     CaptureDiagnostic &d) noexcept {
  const auto *value = DisplayValue(a, d);
  if (!value)
    return false;
  if (a.format.linked) {
    if (value->links.size() != a.format.components)
      return Reject(a, d, CaptureFailure::CountMismatch);
    return writer.PutLinks(a.location, a.field.operation == Operation::Resource,
                           a.format.resource_table, value->links);
  }
  const auto count = a.field.operation == Operation::Container
                         ? value->count
                         : a.format.components;
  if (value->scalar != a.format.scalar ||
      value->bytes.size() !=
          std::uint64_t(CaptureScalarWidth(value->scalar)) * count)
    return Reject(a, d, CaptureFailure::InvalidValue);
  return writer.PutScalar(a.location, value->scalar, count, value->bytes);
}
bool EntitySource::Impl::DisplayOpen(const CaptureFieldAccess &a,
                                     CaptureTraversalSource &out,
                                     CaptureDiagnostic &d) noexcept {
  const auto *value = DisplayValue(a, d);
  if (!value)
    return false;
  const auto count =
      a.field.operation == Operation::Container ? value->count : 1u;
  if (a.field.nested_layout &&
      (value->first_child > facts.display_values.size() ||
       count > facts.display_values.size() - value->first_child))
    return Reject(a, d, CaptureFailure::CountMismatch);
  out = {};
  out.count = count;
  out.identity =
      (std::uint64_t(a.root.address) << 16) | std::uint64_t(a.field.index + 1);
  out.opaque[0] = value->first_child;
  return true;
}
bool EntitySource::Impl::DisplayElement(const CaptureFieldAccess &a,
                                        const CaptureTraversalSource &source,
                                        std::uint16_t index, CaptureRoot &out,
                                        CaptureDiagnostic &d) noexcept {
  const auto at = index == kNullIndex ? 0u : unsigned(index);
  if (at >= source.count || source.opaque[0] > facts.display_values.size() ||
      at >= facts.display_values.size() - source.opaque[0])
    return Reject(a, d, CaptureFailure::CountMismatch);
  const auto &node = facts.display_values[std::size_t(source.opaque[0]) + at];
  const auto *layout = FindLayout(node.layout);
  if (!layout || node.layout != a.field.nested_layout ||
      (!layout->protocol && !node.native))
    return Reject(a, d, CaptureFailure::UnknownLayout);
  out = {a.root.subject,
         node.native ? node.native
                     : std::uint32_t(reinterpret_cast<std::uintptr_t>(&node)),
         layout->native_size, node.layout};
  return true;
}
} // namespace cs::world::binary
