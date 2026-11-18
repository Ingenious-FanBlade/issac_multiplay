#include "game/client/scene/client_flat_install.h"
#include <algorithm>
#include <cstring>

namespace cs::game {
namespace {
namespace S=state;
using Bytes=std::span<const std::byte>;
using Kind=ClientBinarySpecial;
enum class Pass { Owners, Values, Graphics, Drift };

// 父 owner 的资源先于原位写段，子数组始终在 owner 完成后重取承载。
class FlatNative {
public:
  FlatNative(const ClientFlatNativeContext &context,const S::ClientFlatWritePlan &plan,ClientFlatNativeState &state,
             bool full,std::uint32_t sample=0) noexcept
    :context_(context),plan_(plan),state_(state),sample_(sample),full_(full) {}

  bool Root(void *root,const S::FlatLayout &layout,Pass pass) {
    result_.layout=layout.id;
    return Instance(root,layout,plan_.image->Bytes().first(layout.mirror_size),pass,0);
  }
  ClientFlatNativeResult Result() const noexcept {return result_;}

  bool Bind(void *root,const S::FlatLayout &layout) {
    result_.layout=layout.id;
    const auto bytes=plan_.image->Bytes().first(layout.mirror_size);
    const auto *native=S::FindLayout(layout.id);
    if(!native)return Bad("flat_native_root_layout",S::kNullIndex);
    const bool rewrite=state_.references_uncertain;
    state_.references_uncertain=true;
    std::uint8_t ordinal=0;
    for(const auto &patch:layout.patches)if(patch.kind==S::FlatPatchKind::Reference){
      if(ordinal==state_.edges.size()||patch.native_size!=sizeof(void *)||
         patch.native_offset>native->native_size||patch.native_size>native->native_size-patch.native_offset)
        return Bad("flat_native_reference_slot",patch.field);
      const auto &field=layout.fields[patch.field];
      if(!S::ClientFlatSelected(layout,patch,bytes))continue;
      auto &edge=state_.edges[ordinal++];
      const auto *link=plan_.image->Link(field.offset);
      const auto target=link?context_.target(context_.context,link->member):ClientFlatTarget{};
      if(link&&!target.address&&field.value_rule==S::FlatValueRule::NonNull)
        return Bad("flat_native_reference_unready",patch.field);
      const bool rebound=edge.target.address!=target.address||edge.target.instance!=target.instance||
        edge.target.identity!=target.identity;
      if(rewrite||rebound||Enabled(field.group,0)){
        auto *slot=static_cast<std::byte *>(root)+patch.native_offset;
        ClientAtomicWrite write{ClientAtomicWriteKind::EntityReference,slot,native->native_size-patch.native_offset,
          sizeof(void *),nullptr,target.address};
        if(!Current()||!ClientAtomicWritePreflight(write,region_))return Bad("flat_native_reference_preflight",patch.field);
        if(!ClientAtomicWriteCheck(write)){
          result_.written=true;
          if(!ClientAtomicWriteApply(write)||!Current())return Bad("flat_native_set_reference",patch.field);
        }
      }
      edge={patch.native_offset,target};
      state_.count=ordinal;
    }
    state_.count=ordinal;
    state_.references_uncertain=false;
    return true;
  }

private:
  bool Bad(const char *reason,std::uint16_t ordinal) noexcept {
    result_.reason=reason;result_.field=ordinal;
    return false;
  }
  bool Current() noexcept {
    region_.Clear();
    return context_.current(context_.context);
  }
  bool Enabled(std::uint16_t group,unsigned depth) const noexcept {
    return full_||depth||((plan_.groups[group/32]>>(group%32))&1u)!=0;
  }
  std::uint16_t Ordinal(const S::FlatLayout &layout,const S::FlatWriteSegment &segment) const noexcept {
    return layout.byte_fields[segment.mirror_offset];
  }

