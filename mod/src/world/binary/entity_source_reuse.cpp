#include "world/binary/entity_source_internal.h"

namespace cs::world::binary {
// 只合并相邻且按顺序读取的字节。证明耗尽时原采集照常成功。
void NativeInputView::Add(std::uint32_t address,std::span<const std::byte> value) noexcept {
  if(!evidence.valid||value.empty())return;
  if(value.size()>bytes.size()-evidence.used){
    evidence.valid=false;return;
  }
  auto *last=evidence.count?&ranges[evidence.count-1]:nullptr;
  if(last&&std::uint64_t(last->address)+last->size==address)
    last->size+=std::uint16_t(value.size());
  else{
    if(evidence.count==ranges.size()){evidence.valid=false;return;}
    ranges[evidence.count++]={address,evidence.used,std::uint16_t(value.size())};
  }
  std::copy(value.begin(),value.end(),bytes.begin()+evidence.used);
  evidence.used+=std::uint16_t(value.size());
}
void EntitySource::Impl::RecordInput(std::uint32_t address,std::span<const std::byte> bytes) noexcept {
  for(auto *record=input_recorder;record;record=record->parent)record->input.Add(address,bytes);
}
bool EntitySource::Impl::CopyInput(std::uint32_t address,std::span<std::byte> bytes) noexcept {
  if(!services.reads->Copy(address,bytes.data(),bytes.size()))return false;
  if(input_recorder)RecordInput(address,bytes);
  return true;
}
bool EntitySource::Impl::InputsMatch(NativeInputView proof,std::uint32_t root,std::uint32_t extent) noexcept {
  if(!proof.evidence.valid||!proof.evidence.count)return false;
  std::array<std::byte,4096> local,external;
  if(extent>local.size()||(extent&&!services.reads->Probe(root,std::span(local).first(extent))))return false;
  for(const auto &range:proof.ranges.first(proof.evidence.count)){
    const auto expected=proof.bytes.subspan(range.offset,range.size);
    if(root&&range.address>=root&&std::uint64_t(range.address)+range.size<=std::uint64_t(root)+extent){
      // 根内读取合并成一次连续复制。只比较曾实际消费的字段字节。
      const auto current=std::span(local).subspan(range.address-root,range.size);
      if(!std::equal(expected.begin(),expected.end(),current.begin()))return false;
    }else{
      // 外部段按源读取顺序复核，指针、长度或选择器一旦变化便停止沿旧地址查找。
      const auto current=std::span(external).first(range.size);
      if(!services.reads->Probe(range.address,current)||!std::equal(expected.begin(),expected.end(),current.begin()))return false;
    }
  }
  return true;
}
void EntitySource::Impl::ReplayInput(NativeInputView proof) noexcept {
  if(!input_recorder)return;
  for(const auto &range:proof.ranges.first(proof.evidence.count))
    RecordInput(range.address,proof.bytes.subspan(range.offset,range.size));
}
CaptureLink EntitySource::Impl::ReadReference(std::uint32_t address) noexcept {
  const auto link=services.reference(services.owner,address);
  if(!address||!root_input||!root_input->input.evidence.valid)return link;
  auto &proof=*root_input;
  const auto current=std::span(proof.references).first(proof.reference_count);
  const auto existing=std::find_if(current.begin(),current.end(),[&](const auto &ref){return ref.address==address;});
  CaptureSubject identity;
  if(link.kind!=CapturedReferenceKind::Identity||!services.reference_identity||
     !services.reference_identity(services.owner,address,identity))
    proof.input.evidence.valid=false;
  else if(existing!=current.end())
    proof.input.evidence.valid=existing->target==identity;
  else if(current.size()==proof.references.size())
    proof.input.evidence.valid=false;
  else proof.references[proof.reference_count++]={address,identity};
  return link;
}
bool EntitySource::Impl::MatchesRoot(NativeRootReuse &entry,const CaptureRoot &root) noexcept {
  ++stats.unchanged_checks;
  if(!InputsMatch(entry.input,root.address,root.extent))return false;
  for(const auto &ref:std::span(entry.references).first(entry.reference_count)){
    CaptureSubject identity;
    if(!services.reference_identity||!services.reference_identity(services.owner,ref.address,identity)||identity!=ref.target)return false;
  }
  return ReviewRoot();
}
void EntitySource::Impl::CheckReuseRecipe(EntityRead recipe) noexcept {
  if(!root_input||!root_input->input.evidence.valid)return;
  // 目前只批准 Tear 可达的纯字段、Sprite、容器和引用来源。帧号换算和其它上下文源回退。
  switch(recipe){
    case EntityRead::None:case EntityRead::Raw:case EntityRead::Mask:case EntityRead::Bool:
    case EntityRead::Order:case EntityRead::Flying:case EntityRead::Water:case EntityRead::Reference:
    case EntityRead::File:case EntityRead::ImagePath:case EntityRead::Image:case EntityRead::Animation:case EntityRead::Config:
    case EntityRead::String:case EntityRead::Fixed:case EntityRead::Layers:case EntityRead::LayerFrames:
    case EntityRead::NullFrames:case EntityRead::Plus:
      break;
    default:root_input->input.evidence.valid=false;break;
  }
}
bool EntitySource::Impl::ReuseResource(NativeResourceKey key,CaptureLink &out,void *context,
    bool (*parse)(void *) noexcept,bool cross_frame) noexcept {
  const auto serial=services.reads->Serial();
  if(!serial)return false;
  const auto &w=window.witness;
  const auto category=unsigned(key.kind);
  const bool stable=key.kind==NativeResourceSource::Item || key.kind==NativeResourceSource::Config;
  // 只有已核整局地址稳定的配置允许无内容证言命中。scope／代次／写水位更迭时撤销。
  if(memo_scope!=w.scope || memo_generation!=w.generation || memo_epoch!=reuse_epoch){
    memo_used=0;
    memo_scope=w.scope;
    memo_generation=w.generation;
    memo_epoch=reuse_epoch;
  }
  const auto find=[&](auto end){
    return std::lower_bound(resource_memos.begin(),end,key,[](const auto &entry,const auto &wanted){return entry.key<wanted;});
  };
  // 外层需要完整输入证明时，从真实来源读一遍，或复用下面带完整证言的条目。
  bool recording_inputs=false;
  for(auto *record=input_recorder;record;record=record->parent)recording_inputs|=record->input.evidence.valid;
  if(stable&&!recording_inputs){
    const auto end=resource_memos.begin()+memo_used;
    const auto at=find(end);
    if(at!=end&&at->key==key){out=at->link;++stats.resource_hits[category];return true;}
  }

  NativeResourceReuse *entry=nullptr;
  if(cross_frame&&!resource_reuse.empty()){
    const auto slot=(std::uint64_t(key.address)/4+std::uint64_t(key.owner)/16+category*31u)%resource_reuse.size();
    entry=&resource_reuse[std::size_t(slot)];
  }
  // 动态资源即使在同一读段也必须核完整输入。覆盖不到的记录正常重读。
  const bool hit=entry&&entry->epoch==reuse_epoch&&entry->scope==w.scope&&entry->generation==w.generation&&
    entry->key==key&&InputsMatch(entry->input);
  if(hit){
    out=entry->link;
    if(entry->serial==serial)++stats.resource_hits[category];else ++stats.resource_frame_hits[category];
    ReplayInput(entry->input);
  }else{
    ++stats.resource_parses[category];
    if(entry){
      // 解析依赖可以再次使用缓存。新证明先写在自己的有界栈区，避免内层占用同一槽。
      NativeInputStorage<80,1536> input;
      input.evidence={0,0,true};
      const auto start=witness_used;
      {
        NativeInputRecorder record(input_recorder,input);
        if(!parse(context))return false;
      }
      // 原解析登记了结构证言时照常走该解析入口，保留本根的 Verify 责任。
      if(input.evidence.valid&&input.evidence.count&&witness_used==start&&services.reads->Serial()==serial){
        entry->key=key;entry->scope=w.scope;entry->generation=w.generation;
        entry->epoch=reuse_epoch;entry->serial=serial;entry->link=out;
        entry->input.evidence=input.evidence;
        std::copy_n(input.ranges.begin(),input.evidence.count,entry->input.ranges.begin());
        std::copy_n(input.bytes.begin(),input.evidence.used,entry->input.bytes.begin());
      }
    }else if(!parse(context))return false;
  }
  if(!stable)return true;
  // 解析可能先登记依赖，返回后重新定位稳定配置的键。
  const auto end=resource_memos.begin()+memo_used;
  const auto at=find(end);
  if(at!=end&&at->key==key)at->link=out;
  else if(memo_used<resource_memos.size()){
    std::move_backward(at,end,end+1);*at={key,out};++memo_used;
  }
  return true;
}
void EntitySource::ReserveReuse(std::uint64_t bytes) noexcept {
  auto &s=*impl_;
  const auto assets=std::size_t((std::min)(std::uint64_t{512},bytes/4/sizeof(NativeResourceReuse)));
  const auto roots=std::size_t((std::min)(std::uint64_t{512},(bytes-assets*sizeof(NativeResourceReuse))/sizeof(NativeRootReuse)));
  if(!roots&&!assets)return;
  try {
    auto lease=memory::Reserve(memory::Pool::Values,roots*sizeof(NativeRootReuse)+assets*sizeof(NativeResourceReuse),"entity_capture_reuse");
    std::vector<NativeRootReuse> root_entries(roots);
    std::vector<NativeResourceReuse> resource_entries(assets);
    lease->Resize(root_entries.capacity()*sizeof(NativeRootReuse)+resource_entries.capacity()*sizeof(NativeResourceReuse));
    s.root_reuse.swap(root_entries);s.resource_reuse.swap(resource_entries);s.reuse_charge=std::move(lease);
    s.stats.unchanged_slots=std::uint32_t(roots);s.stats.resource_frame_slots=std::uint32_t(assets);
  }catch(...){
    // 缓存是可选优化，预留失败仍保留正常采集能力。
  }
}
}
