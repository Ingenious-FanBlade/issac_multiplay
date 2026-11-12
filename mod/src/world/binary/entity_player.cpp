#include "world/binary/entity_source_internal.h"
#include <cmath>

namespace cs::world::binary {
// J273工厂／vtable与分配大小，来源见5-player-p3-p5；10/11共用Bone类但type仍分别核验。
const NativeWeaponContract *WeaponContract(std::uint32_t type) noexcept {
  static constexpr NativeWeaponContract table[]={
    {},{0x731ea4,0x50,181},{0x731db4,0x4c,182},{0x731aac,0x50,183},{0x731d64,0x54,184},
    {0x731a64,0x48,185},{0x731e5c,0x48,186},{0x731dfc,0x44,187},{0x731a1c,0x48,188},
    {0x731cd4,0x44,189},{0x731d1c,0x60,190},{0x731d1c,0x60,190},{0x731b6c,0x54,191},
    {0x731eec,0x50,192},{0x731c1c,0x44,193},{0x731bc4,0x48,194}};
  return type && type<std::size(table)?&table[type]:nullptr;
}
bool EntitySource::Impl::Weapon(std::uint32_t address,NativeWeaponSlot &out,std::uint32_t &type,CaptureDiagnostic &d,bool tracked) noexcept {
  std::uint32_t vtable=0;
  if(!(tracked?Tracked(address,vtable,d):Value(address,vtable,d)) || !(tracked?Tracked(std::uint64_t(address)+0x30,type,d):Value(std::uint64_t(address)+0x30,type,d)))return false;
  const auto *contract=WeaponContract(type);
  if(!contract || vtable!=memory.image_base+contract->vtable)return FailCapture(d,CaptureFailure::UnknownVtable,{facts.subject});
  out={address,contract->extent,contract->layout};return true;
}
// 复现已核slot6/7纯公式，逐步落f32；绝不调用虚getter、分配玩法Weapon或修改DB0。
bool EntitySource::Impl::Incubus(std::uint32_t familiar,std::int32_t &amount,CaptureDiagnostic &d) noexcept {
  std::uint32_t pointer=0,vtable=0,type=0;amount=0;
  if(!Value(std::uint64_t(familiar)+0xdb0,pointer,d))return false;if(!pointer)return true;
  if(!Value(pointer,vtable,d) || !Value(std::uint64_t(pointer)+0x30,type,d))return false;
  const auto *contract=WeaponContract(type);const bool base=vtable==memory.image_base+0x731c64,deathmatch=vtable==memory.image_base+0x731b14;
  if(!base && !deathmatch && (!contract || vtable!=memory.image_base+contract->vtable))return FailCapture(d,CaptureFailure::UnknownVtable,{facts.subject});
  float delay=0,charge=0;std::uint32_t modifiers=0;
  if(!base && !deathmatch && (type==12 || type==15)){
    std::uint32_t after=0,after_vtable=0;
    return Value(std::uint64_t(familiar)+0xdb0,after,d) && after==pointer && Value(pointer,after_vtable,d) && after_vtable==vtable ||
      FailCapture(d,CaptureFailure::SourceChanged,{facts.subject});}
  if(!Value(std::uint64_t(pointer)+0x10,delay,d) || !Value(std::uint64_t(pointer)+0x14,charge,d) ||
     !Value(std::uint64_t(pointer)+0x34,modifiers,d) || !std::isfinite(delay) || !std::isfinite(charge))return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
  const auto a=std::max(delay,0.001f);float maximum=0;
  if(deathmatch)maximum=a;
  else if(!base)switch(type){
    case 1:{float scale=(modifiers&0x100)?12.0f:1.0f;if(!(modifiers&0x100)){if(modifiers&1)scale=float(scale*2.5f);if(modifiers&2)scale=float(scale*2.0f);}
      if(scale>1)maximum=float(float(float(delay+1.0f)*scale)-1.0f);break;}
    case 2:if(!(modifiers&0x10))maximum=float(a*((modifiers&1)?2.5f:1.0f));break;
    case 3:{float scale=(modifiers&8)?4.3000002f:1.0f;if(modifiers&1)scale=float(scale*2.5f);if(modifiers&2)scale=float(scale*2.0f);
      if(scale>1)maximum=float(float(float(delay+1.0f)*scale)-1.0f);break;}
    case 4:if(!(modifiers&0x10))maximum=float(a*4.0f);break;
    case 7:maximum=float(a*((modifiers&1)?2.5f:1.0f));break;
    case 9:maximum=float(delay*3.0f);break;
    case 10:case 11:maximum=float(a*((!(modifiers&0x80e)||(modifiers&1))?3.0f:2.0f));break;
    case 13:maximum=float(delay*4.0f);break;
    case 14:maximum=std::max(float(delay*3.0f),0.001f);break;
    default:break;
  }
  if(!std::isfinite(maximum))return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
  if(!(modifiers&0x80) && maximum>0 && !(charge<=1 && charge<maximum)){
    const float ratio=std::clamp(float(charge/maximum),0.0f,1.0f),percent=float(ratio*100.0f);
    if(!std::isfinite(percent))return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});amount=std::int32_t(percent);
  }
  std::uint32_t after=0,after_vtable=0;
  return Value(std::uint64_t(familiar)+0xdb0,after,d) && after==pointer && Value(pointer,after_vtable,d) && after_vtable==vtable ||
    FailCapture(d,CaptureFailure::SourceChanged,{facts.subject});
}

