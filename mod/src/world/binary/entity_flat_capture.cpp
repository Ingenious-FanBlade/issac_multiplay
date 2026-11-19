#include "world/binary/entity_source_internal.h"
#include "state/flat_capture.h"
#include "state_host_fields.inc"
#include <stdexcept>

namespace cs::world::binary {
namespace {
constexpr std::uint32_t kFlatRootLimit=2u<<20;
constexpr std::uint64_t kAddressEnd=std::uint64_t{1}<<32;

// 图层资源命中只在该 Sprite 的本层实例内有效；动画姿态等其他字节仍按表采集。
struct FlatLayerResources {
  SpriteLayerResourceMemo *memo=nullptr;
  std::span<const std::byte> prepared_image; // 拓扑编码的 kind 和 8B handle。
  std::uint32_t prepared_owner=0;
};

bool Qualified(std::string_view requirement,const EntityFacts &facts) noexcept {
  if(requirement.empty())return true;
  if(requirement=="current_body_field_witness_same_use_and_binding"||requirement=="player-equipment-current-window")
    return facts.ready&&facts.player_ready;
  if(requirement=="text-current-use-writer")return facts.ready&&facts.text_ready;
  return facts.ready&&(requirement=="entity-current-window"||requirement=="grid-current-window"||
                      requirement=="entity-current-window-and-stable-incubus-weapon");
}

// 复用同一块预分配字节区。每个容器先为所有元素留出主区，再按顺序追加递归内容。
class FlatNativeCapture {
public:
  FlatNativeCapture(EntitySource::Impl &source,std::span<std::byte> bytes,CaptureValueWriter &writer,
                    CaptureDiagnostic &diagnostic,BodyStream stream) noexcept
    :source_(source),bytes_(bytes),writer_(writer),diagnostic_(diagnostic),stream_(unsigned(stream)) {}

  bool Capture(const CaptureRoot &root) noexcept {
    const auto *layout=FindFlatLayout(root.layout);
    if(!layout || !Reserve(layout->mirror_size,root,kNullIndex))return false;
    return Instance(root,bytes_.first(layout->mirror_size),0);
  }

  std::uint32_t Used() const noexcept {return used_;}

private:
  bool Fail(CaptureFailure reason,const CaptureRoot &root,std::uint16_t field) noexcept {
    FailCapture(diagnostic_,reason,root,field);
    if(!diagnostic_.layout)diagnostic_.layout=root.layout;
    if(diagnostic_.field==kNullIndex)diagnostic_.field=field;
    return false;
  }

  bool Reserve(std::uint64_t size,const CaptureRoot &root,std::uint16_t field) noexcept {
    if(size>bytes_.size()-used_)return Fail(CaptureFailure::ValueBudget,root,field);
    used_+=std::uint32_t(size);
    return true;
  }

  bool Instance(const CaptureRoot &root,std::span<std::byte> mirror,unsigned depth,FlatLayerResources layer={}) noexcept {
    const auto *binding=source_.FlatProgram(root.layout);
    if(!binding)return Fail(CaptureFailure::UnknownLayout,root,kNullIndex);
    if(depth>=kFlatMaxDepth)return Fail(CaptureFailure::TraversalBudget,root,kNullIndex);
    const auto &layout=*binding->layout;
    std::fill(mirror.begin(),mirror.end(),std::byte{});
    source_.evidence_layout=root.layout;
    source_.evidence_field=kNullIndex;
    for(const auto &copy:layout.copies){
      if(!depth&&unsigned(copy.stream)!=stream_)continue;
      if(!Qualified(copy.read_requirement,source_.facts))return Fail(CaptureFailure::MissingQualification,root,kNullIndex);
      if(!source_.window.Copy(root,copy.native_offset,mirror.subspan(copy.mirror_offset,copy.size),diagnostic_))return false;
    }
    // 先补齐条件引用的定长值。其余补丁按表序执行，容器的附加区顺序不会被条件改变。
    if(binding->has_conditions){
      for(std::size_t i=0;i<layout.patches.size();++i){
        const auto &patch=layout.patches[i];
        if(!Early(layout,patch))continue;
        if(!Patch(root,layout,patch,binding->reads[i],mirror,depth,layer))return false;
      }
    }
    for(std::size_t i=0;i<layout.patches.size();++i){
      const auto &patch=layout.patches[i];
      if(binding->has_conditions && Early(layout,patch))continue;
      if(!Patch(root,layout,patch,binding->reads[i],mirror,depth,layer))return false;
    }
    // 空装备和无效服装索引的原生槽可残留旧值，平铺镜像沿用正文的规范清零规则。
    for(const auto &rule:layout.zero_constraints){
      if(!depth&&!FlatFieldInStream(layout,rule.selector,stream_))continue;
      const auto &selector=layout.fields[rule.selector];
      std::uint64_t value=0;
      std::memcpy(&value,mirror.data()+selector.offset,selector.size);
      if(value!=rule.empty)continue;
      for(const auto ordinal:rule.fields){
        const auto &field=layout.fields[ordinal];
        std::fill_n(mirror.begin()+field.offset,field.size,std::byte{});
      }
    }
    return true;
  }

