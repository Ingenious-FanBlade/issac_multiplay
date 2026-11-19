#include "game/client/scene/client_flat_install.h"
#include <algorithm>
#include <cstring>

namespace cs::game {
namespace {
namespace S=state;
using Bytes=std::span<const std::byte>;
using Kind=ClientBinarySpecial;
using Sequence=ClientBinaryFlatSequence;
enum class Pass { Owners, Values, Graphics, Drift, References, Retire };

bool Borrowed(const S::FlatPatch &patch) noexcept {
  const std::string_view read=patch.host;
  return read=="familiar_player_to_member"||read=="player__heldEntity_to_member"||read=="player__backupPlayer_to_member";
}
bool CarriesReferences(const S::FlatPatch &patch,unsigned depth=0) noexcept {
  const auto has=[depth](unsigned id){
    const auto *layout=S::FindFlatLayout(std::uint16_t(id));
    return layout&&std::any_of(layout->patches.begin(),layout->patches.end(),[depth](const auto &p){return CarriesReferences(p,depth+1);});
  };
  return depth<S::kFlatMaxDepth&&((patch.kind==S::FlatPatchKind::Reference&&patch.source==S::FieldSource::Object)||
    (patch.child_layout&&has(patch.child_layout))||std::any_of(patch.choices.begin(),patch.choices.end(),has));
}

// 父 owner 的资源先于原位写段，子数组始终在 owner 完成后重取承载。
class FlatNative {
public:
  FlatNative(const ClientFlatNativeContext &context,const S::ClientFlatWritePlan &plan,ClientFlatNativeState &state,
             bool full,std::uint32_t sample=0) noexcept
    :context_(&context),plan_(&plan),state_(state),image_(plan.image.get()),sample_(sample),full_(full) {}
  FlatNative(ClientFlatNativeState &state,bool retiring,std::span<const ClientLiveRetirement> leaving) noexcept
    :state_(state),leaving_(leaving),retiring_(retiring) {}

  bool Root(void *root,const S::FlatLayout &layout,Pass pass) {
    raw_window_=false;
    result_.layout=layout.id;
    const auto main=image_?image_->Bytes().first(layout.mirror_size):Bytes{};
    if(layout.id==197&&(pass==Pass::Values||pass==Pass::Drift)){
      const auto first=std::find_if(layout.fields.begin(),layout.fields.end(),[](const auto &field){
        return std::string_view(field.name)=="_playerCostumeMap[0].index";
      });
      if(first==layout.fields.end())return Bad("flat_native_costume_map_layout",S::kNullIndex);
      costume_group_=first->group;
      if(pass==Pass::Values&&context_->stream==S::BodyStream::World&&context_->fresh&&
         !CostumeMap(root,layout,{},true))return false;
    }
    if(!Instance(root,layout,main,pass,0))return false;
    if(costume_group_!=S::kNullIndex&&pass==Pass::Values&&context_->stream==S::BodyStream::Body)
      return PublishCostumeMap(root,layout);
    return true;
  }
  ClientFlatNativeResult Result() const noexcept {return result_;}

  bool Bind(void *root,const S::FlatLayout &layout) {
    state_.references_uncertain=true;
    state_.edges.clear();
    if(!Root(root,layout,Pass::References))return false;
    state_.references_uncertain=false;
    return true;
  }

private:
  bool Bad(const char *reason,std::uint16_t ordinal) noexcept {
    result_.reason=reason;result_.field=ordinal;
    return false;
  }
  bool Current() noexcept {
    raw_window_=false;
    region_.Clear();
    return context_->current(context_->context);
  }
  bool Enabled(std::uint16_t group,unsigned depth) const noexcept {
    return full_||depth||((plan_->groups[group/32]>>(group%32))&1u)!=0;
  }
  std::uint16_t Ordinal(const S::FlatLayout &layout,const S::FlatWriteSegment &segment) const noexcept {
    return layout.byte_fields[segment.mirror_offset];
  }

