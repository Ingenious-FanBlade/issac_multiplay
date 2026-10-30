#include "world/common/native_source.h"
#include "binding/world_layout.h"
namespace cs::world {
Class ClassifyNative(const NativeSource &source,ObjectDomain domain, std::uint32_t address,
                          std::uint32_t &vptr, bool &base_known) noexcept {
  const auto Image=[&](std::uint32_t address,std::size_t bytes) noexcept { return address>=source.image_base && bytes<=source.image_size && address-source.image_base<=source.image_size-bytes; };
  base_known = false;
  if ((address & 3u) || !source.Value(address, vptr) || vptr < 4 || (vptr & 3u) ||
      !Image(vptr - 4, 8))
    return Class::Unknown;
  std::uint32_t locator = 0, col[5]{}, chd[4]{};
  const auto &c = layout::LayoutConstants();
  if (!source.Value(vptr - 4, locator) || !Image(locator, sizeof(col)) ||
      !source.Read(locator, col, sizeof(col)) || col[0] != c.rtti_expected_signature ||
      col[1] || col[2] || !Image(col[3], 8) || !Image(col[4], sizeof(chd)) ||
      !source.Read(col[4], chd, sizeof(chd)) || chd[0] != c.rtti_expected_signature ||
      chd[1] != c.rtti_expected_hierarchy_attributes || !chd[2] ||
      chd[2] > 16 || !Image(chd[3], chd[2] * 4))
    return Class::Unknown;
  std::size_t count = 0;
  const auto *classes = layout::Classes(count);
  const auto root = domain == ObjectDomain::Grid ? layout::RttiClass::Grid
                                                 : layout::RttiClass::Entity;
  const layout::ClassSpec *exact = nullptr, *root_spec = nullptr;
  for (std::size_t i = 0; i < count; ++i) {
    if (classes[i].rtti == root)
      root_spec = &classes[i];
    if (col[3] - source.image_base == classes[i].td_rva)
      exact = &classes[i];
  }
  if (!root_spec)
    return Class::Unknown;
  bool found_root = false;
  for (std::uint32_t i = 0; i < chd[2]; ++i) {
    std::uint32_t pointer = 0, bcd[6]{};
    if (!source.Value(std::uint64_t(chd[3]) + i * 4, pointer) ||
        !Image(pointer, sizeof(bcd)) || !source.Read(pointer, bcd, sizeof(bcd)) ||
        !Image(bcd[0], 8) || bcd[1] >= chd[2] || bcd[2] ||
        bcd[3] != UINT32_MAX || bcd[4] ||
        bcd[5] != c.rtti_expected_base_attributes || (!i && bcd[0] != col[3]))
      return Class::Unknown;
    if (bcd[0] - source.image_base == root_spec->td_rva)
      found_root = true;
    if (exact && (chd[2] != exact->base_count ||
                  bcd[0] - source.image_base != exact->bases[i].td_rva))
      return Class::Unknown;
  }
  base_known = found_root;
  if (!found_root || !exact)
    return Class::Unknown;
  if (domain == ObjectDomain::Grid)
    return Class::Grid;
  return exact->schema ? exact->schema_class : Class::Unknown;
}

}
