#pragma once
#include <cstddef>
#include <cstdint>

namespace cs::state {
// 差异和序号选择均输出 u32 位图，bit 0 对应每组第一个字节／字段。
struct FlatSimd {
  bool (*equal)(const unsigned char *,const unsigned char *,std::size_t) noexcept;
  void (*different)(const unsigned char *,const unsigned char *,std::size_t padded_bytes,std::uint32_t *) noexcept;
  void (*newer)(const std::uint32_t *,std::size_t count,std::uint32_t ack,std::uint32_t *) noexcept;
  void (*groups)(const std::uint16_t *,std::size_t count,unsigned char *top) noexcept;
  unsigned (*popcount)(std::uint32_t) noexcept;
  bool avx2_available,uses_avx2,has_popcnt;
};

// 进程第一次调用时检测；启动代码可显式为本进程选择 SSE2，后续两条线程共享表。
const FlatSimd &FlatKernels(bool force_sse2=false) noexcept;
}