  bool Instance(void *root,const S::FlatLayout &layout,Bytes main,Pass pass,unsigned depth) {
    if(!root||depth>=S::kFlatMaxDepth)return Bad("flat_native_instance",S::kNullIndex);
    result_.layout=layout.id;
    const auto *native=S::FindLayout(layout.id);
    if(!native)return Bad("flat_native_layout",S::kNullIndex);
    const auto extent=native->native_size;
    const auto flow=unsigned(context_?context_->stream:state_.stream);
    const bool references=pass==Pass::References||pass==Pass::Retire;
    const auto main_offset=main.empty()?0u:Offset(main);
    if(pass==Pass::Values||pass==Pass::Drift){
      for(std::uint16_t i=0;i<layout.write_groups.size();++i){
        if(!depth&&i==costume_group_)continue;
        if(pass==Pass::Values&&!Enabled(i,depth))continue;
        for(const auto &segment:layout.write_groups[i].segments){
          if(!depth&&unsigned(segment.stream)!=flow)continue;
          if(!Raw(root,extent,segment.native_offset,main.subspan(segment.mirror_offset,segment.size),pass==Pass::Drift))
            return Bad(pass==Pass::Drift?"flat_native_drift":"flat_native_copy",Ordinal(layout,segment));
        }
      }
    }
    for(const auto &patch:layout.patches){
      result_.layout=layout.id;
      if(patch.field==S::kNullIndex||(!depth&&!S::FlatFieldInStream(layout,patch.field,flow)))continue;
      const auto &field=layout.fields[patch.field];
      if(!depth&&field.group==costume_group_)continue;
      if(references&&!CarriesReferences(patch))continue;
      if(pass==Pass::Retire&&(patch.header==S::FlatHeader::Weapon||
                              (patch.header==S::FlatHeader::Deque&&patch.child_layout==10))){
        if(patch.native_offset>extent||patch.native_size>extent-patch.native_offset||
           !ClientBinaryFlatOwnedReferences(static_cast<std::byte *>(root)+patch.native_offset,
             patch.header==S::FlatHeader::Weapon,this,[](void *context,void *slot) noexcept {
               return static_cast<FlatNative *>(context)->ClearSlot(slot,false);
             }))return Bad("flat_native_retire_owner",patch.field);
        continue;
      }
      if(patch.kind==S::FlatPatchKind::Reference){
        if(references&&(!image_||image_->Active(main_offset+patch.mirror_offset))&&
           !Reference(root,extent,patch,field,main_offset,pass))return false;
        continue;
      }
      if(!image_)continue;
      if(!references&&pass!=Pass::Drift&&!patch.frame_relative&&!Enabled(field.group,depth))continue;
      if(!image_->Active(main_offset+patch.mirror_offset)){
        if(pass==Pass::Values&&std::string_view(patch.host)=="player_current_record_node"&&
           !SequenceResize(static_cast<std::byte *>(root)+patch.owner_offset,Sequence::Records,165,40,0,patch.field))return false;
        continue;
      }
      const auto slot=main.subspan(patch.mirror_offset,patch.size);
      const auto tail=image_->Extra(Offset(slot));
      const std::string_view read=patch.host;
      const bool replace=read=="anm2_selected_file_to_resource"||read=="anm2_animation_to_index";
      if(pass==Pass::Owners&&!replace)continue;
      if(pass==Pass::Values&&replace)continue;
      if(patch.header==S::FlatHeader::LayerArray){
        if(!Layers(root,extent,patch,slot,pass,depth))return false;
      }else if(patch.kind==S::FlatPatchKind::Owner){
        if(!Owner(root,extent,patch,slot,pass,depth))return false;
      }else if(patch.header==S::FlatHeader::Vector||patch.header==S::FlatHeader::Deque||
               patch.header==S::FlatHeader::InlinePrefix||patch.header==S::FlatHeader::Tree||
               patch.header==S::FlatHeader::Unordered||patch.header==S::FlatHeader::Aggregate){
        if(!depth&&layout.id==197&&pass==Pass::Values&&read=="entityplayer__costumeSpriteDescs_read"&&
           !CostumeMap(root,layout,{},true))return false;
        if(!Container(root,extent,patch,slot,tail,pass,depth))return false;
      }else if(!references&&pass!=Pass::Graphics&&patch.source!=S::FieldSource::Synthetic){
        if(pass==Pass::Drift&&patch.field!=sample_%layout.fields.size())continue;
        if(!Leaf(root,extent,patch,slot,tail,read,pass))return false;
      }
    }
    return true;
  }

