#include "world/binary/entity_source_internal.h"
#include "state/resource_local.h"
#include <cmath>
#include <compare>
#include <tuple>

namespace cs::world::binary {
// 接受点已复制的音频路径可延后登记；只建立自有资源描述，不复读生命周期内的旧sample。
bool EntitySource::DisplayAudioPathResource(const CaptureWitness &w,EntityFacts f,std::span<const std::byte> path,CaptureLink &out,CaptureDiagnostic &d) noexcept {
  auto &s=*impl_;if(s.active||std::this_thread::get_id()!=s.thread||!s.services.current(s.services.owner,w,f))return FailCapture(d,CaptureFailure::InvalidWindow,{f.subject});
  s.facts=f;s.window.witness=w;
  std::array<std::byte,4096> bytes{};if(path.empty()||path.size()>bytes.size())return FailCapture(d,CaptureFailure::ResourcePending,{f.subject});
  std::copy(path.begin(),path.end(),bytes.begin());for(auto &v:std::span(bytes).first(path.size()))if(v==std::byte{'\\'})v=std::byte{'/'};
  const std::string_view text(reinterpret_cast<const char *>(bytes.data()),path.size());
  if(!(text.starts_with("sfx/")||text.starts_with("music/"))||text.find("..")!=text.npos||text.find_first_of(":<>\"|?*")!=text.npos||
     !(text.ends_with(".wav")||text.ends_with(".ogg")||text.ends_with(".mp3")))return FailCapture(d,CaptureFailure::ResourcePending,{f.subject});
  NativeResource value;value.table=7;value.kind=9;return s.Intern(value,std::span(bytes).first(path.size()),{},out,d);
}
// 只解析正常来源已经选出的配置，不调用配置生成器或Lua，不传原生地址。
bool EntitySource::DisplayItemResource(const CaptureWitness &w,EntityFacts f,std::uint32_t config,CaptureLink &out,CaptureDiagnostic &d) noexcept {
  auto &s=*impl_;if(s.active || std::this_thread::get_id()!=s.thread)return FailCapture(d,CaptureFailure::InvalidWindow,{f.subject});
  s.facts=f;s.active=true;s.scratch_used=s.witness_used=0;
  struct End{bool &active;~End(){active=false;}}end{s.active};
  s.window={s.thread,w,&s,[](void *p,const CaptureWitness &v,const CaptureRoot &r) noexcept {return static_cast<Impl *>(p)->Current(v,r);},nullptr};
  return s.ItemResource(config,out,d);
}
bool EntitySource::DisplayProceduralAtlas(const CaptureWitness &w,EntityFacts f,std::uint32_t image,CaptureLink &out,CaptureDiagnostic &d) noexcept {
  auto &s=*impl_;if(s.active||std::this_thread::get_id()!=s.thread)return FailCapture(d,CaptureFailure::InvalidWindow,{f.subject});
  s.facts=f;s.active=true;s.scratch_used=s.witness_used=0;
  struct End{bool &active;~End(){active=false;}}end{s.active};
  s.window={s.thread,w,&s,[](void *p,const CaptureWitness &v,const CaptureRoot &r) noexcept {return static_cast<Impl *>(p)->Current(v,r);},nullptr};
  return s.ImageResource(image,out,d);
}
namespace {
// 与客机的 SamePath 一致，将官方资源根下的实际名称转为逻辑 VFS 路径。
std::span<std::byte> Vfs(std::span<std::byte> bytes,std::uint8_t kind) noexcept {
  for(auto &b:bytes)if(b==std::byte{'\\'})b=std::byte{'/'};
  std::string_view name(reinterpret_cast<const char *>(bytes.data()),bytes.size());
  for(std::string_view root:{"resources/","resources-dlc3/","resources-repentogon/"}){
    if(name.starts_with(root)){bytes=bytes.subspan(root.size());break;}
  }
  const std::string_view path(reinterpret_cast<const char *>(bytes.data()),bytes.size());
  const auto equal=[](std::string_view a,std::string_view b){return a.size()==b.size() && std::equal(a.begin(),a.end(),b.begin(),[](char x,char y){return (x>='A'&&x<='Z'?char(x+32):x)==y;});};
  const auto ends=[&](std::string_view extension){return path.size()>=extension.size() && equal(path.substr(path.size()-extension.size()),extension);};
  if(!path.starts_with("gfx/") || path.empty() || path.back()=='/' || path.find_first_of(":<>\"|?*")!=path.npos ||
     (kind==0?!ends(".anm2"):!(ends(".png")||ends(".pcx"))))return {};
  for(std::size_t start=0;start<path.size();){auto end=path.find('/',start);if(end==path.npos)end=path.size();const auto part=path.substr(start,end-start);
    if(part.empty() || part=="." || part==".." || part.front()==' ' || part.back()==' ' || part.back()=='.')return {};
    const auto stem=part.substr(0,part.find('.'));
    if(equal(stem,"con") || equal(stem,"prn") || equal(stem,"aux") || equal(stem,"nul") ||
       (stem.size()==4 && stem[3]>='1' && stem[3]<='9' && (equal(stem.substr(0,3),"com") || equal(stem.substr(0,3),"lpt"))))return {};
    start=end+1;}
  for(std::size_t i=0;i<bytes.size();){const auto c=std::to_integer<unsigned>(bytes[i++]);std::uint32_t cp=c,minimum=0;unsigned remaining=0;
    if(c>=0xc2&&c<=0xdf){cp=c&31;minimum=0x80;remaining=1;}
    else if(c>=0xe0&&c<=0xef){cp=c&15;minimum=0x800;remaining=2;}
    else if(c>=0xf0&&c<=0xf4){cp=c&7;minimum=0x10000;remaining=3;}
    else if(c>0x7f)return {};
    if(remaining>bytes.size()-i)return {};
    while(remaining--){const auto n=std::to_integer<unsigned>(bytes[i++]);if((n&0xc0)!=0x80)return {};cp=(cp<<6)|(n&63);}
    if(cp<minimum || cp<32 || (cp>=0x7f&&cp<=0x9f) || cp>0x10ffff || (cp>=0xd800&&cp<=0xdfff))return {};
  }
  return bytes;
}
}
// 资源按完整具名描述比较并只追加；没有原生地址、哈希、覆盖旧号或逐帧堆分配。
bool EntitySource::Impl::Intern(NativeResource value,std::span<const std::byte> text,std::span<const NativeAnimationLayer> layers,
    CaptureLink &out,CaptureDiagnostic &d) noexcept {
  const auto fields=[](const NativeResource &r){return std::tie(r.table,r.kind,r.source_owner,r.links,r.values);};
  const auto compare=[&](std::uint32_t i){const auto &prior=resources[i];
    if(const auto c=fields(prior)<=>fields(value);c!=0)return c;
    const auto p=std::span(resource_bytes).subspan(prior.text,prior.text_bytes);
    if(const auto c=std::lexicographical_compare_three_way(p.begin(),p.end(),text.begin(),text.end());c!=0)return c;
    const auto a=std::span(animation_layers).subspan(prior.extra,prior.extra_count);
    return std::lexicographical_compare_three_way(a.begin(),a.end(),layers.begin(),layers.end(),[](const auto &x,const auto &y){
      return std::tie(x.id,x.frames)<=>std::tie(y.id,y.frames);});};
  const auto end=resource_order.begin()+resource_used;
  const auto at=std::lower_bound(resource_order.begin(),end,0,[&](auto i,int){return compare(i)<0;});
  if(at!=end&&compare(*at)==0){out={CapturedReferenceKind::Identity,resources[*at].handle};return true;}
  if(resource_used==resources.size() || text.size()>resource_bytes.size()-resource_bytes_used || layers.size()>animation_layers.size()-layer_used)
    return FailCapture(d,CaptureFailure::ResourcePending,{facts.subject});
  value.handle=std::uint64_t(resource_used)+1;value.text=resource_bytes_used;value.text_bytes=std::uint32_t(text.size());
  value.extra=layer_used;value.extra_count=std::uint32_t(layers.size());
  std::copy(text.begin(),text.end(),resource_bytes.begin()+resource_bytes_used);resource_bytes_used+=std::uint32_t(text.size());
  std::copy(layers.begin(),layers.end(),animation_layers.begin()+layer_used);layer_used+=std::uint32_t(layers.size());
  std::move_backward(at,end,end+1);*at=resource_used;
  resources[resource_used++]=value;out={CapturedReferenceKind::Identity,value.handle};return true;
}
// ANM2.loaded只决定可空文件选择，不表示GPU ready；配置中的文件路径没有该字节。
bool EntitySource::Impl::FileResource(std::uint32_t address,bool selected,CaptureLink &out,CaptureDiagnostic &d,bool player_sprite) noexcept {
  out={CapturedReferenceKind::Null,0};std::uint8_t loaded=1;
  if(selected){auto end=sprite_dependencies.begin()+sprite_dependency_used;
    auto where=std::lower_bound(sprite_dependencies.begin(),end,address,[](const auto &entry,std::uint32_t p){return entry.address<p;});
    if(where==end || where->address!=address){if(sprite_dependency_used==sprite_dependencies.size())return FailCapture(d,CaptureFailure::ValueBudget,{facts.subject});
      std::move_backward(where,end,end+1);++sprite_dependency_used;}
    *where={address,facts.address,facts.subject.local,facts.subject.use,facts.subject.space};}
  if(selected&&(!(player_sprite?Tracked(std::uint64_t(address)+0x109,loaded,d,{address,NativeEvidenceUse::Loaded}):
      Value(std::uint64_t(address)+0x109,loaded,d))||loaded>1))return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
  auto *memo=selected?SpriteMemo(address,loaded!=0):nullptr;
  if(!loaded){
    if(memo&&memo->file_ready){memo->file_ready=false;memo->stamp=++resource_cache.stamp;memo->animations={};}
    return true;
  }
  const auto parse=[&]{
    std::array<std::byte,4096> bytes{};std::uint32_t size=0;
    if(!StringAt(address,bytes,size,d,player_sprite,{address,NativeEvidenceUse::FileName}))return false;
    if(!size)return !selected || FailCapture(d,CaptureFailure::ResourcePending,{facts.subject});
    const auto path=Vfs(std::span(bytes).first(size),0);if(path.empty())return FailCapture(d,CaptureFailure::ResourcePending,{facts.subject});
    NativeResource value;value.table=2;value.kind=0;return Intern(value,path,{},out,d);
  };
  if(!memo)return ResourceOnce({selected?NativeResourceSource::SelectedFile:NativeResourceSource::File,address},out,parse,!player_sprite);
  std::array<std::byte,24> head;
  // 文件名的 SSO 或堆指针、长度与容量都在 Sprite 头中。原地改写由 ANM2 Hook 撤销。
  if(!Read(address,head,d))return false;
  const auto kind=unsigned(NativeResourceSource::SelectedFile);
  if(memo->file_ready&&memo->file_head==head){
    out=memo->file;++stats.resource_instance_hits[kind];return true;
  }
  ++stats.resource_parses[kind];
  if(!parse())return false;
  if(memo->file_ready){memo->stamp=++resource_cache.stamp;memo->animations={};}
  memo->file_ready=true;memo->file_head=head;memo->file=out;
  return true;
}
// 只解释已核四种Image的尺寸getter对应成员，不执行虚函数或GPU读回。
bool EntitySource::Impl::ImageGeometry(std::uint32_t image,std::array<std::uint32_t,4> &geometry,CaptureDiagnostic &d) noexcept {
  std::uint32_t type=0;if(!Value(image,type,d))return false;
  if(type==memory.image_base+0x73ba80){std::array<std::uint16_t,4> values{};if(!Value(std::uint64_t(image)+128,values,d))return false;std::copy(values.begin(),values.end(),geometry.begin());}
  else if(type==memory.image_base+0x73b8ec){if(!Value(std::uint64_t(image)+256,geometry,d))return false;}
  else if(type==memory.image_base+0x73bcac){if(!Value(std::uint64_t(image)+128,geometry,d))return false;}
  else if(type==memory.image_base+0x73bc14){std::array<std::uint16_t,2> logical{};std::array<std::uint32_t,2> stored{};
    if(!Value(std::uint64_t(image)+128,logical,d) || !Value(std::uint64_t(image)+132,stored,d))return false;geometry={logical[0],logical[1],stored[0],stored[1]};}
  else return FailCapture(d,CaptureFailure::ResourcePending,{facts.subject});
  if(!geometry[0] || !geometry[1] || geometry[0]>geometry[2] || geometry[1]>geometry[3] || geometry[2]>65535 || geometry[3]>65535)
    return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});return true;
}
// PNG为四个u16(+80..86)，PCX为四个u32(+100..10C)；按实际vtable区分，不执行虚getter。
bool EntitySource::Impl::ImageResource(std::uint32_t address,CaptureLink &out,CaptureDiagnostic &d) noexcept {
  out={CapturedReferenceKind::Null,0};if(!address)return true;
  return ResourceOnce({NativeResourceSource::Image,address,0,facts.scene?facts.scene->game:0},out,[&]{
  std::uint32_t vtable=0,name=0;
  std::array<std::uint32_t,4> geometry{};
  std::uint64_t flags=0;if(!Value(std::uint64_t(address)+16,flags,d))return false;
  if(flags&2){std::uint32_t shape=0;if(!Value(std::uint64_t(memory.image_base)+0x7e743c,shape,d))return false;
    if(facts.scene){
      // Game+669C8／669D0 是程序道具两张原生图集。资源目录只描述本地分配，不发画面或强引用。
      for(unsigned size:{1024u,512u}){
        std::uint32_t atlas=0;
        if(!Value(std::uint64_t(facts.scene->game)+(size==1024?0x669c8:0x669d0),atlas,d))return false;
        if(address!=atlas)continue;
        if(!ImageGeometry(address,geometry,d)||geometry!=std::array<std::uint32_t,4>{size,size,size,size})
          return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
        NativeSurfaceOperation clear;
        NativeResource value;value.table=1;value.kind=4;
        std::copy(geometry.begin(),geometry.end(),value.values.begin());value.values[4]=1;value.values[5]=1;
        return Intern(value,std::as_bytes(std::span(&clear,1)),{},out,d);
      }
    }
    if(address==shape){if(!ImageGeometry(address,geometry,d))return false;
      // 原生Shape Renderer在A2AB60以白色创建；本地固定Clear recipe，无纹理字节／下载。
      NativeSurfaceOperation clear;clear.colors[0]={0x3f800000,0x3f800000,0x3f800000,0x3f800000,0};
      NativeResource value;value.table=1;value.kind=4;std::copy(geometry.begin(),geometry.end(),value.values.begin());value.values[4]=1;value.values[5]=1;
      return Intern(value,std::as_bytes(std::span(&clear,1)),{},out,d);}
    return FailCapture(d,CaptureFailure::ResourcePending,{facts.subject});}
  if(!Value(address,vtable,d) || (vtable!=memory.image_base+0x73ba80 && vtable!=memory.image_base+0x73b8ec) ||
     !Value(std::uint64_t(address)+0x3c,name,d) || !name)
    return FailCapture(d,CaptureFailure::ResourcePending,{facts.subject});
  if(vtable==memory.image_base+0x73ba80){std::array<std::uint16_t,4> png{};if(!Value(std::uint64_t(address)+0x80,png,d))return false;
    std::copy(png.begin(),png.end(),geometry.begin());}
  else if(!Value(std::uint64_t(address)+0x100,geometry,d))return false;
  if(!geometry[0] || !geometry[1] || geometry[0]>geometry[2] || geometry[1]>geometry[3])return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
  std::array<std::byte,4097> bytes{};std::uint32_t size=0;if(!CStringAt(name,4097,bytes,size,d))return false;
  const auto path=Vfs(std::span(bytes).first(size),1);
  if(path.empty()){
    // 记录有界的原生图像名称，控制回复保留单行格式。
    for(std::size_t i=0;i<std::min<std::size_t>(size,d.detail.size()-1);++i){
      const auto ch=std::to_integer<unsigned char>(bytes[i]);d.detail[i]=ch>=32&&ch<=126?char(ch):'?';
    }
    return FailCapture(d,CaptureFailure::ResourcePending,{facts.subject});
  }
  NativeResource value;value.table=1;value.kind=1;for(unsigned i=0;i<4;++i)value.values[i]=geometry[i];
  if(!Intern(value,path,{},out,d))return false;
  std::uint32_t after=0;return (Value(address,after,d)&&after==vtable)||FailCapture(d,CaptureFailure::SourceChanged,{facts.subject});});
}
bool EntitySource::Impl::ItemResource(std::uint32_t address,CaptureLink &out,CaptureDiagnostic &d) noexcept {
  out={CapturedReferenceKind::Null,0};if(!address)return true;
  return ResourceOnce({NativeResourceSource::Item,address},out,[&]{NativeResource value;value.table=3;value.kind=7;
  if(address>UINT32_MAX-0x9c)return FailCapture(d,CaptureFailure::AddressRange,{facts.subject});
  if(!Value(address,value.values[0],d) || !Value(std::uint64_t(address)+4,value.values[1],d) ||
     !Value(std::uint64_t(address)+0x78,value.values[2],d))return false;
  if(value.values[0]>4)return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
  // 直接授予道具时可能一直没有 Host 图像实例。配置路径可以独立登记并在 Client 本地装载。
  std::array<std::byte,4096> bytes{};std::uint32_t size=0;CaptureLink image,costume;
  if(!StringAt(address+0x38,bytes,size,d))return false;
  if(size&&!(value.values[1]&0x80000000u)){const auto path=Vfs(std::span(bytes).first(size),1);if(path.empty())return FailCapture(d,CaptureFailure::ResourcePending,{facts.subject});
    NativeResource file;file.table=1;file.kind=state::kResourceImagePathKind;
    if(!Intern(file,path,{},image,d))return false;value.links[0]=image.handle;}
  if(!FileResource(address+0x84,false,costume,d))return false;value.links[1]=costume.handle;
  return Intern(value,{},{},out,d);});
}
// 动画描述来自实际AnimationData和其ANM2文件owner；普通与null层分别保留原顺序。
bool EntitySource::Impl::AnimationResource(std::uint32_t owner,std::uint32_t animation,CaptureLink &out,CaptureDiagnostic &d,
    const CaptureLink *prepared_file) noexcept {
  out={CapturedReferenceKind::Null,0};if(!animation)return true;
  // 即使命中动画，仍核当前文件并登记 Sprite 的生命周期来源。
  CaptureLink file;
  if(prepared_file)file=*prepared_file;
  else if(!FileResource(owner,true,file,d))return false;
  if(file.kind==CapturedReferenceKind::Null)return FailCapture(d,CaptureFailure::ResourcePending,{facts.subject});
  NativeResourceKey key{NativeResourceSource::Animation,animation,owner};
  key.dependency=file.handle;
  auto *memo=(resource_cache.observed&1u)?SpriteMemo(owner):nullptr;
  const auto epoch=resource_cache.animation_epoch;
  if(memo)for(const auto &entry:memo->animations)
    if(entry.epoch==epoch&&entry.file==file.handle&&entry.address==animation){
      out=entry.link;++stats.resource_instance_hits[unsigned(NativeResourceSource::Animation)];return true;
    }
  const bool parsed=ResourceOnce(key,out,[&]{
  NativeResource value;value.table=0;value.kind=6;value.links[0]=file.handle;
  std::array<std::uint32_t,4> layers{};
  if(!Value(std::uint64_t(animation)+0x18,layers,d)||
     !Value(std::uint64_t(animation)+0x30,value.values[0],d))return false;
  const std::array<std::uint32_t,2> arrays{layers[0],layers[2]},counts{layers[1],layers[3]};
  if(counts[0]>65535 || counts[1]>65535 || value.values[0]>INT32_MAX || (counts[0]&&!arrays[0]) || (counts[1]&&!arrays[1]))
    return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
  value.values[1]=counts[0];value.values[2]=counts[1];
  const auto mark=scratch_used;
  auto bytes=Allocate(std::uint64_t(counts[0]+counts[1])*sizeof(NativeAnimationLayer),alignof(NativeAnimationLayer),d);
  if(d.reason!=CaptureFailure::None)return false;auto rows=std::span(reinterpret_cast<NativeAnimationLayer *>(bytes.data()),counts[0]+counts[1]);
  std::uint32_t offset=0;
  for(unsigned group=0;group<2;++group){for(std::uint32_t i=0;i<counts[group];++i){std::uint32_t id=0,frames=0;
      const auto address=std::uint64_t(arrays[group])+std::uint64_t(i)*16;
      if(!Value(address,id,d) || !Value(address+(group?12u:8u),frames,d) || id>65535 || frames>INT32_MAX)return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
      for(std::uint32_t j=offset;j<offset+i;++j)if(rows[j].id==id)return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
      rows[offset+i]={std::uint16_t(id),frames};}offset+=counts[group];}
  std::array<std::byte,2048> name{};std::uint32_t length=0;if(!StringAt(animation,name,length,d))return false;
  // Intern 已复制层目录，归还临时解析区，避免两流累积已消费的中间结果。
  const bool stored=Intern(value,std::span(name).first(length),rows,out,d);
  scratch_used=mark;return stored;});
  if(parsed&&memo){
    memo->animations[1]=memo->animations[0];
    memo->animations[0]={epoch,file.handle,animation,out};
  }
  return parsed;
}
// 两种采集器共用 VFS 规范化及目录登记，构造产物不含原生字符串地址。
bool EntitySource::Impl::ImagePathResource(std::uint32_t address,CaptureLink &out,CaptureDiagnostic &d) noexcept {
  out={CapturedReferenceKind::Null,0};
  return ResourceOnce({NativeResourceSource::ImagePath,address},out,[&]{
    std::array<std::byte,4096> bytes{};
    std::uint32_t size=0;
    if(!StringAt(address,bytes,size,d))return false;
    if(!size)return true;
    const auto path=Vfs(std::span(bytes).first(size),1);
    if(path.empty())return FailCapture(d,CaptureFailure::ResourcePending,{facts.subject});
    NativeResource value;
    value.table=1;
    value.kind=state::kResourceImagePathKind;
    return Intern(value,path,{},out,d);
  });
}

