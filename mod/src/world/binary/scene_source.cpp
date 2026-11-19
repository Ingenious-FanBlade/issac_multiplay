#include "world/binary/entity_source_internal.h"
#include <cmath>

namespace cs::world::binary {
#include "state_host_fields.inc"
EntityRead SceneRecipe(std::string_view name) noexcept {
  if(name=="read_room_wave_sprite_layers")return EntityRead::Layers;
  if(name=="room_descriptor_data_read" || name=="room_descriptor_override_data_read")return EntityRead::RoomConfig;
  if(name.starts_with("room_") || name.starts_with("backdrop_") || name.starts_with("f_x_") ||
     name.starts_with("ray_") || name.starts_with("vein_") || name.starts_with("rail_manager_") ||
     name.starts_with("level_game_prefix_") || name.starts_with("water_") || name.starts_with("forced_camera_") ||
     name.starts_with("selected_ray_") || name.starts_with("poop_visual_") || name.starts_with("saved_map_") ||
     name.starts_with("final_saved_map_") || name.starts_with("current_descriptor_visual_"))return EntityRead::Scene;
  return EntityRead::None;
}
// Seeds只读既有树，全部cursor留当前同步栈；不调用Getter／构造／插入或随机。
bool EntitySource::ReadSceneContext(const CaptureWitness &w,EntityFacts f,NativeScene &scene,CaptureDiagnostic &d) noexcept {
  auto &s=*impl_;if(s.active || std::this_thread::get_id()!=s.thread)return FailCapture(d,CaptureFailure::InvalidWindow,{f.subject});
  s.facts=f;s.active=true;s.scratch_used=s.witness_used=0;
  s.scene_scan_remaining=262144;
  struct End {bool &active;~End(){active=false;}} end{s.active};
  s.window={s.thread,w,&s,[](void *p,const CaptureWitness &v,const CaptureRoot &r) noexcept {return static_cast<Impl *>(p)->Current(v,r);},nullptr};
  std::array<std::uint32_t,80> nodes{};std::uint32_t count=0;
  // J273 的 Seeds 内嵌在 Game+0x1BB84，效果树位于 Seeds+0x54。
  constexpr std::uint32_t seed_effect_tree=0x1bb84+0x54;
  if(scene.game>UINT32_MAX-seed_effect_tree || !s.NativeTree(scene.game+seed_effect_tree,nodes,count,d))return false;
  scene.seed_effects={};
  scene.seed_effect_count=count;
  std::int32_t previous=-1;
  for(std::uint32_t i=0;i<count;++i){std::int32_t effect=0;if(!s.Tracked(std::uint64_t(nodes[i])+16,effect,d))return false;
    if(effect<0 || effect>=int(scene.seed_effects.size()) || effect<=previous)return FailCapture(d,CaptureFailure::InvalidValue,{f.subject});
    scene.seed_effects[std::size_t(effect)]=true;scene.seed_effect_ids[i]=std::uint32_t(effect);previous=effect;}
  std::uint32_t raw=0,temporary=0,stage=0,difficulty=0,challenge=0;
  if(!s.Value(std::uint64_t(scene.game)+12,raw,d) || !s.Value(std::uint64_t(scene.game)+0x25ac8,temporary,d) ||
     !s.Value(scene.game,stage,d) || !s.Value(std::uint64_t(scene.game)+0x25f40,difficulty,d) ||
     !s.Value(std::uint64_t(scene.game)+0x25afc,challenge,d))return false;
  constexpr std::uint32_t masks[]={1,2,4,8,0x20,0x40,0x10};std::uint32_t forced=0,prevented=0;
  for(unsigned i=0;i<7;++i)if(scene.seed_effects[56+i] && (i!=1 || (challenge!=44 && std::bit_cast<std::int32_t>(stage)<8 && (stage&1) && difficulty!=2 && difficulty!=3)))forced|=masks[i];
  for(unsigned i=0;i<6;++i)if(scene.seed_effects[63+i])prevented|=masks[i];
  if(scene.seed_effects[70])prevented=0x7f;if(scene.seed_effects[79])prevented|=0x40;
  scene.effective_curses=(raw|temporary|forced)&~prevented;scene.greed=difficulty==2 || difficulty==3;
  return s.Verify({f.subject,f.address,0,f.layout},d);
}

// 所有转换按数字布局／字段解释。纯协议视图的address是具名owner的同步借用，不是偏移基址。
bool EntitySource::Impl::SceneLeaf(const CaptureFieldAccess &a,CaptureValueWriter &writer,CaptureDiagnostic &d) noexcept {
  if(!facts.scene)return Reject(a,d,CaptureFailure::MissingQualification);const auto &scene=*facts.scene;
  const auto id=a.field.layout,field=a.field.index;const auto address=std::uint64_t(a.root.address)+a.field.native_offset;
  const auto scalar=[&](std::span<const std::byte> bytes){return writer.PutScalar(a.location,a.format.scalar,a.format.components,bytes);};
  const auto number=[&](std::uint64_t value){std::array<std::byte,8> bytes{};const auto width=CaptureScalarWidth(a.format.scalar);
    if(!width || width>8 || (width<8 && (value>>(width*8))))return Reject(a,d,CaptureFailure::InvalidValue);
    for(unsigned i=0;i<width;++i)bytes[i]=std::byte(value>>(8*i));return scalar(std::span(bytes).first(width));};
  const auto native=[&](std::uint64_t p){std::array<std::byte,64> bytes{};const auto width=CaptureScalarWidth(a.format.scalar)*a.format.components;
    if(width>bytes.size() || !Read(p,std::span(bytes).first(width),d))return false;return scalar(std::span(bytes).first(width));};
  const auto image=[&](std::uint64_t p){std::array<std::uint32_t,2> pair{};CaptureLink link;
    if(!Tracked(p,pair,d) || (pair[0]&&!pair[1]) || !ImageResource(pair[0],link,d))return false;
    return writer.PutLinks(a.location,true,a.format.resource_table,std::span(&link,1));};
  if(a.field.operation==Operation::Container)return ScalarContainer(a,{&writer},d);
  if(Recipe(a.field)==EntityRead::RoomConfig){CaptureLink link;if(!Resource(a,link,d))return false;return writer.PutLinks(a.location,true,a.format.resource_table,std::span(&link,1));}
  if(id==239){
    if(field==0){for(std::uint32_t i=0;i<scene.descriptor_count;++i){const auto slot=scene.descriptor_order[i];if(scene.descriptors[slot]==scene.current_descriptor)return number(slot);}return Reject(a,d,CaptureFailure::ReferencePending);}
    if(field==1)return number(scene.dimension);
    if(field==13){std::array<CaptureLink,8> links{};
      for(unsigned i=0;i<8;++i)links[i]=scene.grids[448+i]?CaptureLink{CapturedReferenceKind::Identity,scene.grid_handles[448+i]}:CaptureLink{CapturedReferenceKind::Null,0};
      for(const auto link:links)if(link.kind!=CapturedReferenceKind::Null&&!link.handle)return Reject(a,d,CaptureFailure::ReferencePending);
      return writer.PutLinks(a.location,false,255,links);}
    if(field==host_field::Room::wave_sprite_present){std::uint32_t mask=0;if(!WaveSpriteMask(mask,d))return false;return number(mask?1u:0u);}
    if(field==host_field::Room::grid_path_block){
      // 预测插值 5.3.1：按 Room::GetGridCollision（0x7E6AF0）的阈值把 Room+0x76C 的 448 格寻路 cost
      // 派生为每格 2 位碰撞类别（0无、1PIT、3SOLID），低位在前；原始 cost 不发送。
      std::array<std::int32_t,448> cost{};
      if(!Read(std::uint64_t(a.root.address)+0x76c,std::as_writable_bytes(std::span(cost)),d))return false;
      std::array<std::byte,112> packed{};
      for(unsigned i=0;i<cost.size();++i){
        const auto c=cost[i];
        const unsigned kind=c>=3000?(c<=3999?1u:3u):(c>=1000?3u:0u);
        packed[i/4]|=std::byte(kind<<((i%4)*2));
      }
      return scalar(packed);
    }
  }
  if(id==242 || id==253){if((id==242&&field==0)||(id==253&&field==0)){std::uint8_t enabled=0;if(!Value(address,enabled,d) || enabled>1)return Reject(a,d,CaptureFailure::InvalidValue);return number(enabled);}
    if(id==253&&(field==host_field::RoomHellBackdrop::display_offset||field==host_field::RoomHellBackdrop::accumulated_offset))return native(address);}
  if(id==240 && field==host_field::Backdrop::sprite_mask){std::uint32_t backdrop=0;if(!Tracked(std::uint64_t(a.root.address)+336,backdrop,d) || backdrop>=63)return Reject(a,d,CaptureFailure::InvalidValue);
    // 墙细节已经烘入recipe；教程控件由本端C/S绑定显示，不下发Host设备按钮层。
    return number(backdrop==18 || backdrop==26 || backdrop==35 || (backdrop>=52 && backdrop<=54)?1u:0u);}
  if(id==270){
    if(field==host_field::RoomDescriptor::spawn_seed_current)return number(a.root.address==scene.current_descriptor);
    if(field==host_field::RoomDescriptor::spawn_seed){if(a.root.address!=scene.current_descriptor)return Reject(a,d,CaptureFailure::InvalidTransaction);return native(address);}
    if(field==2 || field==5){std::uint32_t v=0;if(!Value(address,v,d) || v>(field==2?2u:255u))return Reject(a,d,CaptureFailure::InvalidValue);return number(v);}
    if(field==8){std::int32_t count=0;if(!Value(address,count,d))return false;return number(count?1u:0u);}
    if(field==host_field::RoomDescriptor::flags){
      std::uint32_t flags=0;if(!Value(address,flags,d))return false;
      // 保留背景、灯光与坑面实际读取的位，包括祭祀房的原生光照条件。
      return number((flags&1)|((flags>>9)&6)|((flags>>15)&8)|((flags>>1)&16)|
        ((flags>>4)&32)|((flags>>6)&64)|((flags<<5)&128));
    }
    if(field==10){if(!scene.greed)return number(0);std::int32_t special=0,index=0;std::uint32_t config=0,type=0;
      if(!Value(std::uint64_t(scene.game)+0x18370,special,d) || !Value(a.root.address,index,d) || !Value(std::uint64_t(a.root.address)+16,config,d))return false;
      if(config&&!Value(std::uint64_t(config)+8,type,d))return false;return number(config&&type==4&&index==special?1u:0u);}
  }
  if(id==276){if(field==host_field::LevelGamePrefix::current_dimension)return number(scene.dimension);
    if(field==host_field::LevelGamePrefix::greed_wave){if(!scene.greed)return number(0);return native(address);}
    if(field==host_field::LevelGamePrefix::current_is_last_boss){std::int32_t list=0,boss=0;if(!Value(std::uint64_t(scene.current_descriptor)+8,list,d) || !Value(std::uint64_t(scene.game)+0x18314,boss,d))return false;return number(list==boss?1u:0u);}
    if(field==host_field::LevelGamePrefix::room_offset)return scalar(std::as_bytes(std::span(scene.occupancy)));}
  if(id==299){std::uint32_t seed=0;if(!Value(std::uint64_t(scene.current_descriptor)+0x5c,seed,d))return false;
    if(!seed)return number(0);seed^=seed>>2;seed^=seed<<7;seed^=seed>>25;return number(seed%189+1==166?1u:0u);}
  if(id==245){if(field==1)return image(address);if(field==3){std::uint8_t v=0;if(!Value(address,v,d)||v>1)return Reject(a,d,CaptureFailure::InvalidValue);return number(v);}
    if(field==4){float v=0;if(!Value(address,v,d)||!std::isfinite(v))return Reject(a,d,CaptureFailure::InvalidValue);return number(v!=0?1u:0u);}
    if(field==5){std::uint32_t v=0;if(!Value(address,v,d))return false;return number(v==35?1u:0u);}
    if(field==6)return native(address);}
  if(id==246){if(field==2)return image(address);if(field==1)return native(address);}
  if(id==434){const auto name=std::string_view(a.field.name);
    if(name=="active_mask"){std::uint32_t mask=0;if(!WaveSpriteMask(mask,d))return false;return number(mask);}
    if(name=="animation"){std::uint32_t selected=0;
      if(!NamedAnimation(a.root.address,"Idle",selected,d))return false;
      CaptureLink link;if(!AnimationResource(a.root.address,selected,link,d))return false;
      return writer.PutLinks(a.location,true,a.format.resource_table,std::span(&link,1));}
  }
  if(id==248&&field==1){std::array<std::uint32_t,3> pointers{};std::array<std::uint8_t,3> children{};
    if(!Value(address,pointers,d))return false;
    const auto base=scene.room+5656u+192u;
    for(unsigned i=0;i<3;++i){children[i]=255;if(!pointers[i])continue;if(pointers[i]<base || (pointers[i]-base)%52 || (pointers[i]-base)/52>=10)return Reject(a,d,CaptureFailure::InvalidValue);children[i]=std::uint8_t((pointers[i]-base)/52);}
    return scalar(std::as_bytes(std::span(children)));}
  if(id==298 || id==297){std::uint32_t display=0;if(!Value(std::uint64_t(a.root.address)+0x3c,display,d))return false;
    if(id==298)return number(display?1u:0u);
    if(!map_ready){if(!SavedMapIcons(a.root.address,map_mask,map_frames,d))return false;map_ready=true;}
    constexpr unsigned bits[]={0,1,3,4,7,8,9,11,13};return field==0?number(map_mask):number(map_frames[bits[field-1]]);}
  // 未登记的结果保持数字诊断，不能以mask0／Null代替来源。
  return Reject(a,d,a.format.linked?CaptureFailure::ResourcePending:CaptureFailure::MissingQualification);
}

// 只遍历已经加载的AnimationData目录；不调用会改变当前姿态的SetAnimation／SetFrame。
bool EntitySource::Impl::NamedAnimation(std::uint32_t owner,std::string_view name,std::uint32_t &out,CaptureDiagnostic &d) noexcept {
  std::uint8_t loaded=0;std::array<std::uint32_t,2> directory{};out=0;
  if(!Tracked(std::uint64_t(owner)+0x109,loaded,d) || loaded!=1 || !Tracked(std::uint64_t(owner)+0x74,directory,d) ||
     !directory[0] || !directory[1] || directory[1]>65535)return FailCapture(d,CaptureFailure::ResourcePending,{facts.subject});
  std::array<std::byte,4096> text{};
  for(std::uint32_t i=0;i<directory[1];++i){const auto address=std::uint64_t(directory[0])+std::uint64_t(i)*316;std::uint32_t size=0;
    if(address>UINT32_MAX || !StringAt(std::uint32_t(address),text,size,d,true))return false;
    if(size==name.size() && !std::memcmp(text.data(),name.data(),size)){if(out)return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});out=std::uint32_t(address);}}
  return out || FailCapture(d,CaptureFailure::ResourcePending,{facts.subject});
}
// 两个波纹各有一份主帧；共享ANM2的最后一次Render scratch不代表这两份结果。
bool EntitySource::Impl::WaveSpriteMask(std::uint32_t &mask,CaptureDiagnostic &d) noexcept {
  if(!facts.scene)return FailCapture(d,CaptureFailure::MissingQualification,{facts.subject});const auto &scene=*facts.scene;
  std::uint32_t backdrop=0,flags=0;mask=0;
  if(!Tracked(std::uint64_t(scene.room)+7448,backdrop,d) || !Tracked(std::uint64_t(scene.current_descriptor)+68,flags,d))return false;
  if((backdrop!=18 && backdrop!=28) || (flags&1))return true;
  for(unsigned i=0;i<2;++i){std::array<std::int32_t,2> timing{};if(!Tracked(std::uint64_t(scene.room)+28888+i*28u,timing,d))return false;
    if(timing[0]<timing[1]){if(timing[0]<0 || timing[1]<=0)return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});mask|=1u<<i;}}
  return true;
}