  // 原生绘制把非负映射直接用作向量下标。先闭合全部选择，再换承载，最后提交配对的 World 映射。
  bool CostumeMap(void *root,const S::FlatLayout &layout,Bytes world,bool clear) {
    result_.layout=layout.id;
    const auto extent=S::FindLayout(layout.id)->native_size;
    if(clear)state_.costume_topology=0;
    for(const auto &patch:layout.patches){
      if(patch.field==S::kNullIndex||layout.fields[patch.field].group!=costume_group_)continue;
      if(clear){
        if(std::string_view(patch.host)!="playercostumemap_index_read")continue;
        const std::int32_t empty=-1;
        if(!Raw(root,extent,patch.native_offset,std::as_bytes(std::span(&empty,1)),false))
          return Bad("flat_native_costume_map_clear",patch.field);
      }else if(!Leaf(root,extent,patch,world.subspan(patch.mirror_offset,patch.size),{},patch.host,Pass::Values))
        return false;
    }
    if(!clear)for(const auto &segment:layout.write_groups[costume_group_].segments)
      if(!Raw(root,extent,segment.native_offset,world.subspan(segment.mirror_offset,segment.size),false))
        return Bad("flat_native_costume_map_copy",Ordinal(layout,segment));
    return true;
  }
  bool PublishCostumeMap(void *root,const S::FlatLayout &layout) {
    const auto revision=S::ClientFlatNamed(layout,*image_,"render_topology_revision");
    if(revision.size()!=4)return Bad("flat_native_costume_revision",S::kNullIndex);
    const auto topology=S::FlatWord(revision);
    if(state_.costume_topology==topology)return true;
    const auto *world=context_->player_world;
    if(!world||!S::ClientFlatPlayerTopologyMatches(*world,*image_))
      return Bad("flat_native_costume_topology",S::kNullIndex);
    if(!Current()||!CostumeMap(root,layout,world->Bytes().first(layout.mirror_size),false))return false;
    state_.costume_topology=topology;
    return true;
  }

  std::uint32_t Offset(Bytes bytes) const noexcept {
    return std::uint32_t(bytes.data()-image_->Bytes().data());
  }
  bool ClearSlot(void *slot,bool borrowed) const noexcept {
    if(retiring_)return borrowed?ClientBinaryFlatBorrow(slot,nullptr,false,false):ClientBinaryClearFlatReference(slot);
    return std::all_of(leaving_.begin(),leaving_.end(),[&](const auto &entry){
      return ClientAtomicClearRetiringReference(slot,entry.address,!borrowed);
    });
  }
  bool Reference(void *root,std::uint32_t extent,const S::FlatPatch &patch,const S::FlatField &field,
                 unsigned main,Pass pass) {
    if(patch.source!=S::FieldSource::Object)return true;
    if(patch.native_size!=sizeof(void *)||patch.native_offset>extent||patch.native_size>extent-patch.native_offset)
      return Bad("flat_native_reference_slot",patch.field);
    auto *slot=static_cast<std::byte *>(root)+patch.native_offset;
    const bool borrowed=Borrowed(patch);
    if(pass==Pass::Retire)return ClearSlot(slot,borrowed);
    const auto *link=image_->Link(main+field.offset);
    const auto target=link?context_->target(context_->context,link->member):ClientFlatTarget{};
    if(link&&!target.address&&field.value_rule==S::FlatValueRule::NonNull)
      return Bad("flat_native_reference_unready",patch.field);
    if(!Current())return Bad("flat_native_reference_window",patch.field);
    ClientAtomicWrite write{ClientAtomicWriteKind::EntityReference,slot,extent-patch.native_offset,sizeof(void *),nullptr,target.address};
    if(borrowed){
      if(!ClientBinaryFlatBorrow(slot,target.address,patch.host!=std::string_view("player__heldEntity_to_member"),true)){
        result_.written=true;
        if(!ClientBinaryFlatBorrow(slot,target.address,patch.host!=std::string_view("player__heldEntity_to_member"),false)||!Current())
          return Bad("flat_native_borrow_reference",patch.field);
      }
    }else if(!ClientAtomicWriteCheck(write)){
      if(!ClientAtomicWritePreflight(write,region_))return Bad("flat_native_reference_preflight",patch.field);
      result_.written=true;
      if(!ClientAtomicWriteApply(write)||!Current())return Bad("flat_native_set_reference",patch.field);
    }
    if(target.address)state_.edges.push_back(target);
    return true;
  }