  static bool Early(const FlatLayout &layout,const FlatPatch &patch) noexcept {
    if(!patch.conditions.empty())return false;
    if(patch.kind==FlatPatchKind::Copy||patch.kind==FlatPatchKind::Convert)return true;
    return patch.kind==FlatPatchKind::Resource&&std::any_of(layout.patches.begin(),layout.patches.end(),[&](const auto &consumer){
      return std::any_of(consumer.conditions.begin(),consumer.conditions.end(),[&](const auto &c){return c.selector==patch.field;});
    });
  }

  bool Conditions(const FlatLayout &layout,const FlatPatch &patch,std::span<const std::byte> mirror,
                  bool &selected,const CaptureRoot &root) noexcept {
    selected=true;
    for(const auto &condition:patch.conditions){
      const auto &field=layout.fields[condition.selector];
      if(condition.predicate==FlatPredicate::ItemConfig){
        const auto handle=FlatWord(mirror,field.offset);
        if(!handle||handle==kNullIndex||handle>source_.resource_used)
          return Fail(CaptureFailure::ResourcePending,root,patch.field);
        const auto &config=source_.resources[handle-1];
        if(config.table!=3||config.kind!=7)return Fail(CaptureFailure::ResourcePending,root,patch.field);
        selected=config.values[1]==condition.operand&&config.values[0]<64&&
          (condition.mask&(std::uint64_t{1}<<config.values[0]));
        if(selected&&!config.values[2])return Fail(CaptureFailure::InvalidValue,root,patch.field);
        if(!selected)return true;
        continue;
      }
      if(field.size>8 || (field.kind!=FlatPatchKind::Copy && field.kind!=FlatPatchKind::Convert))
        return Fail(CaptureFailure::MissingHandler,root,patch.field);
      std::uint64_t value=0;
      std::memcpy(&value,mirror.data()+field.offset,field.size);
      switch(condition.predicate){
        case FlatPredicate::Equal:selected=value==condition.operand;break;
        case FlatPredicate::NotEqual:selected=value!=condition.operand;break;
        case FlatPredicate::AnyBits:selected=(value&condition.operand)!=0;break;
        case FlatPredicate::OneOf:selected=value<64 && (condition.mask&(std::uint64_t{1}<<value));break;
        default:return Fail(CaptureFailure::MissingHandler,root,patch.field);
      }
      if(!selected)return true;
    }
    return true;
  }