// 图像选择还须验证控制块存在。图集等程序资源继续走当前 owner 解析。
bool EntitySource::Impl::LayerImage(SpriteLayerResourceMemo *memo,const std::array<std::uint32_t,2> &head,
    CaptureLink &out,CaptureDiagnostic &d) noexcept {
  if(head[0]&&!head[1])return FailCapture(d,CaptureFailure::ResourcePending,{facts.subject,0,0,evidence_layout},evidence_field);
  if(!(resource_cache.observed&2u))memo=nullptr;
  const auto epoch=resource_cache.image_epoch;
  if(memo&&memo->image_ready&&memo->image_epoch==epoch&&memo->image_head==head){
    out=memo->image;++stats.resource_instance_hits[unsigned(NativeResourceSource::Image)];return true;
  }
  if(!ImageResource(head[0],out,d))return false;
  if(memo&&(out.kind==CapturedReferenceKind::Null||resources[std::size_t(out.handle-1)].kind==1)){
    memo->image_ready=true;memo->image_epoch=epoch;memo->image_head=head;memo->image=out;
  }
  return true;
}
bool EntitySource::Impl::LayerImagePath(SpriteLayerResourceMemo *memo,std::uint32_t address,
    std::span<const std::byte> head,CaptureLink &out,CaptureDiagnostic &d) noexcept {
  if(!memo||head.size()!=memo->path_head.size())return ImagePathResource(address,out,d);
  if(memo->path_ready&&std::equal(head.begin(),head.end(),memo->path_head.begin())){
    out=memo->path;++stats.resource_instance_hits[unsigned(NativeResourceSource::ImagePath)];return true;
  }
  if(!ImagePathResource(address,out,d))return false;
  std::copy(head.begin(),head.end(),memo->path_head.begin());
  memo->path_ready=true;memo->path=out;
  return true;
}

