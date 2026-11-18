#include "world/binary/resource_export.h"
#include <algorithm>
#include <cstring>

namespace cs::world::binary {
namespace {
using namespace state;
[[noreturn]] void Fail(const char *s){throw Gap("host_resources",s);}
void Put(std::vector<std::byte> &out,std::uint64_t value,unsigned width){for(unsigned i=0;i<width;++i)out.push_back(std::byte(value>>(i*8)));}
std::span<const std::byte> Text(NativeResourceView view,const NativeResource &r){
  if(r.text>view.bytes.size()||r.text_bytes>view.bytes.size()-r.text)Fail("text_bounds");return view.bytes.subspan(r.text,r.text_bytes);
}
void String(std::vector<std::byte> &out,std::span<const std::byte> text){if(text.size()>4096)Fail("path_length");Put(out,text.size(),2);out.insert(out.end(),text.begin(),text.end());}
}
std::vector<state::HostStateResource> ExportStateResources(NativeResourceView view,std::span<const state::HostStateResource> previous){
  // Intern 在目录末尾分配 1-based 句柄，各表的线上号独立累加。
  if(previous.size()>view.entries.size())Fail("catalog_rewound");
  std::vector<HostStateResource> result(previous.begin(),previous.end());
  std::array<std::uint32_t,8> counts{};
  for(std::size_t i=0;i<result.size();++i){
    const auto &entry=result[i];
    if(entry.handle!=i+1 || entry.key.table>=counts.size())Fail("resource_index_conflict");
    counts[entry.key.table]=std::max(counts[entry.key.table],std::uint32_t(entry.key.index)+1);
  }
  auto key_memory=memory::Reserve(memory::Pool::Workspace,
    (view.entries.size()-result.size())*sizeof(ResourceKey),"host_resource_keys");
  std::vector<ResourceKey> added(view.entries.size()-result.size());
  key_memory->Resize(added.capacity()*sizeof(ResourceKey));
  for(std::size_t i=result.size();i<view.entries.size();++i){
    const auto &entry=view.entries[i];
    if(entry.handle!=i+1 || entry.table>=counts.size() || counts[entry.table]>=kNullIndex)Fail("resource_index_exhausted_or_conflict");
    added[i-previous.size()]={entry.table,std::uint16_t(counts[entry.table]++)};
  }
  const auto reference=[&](std::uint64_t handle,std::uint8_t table){
    if(!handle)return kNullIndex;
    if(handle>view.entries.size())Fail("dependency_missing");
    const auto i=std::size_t(handle-1);
    const auto key=i<previous.size()?previous[i].key:added[i-previous.size()];
    if(key.table!=table)Fail("dependency_missing");
    return key.index;
  };
  std::array<std::byte,65536> check{};std::array<ProjectionFact,256> facts{};std::array<BodyFieldView,32> fields{};
  ResourceCodecScratch scratch{{facts,0,fields}};
  for(std::size_t i=result.size();i<view.entries.size();++i){const auto &r=view.entries[i];const auto &values=r.values;
    std::vector<std::byte> bytes;bytes.reserve(8192);
    switch(r.kind){
    case 0:case state::kResourceImagePathKind:String(bytes,Text(view,r));break;
    case 1:String(bytes,Text(view,r));for(unsigned n=0;n<4;++n){if(values[n]>65535)Fail("image_geometry_range");Put(bytes,values[n],2);}break;
    case 2:Put(bytes,values[0],1);break;
    case 3:for(unsigned n=0;n<3;++n)Put(bytes,values[n],4);for(unsigned n=3;n<6;++n)Put(bytes,values[n],1);break;
    case 6:{Put(bytes,reference(r.links[0],2),2);String(bytes,Text(view,r));Put(bytes,values[0],4);
      if(r.extra>view.layers.size()||r.extra_count>view.layers.size()-r.extra||std::uint64_t(values[1])+values[2]!=r.extra_count)Fail("animation_layers_bounds");
      unsigned offset=0;for(unsigned group=0;group<2;++group){const auto count=values[group+1];if(count>256)Fail("animation_layer_budget");Put(bytes,count,2);
        for(unsigned n=0;n<count;++n){const auto &layer=view.layers[r.extra+offset++];Put(bytes,layer.id,2);Put(bytes,layer.frames,4);}}break;}
    case 7:for(unsigned n=0;n<3;++n)Put(bytes,values[n],4);Put(bytes,reference(r.links[0],1),2);Put(bytes,reference(r.links[1],2),2);break;
    case 8:for(unsigned n=0;n<3;++n)Put(bytes,values[n],4);Put(bytes,reference(r.links[0],2),2);Put(bytes,values[3],4);Put(bytes,values[4],4);break;
    case 9:Put(bytes,values[0],1);Put(bytes,values[1],1);String(bytes,Text(view,r));if(values[1])Fail("music_intro_source_missing");break;
    case 4:{
      // Shape白图和TMTRAINER图集均只描述本地分配所需的一次Clear。
      if(r.values[5]!=1||Text(view,r).size()!=sizeof(NativeSurfaceOperation))Fail("retired_surface_production");
      NativeSurfaceOperation op;std::memcpy(&op,Text(view,r).data(),sizeof(op));if(op.op)Fail("shape_clear_required");
      Put(bytes,values[4],2);for(unsigned n=0;n<4;++n)Put(bytes,values[n],2);for(unsigned n=0;n<4;++n)Put(bytes,0,1);Put(bytes,1,2);
      Put(bytes,0,1);for(const auto v:op.colors[0])Put(bytes,v,4);break;}
    default:Fail("unsupported_resource_kind");
    }
    const ResourceEntryView entry{added[i-previous.size()],r.kind,bytes};
    // 正式编码器核typed描述及合法路径；不加载资源，不把元数据标为GPU／音频ready。
    EncodeResourceEntryInto(entry,check,scratch);
    result.push_back({r.handle,entry.key,r.kind,std::make_shared<const HostStateBytes>(bytes)});
  }
  return result;
}
}
