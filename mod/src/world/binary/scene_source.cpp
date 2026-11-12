#include "world/binary/entity_source_internal.h"
#include <cmath>

namespace cs::world::binary {
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
  if(!s.NativeTree(scene.game+0x1bbd8,nodes,count,d))return false;
  scene.seed_effects={};
  std::int32_t previous=-1;
  for(std::uint32_t i=0;i<count;++i){std::int32_t effect=0;if(!s.Tracked(std::uint64_t(nodes[i])+16,effect,d))return false;
    if(effect<0 || effect>=int(scene.seed_effects.size()) || effect<=previous)return FailCapture(d,CaptureFailure::InvalidValue,{f.subject});
    scene.seed_effects[std::size_t(effect)]=true;previous=effect;}
  std::uint32_t raw=0,temporary=0,stage=0,difficulty=0,challenge=0;
  if(!s.Value(std::uint64_t(scene.game)+12,raw,d) || !s.Value(std::uint64_t(scene.game)+0x25ac8,temporary,d) ||
     !s.Value(scene.game,stage,d) || !s.Value(std::uint64_t(scene.game)+0x25f40,difficulty,d) ||
     !s.Value(std::uint64_t(scene.game)+0x25afc,challenge,d))return false;
  constexpr std::uint32_t masks[]={1,2,4,8,0x20,0x40,0x10};std::uint32_t forced=0,prevented=0;
  for(unsigned i=0;i<7;++i)if(scene.seed_effects[56+i] && (i!=1 || (challenge!=44 && std::bit_cast<std::int32_t>(stage)<8 && (stage&1) && difficulty!=2 && difficulty!=3)))forced|=masks[i];
  for(unsigned i=0;i<6;++i)if(scene.seed_effects[63+i])prevented|=masks[i];
  if(scene.seed_effects[70])prevented=0x7f;if(scene.seed_effects[79])prevented|=0x40;
  scene.effective_curses=(raw|temporary|forced)&~prevented;scene.greed=difficulty==2 || difficulty==3;
  std::array<std::byte,32> after{};
  for(std::uint32_t i=0;i<s.witness_used;++i){const auto &mark=s.witnesses[i];if(!s.Read(mark.address,std::span(after).first(mark.size),d) ||
      !std::equal(after.begin(),after.begin()+mark.size,mark.bytes.begin()))return FailCapture(d,CaptureFailure::SourceChanged,{f.subject});}
  return true;
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
  if(a.field.operation==Operation::Container)return ScalarContainer(a,writer,d);
  if(Recipe(a.field)==EntityRead::RoomConfig){CaptureLink link;if(!Resource(a,link,d))return false;return writer.PutLinks(a.location,true,a.format.resource_table,std::span(&link,1));}
  if(id==239){
    if(field==0){for(std::uint32_t i=0;i<scene.descriptor_count;++i){const auto slot=scene.descriptor_order[i];if(scene.descriptors[slot]==scene.current_descriptor)return number(slot);}return Reject(a,d,CaptureFailure::ReferencePending);}
    if(field==1)return number(scene.dimension);
    if(field==13){std::array<CaptureLink,8> links{};
      for(unsigned i=0;i<8;++i)links[i]=scene.grids[448+i]?CaptureLink{CapturedReferenceKind::Identity,scene.grid_handles[448+i]}:CaptureLink{CapturedReferenceKind::Null,0};
      for(const auto link:links)if(link.kind!=CapturedReferenceKind::Null&&!link.handle)return Reject(a,d,CaptureFailure::ReferencePending);
      return writer.PutLinks(a.location,false,255,links);}
    if(field==23){std::uint32_t mask=0;if(!WaveSpriteMask(mask,d))return false;return number(mask?1u:0u);}
  }
  if(id==242 || id==253){if((id==242&&field==0)||(id==253&&field==0)){std::uint8_t enabled=0;if(!Value(address,enabled,d) || enabled>1)return Reject(a,d,CaptureFailure::InvalidValue);return number(enabled);}
    if(id==253&&field==2)return native(address);}
  if(id==240 && field==4){std::uint32_t backdrop=0;if(!Tracked(std::uint64_t(a.root.address)+336,backdrop,d) || backdrop>=63)return Reject(a,d,CaptureFailure::InvalidValue);
    // 墙细节已经烘入recipe；教程控件由本端C/S绑定显示，不下发Host设备按钮层。
    return number(backdrop==18 || backdrop==26 || backdrop==35 || (backdrop>=52 && backdrop<=54)?1u:0u);}
  if(id==283){std::array<std::uint32_t,2> binding{};if(!Value(a.root.address,binding,d))return false;
    if(!field)return number(binding[0]);CaptureLink link;if(!ImageResource(binding[1],link,d) || link.kind==CapturedReferenceKind::Null)return Reject(a,d,CaptureFailure::ResourcePending);
    return writer.PutLinks(a.location,true,a.format.resource_table,std::span(&link,1));}
  if(id==270){
    if(field==2 || field==5){std::uint32_t v=0;if(!Value(address,v,d) || v>(field==2?2u:255u))return Reject(a,d,CaptureFailure::InvalidValue);return number(v);}
    if(field==8){std::int32_t count=0;if(!Value(address,count,d))return false;return number(count?1u:0u);}
    if(field==9){std::uint32_t flags=0;if(!Value(address,flags,d))return false;return number((flags&1)|((flags>>9)&2)|((flags>>9)&4)|((flags>>15)&8)|((flags>>1)&16));}
    if(field==10){if(!scene.greed)return number(0);std::int32_t special=0,index=0;std::uint32_t config=0,type=0;
      if(!Value(std::uint64_t(scene.game)+0x18370,special,d) || !Value(a.root.address,index,d) || !Value(std::uint64_t(a.root.address)+16,config,d))return false;
      if(config&&!Value(std::uint64_t(config)+8,type,d))return false;return number(config&&type==4&&index==special?1u:0u);}
  }
  if(id==276){if(field==2)return number(scene.effective_curses);if(field==4)return number(scene.dimension);
    if(field==5){if(!scene.greed)return number(0);return native(address);}
    if(field==6){std::int32_t list=0,boss=0;if(!Value(std::uint64_t(scene.current_descriptor)+8,list,d) || !Value(std::uint64_t(scene.game)+0x18314,boss,d))return false;return number(list==boss?1u:0u);}
    if(field==7)return scalar(std::as_bytes(std::span(scene.occupancy)));}
  if(id==299){std::uint32_t seed=0;if(!Value(std::uint64_t(scene.current_descriptor)+0x5c,seed,d))return false;
    if(!seed)return number(0);seed^=seed>>2;seed^=seed<<7;seed^=seed>>25;return number(seed%189+1==166?1u:0u);}
  if(id==245){if(field==1)return image(address);if(field==3){std::uint8_t v=0;if(!Value(address,v,d)||v>1)return Reject(a,d,CaptureFailure::InvalidValue);return number(v);}
    if(field==4){float v=0;if(!Value(address,v,d)||!std::isfinite(v))return Reject(a,d,CaptureFailure::InvalidValue);return number(v!=0?1u:0u);}
    if(field==5){std::uint32_t v=0;if(!Value(address,v,d))return false;return number(v==35?1u:0u);}
    if(field==6)return native(address);}
  if(id==246){if(field==2)return image(address);if(field==1)return native(address);}
  if(id==296){if(field==0)return native(a.root.address);if(field==1)return native(std::uint64_t(a.root.address)+176);}
  if(id==287){float amount=0;if(!Value(std::uint64_t(scene.room)+29248,amount,d) || !std::isfinite(amount))return Reject(a,d,CaptureFailure::InvalidValue);return number(amount>0?1u:0u);}
  if(id==285){std::array<std::uint32_t,8> result{};if(!WaterShader(result,d))return false;
    if(field==0)return scalar(std::as_bytes(std::span(result)).first(16));
    if(field==1)return number(result[4]);if(field==2)return scalar(std::as_bytes(std::span(result)).subspan(20,12));}
  if(id==288){if(!Lighting(d))return false;
    if(field==0)return number(lighting.mask);if(field==3)return scalar(std::as_bytes(std::span(&lighting.alpha,1)));
    if(field==4)return number(lighting.special);if(field==5)return scalar(std::as_bytes(std::span(&lighting.lightning,1)));}
  if(id==286){if(field==0)return image(std::uint64_t(scene.room)+7112+44);
    if(field==1){std::uint32_t backdrop=0;std::uint8_t reverse=0;
      if(!Tracked(std::uint64_t(scene.room)+7448,backdrop,d) || backdrop>=63 || !Tracked(std::uint64_t(scene.room)+7112+340+backdrop*316u+313,reverse,d) || reverse>1)
        return Reject(a,d,CaptureFailure::InvalidValue);
      float phase=float(std::bit_cast<std::int32_t>(a.window.witness.game_frame))*0.5f;if(reverse)phase=-phase;return scalar(std::as_bytes(std::span(&phase,1)));}
    if(field==2)return native(std::uint64_t(scene.room)+29792);}
  if(id==289){std::array<float,4> values{};bool enabled=false;
    if(!field){unsigned mask=0;for(unsigned i=0;i<2;++i){if(!Wave(i,values,enabled,d))return false;if(enabled)mask|=1u<<i;}return number(mask);}
    const auto wave=(field-1)/6,part=(field-1)%6;if(wave>=2 || !Wave(wave,values,enabled,d) || !enabled)return Reject(a,d,CaptureFailure::InvalidValue);
    if(part<3)return scalar(part==0?std::as_bytes(std::span(values)).first(8):std::as_bytes(std::span(values)).subspan(part==1?8:12,4));
    const auto base=std::uint64_t(scene.room)+28868+wave*28u;
    return native(base+(part==3?16u:part==4?20u:24u));}
  if(id==434){const auto name=std::string_view(a.field.name);
    if(name=="active_mask"){std::uint32_t mask=0;if(!WaveSpriteMask(mask,d))return false;return number(mask);}
    if(name=="animation"){std::uint32_t selected=0;
      if(!NamedAnimation(a.root.address,"Idle",selected,d))return false;
      CaptureLink link;if(!AnimationResource(a.root.address,selected,link,d))return false;
      return writer.PutLinks(a.location,true,a.format.resource_table,std::span(&link,1));}
  }
  if(id==291){std::int32_t selected=0;std::array<std::uint32_t,3> groups{};
    if(!Value(std::uint64_t(a.root.address)+48,selected,d) || !Value(std::uint64_t(a.root.address)+36,groups,d))return false;
    if(selected< -1 || groups[0]>groups[1] || groups[1]>groups[2] || (groups[1]-groups[0])%44)return Reject(a,d,CaptureFailure::InvalidValue);
    if(selected==-1)return number(0);if(std::uint32_t(selected)>=(groups[1]-groups[0])/44)return Reject(a,d,CaptureFailure::CountMismatch);
    std::array<std::uint32_t,3> rays{};if(!Value(std::uint64_t(groups[0])+std::uint64_t(selected)*44+28,rays,d) || rays[0]>rays[1] || rays[1]>rays[2] || (rays[1]-rays[0])%20)return Reject(a,d,CaptureFailure::InvalidValue);
    return number(rays[0]!=rays[1]?1u:0u);}
  if(id==293){std::int32_t countdown=0;if(!Value(std::uint64_t(a.root.address)+52,countdown,d))return false;return number(countdown>0?1u:0u);}
  if(id==292){if(field==0){std::int32_t countdown=0;if(!Value(std::uint64_t(a.root.address)+52,countdown,d) || countdown<=0 || countdown>180)return Reject(a,d,CaptureFailure::InvalidValue);
      const float alpha=countdown>150?float(float(countdown)-180.0f)/-30.0f:float(countdown)/150.0f;return scalar(std::as_bytes(std::span(&alpha,1)));}
    if(field==1)return image(std::uint64_t(a.root.address)+60);if(field==2)return native(std::uint64_t(a.root.address)+132);}
  if(id==295){std::uint32_t first=0;if(!Value(std::uint64_t(a.root.address)+196,first,d))return false;return number(first?1u:0u);}
  if(id==294&&field==0){std::array<std::uint8_t,10> roots{};for(unsigned i=0;i<10;++i)roots[i]=std::uint8_t(i);return scalar(std::as_bytes(std::span(roots)));}
  if(id==248&&field==1){std::array<std::uint32_t,3> pointers{};std::array<std::uint8_t,3> children{};
    if(!Value(address,pointers,d))return false;
    const auto base=scene.room+5656u+192u;
    for(unsigned i=0;i<3;++i){children[i]=255;if(!pointers[i])continue;if(pointers[i]<base || (pointers[i]-base)%52 || (pointers[i]-base)/52>=10)return Reject(a,d,CaptureFailure::InvalidValue);children[i]=std::uint8_t((pointers[i]-base)/52);}
    return scalar(std::as_bytes(std::span(children)));}
  if(id==290){std::uint32_t camera=0;if(!Tracked(std::uint64_t(scene.room)+4600,camera,d) || !camera)return Reject(a,d,CaptureFailure::MissingQualification);
    std::uint8_t override=0;std::array<std::uint32_t,4> ring{};
    if(!Tracked(std::uint64_t(camera)+140,override,d) || override>1)return Reject(a,d,CaptureFailure::InvalidValue);
    if(!override)return field==0?number(0):Reject(a,d,CaptureFailure::InvalidTransaction);
    if(!Tracked(std::uint64_t(camera)+80,ring,d) || !ring[0] || !ring[1] || ring[2]>ring[1] || ring[3]>=ring[1])return Reject(a,d,CaptureFailure::InvalidValue);
    if(field==0)return number(ring[2]?1u:0u);if(!ring[2])return Reject(a,d,CaptureFailure::InvalidValue);
    return native(std::uint64_t(ring[0])+((std::uint64_t(ring[3])+ring[2]-1)%ring[1])*8);}
  if(id==298 || id==297){std::uint32_t display=0;if(!Value(std::uint64_t(a.root.address)+0x3c,display,d))return false;
    if(id==298)return number(display?1u:0u);
    if(!map_ready){if(!SavedMapIcons(a.root.address,map_mask,map_frames,d))return false;map_ready=true;}
    constexpr unsigned bits[]={0,1,3,4,7,8,9,11,13};return field==0?number(map_mask):number(map_frames[bits[field-1]]);}
  // 未登记的结果保持数字诊断，不能以mask0／Null代替来源。
  return Reject(a,d,a.format.linked?CaptureFailure::ResourcePending:CaptureFailure::MissingQualification);
}

