#include "state/production_cache.h"
#include <algorithm>
#include <cstring>

namespace cs::state {
namespace {
constexpr std::uint32_t page_bytes=1024;
std::size_t Align(std::size_t n) noexcept {return (n+7)&~std::size_t(7);}
bool SameBody(CaptureSubject a,CaptureSubject b) noexcept {return a.space==b.space&&a.local==b.local&&a.use==b.use;}
}
ProductionCache::ProductionCache(std::thread::id owner,std::uint32_t bytes,std::uint32_t groups):thread_(owner) {
  if(owner==std::thread::id{} || !groups || !bytes || bytes%page_bytes)throw std::invalid_argument("production_cache_capacity");
  memory_=memory::Reserve(memory::Pool::Values,sizeof(*this)+bytes+bytes/page_bytes+std::uint64_t(groups)*sizeof(Entry)+2u*65536u*4u,"binary_production_cache");
  arena_.resize(bytes/8);pages_.resize(bytes/page_bytes);entries_.resize(groups);
  reference_map_.resize(65536);reference_seen_.resize(65536);
}
CapturePacket ProductionCache::View(const Entry &entry) const noexcept {
  const auto *first=reinterpret_cast<const std::byte *>(arena_.data())+std::size_t(entry.page)*page_bytes;
  CapturePacket out;out.witness=entry.witness;
  out.records={reinterpret_cast<const CaptureRecord *>(first),entry.records};first+=Align(entry.records*sizeof(CaptureRecord));
  out.cells={reinterpret_cast<const CaptureCell *>(first),entry.cells};first+=Align(entry.cells*sizeof(CaptureCell));
  out.paths={reinterpret_cast<const CodecPathStep *>(first),entry.paths};first+=Align(entry.paths*sizeof(CodecPathStep));out.bytes={first,entry.bytes};first+=Align(entry.bytes);
  out.references={reinterpret_cast<const CaptureReferenceEntry *>(first),entry.references};out.catalog=entry.catalog;return out;
}
void ProductionCache::Release(Entry &entry) noexcept {
  if(entry.pages)std::fill_n(pages_.begin()+entry.page,entry.pages,std::uint8_t{});entry={};
}
// 替换先分配新页并复制，成功后才退休旧页；失败保留旧值但本次目标明确失败。
bool ProductionCache::Store(const CapturePacket &packet,CaptureDiagnostic &d) noexcept {
  if(std::this_thread::get_id()!=thread_)return FailCapture(d,CaptureFailure::WrongThread,{});
  if(packet.records.empty() || packet.records.size()>UINT32_MAX || packet.cells.size()>UINT32_MAX ||
     packet.paths.size()>UINT32_MAX || packet.bytes.size()>UINT32_MAX)return FailCapture(d,CaptureFailure::InvalidValue,{});
  const auto &root=packet.records.front();CaptureRoot location{root.subject,0,0,root.layout};
  if(!root.purpose || !root.instance || !root.generation || !ValidCaptureWitness(root.production) || root.production.phase!=CapturePhase::Production)
    return FailCapture(d,CaptureFailure::MissingQualification,location);
  if(++reference_generation_==0){std::fill(reference_seen_.begin(),reference_seen_.end(),0u);reference_generation_=1;}
  std::uint32_t references=0;
  for(const auto &cell:packet.cells)if(cell.kind==CaptureCellKind::References){
    if(cell.offset>packet.bytes.size() || cell.bytes>packet.bytes.size()-cell.offset || cell.bytes!=std::uint64_t(cell.count)*9)
      return FailCapture(d,CaptureFailure::InvalidValue,location);
    const auto bytes=packet.bytes.subspan(cell.offset,cell.bytes);
    for(std::size_t i=0;i<bytes.size();i+=9){std::uint64_t handle=0;for(unsigned j=0;j<8;++j)handle|=std::uint64_t(std::to_integer<unsigned>(bytes[i+j+1]))<<(8*j);
      if(bytes[i]==std::byte(CapturedReferenceKind::Null)){if(handle)return FailCapture(d,CaptureFailure::InvalidValue,location);continue;}
      if(!handle || handle>packet.references.size() || handle>=reference_map_.size())return FailCapture(d,CaptureFailure::ReferencePending,location);
      const auto at=std::size_t(handle);if(reference_seen_[at]!=reference_generation_){reference_seen_[at]=reference_generation_;reference_map_[at]=++references;}}
  }
  auto slot=entries_.size(),free=entries_.size();
  for(std::size_t i=0;i<entries_.size();++i){const auto &entry=entries_[i];if(!entry.pages){if(free==entries_.size())free=i;continue;}
    if(entry.purpose==root.purpose && SameBody(entry.subject,root.subject) && entry.instance==root.instance && entry.generation==root.generation){slot=i;break;}}
  if(slot==entries_.size())slot=free;if(slot==entries_.size())return FailCapture(d,CaptureFailure::RecordBudget,location);
  const auto total=Align(packet.records.size_bytes())+Align(packet.cells.size_bytes())+Align(packet.paths.size_bytes())+Align(packet.bytes.size())+std::size_t(references)*sizeof(CaptureReferenceEntry);
  const auto required=(total+page_bytes-1)/page_bytes;std::size_t start=0,run=0;
  for(std::size_t i=0;i<pages_.size();++i){if(pages_[i])run=0;else if(++run==required){start=i+1-run;break;}}
  if(!required || run<required)return FailCapture(d,CaptureFailure::ValueBudget,location);
  if(entries_[slot].pages && root.production.revision<=entries_[slot].witness.revision)return FailCapture(d,CaptureFailure::SourceChanged,location);
  auto *first=reinterpret_cast<std::byte *>(arena_.data())+start*page_bytes;
  const auto copy=[&](auto values){if(!values.empty())std::memcpy(first,values.data(),values.size_bytes());first+=Align(values.size_bytes());};
  copy(packet.records);copy(packet.cells);copy(packet.paths);auto *values=first;copy(packet.bytes);
  // 缓存仅保存实际引用的纯值身份，避免每个HUD复制整份世界引用目录。
  auto *catalog=reinterpret_cast<CaptureReferenceEntry *>(first);
  for(std::size_t i=1;i<=packet.references.size() && i<reference_seen_.size();++i)if(reference_seen_[i]==reference_generation_)catalog[reference_map_[i]-1]=packet.references[i-1];
  for(const auto &cell:packet.cells)if(cell.kind==CaptureCellKind::References)for(std::size_t i=0;i<cell.bytes;i+=9){auto *value=values+cell.offset+i;
    std::uint64_t handle=0;for(unsigned j=0;j<8;++j)handle|=std::uint64_t(std::to_integer<unsigned>(value[j+1]))<<(8*j);
    if(handle){const auto mapped=reference_map_[std::size_t(handle)];for(unsigned j=0;j<8;++j)value[j+1]=std::byte(std::uint64_t(mapped)>>(8*j));}}
  // 内容及真实依赖未变时保留初次生产修订；观察次数不构成发送新版本的理由。
  if(root.purpose!=356 && entries_[slot].pages && entries_[slot].witness.scope==root.production.scope){
    const auto old=View(entries_[slot]);bool same=old.records.size()==packet.records.size()&&old.cells.size()==packet.cells.size()&&
      old.paths.size()==packet.paths.size()&&old.bytes.size()==packet.bytes.size()&&old.references.size()==references;
    for(std::size_t i=0;i<old.records.size()&&same;++i){const auto &a=old.records[i],&b=packet.records[i];same=a.subject==b.subject&&a.owner==b.owner&&
      a.layout==b.layout&&a.stream==b.stream&&a.mask==b.mask&&a.first_cell==b.first_cell&&a.cell_count==b.cell_count&&
      a.instance==b.instance&&a.generation==b.generation&&a.purpose==b.purpose&&a.event_sequence==b.event_sequence&&a.pass==b.pass;}
    for(std::size_t i=0;i<old.cells.size()&&same;++i){const auto &a=old.cells[i],&b=packet.cells[i];same=a.path==b.path&&a.offset==b.offset&&a.bytes==b.bytes&&
      a.count==b.count&&a.layout==b.layout&&a.field==b.field&&a.aux==b.aux&&a.depth==b.depth&&a.kind==b.kind&&a.scalar==b.scalar;}
    if(same)same=std::equal(old.paths.begin(),old.paths.end(),packet.paths.begin())&&std::equal(old.bytes.begin(),old.bytes.end(),values);
    for(std::size_t i=0;i<old.references.size()&&same;++i){const auto &a=old.references[i],&b=catalog[i];same=a.space==b.space&&a.local==b.local&&a.use==b.use&&a.source_slot==b.source_slot&&a.order==b.order;}
    if(same)return true;
  }
  std::fill_n(pages_.begin()+start,required,std::uint8_t{1});Release(entries_[slot]);
  entries_[slot]={std::uint32_t(start),std::uint32_t(required),std::uint32_t(packet.records.size()),std::uint32_t(packet.cells.size()),
    std::uint32_t(packet.paths.size()),std::uint32_t(packet.bytes.size()),references,root.production,packet.catalog,root.subject,root.purpose,root.instance,root.generation};
  entries_[slot].catalog.references=references;return true;
}
bool ProductionCache::Append(const StateScope &scope,CaptureValueWriter &writer,std::uint32_t &roots,CaptureDiagnostic &d,CaptureAppendContext context) const noexcept {
  if(std::this_thread::get_id()!=thread_)return FailCapture(d,CaptureFailure::WrongThread,{});
  for(const auto &entry:entries_)if(entry.pages){if(entry.witness.scope!=scope)continue;const auto packet=View(entry);
    for(std::uint32_t i=0;i<packet.records.size();++i){if(!writer.Append(packet,i,context)){d=writer.Diagnostic();return false;}++roots;}}
  return true;
}
void ProductionCache::Retire(CaptureSubject subject) noexcept {if(std::this_thread::get_id()!=thread_)return;
  for(auto &entry:entries_)if(entry.pages && SameBody(entry.subject,subject))Release(entry);}
void ProductionCache::RetirePurpose(CaptureSubject subject,std::uint16_t purpose) noexcept {if(std::this_thread::get_id()!=thread_)return;
  for(auto &entry:entries_)if(entry.pages&&entry.purpose==purpose&&SameBody(entry.subject,subject))Release(entry);}
void ProductionCache::Clear() noexcept {if(std::this_thread::get_id()!=thread_)return;for(auto &entry:entries_)Release(entry);}
bool ProductionCache::Contains(const StateScope &scope,std::uint16_t purpose) const noexcept {
  if(std::this_thread::get_id()!=thread_)return false;
  for(const auto &entry:entries_)if(entry.pages&&entry.witness.scope==scope&&entry.purpose==purpose)return true;return false;
}
std::uint64_t ProductionCache::Revision(CaptureSubject subject,std::uint16_t purpose,std::uint64_t instance) const noexcept {
  if(std::this_thread::get_id()!=thread_)return 0;
  for(const auto &entry:entries_)if(entry.pages&&SameBody(entry.subject,subject)&&entry.purpose==purpose&&entry.instance==instance)return entry.witness.revision;return 0;
}
}