bool EntitySource::Impl::SceneOpen(const CaptureFieldAccess &a,CaptureTraversalSource &out,CaptureDiagnostic &d) noexcept {
  if(!facts.scene)return Reject(a,d,CaptureFailure::MissingQualification);const auto &scene=*facts.scene;
  const auto start=witness_used;const auto id=a.field.layout,field=a.field.index;out={};out.identity=a.root.address;
  const auto source=[&](std::uint64_t address,std::uint32_t count,std::uint32_t stride){if(address>UINT32_MAX || (count&&!address) || std::uint64_t(count)*stride>(std::uint64_t{1}<<32)-address)return Reject(a,d,CaptureFailure::AddressRange);
    out.count=count;out.opaque[0]=address;out.opaque[1]=stride;out.opaque[2]=start;out.opaque[3]=witness_used-start;return true;};
  if(Recipe(a.field)==EntityRead::Inline)return source(std::uint64_t(a.root.address)+a.field.native_offset,1,FindLayout(a.field.nested_layout)->native_size);
  if(id==270&&field==host_field::RoomDescriptor::map_icons)return source(a.root.address,1,0);
  if((id==239&&field==host_field::Room::shockwave_overlay_sprite)||(id==240&&field>=host_field::Backdrop::floor_sprite))
    return source(std::uint64_t(a.root.address)+a.field.native_offset,1,FindLayout(a.field.nested_layout)->native_size);
  if((id==239&&field>=17&&field<=20) || (id==240&&field==3) || (id==242&&field==1) ||
     (id==270&&field==11) || (id==276&&field==8) || id==298)return source(a.root.address,1,0);
  if((id==239&&field==24) || (id==240&&field>=5) || (id==253&&field==1))return source(std::uint64_t(a.root.address)+a.field.native_offset,1,FindLayout(a.field.nested_layout)->native_size);
  if(id==249&&field==0)return source(std::uint64_t(a.root.address)+4,scene.extent,1);
  return Reject(a,d,CaptureFailure::MissingQualification);
}
}
