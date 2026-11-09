#include "state/entity_extension.h"
#include <limits>

namespace cs::state {
namespace {
// 地址只在当前 owner 线程栈内借用；ReadWindow 负责实际读取和 generation 见证。
std::uint32_t Address(std::uint32_t base, std::uint32_t offset, std::string_view field) {
  if (!base || base > (std::numeric_limits<std::uint32_t>::max)() - offset)
    throw Gap(field, "entity_plus_address");
  return base + offset;
}
// 单个原生值只经有界ReadWindow复制，不直接解引用游戏地址。
template<class T> T Read(const ReadWindow &window, std::uint32_t base,
                         std::uint32_t offset, std::string_view field) {
  T value{};
  window.Copy(Address(base, offset, field), std::as_writable_bytes(std::span<T>(&value, 1)), field);
  return value;
}
// 两次读取的来源链只在栈内比较，状态结果中不保留任何原生地址。
struct Source {
  std::uint16_t upper, lower;
  std::uint32_t holder, data;
};
// 只借用已经由官方owner建立的holder/data，缺失时报缺口，不触发分配或初始化。
Source Existing(const ReadWindow &window, std::uint32_t entity, std::string_view field) {
  Source source{};
  source.upper = Read<std::uint16_t>(window, entity, 0x18e, field);
  source.lower = Read<std::uint16_t>(window, entity, 0x192, field);
  source.holder = (std::uint32_t(source.upper) << 16) | source.lower;
  if (!source.holder) throw Gap(field, "entity_plus_holder_missing");
  source.data = Read<std::uint32_t>(window, source.holder, 0, field);
  if (!source.data) throw Gap(field, "entity_plus_data_missing");
  return source;
}
bool SameSource(const Source &a, const Source &b) noexcept {
  return a.upper == b.upper && a.lower == b.lower && a.holder == b.holder && a.data == b.data;
}
// 共用叶子读取只使用本次已核data；派生读取不再次遍历holder链。
EntityExtensionState Common(const ReadWindow &window, std::uint32_t data, std::string_view field) {
  EntityExtensionState result;
  // MSVC Win32 Release optional<bool>：value +4，engaged +5；未设置时不读取 value。
  const auto flying_set = Read<std::uint8_t>(window, data, 5, field);
  if (flying_set > 1) throw Gap(field, "entity_plus_flying_engaged");
  if (flying_set) {
    const auto value = Read<std::uint8_t>(window, data, 4, field);
    if (value > 1) throw Gap(field, "entity_plus_flying_value");
    result.flying = static_cast<std::int8_t>(value);
  }
  // optional<uint32_t>：value +8，engaged +12；未设置时负载与内部填隙均不复制。
  const auto water_set = Read<std::uint8_t>(window, data, 12, field);
  if (water_set > 1) throw Gap(field, "entity_plus_water_engaged");
  if (water_set) result.water = (std::uint64_t{1} << 32) | Read<std::uint32_t>(window, data, 8, field);
  return result;
}
}

EntityExtensionState ReadEntityExtension(const ReadWindow &window, std::uint32_t entity,
                                         std::string_view field) {
  window.Require(field);
  const auto before = Existing(window, entity, field);
  const auto result = Common(window, before.data, field);
  const auto after = Existing(window, entity, field);
  if (!SameSource(before, after))
    throw Gap(field, "entity_plus_source_changed");
  window.Require(field);
  return result;
}

std::optional<bool> DecodeFlyingOverride(std::int8_t value, std::string_view field) {
  if (value == -1) return std::nullopt;
  if (value == 0 || value == 1) return value != 0;
  throw Gap(field, "entity_plus_flying_encoding");
}
std::optional<std::uint32_t> DecodeWaterOverride(std::uint64_t value, std::string_view field) {
  if (!value) return std::nullopt;
  if ((value >> 32) != 1) throw Gap(field, "entity_plus_water_encoding");
  return static_cast<std::uint32_t>(value);
}
}
