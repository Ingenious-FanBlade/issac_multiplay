#include "world/binary/entity_source_internal.h"

namespace cs::world::binary {
namespace {
SpriteResourceMember Member(CaptureSubject subject) noexcept {return {subject.local,subject.use,subject.space};}
std::uint64_t MemberIndex(SpriteResourceMember member) noexcept {
  return member.local*313u+member.space*101u;
}
}

// 两层共享表的代次只在对应原生缓存清理时推进。成员表与目录在换局、撤销或回卷时一起丢弃。
void EntitySource::ClearResourceCaches() noexcept {
  auto &s=*impl_;
  if(std::this_thread::get_id()!=s.thread)return;
  auto &cache=s.resource_cache;
  std::fill(cache.members.begin(),cache.members.end(),SpriteMemberEpoch{});
  std::fill(cache.sprites.begin(),cache.sprites.end(),SpriteResourceMemo{});
  std::fill(cache.layers.begin(),cache.layers.end(),SpriteLayerResourceMemo{});
  std::fill(cache.animations.begin(),cache.animations.end(),SharedResourceMemo{});
  std::fill(cache.images.begin(),cache.images.end(),SharedResourceMemo{});
  cache.stamp=0;cache.used={};
  s.memo_used=0;
}
void EntitySource::SharedResourceChanged(bool animation) noexcept {
  auto &s=*impl_;
  if(std::this_thread::get_id()!=s.thread)return;
  auto &cache=s.resource_cache;
  auto &epoch=animation?cache.animation_epoch:cache.image_epoch;
  if(++epoch==0){ClearResourceCaches();epoch=1;}
  cache.used[animation?2:3]=0;
}
void EntitySource::ConfigureSharedResourceHooks(unsigned layers) noexcept {
  auto &s=*impl_;
  if(std::this_thread::get_id()==s.thread)s.resource_cache.observed=layers;
}
void EntitySource::Impl::UseResourceScope(const CaptureWitness &w) noexcept {
  auto &cache=resource_cache;
  if(cache.session==w.scope.session&&cache.run==w.scope.run&&cache.generation==w.generation)return;
  std::fill(cache.members.begin(),cache.members.end(),SpriteMemberEpoch{});
  cache.used[2]=cache.used[3]=cache.used[4]=0;
  if(++cache.animation_epoch==0){std::fill(cache.animations.begin(),cache.animations.end(),SharedResourceMemo{});cache.animation_epoch=1;}
  if(++cache.image_epoch==0){std::fill(cache.images.begin(),cache.images.end(),SharedResourceMemo{});cache.image_epoch=1;}
  memo_used=0;
  cache.session=w.scope.session;cache.run=w.scope.run;cache.generation=w.generation;
}
void EntitySource::ForgetSpriteResources(CaptureSubject subject) noexcept {
  auto &s=*impl_;
  auto &members=s.resource_cache.members;
  if(std::this_thread::get_id()!=s.thread||members.empty())return;
  const auto key=Member(subject);
  auto &member=members[MemberIndex(key)%members.size()];
  if(member.member==key)member={};
}
void EntitySource::Impl::CountCacheInsert(unsigned layer) noexcept {
  auto &used=resource_cache.used[layer];
  auto &high=stats.resource_cache_high_water[layer];
  high=std::max(high,++used);
}
// 内嵌 Sprite 用相对对象的槽，Player 外置装扮用容器元素序号；其余间接来源走正常解析。
SpriteResourceMemo *EntitySource::Impl::SpriteMemo(std::uint32_t address,bool create) noexcept {
  auto &cache=resource_cache;
  if(!window.synchronous_root||root_input||cache.sprites.empty()||cache.stamp>=UINT64_MAX-2||
     (facts.subject.space!=1&&facts.subject.space!=2))return nullptr;
  std::uint32_t slot;
  if(address>=facts.address&&address-facts.address<root_extent)
    slot=address-facts.address;
  else if(facts.subject.space==1&&root_layout==197&&player.costume_data&&address>=player.costume_data&&
          (address-player.costume_data)%304==0&&(address-player.costume_data)/304<player.costumes.size())
    slot=0x80000000u|(address-player.costume_data)/304;
  else return nullptr;
  const auto key=Member(facts.subject);
  const auto at=MemberIndex(key);
  auto &member=cache.members[at%cache.members.size()];
  if(!member.epoch||member.member!=key){
    if(!create)return nullptr;
    member={key,++cache.stamp};
  }
  auto &memo=cache.sprites[(at+std::uint64_t(slot)*193u)%cache.sprites.size()];
  if(memo.member_epoch==member.epoch&&memo.slot==slot&&memo.address==address)return &memo;
  if(!create)return nullptr;
  if(!memo.stamp)CountCacheInsert(0);
  memo={};
  memo.member_epoch=member.epoch;memo.stamp=++cache.stamp;memo.slot=slot;memo.address=address;
  return &memo;
}
SpriteLayerResourceMemo *EntitySource::Impl::LayerMemo(const SpriteResourceMemo *sprite,std::uint32_t data,
    std::uint32_t count,std::uint32_t index) noexcept {
  if(!sprite||resource_cache.layers.empty())return nullptr;
  auto &memo=resource_cache.layers[(sprite->stamp*23u+index)%resource_cache.layers.size()];
  if(memo.sprite!=sprite->stamp||memo.data!=data||memo.count!=count||memo.index!=index){
    if(!memo.sprite)CountCacheInsert(1);
    memo={};
    memo.sprite=sprite->stamp;memo.data=data;memo.count=count;memo.index=index;
  }
  return &memo;
}

// 在 Values 池剩余额度内预留整组表。热路径只替换有界槽位，冲突或容量不足时正常解析。
std::uint64_t EntitySource::Impl::ReserveResourceCaches(std::uint64_t bytes) noexcept {
  const auto per_slot=sizeof(SpriteMemberEpoch)+sizeof(SpriteResourceMemo)+
    3*sizeof(SpriteLayerResourceMemo)+2*sizeof(SharedResourceMemo);
  const auto count=std::size_t(std::min<std::uint64_t>(8192,bytes/2/per_slot));
  if(!count)return 0;
  try {
    auto lease=memory::Reserve(memory::Pool::Values,count*per_slot,"entity_capture_resource_layers");
    LayeredResourceCache cache;
    cache.members.resize(count);cache.sprites.resize(count);cache.layers.resize(count*3);
    cache.animations.resize(count);cache.images.resize(count);
    lease->Resize(cache.members.capacity()*sizeof(SpriteMemberEpoch)+cache.sprites.capacity()*sizeof(SpriteResourceMemo)+
      cache.layers.capacity()*sizeof(SpriteLayerResourceMemo)+(cache.animations.capacity()+cache.images.capacity())*sizeof(SharedResourceMemo));
    stats.resource_cache_capacity={std::uint32_t(count),std::uint32_t(count*3),std::uint32_t(count),std::uint32_t(count),
      std::uint32_t(resource_memos.size())};
    resource_cache=std::move(cache);resource_cache_charge=std::move(lease);
    return resource_cache_charge->Bytes();
  }catch(...){return 0;}
}

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
  const auto slice=snapshot.Slice(address,bytes.size());
  if(!slice.empty())std::memcpy(bytes.data(),slice.data(),slice.size());
  else if(!services.reads->Copy(address,bytes.data(),bytes.size()))return false;
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
      // 根内证明使用本根快照。对象外仍按原依赖顺序读取，指针或长度改变便停止沿旧地址查找。
      auto current=snapshot.Slice(range.address,range.size);
      if(current.empty()){
        const auto live=std::span(external).first(range.size);
        if(!services.reads->Probe(range.address,live))return false;
        current=live;
      }
      if(!std::equal(expected.begin(),expected.end(),current.begin()))return false;
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
  UseResourceScope(w);
  const auto category=unsigned(key.kind);
  const bool stable=key.kind==NativeResourceSource::Item || key.kind==NativeResourceSource::Config;
  const auto find=[&](auto end){
    return std::lower_bound(resource_memos.begin(),end,key,[](const auto &entry,const auto &wanted){return entry.key<wanted;});
  };
  // 外层需要完整输入证明时，从真实来源读一遍，或复用下面带完整证言的条目。
  bool recording_inputs=false;
  for(auto *record=input_recorder;record;record=record->parent)recording_inputs|=record->input.evidence.valid;
  const bool animation=key.kind==NativeResourceSource::Animation;
  const bool shared=animation?(resource_cache.observed&1u):key.kind==NativeResourceSource::Image&&(resource_cache.observed&2u);
  if(window.synchronous_root&&!recording_inputs&&shared){
    auto &table=animation?resource_cache.animations:resource_cache.images;
    const auto epoch=animation?resource_cache.animation_epoch:resource_cache.image_epoch;
    if(!table.empty()){
      auto &memo=table[(std::uint64_t(key.address>>2)*137u+key.dependency*79u)%table.size()];
      if(memo.epoch==epoch&&memo.address==key.address&&memo.file==key.dependency){
        out=memo.link;++stats.resource_shared_hits[category];return true;
      }
      ++stats.resource_parses[category];
      if(!parse(context))return false;
      // 只有文件图像进共享表；程序图集类资源仍按调用点的实际 owner 解析。
      if(animation||(out.kind==CapturedReferenceKind::Identity&&resources[std::size_t(out.handle-1)].kind==1)){
        if(memo.epoch!=epoch)CountCacheInsert(animation?2:3);
        memo={epoch,key.dependency,key.address,out};
      }
      return true;
    }
  }
  if(stable&&!recording_inputs){
    const auto end=resource_memos.begin()+memo_used;
    const auto at=find(end);
    if(at!=end&&at->key==key){out=at->link;++stats.resource_hits[category];++stats.resource_shared_hits[category];return true;}
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
      // 原解析登记了结构证言时照常走该解析入口，保留同址投影比较及需要时的根尾复读。
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
    CountCacheInsert(4);
  }
  return true;
}
void EntitySource::ReserveReuse(std::uint64_t bytes) noexcept {
  auto &s=*impl_;
  bytes-=s.ReserveResourceCaches(bytes);
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
