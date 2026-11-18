#include "state/flat_capture.h"

namespace cs::state {
namespace {
// 游标只在后台私有镜像中行进，依次消费该容器的全部元素主区和每个元素的附加区。
class FlatTranslation {
public:
  FlatTranslation(CaptureSubject subject,CaptureLinkResolver resolver,CaptureDiagnostic &diagnostic) noexcept
    :root_{subject},resolver_(resolver),diagnostic_(diagnostic) {}

  bool Translate(const FlatLayout &layout,std::span<std::byte> bytes) noexcept {
    if(bytes.size()<layout.mirror_size)return Fail(layout.id,kNullIndex,CaptureFailure::InvalidValue);
    auto rest=bytes.subspan(layout.mirror_size);
    if(!Instance(layout,bytes.first(layout.mirror_size),rest,0))return false;
    return rest.empty() || Fail(layout.id,kNullIndex,CaptureFailure::CountMismatch);
  }

private:
  bool Fail(std::uint16_t layout,std::uint16_t field,CaptureFailure reason) noexcept {
    root_.layout=layout;
    return FailCapture(diagnostic_,reason,root_,field);
  }

  bool Instance(const FlatLayout &layout,std::span<std::byte> main,std::span<std::byte> &rest,unsigned depth) noexcept {
    if(depth>=kFlatMaxDepth)return Fail(layout.id,kNullIndex,CaptureFailure::TraversalBudget);
    for(const auto &patch:layout.patches){
      auto slot=main.subspan(patch.mirror_offset,patch.size);
      const auto resource=patch.kind==FlatPatchKind::Resource;
      if(resource || patch.kind==FlatPatchKind::Reference){
        const auto &field=layout.fields[patch.field];
        if(!Links(patch,field,slot,resource))
          return Fail(layout.id,patch.field,resource?CaptureFailure::ResourcePending:CaptureFailure::ReferencePending);
      }else if(patch.kind==FlatPatchKind::Container){
        if(!Container(patch,slot,rest,depth))return Fail(layout.id,patch.field,CaptureFailure::CountMismatch);
      }else if(patch.kind==FlatPatchKind::Owner){
        const auto word=FlatWord(slot);
        if(patch.header!=FlatHeader::Owner ||
           (word && (word!=(kFlatOwnerPresent|patch.child_layout) || !Children(patch.child_layout,1,rest,depth))))
          return Fail(layout.id,patch.field,CaptureFailure::UnknownLayout);
      }
    }
    return true;
  }

  bool Links(const FlatPatch &patch,const FlatField &field,std::span<std::byte> slot,bool resource) noexcept {
    for(unsigned i=0;i<field.components;++i){
      const auto word=FlatWord(slot,i*4);
      if(!word && !patch.conditions.empty())continue; // 条件关闭的槽采用全零表示。
      if(!word || word>kNullIndex)return false;
      std::uint16_t index=kNullIndex;
      if(word!=kNullIndex &&
         (!resolver_.resolve || !resolver_.resolve(resolver_.context,root_.subject,resource,patch.resource_table,
           {CapturedReferenceKind::Identity,word},index)))return false;
      if(index==kNullIndex && (field.value_rule==FlatValueRule::NonNull ||
         (word!=kNullIndex && (resource || !resolver_.allow_retained_null))))return false;
      SetFlatWord(slot,index,i*4);
    }
    return true;
  }

  bool Container(const FlatPatch &patch,std::span<const std::byte> slot,std::span<std::byte> &rest,unsigned depth) noexcept {
    std::uint32_t count=0,size=0;
    switch(patch.header){
      case FlatHeader::String:
        count=FlatWord(slot,16);
        size=count>15?count:0;
        break;
      case FlatHeader::LayerArray:
        count=FlatWord(slot,4);
        size=FlatWord(slot);
        break;
      case FlatHeader::Array:
      case FlatHeader::Vector:
        count=FlatWord(slot);
        size=FlatWord(slot,4);
        break;
      default:return false;
    }
    if(count>patch.limit || size>rest.size() || (patch.max_bytes && size>patch.max_bytes))return false;
    auto tail=rest.first(size);
    rest=rest.subspan(size);
    if(patch.child_layout)return Children(patch.child_layout,count,tail,depth) && tail.empty();
    return patch.header==FlatHeader::String || std::uint64_t(count)*patch.stride==size;
  }

  bool Children(std::uint16_t layout,std::uint32_t count,std::span<std::byte> &tail,unsigned depth) noexcept {
    const auto *child=FindFlatLayout(layout);
    if(!child || std::uint64_t(count)*child->mirror_size>tail.size())return false;
    const auto mirrors=tail.first(std::size_t(count)*child->mirror_size);
    tail=tail.subspan(mirrors.size());
    for(std::uint32_t i=0;i<count;++i){
      if(!Instance(*child,mirrors.subspan(std::size_t(i)*child->mirror_size,child->mirror_size),tail,depth+1))return false;
    }
    return true;
  }

  CaptureRoot root_;
  CaptureLinkResolver resolver_;
  CaptureDiagnostic &diagnostic_;
};
}

bool TranslateFlatCapture(const CapturePacket &packet,const CaptureRecord &record,CaptureLinkResolver resolver,
    std::span<std::byte> output,CaptureDiagnostic &diagnostic) noexcept {
  const auto *layout=FindFlatLayout(record.layout);
  if(!layout || record.retained || !record.flat_bytes || record.flat_offset>packet.bytes.size() ||
     record.flat_bytes>packet.bytes.size()-record.flat_offset || record.flat_bytes>output.size())
    return FailCapture(diagnostic,CaptureFailure::InvalidTransaction,{record.subject,0,0,record.layout});
  const auto image=output.first(record.flat_bytes);
  std::memmove(image.data(),packet.bytes.data()+record.flat_offset,image.size());
  return FlatTranslation(record.subject,resolver,diagnostic).Translate(*layout,image);
}
}