bool EntitySource::Impl::ConfigResource(std::uint32_t pointer,CaptureLink &out,CaptureDiagnostic &d) noexcept {
  out={CapturedReferenceKind::Null,0};
  if(!pointer)return true;
  return ResourceOnce({NativeResourceSource::Config,pointer},out,[&]{
    NativeResource value;
    value.table=4;
    value.kind=8;
    if(pointer>UINT32_MAX-0x8c)return FailCapture(d,CaptureFailure::AddressRange,{facts.subject});
    for(unsigned i=0;i<3;++i)if(!Value(std::uint64_t(pointer)+4*i,value.values[i],d))return false;
    if(!Value(std::uint64_t(pointer)+0x64,value.values[3],d) || !Value(std::uint64_t(pointer)+0x24,value.values[4],d) ||
       !std::isfinite(std::bit_cast<float>(value.values[4])))return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
    CaptureLink file;
    if(!FileResource(pointer+0x74,false,file,d))return false;
    value.links[0]=file.handle;
    return Intern(value,{},{},out,d);
  });
}

bool EntitySource::Impl::Resource(const CaptureFieldAccess &a,CaptureLink &out,CaptureDiagnostic &d) noexcept {
  const auto address=std::uint64_t(a.root.address)+a.field.native_offset;const auto recipe=Recipe(a.field);
  if(address>UINT32_MAX)return Reject(a,d,CaptureFailure::AddressRange);
  out={CapturedReferenceKind::Null,0};
  if(recipe==EntityRead::File){
    if(const auto *sprite=PlayerSprite(a.location.parents);sprite&&sprite->address==address){out=sprite->file;return true;}
    return FileResource(std::uint32_t(address),true,out,d);
  }
  if(recipe==EntityRead::ImagePath)return ImagePathResource(std::uint32_t(address),out,d);
  if(recipe==EntityRead::Image){if(PlayerLayerImage(a,out))return true;
    std::array<std::uint32_t,2> pair{};
    if(!Tracked(address,pair,d) || (pair[0]&&!pair[1]) || !ImageResource(pair[0],out,d))
      return Reject(a,d,CaptureFailure::SourceChanged);return true;}
  if(recipe==EntityRead::Animation){std::uint32_t owner=0,animation=0;std::uint8_t loaded=0;
    if(const auto *sprite=PlayerAnimation(a)){
      return ReadPlayerAnimation(a,*sprite,owner,animation,d)&&AnimationResource(owner,animation,out,d,owner==sprite->address?&sprite->file:nullptr);
    }
    if(!Value(a.root.address,owner,d) || !owner || !Value(std::uint64_t(owner)+0x109,loaded,d) || loaded>1)return Reject(a,d,CaptureFailure::InvalidValue);
    if(!loaded)return true;if(!Value(address,animation,d))return false;return AnimationResource(owner,animation,out,d);}
  if(recipe==EntityRead::Item){if(const auto *costume=PlayerCostume(a)){out=costume->item;return true;}}
  std::uint32_t pointer=0;if(!Value(address,pointer,d))return false;if(!pointer)return true;
  if(recipe==EntityRead::RoomConfig){NativeResource value;value.table=5;value.kind=3;
    if(!facts.scene || pointer>UINT32_MAX-92)return Reject(a,d,CaptureFailure::MissingQualification);
    for(unsigned i=0;i<3;++i)if(!Tracked(std::uint64_t(pointer)+i*8,value.values[i],d))return false;
    std::array<std::int8_t,2> dimensions{};std::int32_t shape=0;
    if(!Tracked(std::uint64_t(pointer)+70,dimensions,d) || !Tracked(std::uint64_t(pointer)+72,shape,d) || shape<1 || shape>12 || dimensions[0]<=0 || dimensions[1]<=0)
      return Reject(a,d,CaptureFailure::InvalidValue);
    value.values[3]=std::uint8_t(dimensions[0]);value.values[4]=std::uint8_t(dimensions[1]);value.values[5]=std::uint32_t(shape);
    for(std::uint32_t i=0;i<config_owner_used;++i)if(config_owners[i].address==pointer && config_owners[i].run==window.witness.scope.run){value.source_owner=config_owners[i].owner;break;}
    if(!value.source_owner){if(config_owner_used==config_owners.size())return Reject(a,d,CaptureFailure::ResourcePending);
      value.source_owner=std::uint64_t(config_owner_used)+1;config_owners[config_owner_used++]={pointer,window.witness.scope.run,value.source_owner};}
    return Intern(value,{},{},out,d);}
  if(recipe==EntityRead::Item)return ItemResource(pointer,out,d);
  if(recipe==EntityRead::QueueItem){std::uint32_t type=0;if(!Value(pointer,type,d))return false;return type!=2 || ItemResource(pointer,out,d);}
  if(recipe==EntityRead::Config)return ConfigResource(pointer,out,d);
  return Reject(a,d,CaptureFailure::MissingHandler);
}
}
