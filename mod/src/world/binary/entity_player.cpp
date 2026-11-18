#include "world/binary/entity_source_internal.h"
#include "core/capture_cost.h"
#include <cmath>

namespace cs::world::binary {
#include "state_host_fields.inc"
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
  if(!Tracked(std::uint64_t(familiar)+0xdb0,pointer,d))return false;if(!pointer)return true;
  if(!Tracked(pointer,vtable,d) || !Tracked(std::uint64_t(pointer)+0x30,type,d))return false;
  const auto *contract=WeaponContract(type);const bool base=vtable==memory.image_base+0x731c64,deathmatch=vtable==memory.image_base+0x731b14;
  if(!base && !deathmatch && (!contract || vtable!=memory.image_base+contract->vtable))return FailCapture(d,CaptureFailure::UnknownVtable,{facts.subject});
  float delay=0,charge=0;std::uint32_t modifiers=0;
  if(!base && !deathmatch && (type==12 || type==15))return true;
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
  return true;
}

// 每个叶子只写当前数字路径的一项，所有有符号窄化、掩码及可空关系都显式处理。
// 原生字段到规范值的转换独立于正文编码，帧换算只使用本次窗口的 game_frame。
bool EntitySource::Impl::BasicConversion(EntityRead recipe,const CaptureRoot &root,std::uint64_t address,std::uint32_t mask,
    std::uint64_t &result,CaptureDiagnostic &d) noexcept {
  switch(recipe){
    case EntityRead::Mask:
    case EntityRead::Order:{
      std::uint32_t raw=0;
      if(!Value(address,raw,d))return false;
      result=recipe==EntityRead::Mask?raw&mask:raw;
      return true;
    }
    case EntityRead::Bool:{
      std::uint8_t raw=0;
      if(!Value(address,raw,d))return false;
      result=raw!=0;
      return true;
    }
    case EntityRead::Expire:{
      std::int32_t raw=0;
      if(!Value(address,raw,d))return false;
      result=std::bit_cast<std::uint32_t>(ExpireFrame(window.witness.game_frame,raw));
      return true;
    }
    case EntityRead::Flying:{
      std::uint8_t engaged=0,raw=0;
      if(!Value(address+1,engaged,d) || engaged>1 || (engaged && (!Value(address,raw,d) || raw>1)))
        return FailCapture(d,CaptureFailure::InvalidValue,root);
      result=engaged?raw:255u;
      return true;
    }
    case EntityRead::Water:{
      std::uint8_t engaged=0;
      std::uint32_t raw=0;
      if(!Value(address+4,engaged,d) || engaged>1)return FailCapture(d,CaptureFailure::InvalidValue,root);
      if(engaged && !Value(address,raw,d))return false;
      result=engaged?(std::uint64_t{1}<<32)|raw:0;
      return true;
    }
    default:return FailCapture(d,CaptureFailure::MissingHandler,root);
  }
}

