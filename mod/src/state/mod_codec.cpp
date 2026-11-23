#include "state/mod_codec.h"
#include "state/wire_protocol.h"
#include "core/deferred_values.h"
#include <algorithm>
#include <bit>
#include <set>

namespace cs::state {
// 线格式统一小端。ModRead／ModWrite 读写 1—8 字节无符号整数，不依赖本机字节序和对齐。
std::uint64_t ModRead(const std::byte *bytes,std::size_t size) noexcept {
  std::uint64_t value=0;
  for(std::size_t i=0;i<size;++i)value|=std::uint64_t(std::to_integer<unsigned>(bytes[i]))<<(8*i);
  return value;
}
void ModWrite(std::byte *bytes,std::size_t size,std::uint64_t value) noexcept {
  for(std::size_t i=0;i<size;++i)bytes[i]=std::byte(value>>(8*i));
}
// 两个值相同：同一指针，或都非空且字节完全相等。一方为空（无记录）时视为不同。
bool ModSame(const ModValueRef &a,const ModValueRef &b) noexcept {
  return a==b||(a&&b&&a->bytes.size()==b->bytes.size()&&
    (a->bytes.empty()||!std::memcmp(a->bytes.data(),b->bytes.data(),a->bytes.size())));
}
namespace {
// 记账用的每条 map 记录估算：键值对本身加红黑树节点的指针与颜色开销。
constexpr auto kMapEntry=sizeof(std::pair<const ModRecordKey,ModRecord>)+4*sizeof(void *);
using ModBytes=std::span<const std::byte>;
using Kind=ModNodeKind;

// 复制字节为新的不可变值，并按实际字节数记入 Values 池。
ModValueRef Own(ModBytes bytes) {
  auto result=memory::DeferredValue<ModValue>();
  result->memory=memory::Reserve(memory::Pool::Values,sizeof(ModValue)+bytes.size(),"mod_value");
  result->bytes.assign(bytes.begin(),bytes.end());
  return result;
}
std::uint64_t Read(ModBytes bytes) noexcept {return ModRead(bytes.data(),bytes.size());}
// map 键的规范顺序，Host 排序与客机校验共用。字符串键为"u16 长度＋字节"，按无符号字节序比较；
// 整数键为 4 字节，按有符号 i32 比较。a、b 只需从键起点开始，尾部多余字节不参与比较。
bool KeyLess(const ModNode &node,ModBytes a,ModBytes b) noexcept {
  if(node.string_key){
    a=a.subspan(2,std::size_t(Read(a.first(2))));b=b.subspan(2,std::size_t(Read(b.first(2))));
    return std::lexicographical_compare(a.begin(),a.end(),b.begin(),b.end());
  }
  return std::bit_cast<std::int32_t>(std::uint32_t(Read(a.first(4))))<
         std::bit_cast<std::int32_t>(std::uint32_t(Read(b.first(4))));
}

// 生成规范字节：先把每个实体引用槽（2 字节）写成 lookup 给出的成员号，再把 map 项按键排序。
// 原始值中 maps 按前序登记（外层先于内层），倒序遍历即先排内层；每个 map 项是"键＋元素"的
// 连续区间，整段搬动不改变项长，外层项记下的区间因此仍然有效。
void CanonicalValue(const ModRawValue &source,std::vector<std::byte> &bytes,void *context,
    ModMemberNumber lookup,std::uint64_t &missing) {
  bytes=source.bytes;
  for(const auto &ref:source.references){
    const auto index=lookup?lookup(context,{ref.id,ref.use}):kModGlobal;
    if(index==kModGlobal)++missing;
    ModWrite(bytes.data()+ref.offset,2,index);
  }
  std::vector<std::byte> sorted;
  // 子 map 先排序，外层各项的原长度和偏移保持有效。
  for(auto map_it=source.maps.rbegin();map_it!=source.maps.rend();++map_it){
    const auto &map=*map_it;
    if(map.items.size()<2)continue;
    const auto &node=source.layout->nodes[map.node];
    auto items=map.items;
    std::sort(items.begin(),items.end(),[&](auto a,auto b){
      return KeyLess(node,ModBytes(bytes).subspan(a.first,a.last-a.first),ModBytes(bytes).subspan(b.first,b.last-b.first));
    });
    sorted.clear();
    for(auto item:items)sorted.insert(sorted.end(),bytes.begin()+item.first,bytes.begin()+item.last);
    std::copy(sorted.begin(),sorted.end(),bytes.begin()+map.items.front().first);
  }
}

// 定长槽和尾部游标分离；子结构的槽在父结构内，容器元素则拥有自己的定长区。
// 按前序节点 i 校验一个值：struct 开头是字段存在位图，未用的高位与缺席字段的槽必须全零；
// bool 只取 0/1，player 只取 0—3 或 0xFF；string／list／map 的计数不超过声明 max，
// 正文从尾部游标依次取出；map 键须严格递增（不允许重复），字符串键长不超过 keymax。
// 其余标量不查内容（浮点可为任意位型），实体成员号在安装时再解析。
bool Validate(const std::vector<ModNode> &nodes,unsigned i,ModBytes fixed,ModBytes &tail) {
  const auto &node=nodes[i];
  if(node.kind==Kind::Struct){
    const auto bits=(node.children+7)/8;
    if(node.children%8 && Read(fixed.subspan(bits-1,1))>> (node.children%8))return false;
    auto child=i+1;
    for(unsigned n=0;n<node.children;++n){
      const auto &field=nodes[child];const auto slot=fixed.subspan(field.off,field.fixed);
      if(Read(fixed.subspan(n/8,1))&(std::uint64_t{1}<<(n%8))){
        if(!Validate(nodes,child,slot,tail))return false;
      }else if(std::any_of(slot.begin(),slot.end(),[](auto byte){return byte!=std::byte{};}))return false;
      child=field.end;
    }
    return true;
  }
  if(node.kind==Kind::Bool)return Read(fixed)<=1;
  if(node.kind==Kind::Player){const auto ordinal=Read(fixed);return ordinal<4||ordinal==kModPublic;}
  if(node.kind!=Kind::String&&node.kind!=Kind::List&&node.kind!=Kind::Map)return true;
  const auto count=Read(fixed);
  if(count>node.max)return false;
  if(node.kind==Kind::String){
    if(count>tail.size())return false;
    tail=tail.subspan(std::size_t(count));return true;
  }
  const auto &element=nodes[i+1];
  ModBytes previous;
  for(std::size_t n=0;n<count;++n){
    if(node.kind==Kind::Map){
      auto size=4u;
      if(node.string_key){
        if(tail.size()<2)return false;
        const auto length=Read(tail.first(2));
        if(length>node.keymax)return false;
        size=2+unsigned(length);
      }
      if(size>tail.size())return false;
      const auto key=tail.first(size);tail=tail.subspan(size);
      if(n&&!KeyLess(node,previous,key))return false;
      previous=key;
    }
    if(element.fixed>tail.size())return false;
    const auto slot=tail.first(element.fixed);tail=tail.subspan(element.fixed);
    if(!Validate(nodes,i+1,slot,tail))return false;
  }
  return true;
}
// 整条值：空字节表示顶层 nil（合法）；否则长度在 [根定长, 最坏长度] 内，校验后尾部须恰好用完。
bool ValidValue(const ModLayout &layout,ModBytes bytes) {
  if(bytes.empty())return true;
  const auto &root=layout.nodes.front();
  if(bytes.size()<root.fixed||bytes.size()>root.worst)return false;
  auto tail=bytes.subspan(root.fixed);
  return Validate(layout.nodes,0,bytes.first(root.fixed),tail)&&tail.empty();
}
}

bool ValidModBytes(const ModLayout &layout,const std::byte *bytes,std::size_t size) {
  return !layout.nodes.empty()&&ValidValue(layout,{bytes,size});
}
void CanonicalModValue(const ModRawValue &source,std::vector<std::byte> &bytes,void *context,
    ModMemberNumber lookup,std::uint64_t &missing) {
  CanonicalValue(source,bytes,context,lookup,missing);
}

std::shared_ptr<ModSnapshot> CopyModSnapshot(const ModShadowRef &base,std::size_t extra) {
  auto next=memory::DeferredValue<ModSnapshot>();
  next->memory=memory::Reserve(memory::Pool::Values,sizeof(ModSnapshot)+kMapEntry*(extra+(base?base->records.size():0)),"mod_snapshot");
  if(base)next->records=base->records;
  return next;
}

ModShadowRef EncodeModValues(const ModRawRef &raw,const ModShadowRef &before,std::span<const StateMemberEntry> members,
    std::uint64_t version,std::uint32_t member_epoch,std::uint64_t &missing) {
  if(!raw)return before;
  if(raw->records.empty())return {};
  auto next=CopyModSnapshot({},raw->records.size());
  std::map<std::pair<std::uint64_t,std::uint64_t>,std::uint16_t> entities;
  auto work=memory::Reserve(memory::Pool::Workspace,members.size()*64+8*kModValueBytes,"mod_encode");
  for(const auto &member:members)if(member.identity.space==1)
    entities.emplace(std::make_pair(member.identity.id,member.identity.use),member.index);
  std::vector<std::byte> bytes;
  for(const auto &[raw_key,source]:raw->records){
    // 实体记录以物理身份暂存，这里换成同帧成员号作为线上目标；不在目录的实体本帧不发。
    auto key=raw_key.record;
    if(raw_key.entity.id){
      const auto target=entities.find({raw_key.entity.id,raw_key.entity.use});
      if(target==entities.end())continue;
      key.target=target->second;
    }
    const ModRecord *previous=nullptr;
    if(before){
      const auto old=before->records.find(key);
      if(old!=before->records.end()&&old->second.entity==raw_key.entity)previous=&old->second;
    }
    // 同一采样且引用所依赖的成员目录代次未变时，规范字节必然相同，跳过重算与比较。
    if(previous&&previous->sample==source->sample&&
       (source->references.empty()||previous->member_epoch==member_epoch)){
      next->records.emplace(key,*previous);continue;
    }
    CanonicalModValue(*source,bytes,&entities,[](void *context,ModEntityId id) noexcept {
      const auto &directory=*static_cast<decltype(entities) *>(context);
      const auto entry=directory.find({id.id,id.use});
      return entry==directory.end()?kModGlobal:entry->second;
    },missing);
    const bool same=previous&&previous->value->bytes.size()==bytes.size()&&
      (bytes.empty()||!std::memcmp(previous->value->bytes.data(),bytes.data(),bytes.size()));
    const auto receiver=source->layout->private_value?std::uint8_t(key.target):kModPublic;
    next->records.emplace(key,ModRecord{same?previous->value:Own(bytes),same?previous->changed:version,
      receiver,source->sample,member_epoch,raw_key.entity});
  }
  return next;
}

void AppendModRecord(std::vector<std::byte> &out,ModRecordKey key,const ModRecord &record) {
  const auto at=out.size();out.resize(at+kModRecordHeaderBytes);
  auto *head=out.data()+at;
  ModWrite(head,2,key.mod);ModWrite(head+2,2,key.declaration);ModWrite(head+4,2,key.target);
  ModWrite(head+6,1,record.receiver);ModWrite(head+7,4,record.value->bytes.size());
  out.insert(out.end(),record.value->bytes.begin(),record.value->bytes.end());
}
void AppendModResult(std::vector<std::byte> &out,const net::ModIntentResult &result) {
  const auto at=out.size();out.resize(at+kModResultHeaderBytes);
  auto *head=out.data()+at;
  ModWrite(head,8,result.id);ModWrite(head+8,2,result.mod);ModWrite(head+10,2,result.declaration);
  ModWrite(head+12,1,result.ok);ModWrite(head+13,2,result.size);
  out.insert(out.end(),result.bytes.begin(),result.bytes.begin()+result.size);
}
bool ReadModResults(std::span<const std::byte> bytes,net::ModIntentResults &out) noexcept {
  out.count=0;
  std::uint64_t previous=0;
  while(!bytes.empty()){
    if(bytes.size()<kModResultHeaderBytes||out.count==out.values.size())return false;
    auto &result=out.values[out.count];
    result.id=Read(bytes.first(8));result.mod=std::uint16_t(Read(bytes.subspan(8,2)));
    result.declaration=std::uint16_t(Read(bytes.subspan(10,2)));
    const auto ok=Read(bytes.subspan(12,1)),size=Read(bytes.subspan(13,2));
    bytes=bytes.subspan(kModResultHeaderBytes);
    if(result.id<=previous||ok>1||size>result.bytes.size()||size>bytes.size())return false;
    result.ok=ok!=0;result.size=std::uint16_t(size);previous=result.id;
    std::copy_n(bytes.begin(),result.size,result.bytes.begin());bytes=bytes.subspan(result.size);++out.count;
  }
  return true;
}

// 规则见头文件。members 是本帧成员目录（按成员号升序），players 是本帧名册的参与者位图。
ModShadowRef MergeModShadow(const ModShadowRef &prior,const ModSchema &schema,ModBytes bytes,
    unsigned receiver,unsigned players,std::span<const StateMemberEntry> members,bool full,std::uint64_t &rejected) {
  // 成员号 → 实体物理身份；不在目录或不是实体时返回空身份。
  const auto entity=[&](std::uint16_t index)->ModEntityId{
    const auto at=std::lower_bound(members.begin(),members.end(),index,[](const auto &m,auto i){return m.index<i;});
    return at!=members.end()&&at->index==index&&at->identity.space==1?ModEntityId{at->identity.id,at->identity.use}:ModEntityId{};
  };
  // 增量帧唯一的删除来源：实体记录的成员号已换人或离开，参与者记录的序号已不在名册。
  const auto gone=[&](const auto &entry){
    const auto target=entry.first.target;
    if(entry.second.entity.id)return !(entry.second.entity==entity(target));
    return target<4&&!(players&(1u<<target));
  };
  if(bytes.empty()&&(!prior||(!full&&std::none_of(prior->records.begin(),prior->records.end(),[&](const auto &entry){
    return gone(entry);
  }))))return prior;
  auto next=CopyModSnapshot(full?ModShadowRef{}:prior,
    (std::min)(bytes.size()/kModRecordHeaderBytes,schema.layouts.size()*(4+members.size())));
  constexpr std::size_t rejection_bytes=48;
  const auto previous_count=prior?prior->rejected_by_mod.size():0;
  next->memory->Resize(next->memory->Bytes()+rejection_bytes*(std::min)(schema.layouts.size(),previous_count+bytes.size()/kModRecordHeaderBytes));
  if(prior)next->rejected_by_mod=prior->rejected_by_mod;
  // 拒绝计数；只有声明表里确有该 Mod 时才按 Mod 归账，伪造的 Mod 序号不会撑大诊断表。
  const auto reject=[&](std::uint16_t mod){
    ++rejected;
    const auto first=schema.layouts.lower_bound({mod,0,kModGlobal});
    if(first!=schema.layouts.end()&&first->first.mod==mod)++next->rejected_by_mod[mod];
  };
  std::optional<ModRecordKey> previous;
  bool truncated=false;
  while(bytes.size()>=kModRecordHeaderBytes){
    const ModRecordKey key{std::uint16_t(Read(bytes.first(2))),std::uint16_t(Read(bytes.subspan(2,2))),
      std::uint16_t(Read(bytes.subspan(4,2)))};
    const auto ordinal=Read(bytes.subspan(6,1)),size=Read(bytes.subspan(7,4));
    bytes=bytes.subspan(kModRecordHeaderBytes);
    if(size>bytes.size()){reject(key.mod);truncated=true;break;}
    const auto value=bytes.first(std::size_t(size));bytes=bytes.subspan(std::size_t(size));
    const auto declaration=schema.layouts.find({key.mod,key.declaration,kModGlobal});
    const auto *layout=declaration==schema.layouts.end()?nullptr:declaration->second.get();
    const auto identity=layout&&layout->entity?entity(key.target):ModEntityId{};
    const bool valid_target=layout&&(layout->entity?identity.id!=0:
      layout->player?(key.target<4&&(players&(1u<<key.target))):key.target==kModGlobal);
    const auto expected=valid_target&&layout->private_value?std::uint8_t(key.target):kModPublic;
    if(!valid_target||(expected!=kModPublic&&expected!=receiver)){
      reject(key.mod);continue;
    }
    ModValueRef old;
    if(prior){const auto it=prior->records.find(key);if(it!=prior->records.end()&&it->second.entity==identity)old=it->second.value;}
    if((previous&&!(*previous<key))||ordinal!=expected||!ValidValue(*layout,value)){
      reject(key.mod);
      if(old)next->records.try_emplace(key,ModRecord{std::move(old),0,expected,0,0,identity});
      continue;
    }
    previous=key;
    if(!old||!std::equal(value.begin(),value.end(),old->bytes.begin(),old->bytes.end()))old=Own(value);
    next->records.insert_or_assign(key,ModRecord{std::move(old),0,expected,0,0,identity});
  }
  if(truncated||!bytes.empty()){
    if(!truncated)++rejected;
    // 后续记录边界未知；已验证的前缀仍可合并，未读到的旧值沿用。
    if(full&&prior)next->records.insert(prior->records.begin(),prior->records.end());
  }
  std::erase_if(next->records,gone);
  next->memory->Resize(sizeof(ModSnapshot)+kMapEntry*next->records.size()+rejection_bytes*next->rejected_by_mod.size());
  return next;
}
}
