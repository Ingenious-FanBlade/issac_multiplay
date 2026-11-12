#include "world/binary/entity_source_internal.h"

namespace cs::world::binary {
EntityRead PresentationRecipe(const Field &field) noexcept {
  if(!((field.layout>=310&&field.layout<=383)||(field.layout>=422&&field.layout<=424)||field.layout==435||field.layout==436))return EntityRead::None;
  if(!field.handler || !field.handler[0])return EntityRead::None;
  if(field.source==FieldSource::Object){
    if(field.operation==Operation::Resource && std::string_view(field.target)=="image-table")return EntityRead::Image;
    if(field.operation==Operation::Container){if(field.fixed_count)return EntityRead::Fixed;
      if(std::string_view(field.storage)=="vector")return EntityRead::Vector;}
  }
  return EntityRead::Presentation;
}
// 普通图标须同时匹配具名范围、真实槽状态、配置类别及实际文件图选择；特殊值绝不折空。
bool EntitySource::Impl::HudIcon(const CaptureFieldAccess &a,std::uint8_t &source,CaptureDiagnostic &d) noexcept {
  if(!facts.identity_address || !facts.hud || !facts.manager)return Reject(a,d,CaptureFailure::MissingQualification);
  const bool is_active=a.field.layout==311;const auto first=std::uint64_t(facts.hud)+(is_active?400:512);const auto stride=is_active?28u:24u;
  if(a.root.address<first || (a.root.address-first)%stride)return Reject(a,d,CaptureFailure::InvalidValue);
  const auto slot=(a.root.address-first)/stride;if(slot>=(is_active?4u:2u))return Reject(a,d,CaptureFailure::InvalidValue);
  std::int32_t visual=0,id=0;std::array<std::uint32_t,2> image{};
  if(!Value(a.root.address,visual,d) || !Value(std::uint64_t(a.root.address)+8,image,d) || (image[0]&&!image[1]) ||
     !Value(std::uint64_t(facts.identity_address)+(is_active?0x1574+slot*28:0x16b0+slot*4),id,d))return false;
  if(!is_active)id=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(id)&~0x8000u);
  if(!id && !visual && !image[0]){source=0;return true;}
  source=2;const auto kind=is_active?HudIconKind::Active:HudIconKind::Trinket;
  if(id!=visual || !HudOrdinaryIcon(kind,id,unsigned(slot)))return true;
  // Manager内嵌ItemConfig的既有向量，只读选择，不调用GetCollectible/GetTrinket或加载图像。
  std::array<std::uint32_t,3> vector{};std::uint32_t config=0;CaptureLink selected,configuration;
  if(!Tracked(std::uint64_t(facts.manager)+0x2a234+(is_active?0:12),vector,d) || vector[0]>vector[1] || vector[1]>vector[2] ||
     (vector[1]-vector[0])%4 || std::uint32_t(id)>=(vector[1]-vector[0])/4 ||
     !Tracked(std::uint64_t(vector[0])+std::uint64_t(id)*4,config,d) || !config)return Reject(a,d,CaptureFailure::ResourcePending);
  if(!ItemResource(config,configuration,d) || !ImageResource(image[0],selected,d))return false;
  if(!configuration.handle || !selected.handle)return true;
  const auto &item=resources[std::size_t(configuration.handle-1)],&actual=resources[std::size_t(selected.handle-1)];
  if(item.values[0]!=(is_active?3u:2u) || !item.links[0] || actual.kind!=1)return true;
  const auto &declared=resources[std::size_t(item.links[0]-1)];
  const bool equal=declared.kind==1 && declared.text_bytes==actual.text_bytes &&
    std::equal(resource_bytes.begin()+declared.text,resource_bytes.begin()+declared.text+declared.text_bytes,resource_bytes.begin()+actual.text);
  source=std::uint8_t(SelectHudIconSource(kind,unsigned(slot),id,visual,false,item.values[0],equal));return true;
}
// 协议视图只借用具名正常生产owner；synthetic偏移绝不加到根对象。
bool EntitySource::Impl::PresentationLeaf(const CaptureFieldAccess &a,CaptureValueWriter &writer,CaptureDiagnostic &d) noexcept {
  const auto id=a.field.layout,index=a.field.index;
  const auto scalar=[&](std::span<const std::byte> bytes){return writer.PutScalar(a.location,a.format.scalar,a.format.components,bytes);};
  const auto number=[&](std::uint64_t n){std::array<std::byte,8> bytes{};const auto width=CaptureScalarWidth(a.format.scalar);
    if(!width||width>8||(width<8&&(n>>(width*8))))return Reject(a,d,CaptureFailure::InvalidValue);
    for(unsigned i=0;i<width;++i)bytes[i]=std::byte(n>>(8*i));return scalar(std::span(bytes).first(width));};
  if(id==311 || id==312){if(index==0){std::uint8_t source=0;return HudIcon(a,source,d)&&number(source);}
    if(index==1){std::int32_t visual=0;if(!Value(a.root.address,visual,d))return false;return scalar(std::as_bytes(std::span(&visual,1)));}
    if(id==312&&index==2){std::uint32_t value=0;return Value(std::uint64_t(a.root.address)+4,value,d)&&number(value!=0);}}
  if((id==435||id==436||id==317||id==321)&&index==0){const auto link=services.reference(services.owner,facts.identity_address);
    if(link.kind!=CapturedReferenceKind::Identity && link.kind!=CapturedReferenceKind::Pending)return Reject(a,d,CaptureFailure::ReferencePending);
    return writer.PutLinks(a.location,false,255,std::span(&link,1));}
  if(id==320 && index==1){CaptureLink link;
    if(facts.history_from_player){if(!Value(std::uint64_t(a.root.address)+offsetof(NativeHistoryGlyph,image),link,d))return false;}
    else {std::array<std::uint32_t,2> pair{};if(!Tracked(std::uint64_t(a.root.address)+4,pair,d) || (pair[0]&&!pair[1]) || !ImageResource(pair[0],link,d))return false;}
    return writer.PutLinks(a.location,true,a.format.resource_table,std::span(&link,1));}
  if(id==435){if(!facts.hud_clock)return Reject(a,d,CaptureFailure::MissingQualification);const auto &c=*facts.hud_clock;
    switch(index){case 1:return number(c.binding);case 2:return number(c.update);case 3:return number(c.game_frame);case 4:return number(c.critical_health);
      case 5:return number(c.mantle_revision);case 6:return number(c.drop_anchor);case 7:return number(c.drop_held);case 8:return number(c.drop_phase);default:break;}}
  if(id==353){if(index<4){std::uint8_t value=0;if(!Value(std::uint64_t(a.root.address)+1+index,value,d))return false;
      return scalar(std::as_bytes(std::span(&value,1)));}
    std::uint32_t name=0,animation=0;std::array<std::byte,256> text{};std::uint32_t count=0;CaptureLink link;
    // 主／overlay名称在当前HUD共享hearts ANM2的既有动画目录中消解，不执行SetFrame或动画Getter。
    if(!facts.hud_resources || !Value(std::uint64_t(a.root.address)+(index==4?8:12),name,d) || !name ||
       !CStringAt(name,256,text,count,d) || !NamedAnimation(facts.hud_resources+14784,std::string_view(reinterpret_cast<const char *>(text.data()),count),animation,d) ||
       !AnimationResource(facts.hud_resources+14784,animation,link,d))return false;
    return writer.PutLinks(a.location,true,a.format.resource_table,std::span(&link,1));}
  if(id==354 && index==0){if(!facts.planetarium_known)return Reject(a,d,CaptureFailure::MissingQualification);return number(facts.planetarium_qualified);}
  if(id==356){if(!facts.draw_clock)return Reject(a,d,CaptureFailure::MissingQualification);
    return number(index==0?facts.draw_clock->manager_frame:std::uint32_t(facts.draw_clock->paused));}
  if(id==379){if(!facts.screen)return Reject(a,d,CaptureFailure::MissingQualification);return number(facts.screen->present);}
  if(id>=367 && id<=371){if(!facts.screen)return Reject(a,d,CaptureFailure::MissingQualification);
    if(id==371)return number(facts.screen->enabled);if(index==0)return number(facts.screen->present);
    return scalar(std::as_bytes(std::span(&facts.screen->amount,1)));}
  if(id==381 || id==382 || id==383){if(!facts.audio)return Reject(a,d,CaptureFailure::MissingQualification);const auto &v=*facts.audio;
    if(id==382)return number(v.start_frame);if(id==383)return number(v.kind);
    if(index==0)return writer.PutLinks(a.location,true,7,std::span(&v.asset,1));
    if(index>=2 && index<=4){const float value=index==2?v.gain:index==3?v.pitch:v.pan;return scalar(std::as_bytes(std::span(&value,1)));}
    switch(index){case 1:return number(v.volume_class);case 5:return number(v.loop);case 6:return number(v.paused);case 7:return number(v.segment);case 8:return number(v.mix_rule);default:break;}}
  return Reject(a,d,CaptureFailure::MissingHandler);
}
bool EntitySource::Impl::PresentationOpen(const CaptureFieldAccess &a,CaptureTraversalSource &out,CaptureDiagnostic &d) noexcept {
  out={};out.identity=a.root.address;std::uint64_t address=a.root.address;
  if(a.field.layout==310){} // 可见分支已经由同源selector判定，不读隐藏旧尾值。
  else if(a.field.layout==317){if(facts.stat_rows)address=reinterpret_cast<std::uintptr_t>(facts.stat_rows->data());else address+=4;}
  else if(a.field.layout==354 && a.field.index>=1 && a.field.index<=9)address+=std::uint64_t(a.field.index-1)*20;
  else if(a.field.layout==382 || a.field.layout==383){}
  else if(a.field.layout==379)address+=8;
  else if(a.field.layout==321)return HistoryGlyphs(a,out,d);
  else return Reject(a,d,CaptureFailure::MissingHandler);
  const auto *layout=FindLayout(a.field.nested_layout);if(!layout || address>UINT32_MAX)return Reject(a,d,CaptureFailure::UnknownLayout);
  out.opaque[0]=address;out.opaque[1]=std::max(layout->native_size,1u);out.opaque[2]=witness_used;out.count=1;return true;
}
// 声音资源只用正常已选sample路径；文件字节、PCM与私有选样本RNG不上线。
bool EntitySource::AudioResource(const CaptureWitness &w,EntityFacts f,std::uint32_t sample,CaptureLink &out,CaptureDiagnostic &d) noexcept {
  auto &s=*impl_;if(s.active || std::this_thread::get_id()!=s.thread)return FailCapture(d,CaptureFailure::Reentrant,{f.subject});
  s.facts=f;s.active=true;s.scratch_used=s.witness_used=0;
  struct End{bool &active;~End(){active=false;}}end{s.active};
  s.window={s.thread,w,&s,[](void *p,const CaptureWitness &v,const CaptureRoot &r) noexcept {return static_cast<Impl *>(p)->Current(v,r);},nullptr};
  std::array<std::byte,4096> path{};std::uint32_t size=0;
  if(!s.StringAt(sample,path,size,d,true) || !size)return FailCapture(d,CaptureFailure::ResourcePending,{f.subject});
  for(auto &b:std::span(path).first(size))if(b==std::byte{'\\'})b=std::byte{'/'};
  const std::string_view text(reinterpret_cast<const char *>(path.data()),size);
  if(!(text.starts_with("sfx/")||text.starts_with("music/")) || text.find("..")!=text.npos || text.find_first_of(":<>\"|?*")!=text.npos ||
     !(text.ends_with(".wav")||text.ends_with(".ogg")||text.ends_with(".mp3")))return FailCapture(d,CaptureFailure::ResourcePending,{f.subject});
  std::array<std::byte,32> check{};
  for(std::uint32_t i=0;i<s.witness_used;++i){const auto &mark=s.witnesses[i];if(!s.Read(mark.address,std::span(check).first(mark.size),d)||
      !std::equal(check.begin(),check.begin()+mark.size,mark.bytes.begin()))return FailCapture(d,CaptureFailure::SourceChanged,{f.subject});}
  NativeResource resource;resource.table=7;resource.kind=9;resource.values[0]=0;resource.values[1]=0;
  return s.Intern(resource,std::span(path).first(size),{},out,d);
}
}