  bool Raw(void *root,std::uint32_t extent,std::uint32_t offset,Bytes bytes,bool check,std::uint32_t mask=0) noexcept {
    if(offset>extent||bytes.size()>extent-offset)return false;
    ClientAtomicWrite write{mask?ClientAtomicWriteKind::Masked32:ClientAtomicWriteKind::Bytes,
      static_cast<std::byte *>(root)+offset,extent-offset,bytes.size(),bytes.data()};
    write.mask=mask;
    if(check)return ClientAtomicWriteCheck(write);
    // 同一连续纯写段只核一次安装作用域。原生调用经 Current 撤销资格，页保护和 SEH 仍覆盖每次写入。
    if(!raw_window_){
      if(!ClientAtomicCopyWindow())return false;
      raw_window_=true;
    }
    if(!ClientAtomicWritePreflight(write,region_)||!context_->snapshot(context_->context,write,region_))return false;
    result_.written=true;
    if(!ClientAtomicFastWrite(write.destination,write.value,write.size,write.mask))return false;
    result_.bytes+=std::uint32_t(bytes.size());
    return true;
  }

  bool ResolveResource(Bytes slot,void *&out) noexcept {
    out=nullptr;
    if(S::FlatWord(slot)==S::kNullIndex)return true;
    const auto *link=image_->Link(Offset(slot));
    if(!link||!link->is_resource)return false;
    const auto it=std::lower_bound(context_->resources.begin(),context_->resources.end(),link->resource,
      [](const auto &r,const auto &key){return r.key<key;});
    if(it==context_->resources.end()||it->key!=link->resource||!it->address)return false;
    out=it->address;
    return true;
  }

  bool Special(const ClientBinarySpecialWrite &write,bool check,std::uint16_t field) {
    if(!Current())return Bad("flat_native_window",field);
    if(check){
      const auto existing=ClientBinaryReadSpecial(context_->roots,write);
      return (existing==1||existing==0)||Bad("flat_native_special_drift",field);
    }
    if(state_.undo.storage)return Bad("flat_native_interrupted_owner",field);
    const auto bytes=ClientBinarySpecialCharge(write);
    if(bytes)state_.special_charge=memory::Reserve(memory::Pool::Values,bytes,"client_flat_special_owner");
    state_.interrupted=write;
    state_.undo={};
    const bool ok=ClientBinaryApplySpecial(context_->roots,write,state_.undo);
    result_.written|=state_.undo.applied||state_.undo.uncertain;
    const bool released=ClientBinaryCommitFlatSpecial(write,state_.undo);
    if(released){state_.special_charge.reset();state_.interrupted={};state_.undo={};}
    return (ok&&released&&Current())||Bad("flat_native_special_write",field);
  }

  bool Child(void *parent,const S::ClientFlatChild &entry,Pass pass,unsigned depth) {
    const auto *child=S::FindFlatLayout(entry.layout);
    if(!child||!parent)return false;
    const auto bytes=image_->Bytes().subspan(entry.main,child->mirror_size);
    return (pass!=Pass::Values||Instance(parent,*child,bytes,Pass::Owners,depth+1))&&
      Instance(parent,*child,bytes,pass,depth+1);
  }