bool EntitySource::Impl::Leaf(const CaptureFieldAccess &a,CaptureValueWriter &writer,CaptureDiagnostic &d) noexcept {
  evidence_layout=a.root.layout;evidence_field=a.field.index;
  const auto recipe=Recipe(a.field);CheckReuseRecipe(recipe);
  const auto address=std::uint64_t(a.root.address)+a.field.native_offset;
  if(recipe==EntityRead::Display)return DisplayLeaf(a,writer,d);
  if(recipe>=EntityRead::GridWater && recipe<=EntityRead::DoorLight)return GridLeaf(a,writer,d);
  if(recipe==EntityRead::Run)return RunLeaf(a,writer,d);
  if(recipe==EntityRead::Scene || recipe==EntityRead::RoomConfig)return SceneLeaf(a,writer,d);
  if(a.field.operation==Operation::Container)return ScalarContainer(a,writer,d);
  if(a.format.linked){CaptureLink link{CapturedReferenceKind::Null,0};
    if(recipe==EntityRead::Reference){std::uint32_t pointer=0;if(!Value(address,pointer,d))return false;
      if(pointer)link=ReadReference(pointer);
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
      if(const auto *map=PlayerMap(a);map&&a.field.index==host_field::PlayerCostumeMap::layer_id){result=std::bit_cast<std::uint32_t>(map->layer);break;}
      if(size!=a.field.native_size || !Read(address,std::span(bytes).first(std::size_t(size)),d))return Reject(a,d,CaptureFailure::InvalidValue);
      return writer.PutScalar(a.location,a.format.scalar,a.format.components,std::span(bytes).first(std::size_t(size)));
    case EntityRead::Mask:case EntityRead::Bool:case EntityRead::Order:case EntityRead::Expire:
    case EntityRead::Flying:case EntityRead::Water:
      if(!BasicConversion(recipe,a.root,address,a.field.value_mask,result,d))return Reject(a,d,CaptureFailure::InvalidValue);
      break;
    case EntityRead::QueueFlags:{std::uint32_t raw=0;if(!Value(address,raw,d))return false;result=(raw&2)?1u:0u;break;}
    case EntityRead::CostumeIndex:{if(const auto *map=PlayerMap(a)){result=map->index;break;}
      std::int32_t raw=0;if(!Value(address,raw,d) || raw< -1 || raw>=65535)return Reject(a,d,CaptureFailure::InvalidValue);
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
  player.read_serial=services.reads->Serial();
  if(!player.read_serial)return FailCapture(d,CaptureFailure::InvalidWindow,{facts.subject});
  auto index=FindTopology(facts.subject);
  if(index==UINT32_MAX){for(std::uint32_t i=0;i<topology_used;++i)if(!topologies[i].subject.local){index=i;break;}
    if(index==UINT32_MAX){if(topology_used==topologies.size())return FailCapture(d,CaptureFailure::ValueBudget,{facts.subject});index=topology_used++;}
    topologies[index].subject=facts.subject;}
  auto &entry=topologies[index];
  capture::Cost topology(capture::CostPart::TopologyResources);
  capture::Cost shared(capture::CostPart::JointPrepare,player.shared_streams);
  ++stats.topology_builds;
  auto output=Allocate(capacity.topology_bytes_per_body,8,d);if(d.reason!=CaptureFailure::None)return false;std::uint32_t used=0;
  const auto append=[&](std::span<const std::byte> value){if(value.size()>output.size()-used)return FailCapture(d,CaptureFailure::ValueBudget,{facts.subject});
    std::memcpy(output.data()+used,value.data(),value.size());used+=std::uint32_t(value.size());return true;};
  const auto number=[&](std::uint64_t value,unsigned width){std::array<std::byte,8> bytes{};for(unsigned i=0;i<width;++i)bytes[i]=std::byte(value>>(8*i));return append(std::span(bytes).first(width));};
  const auto link=[&](CaptureLink value){return number(unsigned(value.kind),1)&&number(value.handle,8);};
  using E=NativeEvidenceUse;
  const auto evidence=[&](std::uint16_t layout,std::uint16_t field){evidence_layout=layout;evidence_field=field;};
  const auto invalid=[&](CaptureFailure reason){return FailCapture(d,reason,{facts.subject,0,0,evidence_layout},evidence_field);};
  const auto sprite=[&](std::uint32_t address,PreparedPlayerSprite &projection){
    projection={};projection.address=address;
    evidence(host_field::ANM2::kLayout,host_field::ANM2::file_name);
    if(!FileResource(address,true,projection.file,d,true)||!link(projection.file))return false;
    evidence(host_field::ANM2::kLayout,host_field::ANM2::layer_states);
    if(projection.file.kind!=CapturedReferenceKind::Null){
      if(!Tracked(std::uint64_t(address)+0x80,projection.layer_count,d,{address,E::Layers}))return false;
      if(projection.layer_count){
        if(projection.layer_count>UINT16_MAX)return invalid(CaptureFailure::CountMismatch);
        if(!Tracked(std::uint64_t(address)+0x7c,projection.layer_data,d,{address,E::Layers}))return false;
        if(!projection.layer_data||
           std::uint64_t(projection.layer_data)+std::uint64_t(projection.layer_count)*160>(std::uint64_t{1}<<32))
          return invalid(CaptureFailure::AddressRange);
      }
    }
    if(!number(projection.layer_count,4))return false;
    const auto images=used;
    evidence(host_field::LayerState::kLayout,host_field::LayerState::image_selection);
    for(std::uint32_t layer=0;layer<projection.layer_count;++layer){
      const auto source=projection.layer_data+layer*160;std::array<std::uint32_t,2> pair{};CaptureLink image;
      if(!Tracked(std::uint64_t(source)+0x98,pair,d,{address,E::Image})||
         (pair[0]&&!pair[1])||!ImageResource(pair[0],image,d)||!link(image)){
        if(d.reason==CaptureFailure::None)FailCapture(d,CaptureFailure::SourceChanged,{facts.subject,source,160,evidence_layout},evidence_field);
        return false;
      }
    }
    projection.images=output.subspan(images,used-images);
    return true;};
  evidence(host_field::EntityPlayer::kLayout,host_field::EntityPlayer::_canFly);
  std::uint8_t can_fly=0;
  if(!Tracked(std::uint64_t(facts.address)+0x1564,can_fly,d,{facts.address,E::CanFly}) || can_fly>1 || !number(can_fly,1))
    return invalid(CaptureFailure::InvalidValue);
  // 主Sprite和固定13项的资源结构同样参与P0解释，未loaded时不读无效层尾部。
  if(!sprite(facts.address+0x48,player.sprites[0]))return false;
  for(unsigned i=0;i<13;++i)if(!sprite(facts.address+0x41c+i*276,player.sprites[i+1]))return false;
  evidence(host_field::EntityPlayer::kLayout,host_field::EntityPlayer::_costumeSpriteDescs_0);
  std::array<std::uint32_t,3> vector{};
  if(!Tracked(std::uint64_t(facts.address)+0x1220,vector,d,{facts.address,E::CostumeVector}))return false;
  if(vector[0]>vector[1] || vector[1]>vector[2] || (!vector[0]&&(vector[1]||vector[2])) ||
     (vector[1]-vector[0])%304 || (vector[2]-vector[0])%304)return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
  const auto count=(vector[1]-vector[0])/304;if(count>16384 || !number(count,4))return FailCapture(d,CaptureFailure::ValueBudget,{facts.subject});
  // 最短 costume 条目为配置9B、标志2B、文件9B、层数4B，超出比较区的目录无须分配。
  if(std::uint64_t(count)*24>output.size()-used)return FailCapture(d,CaptureFailure::ValueBudget,{facts.subject});
  auto costumes=Allocate(std::uint64_t(count)*sizeof(PreparedPlayerCostume),alignof(PreparedPlayerCostume),d);
  if(d.reason!=CaptureFailure::None)return false;
  player.costume_data=vector[0];
  player.costumes={reinterpret_cast<PreparedPlayerCostume *>(costumes.data()),count};
  for(std::uint32_t i=0;i<count;++i){const auto address=vector[0]+i*304;
    auto &projection=*std::construct_at(player.costumes.data()+i);
    std::uint32_t item=0;std::array<std::uint8_t,2> flags{};
    evidence(host_field::CostumeSpriteDesc::kLayout,host_field::CostumeSpriteDesc::item);
    if(!Tracked(std::uint64_t(address)+0x114,item,d,{address,E::CostumeItem})||!ItemResource(item,projection.item,d)||!link(projection.item))return false;
    evidence(host_field::CostumeSpriteDesc::kLayout,host_field::CostumeSpriteDesc::flying);
    if(!Tracked(std::uint64_t(address)+0x11d,flags,d,{address,E::CostumeFlags})||flags[0]>1||flags[1]>1||
       !append(std::as_bytes(std::span(flags))))return invalid(CaptureFailure::InvalidValue);
    if(!sprite(address,projection.sprite))return false;
  }
  for(unsigned i=0;i<15;++i){const auto address=std::uint64_t(facts.address)+0x122c+i*16;std::int32_t selected=0,layer=0;std::uint8_t body=0;
    const auto owner=std::uint32_t(address);
    evidence(host_field::PlayerCostumeMap::kLayout,host_field::PlayerCostumeMap::index);
    if(!Tracked(address,selected,d,{owner,E::CostumeIndex})||selected< -1||selected>=65535)return invalid(CaptureFailure::InvalidValue);
    evidence(host_field::PlayerCostumeMap::kLayout,host_field::PlayerCostumeMap::layer_id);
    if(selected>=0&&!Tracked(address+4,layer,d,{owner,E::CostumeLayer}))return false;
    evidence(host_field::PlayerCostumeMap::kLayout,host_field::PlayerCostumeMap::body);
    auto &projection=player.map[i];projection={layer,selected<0?kNullIndex:std::uint16_t(selected)};
    if(!Tracked(address+12,body,d,{owner,E::CostumeBody})||body>1||!number(projection.index,2)||
       !number(std::bit_cast<std::uint32_t>(projection.layer),4)||!number(body,1))return invalid(CaptureFailure::InvalidValue);
  }
  auto previous=std::span(topology_bytes).subspan(std::size_t(index)*capacity.topology_bytes_per_body,capacity.topology_bytes_per_body);
  const bool same_identity=entry.subject.space==facts.subject.space && entry.subject.local==facts.subject.local && entry.subject.use==facts.subject.use &&
    entry.subject.shape==facts.subject.shape && entry.subject.binding==facts.subject.binding;
  if(!same_identity || entry.size!=used || !std::equal(output.begin(),output.begin()+used,previous.begin())){
    if(entry.revision==UINT32_MAX)return FailCapture(d,CaptureFailure::SequenceExhausted,{facts.subject});
    ++entry.revision;entry.ordinary=0;entry.subject=facts.subject;entry.size=used;std::copy_n(output.begin(),used,previous.begin());}
  topology_revision=entry.revision;entry.subject.topology=entry.revision;
  player.ready=true;return true;
}

// 只按当前 Player 的 canonical 路径定位投影；其他 draw/afterimage Sprite 沿用各自的来源。
const PreparedPlayerSprite *EntitySource::Impl::PlayerSprite(std::span<const CodecPathStep> path) const noexcept {
  using namespace host_field;
  if(!player.ready||path.empty()||path.front().layout!=EntityPlayer::kLayout)return nullptr;
  const auto &parent=path.front();
  if(path.size()==1){
    if(parent.field==EntityPlayer::sprite_0&&parent.element==kNullIndex)return &player.sprites[0];
    if(parent.field==EntityPlayer::inline_sprites_0&&parent.element<13)return &player.sprites[parent.element+1];
  }
  if(path.size()==2&&parent.field==EntityPlayer::_costumeSpriteDescs_0&&parent.element<player.costumes.size()&&
     path[1].layout==CostumeSpriteDesc::kLayout&&path[1].field==CostumeSpriteDesc::sprite_0&&path[1].element==kNullIndex)
    return &player.costumes[parent.element].sprite;
  return nullptr;
}
const PreparedPlayerCostume *EntitySource::Impl::PlayerCostume(const CaptureFieldAccess &a) const noexcept {
  using namespace host_field;
  const auto path=a.location.parents;
  if(!player.ready||a.root.layout!=CostumeSpriteDesc::kLayout||a.field.index!=CostumeSpriteDesc::item||path.size()!=1)return nullptr;
  const auto &parent=path.front();
  if(parent.layout!=EntityPlayer::kLayout||parent.field!=EntityPlayer::_costumeSpriteDescs_0||parent.element>=player.costumes.size())return nullptr;
  const auto &costume=player.costumes[parent.element];
  return costume.sprite.address==a.root.address?&costume:nullptr;
}
const PreparedPlayerMap *EntitySource::Impl::PlayerMap(const CaptureFieldAccess &a) const noexcept {
  using namespace host_field;
  const auto path=a.location.parents;
  if(!player.ready||a.root.layout!=PlayerCostumeMap::kLayout||path.size()!=1)return nullptr;
  const auto &parent=path.front();
  if(parent.layout!=EntityPlayer::kLayout||parent.field!=EntityPlayer::_playerCostumeMap||parent.element>=player.map.size()||
     std::uint64_t(facts.address)+0x122c+parent.element*16!=a.root.address)return nullptr;
  return &player.map[parent.element];
}
bool EntitySource::Impl::PlayerLayerImage(const CaptureFieldAccess &a,CaptureLink &out) const noexcept {
  using namespace host_field;
  const auto path=a.location.parents;
  if(a.root.layout!=LayerState::kLayout||a.field.index!=LayerState::image_selection||path.empty())return false;
  const auto &layer=path.back();
  if(layer.layout!=ANM2::kLayout||layer.field!=ANM2::layer_states)return false;
  const auto *sprite=PlayerSprite(path.first(path.size()-1));
  if(!sprite||layer.element>=sprite->layer_count||std::uint64_t(sprite->layer_data)+std::uint64_t(layer.element)*160!=a.root.address)return false;
  const auto bytes=sprite->images.subspan(std::size_t(layer.element)*9,9);
  out={CapturedReferenceKind(std::to_integer<unsigned>(bytes[0])),0};
  for(unsigned i=0;i<8;++i)out.handle|=std::uint64_t(std::to_integer<unsigned>(bytes[i+1]))<<(i*8);
  return true;
}
const PreparedPlayerSprite *EntitySource::Impl::PlayerAnimation(const CaptureFieldAccess &a) const noexcept {
  using namespace host_field;
  const auto path=a.location.parents;
  if(a.root.layout!=AnimationState::kLayout||path.empty())return nullptr;
  const auto &slot=path.back();
  if(slot.layout!=ANM2::kLayout||(slot.field!=ANM2::main_state&&slot.field!=ANM2::overlay_state))return nullptr;
  const auto *sprite=PlayerSprite(path.first(path.size()-1));
  const auto offset=slot.field==ANM2::main_state?0x30u:0x50u;
  return sprite&&std::uint64_t(sprite->address)+offset==a.root.address?sprite:nullptr;
}
// 动画选择只在实际选块使用时解析。先核回指与 loaded，再读活动槽，未 loaded 时不碰残留指针。
bool EntitySource::Impl::ReadPlayerAnimation(const CaptureFieldAccess &a,const PreparedPlayerSprite &sprite,
    std::uint32_t &owner,std::uint32_t &animation,CaptureDiagnostic &d) noexcept {
  using E=NativeEvidenceUse;
  animation=0;
  if(!Tracked(a.root.address,owner,d,{a.root.address,E::AnimationOwner})||!owner)return Reject(a,d,CaptureFailure::InvalidValue);
  std::uint8_t loaded=0;
  if(owner==sprite.address)loaded=sprite.file.kind!=CapturedReferenceKind::Null;
  else if(!Tracked(std::uint64_t(owner)+0x109,loaded,d,{owner,E::Loaded})||loaded>1)return Reject(a,d,CaptureFailure::InvalidValue);
  return !loaded||Tracked(std::uint64_t(a.root.address)+4,animation,d,{a.root.address,E::Animation});
}
}
