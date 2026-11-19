#include "world/binary/entity_source_internal.h"
#include <cmath>

namespace cs::world::binary {
// 冷绑定全部Grid特殊操作；嵌套状态仍交同源执行器，绝不按Desc.Type猜动态类。
EntityRead GridRecipe(std::string_view name) noexcept {
  if(name=="read_grid_final_water_clip_flags")return EntityRead::GridWater;
  if(name=="read_grid_sprite_glitch_draw_bits")return EntityRead::Glitch;
  if(name=="read_grid_rock_has_main_pose")return EntityRead::RockPose;
  if(name=="read_grid_rock_layer_has_position")return EntityRead::RockPosition;
  if(name=="read_grid_rock_visible_rubble")return EntityRead::RockRubble;
  if(name=="grid_rock_rubble_to_resource")return EntityRead::RockAnimation;
  if(name=="read_grid_fire_visible_embers")return EntityRead::FireEmbers;
  if(name=="read_grid_door_draw_enabled")return EntityRead::DoorAlpha;
  if(name=="read_grid_door_light_bit")return EntityRead::DoorLight;
  if(name=="read_grid_rock_layers")return EntityRead::Layers;
  if(name=="read_grid_rock_main_pose" || name=="read_grid_rock_layer_position")return EntityRead::Inline;
  return EntityRead::None;
}
// 碎块动画只在state2且主文件实际存在时解析；遍历已加载的真实动画目录，不执行GetAnimation。
bool EntitySource::Impl::RockAnimation(CaptureLink &out,CaptureDiagnostic &d) noexcept {
  out={CapturedReferenceKind::Null,0};std::int32_t state=0;std::uint8_t loaded=0;
  if(!Value(std::uint64_t(facts.address)+12,state,d))return false;
  if(state!=2)return true;
  if(!Tracked(std::uint64_t(facts.address)+0x149,loaded,d) || loaded>1)return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
  if(!loaded)return true;
  std::array<std::byte,4096> name{},candidate{};std::uint32_t size=0;
  if(!StringAt(facts.address+0x16c,name,size,d,true) || !size)return FailCapture(d,CaptureFailure::ResourcePending,{facts.subject});
  std::array<std::uint32_t,2> animations{};
  if(!Tracked(std::uint64_t(facts.address)+0xb4,animations,d) || !animations[0] || !animations[1] || animations[1]>65535)
    return FailCapture(d,CaptureFailure::ResourcePending,{facts.subject});
  std::uint32_t selected=0;
  for(std::uint32_t i=0;i<animations[1];++i){const auto address=std::uint64_t(animations[0])+std::uint64_t(i)*0x13c;std::uint32_t length=0;
    if(address>UINT32_MAX || !StringAt(std::uint32_t(address),candidate,length,d,true))return false;
    if(length==size && std::equal(name.begin(),name.begin()+size,candidate.begin())){
      if(selected)return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});selected=std::uint32_t(address);}}
  std::int32_t frames=0;if(!selected || !Value(std::uint64_t(selected)+0x30,frames,d) || frames<=0)
    return FailCapture(d,CaptureFailure::ResourcePending,{facts.subject});
  return AnimationResource(facts.address+0x40,selected,out,d);
}
bool EntitySource::Impl::GridLeaf(const CaptureFieldAccess &a,EntityLeafSink writer,CaptureDiagnostic &d) noexcept {
  std::uint32_t value=0;const auto recipe=Recipe(a.field);
  if(recipe==EntityRead::RockAnimation){CaptureLink link;if(!RockAnimation(link,d))return false;return writer.PutLinks(a.location,true,a.format.resource_table,std::span(&link,1));}
  if(recipe==EntityRead::GridWater){
    switch(root_layout){
      case 199:case 207:case 210:case 213:case 219:case 220:case 221:value=57;break;
      case 206:case 208:case 209:case 211:case 212:case 218:value=38;break;
      case 228:value=0;break;case 227:value=25;break;
      case 222:case 223:{std::int32_t state=0;if(!Value(std::uint64_t(facts.address)+12,state,d))return false;value=state==(root_layout==222?1000:2)?38u:57u;break;}
      case 217:{std::uint32_t collision=0;if(!Value(std::uint64_t(facts.address)+0x3c,collision,d))return false;value=collision?57u:38u;break;}
      default:return Reject(a,d,CaptureFailure::UnknownLayout);
    }
    // Lock没有官方水面Hook；其余六族先读engaged，仅设置后读取完整u32。
    if(root_layout!=217){if(!capacity.grid_plus || facts.grid_index>=448)return Reject(a,d,CaptureFailure::MissingQualification);
      const auto slot=std::uint64_t(capacity.grid_plus)+facts.grid_index*12u;std::uint8_t engaged=0;
      std::uint32_t vtable=0;if(!capacity.grid_plus_vtable || !Tracked(slot,vtable,d) || vtable!=capacity.grid_plus_vtable)return Reject(a,d,CaptureFailure::MissingQualification);
      if(!Tracked(slot+8,engaged,d) || engaged>1)return Reject(a,d,CaptureFailure::InvalidValue);
      if(engaged && !Tracked(slot+4,value,d))return false;}
  }else if(recipe==EntityRead::Glitch){
    // J273 SetSeed(index35)为5/9/7；只推进栈上u32，两次RandomInt(2)等价低位。
    auto seed=a.root.address;const auto next=[&]() {seed^=seed>>5;seed^=seed<<9;seed^=seed>>7;return seed;};
    const auto first=next()&1u;const auto second=next()&1u;value=first|(second<<1);
  }else if(recipe==EntityRead::RockPose || recipe==EntityRead::RockPosition){std::int32_t state=0;
    if(root_layout!=223 || !Value(std::uint64_t(facts.address)+12,state,d))return Reject(a,d,CaptureFailure::UnknownLayout);
    value=state!=2?1u:0u;
    if(recipe==EntityRead::RockPosition){if(a.location.parents.empty())return Reject(a,d,CaptureFailure::InvalidTransaction);
      const auto &step=a.location.parents.back();if(step.element==kNullIndex)return Reject(a,d,CaptureFailure::InvalidTransaction);
      value=state==2 && !step.element?0u:1u;}
  }else if(recipe==EntityRead::DoorAlpha){float alpha=0;if(!Value(std::uint64_t(a.root.address)+a.field.native_offset,alpha,d))return false;
    if(!std::isfinite(alpha))return Reject(a,d,CaptureFailure::InvalidValue);value=alpha>0?1u:0u;
  }else if(recipe==EntityRead::DoorLight){std::uint32_t flags=0;if(!Value(std::uint64_t(a.root.address)+a.field.native_offset,flags,d))return false;value=(flags&0x20)?1u:0u;
  }else return Reject(a,d,CaptureFailure::MissingHandler);
  std::array<std::byte,4> bytes{};const auto width=CaptureScalarWidth(a.format.scalar);
  if((width!=1 && width!=4) || (width==1 && value>255))return Reject(a,d,CaptureFailure::InvalidValue);
  for(unsigned i=0;i<width;++i)bytes[i]=std::byte(value>>(8*i));
  return writer.PutScalar(a.location,a.format.scalar,1,std::span(bytes).first(width));
}
// 可见前缀和碎块分支在读vector之前确定；不读capacity尾、无效粒子或旧字符串。
bool EntitySource::Impl::GridOpen(const CaptureFieldAccess &a,CaptureTraversalSource &out,CaptureDiagnostic &d) noexcept {
  const auto start=witness_used;out={};out.identity=a.root.address;
  std::int32_t state=0;if(!Tracked(std::uint64_t(facts.address)+12,state,d))return false;
  const auto recipe=Recipe(a.field);bool present=state<4;
  if(recipe==EntityRead::RockRubble){std::uint8_t loaded=0;present=state==2;
    if(present && (!Tracked(std::uint64_t(facts.address)+0x149,loaded,d) || loaded>1))return Reject(a,d,CaptureFailure::InvalidValue);
    present=present&&loaded;}
  std::array<std::uint32_t,3> header{};
  const auto address=std::uint64_t(a.root.address)+a.field.native_offset;
  if(present){if(!Tracked(address,header,d))return false;const auto stride=a.field.native_stride;
    if(!stride || header[0]>header[1] || header[1]>header[2] || (!header[0]&&(header[1]||header[2])) ||
       (header[1]-header[0])%stride || (header[2]-header[0])%stride)return Reject(a,d,CaptureFailure::InvalidValue);
    out.count=(header[1]-header[0])/stride;if(out.count>a.field.limit)return Reject(a,d,CaptureFailure::CountMismatch);
    if(recipe==EntityRead::FireEmbers)for(std::uint32_t i=0;i<out.count;++i){std::array<std::int32_t,2> life{};
      if(!Tracked(std::uint64_t(header[0])+i*24u+16,life,d))return false;
      if(life[0]<0 || life[0]>=life[1] || life[1]>29)return Reject(a,d,CaptureFailure::InvalidValue);}}
  out.opaque={header[0],a.field.native_stride,start,witness_used-start};out.identity=address;return true;
}
}
