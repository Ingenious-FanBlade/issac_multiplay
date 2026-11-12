#include "world/binary/entity_source_internal.h"

namespace cs::world::binary {
// 从每个真实身体的有序History生成自有显示列表，沿82FF40的倒序／主动过滤／饰品标记。
// 这里只读取isTrinket与已选item身份，不恢复原始时间、房间、池、保存史或调用Recompute。
bool EntitySource::Impl::HistoryGlyphs(const CaptureFieldAccess &a,CaptureTraversalSource &out,CaptureDiagnostic &d) noexcept {
  const auto start=witness_used;out={};out.identity=a.root.address;
  std::array<std::uint32_t,3> header{};
  const auto address=std::uint64_t(a.root.address)+(facts.history_from_player?0x1dd8:8);
  const auto stride=facts.history_from_player?28u:40u;
  if(!Tracked(address,header,d) || header[0]>header[1] || header[1]>header[2] || (!header[0]&&(header[1]||header[2])) ||
     (header[1]-header[0])%stride || (header[2]-header[0])%stride)return Reject(a,d,CaptureFailure::InvalidValue);
  const auto count=(header[1]-header[0])/stride;if(count>4096)return Reject(a,d,CaptureFailure::CountMismatch);
  if(!facts.history_from_player){out.count=count;out.opaque[0]=header[0];out.opaque[1]=40;out.opaque[2]=start;out.opaque[3]=witness_used-start;return true;}
  if(!facts.player_ready)return Reject(a,d,CaptureFailure::MissingQualification);
  auto bytes=Allocate(std::uint64_t(count)*sizeof(NativeHistoryGlyph),alignof(NativeHistoryGlyph),d);if(d.reason!=CaptureFailure::None)return false;
  auto glyphs=std::span(reinterpret_cast<NativeHistoryGlyph *>(bytes.data()),count);std::uint32_t used=0;
  std::uint32_t manager=0;if(!Tracked(std::uint64_t(memory.image_base)+0x7e4a1c,manager,d) || !manager)return Reject(a,d,CaptureFailure::MissingQualification);
  for(std::uint32_t reverse=count;reverse;--reverse){const auto item=std::uint64_t(header[0])+std::uint64_t(reverse-1)*28;std::int32_t id=0;std::uint8_t trinket=0;
    if(!Tracked(item+4,trinket,d) || trinket>1 || !Tracked(item+8,id,d))return Reject(a,d,CaptureFailure::InvalidValue);
    if(id<0)return Reject(a,d,CaptureFailure::ResourcePending); // 程序道具必须有实际atlas／recipe，不借普通文件图兜底。
    const auto index=trinket?(std::uint32_t(id)&0x7fffu):std::uint32_t(id);std::array<std::uint32_t,3> configs{};std::uint32_t config=0;
    if(!Tracked(std::uint64_t(manager)+0x2a234+(trinket?12:0),configs,d) || configs[0]>configs[1] || configs[1]>configs[2] ||
       (configs[1]-configs[0])%4 || (configs[2]-configs[0])%4)return Reject(a,d,CaptureFailure::InvalidValue);
    if(index>=(configs[1]-configs[0])/4)continue; // 与原生GetCollectible/GetTrinket明确Null选择一致。
    if(!Tracked(std::uint64_t(configs[0])+std::uint64_t(index)*4,config,d))return false;if(!config)continue;
    std::uint32_t category=0;if(!Tracked(config,category,d))return false;if(category==3)continue;
    CaptureLink item_resource;if(!ItemResource(config,item_resource,d) || !item_resource.handle)return Reject(a,d,CaptureFailure::ResourcePending);
    const auto &resource=resources[std::size_t(item_resource.handle-1)];
    if(!resource.links[0])return Reject(a,d,CaptureFailure::ResourcePending);
    const auto image=resource.links[0];const auto &image_resource=resources[std::size_t(image-1)];
    glyphs[used++]={std::bit_cast<std::int32_t>(std::uint32_t(id)|(trinket?0x80000u:0u)),
      {image_resource.metadata_known?CapturedReferenceKind::Identity:CapturedReferenceKind::Pending,image}};
  }
  out.count=used;out.opaque[0]=NativeAddress(bytes);out.opaque[1]=sizeof(NativeHistoryGlyph)|(std::uint64_t(NativeSequence::Owned)<<32);
  out.opaque[2]=start;out.opaque[3]=witness_used-start;return true;
}
}