  bool Patch(const CaptureRoot &root,const FlatLayout &layout,const FlatPatch &patch,EntityRead read,
             std::span<std::byte> mirror,unsigned depth,const FlatLayerResources &layer) noexcept {
    source_.evidence_layout=root.layout;
    source_.evidence_field=patch.field;
    if(!depth&&!FlatFieldInStream(layout,patch.field,stream_))return true;
    if(!Qualified(patch.read_requirement,source_.facts))
      return Fail(CaptureFailure::MissingQualification,root,patch.field);
    bool selected;
    if(!Conditions(layout,patch,mirror,selected,root))return false;
    auto slot=mirror.subspan(patch.mirror_offset,patch.size);
    if(!selected || patch.kind==FlatPatchKind::Zero){
      std::fill(slot.begin(),slot.end(),std::byte{});
      return true;
    }
    if(patch.native_offset!=kDynamicOffset &&
       (patch.native_offset>root.extent || patch.native_size>root.extent-patch.native_offset))
      return Fail(CaptureFailure::AddressRange,root,patch.field);
    const auto address=std::uint64_t(root.address)+patch.native_offset;
    const auto owner=std::uint64_t(root.address)+patch.owner_offset;
    switch(patch.kind){
      case FlatPatchKind::Copy:
        if(source_.window.Copy(root,patch.native_offset,slot,diagnostic_))return true;
        break;
      case FlatPatchKind::Convert:{
        std::uint64_t value=0;
        if(read==EntityRead::Raw){
          if(patch.native_size==slot.size() && source_.window.Copy(root,patch.native_offset,slot,diagnostic_))return true;
          break;
        }
        if(read!=EntityRead::Mask&&read!=EntityRead::Bool&&read!=EntityRead::Order&&read!=EntityRead::Expire&&
           read!=EntityRead::Flying&&read!=EntityRead::Water)
          return Recipe(root,patch,slot,depth);
        if(slot.size()<=sizeof(value) && source_.BasicConversion(read,root,address,patch.value_mask,value,diagnostic_)){
          if(slot.size()<sizeof(value) && (value>>(slot.size()*8)))break;
          std::memcpy(slot.data(),&value,slot.size());
          return true;
        }
        break;
      }
      case FlatPatchKind::Reference:
      case FlatPatchKind::Resource:
        if(patch.source!=FieldSource::Object||read==EntityRead::QueueItem||read==EntityRead::RockAnimation)
          return Recipe(root,patch,slot,depth);
        if(Link(root,layout.fields[patch.field],patch,read,address,owner,slot,layer))return true;
        break;
      case FlatPatchKind::Container:
        if(read!=EntityRead::String&&read!=EntityRead::Layers&&read!=EntityRead::LayerFrames&&read!=EntityRead::NullFrames)
          return Recipe(root,patch,slot,depth);
        if(patch.header==FlatHeader::CString)return Recipe(root,patch,slot,depth);
        if(Container(root,patch,read,address,owner,slot,depth))return true;
        break;
      case FlatPatchKind::Owner:
        if(read!=EntityRead::Plus)return Recipe(root,patch,slot,depth);
        if(Owner(root,patch,slot,depth))return true;
        break;
      default:break;
    }
    return Fail(CaptureFailure::InvalidValue,root,patch.field);
  }

  bool Loaded(std::uint64_t owner,bool &loaded) noexcept {
    std::uint8_t raw=0;
    if(!source_.Tracked(owner+0x109,raw,diagnostic_,{std::uint32_t(owner),NativeEvidenceUse::Loaded}) || raw>1)return false;
    loaded=raw!=0;
    return true;
  }

  // 主动画和叠加动画共享这个来源检查，各自的选择器数量取自实际选择的 AnimationData。
  bool SpriteAnimation(std::uint64_t state,std::uint32_t &sprite,std::uint32_t &animation,
                       std::span<const std::byte> seeded={}) noexcept {
    bool loaded=false;
    if(!source_.Tracked(state,sprite,diagnostic_) || !sprite || !Loaded(sprite,loaded))return false;
    if(!loaded)return true;
    if(seeded.empty())return source_.Tracked(state+4,animation,diagnostic_);
    animation=FlatWord(seeded);
    return source_.Remember(state+4,seeded.first(4),diagnostic_);
  }