  bool Instance(void *root,const S::FlatLayout &layout,Bytes main,Pass pass,unsigned depth) {
    if(!root||depth>=S::kFlatMaxDepth||!layout.scopes.empty())return Bad("flat_native_instance",S::kNullIndex);
    result_.layout=layout.id;
    const auto *native=S::FindLayout(layout.id);
    if(!native)return Bad("flat_native_layout",S::kNullIndex);
    const auto extent=native->native_size;
    const auto *image=plan_.image.get();
    if(pass==Pass::Values||pass==Pass::Drift){
      for(std::uint16_t i=0;i<layout.write_groups.size();++i){
        if(pass==Pass::Values&&!Enabled(i,depth))continue;
        for(const auto &segment:layout.write_groups[i].segments){
          if(!Raw(root,extent,segment.native_offset,main.subspan(segment.mirror_offset,segment.size),pass==Pass::Drift))
            return Bad(pass==Pass::Drift?"flat_native_drift":"flat_native_copy",Ordinal(layout,segment));
        }
      }
    }
    for(const auto &patch:layout.patches){
      result_.layout=layout.id;
      if(patch.field==S::kNullIndex||patch.kind==S::FlatPatchKind::Reference)continue;
      const auto &field=layout.fields[patch.field];
      if(pass!=Pass::Drift&&!patch.frame_relative&&!Enabled(field.group,depth))continue;
      if(!S::ClientFlatSelected(layout,patch,main))continue;
      const auto slot=main.subspan(patch.mirror_offset,patch.size);
      const auto tail=depth||field.tail==S::kNullIndex?Bytes{}:image->Tail(field.tail);
      const std::string_view read=patch.host;
      const bool replace=read=="anm2_selected_file_to_resource"||read=="anm2_animation_to_index";
      if(pass==Pass::Owners&&!replace)continue;
      if(pass==Pass::Values&&replace)continue;
      if(patch.header==S::FlatHeader::LayerArray){
        if(!Layers(root,extent,patch,slot,tail,pass,depth))return false;
      }else if(patch.header==S::FlatHeader::Owner){
        if(pass==Pass::Graphics)continue;
        if(patch.source!=S::FieldSource::Companion||!S::FlatWord(slot))return Bad("flat_native_plus_header",patch.field);
        const auto *child=S::FindFlatLayout(patch.child_layout);
        auto *parent=ClientBinaryFlatPlus(root,patch.child_layout);
        if(!child||!parent||tail.size()!=child->mirror_size)return Bad("flat_native_plus_owner",patch.field);
        if(!Instance(parent,*child,tail,pass,depth+1))return false;
        result_.layout=layout.id;
      }else if(pass!=Pass::Graphics){
        if(pass==Pass::Drift&&patch.field!=sample_%layout.fields.size())continue;
        if(!Leaf(root,extent,patch,slot,tail,read,pass))return false;
      }
    }
    return true;
  }

  bool Raw(void *root,std::uint32_t extent,std::uint32_t offset,Bytes bytes,bool check) noexcept {
    if(offset>extent||bytes.size()>extent-offset)return false;
    ClientAtomicWrite write{ClientAtomicWriteKind::Bytes,static_cast<std::byte *>(root)+offset,extent-offset,
      bytes.size(),bytes.data()};
    if(check)return ClientAtomicWriteCheck(write);
    if(!ClientAtomicWritePreflight(write,region_)||!context_.snapshot(context_.context,write,region_))return false;
    result_.written=true;
    if(!ClientAtomicWriteApply(write))return false;
    result_.bytes+=std::uint32_t(bytes.size());
    return true;
  }

  bool ResolveResource(Bytes slot,void *&out) noexcept {
    out=nullptr;
    if(S::FlatWord(slot)==S::kNullIndex)return true;
    const auto *link=plan_.image->Link(std::uint32_t(slot.data()-plan_.image->Bytes().data()));
    if(!link||!link->is_resource)return false;
    const auto it=std::lower_bound(context_.resources.begin(),context_.resources.end(),link->resource,
      [](const auto &r,const auto &key){return r.key<key;});
    if(it==context_.resources.end()||it->key!=link->resource||!it->address)return false;
    out=it->address;
    return true;
  }

