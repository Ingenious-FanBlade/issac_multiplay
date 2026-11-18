#pragma once
#include "state/flat_capture.h"
#include <optional>

namespace cs::state {
using FlatVersions=std::array<std::uint32_t,2>;
inline constexpr std::uint32_t kFlatMaxVersion=0x7fffffffu;

// 同一布局的完整镜像只在编码器来源快照中共享。主区 32 字节对齐，附加段按实际长度比较。
struct HostFlatImage {
  struct alignas(32) Block {std::byte bytes[32];};
  memory::Lease memory;
  std::vector<Block> storage;
  std::uint32_t size=0;
  std::vector<FlatTailSpan> tails;
  std::vector<std::uint32_t> stamps;
  FlatVersions newest{};
  std::span<const std::byte> Bytes() const noexcept {return std::as_bytes(std::span(storage)).first(size);}
};
using HostFlatImageRef=std::shared_ptr<const HostFlatImage>;

// 字节差异位图和字段位图分离。客户端后台可复用定长区比较。
bool MarkFlatMainChanges(const FlatLayout &,std::span<const std::byte> before,std::span<const std::byte> after,
                         std::span<std::uint32_t> byte_bits,std::span<std::uint32_t> field_bits) noexcept;
struct FlatCompareScratch {
  memory::Lease memory;
  std::vector<std::uint32_t> byte_bits,field_bits;
  std::vector<FlatTailSpan> tails;
  void Prepare(const FlatLayout &);
};
HostFlatImageRef UpdateFlatImage(const FlatLayout &,std::span<const std::byte>,std::span<const FlatTailSpan>,
                                const HostFlatImageRef &previous,FlatVersions,FlatCompareScratch &,
                                const CaptureRoot &,CaptureDiagnostic &);

// 每 Peer 每次组帧借用同一选择工作区，返回值只持有本条待发正文。
struct FlatEncodeScratch {
  memory::Lease memory;
  std::vector<std::uint32_t> field_bits;
  std::vector<std::uint16_t> groups;
  std::vector<unsigned char> top;
  void Prepare(const FlatLayout &);
};
struct FlatEncodedRecord {
  FlatSetMode mode;
  std::vector<std::byte> bytes;
};
std::optional<FlatEncodedRecord> EncodeFlatRecord(const FlatLayout &,const HostFlatImage &,FlatStream,
                                                 std::uint32_t confirmed,bool all,FlatEncodeScratch &,memory::Lease &payload_memory);
}