  bool SequenceResize(void *native,Sequence kind,unsigned layout,unsigned stride,unsigned count,std::uint16_t field) {
    if(!Current())return Bad("flat_native_sequence_window",field);
    result_.written=true;
    return (ClientBinaryResizeFlat(native,kind,layout,stride,count)&&Current())||
      Bad("flat_native_sequence_resize",field);
  }

  bool Layers(void *root,std::uint32_t extent,const S::FlatPatch &patch,Bytes slot,Pass pass,unsigned depth) {
    if(patch.owner_offset>=extent)return Bad("flat_native_sprite_owner",patch.field);
    const auto count=S::FlatWord(slot,4);
    auto *sprite=static_cast<std::byte *>(root)+patch.owner_offset;
    const auto children=image_->Children(Offset(slot));
    if(count!=children.size())return Bad("flat_native_layer_shape",patch.field);
    // 数量由 file owner 的本地 ANM2 决定。禁止把规范计数或指针回写到层头。
    const ClientBinarySpecialWrite count_check{Kind::LayerCount,sprite,nullptr,0,0,count};
    if(ClientBinaryReadSpecial(context_->roots,count_check)!=1)return Bad("flat_native_layer_count",patch.field);
    for(unsigned i=0;i<count;++i){
      auto *layer=ClientBinaryFlatLayer(sprite,i);
      if(!layer)return Bad("flat_native_layer_owner",patch.field);
      if(pass==Pass::Graphics){
        bool touched=false;
        if(!ClientBinaryLayerGraphics(layer,false,touched))return Bad("flat_native_layer_graphics_conflict",patch.field);
      }else if(!Child(layer,children[i],pass,depth)){
        return false;
      }else if(pass==Pass::Values){
        bool touched=false;
        const auto ok=ClientBinaryLayerGraphics(layer,true,touched);
        result_.written|=touched;
        if(!ok||!Current())return Bad("flat_native_layer_graphics",patch.field);
      }
    }
    return true;
  }

  // 每次从当前原生头取元素，尤其是触发原生 resize 或替换 owner 之后。
  bool Container(void *root,std::uint32_t extent,const S::FlatPatch &patch,Bytes slot,Bytes tail,Pass pass,unsigned depth) {
    if(patch.native_offset>extent||patch.native_size>extent-patch.native_offset)
      return Bad("flat_native_sequence_range",patch.field);
    auto *native=static_cast<std::byte *>(root)+patch.native_offset;
    const auto header=S::FlatContainer(slot,patch.header);
    const auto children=image_->Children(Offset(slot));
    const auto count=header.count;
    using Associative=ClientBinaryFlatAssociative;
    if(patch.header==S::FlatHeader::Tree||patch.header==S::FlatHeader::Aggregate||patch.header==S::FlatHeader::Unordered){
      if(pass!=Pass::Values)return true;
      const auto kind=patch.header==S::FlatHeader::Tree?Associative::Map:
        patch.header==S::FlatHeader::Aggregate?Associative::Innate:Associative::Blocked;
      S::ClientStateVector<std::byte> pairs(state_.edges.get_allocator());
      if(kind!=Associative::Blocked){
        const auto *layout=S::FindFlatLayout(patch.child_layout);
        if(!layout||children.size()!=count||layout->fields.size()!=2)return Bad("flat_native_associative_shape",patch.field);
        pairs.resize(std::size_t(count)*8);
        for(unsigned i=0;i<count;++i){
          const auto bytes=image_->Bytes().subspan(children[i].main,layout->mirror_size);
          for(unsigned j=0;j<2;++j){
            const auto &field=layout->fields[j];
            if(field.size!=4)return Bad("flat_native_associative_field",patch.field);
            std::memcpy(pairs.data()+i*8+j*4,bytes.data()+field.offset,4);
          }
        }
        tail=pairs;
      }
      if(tail.size()!=std::uint64_t(count)*(kind==Associative::Blocked?4:8)||!Current())
        return Bad("flat_native_associative_range",patch.field);
      result_.written=true;
      return (ClientBinaryFlatItems(native,kind,tail.data(),count)&&Current())||Bad("flat_native_associative_items",patch.field);
    }
    const auto kind=patch.header==S::FlatHeader::Deque?Sequence::Deque:
      patch.header==S::FlatHeader::InlinePrefix?Sequence::Inline:Sequence::Vector;
    if(pass==Pass::Values&&!SequenceResize(native,kind,patch.child_layout,patch.stride,count,patch.field))return false;
    if(!patch.child_layout){
      if(pass!=Pass::Values&&pass!=Pass::Drift)return true;
      if(tail.size()!=std::uint64_t(count)*patch.stride)return Bad("flat_native_sequence_scalar_shape",patch.field);
      if(!count)return true;
      auto *first=ClientBinaryFlatElement(native,kind,0,patch.stride,count,0);
      return (first&&Raw(first,std::uint32_t(tail.size()),0,tail,pass==Pass::Drift))||Bad("flat_native_sequence_scalar",patch.field);
    }
    if(children.size()!=count)return Bad("flat_native_sequence_shape",patch.field);
    for(unsigned i=0;i<count;++i){
      auto *element=ClientBinaryFlatElement(native,kind,patch.child_layout,patch.stride,count,i);
      if(!element)return Bad("flat_native_sequence_element",patch.field);
      if(!Child(element,children[i],pass,depth))return false;
    }
    return true;
  }