  bool Special(const ClientBinarySpecialWrite &write,bool check,std::uint16_t field) {
    if(!Current())return Bad("flat_native_window",field);
    if(check){
      const auto existing=ClientBinaryReadSpecial(context_.roots,write);
      return (existing==1||existing==0)||Bad("flat_native_special_drift",field);
    }
    if(state_.undo.storage)return Bad("flat_native_interrupted_owner",field);
    const auto bytes=ClientBinarySpecialCharge(write);
    if(bytes)state_.special_charge=memory::Reserve(memory::Pool::Values,bytes,"client_flat_special_owner");
    state_.interrupted=write;
    state_.undo={};
    const bool ok=ClientBinaryApplySpecial(context_.roots,write,state_.undo);
    result_.written|=state_.undo.applied||state_.undo.uncertain;
    const bool released=ClientBinaryCommitFlatSpecial(write,state_.undo);
    if(released){state_.special_charge.reset();state_.interrupted={};state_.undo={};}
    return (ok&&released&&Current())||Bad("flat_native_special_write",field);
  }

  bool Layers(void *root,std::uint32_t extent,const S::FlatPatch &patch,Bytes slot,Bytes tail,Pass pass,unsigned depth) {
    if(patch.owner_offset>=extent)return Bad("flat_native_sprite_owner",patch.field);
    const auto count=S::FlatWord(slot,4);
    auto *sprite=static_cast<std::byte *>(root)+patch.owner_offset;
    const auto *child=S::FindFlatLayout(patch.child_layout);
    if(!child||std::uint64_t(count)*child->mirror_size!=tail.size())return Bad("flat_native_layer_shape",patch.field);
    // 数量由 file owner 的本地 ANM2 决定。禁止把规范计数或指针回写到层头。
    const ClientBinarySpecialWrite count_check{Kind::LayerCount,sprite,nullptr,0,0,count};
    if(ClientBinaryReadSpecial(context_.roots,count_check)!=1)return Bad("flat_native_layer_count",patch.field);
    for(unsigned i=0;i<count;++i){
      auto *layer=ClientBinaryFlatLayer(sprite,i);
      if(!layer)return Bad("flat_native_layer_owner",patch.field);
      if(pass==Pass::Graphics){
        bool touched=false;
        if(!ClientBinaryLayerGraphics(layer,false,touched))return Bad("flat_native_layer_graphics_conflict",patch.field);
      }else if(!Instance(layer,*child,tail.subspan(std::size_t(i)*child->mirror_size,child->mirror_size),pass,depth+1)){
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
      else if(read=="entity_selected_config_to_resource")write.kind=Kind::Config;
      else return Bad("flat_native_resource_handler",field);
      if(patch.owner_offset>patch.native_offset)return Bad("flat_native_resource_owner",field);
      write.parent=base+patch.owner_offset;write.offset=patch.native_offset-patch.owner_offset;write.size=patch.native_size;
      return Special(write,check,field);
    }
    if(patch.kind==S::FlatPatchKind::Container){
      if(patch.header==S::FlatHeader::String){
        const auto count=S::FlatWord(slot,16);
        const auto text=count<=15?slot.first(count):tail;
        write.kind=Kind::DefaultName;write.parent=base+patch.owner_offset;
        write.offset=patch.native_offset-patch.owner_offset;write.size=patch.native_size;
        write.bytes=text.data();write.length=text.size();
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
    if(read=="entity_index_to_draw_order")return true; // 绘制序号由名册 owner 安装。
    std::array<std::byte,8> converted{};
    Bytes value;
    if(read=="entity_plus_flying_to_value"){
      const auto n=std::to_integer<unsigned>(slot[0]);
      if(n>1&&n!=255)return Bad("flat_native_flying",field);
      converted[0]=std::byte(n==1);converted[1]=std::byte(n!=255);
      value=std::span(converted).first(2);
    }else if(read=="entity_plus_water_to_value")value=slot.first(5);
    else if(read=="ExpireFrame"){
      const auto frame=S::FlatWord(slot)-context_.frame;
      std::memcpy(converted.data(),&frame,4);value=std::span(converted).first(4);
    }else return Bad("flat_native_conversion_handler",field);
    return Raw(root,extent,patch.native_offset,value,check)||Bad("flat_native_converted_copy",field);
  }

  const ClientFlatNativeContext &context_;
  const S::ClientFlatWritePlan &plan_;
  ClientFlatNativeState &state_;
  ClientAtomicReadRegion region_;
  ClientFlatNativeResult result_;
  std::uint32_t sample_;
  bool full_;
};
}

ClientFlatNativeResult ClientWriteFlatTear(const ClientFlatNativeContext &context,void *root,const state::FlatLayout &layout,
    const state::ClientFlatWritePlan &plan,bool full,ClientFlatNativeState &applied) noexcept {
  try{
    FlatNative writer(context,plan,applied,full);
    if(writer.Root(root,layout,Pass::Owners))writer.Root(root,layout,Pass::Values);
    return writer.Result();
  }catch(...){return {"flat_native_write_exception",layout.id,state::kNullIndex,0,true};}
}
ClientFlatNativeResult ClientBindFlatTear(const ClientFlatNativeContext &context,void *root,const state::FlatLayout &layout,
    const state::ClientFlatWritePlan &plan,bool full,ClientFlatNativeState &applied) noexcept {
  FlatNative writer(context,plan,applied,full);
  writer.Bind(root,layout);
  return writer.Result();
}
ClientFlatNativeResult ClientCheckFlatTear(const ClientFlatNativeContext &context,void *root,const state::FlatLayout &layout,
    const state::ClientFlatWritePlan &plan,ClientFlatNativeState &applied,std::uint32_t sample,bool graphics,bool full) noexcept {
  try{
    FlatNative writer(context,plan,applied,full,sample);
    writer.Root(root,layout,graphics?Pass::Graphics:Pass::Drift);
    return writer.Result();
  }catch(...){return {"flat_native_read_exception",layout.id};}
}
bool ClientClearFlatTear(void *root,const state::FlatLayout &layout,ClientFlatNativeState &applied,
                         bool retiring,std::span<const ClientLiveRetirement> leaving) noexcept {
  if(retiring){
    for(const auto &patch:layout.patches)if(patch.kind==state::FlatPatchKind::Reference&&
        !ClientBinaryClearFlatReference(static_cast<std::byte *>(root)+patch.native_offset))return false;
    applied.count=0;
    applied.references_uncertain=false;
    return true;
  }
  if(applied.references_uncertain){
    // SetReference 中途失败时无法沿用缓存边，只拆活槽中仍指向待退休实例的引用。
    for(const auto &patch:layout.patches)if(patch.kind==state::FlatPatchKind::Reference){
      auto *slot=static_cast<std::byte *>(root)+patch.native_offset;
      for(const auto &entry:leaving)
        if(!ClientAtomicClearRetiringReference(slot,entry.address,true))return false;
    }
    return true;
  }
  for(std::uint8_t i=0;i<applied.count;++i){
    auto &edge=applied.edges[i];
    if(!edge.target.address||std::none_of(leaving.begin(),leaving.end(),[&](const auto &entry){
      return entry.identity==edge.target.identity&&entry.address==edge.target.address&&entry.instance==edge.target.instance;
    }))continue;
    if(!ClientAtomicClearRetiringReference(static_cast<std::byte *>(root)+edge.offset,edge.target.address,true))return false;
    edge.target={};
  }
  return true;
}
}
