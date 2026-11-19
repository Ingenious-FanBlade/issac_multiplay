#pragma once
#include "state/capture_values.h"
#include "state/flat_layout.h"
#include <algorithm>
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

// 容器模板的计数和字节长度在两端共用。字符串的短值留在槽内。
struct FlatContainerHead {std::uint32_t count=0,bytes=0;};
inline FlatContainerHead FlatContainer(std::span<const std::byte> slot,FlatHeader header) noexcept {
  switch(header){
    case FlatHeader::String:{const auto size=FlatWord(slot,16);return {size,size>15?size:0u};}
    case FlatHeader::Deque:return {FlatWord(slot,16),FlatWord(slot,4)};
    case FlatHeader::Tree:case FlatHeader::LayerArray:return {FlatWord(slot,4),FlatWord(slot)};
    default:return {FlatWord(slot),FlatWord(slot,4)};
  }
}
inline void SetFlatContainer(std::span<std::byte> slot,FlatHeader header,FlatContainerHead head) noexcept {
  std::fill(slot.begin(),slot.end(),std::byte{});
  if(header==FlatHeader::Deque){SetFlatWord(slot,head.count,16);SetFlatWord(slot,head.bytes,4);}
  else if(header==FlatHeader::Tree||header==FlatHeader::LayerArray){
    SetFlatWord(slot,head.count,4);SetFlatWord(slot,head.bytes);
  }else{
    SetFlatWord(slot,head.count);SetFlatWord(slot,head.bytes,4);
  }
}
inline bool FlatHasStream(const FlatLayout &layout,unsigned stream) noexcept {
  for(auto word:layout.stream_fields[stream])if(word)return true;
  return false;
}
inline bool FlatFieldInStream(const FlatLayout &layout,std::uint16_t field,unsigned stream) noexcept {
  return field==kNullIndex||((layout.stream_fields[stream][field/32]>>(field%32))&1u)!=0;
}

// 按同一补丁程序遍历纯值镜像。output 由调用方独占，失败时整根丢弃。
bool TranslateFlatCapture(const CapturePacket &,const CaptureRecord &,CaptureLinkResolver,
                          std::span<std::byte> output,std::span<FlatTailSpan> tails,CaptureDiagnostic &) noexcept;
// 快槽只重编号包内引用句柄。资源仍沿用当前普通目录的句柄，留后台解析。
bool RemapFlatCaptureReferences(const CaptureRecord &,std::span<std::byte>,CaptureLinkResolver,CaptureDiagnostic &) noexcept;
}
