#include "world/binary/entity_source_internal.h"
#include "state/flat_capture.h"
#include "state_host_fields.inc"
#include <stdexcept>

namespace cs::world::binary {
namespace {
constexpr std::uint32_t kFlatRootLimit=2u<<20;
constexpr std::uint64_t kAddressEnd=std::uint64_t{1}<<32;

bool Qualified(std::string_view requirement,bool ready) noexcept {
  return requirement.empty() || (requirement=="entity-current-window" && ready);
}

// 复用同一块预分配字节区。每个容器先为所有元素留出主区，再按顺序追加递归内容。
class FlatNativeCapture {
public:
  FlatNativeCapture(EntitySource::Impl &source,std::span<std::byte> bytes,CaptureDiagnostic &diagnostic) noexcept
    :source_(source),bytes_(bytes),diagnostic_(diagnostic) {}

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

  bool Instance(const CaptureRoot &root,std::span<std::byte> mirror,unsigned depth) noexcept {
    const auto binding=std::find_if(source_.flat_programs.begin(),source_.flat_programs.end(),
      [&](const auto &program){return program.layout->id==root.layout;});
    if(binding==source_.flat_programs.end() || !binding->layout->scopes.empty())
      return Fail(CaptureFailure::UnknownLayout,root,kNullIndex);
    if(depth>=kFlatMaxDepth)return Fail(CaptureFailure::TraversalBudget,root,kNullIndex);
    const auto &layout=*binding->layout;
    source_.evidence_layout=root.layout;
    source_.evidence_field=kNullIndex;
    for(const auto &copy:layout.copies){
      if(!Qualified(copy.read_requirement,source_.facts.ready))return Fail(CaptureFailure::MissingQualification,root,kNullIndex);
      if(!source_.window.Copy(root,copy.native_offset,mirror.subspan(copy.mirror_offset,copy.size),diagnostic_))return false;
    }
    // 先补齐条件引用的定长值。其余补丁按表序执行，容器的附加区顺序不会被条件改变。
    if(binding->has_conditions){
      for(std::size_t i=0;i<layout.patches.size();++i){
        const auto &patch=layout.patches[i];
        if(!EarlyScalar(patch))continue;
        if(!Patch(root,layout,patch,binding->reads[i],mirror,depth))return false;
      }
    }
    for(std::size_t i=0;i<layout.patches.size();++i){
      const auto &patch=layout.patches[i];
      if(binding->has_conditions && EarlyScalar(patch))continue;
      if(!Patch(root,layout,patch,binding->reads[i],mirror,depth))return false;
    }
    return true;
  }

  static bool EarlyScalar(const FlatPatch &patch) noexcept {
    return patch.conditions.empty() && (patch.kind==FlatPatchKind::Copy || patch.kind==FlatPatchKind::Convert);
  }

