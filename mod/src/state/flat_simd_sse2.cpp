#include "state/flat_simd.h"
#include <intrin.h>
#include <emmintrin.h>

extern "C" {
bool FlatAvxEqual(const unsigned char *,const unsigned char *,std::size_t) noexcept;
void FlatAvxDifferent(const unsigned char *,const unsigned char *,std::size_t,std::uint32_t *) noexcept;
void FlatAvxNewer(const std::uint32_t *,std::size_t,std::uint32_t,std::uint32_t *) noexcept;
void FlatAvxGroups(const std::uint16_t *,std::size_t,unsigned char *) noexcept;
}

namespace cs::state {
namespace {
// 主区使用完整向量；附加段比较还要处理最后不足 16 字节的实际长度。
bool Equal(const unsigned char *a,const unsigned char *b,std::size_t size) noexcept {
  auto changed=_mm_setzero_si128();
  while(size>=16){
    const auto x=_mm_loadu_si128(reinterpret_cast<const __m128i *>(a));
    const auto y=_mm_loadu_si128(reinterpret_cast<const __m128i *>(b));
    changed=_mm_or_si128(changed,_mm_xor_si128(x,y));
    a+=16;b+=16;size-=16;
  }
  if(_mm_movemask_epi8(_mm_cmpeq_epi8(changed,_mm_setzero_si128()))!=0xffff)return false;
  for(std::size_t i=0;i<size;++i)if(a[i]!=b[i])return false;
  return true;
}

void Different(const unsigned char *a,const unsigned char *b,std::size_t size,std::uint32_t *out) noexcept {
  for(std::size_t i=0;i<size;i+=32){
    const auto x0=_mm_loadu_si128(reinterpret_cast<const __m128i *>(a+i));
    const auto y0=_mm_loadu_si128(reinterpret_cast<const __m128i *>(b+i));
    const auto x1=_mm_loadu_si128(reinterpret_cast<const __m128i *>(a+i+16));
    const auto y1=_mm_loadu_si128(reinterpret_cast<const __m128i *>(b+i+16));
    const auto lo=std::uint32_t(_mm_movemask_epi8(_mm_cmpeq_epi8(x0,y0)));
    const auto hi=std::uint32_t(_mm_movemask_epi8(_mm_cmpeq_epi8(x1,y1)));
    out[i/32]=~(lo|(hi<<16));
  }
}

void Newer(const std::uint32_t *stamps,std::size_t count,std::uint32_t ack,std::uint32_t *out) noexcept {
  const auto base=_mm_set1_epi32(int(ack));
  for(std::size_t start=0;start<count;start+=32){
    std::uint32_t word=0;
    std::size_t i=start;
    for(;i+4<=count && i<start+32;i+=4){
      const auto v=_mm_loadu_si128(reinterpret_cast<const __m128i *>(stamps+i));
      const auto set=_mm_movemask_ps(_mm_castsi128_ps(_mm_cmpgt_epi32(v,base)));
      word|=std::uint32_t(set)<<(i-start);
    }
    for(;i<count && i<start+32;++i)if(stamps[i]>ack)word|=std::uint32_t{1}<<(i-start);
    out[start/32]=word;
  }
}

void Groups(const std::uint16_t *words,std::size_t count,unsigned char *out) noexcept {
  const auto zero=_mm_setzero_si128();
  while(count>=8){
    const auto values=_mm_loadu_si128(reinterpret_cast<const __m128i *>(words));
    const auto empty=_mm_cmpeq_epi16(values,zero);
    *out++=static_cast<unsigned char>(~_mm_movemask_epi8(_mm_packs_epi16(empty,zero)));
    words+=8;count-=8;
  }
  if(count){
    unsigned set=0;
    for(std::size_t i=0;i<count;++i)if(words[i])set|=1u<<i;
    *out=static_cast<unsigned char>(set);
  }
}

unsigned CountSoftware(std::uint32_t value) noexcept {
  constexpr unsigned nibble[]{0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4};
  unsigned count=0;
  for(unsigned i=0;i<32;i+=4)count+=nibble[(value>>i)&15];
  return count;
}

unsigned CountHardware(std::uint32_t value) noexcept {return __popcnt(value);}

FlatSimd Detect(bool force_sse2) noexcept {
  int cpu[4]{};
  __cpuid(cpu,0);
  const auto maximum=cpu[0];
  __cpuid(cpu,1);
  const auto ecx=unsigned(cpu[2]);
  const bool popcnt=(ecx&(1u<<23))!=0;
  const auto avx_os=(1u<<27)|(1u<<28);
  bool avx2=false;
  // 仅 OSXSAVE+AVX 后执行 XGETBV；XMM 与 YMM 两位都要被操作系统保存。
  if(maximum>=7 && (ecx&avx_os)==avx_os && (_xgetbv(0)&6u)==6u){
    __cpuidex(cpu,7,0);
    avx2=(unsigned(cpu[1])&(1u<<5))!=0;
  }
  const bool use_avx2=avx2&&!force_sse2;
  return {use_avx2?FlatAvxEqual:Equal,use_avx2?FlatAvxDifferent:Different,use_avx2?FlatAvxNewer:Newer,
    use_avx2?FlatAvxGroups:Groups,popcnt?CountHardware:CountSoftware,avx2,use_avx2,popcnt};
}
}

const FlatSimd &FlatKernels(bool force_sse2) noexcept {
  static const auto kernels=Detect(force_sse2);
  return kernels;
}
}