  bool Link(const CaptureRoot &root,const FlatField &field,const FlatPatch &patch,EntityRead read,
            std::uint64_t address,std::uint64_t owner,std::span<std::byte> slot,const FlatLayerResources &layer) noexcept {
    CaptureLink link{CapturedReferenceKind::Null,0};
    if(field.components!=1 || slot.size()<4)return Fail(CaptureFailure::MissingHandler,root,patch.field);
    if(patch.kind==FlatPatchKind::Reference){
      std::uint32_t pointer=0;
      if(patch.seeded)pointer=FlatWord(slot);
      else if(!source_.Value(address,pointer,diagnostic_))return false;
      if(pointer)link=source_.ReadReference(pointer);
    }else{
      switch(read){
        case EntityRead::File:
          if(const auto *prepared=source_.PlayerSpriteAt(std::uint32_t(address)))link=prepared->file;
          else if(!source_.FileResource(std::uint32_t(address),true,link,diagnostic_))return false;
          break;
        case EntityRead::ImagePath:
          if(!source_.LayerImagePath(layer.memo,std::uint32_t(address),
               patch.seeded?std::span<const std::byte>(slot):std::span<const std::byte>{},link,diagnostic_))return false;
          break;
        case EntityRead::Image:{
          std::array<std::uint32_t,2> pair{};
          auto input=std::as_writable_bytes(std::span(pair));
          if(patch.seeded){
            std::memcpy(input.data(),slot.data(),input.size());
            const auto evidence=layer.prepared_owner?NativeEvidenceSource{layer.prepared_owner,NativeEvidenceUse::Image}:NativeEvidenceSource{};
            if(!source_.Remember(address,input,diagnostic_,evidence))return false;
          }else if(!source_.Track(address,input,diagnostic_))return false;
          if(!layer.prepared_image.empty()){
            if(pair[0]&&!pair[1])return false;
            link.kind=CapturedReferenceKind(std::to_integer<unsigned>(layer.prepared_image.front()));
            std::memcpy(&link.handle,layer.prepared_image.data()+1,sizeof(link.handle));
          }else if(!source_.LayerImage(layer.memo,pair,link,diagnostic_))return false;
          break;
        }
        case EntityRead::Animation:{
          std::uint32_t sprite=0,animation=0;
          const auto seeded=patch.seeded?std::span<const std::byte>(slot):std::span<const std::byte>{};
          if(!SpriteAnimation(owner,sprite,animation,seeded))return false;
          const auto *prepared=source_.PlayerSpriteAt(sprite);
          if(!source_.AnimationResource(sprite,animation,link,diagnostic_,prepared?&prepared->file:nullptr))return false;
          break;
        }
        case EntityRead::Config:
        case EntityRead::Item:{
          std::uint32_t pointer=0;
          if(patch.seeded)pointer=FlatWord(slot);
          else if(!source_.Value(address,pointer,diagnostic_))return false;
          if(read==EntityRead::Config){
            if(!source_.ConfigResource(pointer,link,diagnostic_))return false;
          }else if(!source_.ItemResource(pointer,link,diagnostic_))return false;
          break;
        }
        default:return Fail(CaptureFailure::MissingHandler,root,patch.field);
      }
      if(link.kind==CapturedReferenceKind::Identity &&
         (!link.handle || link.handle>source_.resource_used || source_.resources[std::size_t(link.handle-1)].table!=patch.resource_table))
        return Fail(CaptureFailure::ResourcePending,root,patch.field);
    }
    const auto failure=patch.kind==FlatPatchKind::Resource?CaptureFailure::ResourcePending:CaptureFailure::ReferencePending;
    if(link.kind==CapturedReferenceKind::Null){
      if(link.handle || field.value_rule==FlatValueRule::NonNull)return Fail(failure,root,patch.field);
    }else if((link.kind!=CapturedReferenceKind::Identity &&
              (patch.kind!=FlatPatchKind::Reference || link.kind!=CapturedReferenceKind::Pending)) ||
             !link.handle || link.handle>=kNullIndex)return Fail(failure,root,patch.field);
    std::fill(slot.begin(),slot.end(),std::byte{});
    if(patch.kind==FlatPatchKind::Reference)writer_.NoteFlatReference(link.handle);
    SetFlatWord(slot,link.kind==CapturedReferenceKind::Null?kNullIndex:std::uint32_t(link.handle));
    return true;
  }

