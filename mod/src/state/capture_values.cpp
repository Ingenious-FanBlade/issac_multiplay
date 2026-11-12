#include "state/capture_values.h"
#include <algorithm>
#include <cstring>
#include <tuple>

namespace cs::state {
namespace {
// 规范父路径按整数逐项比较，不计算内容哈希；每根Finish时排序一次，后台二分查找。
int ComparePath(std::span<const CodecPathStep> a,std::span<const CodecPathStep> b) noexcept {
  if(std::lexicographical_compare(a.begin(),a.end(),b.begin(),b.end()))return -1;
  if(std::lexicographical_compare(b.begin(),b.end(),a.begin(),a.end()))return 1;
  return 0;
}
bool CanonicalKey(CaptureCellKind kind) noexcept {
  return kind==CaptureCellKind::Count || kind==CaptureCellKind::Presence;
}
bool SameKey(const CaptureCell &a,const CaptureCell &b,std::span<const CodecPathStep> paths) noexcept {
  return std::tie(a.layout,a.field,a.kind,a.aux)==std::tie(b.layout,b.field,b.kind,b.aux) &&
    !ComparePath(paths.subspan(a.path,a.depth),paths.subspan(b.path,b.depth));
}
// 只允许生成的嵌套／容器／choice路径，禁止把另一个根的同号字段混入本事务。
bool ValidLocation(std::uint16_t root,std::uint32_t mask,const CodecLocation &location) noexcept {
  if(location.parents.size()>kCodecMaxDepth)return false;
  auto expected=root;
  for(const auto &step:location.parents){
    const auto *field=FindField(step.layout,step.field);BodySourceFieldInfo info;
    if(step.layout!=expected || !field || !DescribeBodySourceField(step.layout,step.field,info) ||
       info.canonical!=step.field)return false;
    if(field->operation==Operation::Container || field->operation==Operation::Choice){
      if(step.element==kNullIndex || step.element>=field->limit ||
         (field->fixed_count && step.element>=field->fixed_count))return false;
    }else if(step.element!=kNullIndex)return false;
    expected=field->nested_layout;
    if(field->operation==Operation::Choice){
      bool allowed=false;
      for(const auto &choice:LayoutChoices())if(choice.layout==step.layout && choice.field==step.field &&
          choice.choice==step.choice_layout){allowed=true;break;}
      if(!allowed)return false;expected=step.choice_layout;
    }else if(step.choice_layout)return false;
    if(!expected)return false;
    const auto *parent=FindLayout(step.layout),*child=FindLayout(expected);
    if(!parent || !child)return false;
    // 多投影共用canonical父路径，但只允许本记录实际选中投影的子块，不扩成整对象。
    std::uint32_t child_mask=0;
    for(std::uint16_t i=0;i<parent->field_count;++i){const auto &projection=Fields()[parent->first_field+i];BodySourceFieldInfo projected;
      if(!(mask&(std::uint32_t{1}<<projection.block)) || !DescribeBodySourceField(parent->id,i,projected) || projected.canonical!=step.field)continue;
      for(std::uint8_t block=0;block<child->block_count;++block)if(Blocks()[child->first_block+block].max_size &&
          (projection.nested_block==255 || projection.nested_block==block))child_mask|=std::uint32_t{1}<<block;
    }
    mask=child_mask;
  }
  BodySourceFieldInfo info;
  const auto *field=FindField(location.layout,location.field);
  return field && (mask&(std::uint32_t{1}<<field->block)) && expected==location.layout && DescribeBodySourceField(location.layout,location.field,info) &&
         info.canonical==location.canonical_field;
}
bool Overlap(std::span<const std::byte> a,std::span<const std::byte> b) noexcept {
  if(a.empty()||b.empty())return false;
  const auto x=reinterpret_cast<std::uintptr_t>(a.data()),y=reinterpret_cast<std::uintptr_t>(b.data());
  return x<=y ? y-x<a.size() : x-y<b.size();
}
[[noreturn]] void SourceGap(const char *reason){throw Gap("captured_body_source",reason);}
}
CaptureBuffer::CaptureBuffer(CaptureCapacity cap) {
  if(!cap.records || !cap.cells || !cap.paths || !cap.bytes)throw std::invalid_argument("capture_capacity");
  const auto estimate=sizeof(*this)+128+std::uint64_t(cap.records)*sizeof(CaptureRecord)+
    std::uint64_t(cap.cells)*sizeof(CaptureCell)+std::uint64_t(cap.paths)*sizeof(CodecPathStep)+cap.bytes+std::uint64_t(cap.references)*sizeof(CaptureReferenceEntry);
  memory_=memory::Reserve(memory::Pool::Values,estimate,"state_capture_buffers");
  records_.resize(cap.records);cells_.resize(cap.cells);paths_.resize(cap.paths);bytes_.resize(cap.bytes);references_.resize(cap.references);
  memory_->Resize(sizeof(*this)+128+std::uint64_t(records_.capacity())*sizeof(CaptureRecord)+
    std::uint64_t(cells_.capacity())*sizeof(CaptureCell)+std::uint64_t(paths_.capacity())*sizeof(CodecPathStep)+bytes_.capacity()+
    std::uint64_t(references_.capacity())*sizeof(CaptureReferenceEntry));
}
CaptureValueWriter::CaptureValueWriter(CaptureBuffer &buffer,CaptureWitness witness) noexcept
    :buffer_(buffer),witness_(witness) {
  if(!ValidCaptureWitness(witness))Fail(CaptureFailure::InvalidWindow);
}
bool CaptureValueWriter::Production(const CaptureRecord &source) noexcept {
  if((!open_ && !records_) || sealed_ || !source.purpose || !source.instance || !source.generation ||
     source.production.phase!=CapturePhase::Production || !ValidCaptureWitness(source.production))return Fail(CaptureFailure::InvalidTransaction);
  auto &out=buffer_.records_[open_?records_:records_-1];out.production=source.production;out.owner=source.owner;
  out.instance=source.instance;out.generation=source.generation;out.event_sequence=source.event_sequence;
  out.call=source.call;out.parent=source.parent;out.purpose=source.purpose;out.pass=source.pass;return true;
}
bool CaptureValueWriter::Append(const CapturePacket &packet,std::uint32_t index,CaptureAppendContext context) noexcept {
  if(index>=packet.records.size())return Fail(CaptureFailure::InvalidTransaction);
  const auto &record=packet.records[index];
  if(record.first_cell>packet.cells.size() || record.cell_count>packet.cells.size()-record.first_cell ||
     !BeginRoot(record.subject,record.layout,record.stream,record.mask))return Fail(CaptureFailure::InvalidTransaction);
  if(record.purpose && !Production(record))return false;
  for(const auto &cell:packet.cells.subspan(record.first_cell,record.cell_count)){
    if(cell.path>packet.paths.size() || cell.depth>packet.paths.size()-cell.path ||
       cell.offset>packet.bytes.size() || cell.bytes>packet.bytes.size()-cell.offset)return Fail(CaptureFailure::InvalidValue);
    BodySourceFieldInfo format;if(!DescribeBodySourceField(cell.layout,cell.field,format))return Fail(CaptureFailure::UnknownLayout);
    const CodecLocation location{packet.paths.subspan(cell.path,cell.depth),cell.layout,cell.field,format.canonical};
    if(!Put(location,cell.kind,cell.scalar,cell.count,cell.aux,packet.bytes.subspan(cell.offset,cell.bytes)))return false;
    if(cell.kind==CaptureCellKind::References){
      if(cell.bytes!=std::uint64_t(cell.count)*9)return Fail(CaptureFailure::InvalidValue,cell.field,location.parents);
      auto *out=buffer_.bytes_.data()+buffer_.cells_[cells_-1].offset;
      for(std::uint32_t i=0;i<cell.count;++i,out+=9){CaptureLink link{CapturedReferenceKind(std::to_integer<unsigned>(out[0])),0};
        for(unsigned j=0;j<8;++j)link.handle|=std::uint64_t(std::to_integer<unsigned>(out[j+1]))<<(j*8);
        if(link.kind!=CapturedReferenceKind::Null){if(!context.reference || !link.handle || link.handle>packet.references.size())return Fail(CaptureFailure::ReferencePending,cell.field,location.parents);
          link=context.reference(context.owner,packet.references[std::size_t(link.handle-1)]);
          if(link.kind!=CapturedReferenceKind::Identity && link.kind!=CapturedReferenceKind::Pending)return Fail(CaptureFailure::ReferencePending,cell.field,location.parents);}
        out[0]=std::byte(link.kind);for(unsigned j=0;j<8;++j)out[j+1]=std::byte(link.handle>>(8*j));reference_limit_=std::max(reference_limit_,link.handle);
      }
    }
  }
  return FinishRoot();
}
bool CaptureValueWriter::Fail(CaptureFailure reason,std::uint16_t field,std::span<const CodecPathStep> parents) noexcept {
  auto location=root_;
  if(!parents.empty()){const auto &last=parents.back();const auto *parent=FindField(last.layout,last.field);
    if(last.choice_layout)location.layout=last.choice_layout;else if(parent && parent->nested_layout)location.layout=parent->nested_layout;}
  return FailCapture(diagnostic_,reason,location,field,parents);
}
bool CaptureValueWriter::Fail(const CaptureDiagnostic &diagnostic) noexcept {
  if(diagnostic_.reason==CaptureFailure::None)diagnostic_=diagnostic;
  if(diagnostic_.reason==CaptureFailure::None)Fail(CaptureFailure::Incomplete);
  return false;
}
bool CaptureValueWriter::BeginRoot(CaptureSubject subject,std::uint16_t layout,BodyStream stream,std::uint32_t mask) noexcept {
  if(diagnostic_.reason!=CaptureFailure::None)return false;
  if(open_||sealed_)return Fail(CaptureFailure::InvalidTransaction);
  root_={subject,0,0,layout};
  const auto *description=FindLayout(layout);BodySourceFieldInfo ignored;
  if(!description || (description->field_count && !DescribeBodySourceField(layout,0,ignored)))return Fail(CaptureFailure::UnknownLayout);
  if(!subject.space || !subject.local || !subject.use ||
     (stream!=BodyStream::Value && stream!=BodyStream::World && stream!=BodyStream::Body))return Fail(CaptureFailure::InvalidValue);
  std::uint32_t allowed=0;
  for(std::uint8_t i=0;i<description->block_count;++i){const auto &block=Blocks()[description->first_block+i];
    if(block.max_size && (stream==BodyStream::Value || block.body==(stream==BodyStream::Body)))allowed|=std::uint32_t{1}<<i;}
  if((mask&~allowed) || (stream!=BodyStream::Value && !mask) || (stream==BodyStream::Value && mask!=allowed))return Fail(CaptureFailure::InvalidValue);
  if(records_>=buffer_.records_.size())return Fail(CaptureFailure::RecordBudget);
  mark_cells_=cells_;mark_paths_=paths_;mark_bytes_=bytes_;open_=true;
  buffer_.records_[records_]={subject,layout,stream,mask,cells_,0};return true;
}
bool CaptureValueWriter::Put(const CodecLocation &location,CaptureCellKind kind,ScalarKind scalar,
    std::uint32_t count,std::uint16_t aux,std::span<const std::byte> value) noexcept {
  if(diagnostic_.reason!=CaptureFailure::None)return false;
  if(!Allows(location))return Fail(CaptureFailure::InvalidTransaction,location.field,location.parents);
  if(cells_>=buffer_.cells_.size())return Fail(CaptureFailure::ValueBudget,location.field,location.parents);
  if(location.parents.size()>buffer_.paths_.size()-paths_)return Fail(CaptureFailure::PathBudget,location.field,location.parents);
  if(value.size()>buffer_.bytes_.size()-bytes_ || value.size()>UINT32_MAX)return Fail(CaptureFailure::ValueBudget,location.field,location.parents);
  auto &cell=buffer_.cells_[cells_++];
  cell={paths_,bytes_,std::uint32_t(value.size()),count,location.layout,
    CanonicalKey(kind)?location.canonical_field:location.field,aux,std::uint8_t(location.parents.size()),kind,scalar};
  std::copy(location.parents.begin(),location.parents.end(),buffer_.paths_.begin()+paths_);paths_+=std::uint32_t(location.parents.size());
  if(!value.empty())std::memmove(buffer_.bytes_.data()+bytes_,value.data(),value.size());
  bytes_+=std::uint32_t(value.size());return true;
}
bool CaptureValueWriter::Allows(const CodecLocation &location) const noexcept {
  return open_ && !sealed_ && diagnostic_.reason==CaptureFailure::None &&
    ValidLocation(root_.layout,buffer_.records_[records_].mask,location);
}
bool CaptureValueWriter::LastValue(const CodecLocation &location,CaptureCellKind kind,CaptureWrittenValue &out) const noexcept {
  out={};if(!Allows(location) || cells_==mark_cells_)return false;
  const auto &cell=buffer_.cells_[cells_-1];
  if(cell.layout!=location.layout || cell.field!=(CanonicalKey(kind)?location.canonical_field:location.field) ||
     cell.kind!=kind || ComparePath(std::span(buffer_.paths_).subspan(cell.path,cell.depth),location.parents))return false;
  out={cell.scalar,cell.count,cell.aux,std::span(buffer_.bytes_).subspan(cell.offset,cell.bytes)};return true;
}
bool CaptureValueWriter::PutScalar(const CodecLocation &location,ScalarKind scalar,std::uint32_t count,std::span<const std::byte> value) noexcept {
  BodySourceFieldInfo info;const auto *field=FindField(location.layout,location.field);
  const auto width=CaptureScalarWidth(scalar);
  if(!field || !DescribeBodySourceField(location.layout,location.field,info) || info.linked ||
     !width || scalar!=info.scalar || !info.components || std::uint64_t(count)*width!=value.size() ||
     (field->operation!=Operation::Container && count!=info.components) ||
     (field->operation==Operation::Container && (count%info.components || count/info.components>field->limit)) ||
     value.size()>field->max_size)return Fail(CaptureFailure::InvalidValue,location.field,location.parents);
  return Put(location,CaptureCellKind::Scalar,scalar,count,0,value);
}
bool CaptureValueWriter::PutCount(const CodecLocation &location,std::uint32_t value) noexcept {
  const auto *field=FindField(location.layout,location.field);
  if(!field || field->operation!=Operation::Container || value>field->limit || value>UINT16_MAX ||
     (field->count_bytes==1 && value>UINT8_MAX) || (field->fixed_count && value!=field->fixed_count))
    return Fail(CaptureFailure::InvalidValue,location.field,location.parents);
  return Put(location,CaptureCellKind::Count,ScalarKind::None,value,0,{});
}
bool CaptureValueWriter::PutPresence(const CodecLocation &location,bool value) noexcept {
  BodySourceFieldInfo info;
  if(!DescribeBodySourceField(location.layout,location.field,info) || !info.shared_owner)return Fail(CaptureFailure::InvalidValue,location.field,location.parents);
  return Put(location,CaptureCellKind::Presence,ScalarKind::None,value?1u:0u,0,{});
}
bool CaptureValueWriter::PutSlotMask(const CodecLocation &location,std::uint32_t value) noexcept {
  const auto *field=FindField(location.layout,location.field);BodySourceFieldInfo info;
  if(!field || !DescribeBodySourceField(location.layout,location.field,info) || !info.slots ||
     field->limit>=32 || value&~((1u<<field->limit)-1u))return Fail(CaptureFailure::InvalidValue,location.field,location.parents);
  return Put(location,CaptureCellKind::SlotMask,ScalarKind::None,value,0,{});
}
bool CaptureValueWriter::PutSlotLayout(const CodecLocation &location,std::uint8_t slot,std::uint16_t layout) noexcept {
  const auto *field=FindField(location.layout,location.field);bool allowed=false;
  if(field && slot<field->limit)for(const auto &choice:LayoutChoices())if(choice.layout==location.layout &&
     choice.field==location.field && choice.choice==layout){allowed=true;break;}
  if(!allowed)return Fail(CaptureFailure::InvalidValue,location.field,location.parents);
  return Put(location,CaptureCellKind::SlotLayout,ScalarKind::None,layout,slot,{});
}
bool CaptureValueWriter::PutLinks(const CodecLocation &location,bool resource,std::uint8_t table,std::span<const CaptureLink> links) noexcept {
  const auto *field=FindField(location.layout,location.field);BodySourceFieldInfo info;
  if(!field || !DescribeBodySourceField(location.layout,location.field,info) || !info.linked ||
     resource!=(info.resource_table!=255) || table!=info.resource_table ||
     (field->operation!=Operation::Container && links.size()!=info.components) ||
     (field->operation==Operation::Container && links.size()>std::uint64_t(field->limit)*info.components))return Fail(CaptureFailure::InvalidValue,location.field,location.parents);
  for(const auto &link:links)if((link.kind==CapturedReferenceKind::Null && link.handle) ||
      ((link.kind==CapturedReferenceKind::Identity || link.kind==CapturedReferenceKind::Pending) && !link.handle) ||
      link.kind==CapturedReferenceKind::Invalid || unsigned(link.kind)>unsigned(CapturedReferenceKind::Invalid))
    return Fail(CaptureFailure::InvalidValue,location.field,location.parents);
  // 固定9B的自有编码排除C++ padding；后续memcmp不受调用者栈上填充字节影响。
  if(std::uint64_t(links.size())*9>buffer_.bytes_.size()-bytes_)return Fail(CaptureFailure::ValueBudget,location.field,location.parents);
  if(!Put(location,resource?CaptureCellKind::Resources:CaptureCellKind::References,ScalarKind::U16,
          std::uint32_t(links.size()),table,{}))return false;
  auto &cell=buffer_.cells_[cells_-1];cell.bytes=std::uint32_t(links.size()*9);
  for(const auto &link:links){buffer_.bytes_[bytes_++]=std::byte(link.kind);
    if(!resource)reference_limit_=std::max(reference_limit_,link.handle);
    for(unsigned i=0;i<8;++i)buffer_.bytes_[bytes_++]=std::byte((link.handle>>(8*i))&255);}
  return true;
}
bool CaptureValueWriter::FinishRoot() noexcept {
  if(diagnostic_.reason!=CaptureFailure::None){AbortRoot();return false;}
  if(!open_||sealed_)return Fail(CaptureFailure::InvalidTransaction);
  auto begin=buffer_.cells_.begin()+mark_cells_,end=buffer_.cells_.begin()+cells_;
  const auto paths=std::span<const CodecPathStep>(buffer_.paths_).first(paths_);
  std::sort(begin,end,[paths](const auto &a,const auto &b){
    const auto path=ComparePath(paths.subspan(a.path,a.depth),paths.subspan(b.path,b.depth));
    return path ? path<0 : std::tie(a.layout,a.field,a.kind,a.aux)<std::tie(b.layout,b.field,b.kind,b.aux);});
  auto output=mark_cells_;
  for(auto cursor=mark_cells_;cursor<cells_;++cursor){const auto &cell=buffer_.cells_[cursor];
    if(output>mark_cells_ && SameKey(buffer_.cells_[output-1],cell,paths)){
      const auto &prior=buffer_.cells_[output-1];
      if(cell.kind!=CaptureCellKind::Count && cell.kind!=CaptureCellKind::Presence)
        return Fail(CaptureFailure::DuplicateValue,cell.field,paths.subspan(cell.path,cell.depth));
      if(cell.count!=prior.count)return Fail(CaptureFailure::DuplicateValue,cell.field,paths.subspan(cell.path,cell.depth));
      continue;
    }
    buffer_.cells_[output++]=cell;
  }
  cells_=output;auto &record=buffer_.records_[records_++];record.cell_count=cells_-record.first_cell;
  open_=false;return true;
}
void CaptureValueWriter::AbortRoot() noexcept {
  if(!open_)return;
  cells_=mark_cells_;paths_=mark_paths_;bytes_=mark_bytes_;open_=false;
  Fail(CaptureFailure::Incomplete);
}
bool CaptureValueWriter::Seal(std::uint32_t expected,CapturePacket &out) noexcept {
  out={};if(diagnostic_.reason!=CaptureFailure::None)return false;
  if(open_||sealed_||!expected||expected!=records_)return Fail(CaptureFailure::Incomplete);
  sealed_=true;out={witness_,std::span(buffer_.records_).first(records_),std::span(buffer_.cells_).first(cells_),
    std::span(buffer_.paths_).first(paths_),std::span(buffer_.bytes_).first(bytes_),catalog_,std::span(buffer_.references_).first(catalog_.references)};return true;
}
bool CaptureValueWriter::Catalog(CaptureCatalogStamp stamp,std::span<const CaptureReferenceEntry> references) noexcept {
  if(sealed_ || open_ || diagnostic_.reason!=CaptureFailure::None)return Fail(CaptureFailure::InvalidTransaction);
  if(reference_limit_>references.size() || reference_limit_>buffer_.references_.size())return Fail(CaptureFailure::ReferencePending);
  stamp.references=std::uint32_t(reference_limit_);std::copy_n(references.begin(),stamp.references,buffer_.references_.begin());catalog_=stamp;return true;
}
CapturedBodySource::CapturedBodySource(const CapturePacket &packet,std::uint32_t record,CaptureLinkResolver resolver,std::span<std::byte> scratch)
    :packet_(packet),resolver_(resolver),scratch_(scratch) {
  if(record>=packet.records.size())SourceGap("record_index");record_=packet.records[record];
  if(record_.first_cell>packet.cells.size() || record_.cell_count>packet.cells.size()-record_.first_cell ||
     Overlap(packet.bytes,scratch))SourceGap("record_or_scratch_range");
}
const CaptureCell &CapturedBodySource::Find(const CodecLocation &location,CaptureCellKind kind,std::uint16_t aux) const {
  const auto field=CanonicalKey(kind)?location.canonical_field:location.field;
  const auto cells=packet_.cells.subspan(record_.first_cell,record_.cell_count);
  const auto it=std::lower_bound(cells.begin(),cells.end(),0,[&](const CaptureCell &cell,int){
    if(cell.path>packet_.paths.size() || cell.depth>packet_.paths.size()-cell.path)SourceGap("path_range");
    const auto path=ComparePath(packet_.paths.subspan(cell.path,cell.depth),location.parents);
    return path ? path<0 : std::tie(cell.layout,cell.field,cell.kind,cell.aux)<std::tie(location.layout,field,kind,aux);});
  if(it==cells.end() || std::tie(it->layout,it->field,it->kind,it->aux)!=std::tie(location.layout,field,kind,aux) ||
     it->path>packet_.paths.size() || it->depth>packet_.paths.size()-it->path ||
     ComparePath(packet_.paths.subspan(it->path,it->depth),location.parents))SourceGap("missing_captured_value");
  return *it;
}
std::span<const std::byte> CapturedBodySource::Bytes(const CaptureCell &cell) const {
  if(cell.offset>packet_.bytes.size() || cell.bytes>packet_.bytes.size()-cell.offset)SourceGap("value_range");
  return packet_.bytes.subspan(cell.offset,cell.bytes);
}
std::span<const std::byte> CapturedBodySource::Scalar(const CodecLocation &location,ScalarKind scalar,std::uint32_t count) const {
  BodySourceFieldInfo info;if(!DescribeBodySourceField(location.layout,location.field,info))SourceGap("unknown_source_field");
  if(!info.linked){const auto &cell=Find(location,CaptureCellKind::Scalar);
    if(cell.scalar!=scalar || cell.count!=count || std::uint64_t(count)*CaptureScalarWidth(scalar)!=cell.bytes)SourceGap("scalar_shape");return Bytes(cell);}
  const bool resource=info.resource_table!=255;
  const auto &cell=Find(location,resource?CaptureCellKind::Resources:CaptureCellKind::References,info.resource_table);
  const auto values=Bytes(cell);
  if(scalar!=ScalarKind::U16 || count!=cell.count || std::uint64_t(count)*9!=values.size() ||
     std::uint64_t(count)*2>scratch_.size())SourceGap("link_shape_or_scratch");
  for(std::uint32_t i=0;i<count;++i){CaptureLink link;const auto begin=std::size_t(i)*9;
    link.kind=CapturedReferenceKind(std::to_integer<unsigned>(values[begin]));link.handle=0;
    for(unsigned j=0;j<8;++j)link.handle|=std::uint64_t(std::to_integer<unsigned>(values[begin+1+j]))<<(8*j);
    std::uint16_t id=kNullIndex;
    if(link.kind==CapturedReferenceKind::Null){if(link.handle)SourceGap("null_link_handle");}
    else if((link.kind!=CapturedReferenceKind::Identity && link.kind!=CapturedReferenceKind::Pending) || !link.handle ||
       !resolver_.resolve || !resolver_.resolve(resolver_.context,record_.subject,resource,info.resource_table,link,id) || id==kNullIndex)
      SourceGap(resource?"resource_handle_unresolved":"reference_handle_pending");
    scratch_[std::size_t(i)*2]=std::byte(id&255);scratch_[std::size_t(i)*2+1]=std::byte(id>>8);
  }
  return scratch_.first(std::size_t(count)*2);
}
std::uint32_t CapturedBodySource::Count(const CodecLocation &location) const {return Find(location,CaptureCellKind::Count).count;}
bool CapturedBodySource::SharedPresent(const CodecLocation &location) const {return Find(location,CaptureCellKind::Presence).count!=0;}
std::uint32_t CapturedBodySource::SlotMask(const CodecLocation &location) const {return Find(location,CaptureCellKind::SlotMask).count;}
std::uint16_t CapturedBodySource::SlotLayout(const CodecLocation &location,std::uint8_t slot) const {return std::uint16_t(Find(location,CaptureCellKind::SlotLayout,slot).count);}
}