// 816BB0的纯f32水色投影：默认／seed79／端点插值后恰好一次乘水量，保留实际KColor标签。
bool EntitySource::Impl::WaterShader(std::array<std::uint32_t,8> &out,CaptureDiagnostic &d) noexcept {
  if(!facts.scene)return FailCapture(d,CaptureFailure::MissingQualification,{facts.subject});const auto room=facts.scene->room;
  std::array<float,4> color{};std::array<float,3> multiplier{};std::uint32_t space=0;float amount=0;
  if(!Tracked(std::uint64_t(room)+6960,color,d) || !Tracked(std::uint64_t(room)+6976,space,d) ||
     !Tracked(std::uint64_t(room)+6980,multiplier,d) || !Tracked(std::uint64_t(room)+29248,amount,d))return false;
  if(facts.scene->seed_effects[79]){color={0.5f,0.7f,1.0f,0.5f};multiplier={0.5f,0.7f,1.0f};space=0;}
  std::array<std::int32_t,2> timing{};if(!Tracked(std::uint64_t(room)+29332,timing,d))return false;
  if(timing[0]>=0){std::array<float,4> start{},target{};std::array<float,3> from{},to{};
    if(!Tracked(std::uint64_t(room)+29252,start,d) || !Tracked(std::uint64_t(room)+29292,target,d) ||
       !Tracked(std::uint64_t(room)+29272,from,d) || !Tracked(std::uint64_t(room)+29312,to,d))return false;
    const float alpha=timing[0]?std::clamp(1.0f-float(timing[1])/float(timing[0]),0.0f,1.0f):1.0f;
    for(unsigned i=0;i<4;++i)color[i]=float(float(target[i]-start[i])*alpha)+start[i];
    for(unsigned i=0;i<3;++i)multiplier[i]=float(float(to[i]-from[i])*alpha)+from[i];}
  color[3]=float(color[3]*amount);for(auto &v:multiplier)v=float(float(v-1.0f)*amount)+1.0f;
  if(!std::isfinite(amount))return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
  for(unsigned i=0;i<4;++i){if(!std::isfinite(color[i]))return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});out[i]=std::bit_cast<std::uint32_t>(color[i]);}
  out[4]=space;for(unsigned i=0;i<3;++i){if(!std::isfinite(multiplier[i]))return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});out[5+i]=std::bit_cast<std::uint32_t>(multiplier[i]);}
  return true;
}
// 仅显示的波纹锚点：世界中心／半帧半径／强度；计时与速度交本地纯显示推进，不运行Room.Update。
bool EntitySource::Impl::Wave(unsigned index,std::array<float,4> &out,bool &enabled,CaptureDiagnostic &d) noexcept {
  if(!facts.scene || index>=2)return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});const auto base=std::uint64_t(facts.scene->room)+28868+index*28u;
  std::array<std::int32_t,2> timing{};if(!Tracked(base+20,timing,d))return false;enabled=timing[0]<timing[1];if(!enabled)return true;
  if(timing[0]<0 || timing[1]<=0)return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
  std::array<float,2> center{};float radius=0,strength=0,speed=0;std::uint8_t half=0;
  if(!Tracked(base,center,d) || !Tracked(base+8,radius,d) || !Tracked(base+12,strength,d) ||
     !Tracked(std::uint64_t(facts.scene->room)+29216,half,d) || half>1 || (half&&!Tracked(base+16,speed,d)))return false;
  out={center[0],center[1],half?float(radius+float(speed*0.5f)):radius,strength};
  for(const auto v:out)if(!std::isfinite(v))return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});return true;
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
  if(id==284){std::array<std::array<std::uint32_t,2>,7> entries{};std::uint32_t count=0;if(!BackdropBindings(entries,count,d))return false;
    auto bytes=Allocate(count*sizeof(entries[0]),alignof(std::uint32_t),d);if(d.reason!=CaptureFailure::None)return false;
    std::memcpy(bytes.data(),entries.data(),bytes.size());return source(NativeAddress(bytes),count,sizeof(entries[0]));}
  if(id==288 && (field==1 || field==2)){if(!Lighting(d))return false;const auto &color=field==1?lighting.shadow:lighting.tint;
    return source(NativeAddress(std::as_writable_bytes(std::span(field==1?lighting.shadow:lighting.tint))),1,std::uint32_t(sizeof(color)));}
  if((id==239&&field>=17&&field<=20) || (id==240&&field==3) || (id==242&&field==1) ||
     id==287 || id==286 || (id==270&&field==11) || (id==276&&field==8) || id==298 ||
     (id==296&&field>=3) || id==293 || id==295)return source(a.root.address,1,0);
  if((id==239&&field==24) || (id==240&&field>=5) || (id==253&&field==1))return source(std::uint64_t(a.root.address)+a.field.native_offset,1,FindLayout(a.field.nested_layout)->native_size);
  if(id==249&&field==0)return source(std::uint64_t(a.root.address)+4,scene.extent,1);
  if(id==294){if(field>=2)return source(std::uint64_t(a.root.address)+712+(field-2)*276u,1,276);
    // 十个内嵌节点组成固定有界图，先核所有child归属和环，再允许递归元素读取。
    std::array<std::array<std::uint8_t,3>,10> edges{};std::array<std::uint8_t,10> color{};
    const auto base=std::uint64_t(a.root.address)+192;
    for(unsigned n=0;n<10;++n){std::array<std::uint32_t,3> children{};if(!Tracked(base+n*52+4,children,d))return false;
      for(unsigned c=0;c<3;++c){auto &edge=edges[n][c];edge=255;if(!children[c])continue;
        if(children[c]<base || (children[c]-base)%52 || (children[c]-base)/52>=10)return Reject(a,d,CaptureFailure::InvalidValue);edge=std::uint8_t((children[c]-base)/52);}}
    const auto walk=[&](auto &self,unsigned n)->bool {if(color[n]==1)return false;if(color[n]==2)return true;color[n]=1;
      for(const auto child:edges[n])if(child!=255&&!self(self,child))return false;color[n]=2;return true;};
    for(unsigned n=0;n<10;++n)if(!walk(walk,n))return Reject(a,d,CaptureFailure::InvalidValue);
    return source(base,10,52);}
  if(id==291){std::int32_t selected=0;std::array<std::uint32_t,3> groups{};
    if(!Tracked(std::uint64_t(a.root.address)+48,selected,d) || !Tracked(std::uint64_t(a.root.address)+36,groups,d) || selected<0 ||
       groups[0]>groups[1] || groups[1]>groups[2] || (groups[1]-groups[0])%44 || std::uint32_t(selected)>=(groups[1]-groups[0])/44)return Reject(a,d,CaptureFailure::CountMismatch);
    return source(std::uint64_t(groups[0])+std::uint64_t(selected)*44,1,44);}
  if((id==296&&field==2) || (id==245&&field==2) || (id==247&&field==1) || (id==292&&field==3)){
    const auto offset=id==296?24u:id==292?68u:a.field.native_offset;
    const auto stride=id==296?76u:20u;std::array<std::uint32_t,3> header{};
    if(!Tracked(std::uint64_t(a.root.address)+offset,header,d) || header[0]>header[1] || header[1]>header[2] || (!header[0]&&(header[1]||header[2])) ||
       (header[1]-header[0])%stride || (header[2]-header[0])%stride)return Reject(a,d,CaptureFailure::InvalidValue);
    auto count=(header[1]-header[0])/stride;
    if(id==245 || id==292){std::int32_t used=0;if(!Tracked(std::uint64_t(a.root.address)+(id==292?80u:24u),used,d) || used<0 || std::uint32_t(used)>count)return Reject(a,d,CaptureFailure::CountMismatch);count=std::uint32_t(used);}
    return source(header[0],count,stride);}
  return Reject(a,d,CaptureFailure::MissingQualification);
}
}