  // 复杂来源复用具名 Open／Element 的原生窗口证言。输出直接落入镜像和连续附加区。
  bool Recipe(const CaptureRoot &root,const FlatPatch &patch,std::span<std::byte> slot,unsigned depth) noexcept {
    const auto *definition=FindField(patch.owner_layout,patch.recipe_field);
    const auto *native=FindLayout(patch.owner_layout);
    const auto *format=source_.plan->Format(patch.owner_layout,patch.recipe_field);
    if(!definition||!native||!format||patch.owner_offset>=root.extent||
       native->native_size>root.extent-patch.owner_offset)
      return Fail(CaptureFailure::MissingHandler,root,patch.field);
    const CaptureRoot owner{root.subject,root.address+patch.owner_offset,native->native_size,native->id};
    CaptureFieldAccess access{source_.window,owner,*definition,*format,
      {std::span(path_).first(depth),native->id,definition->index,format->canonical}};
    CaptureTraversalSource traversal;
    if(patch.kind==FlatPatchKind::Container||patch.kind==FlatPatchKind::Owner){
      if(!source_.Open(access,traversal,diagnostic_))return false;
      access.traversal=&traversal;
      if(patch.header==FlatHeader::Weapon||patch.header==FlatHeader::Owner||patch.child_layout)
        return RecipeChildren(root,patch,slot,depth,access,traversal)&&source_.Close(access,traversal,diagnostic_);
    }
    struct Sink {
      FlatNativeCapture &capture;
      const CaptureRoot &root;
      const FlatPatch &patch;
      std::span<std::byte> slot;
      const CaptureTraversalSource *traversal;
      std::uint32_t Count()const noexcept {return traversal?traversal->count:0;}
      bool Write(std::span<const std::byte> bytes) noexcept {
        if(!traversal){
          if(slot.size()!=bytes.size())return false;
          std::copy(bytes.begin(),bytes.end(),slot.begin());
          return true;
        }
        if(Count()>patch.limit||bytes.size()>UINT32_MAX||(patch.max_bytes&&bytes.size()>patch.max_bytes))return false;
        const auto start=capture.used_;
        if(!capture.Reserve(bytes.size(),root,patch.field))return false;
        std::copy(bytes.begin(),bytes.end(),capture.bytes_.begin()+start);
        SetFlatContainer(slot,patch.header,{Count(),std::uint32_t(bytes.size())});
        return true;
      }
    } sink{*this,root,patch,slot,access.traversal};
    const EntityLeafSink writer{nullptr,&sink,[](void *p,ScalarKind,std::uint32_t,std::span<const std::byte> bytes) noexcept {
      return static_cast<Sink *>(p)->Write(bytes);
    },[](void *p,bool resource,std::uint8_t table,std::span<const CaptureLink> links) noexcept {
      auto &x=*static_cast<Sink *>(p);
      auto output=x.slot;
      if(x.traversal){
        if(links.size()!=x.Count()||x.Count()>x.patch.limit)return false;
        const auto start=x.capture.used_;
        const auto bytes=std::uint64_t(links.size())*4;
        if((x.patch.max_bytes&&bytes>x.patch.max_bytes)||!x.capture.Reserve(bytes,x.root,x.patch.field))return false;
        output=x.capture.bytes_.subspan(start,std::size_t(bytes));
        SetFlatContainer(x.slot,x.patch.header,{x.Count(),std::uint32_t(bytes)});
      }
      if(links.size()>output.size()/4)return false;
      std::fill(output.begin(),output.end(),std::byte{});
      for(std::size_t i=0;i<links.size();++i){
        const auto link=links[i];
        if(link.kind==CapturedReferenceKind::Null){if(link.handle)return false;}
        else if((link.kind!=CapturedReferenceKind::Identity&&(!(!resource&&link.kind==CapturedReferenceKind::Pending)))||
                !link.handle||link.handle>=kNullIndex||
                (resource&&(link.handle>x.capture.source_.resource_used||
                  x.capture.source_.resources[std::size_t(link.handle-1)].table!=table)))return false;
        if(!resource)x.capture.writer_.NoteFlatReference(link.handle);
        SetFlatWord(output,link.kind==CapturedReferenceKind::Null?kNullIndex:std::uint32_t(link.handle),i*4);
      }
      return true;
    }};
    if(!source_.Leaf(access,writer,diagnostic_))return Fail(CaptureFailure::InvalidValue,root,patch.field);
    return !access.traversal||source_.Close(access,traversal,diagnostic_);
  }

