#include "state/capture.h"
#include "state/capture_values.h"
#include <algorithm>
#include <bit>

namespace cs::state {
bool ValidCaptureWitness(const CaptureWitness &witness) noexcept {
  return witness.generation && witness.revision && witness.batch && witness.outer && witness.owner &&
    witness.scope.session && witness.scope.run && witness.scope.scene &&
    unsigned(witness.phase)<=unsigned(CapturePhase::IntentionBinding) &&
    (witness.phase==CapturePhase::Production || witness.tick);
}
bool FailCapture(CaptureDiagnostic &out,CaptureFailure reason,const CaptureRoot &root,std::uint16_t field,
    std::span<const CodecPathStep> parents) noexcept {
  if(out.reason==CaptureFailure::None){out.reason=reason;out.subject=root.subject;out.layout=root.layout;out.field=field;
    out.depth=std::uint8_t(std::min(parents.size(),out.parents.size()));std::copy_n(parents.begin(),out.depth,out.parents.begin());}
  return false;
}
std::uint32_t CaptureScalarWidth(ScalarKind kind) noexcept {
  switch(kind){
    case ScalarKind::U8:case ScalarKind::I8:return 1;
    case ScalarKind::U16:case ScalarKind::I16:return 2;
    case ScalarKind::U32:case ScalarKind::I32:case ScalarKind::F32:return 4;
    case ScalarKind::U64:case ScalarKind::I64:case ScalarKind::F64:return 8;
    default:return 0;
  }
}
bool CaptureWindow::Check(const CaptureRoot &root,CaptureDiagnostic &out) const noexcept {
  if(out.reason!=CaptureFailure::None)return false;
  if(std::this_thread::get_id()!=thread)return FailCapture(out,CaptureFailure::WrongThread,root);
  if(!ValidCaptureWitness(witness) || !current || !current(context,witness,root))return FailCapture(out,CaptureFailure::InvalidWindow,root);
  return true;
}
bool CaptureWindow::Copy(const CaptureRoot &root,std::uint32_t offset,std::span<std::byte> bytes,CaptureDiagnostic &out) const noexcept {
  if(!Check(root,out))return false;
  const auto address=std::uint64_t(root.address)+offset;
  if(!root.address || offset>root.extent || bytes.size()>root.extent-offset || address>UINT32_MAX ||
     bytes.size()>(std::uint64_t{1}<<32)-address)return FailCapture(out,CaptureFailure::AddressRange,root);
  if(!read || !read(context,std::uint32_t(address),bytes))return FailCapture(out,CaptureFailure::ReadFailed,root);
  return Check(root,out);
}
CapturePlan::CapturePlan(std::span<const CaptureQualificationBinding> qualifications,
    std::span<const CaptureHandlerBinding> handlers,std::span<const CaptureVtableBinding> vtables) {
  static_assert(std::endian::native==std::endian::little,"J273 Win32 copy requires little endian");
  for(std::size_t i=0;i<qualifications.size();++i){
    if(qualifications[i].name.empty() || !qualifications[i].check)throw std::invalid_argument("capture_qualification_binding");
    for(std::size_t j=0;j<i;++j)if(qualifications[j].name==qualifications[i].name)throw std::invalid_argument("duplicate_capture_qualification");
  }
  for(std::size_t i=0;i<handlers.size();++i){
    if(!handlers[i].capture)throw std::invalid_argument("capture_handler_binding");
    for(std::size_t j=0;j<i;++j)if(handlers[j].operation==handlers[i].operation && handlers[j].name==handlers[i].name)
      throw std::invalid_argument("duplicate_capture_handler");
  }
  memory_=memory::Reserve(memory::Pool::Values,sizeof(*this)+128+Fields().size()*sizeof(Binding)+
    vtables.size()*sizeof(CaptureVtableBinding),"state_capture_plan");
  fields_.resize(Fields().size());vtables_.assign(vtables.begin(),vtables.end());
  for(const auto &field:Fields()){
    const auto *layout=FindLayout(field.layout);auto &bound=fields_[layout->first_field+field.index];
    if(!DescribeBodySourceField(field.layout,field.index,bound.format))continue;
    const std::string_view required=field.read_requirement?field.read_requirement:"";
    bound.required=!required.empty();
    for(const auto &binding:qualifications)if(binding.name==required)bound.qualify=binding.check;
    for(const auto &binding:handlers)if(binding.operation==field.operation && binding.name==(field.handler?field.handler:""))bound.handler=binding.capture;
    const auto size=std::uint64_t(CaptureScalarWidth(bound.format.scalar))*bound.format.components;
    bound.direct=field.operation==Operation::Copy && field.source==FieldSource::Object && !layout->protocol &&
      !bound.format.linked && !field.nested_layout &&
      size && size==field.native_size && size==field.min_size && size==field.max_size &&
      field.native_offset<=layout->native_size && size<=layout->native_size-field.native_offset;
  }
  // 只有整个合并区间都是无条件直接值时才批读；先核全部字段资格，再缓存一次复制。
  const auto ranges=CopyRanges();
  for(std::uint32_t r=0;r<ranges.size();++r){const auto &range=ranges[r];const auto *layout=FindLayout(range.layout);
    if(!layout || range.native_offset>layout->native_size || range.size>layout->native_size-range.native_offset)continue;
    std::uint64_t covered=0;bool safe=true;
    for(std::uint32_t i=0;i<layout->field_count;++i){const auto &field=Fields()[layout->first_field+i];
      if(field.block!=range.block || field.native_offset<range.native_offset ||
         std::uint64_t(field.native_offset)+field.native_size>std::uint64_t(range.native_offset)+range.size)continue;
      // 容器／转换字段可能有重叠来源，只有Copy且线上偏移同样相邻才属于该合并区间。
      if(field.operation!=Operation::Copy)continue;
      const auto &bound=fields_[layout->first_field+i];
      if(!bound.direct || bound.format.conditional || bound.format.canonical_zero || field.wire_offset==kDynamicOffset ||
         std::uint64_t(field.wire_offset)!=std::uint64_t(range.wire_offset)+field.native_offset-range.native_offset)safe=false;
      covered+=field.native_size;
    }
    if(!safe || covered!=range.size)continue;
    for(std::uint32_t i=0;i<layout->field_count;++i){const auto &field=Fields()[layout->first_field+i];auto &bound=fields_[layout->first_field+i];
      if(bound.direct && field.block==range.block && field.native_offset>=range.native_offset &&
         std::uint64_t(field.native_offset)+field.native_size<=std::uint64_t(range.native_offset)+range.size &&
         std::uint64_t(field.wire_offset)==std::uint64_t(range.wire_offset)+field.native_offset-range.native_offset)bound.copy=r;
    }
  }
  std::sort(vtables_.begin(),vtables_.end(),[](const auto &a,const auto &b){return a.vtable<b.vtable;});
  for(std::size_t i=0;i<vtables_.size();++i){const auto &binding=vtables_[i];const auto *layout=FindLayout(binding.layout);
    if(!binding.vtable || (binding.vtable&3) || !layout || layout->protocol || !layout->rtti_rva ||
       (i && vtables_[i-1].vtable==binding.vtable))throw std::invalid_argument("capture_vtable_binding");
    for(const auto &variant:Variants())if(variant.arm==binding.layout && variant.arm!=variant.base)
      throw std::invalid_argument("capture_vtable_requires_base_layout");
  }
  memory_->Resize(sizeof(*this)+128+fields_.capacity()*sizeof(Binding)+vtables_.capacity()*sizeof(CaptureVtableBinding));
}
bool CapturePlan::Identify(const CaptureWindow &window,CaptureRoot &root,CaptureDiagnostic &out) const noexcept {
  std::array<std::byte,4> bytes;
  if(!window.Copy(root,0,bytes,out))return false;
  std::uint32_t vtable=0;for(unsigned i=0;i<4;++i)vtable|=std::to_integer<std::uint32_t>(bytes[i])<<(i*8);
  const auto found=std::lower_bound(vtables_.begin(),vtables_.end(),vtable,[](const auto &entry,std::uint32_t value){return entry.vtable<value;});
  if(found==vtables_.end() || found->vtable!=vtable)return FailCapture(out,CaptureFailure::UnknownVtable,root);
  root.layout=found->layout;return SelectArm(window,root,out);
}
bool CapturePlan::SelectArm(const CaptureWindow &window,CaptureRoot &root,CaptureDiagnostic &out) const noexcept {
  if(!window.Check(root,out))return false;
  const auto *layout=FindLayout(root.layout);
  if(!layout || layout->protocol || !root.address || root.extent<layout->native_size)return FailCapture(out,CaptureFailure::UnknownLayout,root);
  const auto base=root.layout;std::uint32_t cached_offset=UINT32_MAX,value=0;
  for(const auto &variant:Variants())if(variant.base==base){
    if(cached_offset!=variant.selector_offset){std::array<std::byte,4> bytes;
      if(!window.Copy(root,variant.selector_offset,bytes,out)){
        // Copy已保留更具体的读取／窗口失败；不把selector失败降级为基础臂。
        return false;
      }
      value=0;for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);
      cached_offset=variant.selector_offset;
    }
    if(value==std::uint32_t(variant.value)){const auto *arm=FindLayout(variant.arm);
      if(!arm || root.extent<arm->native_size)return FailCapture(out,CaptureFailure::UnknownLayout,root);
      root.layout=variant.arm;break;
    }
  }
  return window.Check(root,out);
}
std::uint32_t CapturePlan::Mask(std::uint16_t id,BodyStream stream) const noexcept {
  const auto *layout=FindLayout(id);if(!layout || (stream!=BodyStream::Value && stream!=BodyStream::World && stream!=BodyStream::Body))return 0;
  std::uint32_t mask=0;
  for(std::uint8_t i=0;i<layout->block_count;++i){const auto &block=Blocks()[layout->first_block+i];
    if(block.max_size && (stream==BodyStream::Value || block.body==(stream==BodyStream::Body)))mask|=std::uint32_t{1}<<i;}
  return mask;
}
bool CapturePlan::Dispatch(const CaptureWindow &window,const CaptureRoot &root,std::uint16_t index,
    std::span<const CodecPathStep> parents,CaptureValueWriter &writer,CaptureCopyScratch &scratch,CaptureDiagnostic &out) const noexcept {
  return DispatchSelected(window,root,index,parents,writer,scratch,out,false);
}
const BodySourceFieldInfo *CapturePlan::Format(std::uint16_t id,std::uint16_t index) const noexcept {
  const auto *layout=FindLayout(id);
  return layout && index<layout->field_count?&fields_[layout->first_field+index].format:nullptr;
}
bool CapturePlan::Qualify(const CaptureWindow &window,const CaptureRoot &root,std::uint16_t index,
    std::span<const CodecPathStep> parents,CaptureValueWriter &writer,CaptureDiagnostic &out) const noexcept {
  const auto reject=[&](CaptureFailure reason){FailCapture(out,reason,root,index,parents);
    if(out.field==kNullIndex){out.layout=root.layout;out.field=index;out.depth=std::uint8_t(std::min(parents.size(),out.parents.size()));
      std::copy_n(parents.begin(),out.depth,out.parents.begin());}writer.Fail(out);return false;};
  if(out.reason!=CaptureFailure::None)return reject(out.reason);
  if(writer.Diagnostic().reason!=CaptureFailure::None)return reject(writer.Diagnostic().reason);
  if(window.witness!=writer.Witness() || root.subject!=writer.Root().subject || !window.Check(root,out))return reject(CaptureFailure::InvalidWindow);
  const auto *field=FindField(root.layout,index);const auto *layout=FindLayout(root.layout);
  BodySourceFieldInfo format;
  if(!field || !layout || parents.size()>kCodecMaxDepth || !DescribeBodySourceField(root.layout,index,format))return reject(CaptureFailure::UnknownLayout);
  const auto &bound=fields_[layout->first_field+index];
  const CaptureFieldAccess access{window,root,*field,bound.format,{parents,root.layout,index,bound.format.canonical}};
  if(!writer.Allows(access.location))return reject(CaptureFailure::InvalidTransaction);
  if(bound.required && !bound.qualify)return reject(CaptureFailure::MissingQualification);
  if(bound.qualify && !bound.qualify(access))return reject(CaptureFailure::QualificationFailed);
  return window.Check(root,out) || reject(CaptureFailure::InvalidWindow);
}
bool CapturePlan::DispatchSelected(const CaptureWindow &window,const CaptureRoot &root,std::uint16_t index,
    std::span<const CodecPathStep> parents,CaptureValueWriter &writer,CaptureCopyScratch &scratch,CaptureDiagnostic &out,bool selected,
    const CaptureTraversalSource *source) const noexcept {
  const auto reject=[&](CaptureFailure reason){FailCapture(out,reason,root,index,parents);
    if(out.field==kNullIndex){out.field=index;out.depth=std::uint8_t(std::min(parents.size(),out.parents.size()));std::copy_n(parents.begin(),out.depth,out.parents.begin());}
    writer.Fail(out);return false;};
  if(!Qualify(window,root,index,parents,writer,out))return false;
  const auto *field=FindField(root.layout,index);const auto *layout=FindLayout(root.layout);
  if(!field || !layout || parents.size()>kCodecMaxDepth)return reject(CaptureFailure::UnknownLayout);
  const auto &bound=fields_[layout->first_field+index];
  if(bound.required && !bound.qualify)return reject(CaptureFailure::MissingQualification);
  const CaptureFieldAccess access{window,root,*field,bound.format,{parents,root.layout,index,bound.format.canonical},source};
  if(!writer.Allows(access.location))return reject(CaptureFailure::InvalidTransaction);
  if(bound.qualify && !bound.qualify(access))return reject(CaptureFailure::QualificationFailed);
  if(bound.handler){
    if(!bound.handler(access,writer,out))return reject(out.reason==CaptureFailure::None?CaptureFailure::MissingValue:out.reason);
  }else if(bound.direct && (selected || (!bound.format.conditional && !bound.format.canonical_zero))){
    const auto offset=bound.copy==UINT32_MAX?field->native_offset:CopyRanges()[bound.copy].native_offset;
    const auto size=bound.copy==UINT32_MAX?field->native_size:CopyRanges()[bound.copy].size;
    if(scratch.bytes.size()<size)return reject(CaptureFailure::ValueBudget);
    const bool cached=bound.copy!=UINT32_MAX && scratch.range==bound.copy && scratch.address==root.address &&
       scratch.subject==root.subject && scratch.witness==window.witness;
    if(!cached){
      scratch.Reset();
      if(bound.copy!=UINT32_MAX)for(std::uint32_t i=0;i<layout->field_count;++i){const auto &other=fields_[layout->first_field+i];
        if(other.copy!=bound.copy)continue;
        const auto &other_field=Fields()[layout->first_field+i];
        const CaptureFieldAccess qualified{window,root,other_field,other.format,{parents,root.layout,std::uint16_t(i),other.format.canonical}};
        if(other.required && !other.qualify){FailCapture(out,CaptureFailure::MissingQualification,root,other_field.index,parents);return reject(out.reason);}
        if(other.qualify && !other.qualify(qualified)){FailCapture(out,CaptureFailure::QualificationFailed,root,other_field.index,parents);return reject(out.reason);}
      }
      if(!window.Copy(root,offset,scratch.bytes.first(size),out))return reject(out.reason);
      scratch.range=bound.copy;scratch.address=root.address;scratch.subject=root.subject;scratch.witness=window.witness;
    }
    if(!writer.PutScalar(access.location,bound.format.scalar,bound.format.components,
        scratch.bytes.subspan(field->native_offset-offset,field->native_size)))return reject(writer.Diagnostic().reason);
  }else return reject(CaptureFailure::MissingHandler);
  if(!window.Check(root,out) || (bound.qualify && !bound.qualify(access)))return reject(CaptureFailure::QualificationFailed);
  return true;
}
}