// 每个叶子只写当前数字路径的一项，所有有符号窄化、掩码及可空关系都显式处理。
bool EntitySource::Impl::Leaf(const CaptureFieldAccess &a,CaptureValueWriter &writer,CaptureDiagnostic &d) noexcept {
  const auto recipe=Recipe(a.field);const auto address=std::uint64_t(a.root.address)+a.field.native_offset;
  if(recipe>=EntityRead::GridWater && recipe<=EntityRead::DoorLight)return GridLeaf(a,writer,d);
  if(recipe==EntityRead::Run)return RunLeaf(a,writer,d);
  if(recipe==EntityRead::Presentation)return PresentationLeaf(a,writer,d);
  if(recipe==EntityRead::Scene || recipe==EntityRead::RoomConfig)return SceneLeaf(a,writer,d);
  if(a.field.operation==Operation::Container)return ScalarContainer(a,writer,d);
  if(a.format.linked){CaptureLink link{CapturedReferenceKind::Null,0};
    if(recipe==EntityRead::Reference){std::uint32_t pointer=0;if(!Value(address,pointer,d))return false;
      if(pointer)link=services.reference(services.owner,pointer);
      if(link.kind==CapturedReferenceKind::Invalid)return Reject(a,d,CaptureFailure::ReferencePending);
    }else if(recipe==EntityRead::Group){if(!facts.group_known)return Reject(a,d,CaptureFailure::MissingQualification);
      if(facts.group)link={CapturedReferenceKind::Identity,facts.group};
    }else if(!Resource(a,link,d))return false;
    return writer.PutLinks(a.location,a.format.resource_table!=255,a.format.resource_table,std::span(&link,1));
  }
  std::array<std::byte,32> bytes{};const auto size=std::uint64_t(CaptureScalarWidth(a.format.scalar))*a.format.components;
  if(!size || size>bytes.size())return Reject(a,d,CaptureFailure::InvalidValue);
  std::uint64_t result=0;
  switch(recipe){
    case EntityRead::Raw:
      if(size!=a.field.native_size || !Read(address,std::span(bytes).first(std::size_t(size)),d))return Reject(a,d,CaptureFailure::InvalidValue);
      return writer.PutScalar(a.location,a.format.scalar,a.format.components,std::span(bytes).first(std::size_t(size)));
    case EntityRead::Mask:{std::uint32_t raw=0;if(!Value(address,raw,d))return false;result=raw&a.field.value_mask;break;}
    case EntityRead::Bool:{std::uint8_t raw=0;if(!Value(address,raw,d))return false;result=raw?1u:0u;break;}
    case EntityRead::Order:{std::uint32_t raw=0;if(!Value(address,raw,d))return false;result=raw;break;}
    case EntityRead::Expire:{std::int32_t raw=0;if(!Value(address,raw,d))return false;result=std::bit_cast<std::uint32_t>(ExpireFrame(a.window.witness.game_frame,raw));break;}
    case EntityRead::Flying:{std::uint8_t engaged=0,raw=0;if(!Value(std::uint64_t(a.root.address)+5,engaged,d)||engaged>1)return Reject(a,d,CaptureFailure::InvalidValue);
      if(engaged && (!Value(std::uint64_t(a.root.address)+4,raw,d)||raw>1))return Reject(a,d,CaptureFailure::InvalidValue);result=engaged?raw:255u;break;}
    case EntityRead::Water:{std::uint8_t engaged=0;std::uint32_t raw=0;if(!Value(std::uint64_t(a.root.address)+12,engaged,d)||engaged>1)return Reject(a,d,CaptureFailure::InvalidValue);
      if(engaged && !Value(std::uint64_t(a.root.address)+8,raw,d))return false;result=engaged?(std::uint64_t{1}<<32)|raw:0;break;}
    case EntityRead::QueueFlags:{std::uint32_t raw=0;if(!Value(address,raw,d))return false;result=(raw&2)?1u:0u;break;}
    case EntityRead::CostumeIndex:{std::int32_t raw=0;if(!Value(address,raw,d) || raw< -1 || raw>=65535)return Reject(a,d,CaptureFailure::InvalidValue);
      result=raw<0?kNullIndex:std::uint32_t(raw);break;}
    case EntityRead::NpcControl:{std::int32_t raw=0;if(!Value(address,raw,d))return false;result=raw==-1?0u:2u;break;}
    case EntityRead::Incubus:{std::int32_t raw=0;if(!Incubus(a.root.address,raw,d))return false;result=std::uint32_t(raw);break;}
    case EntityRead::Topology:if(!topology_revision)return Reject(a,d,CaptureFailure::MissingValue);result=topology_revision;break;
    case EntityRead::Membership:result=(facts.activation_qualified?1u:0u)|(facts.memberships&1u?2u:0u)|(facts.memberships&2u?4u:0u)|(facts.memberships&4u?8u:0u);break;
    case EntityRead::Roster:result=facts.roster;break;
    case EntityRead::Role:if(!facts.group_known)return Reject(a,d,CaptureFailure::MissingQualification);result=facts.role;break;
    case EntityRead::Control:if(!facts.group_known || facts.control_flags>3 || ((facts.control_flags&2) && !(facts.control_flags&1)))return Reject(a,d,CaptureFailure::MissingQualification);result=facts.control_flags;break;
    case EntityRead::DrawPresent:{std::array<std::uint32_t,3> list{};if(!Value(a.root.address,list,d) || !list[0] || !list[2] || list[1]>16384)return Reject(a,d,CaptureFailure::InvalidValue);
      if(list[2]!=list[0]){if(!list[1])return Reject(a,d,CaptureFailure::InvalidValue);
        {auto current=list[0];bool found=false;std::uint32_t previous=list[0];
          if(!Value(current,current,d))return false;
          for(std::uint32_t i=0;i<list[1];++i){std::array<std::uint32_t,2> links{};if(!current || current==list[0] || !Value(current,links,d) || links[1]!=previous)return Reject(a,d,CaptureFailure::SourceChanged);
            found|=current==list[2];previous=current;current=links[0];}
          std::uint32_t tail=0;if(!found || current!=list[0] || !Value(std::uint64_t(list[0])+4,tail,d) || tail!=previous)return Reject(a,d,CaptureFailure::SourceChanged);}
        // 先证明cursor属于当前链，再读选中节点；D=0不碰Sprite及旧帧尾。
        std::int32_t animation=0;if(!Value(std::uint64_t(list[2])+24,animation,d))return false;result=animation>=0?1u:0u;}
      break;}
    case EntityRead::DrawSecond:{std::array<std::int32_t,2> pair{};if(!Value(address,pair[0],d))return false;
      if(pair[0]<0)pair[0]=-1;else if(!Value(address+4,pair[1],d))return false;
      return writer.PutScalar(a.location,a.format.scalar,2,std::as_bytes(std::span(pair)));}
    default:return Reject(a,d,CaptureFailure::MissingHandler);
  }
  if(size<8 && result>>(size*8))return Reject(a,d,CaptureFailure::InvalidValue);
  for(std::size_t i=0;i<size;++i)bytes[i]=std::byte(result>>(8*i));
  return writer.PutScalar(a.location,a.format.scalar,a.format.components,std::span(bytes).first(std::size_t(size)));
}