  bool RecipeChildren(const CaptureRoot &root,const FlatPatch &patch,std::span<std::byte> slot,unsigned depth,
                      const CaptureFieldAccess &access,const CaptureTraversalSource &traversal) noexcept {
    if(depth+1>=kFlatMaxDepth)return Fail(CaptureFailure::TraversalBudget,root,patch.field);
    const auto start=used_;
    const bool owner=patch.header==FlatHeader::Owner,weapon=patch.header==FlatHeader::Weapon;
    std::fill(slot.begin(),slot.end(),std::byte{});
    if(weapon){
      std::array<CaptureRoot,5> children{};
      std::array<std::uint32_t,5> offsets{};
      if(slot.size()!=children.size()*4||traversal.slot_mask>>children.size())return false;
      for(std::uint16_t i=0;i<children.size();++i)if(traversal.slot_mask&(1u<<i)){
        auto &child=children[i];
        if(!source_.Element(access,traversal,i,child,diagnostic_))return false;
        const auto *layout=FindFlatLayout(child.layout);
        if(!layout||std::find(patch.choices.begin(),patch.choices.end(),child.layout)==patch.choices.end())return false;
        offsets[i]=used_;
        if(!Reserve(layout->mirror_size,root,patch.field))return false;
        SetFlatWord(slot,kFlatOwnerPresent|child.layout,i*4);
      }
      for(std::uint16_t i=0;i<children.size();++i)if(const auto &child=children[i];child.layout){
        path_[depth]={access.root.layout,access.format.canonical,i,child.layout};
        if(!Instance(child,bytes_.subspan(offsets[i],FindFlatLayout(child.layout)->mirror_size),depth+1))return false;
      }
      return true;
    }
    const auto count=owner?unsigned(traversal.count!=0):traversal.count;
    if(owner&&!count)return true;
    const auto *layout=FindFlatLayout(patch.child_layout);
    if(!layout||(!owner&&count>patch.limit)||count>=kNullIndex||
       !Reserve(std::uint64_t(count)*layout->mirror_size,root,patch.field))return false;
    for(std::uint16_t i=0;i<count;++i){
      CaptureRoot child;
      const auto index=owner?kNullIndex:i;
      if(!source_.Element(access,traversal,index,child,diagnostic_)||child.layout!=layout->id)return false;
      path_[depth]={access.root.layout,access.format.canonical,index,0};
      if(!Instance(child,bytes_.subspan(start+std::size_t(i)*layout->mirror_size,layout->mirror_size),depth+1))return false;
    }
    const auto size=used_-start;
    if(patch.max_bytes&&size>patch.max_bytes)return false;
    if(owner)SetFlatWord(slot,kFlatOwnerPresent|layout->id);
    else SetFlatContainer(slot,patch.header,{count,size});
    return true;
  }

  bool String(const CaptureRoot &root,const FlatPatch &patch,std::uint64_t address,std::span<std::byte> slot) noexcept {
    std::array<std::byte,24> header;
    if(patch.seeded)std::memcpy(header.data(),slot.data(),header.size());
    else if(!source_.window.Copy(root,patch.native_offset,header,diagnostic_))return false;
    const auto size=FlatWord(header,16),capacity=FlatWord(header,20);
    if(size>patch.limit || size>capacity || capacity<15 || capacity==UINT32_MAX)
      return Fail(CaptureFailure::InvalidValue,root,patch.field);
    const auto data=capacity>=16?FlatWord(header):std::uint32_t(address);
    if(!data || !source_.Remember(address+16,std::span(header).subspan(16),diagnostic_) ||
       (capacity>=16 && !source_.Remember(address,std::span(header).first(4),diagnostic_)))return false;
    std::fill(slot.begin(),slot.end(),std::byte{});
    SetFlatWord(slot,size,16);
    if(!size)return true;
    if(size<=15){
      if(capacity==15)std::copy_n(header.begin(),size,slot.begin());
      else if(!source_.Read(data,slot.first(size),diagnostic_))return false;
      return true;
    }
    const auto start=used_;
    return Reserve(size,root,patch.field) && source_.Read(data,bytes_.subspan(start,size),diagnostic_);
  }

  bool Container(const CaptureRoot &root,const FlatPatch &patch,EntityRead read,std::uint64_t address,
                 std::uint64_t owner,std::span<std::byte> slot,unsigned depth) noexcept {
    if(patch.header==FlatHeader::String&&read==EntityRead::String){
      if(std::string_view(patch.host)=="read_anm2_default_name"){
        bool loaded=false;
        if(!Loaded(owner,loaded))return false;
        if(!loaded){std::fill(slot.begin(),slot.end(),std::byte{});return true;}
      }
      return String(root,patch,address,slot);
    }
    std::uint32_t count=0,data=0;
    switch(read){
      case EntityRead::Layers:{
        bool loaded=false;
        if(!Loaded(owner,loaded))return false;
        if(loaded){
          std::array<std::uint32_t,2> native{};
          auto header=std::as_writable_bytes(std::span(native));
          if(patch.seeded){
            std::memcpy(header.data(),slot.data(),header.size());
            if(!source_.Remember(address,header,diagnostic_))return false;
          }else if(!source_.Track(address,header,diagnostic_))return false;
          data=native[0];
          count=data?native[1]:0u;
        }
        break;
      }
      case EntityRead::LayerFrames:
      case EntityRead::NullFrames:{
        std::uint32_t sprite=0,animation=0;
        if(!SpriteAnimation(owner,sprite,animation))return false;
        if(animation && !source_.Tracked(std::uint64_t(animation)+(read==EntityRead::LayerFrames?0x1cu:0x24u),count,diagnostic_))
          return false;
        if(count && !source_.Tracked(address,data,diagnostic_))return false;
        break;
      }
      default:return Fail(CaptureFailure::MissingHandler,root,patch.field);
    }
    if(count>patch.limit || (count && !data) || !patch.stride ||
       std::uint64_t(data)+std::uint64_t(count)*patch.stride>kAddressEnd)
      return Fail(CaptureFailure::CountMismatch,root,patch.field);
    const auto start=used_;
    if(count){
      if(patch.child_layout){
        const auto sprite=read==EntityRead::Layers?std::uint32_t(owner):0;
        if(!Children(root,patch,data,count,patch.stride,depth,sprite))return false;
      }else{
        const auto size=std::uint64_t(count)*patch.stride;
        if(!Reserve(size,root,patch.field) || !source_.Read(data,bytes_.subspan(start,std::size_t(size)),diagnostic_))return false;
      }
    }
    const auto tail=used_-start;
    if(patch.max_bytes && tail>patch.max_bytes)return Fail(CaptureFailure::ValueBudget,root,patch.field);
    std::fill(slot.begin(),slot.end(),std::byte{});
    const auto layer_array=patch.header==FlatHeader::LayerArray;
    SetFlatWord(slot,count,layer_array?4:0);
    SetFlatWord(slot,tail,layer_array?0:4);
    return true;
  }

