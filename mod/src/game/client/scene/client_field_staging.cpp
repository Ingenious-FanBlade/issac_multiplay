#include "game/client/scene/client_field_staging.h"
#include <algorithm>
#include <cstring>
#include <map>
#include <tuple>
#include <thread>

namespace cs::game {
namespace {
namespace S=state;
using Kind=ClientStagedFieldKind;
[[noreturn]]void Invalid(const char *why){throw S::ClientShadowError(S::StateReadiness::InvalidEncoding,why);}
bool Sprite(unsigned id){switch(id){case 7:case 134:case 144:case 201:case 202:case 204:case 215:case 226:case 281:case 282:case 355:case 434:return true;default:return false;}}
bool Shared(unsigned id){return id==18||id==19||id==25||id==27||id==28||id==133;}
// ABI 选择只来自可信生成表及具名原生类型；不能由报文传入函数、偏移或分配大小。
ClientStagedFieldInput DescribeField(const S::ClientInstallField &f,const S::Field &d) {
  S::BodySourceFieldInfo format;if(!S::DescribeBodySourceField(d.layout,d.index,format))Invalid("binary_fields_format");
  ClientStagedFieldInput in;in.layout=d.nested_layout;in.count=f.elements;in.stride=d.native_stride;
  in.slot_mask=f.slot_mask;in.present=f.present;in.payload_present=f.payload_present;
  const auto *nested=d.nested_layout?S::FindLayout(d.nested_layout):nullptr;
  if(d.nested_layout&&!nested)Invalid("binary_fields_nested_layout");
  const std::string_view handler=d.handler;
  // 这些数组的承载属于装载后的原生Sprite。只暂存数量，避免为每个层重复构造对象。
  if(handler=="read_anm2_layer_states"||handler=="read_grid_rock_layers"||handler=="read_room_wave_sprite_layers"||
     handler=="read_layer_frame_selectors"||handler=="read_null_layer_frame_selectors")return in;
  if(d.operation==S::Operation::Choice){in.kind=Kind::Weapons;return in;}
  if(format.shared_owner||(d.operation==S::Operation::Container&&Shared(d.nested_layout)&&d.limit==1&&d.native_size==8)){
    in.kind=Kind::Shared;in.stride=nested->native_size;return in;}
  if(d.nested_layout&&d.operation!=S::Operation::Container){if(Sprite(d.nested_layout)){in.kind=Kind::Rows;in.stride=nested->native_size;}
    else if(d.nested_layout==165){in.kind=Kind::Records;in.stride=32;}return in;}
  if(S::FindLayout(d.layout)->protocol&&d.source==S::FieldSource::Synthetic&&!d.native_size)return in;
  switch(format.storage){
    case S::BodyStorage::Vector:in.kind=Kind::Vector;break;
    case S::BodyStorage::String:in.kind=Kind::String;break;
    case S::BodyStorage::CString:in.kind=Kind::CString;break;
    case S::BodyStorage::Deque:in.kind=Kind::Deque;break;
    case S::BodyStorage::OrderedMap:if(d.nested_layout!=177)Invalid("binary_fields_map_layout");in.kind=Kind::Map;break;
    case S::BodyStorage::Set:if(d.layout!=170)Invalid("binary_fields_set_layout");in.kind=Kind::Blocked;break;
    case S::BodyStorage::Aggregate:
      if(d.layout==170&&d.nested_layout==178)in.kind=Kind::Innate;
      else if(d.layout==230&&d.nested_layout==238)in.kind=Kind::Seeds;
      // 纯显示协议的聚合没有可加偏移的原生容器；保留数字 Group 供具名显示 owner 消费。
      else if(d.source!=S::FieldSource::Synthetic)Invalid("binary_fields_aggregate_owner");break;
    case S::BodyStorage::Array:in.kind=(d.layout==133&&d.nested_layout==132)||(d.layout==27&&d.nested_layout==26)?Kind::Deque:Kind::Rows;break;
    default:Invalid("binary_fields_storage_owner");
  }
  if(in.kind==Kind::Rows&&d.source==S::FieldSource::Synthetic&&!d.native_stride)in.kind=Kind::Group;
  if((in.kind==Kind::Rows||in.kind==Kind::Vector||in.kind==Kind::Deque||in.kind==Kind::CString)&&(!in.stride||(nested&&in.stride<nested->native_size)))Invalid("binary_fields_native_stride");
  return in;
}
bool Same(const ClientStagedFieldInput &a,const ClientStagedFieldInput &b){return a.kind==b.kind&&a.layout==b.layout&&a.count==b.count&&a.stride==b.stride&&a.slot_mask==b.slot_mask&&a.present==b.present&&a.payload_present==b.payload_present;}
using PathKey=std::tuple<std::uint32_t,std::uint16_t,std::uint16_t,std::uint16_t,std::uint16_t>;
using FieldKey=std::pair<std::uint32_t,std::uint32_t>;
template<class K>using Index=std::map<K,std::uint32_t,std::less<K>,S::ClientStateAllocator<std::pair<const K,std::uint32_t>>>;
}
struct ClientSceneFieldStaging::Impl {
  struct Slot {
    ClientStagedFieldInput input;ClientStagedField *native=nullptr;
    S::ClientStateVector<std::byte> data;
    S::ClientStateVector<std::uint8_t> seen;
    explicit Slot(const std::shared_ptr<S::ClientStateMemory> &m):data(S::ClientStateAllocator<std::byte>(m)),seen(S::ClientStateAllocator<std::uint8_t>(m)){}
  };
  std::shared_ptr<S::ClientStateMemory> memory;ClientPreparationAllocator allocator;
  S::ClientStateVector<Slot> slots;
  S::ClientStateVector<std::uint32_t> fields;
  const S::ClientStateInstallPlan *plan=nullptr;
  const std::thread::id thread=std::this_thread::get_id();
  std::size_t cursor=0;bool described=false,visiting=false;
  Impl(std::shared_ptr<S::ClientStateMemory> m,ClientPreparationAllocator a):memory(std::move(m)),allocator(a),slots(S::ClientStateAllocator<Slot>(memory)),fields(S::ClientStateAllocator<std::uint32_t>(memory)){}
  bool Thread()const noexcept{return thread==std::this_thread::get_id();}
};
ClientSceneFieldStaging::ClientSceneFieldStaging(std::shared_ptr<S::ClientStateMemory> m,ClientPreparationAllocator a):impl_(std::make_unique<Impl>(std::move(m),a)){}
ClientSceneFieldStaging::~ClientSceneFieldStaging(){if(impl_&&(!impl_->Thread()||HasNative()))impl_.release();}
ClientSceneFieldStaging::ClientSceneFieldStaging(ClientSceneFieldStaging &&other) noexcept:impl_(std::move(other.impl_)){}
ClientSceneFieldStaging &ClientSceneFieldStaging::operator=(ClientSceneFieldStaging &&other) noexcept {
  if(this!=&other){if(impl_&&(!impl_->Thread()||HasNative()))impl_.release();impl_=std::move(other.impl_);}return *this;
}
void ClientSceneFieldStaging::Describe(const S::ClientStateInstallPlan &plan) {
  auto &s=*impl_;if(!s.Thread()||s.plan||!s.slots.empty()||plan.Memory()!=s.memory)Invalid("binary_fields_description_lifecycle");s.plan=&plan;
  using Pair=std::pair<const PathKey,std::uint32_t>;
  Index<PathKey> paths(S::ClientStateAllocator<Pair>(s.memory));
  Index<FieldKey> groups(S::ClientStateAllocator<std::pair<const FieldKey,std::uint32_t>>(s.memory));
  S::ClientStateVector<std::uint32_t> normalized(S::ClientStateAllocator<std::uint32_t>(s.memory));normalized.reserve(plan.Paths().size());
  const auto canonical=[](const S::CodecPathStep &p){S::BodySourceFieldInfo f;if(!S::DescribeBodySourceField(p.layout,p.field,f))Invalid("binary_fields_path_definition");return f.canonical;};
  const auto path_key=[&](std::uint32_t n,bool field){const auto &p=plan.Paths()[n];return PathKey{p.parent==S::kClientPlanNone?S::kClientPlanNone:normalized.at(p.parent),p.step.layout,canonical(p.step),field?S::kNullIndex:p.step.element,field?std::uint16_t(0):p.step.choice_layout};};
  // 同一投影的父链也须 canonical 化，且保留元素／槽位；不同 root 永不共享字段 owner。
  for(std::uint32_t n=0;n<plan.Paths().size();++n){const auto key=path_key(n,false);const auto [i,inserted]=paths.emplace(key,std::uint32_t(paths.size()));normalized.push_back(i->second);}
  s.fields.resize(plan.Fields().size(),S::kClientPlanNone);
  for(const auto &command:plan.Commands())if(command.phase==S::ClientInstallPhase::StageFields){
    const auto &f=plan.Fields()[command.index];const auto &record=plan.Records()[f.record];if(record.event!=S::kClientPlanNone||record.object==S::kClientPlanNone)Invalid("binary_fields_event_or_root");
    const auto &d=S::Fields()[f.definition];auto in=DescribeField(f,d);const FieldKey key{record.object,normalized.at(f.path)};
    const auto [i,inserted]=groups.emplace(key,std::uint32_t(s.slots.size()));
    if(inserted){s.slots.emplace_back(s.memory);auto &slot=s.slots.back();slot.input=in;
      const auto bytes=in.present?(in.kind==Kind::String||in.kind==Kind::CString?in.count:in.kind==Kind::Blocked||in.kind==Kind::Seeds?std::uint64_t(in.count)*4:in.kind==Kind::Map||in.kind==Kind::Innate?std::uint64_t(in.count)*8:0):0;
      if(bytes>UINT32_MAX)Invalid("binary_fields_data_budget");slot.data.resize(std::size_t(bytes));slot.seen.resize(std::size_t(bytes));}
    else if(!Same(s.slots[i->second].input,in))Invalid("binary_fields_projection_shape_conflict");
    s.fields[command.index]=i->second;
  }
  const auto group=[&](std::uint32_t object,std::uint32_t path)->Impl::Slot *{if(path==S::kClientPlanNone)return nullptr;
    const auto p=paths.find(path_key(path,true));if(p==paths.end())return nullptr;const auto i=groups.find({object,p->second});return i==groups.end()?nullptr:&s.slots[i->second];};
  const auto put=[](Impl::Slot &slot,std::size_t offset,std::span<const std::byte> bytes){if(offset>slot.data.size()||bytes.size()>slot.data.size()-offset)Invalid("binary_fields_leaf_range");
    for(std::size_t j=0;j<bytes.size();++j){if(slot.seen[offset+j]&&slot.data[offset+j]!=bytes[j])Invalid("binary_fields_projection_value_conflict");slot.seen[offset+j]=1;slot.data[offset+j]=bytes[j];}};
  // 字符串／键和值直接取安装计划的标量叶。没有读取长度头、变长记录或重跑 codec。
  for(const auto &v:plan.Values()){const auto &record=plan.Records()[v.record];if(record.event!=S::kClientPlanNone||v.path==S::kClientPlanNone||v.layout)continue;
    const auto &p=plan.Paths()[v.path];const auto bytes=plan.Bytes(v.bytes);
    if(auto *slot=group(record.object,v.path);slot&&(slot->input.kind==Kind::String||slot->input.kind==Kind::CString||slot->input.kind==Kind::Blocked)){
      const auto width=slot->input.kind==Kind::Blocked?4u:1u;if(bytes.size()!=width||p.step.element==S::kNullIndex)Invalid("binary_fields_direct_leaf");put(*slot,std::size_t(p.step.element)*width,bytes);continue;}
    if(p.parent==S::kClientPlanNone)continue;auto *slot=group(record.object,p.parent);if(!slot)continue;const auto &parent=plan.Paths()[p.parent].step;
    if(slot->input.kind==Kind::Weapons&&p.step.field==0&&p.step.layout>=181&&p.step.layout<=194){
      if(parent.element>=5||parent.choice_layout!=p.step.layout||bytes.size()!=4||!(slot->input.slot_mask&(1u<<parent.element)))Invalid("binary_fields_weapon_leaf");
      std::uint32_t type;std::memcpy(&type,bytes.data(),4);const auto *layout=S::FindLayout(p.step.layout);auto &weapon=slot->input.weapons[parent.element];
      if(weapon.type&&(weapon.type!=type||weapon.layout!=p.step.layout))Invalid("binary_fields_weapon_projection_conflict");weapon={p.step.layout,type,layout->native_size};
    }else if((slot->input.kind==Kind::Map&&p.step.layout==177)||(slot->input.kind==Kind::Innate&&p.step.layout==178)||(slot->input.kind==Kind::Seeds&&p.step.layout==238)){
      const auto width=slot->input.kind==Kind::Seeds?4u:8u;if(parent.element==S::kNullIndex||bytes.size()!=4||p.step.field>=width/4)Invalid("binary_fields_key_leaf");
      put(*slot,std::size_t(parent.element)*width+p.step.field*4,bytes);
    }
  }
  for(auto &slot:s.slots){if(std::find(slot.seen.begin(),slot.seen.end(),0)!=slot.seen.end())Invalid("binary_fields_key_or_text_missing");
    if(slot.input.kind==Kind::Weapons)for(unsigned i=0;i<5;++i)if((slot.input.slot_mask&(1u<<i))&&!slot.input.weapons[i].type)Invalid("binary_fields_weapon_type_missing");
    slot.input.data=slot.data.data();slot.input.bytes=slot.data.size();slot.seen.clear();}
  s.described=true;
}
ClientPreparationResult ClientSceneFieldStaging::Stage(unsigned maximum) noexcept {
  auto &s=*impl_;if(!s.Thread()||!s.described||!maximum||s.visiting)return {ClientPreparationStatus::Failed,"binary_fields_not_described"};
  unsigned work=0;while(work++<maximum&&s.cursor<s.slots.size()){auto &slot=s.slots[s.cursor];
    if(!slot.native)slot.native=ClientStagedFieldCreate(s.allocator);if(!slot.native)return {ClientPreparationStatus::Failed,"binary_fields_owner_allocation"};
    const auto result=ClientStagedFieldConstruct(slot.native,slot.input);if(result.status!=ClientPreparationStatus::Ready)return result;++s.cursor;}
  return s.cursor==s.slots.size()?ClientPreparationResult{ClientPreparationStatus::Ready,"binary_fields_staged"}:ClientPreparationResult{ClientPreparationStatus::Waiting,"binary_fields_staging"};
}
bool ClientSceneFieldStaging::Ready()const noexcept {const auto &s=*impl_;return s.Thread()&&s.described&&s.cursor==s.slots.size()&&std::all_of(s.slots.begin(),s.slots.end(),[](const auto &slot){return ClientStagedFieldCurrent(slot.native);});}
bool ClientSceneFieldStaging::HasNative()const noexcept{return impl_->Thread()&&std::any_of(impl_->slots.begin(),impl_->slots.end(),[](const auto &slot){return slot.native;});}
bool ClientSceneFieldStaging::Drain() noexcept {
  auto &s=*impl_;if(!s.Thread()||s.visiting)return false;for(std::size_t i=s.slots.size();i>0;--i)if(!ClientStagedFieldRelease(s.slots[i-1].native))return false;
  s.slots.clear();s.fields.clear();s.plan=nullptr;s.cursor=0;s.described=false;return true;
}
bool ClientSceneFieldStaging::Visit(void *context,Visitor visit) noexcept {
  auto &s=*impl_;if(!s.Thread()||!visit||s.visiting||!Ready())return false;s.visiting=true;bool ok=true;
  for(std::uint32_t f=0;f<s.fields.size();++f)if(s.fields[f]!=S::kClientPlanNone){auto &slot=s.slots[s.fields[f]];if(!ClientStagedFieldCurrent(slot.native)||!visit(context,f,slot.input,slot.native)){ok=false;break;}}
  s.visiting=false;return ok;
}
} // namespace cs::game
