#pragma once
#include "state/capture_values.h"
#include "state/flat_layout.h"
#include <cstring>

namespace cs::state {
// 镜像槽不保证自然对齐。主线程句柄为目录内的 1-based 序号，后台原位换成表内编号。
inline constexpr std::uint32_t kFlatOwnerPresent=1u<<31;
inline constexpr unsigned kFlatMaxDepth=32;

// 完整镜像内的附加段范围。偏移只供线程内访问，字段上线只带规范头及此段字节。
struct FlatTailSpan {
  std::uint32_t offset=0,size=0;
};

inline std::uint32_t FlatWord(std::span<const std::byte> slot,std::size_t offset=0) noexcept {
  std::uint32_t value;
  std::memcpy(&value,slot.data()+offset,sizeof(value));
  return value;
}

inline void SetFlatWord(std::span<std::byte> slot,std::uint32_t value,std::size_t offset=0) noexcept {
  std::memcpy(slot.data()+offset,&value,sizeof(value));
}

// 按同一补丁程序遍历纯值镜像。output 由调用方独占，失败时整根丢弃。
bool TranslateFlatCapture(const CapturePacket &,const CaptureRecord &,CaptureLinkResolver,
                          std::span<std::byte> output,std::span<FlatTailSpan> tails,CaptureDiagnostic &) noexcept;
}