  bool Owner(void *root,std::uint32_t extent,const S::FlatPatch &patch,Bytes slot,Pass pass,unsigned depth) {
    if(patch.owner_offset>=extent)return Bad("flat_native_owner_range",patch.field);
    auto *base=static_cast<std::byte *>(root);
    const bool install=pass==Pass::Values;
    const auto children=image_->Children(Offset(slot));
    if(patch.header==S::FlatHeader::Weapon){
      if(patch.native_offset>extent||patch.native_size>extent-patch.native_offset||slot.size()!=20)
        return Bad("flat_native_weapon_range",patch.field);
      auto next=children.begin();
      for(unsigned i=0;i<5;++i){
        const auto *entry=next!=children.end()&&next->element==i?&*next++:nullptr;
        unsigned type=0;
        if(entry){
          const auto *layout=S::FindFlatLayout(entry->layout);
          if(!layout)return Bad("flat_native_weapon_layout",patch.field);
          const auto field=std::find_if(layout->fields.begin(),layout->fields.end(),[](const auto &f){
            return std::string_view(f.name)=="weapon_type";
          });
          if(field==layout->fields.end()||field->size!=4)return Bad("flat_native_weapon_type",patch.field);
          type=S::FlatWord(image_->Bytes(),entry->main+field->offset);
        }
        void *payload=nullptr;
        if(install){if(!Current())return Bad("flat_native_weapon_window",patch.field);result_.written=true;}
        if(!ClientBinaryFlatWeapon(base+patch.native_offset+i*4,entry?entry->layout:0,type,install,payload))
          return Bad("flat_native_weapon_owner",patch.field);
        if(entry&&!Child(payload,*entry,pass,depth))return false;
      }
      return next==children.end()||Bad("flat_native_weapon_coverage",patch.field);
    }
    const bool present=S::FlatWord(slot)!=0;
    if(children.size()!=unsigned(present))return Bad("flat_native_owner_shape",patch.field);
    void *payload=nullptr;
    const std::string_view read=patch.host;
    if(read=="player_current_record_node"){
      auto *list=base+patch.owner_offset;
      if(install&&!SequenceResize(list,Sequence::Records,165,40,unsigned(present),patch.field))return false;
      if(present)payload=ClientBinaryFlatElement(list,Sequence::Records,165,40,1,0);
    }else if(patch.source==S::FieldSource::Companion){
      if(present)payload=ClientBinaryFlatPlus(base+patch.owner_offset,patch.child_layout);
    }else{
      if(patch.native_offset>extent||patch.native_size!=8||patch.native_size>extent-patch.native_offset)
        return Bad("flat_native_shared_range",patch.field);
      if(install){if(!Current())return Bad("flat_native_shared_window",patch.field);result_.written=true;}
      if(!ClientBinaryFlatShared(base+patch.native_offset,patch.child_layout,present,install,payload))
        return Bad("flat_native_shared_owner",patch.field);
    }
    if(!present)return true;
    if(!payload)return Bad("flat_native_owner_child",patch.field);
    return Child(payload,children.front(),pass,depth);
  }