  bool Owner(const CaptureRoot &root,const FlatPatch &patch,std::span<std::byte> slot,unsigned depth) noexcept {
    std::uint32_t holder=0,payload=0;
    if(!source_.ExistingPlus(root,holder,payload,diagnostic_))return false;
    if(!Children(root,patch,payload,1,0,depth))return false;
    SetFlatWord(slot,kFlatOwnerPresent|patch.child_layout);
    return true;
  }

  bool Children(const CaptureRoot &root,const FlatPatch &patch,std::uint32_t data,std::uint32_t count,
                std::uint32_t stride,unsigned depth,std::uint32_t sprite=0) noexcept {
    const auto *child=FindFlatLayout(patch.child_layout);
    const auto *native=FindLayout(patch.child_layout);
    if(!child || !native)return Fail(CaptureFailure::UnknownLayout,root,patch.field);
    if((count>1 && stride<native->native_size) || std::uint64_t(data)+std::uint64_t(count-1)*stride+native->native_size>kAddressEnd)
      return Fail(CaptureFailure::AddressRange,root,patch.field);
    const auto start=used_;
    if(!Reserve(std::uint64_t(count)*child->mirror_size,root,patch.field))return false;
    const auto *format=source_.plan->Format(patch.owner_layout,patch.recipe_field);
    if(!format||depth+1>=kFlatMaxDepth)return false;
    const auto *memo=sprite?source_.SpriteMemo(sprite):nullptr;
    const auto *prepared=sprite?source_.PlayerSpriteAt(sprite):nullptr;
    const bool reuse_images=prepared&&prepared->layer_data==data&&prepared->layer_count==count;
    for(std::uint32_t i=0;i<count;++i){
      const CaptureRoot element{root.subject,data+i*stride,native->native_size,child->id};
      path_[depth]={patch.owner_layout,format->canonical,patch.kind==FlatPatchKind::Owner?kNullIndex:std::uint16_t(i),0};
      FlatLayerResources layer{source_.LayerMemo(memo,data,count,i),
        reuse_images?prepared->images.subspan(std::size_t(i)*9,9):std::span<const std::byte>{},reuse_images?sprite:0u};
      if(!Instance(element,bytes_.subspan(start+std::size_t(i)*child->mirror_size,child->mirror_size),depth+1,layer))return false;
    }
    return true;
  }