// 拓扑比较覆盖真实目录和资源内容而非vector地址；所有比较字节都是自有具名投影。
bool EntitySource::Impl::PrepareTopology(CaptureDiagnostic &d) noexcept {
  auto index=FindTopology(facts.subject);
  if(index==UINT32_MAX){for(std::uint32_t i=0;i<topology_used;++i)if(!topologies[i].subject.local){index=i;break;}
    if(index==UINT32_MAX){if(topology_used==topologies.size())return FailCapture(d,CaptureFailure::ValueBudget,{facts.subject});index=topology_used++;}
    topologies[index].subject=facts.subject;}
  auto output=Allocate(capacity.topology_bytes_per_body,8,d);if(d.reason!=CaptureFailure::None)return false;std::uint32_t used=0;
  const auto append=[&](std::span<const std::byte> value){if(value.size()>output.size()-used)return FailCapture(d,CaptureFailure::ValueBudget,{facts.subject});
    std::memcpy(output.data()+used,value.data(),value.size());used+=std::uint32_t(value.size());return true;};
  const auto number=[&](std::uint64_t value,unsigned width){std::array<std::byte,8> bytes{};for(unsigned i=0;i<width;++i)bytes[i]=std::byte(value>>(8*i));return append(std::span(bytes).first(width));};
  const auto link=[&](CaptureLink value){return number(unsigned(value.kind),1)&&number(value.handle,8);};
  const auto sprite=[&](std::uint32_t address){CaptureLink file;if(!FileResource(address,true,file,d) || !link(file))return false;
    std::uint32_t count=0,data=0;if(file.kind!=CapturedReferenceKind::Null &&
      (!Value(std::uint64_t(address)+0x7c,data,d) || !Value(std::uint64_t(address)+0x80,count,d) || count>65535 || (count&&!data)))return false;
    if(!number(count,4))return false;
    for(std::uint32_t layer=0;layer<count;++layer){std::array<std::uint32_t,2> pair{};CaptureLink image;
      if(!Value(std::uint64_t(data)+std::uint64_t(layer)*160+0x98,pair,d) || !ImageResource(pair[0],image,d) || !link(image))return false;}
    return true;};
  std::uint8_t can_fly=0;if(!Value(std::uint64_t(facts.address)+0x1564,can_fly,d) || can_fly>1 || !number(can_fly,1))return false;
  // 主Sprite和固定13项的资源结构同样参与P0解释，未loaded时不读无效层尾部。
  if(!sprite(facts.address+0x48))return false;
  for(unsigned i=0;i<13;++i)if(!sprite(facts.address+0x41c+i*276))return false;
  std::array<std::uint32_t,3> vector{},after{};if(!Value(std::uint64_t(facts.address)+0x1220,vector,d))return false;
  if(vector[0]>vector[1] || vector[1]>vector[2] || (!vector[0]&&(vector[1]||vector[2])) ||
     (vector[1]-vector[0])%304 || (vector[2]-vector[0])%304)return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
  const auto count=(vector[1]-vector[0])/304;if(count>16384 || !number(count,4))return FailCapture(d,CaptureFailure::ValueBudget,{facts.subject});
  for(std::uint32_t i=0;i<count;++i){const auto address=std::uint64_t(vector[0])+std::uint64_t(i)*304;std::uint32_t item=0;CaptureLink config;std::array<std::uint8_t,2> flags{};
    if(!Value(address+0x114,item,d) || !ItemResource(item,config,d) || !link(config) ||
       !Read(address+0x11d,std::as_writable_bytes(std::span(flags)),d) || flags[0]>1 || flags[1]>1 ||
       !append(std::as_bytes(std::span(flags))) || !sprite(std::uint32_t(address)))return false;}
  for(unsigned i=0;i<15;++i){const auto address=std::uint64_t(facts.address)+0x122c+i*16;std::int32_t selected=0,layer=0;std::uint8_t body=0;
    if(!Value(address,selected,d) || selected< -1 || selected>=65535 || (selected>=0 && !Value(address+4,layer,d)) ||
       !Value(address+12,body,d) || body>1 || !number(selected<0?kNullIndex:std::uint32_t(selected),2) ||
       !number(std::bit_cast<std::uint32_t>(layer),4) || !number(body,1))return false;}
  if(!Value(std::uint64_t(facts.address)+0x1220,after,d) || after!=vector)return FailCapture(d,CaptureFailure::SourceChanged,{facts.subject});
  auto &entry=topologies[index];auto previous=std::span(topology_bytes).subspan(std::size_t(index)*capacity.topology_bytes_per_body,capacity.topology_bytes_per_body);
  const bool same_identity=entry.subject.space==facts.subject.space && entry.subject.local==facts.subject.local && entry.subject.use==facts.subject.use &&
    entry.subject.shape==facts.subject.shape && entry.subject.binding==facts.subject.binding;
  if(!same_identity || entry.size!=used || !std::equal(output.begin(),output.begin()+used,previous.begin())){
    if(entry.revision==UINT32_MAX)return FailCapture(d,CaptureFailure::SequenceExhausted,{facts.subject});
    ++entry.revision;entry.ordinary=0;entry.subject=facts.subject;entry.size=used;std::copy_n(output.begin(),used,previous.begin());}
  topology_revision=entry.revision;entry.subject.topology=entry.revision;return true;
}
}