  bool Leaf(void *root,std::uint32_t extent,const S::FlatPatch &patch,Bytes slot,Bytes tail,std::string_view read,Pass pass) {
    const bool check=pass==Pass::Drift;
    const auto field=patch.field;
    if(patch.native_offset>extent||patch.native_size>extent-patch.native_offset)
      return Bad("flat_native_patch_range",field);
    auto *base=static_cast<std::byte *>(root);
    ClientBinarySpecialWrite write;
    if(patch.kind==S::FlatPatchKind::Resource){
      if(!ResolveResource(slot,write.resource))return Bad("flat_native_resource_unready",field);
      if(read=="anm2_selected_file_to_resource")write.kind=Kind::File;
      else if(read=="anm2_animation_to_index")write.kind=Kind::Animation;
      else if(read=="layer_spritesheet_path_to_resource")write.kind=Kind::LayerPath;
      else if(read=="image_base_name_to_resource")write.kind=Kind::LayerImage;
      else if(read=="grid_rock_rubble_to_resource")write.kind=Kind::Rubble;
      else if(read=="entity_selected_config_to_resource"||std::string_view(patch.target)=="item-config")write.kind=Kind::Config;
      else return Bad("flat_native_resource_handler",field);
      if(patch.owner_offset>patch.native_offset)return Bad("flat_native_resource_owner",field);
      write.parent=base+patch.owner_offset;write.offset=patch.native_offset-patch.owner_offset;write.size=patch.native_size;
      // 平铺 Body 完整拥有 Player 主 Sprite，包括文件、动画和图层，可以承接首次 Body 的重建。
      return Special(write,check,field);
    }
    if(patch.kind==S::FlatPatchKind::Container){
      if(patch.header==S::FlatHeader::String){
        const auto count=S::FlatWord(slot,16);
        const auto text=count<=15?slot.first(count):tail;
        write.kind=Kind::DefaultName;write.parent=base+patch.owner_offset;
        write.offset=patch.native_offset-patch.owner_offset;write.size=patch.native_size;
        write.bytes=text.data();write.length=text.size();
      }else if(patch.header==S::FlatHeader::CString){
        write.kind=Kind::Text;write.parent=base+patch.owner_offset;
        write.offset=patch.native_offset-patch.owner_offset;write.size=patch.native_size;
        write.bytes=tail.data();write.length=tail.size();
      }else if(patch.header==S::FlatHeader::Array){
        write.kind=read=="read_null_layer_frame_selectors"?Kind::NullFrames:Kind::LayerFrames;
        write.parent=base+patch.owner_offset;write.count=S::FlatWord(slot);
        write.bytes=tail.data();write.length=tail.size();
      }else return Bad("flat_native_container_handler",field);
      return Special(write,check,field);
    }
    if(patch.kind==S::FlatPatchKind::Copy)
      return Raw(root,extent,patch.native_offset,slot,check)||Bad("flat_native_conditional_copy",field);
    if(patch.kind!=S::FlatPatchKind::Convert)return Bad("flat_native_patch_handler",field);
    if(read=="entity_index_to_draw_order"||read=="npc_controller_to_ui_mode"||
       read=="incubus_weapon_to_charge_bar_amount")return true; // 既有显示 owner 消费这些纯值事实。
    if(read=="read_grid_door_light_bit"||read=="read_grid_door_draw_enabled"){
      write.kind=read=="read_grid_door_light_bit"?Kind::DoorLight:Kind::DoorAlpha;
      write.parent=base+patch.owner_offset;write.offset=patch.native_offset-patch.owner_offset;write.size=patch.native_size;
      write.bytes=slot.data();write.length=slot.size();
      return Special(write,check,field);
    }
    std::array<std::byte,8> converted{};
    Bytes value;
    auto mask=patch.value_mask;
    if(read=="entity_plus_flying_to_value"){
      const auto n=std::to_integer<unsigned>(slot[0]);
      if(n>1&&n!=255)return Bad("flat_native_flying",field);
      converted[0]=std::byte(n==1);converted[1]=std::byte(n!=255);
      value=std::span(converted).first(2);
    }else if(read=="entity_plus_water_to_value")value=slot.first(5);
    else if(read=="ExpireFrame"){
      const auto frame=S::FlatWord(slot)-context_->frame;
      std::memcpy(converted.data(),&frame,4);value=std::span(converted).first(4);
    }else if(read=="playercostumemap_index_read"){
      std::uint16_t n;
      std::memcpy(&n,slot.data(),sizeof(n));
      const std::int32_t index=n==S::kNullIndex?-1:n;
      std::memcpy(converted.data(),&index,4);value=std::span(converted).first(4);
    }else if(read=="queueitemdata_flags_read"){
      converted[0]=slot[0]==std::byte{}?std::byte{}:std::byte{2};
      value=std::span(converted).first(4);mask=2;
    }else if(patch.native_size==slot.size())value=slot;
    else return Bad("flat_native_conversion_handler",field);
    return Raw(root,extent,patch.native_offset,value,check,mask)||Bad("flat_native_converted_copy",field);
  }

