#pragma once
#include <cstddef>
#include <cstdint>

namespace cs::world {
// 只提供页范围与窄 SEH；调用者仍需核定线程、对象范围和读取寿命。
struct NativeReadRegion {
  std::uint32_t first = 0;
  std::uint64_t end = 0;
  bool Contains(std::uint64_t address) const noexcept {
    return address >= first && address < end;
  }
};
inline constexpr std::uint64_t kNativeAddressLimit = std::uint64_t{1} << 32;
bool QueryNativeReadRegion(std::uint64_t address, NativeReadRegion &out) noexcept;
bool CopyNativeReadBytes(void *out, const void *in, std::size_t size) noexcept;
// 返回 0 或窄 SEH 捕获的原生读取异常码。
unsigned CopyNativeReadBytesFault(void *out, const void *in, std::size_t size) noexcept;
int NativeReadExceptionFilter(unsigned code) noexcept;
}
