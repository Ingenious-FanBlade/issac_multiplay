#include "state/nested.h"
#include "state/anchors.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cstring>
#include <limits>
#include <utility>

namespace cs::state {
namespace {
static_assert(std::endian::native == std::endian::little, "J273 state requires little endian");
[[noreturn]] void Fail(std::string_view field, std::string_view reason) { throw Gap(field, reason); }
std::uint32_t U32(std::span<const std::byte> bytes, std::size_t offset) {
  std::uint32_t result;
  std::memcpy(&result, bytes.data() + offset, sizeof result);
  return result;
}
// 先检查整数边界和声明上限，再触碰负载；空容器也必须具有合法的头。
std::size_t ElementSize(std::uint32_t begin, std::uint64_t count,
                        std::uint32_t stride, std::uint16_t limit, std::string_view field) {
  if (!stride || !limit || count > limit || count * stride > UINT32_MAX) Fail(field, "container_limit_or_stride");
  const auto size = count * stride;
  if ((!begin && size) || std::uint64_t(begin) + size > UINT32_MAX) Fail(field, "container_address_overflow");
  return static_cast<std::size_t>(size);
}
struct ArrayInfo { std::uint32_t begin, count; };
ArrayInfo VectorHeader(const ReadWindow &window, std::uint32_t header, std::uint32_t stride, std::string_view field) {
  std::array<std::byte, 12> bytes{};
  window.Copy(header, bytes, field);
  const auto begin = U32(bytes, 0), end = U32(bytes, 4), capacity = U32(bytes, 8);
  if (!stride || begin > end || end > capacity || (!begin && (end || capacity)) ||
      (end - begin) % stride || (capacity - begin) % stride) Fail(field, "vector_header_invalid");
  return {begin, (end - begin) / stride};
}
ArrayInfo ArrayHeader(const ReadWindow &window, std::uint32_t header, std::string_view field) {
  std::array<std::byte, 8> bytes{};
  window.Copy(header, bytes, field);
  return {U32(bytes, 0), U32(bytes, 4)};
}
ArrayInfo CountedHeader(const ReadWindow &window, std::uint32_t pointer_field, std::uint32_t count, std::string_view field) {
  std::array<std::byte, 4> bytes{};
  window.Copy(pointer_field, bytes, field);
  return {U32(bytes, 0), count};
}
NativeElements CopyElements(const ReadWindow &window, ArrayInfo info, std::uint32_t stride,
                            std::uint16_t limit, std::string_view field) {
  const auto size = ElementSize(info.begin, info.count, stride, limit, field);
  NativeElements out{static_cast<std::uint16_t>(info.count), stride, Bytes(size)};
  if (size) window.Copy(info.begin, out.bytes, field);
  window.Require(field);
  return out;
}
NativeElementView CopyElementsInto(const ReadWindow &window, ArrayInfo info, std::uint32_t stride,
                                   std::uint16_t limit, std::span<std::byte> storage, std::string_view field) {
  const auto size = ElementSize(info.begin, info.count, stride, limit, field);
  if (size > storage.size()) Fail(field, "container_capture_storage_exhausted");
  const auto bytes = storage.first(size);
  if (size) window.Copy(info.begin, bytes, field);
  window.Require(field);
  return {static_cast<std::uint16_t>(info.count), stride, bytes};
}
struct StringInfo { std::array<std::byte, 24> header; std::uint32_t size, capacity; };
StringInfo StringHeader(const ReadWindow &window, std::uint32_t header, std::uint16_t limit, std::string_view field) {
  StringInfo info{};
  window.Copy(header, info.header, field);
  info.size = U32(info.header, 16);
  info.capacity = U32(info.header, 20);
  if (!limit || info.size > limit || info.size > info.capacity) Fail(field, "string_size_or_limit");
  if (info.capacity < 16) {
    if (info.capacity != 15) Fail(field, "string_sso_capacity");
  } else if (!U32(info.header, 0) || info.capacity == UINT32_MAX) Fail(field, "string_heap_header");
  return info;
}
std::span<const std::byte> CopyStringInto(const ReadWindow &window, const StringInfo &info,
                                         std::span<std::byte> storage, std::string_view field) {
  if (info.size > storage.size()) Fail(field, "string_capture_storage_exhausted");
  const auto bytes = storage.first(info.size);
  if (info.capacity < 16) std::copy_n(info.header.begin(), info.size, bytes.begin());
  else if (info.size) window.Copy(U32(info.header, 0), bytes, field);
  window.Require(field);
  return bytes;
}
void Path(std::string_view path) {
  if (path.empty() || path.size() > 1024 || path.front() == '/' || path.front() == '\\' ||
      path.find(':') != path.npos || path.find('\0') != path.npos ||
      path.find('\r') != path.npos || path.find('\n') != path.npos) Fail("resource", "invalid_path");
  std::size_t first = 0;
  while (first <= path.size()) {
    const auto end = path.find_first_of("/\\", first);
    const auto part = path.substr(first, end == path.npos ? path.size() - first : end - first);
    if (part == "..") Fail("resource", "path_parent_component");
    if (end == path.npos) break;
    first = end + 1;
  }
}
}
Gap::Gap(std::string_view field, std::string_view reason)
    : std::runtime_error(std::string(field) + ":" + std::string(reason)) {}
void ReadWindow::Require(std::string_view field) const {
  if (std::this_thread::get_id() != owner || !generation || !current || !current(context, generation))
    Fail(field, "native_window_expired_or_wrong_thread");
}
void ReadWindow::Copy(std::uint32_t address, std::span<std::byte> bytes, std::string_view field) const {
  Require(field);
  if (!read || !address || bytes.size() > UINT32_MAX || std::uint64_t(address) + bytes.size() > UINT32_MAX)
    Fail(field, "native_read_bounds");
  if (!read(context, address, bytes)) Fail(field, "native_read_failed");
  Require(field);
}
NativeElements ReadVector(const ReadWindow &window, std::uint32_t header, std::uint32_t stride,
                          std::uint16_t limit, std::string_view field) {
  return CopyElements(window, VectorHeader(window, header, stride, field), stride, limit, field);
}
NativeElements ReadArray(const ReadWindow &window, std::uint32_t header, std::uint32_t stride,
                         std::uint16_t limit, std::string_view field) {
  return CopyElements(window, ArrayHeader(window, header, field), stride, limit, field);
}
NativeElements ReadCountedArray(const ReadWindow &window, std::uint32_t pointer_field, std::uint32_t count,
                                std::uint32_t stride, std::uint16_t limit, std::string_view field) {
  return CopyElements(window, CountedHeader(window, pointer_field, count, field), stride, limit, field);
}
Bytes ReadString(const ReadWindow &window, std::uint32_t header, std::uint16_t limit, std::string_view field) {
  const auto info = StringHeader(window, header, limit, field);
  Bytes out(info.size);
  CopyStringInto(window, info, out, field);
  return out;
}
NativeElementView ReadVectorInto(const ReadWindow &window, std::uint32_t header, std::uint32_t stride,
                                 std::uint16_t limit, std::span<std::byte> storage, std::string_view field) {
  return CopyElementsInto(window, VectorHeader(window, header, stride, field), stride, limit, storage, field);
}
NativeElementView ReadArrayInto(const ReadWindow &window, std::uint32_t header, std::uint32_t stride,
                                std::uint16_t limit, std::span<std::byte> storage, std::string_view field) {
  return CopyElementsInto(window, ArrayHeader(window, header, field), stride, limit, storage, field);
}
NativeElementView ReadCountedArrayInto(const ReadWindow &window, std::uint32_t pointer_field, std::uint32_t count,
                                       std::uint32_t stride, std::uint16_t limit, std::span<std::byte> storage, std::string_view field) {
  return CopyElementsInto(window, CountedHeader(window, pointer_field, count, field), stride, limit, storage, field);
}
std::span<const std::byte> ReadStringInto(const ReadWindow &window, std::uint32_t header, std::uint16_t limit,
                                         std::span<std::byte> storage, std::string_view field) {
  return CopyStringInto(window, StringHeader(window, header, limit, field), storage, field);
}
Bytes EncodeElements(std::uint16_t count, std::span<const std::byte> projected,
                     std::size_t max_bytes, std::string_view field) {
  if (max_bytes < 2 || projected.size() > max_bytes - 2 || (!count && !projected.empty()))
    Fail(field, "container_wire_size");
  Bytes out(2 + projected.size());
  EncodeElementsInto(count, projected, out, max_bytes, field);
  return out;
}
std::span<const std::byte> EncodeElementsInto(std::uint16_t count, std::span<const std::byte> projected,
                                             std::span<std::byte> storage, std::size_t max_bytes, std::string_view field) {
  if (max_bytes < 2 || projected.size() > max_bytes - 2 || (!count && !projected.empty()) ||
      storage.size() < 2 || projected.size() > storage.size() - 2) Fail(field, "container_wire_size");
  // 允许 payload 已位于同一采集区，先移动有效负载再写头，避免覆盖尚未移动的输入。
  if (!projected.empty()) std::memmove(storage.data() + 2, projected.data(), projected.size());
  std::memcpy(storage.data(), &count, 2);
  return storage.first(2 + projected.size());
}
ElementPayload DecodeElements(std::span<const std::byte> bytes, std::uint16_t limit,
                              std::uint32_t width, std::size_t max_bytes, std::string_view field) {
  if (bytes.size() < 2 || bytes.size() > max_bytes || !limit) Fail(field, "container_wire_bounds");
  std::uint16_t count;
  std::memcpy(&count, bytes.data(), 2);
  if (count > limit || (!count && bytes.size() != 2) ||
      (width && std::uint64_t(count) * width != bytes.size() - 2)) Fail(field, "container_wire_count");
  return {count, bytes.subspan(2)};
}
void RebuildContainer(const ReadWindow &window, ContainerBuilder &builder,
                      std::span<const std::span<const std::byte>> elements,
                      std::uint16_t limit, std::string_view field) {
  window.Require(field);
  if (!limit || elements.size() > limit) Fail(field, "container_rebuild_limit");
  try {
    if (!builder.Begin(static_cast<std::uint16_t>(elements.size()))) Fail(field, "container_prepare_failed");
    for (std::size_t i = 0; i < elements.size(); ++i) {
      window.Require(field);
      if (!builder.Element(static_cast<std::uint16_t>(i), elements[i])) Fail(field, "container_element_failed");
    }
    window.Require(field);
    if (!builder.Commit()) Fail(field, "container_commit_failed");
  } catch (...) {
    builder.Abort();
    throw;
  }
}
std::uint16_t ResolveReference(std::uint32_t address, ReferenceResolver resolver, std::string_view field) {
  if (!address) return kNullIndex;
  std::uint16_t result = kNullIndex;
  if (!resolver.lookup || !resolver.lookup(resolver.context, address, result) || result == kNullIndex)
    Fail(field, "reference_not_in_membership");
  return result;
}
CapturedReference CaptureReference(std::uint32_t address, ReferenceCapture capture, std::string_view field) {
  if (!address) return {};
  if (!capture.identify_or_reserve) Fail(field, "reference_capture_owner_missing");
  const auto result = capture.identify_or_reserve(capture.context, address);
  if (!result.handle || (result.kind != CapturedReferenceKind::Identity &&
                         result.kind != CapturedReferenceKind::Pending))
    Fail(field, "reference_identity_or_reservation_missing");
  return result;
}
ResourceTable::ResourceTable(std::uint64_t epoch) : epoch_(epoch) {
  if (!epoch) Fail("resource", "zero_epoch");
}
std::uint16_t ResourceTable::Intern(std::string_view path) {
  if (path.empty()) return kNullIndex;
  Path(path);
  const auto found = index_.find(path);
  if (found != index_.end()) return found->second;
  if (paths_.size() >= kNullIndex) Fail("resource", "table_full");
  const auto id = static_cast<std::uint16_t>(paths_.size());
  std::string owned(path); // 输入可以是本表字符串的子视图，先取得独立所有权再扩容。
  paths_.push_back(std::move(owned));
  try { index_.emplace(paths_.back(), id); }
  catch (...) { paths_.pop_back(); throw; }
  return id;
}
void ResourceTable::Install(std::uint64_t epoch, std::uint16_t first, std::span<const std::string> paths) {
  if (epoch != epoch_ || first > paths_.size() || paths.size() > std::size_t(kNullIndex) - first)
    Fail("resource", "epoch_or_sequence");
  auto next = paths_;
  auto next_index = index_;
  for (std::size_t i = 0; i < paths.size(); ++i) {
    Path(paths[i]);
    const auto index = first + i;
    if (index < next.size()) {
      if (next[index] != paths[i]) Fail("resource", "id_rebound");
    } else {
      if (next_index.find(paths[i]) != next_index.end()) Fail("resource", "duplicate_path_id");
      next.push_back(paths[i]);
      next_index.emplace(paths[i], static_cast<std::uint16_t>(index));
    }
  }
  paths_.swap(next);
  index_.swap(next_index);
}
std::string_view ResourceTable::At(std::uint16_t id) const {
  if (id == kNullIndex) return {};
  if (id >= paths_.size()) Fail("resource", "id_not_installed");
  return paths_[id];
}
}