  EntitySource::Impl &source_;
  std::span<std::byte> bytes_;
  CaptureValueWriter &writer_;
  CaptureDiagnostic &diagnostic_;
  std::array<CodecPathStep,kFlatMaxDepth> path_{};
  unsigned stream_;
  std::uint32_t used_=0;
};
}

std::uint64_t EntitySource::Impl::BindFlatPrograms() {
  std::vector<std::uint16_t> pending;
  for(const auto &layout:Layouts())if(layout.flat_record&&FindFlatLayout(layout.id))pending.push_back(layout.id);
  for(std::size_t i=0;i<pending.size();++i){
    const auto *layout=FindFlatLayout(pending[i]);
    if(!layout)throw std::logic_error("flat_source_layout");
    auto &program=flat_programs.emplace_back();
    program.layout=layout;
    program.reads.reserve(layout->patches.size());
    // 生成的复制段与有名义原生地址的补丁共同界定快照范围，变体臂各自使用自己的表。
    const auto include=[&](std::uint32_t offset,std::uint32_t size){
      if(offset==kDynamicOffset||!size)return;
      program.native_begin=program.native_end?std::min(program.native_begin,offset):offset;
      const auto end=std::min(std::uint64_t(offset)+size,std::uint64_t(UINT32_MAX));
      program.native_end=std::max(program.native_end,std::uint32_t(end));
    };
    for(const auto &copy:layout->copies)include(copy.native_offset,copy.size);
    for(const auto &patch:layout->patches){
      auto read=FindEntityRead(patch.host);
      if(read==EntityRead::None)read=GridRecipe(patch.host);
      program.reads.push_back(read);
      if(patch.kind!=FlatPatchKind::Zero)include(patch.native_offset,patch.native_size);
      program.has_conditions|=!patch.conditions.empty();
      if(patch.child_layout && std::find(pending.begin(),pending.end(),patch.child_layout)==pending.end())
        pending.push_back(patch.child_layout);
      for(auto choice:patch.choices)if(std::find(pending.begin(),pending.end(),choice)==pending.end())
        pending.push_back(choice);
    }
  }
  std::sort(flat_programs.begin(),flat_programs.end(),[](const auto &a,const auto &b){return a.layout->id<b.layout->id;});
  auto bytes=std::uint64_t(flat_programs.capacity())*sizeof(FlatSourceProgram);
  for(const auto &program:flat_programs)bytes+=program.reads.capacity()*sizeof(EntityRead);
  return bytes;
}

const FlatSourceProgram *EntitySource::Impl::FlatProgram(std::uint16_t layout) const noexcept {
  const auto found=std::lower_bound(flat_programs.begin(),flat_programs.end(),layout,
    [](const auto &program,std::uint16_t id){return program.layout->id<id;});
  return found!=flat_programs.end()&&found->layout->id==layout?&*found:nullptr;
}

// 整段复制失败只放弃快照。随后的逐段读取给出具体的失败原因和字段。
void EntitySource::Impl::SnapshotRoot(const CaptureRoot &root) noexcept {
  const auto *program=FlatProgram(root.layout);
  if(!program||!program->native_end)return;
  const auto size=program->native_end-program->native_begin;
  if(program->native_end<=root.extent&&size<=snapshot.storage.size()){
    const auto base=root.address+program->native_begin;
    const auto bytes=std::span(snapshot.storage).first(size);
    if(services.reads->Probe(base,bytes)){
      snapshot.base=base;
      snapshot.bytes=bytes;
      ++stats.snapshot_reads;
      return;
    }
  }
  ++stats.snapshot_fallbacks;
}

// 根尾复核通过前，镜像和新资源都只属于当前采集事务。
bool EntitySource::Impl::ExecuteFlat(const CaptureRoot &root,BodyStream stream,bool pair,CaptureValueWriter &writer,
                                    CaptureDiagnostic &diagnostic) noexcept {
  const auto *layout=FindFlatLayout(root.layout);
  const auto last=pair?1u:unsigned(stream);
  for(unsigned flow=unsigned(stream);flow<=last;++flow){
    if(!layout||!FlatHasStream(*layout,flow)){
      FailCapture(diagnostic,CaptureFailure::UnknownLayout,root);
      break;
    }
    const auto limit=window.witness.phase==CapturePhase::PlayerPost?65536u:kFlatRootLimit;
    const auto bytes=writer.BeginFlatRoot(root.subject,root.layout,BodyStream(flow),limit);
    if(bytes.empty())break;
    FlatNativeCapture capture(*this,bytes,writer,diagnostic,BodyStream(flow));
    // 联合采集的 World 暂留在 ordinary 对象回卷点内，Body 完成后才统一复核。
    if(!capture.Capture(root)||(flow==last&&!Verify(root,diagnostic))||!writer.FinishFlatRoot(capture.Used())){
      writer.AbortRoot();
      break;
    }
    if(flow==last)return true;
  }
  if(diagnostic.reason==CaptureFailure::None)diagnostic=writer.Diagnostic();
  if(diagnostic.reason==CaptureFailure::None)FailCapture(diagnostic,CaptureFailure::InvalidValue,root);
  return writer.Fail(diagnostic);
}
}
