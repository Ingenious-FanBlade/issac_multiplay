#include <immintrin.h>
#include <stddef.h>
#include <stdint.h>

// 此翻译单元独占 /arch:AVX2，入口由基线代码检测后调用；保留普通 C ABI。
extern "C" bool FlatAvxEqual(const unsigned char *a,const unsigned char *b,size_t size) noexcept {
  auto changed=_mm256_setzero_si256();
  while(size>=32){
    const auto x=_mm256_loadu_si256(reinterpret_cast<const __m256i *>(a));
    const auto y=_mm256_loadu_si256(reinterpret_cast<const __m256i *>(b));
    changed=_mm256_or_si256(changed,_mm256_xor_si256(x,y));
    a+=32;b+=32;size-=32;
  }
  if(!_mm256_testz_si256(changed,changed))return false;
  for(size_t i=0;i<size;++i)if(a[i]!=b[i])return false;
  return true;
}

extern "C" void FlatAvxDifferent(const unsigned char *a,const unsigned char *b,size_t size,uint32_t *out) noexcept {
  for(size_t i=0;i<size;i+=32){
    const auto x=_mm256_loadu_si256(reinterpret_cast<const __m256i *>(a+i));
    const auto y=_mm256_loadu_si256(reinterpret_cast<const __m256i *>(b+i));
    out[i/32]=~uint32_t(_mm256_movemask_epi8(_mm256_cmpeq_epi8(x,y)));
  }
}

extern "C" void FlatAvxNewer(const uint32_t *stamps,size_t count,uint32_t ack,uint32_t *out) noexcept {
  const auto base=_mm256_set1_epi32(int(ack));
  for(size_t start=0;start<count;start+=32){
    uint32_t word=0;
    size_t i=count-start<32?count:start+32;
    while((i-start)%8){
      --i;
      word=(word<<1)|uint32_t(stamps[i]>ack);
    }
    while(i>start){
      i-=8;
      const auto v=_mm256_loadu_si256(reinterpret_cast<const __m256i *>(stamps+i));
      const auto set=_mm256_movemask_ps(_mm256_castsi256_ps(_mm256_cmpgt_epi32(v,base)));
      word=(word<<8)|uint32_t(set);
    }
    out[start/32]=word;
  }
}

extern "C" void FlatAvxGroups(const uint16_t *words,size_t count,unsigned char *out) noexcept {
  const auto zero=_mm256_setzero_si256();
  while(count>=16){
    const auto v=_mm256_loadu_si256(reinterpret_cast<const __m256i *>(words));
    auto bits=~uint32_t(_mm256_movemask_epi8(_mm256_cmpeq_epi16(v,zero)));
    // movemask 每组有相邻两位，压成一位并保留原组顺序。
    bits&=0x55555555u;
    bits=(bits|(bits>>1))&0x33333333u;
    bits=(bits|(bits>>2))&0x0f0f0f0fu;
    bits=(bits|(bits>>4))&0x00ff00ffu;
    bits=(bits|(bits>>8))&0x0000ffffu;
    out[0]=static_cast<unsigned char>(bits);
    out[1]=static_cast<unsigned char>(bits>>8);
    out+=2;words+=16;count-=16;
  }
  for(size_t start=0;start<count;start+=8){
    unsigned set=0;
    size_t i=count-start<8?count:start+8;
    while(i>start)set=(set<<1)|unsigned(words[--i]!=0);
    out[start/8]=static_cast<unsigned char>(set);
  }
}
