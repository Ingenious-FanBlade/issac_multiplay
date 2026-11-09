#pragma once
#include "state/layout.h"
#include <cstddef>
#include <cstdint>
#include <map>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace cs::state {
using Bytes = std::vector<std::byte>;
// 缺口携带字段路径，保持空值、无引用、损坏容器与读取失败的区别。
class Gap : public std::runtime_error {
public:
  Gap(std::string_view field, std::string_view reason);
};
// 原生读取只借用合法主线程窗口；读取回调不得保存目标指针或转交后台。
// 第 10 步接入实际 ordinary/PlayerPost token；本层不创建窗口或解除生命周期限制。
struct ReadWindow {
  std::thread::id owner;
  std::uint64_t generation;
  void *context;
  bool (*current)(void *, std::uint64_t) noexcept;
  bool (*read)(void *, std::uint32_t, std::span<std::byte>) noexcept;
  void Require(std::string_view field) const;
  void Copy(std::uint32_t address, std::span<std::byte> bytes, std::string_view field) const;
};
// 返回自有临时元素区，复杂元素仍需按生成表递归投影；严禁直接发送这些原生字节。
struct NativeElements { std::uint16_t count; std::uint32_t stride; Bytes bytes; };
// 正式热路径写入调用方预分配的采集区，不为每个对象或容器分配 vector。
struct NativeElementView { std::uint16_t count; std::uint32_t stride; std::span<const std::byte> bytes; };
NativeElementView ReadVectorInto(const ReadWindow &, std::uint32_t header, std::uint32_t stride,
                                 std::uint16_t limit, std::span<std::byte> storage, std::string_view field);
// J273 Release deque 为20B头、16B元素块；按逻辑顺序归并，保留空与损坏的区别。
// EntityPtr元素仍是当前栈的原生暂存，必须立即转为CapturedReference，不能交给后台。
NativeElementView ReadDequeInto(const ReadWindow &, std::uint32_t header, std::uint32_t stride,
                                std::uint16_t limit, std::span<std::byte> storage, std::string_view field);
NativeElementView ReadArrayInto(const ReadWindow &, std::uint32_t pointer_and_count, std::uint32_t stride,
                                std::uint16_t limit, std::span<std::byte> storage, std::string_view field);
NativeElementView ReadCountedArrayInto(const ReadWindow &, std::uint32_t pointer_field, std::uint32_t count,
                                       std::uint32_t stride, std::uint16_t limit, std::span<std::byte> storage, std::string_view field);
std::span<const std::byte> ReadStringInto(const ReadWindow &, std::uint32_t header, std::uint16_t limit,
                                         std::span<std::byte> storage, std::string_view field);
// Text内联字节串仅扫描至首个零，再复读该前缀（含零）；不读取、编码或比较尾部旧槽数据。
// 调用者须先持有生成字段要求的本use写入见证；本函数只检查窗口和有界字节，不补造见证。
std::span<const std::byte> ReadCStringInto(const ReadWindow &, std::uint32_t address, std::uint16_t capacity,
                                          std::span<std::byte> storage, std::string_view field);
// 自有分配包装用于冷路径；正式逐帧采集应使用上面的 Into 接口。
NativeElements ReadVector(const ReadWindow &, std::uint32_t header, std::uint32_t stride,
                          std::uint16_t limit, std::string_view field);
NativeElements ReadArray(const ReadWindow &, std::uint32_t pointer_and_count,
                         std::uint32_t stride, std::uint16_t limit, std::string_view field);
// AnimationState 的计数来自当前 AnimationData；调用者须在同一合法窗口取得该计数。
NativeElements ReadCountedArray(const ReadWindow &, std::uint32_t pointer_field, std::uint32_t count,
                                std::uint32_t stride, std::uint16_t limit, std::string_view field);
Bytes ReadString(const ReadWindow &, std::uint32_t header, std::uint16_t limit, std::string_view field);
// 只接受已递归投影的元素；容器线上头固定为 little-endian u16，不附结尾零。
Bytes EncodeElements(std::uint16_t count, std::span<const std::byte> projected,
                     std::size_t max_bytes, std::string_view field);
std::span<const std::byte> EncodeElementsInto(std::uint16_t count, std::span<const std::byte> projected,
                                             std::span<std::byte> storage, std::size_t max_bytes, std::string_view field);
struct ElementPayload { std::uint16_t count; std::span<const std::byte> bytes; };
ElementPayload DecodeElements(std::span<const std::byte>, std::uint16_t limit,
                              std::uint32_t fixed_element_size, std::size_t max_bytes, std::string_view field);
// 线上仍为u16长度+原始前缀，不猜UTF-8；拒绝内嵌零。Client暂存时另补终止符，后续owner只安装此有效区。
std::span<const std::byte> StageCStringInto(std::span<const std::byte> encoded, std::uint16_t capacity,
                                           std::span<std::byte> storage, std::string_view field);

// Client 通过具名原生 owner 建立临时容器，全部成功才交换到目标；实现不得 memcpy 容器头。
// 回调对象只在当前主线程栈使用。分配/构造/销毁策略由第 13 步的各原生 owner 提供。
class ContainerBuilder {
public:
  virtual ~ContainerBuilder() = default;
  virtual bool Begin(std::uint16_t count) = 0;
  virtual bool Element(std::uint16_t index, std::span<const std::byte> state) = 0;
  virtual bool Commit() = 0;
  virtual void Abort() noexcept = 0;
};
void RebuildContainer(const ReadWindow &, ContainerBuilder &, std::span<const std::span<const std::byte>> elements,
                      std::uint16_t limit, std::string_view field);

// 身份转换仅借用当前成员表，不把 uint32 原生地址保存在状态缓冲中。
struct ReferenceResolver {
  void *context;
  bool (*lookup)(void *, std::uint32_t, std::uint16_t &) noexcept;
};
std::uint16_t ResolveReference(std::uint32_t address, ReferenceResolver, std::string_view field);

// Host采集的是当前代的纯值身份句柄，成员序号只在后台组帧时分配。
// Pending表示同窗新实体的待登记预约；句柄不可为0，不能用裸地址充当句柄。
enum class CapturedReferenceKind : std::uint8_t { Null, Identity, Pending, Invalid };
struct CapturedReference {
  CapturedReferenceKind kind = CapturedReferenceKind::Null;
  std::uint64_t handle = 0;
};
struct ReferenceCapture {
  void *context;
  // owner在合法主线程窗口查询或预约身份，返回同代句柄；不把原生地址交给后台。
  CapturedReference (*identify_or_reserve)(void *, std::uint32_t) noexcept;
};
CapturedReference CaptureReference(std::uint32_t address, ReferenceCapture, std::string_view field);

// 资源号在一个 epoch 内只追加，0xFFFF 保留为空；已分配的号永不改指向其他路径。
// 可靠资源发布及帧依赖检查由第 9/11/12 步接线；本层提供原子安装和有界查找。
// 每张表由单线程 owner 持有；跨线程交接复制纯值更新，不能共享会被追加失效的视图。
class ResourceTable {
public:
  explicit ResourceTable(std::uint64_t epoch);
  std::uint64_t Epoch() const noexcept { return epoch_; }
  std::uint16_t Intern(std::string_view path);
  void Install(std::uint64_t epoch, std::uint16_t first, std::span<const std::string> paths);
  std::string_view At(std::uint16_t id) const;
  std::span<const std::string> Paths() const noexcept { return paths_; }
private:
  std::uint64_t epoch_;
  std::vector<std::string> paths_;
  std::map<std::string, std::uint16_t, std::less<>> index_;
};
}