  const ClientFlatNativeContext *context_=nullptr;
  const S::ClientFlatWritePlan *plan_=nullptr;
  ClientFlatNativeState &state_;
  const S::ClientFlatImage *image_=nullptr;
  std::span<const ClientLiveRetirement> leaving_;
  ClientAtomicReadRegion region_;
  ClientFlatNativeResult result_;
  std::uint32_t sample_=0;
  std::uint16_t costume_group_=S::kNullIndex;
  bool full_=false,retiring_=false,raw_window_=false;
};
}

ClientFlatNativeResult ClientWriteFlat(const ClientFlatNativeContext &context,void *root,const state::FlatLayout &layout,
    const state::ClientFlatWritePlan &plan,bool full,ClientFlatNativeState &applied) noexcept {
  try{
    applied.references_uncertain=true;
    FlatNative writer(context,plan,applied,full);
    if(writer.Root(root,layout,Pass::Owners))writer.Root(root,layout,Pass::Values);
    return writer.Result();
  }catch(...){return {"flat_native_write_exception",layout.id,state::kNullIndex,0,true};}
}
ClientFlatNativeResult ClientBindFlat(const ClientFlatNativeContext &context,void *root,const state::FlatLayout &layout,
    const state::ClientFlatWritePlan &plan,bool full,ClientFlatNativeState &applied) noexcept {
  try{
    FlatNative writer(context,plan,applied,full);
    writer.Bind(root,layout);
    return writer.Result();
  }catch(...){return {"flat_native_reference_exception",layout.id,state::kNullIndex,0,true};}
}
ClientFlatNativeResult ClientCheckFlat(const ClientFlatNativeContext &context,void *root,const state::FlatLayout &layout,
    const state::ClientFlatWritePlan &plan,ClientFlatNativeState &applied,std::uint32_t sample,bool graphics,bool full) noexcept {
  try{
    FlatNative writer(context,plan,applied,full,sample);
    writer.Root(root,layout,graphics?Pass::Graphics:Pass::Drift);
    return writer.Result();
  }catch(...){return {"flat_native_read_exception",layout.id};}
}
bool ClientClearFlat(void *root,const state::FlatLayout &layout,ClientFlatNativeState &applied,
                         bool retiring,std::span<const ClientLiveRetirement> leaving) noexcept {
  // 结构中途失败也只读当前活头；不能靠前一次镜像中的长度拆边。
  try{
    FlatNative writer(applied,retiring,leaving);
    if(!writer.Root(root,layout,Pass::Retire))return false;
    if(retiring){applied.edges.clear();applied.references_uncertain=false;}
    return true;
  }catch(...){return false;}
}
}
