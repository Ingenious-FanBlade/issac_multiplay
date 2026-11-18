#include "state/flat_encode.h"
#include "state/flat_simd.h"
#include <algorithm>
#include <intrin.h>

namespace cs::state {
namespace {
const unsigned char *U8(std::span<const std::byte> bytes) noexcept {
  return reinterpret_cast<const unsigned char *>(bytes.data());
}

// 位图从低位向高位消费，调用方天然得到严格递增的字段序号。
template<class Visit>
void SetBits(std::span<const std::uint32_t> bits,Visit visit) {
  for(std::size_t i=0;i<bits.size();++i){
    auto word=bits[i];
    unsigned long bit;
    while(_BitScanForward(&bit,word)){
      visit(i*32+bit);
      word&=word-1;
    }
  }
}

std::span<const std::byte> Tail(std::span<const std::byte> bytes,const FlatTailSpan &range) noexcept {
  return bytes.subspan(range.offset,range.size);
}

unsigned LebSize(std::size_t value) noexcept {
  unsigned size=1;
  while(value>=128){value>>=7;++size;}
  return size;
}
void PutLeb(std::vector<std::byte> &out,std::size_t value) {
  while(value>=128){
    out.push_back(std::byte((value&127)|128));
    value>>=7;
  }
  out.push_back(std::byte(value));
}
}

bool MarkFlatMainChanges(const FlatLayout &layout,std::span<const std::byte> before,std::span<const std::byte> after,
    std::span<std::uint32_t> bytes,std::span<std::uint32_t> fields) noexcept {
  const auto size=layout.mirror_size;
  if(size%32 || before.size()!=size || after.size()!=size || layout.byte_fields.size()!=size ||
     bytes.size()<size/32 || fields.size()<(layout.fields.size()+31)/32)return false;
  const auto &simd=FlatKernels();
  if(simd.equal(U8(before),U8(after),size))return true;
  simd.different(U8(before),U8(after),size,bytes.data());
  for(std::size_t i=0;i<size/32;++i){
    auto word=bytes[i];
    unsigned long bit;
    while(_BitScanForward(&bit,word)){
      const auto ordinal=layout.byte_fields[i*32+bit];
      if(ordinal==kNullIndex)return false;
      const auto &field=layout.fields[ordinal];
      fields[ordinal/32]|=1u<<(ordinal%32);
      const auto end=field.offset+field.size-i*32;
      // 位图剩下的候选都比 bit 大，直接消去本字段在该字内的其余字节。
      word=end>=32?0:word & (~std::uint32_t{0}<<end);
    }
  }
  return true;
}

void FlatCompareScratch::Prepare(const FlatLayout &layout) {
  const auto byte_words=layout.mirror_size/32;
  const auto field_words=(layout.fields.size()+31)/32;
  const auto needed=std::uint64_t((std::max<std::size_t>)(byte_words,byte_bits.capacity())+
    (std::max)(field_words,field_bits.capacity()))*4+
    (std::max)(layout.tail_fields.size(),tails.capacity())*sizeof(FlatTailSpan);
  if(!memory)memory=cs::memory::Reserve(memory::Pool::Workspace,needed,"host_flat_compare");
  else memory->Resize(needed);
  byte_bits.resize(byte_words);
  field_bits.resize(field_words);
  tails.resize(layout.tail_fields.size());
  memory->Resize((byte_bits.capacity()+field_bits.capacity())*4+tails.capacity()*sizeof(FlatTailSpan));
}

// 仅当规范字节真的改变才构建新图像；改变的字段取自身所属流的新版本。
HostFlatImageRef UpdateFlatImage(const FlatLayout &layout,std::span<const std::byte> image,std::span<const FlatTailSpan> tails,
    const HostFlatImageRef &previous,FlatVersions versions,FlatCompareScratch &scratch,const CaptureRoot &root,
    CaptureDiagnostic &diagnostic) {
  if(image.size()<layout.mirror_size || tails.size()!=layout.tail_fields.size()){
    FailCapture(diagnostic,CaptureFailure::InvalidValue,root);
    return {};
  }
  auto &changed=scratch.field_bits;
  std::fill(changed.begin(),changed.end(),0);
  if(previous){
    const auto before=previous->Bytes();
    if(!MarkFlatMainChanges(layout,before.first(layout.mirror_size),image.first(layout.mirror_size),scratch.byte_bits,changed)){
      FailCapture(diagnostic,CaptureFailure::InvalidValue,root);
      return {};
    }
    const auto &simd=FlatKernels();
    for(std::size_t i=0;i<tails.size();++i){
      const auto &tail=tails[i],&old=previous->tails[i];
      if(tail.size==old.size && (!tail.size||simd.equal(U8(Tail(image,tail)),U8(Tail(before,old)),tail.size)))continue;
      const auto ordinal=layout.tail_fields[i];
      changed[ordinal/32]|=1u<<(ordinal%32);
    }
    if(std::none_of(changed.begin(),changed.end(),[](auto word){return word!=0;}))return previous;
  }else{
    for(const auto &patch:layout.patches)if(patch.kind==FlatPatchKind::Zero){
      const auto zero=image.subspan(patch.mirror_offset,patch.size);
      if(std::any_of(zero.begin(),zero.end(),[](auto value){return value!=std::byte{};})){
        FailCapture(diagnostic,CaptureFailure::InvalidValue,root);
        return {};
      }
    }
    for(std::size_t i=0;i<changed.size();++i)changed[i]=layout.stream_fields[0][i]|layout.stream_fields[1][i];
  }

  const auto blocks=(image.size()+31)/32;
  auto charge=memory::Reserve(memory::Pool::Values,sizeof(HostFlatImage)+128+blocks*sizeof(HostFlatImage::Block)+
    tails.size()*sizeof(FlatTailSpan)+layout.fields.size()*sizeof(std::uint32_t),"host_flat_image");
  auto next=std::make_shared<HostFlatImage>();
  next->memory=std::move(charge);
  next->size=std::uint32_t(image.size());
  next->storage.resize(blocks);
  std::memcpy(next->storage.data(),image.data(),image.size());
  next->tails.assign(tails.begin(),tails.end());
  if(previous){
    next->stamps=previous->stamps;
    next->newest=previous->newest;
  }else next->stamps.resize(layout.fields.size());
  SetBits(changed,[&](std::size_t ordinal){
    const auto stream=unsigned(layout.fields[ordinal].stream);
    const auto version=versions[stream];
    if(!version || version>kFlatMaxVersion || version<=next->stamps[ordinal])
      FailCapture(diagnostic,CaptureFailure::InvalidTransaction,root,std::uint16_t(ordinal));
    next->stamps[ordinal]=version;
    next->newest[stream]=version;
  });
  if(diagnostic.reason!=CaptureFailure::None)return {};
  next->memory->Resize(sizeof(HostFlatImage)+128+next->storage.capacity()*sizeof(HostFlatImage::Block)+
    next->tails.capacity()*sizeof(FlatTailSpan)+next->stamps.capacity()*sizeof(std::uint32_t));
  return next;
}

void FlatEncodeScratch::Prepare(const FlatLayout &layout) {
  const auto words=(layout.fields.size()+31)/32,group_count=(layout.fields.size()+15)/16;
  const auto top_bytes=(group_count+7)/8;
  const auto needed=(std::max)(words,field_bits.capacity())*4+
    (std::max)(group_count,groups.capacity())*2+(std::max)(top_bytes,top.capacity());
  if(!memory)memory=cs::memory::Reserve(memory::Pool::Workspace,needed,"host_flat_field_set");
  else memory->Resize(needed);
  field_bits.resize(words);
  groups.resize(group_count);
  top.resize(top_bytes);
  memory->Resize(field_bits.capacity()*4+groups.capacity()*2+top.capacity());
}

std::optional<FlatEncodedRecord> EncodeFlatRecord(const FlatLayout &layout,const HostFlatImage &image,FlatStream stream,
    std::uint32_t confirmed,bool all,FlatEncodeScratch &scratch,memory::Lease &payload_memory) {
  const auto flow=unsigned(stream);
  if(!all && image.newest[flow]<=confirmed)return {};
  auto bits=layout.stream_fields[flow];
  auto mode=FlatSetMode::All;
  const auto &simd=FlatKernels();
  std::size_t count=0,set_bytes=0;
  if(!all){
    scratch.Prepare(layout);
    simd.newer(image.stamps.data(),image.stamps.size(),confirmed,scratch.field_bits.data());
    for(std::size_t i=0;i<bits.size();++i){
      auto &word=scratch.field_bits[i];
      word&=bits[i];
      count+=simd.popcount(word);
    }
    if(!count)return {};
    bits=scratch.field_bits;
    for(std::size_t i=0;i<scratch.groups.size();++i)scratch.groups[i]=std::uint16_t(bits[i/2]>>(16*(i%2)));
    simd.groups(scratch.groups.data(),scratch.groups.size(),scratch.top.data());
    std::size_t bitmap_bytes=scratch.top.size(),delta_bytes=LebSize(count);
    for(auto byte:scratch.top)bitmap_bytes+=2*simd.popcount(byte);
    std::size_t next=0;
    SetBits(bits,[&](std::size_t ordinal){delta_bytes+=LebSize(ordinal-next);next=ordinal+1;});
    mode=bitmap_bytes<=delta_bytes?FlatSetMode::Bitmap:FlatSetMode::Deltas;
    set_bytes=(std::min)(bitmap_bytes,delta_bytes);
  }else if(std::none_of(bits.begin(),bits.end(),[](auto word){return word!=0;}))return {};

  std::size_t total=set_bytes;
  SetBits(bits,[&](std::size_t ordinal){
    const auto &f=layout.fields[ordinal];
    total+=f.size;
    if(f.tail!=kNullIndex)total+=image.tails[f.tail].size;
  });
  FlatEncodedRecord out{mode,{}};
  if(!payload_memory)payload_memory=memory::Reserve(memory::Pool::Workspace,0,"host_flat_peer_records");
  const auto previous_bytes=payload_memory->Bytes();
  payload_memory->Resize(previous_bytes+total);
  out.bytes.reserve(total);
  payload_memory->Resize(previous_bytes+out.bytes.capacity());
  if(mode==FlatSetMode::Bitmap){
    for(auto byte:scratch.top)out.bytes.push_back(std::byte{byte});
    for(auto group:scratch.groups)if(group){
      out.bytes.push_back(std::byte(group&255));
      out.bytes.push_back(std::byte(group>>8));
    }
  }else if(mode==FlatSetMode::Deltas){
    PutLeb(out.bytes,count);
    std::size_t next=0;
    SetBits(bits,[&](std::size_t ordinal){PutLeb(out.bytes,ordinal-next);next=ordinal+1;});
  }
  const auto bytes=image.Bytes();
  SetBits(bits,[&](std::size_t ordinal){
    const auto &f=layout.fields[ordinal];
    const auto value=bytes.subspan(f.offset,f.size);
    out.bytes.insert(out.bytes.end(),value.begin(),value.end());
  });
  SetBits(bits,[&](std::size_t ordinal){
    const auto tail=layout.fields[ordinal].tail;
    if(tail==kNullIndex)return;
    const auto value=Tail(bytes,image.tails[tail]);
    out.bytes.insert(out.bytes.end(),value.begin(),value.end());
  });
  return out;
}
}