  bool Conditions(const FlatLayout &layout,const FlatPatch &patch,std::span<const std::byte> mirror,
                  bool &selected,const CaptureRoot &root) noexcept {
    selected=true;
    for(const auto &condition:patch.conditions){
      const auto &field=layout.fields[condition.selector];
      // Tear 仅使用标量选择器。配置驱动的条件在迁移其布局时接入相应资源上下文。
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
             std::span<std::byte> mirror,unsigned depth) noexcept {
    source_.evidence_layout=root.layout;
    source_.evidence_field=patch.field;
    if(!Qualified(patch.read_requirement,source_.facts.ready))
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
        if(slot.size()<=sizeof(value) && source_.BasicConversion(read,root,address,patch.value_mask,value,diagnostic_)){
          if(slot.size()<sizeof(value) && (value>>(slot.size()*8)))break;
          std::memcpy(slot.data(),&value,slot.size());
          return true;
        }
        break;
      }
      case FlatPatchKind::Reference:
      case FlatPatchKind::Resource:
        if(Link(root,layout.fields[patch.field],patch,read,address,owner,slot))return true;
        break;
      case FlatPatchKind::Container:
        if(Container(root,patch,read,address,owner,slot,depth))return true;
        break;
      case FlatPatchKind::Owner:
        if(read==EntityRead::Plus && Owner(root,patch,slot,depth))return true;
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
            std::uint64_t address,std::uint64_t owner,std::span<std::byte> slot) noexcept {
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
          if(!source_.FileResource(std::uint32_t(address),true,link,diagnostic_))return false;
          break;
        case EntityRead::ImagePath:
          if(!source_.ImagePathResource(std::uint32_t(address),link,diagnostic_))return false;
          break;
        case EntityRead::Image:{
          std::array<std::uint32_t,2> pair{};
          auto input=std::as_writable_bytes(std::span(pair));
          if(patch.seeded){
            std::memcpy(input.data(),slot.data(),input.size());
            if(!source_.Remember(address,input,diagnostic_))return false;
          }else if(!source_.Track(address,input,diagnostic_))return false;
          if((pair[0] && !pair[1]) || !source_.ImageResource(pair[0],link,diagnostic_))return false;
          break;
        }
        case EntityRead::Animation:{
          std::uint32_t sprite=0,animation=0;
          const auto seeded=patch.seeded?std::span<const std::byte>(slot):std::span<const std::byte>{};
          if(!SpriteAnimation(owner,sprite,animation,seeded) || !source_.AnimationResource(sprite,animation,link,diagnostic_))return false;
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
    SetFlatWord(slot,link.kind==CapturedReferenceKind::Null?kNullIndex:std::uint32_t(link.handle));
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
    if(patch.header==FlatHeader::String && read==EntityRead::String)return String(root,patch,address,slot);
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
        if(!Children(root,patch,data,count,patch.stride,depth))return false;
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
                std::uint32_t stride,unsigned depth) noexcept {
    const auto *child=FindFlatLayout(patch.child_layout);
    const auto *native=FindLayout(patch.child_layout);
    if(!child || !native)return Fail(CaptureFailure::UnknownLayout,root,patch.field);
    if((count>1 && stride<native->native_size) || std::uint64_t(data)+std::uint64_t(count-1)*stride+native->native_size>kAddressEnd)
      return Fail(CaptureFailure::AddressRange,root,patch.field);
    const auto start=used_;
    if(!Reserve(std::uint64_t(count)*child->mirror_size,root,patch.field))return false;
    for(std::uint32_t i=0;i<count;++i){
      const CaptureRoot element{root.subject,data+i*stride,native->native_size,child->id};
      if(!Instance(element,bytes_.subspan(start+std::size_t(i)*child->mirror_size,child->mirror_size),depth+1))return false;
    }
    return true;
  }

  EntitySource::Impl &source_;
  std::span<std::byte> bytes_;
  CaptureDiagnostic &diagnostic_;
  std::uint32_t used_=0;
};
}

std::uint64_t EntitySource::Impl::BindFlatTear() {
  std::vector<std::uint16_t> pending{host_field::EntityTear::kLayout};
  for(std::size_t i=0;i<pending.size();++i){
    const auto *layout=FindFlatLayout(pending[i]);
    if(!layout)throw std::logic_error("flat_tear_layout");
    auto &program=flat_programs.emplace_back();
    program.layout=layout;
    program.reads.reserve(layout->patches.size());
    for(const auto &patch:layout->patches){
      const auto read=FindEntityRead(patch.host);
      if(patch.kind!=FlatPatchKind::Reference && *patch.host && read==EntityRead::None)
        throw std::logic_error("flat_tear_handler");
      program.reads.push_back(read);
      program.has_conditions|=!patch.conditions.empty();
      if(patch.child_layout && std::find(pending.begin(),pending.end(),patch.child_layout)==pending.end())
        pending.push_back(patch.child_layout);
    }
  }
  auto bytes=std::uint64_t(flat_programs.capacity())*sizeof(FlatSourceProgram);
  for(const auto &program:flat_programs)bytes+=program.reads.capacity()*sizeof(EntityRead);
  return bytes;
}

// 根尾复核通过前，镜像和新资源都只属于当前采集事务。
bool EntitySource::Impl::ExecuteFlatTear(const CaptureRoot &root,CaptureValueWriter &writer,CaptureDiagnostic &diagnostic) noexcept {
  if(root.layout!=host_field::EntityTear::kLayout){
    FailCapture(diagnostic,CaptureFailure::UnknownLayout,root);
    return writer.Fail(diagnostic);
  }
  const auto bytes=writer.BeginFlatRoot(root.subject,root.layout,kFlatRootLimit);
  if(bytes.empty()){
    diagnostic=writer.Diagnostic();
    return false;
  }
  FlatNativeCapture capture(*this,bytes,diagnostic);
  if(capture.Capture(root) && Verify(root,diagnostic) && writer.FinishFlatRoot(capture.Used()))return true;
  if(diagnostic.reason==CaptureFailure::None)diagnostic=writer.Diagnostic();
  writer.Fail(diagnostic);
  writer.AbortRoot();
  return false;
}
}
