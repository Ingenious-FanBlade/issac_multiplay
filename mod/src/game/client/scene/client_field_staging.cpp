#include "game/client/scene/client_field_staging.h"
#include "state/client_memory_resource.h"
#include <algorithm>
#include <cstring>
#include "game/client/scene/client_exact_index.h"
#include <tuple>
#include <thread>

namespace cs::game {
namespace {
namespace S=state;
using Kind=ClientStagedFieldKind;
#include "state_host_fields.inc"
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
bool Same(const ClientStagedFieldInput &a,const ClientStagedFieldInput &b){
  if(a.kind!=b.kind||a.layout!=b.layout||a.count!=b.count||a.stride!=b.stride||
      a.slot_mask!=b.slot_mask||a.present!=b.present||a.payload_present!=b.payload_present)return false;
  // 武器槽即使大小相同，也必须是同一原生构造类型才准借用当前承载。
  return a.kind!=Kind::Weapons||std::equal(a.weapons.begin(),a.weapons.end(),b.weapons.begin(),
    [](const auto &x,const auto &y){return x.layout==y.layout&&x.type==y.type&&x.size==y.size;});
}
// 固定内嵌组和 Rows 的实际后备存储归根对象，暂存只保留定界信息。
bool Inline(Kind kind){return kind==Kind::Group||kind==Kind::Rows;}
using FieldKey=std::pair<std::uint32_t,std::uint32_t>;
template<class K>using Index=ClientExactIndex<K>;
}
struct ClientSceneFieldStaging::Impl {
  struct Slot {
    ClientStagedFieldInput input;ClientStagedField *native=nullptr;
    S::ClientStateVector<std::byte> data;
    S::ClientStateVector<std::uint8_t> seen;
    S::ClientStateVector<S::CodecPathStep> key;
    // 精确原生成员及规范字段号，按涉及的两流块核内容与关系。
    std::array<std::uint32_t,2> blocks{},structure{};
    std::uint32_t object=S::kClientPlanNone,parent=S::kClientPlanNone,owner_object=S::kClientPlanNone;
    std::uint8_t streams=0;
    bool eligible=true,costume=false,value_only=false,live_sequence=false,excluded=false;
    explicit Slot(const std::shared_ptr<S::ClientStateMemory> &m):data(S::ClientStateAllocator<std::byte>(m)),seen(S::ClientStateAllocator<std::uint8_t>(m)),
      key(S::ClientStateAllocator<S::CodecPathStep>(m)){}
  };
  std::shared_ptr<S::ClientStateMemory> memory;ClientPreparationAllocator allocator;
  S::ClientStateVector<Slot> slots;
  S::ClientStateVector<std::uint32_t> fields;
  S::ClientStateVector<std::uint32_t> reuse_order;
  const S::ClientStateInstallPlan *plan=nullptr;
  const std::thread::id thread=std::this_thread::get_id();
  std::size_t cursor=0;bool described=false,visiting=false;
  Impl(std::shared_ptr<S::ClientStateMemory> m,ClientPreparationAllocator a):memory(std::move(m)),allocator(a),slots(S::ClientStateAllocator<Slot>(memory)),fields(S::ClientStateAllocator<std::uint32_t>(memory)),
    reuse_order(S::ClientStateAllocator<std::uint32_t>(memory)){}
  bool Thread()const noexcept{return thread==std::this_thread::get_id();}
  const S::PresentationMember &Identity(const Slot &slot)const noexcept {return plan->Objects()[slot.object].member.identity;}
};
ClientSceneFieldStaging::ClientSceneFieldStaging(std::shared_ptr<S::ClientStateMemory> m,ClientPreparationAllocator a):impl_(std::make_unique<Impl>(std::move(m),a)){}
ClientSceneFieldStaging::~ClientSceneFieldStaging(){if(impl_&&(!impl_->Thread()||HasNative()))impl_.release();}
ClientSceneFieldStaging::ClientSceneFieldStaging(ClientSceneFieldStaging &&other) noexcept:impl_(std::move(other.impl_)){}
ClientSceneFieldStaging &ClientSceneFieldStaging::operator=(ClientSceneFieldStaging &&other) noexcept {
  if(this!=&other){if(impl_&&(!impl_->Thread()||HasNative()))impl_.release();impl_=std::move(other.impl_);}return *this;
}
void ClientSceneFieldStaging::Describe(const S::ClientStateInstallPlan &plan) {
  auto &s=*impl_;if(!s.Thread()||s.plan||!s.slots.empty()||plan.Memory()!=s.memory)Invalid("binary_fields_description_lifecycle");s.plan=&plan;
  // 临时索引整块分配并计入原Values账本；返回时统一释放，不逐节点争用Peer锁。
  S::ClientStateMemoryResource upstream(s.memory,memory::Pool::Values);
  std::pmr::monotonic_buffer_resource arena(8192,&upstream);
  Index<FieldKey> groups(&arena);
  // 原生槽只来自StageFields；普通标量没有槽，不为它们分配空索引空间。
  const auto stage_count=plan.StageFields().size();
  groups.reserve(stage_count);s.slots.reserve(stage_count);
  // 后台已按完整父链封存规范编号；主线程只索引实际需要原生暂存的字段。
  // 数字路径只标记本循环实际消费的叶类型；不同对象仍通过完整(object,path)找槽。
  // 该表是保守并集，只省略不可能承载字符串／键值／Weapon数据的无效查询。
  std::pmr::vector<std::uint8_t> leaf_roles(&arena);leaf_roles.resize(plan.NormalizedPathCount());
  s.fields.resize(plan.FieldCount(),S::kClientPlanNone);
  for(auto field:plan.StageFields()){
    const auto &f=plan.Field(field);const auto &record=plan.Records()[f.record];if(record.event!=S::kClientPlanNone||record.object==S::kClientPlanNone)Invalid("binary_fields_event_or_root");
    const auto &d=S::Fields()[f.definition];auto in=DescribeField(f,d);const FieldKey key{record.object,plan.Paths()[f.path].normalized};
    if(in.kind==Kind::String||in.kind==Kind::CString||in.kind==Kind::Blocked)leaf_roles[key.second]|=1;
    if(in.kind==Kind::Weapons||in.kind==Kind::Map||in.kind==Kind::Innate||in.kind==Kind::Seeds)leaf_roles[key.second]|=2;
    const auto [i,inserted]=groups.emplace(key,std::uint32_t(s.slots.size()));
    if(inserted){s.slots.emplace_back(s.memory);auto &slot=s.slots.back();slot.input=in;slot.owner_object=record.object;
      // 专用文本写入器消费有界字节并自行保存撤销值，不使用通用原生字符串附件。
      slot.value_only=std::string_view(d.handler)=="read_anm2_default_name"||std::string_view(d.handler)=="read_text_bytes";
      slot.costume=in.kind==Kind::Vector&&plan.Paths()[f.path].parent==S::kClientPlanNone&&S::ClientCostumeProjection(d.layout,d.index);
      const auto bytes=in.present?(in.kind==Kind::String||in.kind==Kind::CString?in.count:in.kind==Kind::Blocked||in.kind==Kind::Seeds?std::uint64_t(in.count)*4:in.kind==Kind::Map||in.kind==Kind::Innate?std::uint64_t(in.count)*8:0):0;
      if(bytes>UINT32_MAX)Invalid("binary_fields_data_budget");slot.data.resize(std::size_t(bytes));slot.seen.resize(std::size_t(bytes));}
    else if(!Same(s.slots[i->second].input,in))Invalid("binary_fields_projection_shape_conflict");
    s.fields[field]=i->second;
    s.slots[i->second].streams|=std::uint8_t(1u<<unsigned(record.stream));
  }
  // 父槽已先由 StageFields 清单登记。路径含动态父槽时，只能借同一个仍接入的父承载。
  for(auto field:plan.StageFields()){
    const auto &f=plan.Field(field);const auto &r=plan.Records()[f.record];const auto &d=S::Fields()[f.definition];
    auto &slot=s.slots[s.fields[field]];
    if(!slot.eligible)continue;
    auto path=f.path;auto parent=S::kClientPlanNone;
    std::array<S::CodecPathStep,S::kCodecMaxDepth> steps{};
    std::size_t length=0;
    const S::Field *root=nullptr;
    for(;path!=S::kClientPlanNone;path=plan.Paths()[path].parent){
      const auto &step=plan.Paths()[path].step;
      root=S::FindField(step.layout,step.field);
      if(!root||length==steps.size()||root->source!=S::FieldSource::Object||S::FindLayout(step.layout)->protocol){slot.eligible=false;break;}
      const std::string_view handler=root->handler;
      if(handler.starts_with("read_existing_")||handler=="read_anm2_layer_states"||handler=="read_grid_rock_layers"||
          handler=="read_room_wave_sprite_layers"||handler=="player_existing_spoofs_members"){slot.eligible=false;break;}
      steps[length++]=step;steps[length-1].field=plan.Paths()[path].canonical;
      if(path!=f.path&&parent==S::kClientPlanNone){
        const auto canonical_path=plan.Paths()[path].field_path;
        if(canonical_path==S::kClientPlanNone){slot.eligible=false;break;}
        const auto enclosing=groups.find({r.object,canonical_path});
        if(enclosing==groups.end()){slot.eligible=false;break;}
        const auto kind=s.slots[enclosing->second].input.kind;
        if(kind!=Kind::Group&&kind!=Kind::Rows)parent=enclosing->second;
      }
    }
    if(!slot.eligible||!root)continue;
    if(parent!=S::kClientPlanNone&&parent>=s.fields[field]){slot.eligible=false;continue;}
    S::BodySourceFieldInfo format;
    if(!S::DescribeBodySourceField(d.layout,d.index,format))Invalid("binary_fields_reuse_format");
    const auto bit=std::uint32_t(1)<<root->block;const auto stream=unsigned(r.stream);
    slot.blocks[stream]|=bit;
    // 标量 vector 按位置寻址。服装以 World 目录顺序识别元素，Body 可原位改姿态。
    const bool scalar_vector=slot.input.kind==Kind::Vector&&!format.linked&&
      ClientLiveVectorLayout(d.nested_layout,slot.input.stride);
    // deque 的 EntityPtr 用原生构造和析构维护反向树；其余列出的元素是原生值。
    slot.live_sequence|=scalar_vector||slot.input.kind==Kind::Records||(slot.input.kind==Kind::Deque&&
      (d.nested_layout==host_field::Vector::kLayout||d.nested_layout==host_field::AfterImageFrame::kLayout||
       d.nested_layout==host_field::EffectTrailNode::kLayout||d.nested_layout==host_field::EntityPtr::kLayout));
    // 文本和键值容器的完整构造输入在 data 中逐字节核对，旁支标量变化不使其失效。
    const auto kind=slot.input.kind;
    const bool owns_data=kind==Kind::String||kind==Kind::CString||kind==Kind::Map||kind==Kind::Innate||
      kind==Kind::Seeds||kind==Kind::Blocked;
    if(!scalar_vector&&!slot.costume&&!owns_data)slot.structure[stream]|=bit;
    const auto first=std::make_reverse_iterator(steps.begin()+length);
    const auto end=first+length;
    if(slot.object==S::kClientPlanNone){
      slot.object=r.object;slot.parent=parent;slot.key.assign(first,end);
    }else if(slot.parent!=parent||!std::equal(slot.key.begin(),slot.key.end(),first,end))slot.eligible=false;
  }
  // 所有槽登记结束才复用查找结果；缓存仅含数字槽号，记录 object 防止不同实体串用。
  struct GroupOwner {std::uint32_t object=S::kClientPlanNone,slot=S::kClientPlanNone;};
  std::pmr::vector<GroupOwner> owners(&arena);owners.resize(plan.NormalizedPathCount());
  const auto group=[&](std::uint32_t object,std::uint32_t path)->Impl::Slot *{if(path==S::kClientPlanNone||plan.Paths()[path].field_path==S::kClientPlanNone)return nullptr;
    auto &owner=owners[plan.Paths()[path].field_path];
    if(owner.object!=object){const auto i=groups.find({object,plan.Paths()[path].field_path});owner={object,i==groups.end()?S::kClientPlanNone:i->second};}
    return owner.slot==S::kClientPlanNone?nullptr:&s.slots[owner.slot];};
  const auto consumes=[&](std::uint32_t path,unsigned role){return path!=S::kClientPlanNone&&
    plan.Paths()[path].field_path!=S::kClientPlanNone&&(leaf_roles[plan.Paths()[path].field_path]&role);};
  const auto put=[](Impl::Slot &slot,std::size_t offset,std::span<const std::byte> bytes){if(offset>slot.data.size()||bytes.size()>slot.data.size()-offset)Invalid("binary_fields_leaf_range");
    for(std::size_t j=0;j<bytes.size();++j){if(slot.seen[offset+j]&&slot.data[offset+j]!=bytes[j])Invalid("binary_fields_projection_value_conflict");slot.seen[offset+j]=1;slot.data[offset+j]=bytes[j];}};
  // 字符串／键和值直接取安装计划的标量叶。没有读取长度头、变长记录或重跑 codec。
  for(std::uint32_t i=0;i<plan.ValueCount();++i){
    const auto v=plan.Value(i);const auto &record=plan.Records()[v.record];if(record.event!=S::kClientPlanNone||v.path==S::kClientPlanNone||v.layout)continue;
    const auto &p=plan.Paths()[v.path];const auto bytes=plan.Bytes(v.bytes);
    if(auto *slot=consumes(v.path,1)?group(record.object,v.path):nullptr;slot&&(slot->input.kind==Kind::String||slot->input.kind==Kind::CString||slot->input.kind==Kind::Blocked)){
      const auto width=slot->input.kind==Kind::Blocked?4u:1u;if(bytes.size()!=width||p.step.element==S::kNullIndex)Invalid("binary_fields_direct_leaf");put(*slot,std::size_t(p.step.element)*width,bytes);continue;}
    if(!consumes(p.parent,2))continue;auto *slot=group(record.object,p.parent);if(!slot)continue;const auto &parent=plan.Paths()[p.parent].step;
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
  for(std::uint32_t i=0;i<s.slots.size();++i){
    const auto &slot=s.slots[i];
    if(slot.eligible&&slot.object!=S::kClientPlanNone&&!Inline(slot.input.kind)&&!slot.value_only)s.reuse_order.push_back(i);
  }
  std::sort(s.reuse_order.begin(),s.reuse_order.end(),[&](auto a,auto b){
    const auto &x=s.slots[a],&y=s.slots[b];return std::tie(s.Identity(x),x.key)<std::tie(s.Identity(y),y.key);
  });
  s.described=true;
}
void ClientSceneFieldStaging::ReuseFrom(const ClientSceneFieldStaging &previous,const void *context,SameObject same_object) noexcept {
  auto &s=*impl_;const auto &old=*previous.impl_;
  if(!s.Thread()||!s.described||s.cursor||!old.described||s.visiting||old.visiting||!same_object)return;
  for(auto &next:s.slots){
    if(next.native||next.excluded||!next.eligible||next.object==S::kClientPlanNone||Inline(next.input.kind)||next.value_only)continue;
    const auto &identity=s.Identity(next);
    const auto found=std::lower_bound(old.reuse_order.begin(),old.reuse_order.end(),std::tie(identity,next.key),[&](auto i,const auto &key){
      const auto &x=old.slots[i];return std::tie(old.Identity(x),x.key)<key;
    });
    if(found==old.reuse_order.end())continue;
    const auto &before=old.slots[*found];
    if(old.Identity(before)!=identity||before.key!=next.key)continue;
    if(next.parent!=S::kClientPlanNone){
      const auto &parent=s.slots[next.parent];
      if(!parent.input.retained||before.parent==S::kClientPlanNone||parent.native!=old.slots[before.parent].native)continue;
    }else if(before.parent!=S::kClientPlanNone)continue;
    const auto changed=before.data!=next.data;
    const auto kind=next.input.kind;
    const bool same_carrier=((next.input.present&&before.input.present)||kind==Kind::Records)&&next.input.kind==before.input.kind&&
      next.input.layout==before.input.layout&&next.input.stride==before.input.stride&&
      next.input.slot_mask==before.input.slot_mask&&(kind==Kind::Records||next.input.payload_present==before.input.payload_present);
    // 这些容器的全部同步值由具名写入器消费。标量 vector 的叶会在调整长度后重新定位。
    const bool resize_sequence=same_carrier&&next.live_sequence&&before.live_sequence&&
      next.input.count!=before.input.count;
    const bool update=same_carrier&&changed&&
      (kind==Kind::String||kind==Kind::Map||kind==Kind::Innate||kind==Kind::Blocked||kind==Kind::Seeds);
    if(!before.native||(!Same(before.input,next.input)&&!update&&!resize_sequence)||before.costume!=next.costume||
        (!next.costume&&(before.blocks!=next.blocks||before.structure!=next.structure))||
        (changed&&!update)||!same_object(context,next.object,before.object))continue;
    bool compatible=!next.costume||S::ClientCostumeDirectoryMatches(s.plan->Effective(S::BodyStream::World),
      old.plan->Effective(S::BodyStream::World),identity);
    for(unsigned stream=0;stream<2&&compatible;++stream)if(next.structure[stream]&&!resize_sequence){
      const auto a=s.plan->Effective(S::BodyStream(stream)),b=old.plan->Effective(S::BodyStream(stream));
      const auto x=a?a->Find(identity):std::nullopt,y=b?b->Find(identity):std::nullopt;
      if(!x||!y){compatible=false;break;}
      for(unsigned bit=0;bit<32;++bit)if((next.structure[stream]&(std::uint32_t(1)<<bit))&&!x->SameStructure(*y,std::uint8_t(bit))){
        compatible=false;break;
      }
    }
    if(compatible&&ClientStagedFieldRetain(before.native)){next.native=before.native;next.input.retained=true;next.input.update_live=update||resize_sequence;next.input.resize_live_sequence=resize_sequence;}
  }
}
// 没有原生实例的根不构造字段或借资源承载，完整目标中的数字记录继续保留。
void ClientSceneFieldStaging::SkipObjects(const void *context,bool (*skip)(const void *,std::uint32_t) noexcept) noexcept {
  auto &s=*impl_;
  if(!s.Thread()||!s.described||s.visiting||s.cursor||!skip)return;
  for(auto &slot:s.slots)slot.excluded=skip(context,slot.owner_object);
}
ClientPreparationResult ClientSceneFieldStaging::Stage() noexcept {
  auto &s=*impl_;if(!s.Thread()||!s.described||s.visiting)return {ClientPreparationStatus::Failed,"binary_fields_not_described"};
  while(s.cursor<s.slots.size()){auto &slot=s.slots[s.cursor];
    if(slot.excluded||Inline(slot.input.kind)||slot.value_only){++s.cursor;continue;}
    if(slot.input.retained){
      if(!ClientStagedFieldCurrent(slot.native))return {ClientPreparationStatus::Failed,"binary_fields_retained_owner_changed"};
      s.memory->ApplyMetrics().Volume(slot.streams,S::ClientApplyVolume::Retained);
      ++s.cursor;continue;
    }
    if(slot.costume&&s.plan->Target().count==1&&s.plan->Target().sections[0].stream==S::BodyStream::Body)
      return {ClientPreparationStatus::Failed,"binary_costume_shared_attachment_not_current"};
    // 只登记当前写段需要的新承载。分配和构造由实际接入活对象的写入器完成。
    slot.input.create_live_owner=slot.input.present||slot.input.kind==Kind::Records;
    if(!slot.native)slot.native=ClientStagedFieldCreate(s.allocator);if(!slot.native)return {ClientPreparationStatus::Failed,"binary_fields_owner_allocation"};
    const auto result=ClientStagedFieldConstruct(slot.native,slot.input);
    if(result.status!=ClientPreparationStatus::Ready)return result;
    s.memory->ApplyMetrics().Volume(slot.streams,S::ClientApplyVolume::Constructed);++s.cursor;
  }
  return {ClientPreparationStatus::Ready,"binary_fields_staged"};
}
bool ClientSceneFieldStaging::HasNative()const noexcept{return impl_->Thread()&&std::any_of(impl_->slots.begin(),impl_->slots.end(),[](const auto &slot){return slot.native;});}
bool ClientSceneFieldStaging::Drain() noexcept {
  auto &s=*impl_;if(!s.Thread()||s.visiting)return false;for(std::size_t i=s.slots.size();i>0;--i)if(!ClientStagedFieldRelease(s.slots[i-1].native))return false;
  s.slots.clear();s.fields.clear();s.reuse_order.clear();s.plan=nullptr;s.cursor=0;s.described=false;return true;
}
bool ClientSceneFieldStaging::Visit(void *context,Visitor visit,const void *window,Continue current,
    std::uint64_t ticket,const char *phase) noexcept {
  auto &s=*impl_;if(!s.Thread()||!visit||!current||s.visiting||!s.described||s.cursor!=s.slots.size())return false;
  struct CheckedFields {
    S::ClientApplyMetrics &metrics;std::uint8_t streams;std::uint64_t n=0;
    ~CheckedFields(){metrics.Volume(streams,S::ClientApplyVolume::Checks,n);}
  } checks{s.memory->ApplyMetrics(),S::StateTargetStreams(s.plan->Target())};
  // World／Body 的字段索引可能指向同一个暂存槽；纯观察期间每个实槽只核一次。
  for(std::size_t i=0;i<s.slots.size();++i){
    if(i%128==0&&!current(window))return false;
    const auto &slot=s.slots[i];
    if(slot.excluded||Inline(slot.input.kind)||slot.value_only)continue;
    ++checks.n;
    if(!ClientStagedFieldCurrent(slot.native)){
      S::ClientApplyMetrics::Issue fact;fact.route=s.plan->Target().route.route;fact.ticket=ticket;
      fact.layout=slot.input.layout;
      if(slot.object!=S::kClientPlanNone){const auto &id=s.Identity(slot);fact.member=id.id;fact.instance=id.use;}
      if(!slot.key.empty()){
        fact.layout=slot.key.back().layout;fact.field=slot.key.back().field;
        if(const auto *definition=S::FindField(slot.key.front().layout,slot.key.front().field))fact.block=definition->block;
      }
      s.memory->ApplyMetrics().FirstIssue(slot.streams,fact,phase,"staged_container_changed");
      return false;
    }
  }
  s.visiting=true;bool ok=true;
  for(std::uint32_t f=0;f<s.fields.size();++f){
    if(f%128==0&&!current(window)){ok=false;break;}
    if(s.fields[f]==S::kClientPlanNone)continue;
    auto &slot=s.slots[s.fields[f]];
    if(slot.excluded)continue;
    if(!visit(context,f,slot.input,slot.native)){ok=false;break;}
  }
  s.visiting=false;return ok;
}
} // namespace cs::game
