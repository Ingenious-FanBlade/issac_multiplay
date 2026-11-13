#include "game/client/scene/client_atomic_install.h"
#include "game/client/scene/client_atomic_bindings.h"
#include "game/client/scene/client_lifetime.h"
#include "game/common/bridge.h"
#include "game/common/context.h"
#include "runtime.h"
#include <algorithm>
#include <cstring>
#include <map>
#include <tuple>

namespace cs::game {
namespace {
namespace S=state;
S::ClientStateNativeStep Ready(bool written,const char *why){return {true,written,false,why};}
S::ClientStateNativeStep Failed(bool written,const char *why){return {false,written,true,why};}
[[noreturn]]void Bad(const char *why){throw S::ClientShadowError(S::StateReadiness::InvalidEncoding,why);}
template<class T>T Number(std::span<const std::byte> bytes){if(bytes.size()!=sizeof(T))Bad("binary_atomic_scalar_width");T v;std::memcpy(&v,bytes.data(),sizeof(v));return v;}
using Key=std::tuple<std::uint32_t,std::uint32_t,std::uint16_t,std::uint16_t,std::uint16_t,std::uint16_t>;
using Lookup=std::map<Key,std::uint32_t,std::less<Key>,S::ClientStateAllocator<std::pair<const Key,std::uint32_t>>>;
bool Rows(ClientStagedFieldKind k){return k==ClientStagedFieldKind::Rows;}
// 只绑定已核的原位同宽转换；其余转换必须有具名owner，不能仅因宽度相同就复制。
bool RawConversion(std::string_view h){return h=="playercostumemap_layer_id_read"||h=="playeractiveitemdesc_charge_read"||
  h=="playeractiveitemdesc_battery_charge_read"||h=="playeractiveitemdesc_sub_charge_read"||h=="playeractiveitemdesc_partial_charge_read"||
  h=="playeractiveitemdesc_var_data_read"||h=="playertemporaryeffect_cooldown_read"||h=="drawrecordnode_main_animation_frame_read"||
  h=="drawrecordnode_second_animation_frame_read"||h=="entityplayer_inventory_suppressed_read"||
  h=="f_x_layers_loaded_read"||h=="f_x_mask_read"||h=="f_x_blend_mode_read"||h=="ray_phase_read"||
  h=="level_game_prefix_greed_wave_read"||h=="room_descriptor_spawn_seed_read"||h=="room_hell_backdrop_active_read"||
  h=="room_hell_backdrop_display_offset_read"||h=="room_hell_accumulated_offset_read"||h=="sfx_volume_modifier_read";}
}
// 保存首次写入前的根标量。重用同一地址时保留最早字节，最后一批实际撤场时反序归还。
class ClientSceneAtomicInstaller::RootScalars final {
  struct Entry {void *address=nullptr;std::uint32_t offset=0,size=0;};
  using Index=std::pair<std::uintptr_t,std::uint32_t>;
  S::ClientStateVector<Entry> entries_;
  S::ClientStateVector<std::byte> bytes_;
  std::map<Index,bool,std::less<Index>,S::ClientStateAllocator<std::pair<const Index,bool>>> saved_;
public:
  explicit RootScalars(const std::shared_ptr<S::ClientStateMemory> &m):entries_(S::ClientStateAllocator<Entry>(m)),
    bytes_(S::ClientStateAllocator<std::byte>(m)),saved_(S::ClientStateAllocator<std::pair<const Index,bool>>(m)) {}
  bool Save(const ClientNativeRoots &roots,void *p,std::span<const std::byte> original) noexcept {
    if(!ClientAtomicRootScalar(roots,p,original.size()))return true;
    const Index index{reinterpret_cast<std::uintptr_t>(p),std::uint32_t(original.size())};
    if(saved_.contains(index))return true;
    try{
      const auto start=std::uint32_t(bytes_.size());
      if(original.size()>UINT32_MAX-start)return false;
      bytes_.insert(bytes_.end(),original.begin(),original.end());
      entries_.push_back({p,start,index.second});saved_.emplace(index,true);return true;
    }catch(...){return false;}
  }
  bool Restore(const ClientNativeRoots &roots) noexcept {
    while(!entries_.empty()){
      const auto &entry=entries_.back();
      if(!ClientAtomicRestoreRootScalar(roots,entry.address,bytes_.data()+entry.offset,entry.size))return false;
      entries_.pop_back();
    }
    bytes_.clear();saved_.clear();return true;
  }
};
std::shared_ptr<ClientSceneAtomicInstaller::RootScalars> ClientBinaryRootScalars(const std::shared_ptr<S::ClientStateMemory> &m){
  return std::allocate_shared<ClientSceneAtomicInstaller::RootScalars>(S::ClientStateAllocator<ClientSceneAtomicInstaller::RootScalars>(m),m);
}
struct ClientSceneAtomicInstaller::Impl {
  struct Op {
    ClientAtomicWrite write;
    std::uint32_t field=0,original=0,value=0,order=0,element=S::kClientPlanNone,staged_element=S::kClientPlanNone,reference=S::kClientPlanNone;
    S::ClientInstallPhase phase=S::ClientInstallPhase::Values;
    unsigned priority=5;
    bool delegated=false,late=false,duplicate=false,done=false,restore_entered=false,restored=false;
  };
  std::shared_ptr<S::ClientStateMemory> memory;ClientSceneMainPreparation &preparation;
  std::unique_ptr<ClientAtomicInstallBindings> owned_bindings;
  ClientAtomicInstallBindings &bindings;
  std::shared_ptr<RootScalars> root_scalars;
  const std::thread::id thread=std::this_thread::get_id();
  S::ClientStateApplyBatchRef batch;std::shared_ptr<const ClientSceneInstallPlan> scene;
  S::ClientStateVector<ClientAtomicObject> objects;S::ClientStateVector<ClientAtomicField> fields;
  S::ClientStateVector<ClientAtomicResource> resources;S::ClientStateVector<Op> operations;
  S::ClientStateVector<std::byte> values,originals;
  S::ClientStateVector<std::uint32_t> resolved_writes;
  S::ClientStateVector<std::uint32_t> field_attachments;
  // costume独立分组：两流共享容器随最新批次退休，旧流只检查仍属于自己的普通字段。
  // 切换写入器时沿用Manager游标，避免高频收包让尾部饥饿。
  using ReadbackKey=std::tuple<bool,S::BodyStream,std::uint32_t,std::uint8_t>;
  struct ReadbackEntry {ReadbackKey key;std::uint32_t operation;};
  S::ClientStateVector<ReadbackEntry> readback;
  Lookup paths,field_index;S::ClientStateVector<std::uint32_t> normalized;
  using WriteIndex=std::map<std::uintptr_t,std::uint32_t,std::less<std::uintptr_t>,S::ClientStateAllocator<std::pair<const std::uintptr_t,std::uint32_t>>>;
  WriteIndex write_index;
  ClientNativeRoots roots{};void *vm=nullptr;std::uint64_t manager_token=0;
  bool staged=false,lease=false,install_entered=false,installed=false,publish_entered=false,published=false,written=false;
  bool invalid=false,quiesced=false,retired=false,lease_released=false,uncertain=false,notified=false;
  bool final_scene=false;
  const char *reason="binary_atomic_idle";
  std::array<char,192> failure{};
  Impl(std::shared_ptr<S::ClientStateMemory> m,ClientSceneMainPreparation &p,ClientAtomicInstallBindings *b,std::shared_ptr<RootScalars> scalars={}):
    memory(std::move(m)),preparation(p),owned_bindings(b?nullptr:ClientMakeAtomicInstallBindings(memory,p)),
    bindings(b?*b:*owned_bindings),root_scalars(std::move(scalars)),
    objects(S::ClientStateAllocator<ClientAtomicObject>(memory)),fields(S::ClientStateAllocator<ClientAtomicField>(memory)),
    resources(S::ClientStateAllocator<ClientAtomicResource>(memory)),operations(S::ClientStateAllocator<Op>(memory)),
    values(S::ClientStateAllocator<std::byte>(memory)),originals(S::ClientStateAllocator<std::byte>(memory)),
    resolved_writes(S::ClientStateAllocator<std::uint32_t>(memory)),
    field_attachments(S::ClientStateAllocator<std::uint32_t>(memory)),
    readback(S::ClientStateAllocator<ReadbackEntry>(memory)),
    paths(S::ClientStateAllocator<std::pair<const Key,std::uint32_t>>(memory)),field_index(S::ClientStateAllocator<std::pair<const Key,std::uint32_t>>(memory)),
    normalized(S::ClientStateAllocator<std::uint32_t>(memory)),write_index(S::ClientStateAllocator<std::pair<const std::uintptr_t,std::uint32_t>>(memory)){}
  bool Thread()const noexcept{return thread==std::this_thread::get_id();}
  ClientAtomicInputs Inputs(){
    return {*scene,objects,fields,resources,this,[](void *owner,std::uint32_t record,std::uint32_t path,void *&address,std::size_t &extent){
      return static_cast<Impl *>(owner)->Instance(record,path,address,extent);
    }};
  }
  bool Current(const S::ClientStateApplyBatch &b)const noexcept {
    return Thread()&&batch.get()==&b&&!invalid&&!uncertain&&preparation.Current(b)&&bindings.Current(b)&&
      (!staged||ClientAtomicInstallWindow(roots,vm));
  }
  S::ClientStateNativeStep Fail(const char *why) noexcept {
    if(!why)why="binary_atomic_unspecified_failure";const auto size=(std::min)(std::strlen(why),failure.size()-1);
    std::memmove(failure.data(),why,size);failure[size]=0;reason=failure.data();invalid=true;
    if(written&&batch&&!notified){notified=true;bindings.Invalidate(*batch,reason);}return Failed(written,reason);
  }
  Key PathKey(std::uint32_t object,std::uint32_t n,bool field)const {
    const auto &p=scene->State().Paths()[n];S::BodySourceFieldInfo format;
    if(!S::DescribeBodySourceField(p.step.layout,p.step.field,format))Bad("binary_atomic_path_definition");
    return {object,p.parent==S::kClientPlanNone?S::kClientPlanNone:normalized[p.parent],p.step.layout,format.canonical,
      field?S::kNullIndex:p.step.element,field?0:p.step.choice_layout};
  }
  // 父链逐层定位；动态容器元素取候选私有承载，固定内嵌对象保留其真实地址。
  bool Instance(std::uint32_t record,std::uint32_t path,void *&address,std::size_t &extent,unsigned depth=0) {
    const auto &plan=scene->State();const auto object=plan.Records()[record].object;
    if(object>=objects.size()||depth>=S::kCodecMaxDepth)return false;
    if(path==S::kClientPlanNone){address=objects[object].address;const auto *l=S::FindLayout(plan.Objects()[object].member.identity.layout);extent=l?l->native_size:0;return address&&extent;}
    const auto &p=plan.Paths()[path];const auto *d=S::FindField(p.step.layout,p.step.field);if(!d||(!d->nested_layout&&!p.step.choice_layout))return false;
    const auto i=field_index.find(PathKey(object,path,true));
    if(i!=field_index.end()) {auto &f=fields[i->second];
      if(f.native&&f.input.kind!=ClientStagedFieldKind::Group&&!Rows(f.input.kind)) {
        const auto element=p.step.element==S::kNullIndex?0u:p.step.element;address=ClientStagedFieldElement(f.native,element);
        const auto *l=S::FindLayout(p.step.choice_layout?p.step.choice_layout:d->nested_layout);extent=l?l->native_size:0;return address&&extent;
      }
    }
    void *parent=nullptr;std::size_t parent_size=0;if(!Instance(record,p.parent,parent,parent_size,depth+1))return false;
    if(bindings.LatePath(*d)||d->source!=S::FieldSource::Object||S::FindLayout(p.step.layout)->protocol)
      return bindings.Resolve(Inputs(),record,path,parent,address,extent)&&address&&extent;
    const auto *l=S::FindLayout(p.step.choice_layout?p.step.choice_layout:d->nested_layout);if(!l)return false;
    const auto element=p.step.element==S::kNullIndex?0u:p.step.element;
    const auto offset=std::uint64_t(d->native_offset)+std::uint64_t(element)*d->native_stride;
    if(offset>parent_size||l->native_size>parent_size-offset)return false;
    address=static_cast<std::byte *>(parent)+std::size_t(offset);extent=l->native_size;return true;
  }
  bool Late(std::uint32_t path)const noexcept {
    const auto &plan=scene->State();
    for(;path!=S::kClientPlanNone;path=plan.Paths()[path].parent){
      const auto &step=plan.Paths()[path].step;
      const auto *field=S::FindField(step.layout,step.field);
      if(field&&bindings.LatePath(*field))return true;
    }
    return false;
  }
  bool Destination(std::uint32_t field,void *&address,std::size_t &extent) {
    const auto &plan=scene->State();const auto &f=plan.Fields()[field];const auto &d=S::Fields()[f.definition];const auto &p=plan.Paths()[f.path];
    const auto kind=fields[field].input.kind;const bool records=kind==ClientStagedFieldKind::Records;
    if((d.source!=S::FieldSource::Object&&!records)||S::FindLayout(d.layout)->protocol)return false;
    const auto size=records?12u:kind==ClientStagedFieldKind::Seeds?8u:d.native_size;
    const auto offset=records?0u:d.native_offset;
    void *parent=nullptr;std::size_t parent_size=0;if(!Instance(f.record,p.parent,parent,parent_size)||offset>parent_size||size>parent_size-offset)return false;
    address=static_cast<std::byte *>(parent)+offset;extent=size;return address&&extent;
  }
  void Add(Op op,std::span<const std::byte> bytes={}) {
    if(operations.size()>=262144)Bad("binary_atomic_operation_budget");
    if(!op.delegated){
      if(!op.write.size||op.write.size>op.write.extent)Bad("binary_atomic_destination_range");
      if(!op.late){
      if(!op.write.destination)Bad("binary_atomic_destination_range");
      const auto begin=reinterpret_cast<std::uintptr_t>(op.write.destination);if(op.write.size>UINTPTR_MAX-begin)Bad("binary_atomic_address_overflow");
      // 同址投影只允许一致目标；部分重叠拒绝，避免悄悄接受字段绑定错误。
      auto at=write_index.lower_bound(begin);
      auto overlap=at;if(at!=write_index.begin()){const auto previous=std::prev(at);if(previous->first+operations[previous->second].write.size>begin)overlap=previous;}
      if(overlap!=write_index.end()){const auto &old=operations[overlap->second];const auto other=overlap->first;
        if(begin<other+old.write.size&&other<begin+op.write.size){
          bool same=begin==other&&op.write.size==old.write.size&&op.write.kind==old.write.kind&&op.write.mask==old.write.mask&&
            op.write.reference==old.write.reference&&op.write.field==old.write.field;
          if(same&&!bytes.empty())same=bytes.size()==old.write.size&&std::equal(bytes.begin(),bytes.end(),values.begin()+old.value);
          if(same)return;Bad("binary_atomic_projection_write_conflict");}}
      write_index.emplace(begin,std::uint32_t(operations.size()));
      }
      if(op.write.size>UINT32_MAX-originals.size()||bytes.size()>UINT32_MAX-values.size())Bad("binary_atomic_journal_budget");
      op.original=std::uint32_t(originals.size());originals.resize(originals.size()+op.write.size);
      op.value=std::uint32_t(values.size());values.insert(values.end(),bytes.begin(),bytes.end());
    }
    op.order=std::uint32_t(operations.size());
    operations.push_back(op);
  }
  void Leaf(std::uint32_t field,const S::ClientInstallValue &v) {
    const auto &plan=scene->State();const auto &f=plan.Fields()[field];const auto &d=S::Fields()[f.definition];const auto bytes=plan.Bytes(v.bytes);
    const auto parent=plan.Paths()[v.path].parent;
    if(parent!=S::kClientPlanNone){const auto i=field_index.find(PathKey(plan.Records()[v.record].object,parent,true));
      if(i!=field_index.end()){const auto kind=fields[i->second].input.kind;
        if((kind==ClientStagedFieldKind::Map&&d.layout==177)||(kind==ClientStagedFieldKind::Innate&&d.layout==178)||
           (kind==ClientStagedFieldKind::Seeds&&d.layout==238))return;}}
    const bool late=Late(parent);
    void *address=nullptr;std::size_t extent=0;
    std::uint32_t inline_element=S::kClientPlanNone,staged_element=S::kClientPlanNone;
    const auto &staged_field=fields[field];
    if(d.operation==S::Operation::Container) {
      if(staged_field.native){const auto kind=staged_field.input.kind;
        // 文本／键／聚合已由StageFields从同一叶构造，键不可在有序容器中再次原位修改。
        if(kind==ClientStagedFieldKind::String||kind==ClientStagedFieldKind::CString||kind==ClientStagedFieldKind::Blocked||
           kind==ClientStagedFieldKind::Map||kind==ClientStagedFieldKind::Innate||kind==ClientStagedFieldKind::Seeds)return;
        const auto element=plan.Paths()[v.path].step.element;if(element==S::kNullIndex)Bad("binary_atomic_scalar_element");
        if(Rows(kind)){
          void *base=nullptr;std::size_t capacity=d.native_size;
          if((!late&&!Destination(field,base,capacity))||std::uint64_t(element+1)*staged_field.input.stride>capacity)Bad("binary_atomic_inline_element_range");
          inline_element=element;
          if(!late)address=static_cast<std::byte *>(base)+std::size_t(element)*staged_field.input.stride;
        }else {staged_element=element;address=ClientStagedFieldElement(staged_field.native,element);}
        extent=staged_field.input.stride;
      }
    }else if(late)extent=d.native_size;
    else if(!Destination(field,address,extent))Bad("binary_atomic_field_address_owner");
    if((!address&&!late)||!extent)Bad("binary_atomic_scalar_storage");
    Op op;op.field=field;op.phase=f.phase;op.write.destination=address;op.write.extent=extent;
    op.late=late&&!address;op.element=inline_element;op.staged_element=staged_element;
    S::BodySourceFieldInfo format;if(!S::DescribeBodySourceField(d.layout,d.index,format))Bad("binary_atomic_leaf_format");
    if(v.operation==S::Operation::Reference||v.operation==S::Operation::Resource) {
      const auto index=std::size_t(&v-plan.Values().data());
      const auto link=std::lower_bound(plan.Bindings().begin(),plan.Bindings().end(),index,[](const auto &l,auto i){return l.value<i;});
      if(link==plan.Bindings().end()||link->value!=index||link->is_resource)Bad("binary_atomic_resource_owner_missing");
      if(!link->is_null&&(link->object>=objects.size()||!objects[link->object].address))Bad("binary_atomic_reference_owner_missing");
      const std::string_view handler=d.handler;
      if(handler!="entity_pointer_to_member_index"&&handler!="entity_reference_to_member"&&
         handler!="entity_reference_to_identity"&&handler!="weapon_whip_main_to_member_index"&&
         handler!="player_tail_44_to_member"&&handler!="player__twinPlayer_to_member"&&
         handler!="read_npc_composite_references")Bad("binary_atomic_reference_abi_owner");
      op.write.kind=ClientAtomicWriteKind::EntityReference;op.write.size=sizeof(void *);op.write.reference=link->is_null?nullptr:objects[link->object].address;
      op.reference=link->is_null?S::kClientPlanNone:link->object;
      op.phase=S::ClientInstallPhase::Relations;Add(op);return;
    }
    std::array<std::byte,8> converted{};auto output=bytes;
    if(d.operation==S::Operation::Convert) {
      const std::string_view handler=d.handler;
      if(d.value_mask&&bytes.size()==4){op.write.kind=ClientAtomicWriteKind::Masked32;op.write.mask=d.value_mask;}
      else if(handler=="ExpireFrame") {const auto frame=plan.Records()[f.record].context.game_frame;const auto remaining=Number<std::uint32_t>(bytes)-frame;std::memcpy(converted.data(),&remaining,4);output=std::span(converted).first(4);}
      else if(handler=="playercostumemap_index_read"){const auto i=Number<std::uint16_t>(bytes);const std::int32_t index=i==S::kNullIndex?-1:i;std::memcpy(converted.data(),&index,4);output=std::span(converted).first(4);}
      else if(handler=="queueitemdata_flags_read"){const std::uint32_t flag=Number<std::uint8_t>(bytes)?2u:0u;std::memcpy(converted.data(),&flag,4);output=std::span(converted).first(4);op.write.kind=ClientAtomicWriteKind::Masked32;op.write.mask=2;}
      else if(handler=="entity_plus_flying_to_value"){
        const auto value=Number<std::int8_t>(bytes);
        if(value < -1 || value > 1)Bad("binary_atomic_flying_optional");
        converted[0]=std::byte(value==1);converted[1]=std::byte(value!=-1);output=std::span(converted).first(2);
      }else if(handler=="entity_plus_water_to_value"){
        const auto value=Number<std::uint64_t>(bytes);
        if(value>>32>1||((value>>32)==0&&value))Bad("binary_atomic_water_optional");
        std::memcpy(converted.data(),&value,5);output=std::span(converted).first(5);
      }
      else if(!RawConversion(handler))Bad("binary_atomic_conversion_owner_missing");
    }else if(d.operation!=S::Operation::Copy&&d.operation!=S::Operation::Container)Bad("binary_atomic_leaf_operation");
    if(output.size()!=extent)Bad("binary_atomic_native_value_width");op.write.size=output.size();Add(op,output);
  }
  void Describe() {
    const auto &plan=scene->State();objects.resize(plan.Objects().size());fields.resize(plan.Fields().size());
    if(!preparation.VisitObjects(*batch,this,[](void *p,std::uint32_t i,void *address,const S::PresentationMember &m)noexcept{
      auto &s=*static_cast<Impl *>(p);if(i>=s.objects.size())return false;s.objects[i]={address,m};return true;}))Bad("binary_atomic_object_borrow");
    if(!preparation.VisitFields(*batch,this,[](void *p,std::uint32_t i,const ClientStagedFieldInput &input,ClientStagedField *native)noexcept{
      auto &s=*static_cast<Impl *>(p);if(i>=s.fields.size())return false;s.fields[i]={input,native};return true;}))Bad("binary_atomic_field_borrow");
    resources.reserve(plan.Resources().size());
    if(!preparation.VisitResources(*batch,this,[](void *p,S::ResourceKey key,void *address)noexcept{
      auto &s=*static_cast<Impl *>(p);if(s.resources.size()==s.resources.capacity())return false;s.resources.push_back({key,address});return true;}))Bad("binary_atomic_resource_borrow");
    std::sort(resources.begin(),resources.end(),[](const auto &a,const auto &b){return a.key<b.key;});
    normalized.reserve(plan.Paths().size());
    for(std::uint32_t n=0;n<plan.Paths().size();++n){const auto key=PathKey(0,n,false);const auto [i,added]=paths.emplace(key,std::uint32_t(paths.size()));normalized.push_back(i->second);}
    for(std::uint32_t n=0;n<plan.Fields().size();++n){const auto &f=plan.Fields()[n];const auto &r=plan.Records()[f.record];if(r.event!=S::kClientPlanNone)continue;
      field_index.emplace(PathKey(r.object,f.path,true),n);}
    // 每个完整字段先取得唯一归属，再建立原生指令；委托不能只覆盖一个标量却遗失空／缺席操作。
    S::ClientStateVector<std::uint8_t> delegated{S::ClientStateAllocator<std::uint8_t>(memory)};delegated.resize(plan.Fields().size());
    for(std::uint32_t n=0;n<plan.Fields().size();++n){const auto &f=plan.Fields()[n];const auto &r=plan.Records()[f.record];if(r.event!=S::kClientPlanNone)continue;
      const auto &d=S::Fields()[f.definition];
      if(bindings.Covers(Inputs(),n)){delegated[n]=1;Op op;op.delegated=true;op.field=n;op.phase=f.phase;Add(op);continue;}
      if(!f.present&&fields[n].input.kind!=ClientStagedFieldKind::Records)continue;
      if(d.nested_layout||d.operation==S::Operation::Container||d.operation==S::Operation::Choice){
        auto &field=fields[n];
        if(!field.native)Bad("binary_atomic_field_not_staged");
        const auto size=ClientStagedFieldAttachmentSize(field.native);
        if(size){void *address=nullptr;std::size_t extent=size;
          const bool late=Late(plan.Paths()[f.path].parent);
          if((!late&&!Destination(n,address,extent))||extent!=size)Bad("binary_atomic_container_owner_missing");
          Op op;op.field=n;op.phase=f.phase;op.late=late;
          op.write={ClientAtomicWriteKind::FieldAttachment,address,extent,size,nullptr,nullptr,0,field.native};Add(op);
        }else if(field.input.kind==ClientStagedFieldKind::Group){
          // 内嵌Group无独立header；协议聚合和不透明owner必须明确委托，不能空操作冒充安装。
          if(d.source!=S::FieldSource::Object||S::FindLayout(d.layout)->protocol)Bad("binary_atomic_group_owner_missing");
        }else if(Rows(field.input.kind)){
          S::BodySourceFieldInfo format;if(!S::DescribeBodySourceField(d.layout,d.index,format)||d.source!=S::FieldSource::Object||
            S::FindLayout(d.layout)->protocol||(d.operation==S::Operation::Container&&(format.storage!=S::BodyStorage::Array||
            (!d.fixed_count&&std::string_view(d.handler)!="read_effect_ghost_active_sprites"&&std::string_view(d.handler)!="rail_manager_grid_flags_read")||
            std::uint64_t(field.input.count)*field.input.stride>d.native_size)))Bad("binary_atomic_rows_owner_missing");
        }else Bad("binary_atomic_container_attachment_abi");
      }
    }
    for(const auto &v:plan.Values()){const auto &r=plan.Records()[v.record];if(r.event!=S::kClientPlanNone||v.layout)continue;
      const auto i=field_index.find(PathKey(r.object,v.path,true));if(i==field_index.end())Bad("binary_atomic_value_field_missing");
      if(!delegated[i->second])Leaf(i->second,v);
    }
    for(auto &op:operations){
      const auto &d=S::Fields()[plan.Fields()[op.field].definition];
      const auto order=bindings.Order(d);
      op.priority=op.delegated?order:op.write.kind==ClientAtomicWriteKind::EntityReference?7u:order==0?0u:5u;
    }
    std::sort(operations.begin(),operations.end(),[](const auto &a,const auto &b){
      return std::tie(a.priority,a.phase,a.order)<std::tie(b.priority,b.phase,b.order);
    });
    field_attachments.resize(fields.size(),S::kClientPlanNone);readback.reserve(operations.size());
    for(std::uint32_t i=0;i<operations.size();++i){
      if(!operations[i].delegated&&operations[i].write.kind==ClientAtomicWriteKind::FieldAttachment)
        field_attachments[operations[i].field]=i;
      const auto &f=plan.Fields()[operations[i].field];auto path=f.path;
      while(plan.Paths()[path].parent!=S::kClientPlanNone)path=plan.Paths()[path].parent;
      const auto &step=plan.Paths()[path].step;const auto *root=S::FindField(step.layout,step.field);
      if(!root)Bad("binary_atomic_readback_block");
      readback.push_back({{S::ClientCostumeProjection(step.layout,step.field),plan.Records()[f.record].stream,f.record,root->block},i});
    }
    std::sort(readback.begin(),readback.end(),[](const auto &a,const auto &b){
      return std::tie(a.key,a.operation)<std::tie(b.key,b.operation);
    });
    write_index.clear();
    // vector可能在构造时重分配；全部封存后才绑定叶缓冲地址，安装区间不再分配这些数组。
    resolved_writes.reserve(operations.size());
    for(auto &op:operations)if(!op.delegated){if(op.write.kind==ClientAtomicWriteKind::Bytes||op.write.kind==ClientAtomicWriteKind::Masked32)op.write.value=values.data()+op.value;
      if(!op.late&&!ClientAtomicWritePreflight(op.write,originals.data()+op.original,op.write.size))Bad("binary_atomic_native_preflight");}
  }
  // 装载稳定图层地址后再查实际区间。所有索引空间已在Stage保留，不在写入中扩容。
  void ResolveWrites() {
    for(std::uint32_t i=0;i<operations.size();++i){
      auto &op=operations[i];if(op.delegated)continue;
      if(op.late){
        void *address=nullptr;std::size_t extent=0;
        if(!Destination(op.field,address,extent))Bad("binary_atomic_late_field_address");
        const auto offset=op.element==S::kClientPlanNone?0:std::size_t(op.element)*op.write.extent;
        if(offset>extent||op.write.size>extent-offset)Bad("binary_atomic_late_field_range");
        op.write.destination=static_cast<std::byte *>(address)+offset;
      }
      resolved_writes.push_back(i);
    }
    const auto address=[&](std::uint32_t n){return reinterpret_cast<std::uintptr_t>(operations[n].write.destination);};
    std::sort(resolved_writes.begin(),resolved_writes.end(),[&](auto a,auto b){return std::tuple(address(a),a)<std::tuple(address(b),b);});
    std::uint32_t previous=S::kClientPlanNone;
    for(auto n:resolved_writes){
      auto &w=operations[n].write;const auto start=address(n);
      if(!start||w.size>UINTPTR_MAX-start)Bad("binary_atomic_resolved_range");
      if(previous!=S::kClientPlanNone&&address(previous)+operations[previous].write.size>start){
        const auto &old=operations[previous].write;
        if(address(previous)!=start||old.size!=w.size||old.kind!=w.kind||old.reference!=w.reference||old.field!=w.field||old.mask!=w.mask||
           (old.value&&(!w.value||std::memcmp(old.value,w.value,w.size))))Bad("binary_atomic_resolved_projection_conflict");
        operations[n].duplicate=true;
      }else previous=n;
    }
  }
  bool Step(S::ClientStateNativeStep s) noexcept {
    written|=s.written;if(!s.ready||s.failed){Fail(s.reason);return false;}
    if(!Current(*batch)){Fail("binary_atomic_lifecycle_after_callback");return false;}return true;
  }
  S::ClientStateNativeStep InstallNow() noexcept {
    install_entered=true;
    bool initialized=false,resolved=false,field_owners=false;
    for(auto &op:operations){if(!Current(*batch))return Fail("binary_atomic_lifecycle_during_install");
      if(op.priority>0&&!initialized){if(!Step(bindings.Initialize(*batch,Inputs())))return Failed(written,reason);initialized=true;}
      if(op.priority>0&&!field_owners){if(!Step(bindings.PrepareFieldOwners(*batch,Inputs())))return Failed(written,reason);field_owners=true;}
      if(op.priority>3&&!resolved){try{ResolveWrites();resolved=true;}catch(const S::ClientShadowError &e){return Fail(e.what());}catch(...){return Fail("binary_atomic_late_field_path");}}
      if(op.duplicate)continue;
      if(op.delegated){op.done=true;if(!Step(bindings.InstallField(*batch,Inputs(),op.field)))return Failed(written,reason);}
      else {
        // Init／先前特殊字段可改变原生默认值；还原快照在真实写入之前重新取，不复用暂存期旧header。
        if(!ClientAtomicWritePreflight(op.write,originals.data()+op.original,op.write.size))return Fail("binary_atomic_write_preflight_changed");
        written=true;op.done=true;
        if(!ClientAtomicWriteApply(op.write)){if(op.write.kind==ClientAtomicWriteKind::EntityReference)uncertain=true;return Fail("binary_atomic_native_write_failed");}
      }
      op.done=true;
    }
    if(!initialized&&!Step(bindings.Initialize(*batch,Inputs())))return Failed(written,reason);
    for(const auto &op:operations)if(!op.delegated&&!op.duplicate&&!ClientAtomicWriteCheck(op.write))return Fail("binary_atomic_native_postcondition");
    if(!Step(bindings.InstallMembers(*batch,Inputs())))return Failed(written,reason);
    installed=true;reason="binary_atomic_installed";return Ready(written,reason);
  }
  S::ClientStateNativeStep PublishNow() noexcept {
    publish_entered=true;if(!Step(bindings.Publish(*batch,Inputs())))return Failed(written,reason);
    for(const auto &op:operations)if(!op.delegated&&!op.duplicate&&!ClientAtomicWriteCheck(op.write))return Fail("binary_atomic_publish_postcondition");
    // 先核全部header，准备owner接收本批之后再折叠旧字段租约。
    for(const auto &op:operations)if(!op.delegated&&!op.duplicate&&op.write.kind==ClientAtomicWriteKind::FieldAttachment&&
        !ClientStagedFieldAttached(op.write.field,op.write.destination))return Fail("binary_atomic_attachment_publish_changed");
    return Ready(written,"binary_atomic_members_published");
  }
  S::ClientStateNativeStep CommitFields() noexcept {
    for(const auto &op:operations)if(!op.delegated&&!op.duplicate&&op.write.kind==ClientAtomicWriteKind::FieldAttachment&&
        !ClientStagedFieldCommitAttachment(op.write.field)){uncertain=true;return Fail("binary_atomic_attachment_commit_failed");}
    return Ready(written,"binary_atomic_fields_committed");
  }
  ClientAtomicReadback ReadOp(const Op &op,bool repair) {
    using R=ClientAtomicReadback;
    if(op.duplicate)return R::Untracked;
    const auto &plan=scene->State();const auto &f=plan.Fields()[op.field];
    const auto object=plan.Records()[f.record].object;
    if(S::FindLayout(plan.Records()[f.record].layout)->protocol)return R::Untracked;
    if(object>=objects.size()||!preparation.InstalledObjectCurrent(*batch,object,objects[object].address))return R::Unsafe;
    if(op.reference!=S::kClientPlanNone&&!preparation.InstalledObjectCurrent(*batch,op.reference,op.write.reference))return R::Unsafe;
    // 动态子地址只有在仍由本批容器承载时可读。任一父层换址都交完整恢复。
    for(auto p=plan.Paths()[f.path].parent;p!=S::kClientPlanNone;p=plan.Paths()[p].parent){
      const auto found=field_index.find(PathKey(plan.Records()[f.record].object,p,true));
      if(found==field_index.end())continue;
      const auto n=found->second;const auto attachment=field_attachments[n];
      if(attachment==S::kClientPlanNone)continue;
      const auto &owner=operations[attachment].write;
      void *slot=nullptr;std::size_t extent=0;
      if(!Destination(n,slot,extent)||slot!=owner.destination||!ClientStagedFieldAttached(owner.field,slot))return R::Unsafe;
    }
    if(op.delegated)return bindings.ReadField(Inputs(),op.field,repair);
    void *slot=nullptr;std::size_t extent=0;
    if(op.staged_element!=S::kClientPlanNone){
      if(!Destination(op.field,slot,extent)||!ClientStagedFieldAttached(fields[op.field].native,slot))return R::Unsafe;
      slot=ClientStagedFieldElement(fields[op.field].native,op.staged_element);
    }else {
      if(!Destination(op.field,slot,extent))return R::Unsafe;
      const auto offset=op.element==S::kClientPlanNone?0:std::size_t(op.element)*op.write.extent;
      if(offset>extent||op.write.size>extent-offset)return R::Unsafe;
      slot=static_cast<std::byte *>(slot)+offset;
    }
    if(slot!=op.write.destination)return R::Unsafe;
    if(ClientAtomicWriteCheck(op.write))return R::Match;
    if(op.write.kind==ClientAtomicWriteKind::FieldAttachment)return R::Unsafe;
    if(!repair)return R::Drift;
    if(ClientAtomicWriteApply(op.write))return R::Match;
    if(op.write.kind==ClientAtomicWriteKind::EntityReference)uncertain=true;
    return R::Unsafe;
  }
  S::ClientStateNativeStep RepairNow(S::BodyStream stream,std::size_t &cursor,bool costumes) noexcept {
    using R=ClientAtomicReadback;
    bool repaired=false;
    try{
      const auto group=std::pair{costumes,stream};
      const auto first=std::lower_bound(readback.begin(),readback.end(),group,[](const auto &e,auto s){return std::pair{std::get<0>(e.key),std::get<1>(e.key)}<s;});
      const auto last=std::upper_bound(first,readback.end(),group,[](auto s,const auto &e){return s<std::pair{std::get<0>(e.key),std::get<1>(e.key)};});
      const auto count=std::size_t(last-first);if(!count)return Ready(false,"binary_readback_empty");
      std::size_t visited=0;
      for(unsigned blocks=0;blocks<4&&visited<128&&visited<count;++blocks){
        auto at=first+(cursor%count);
        const auto begin=std::lower_bound(first,at,at->key,[](const auto &e,const auto &key){return e.key<key;});
        const auto end=std::upper_bound(at,last,at->key,[](const auto &key,const auto &e){return key<e.key;});
        const auto stop=at+(std::min)(std::size_t(end-at),(std::min)(128-visited,count-visited));
        bool drift=false;
        for(auto i=at;i!=stop;++i){
          const auto result=ReadOp(operations[i->operation],false);
          if(result==R::Unsafe)return Fail("binary_readback_owner_changed");
          drift|=result==R::Drift;
        }
        if(drift){
          repaired=true;
          if(std::size_t(end-begin)>4096)return Fail("binary_readback_repair_budget");
          // 修复前先核整块可写性。任何未跟随的资源／容器替换要求重建场景。
          for(auto i=begin;i!=end;++i)
            if(ReadOp(operations[i->operation],false)==R::Unsafe)return Fail("binary_readback_repair_owner_changed");
          for(auto i=begin;i!=end;++i)
            if(!Current(*batch)||ReadOp(operations[i->operation],true)==R::Unsafe)return Fail("binary_readback_partial_repair");
          Trace("client","event=binary_native_block_repaired stream=%u record=%u block=%u",unsigned(stream),std::get<2>(begin->key),unsigned(std::get<3>(begin->key)));
        }
        visited+=std::size_t(stop-at);cursor=std::size_t(stop-first);
      }
    }catch(...){return Fail("binary_readback_exception");}
    return Ready(repaired,"binary_readback_current");
  }
  S::ClientStateNativeStep DrainNow() noexcept {
    if(!install_entered&&!publish_entered)return Ready(false,"binary_atomic_staged_only");
    if(uncertain)return Failed(written,"binary_atomic_uncertain_reference_retained");
    // 退休责任取当前保管账，不按本批当初Join／Reuse推断；成功后根可跨批转交。
    if(!preparation.VisitRetirementObjects(*batch,this,[](void *p,std::uint32_t n,bool retire_root)noexcept{
      auto &s=*static_cast<Impl *>(p);if(n>=s.objects.size())return false;s.objects[n].retire_root=retire_root;return true;
    }))return Failed(written,"binary_atomic_retirement_ownership_unavailable");
    if(!quiesced){if(!bindings.Quiesce(*batch,Inputs()))return {false,false,false,"binary_atomic_quiescing"};quiesced=true;}
    // 装载／选择器可能交换图层数组。按照安装顺序统一反向拆除普通与特殊字段。
    for(std::size_t n=operations.size();n>0;--n){auto &op=operations[n-1];if(!op.done||op.restored)continue;
      if(op.delegated){
        if(!bindings.UndoField(*batch,Inputs(),op.field))return Failed(written,"binary_atomic_special_retirement_failed");
        op.restored=true;continue;
      }
      if(published&&op.write.kind!=ClientAtomicWriteKind::FieldAttachment){op.restored=true;continue;}
      if(!bindings.RetirementCurrent(*batch,Inputs()))return Failed(written,"binary_atomic_retirement_use_changed");
      if(op.restore_entered)return Failed(written,"binary_atomic_restore_prefix_retained");op.restore_entered=true;
      if(!ClientAtomicWriteRestore(op.write,originals.data()+op.original,op.write.size))return Failed(written,"binary_atomic_restore_failed_retained");op.restored=true;
    }
    if(!retired){if(!bindings.Retire(*batch,Inputs()))return {false,false,false,"binary_atomic_native_retiring"};retired=true;}
    if(final_scene&&root_scalars&&!root_scalars->Restore(roots))return Failed(written,"binary_atomic_root_scalar_retirement");
    return Ready(written,"binary_atomic_native_drained");
  }
};
ClientSceneAtomicInstaller::ClientSceneAtomicInstaller(std::shared_ptr<S::ClientStateMemory> memory,ClientSceneMainPreparation &p) {
  if(!memory)throw S::ClientTableError(S::ClientTableFailure::Internal,"binary_atomic_memory");
  impl_=std::make_unique<Impl>(std::move(memory),p,nullptr);
}
ClientSceneAtomicInstaller::ClientSceneAtomicInstaller(std::shared_ptr<S::ClientStateMemory> memory,ClientSceneMainPreparation &p,ClientAtomicInstallBindings &b) {
  if(!memory)throw S::ClientTableError(S::ClientTableFailure::Internal,"binary_atomic_memory");impl_=std::make_unique<Impl>(std::move(memory),p,&b);
}
ClientSceneAtomicInstaller::ClientSceneAtomicInstaller(std::shared_ptr<S::ClientStateMemory> memory,ClientSceneMainPreparation &p,
    std::shared_ptr<RootScalars> scalars){
  if(!memory)throw S::ClientTableError(S::ClientTableFailure::Internal,"binary_atomic_memory");
  impl_=std::make_unique<Impl>(std::move(memory),p,nullptr,std::move(scalars));
}
ClientSceneAtomicInstaller::~ClientSceneAtomicInstaller(){if(impl_&&(!impl_->Thread()||impl_->batch))impl_.release();}
S::ClientStateNativeStep ClientSceneAtomicInstaller::Stage(S::ClientStateApplyBatchRef b) noexcept {
  auto &s=*impl_;if(!s.Thread()||!b||!b->target.install||b->target.install->Memory()!=s.memory||s.invalid)return Failed(s.written,"binary_atomic_stage_lifecycle");
  if(s.batch&&s.batch!=b)return Failed(s.written,"binary_atomic_batch_busy");
  if(!s.batch){s.batch=std::move(b);s.scene=std::dynamic_pointer_cast<const ClientSceneInstallPlan>(s.batch->native);
    if(!s.scene||&s.scene->State()!=s.batch->target.install.get())return s.Fail("binary_atomic_plan_type");}
  if(!s.Current(*s.batch))return s.Fail("binary_atomic_stage_revoked");
  if(s.staged)return Ready(false,"binary_atomic_staged");
  const auto prepared=s.preparation.Prepare(s.batch);if(!prepared.ready||prepared.failed)return prepared;
  const auto roots=s.preparation.StageObjects(*s.batch);if(!roots.ready||roots.failed)return roots;
  const auto fields=s.preparation.StageFields(*s.batch);if(!fields.ready||fields.failed)return fields;
  try {
    if(s.objects.empty()&&s.operations.empty())s.Describe();
    const auto preflight=s.bindings.Preflight(*s.batch,s.Inputs());
    if(preflight.written){s.written=true;s.uncertain=true;return s.Fail("binary_atomic_preflight_wrote_world");}
    if(preflight.failed)return s.Fail(preflight.reason);
    if(!preflight.ready)return preflight;
    if(!s.Current(*s.batch))return s.Fail("binary_atomic_preflight_revoked");
    s.roots=ClientPreparationRoots();s.vm=ClientLuaStateIdentity();
    // 只在首次构造这批指令时保存已定址的 Game／Room 标量；热写区不扩容。
    if(s.root_scalars)for(const auto &op:s.operations)if(!op.late&&!op.delegated&&!op.duplicate&&
        (op.write.kind==ClientAtomicWriteKind::Bytes||op.write.kind==ClientAtomicWriteKind::Masked32)){
      auto original=std::span(s.originals).subspan(op.original,op.write.size);
      if(!ClientAtomicWritePreflight(op.write,original.data(),original.size())||
         !s.root_scalars->Save(s.roots,op.write.destination,original))return s.Fail("binary_atomic_root_scalar_budget");
    }
    if(!s.preparation.HoldInstallation(*s.batch,&s))return s.Fail("binary_atomic_installation_lease");s.lease=true;
    s.staged=true;s.reason="binary_atomic_staged";return Ready(false,s.reason);
  }catch(const S::ClientShadowError &e){return s.Fail(e.what());}
   catch(const S::ClientTableError &){return s.Fail("binary_atomic_budget");}
   catch(...){return s.Fail("binary_atomic_description_exception");}
}
S::ClientStateNativeStep ClientSceneAtomicInstaller::Install(const S::ClientStateApplyBatch &b) noexcept {
  auto &s=*impl_;if(!s.Thread())return Failed(false,"binary_atomic_wrong_thread");if(s.batch.get()!=&b)return Failed(false,"binary_atomic_other_ticket");
  if(!s.Current(b)||!s.staged)return s.Fail("binary_atomic_install_not_current");
  if(s.installed)return Ready(s.written,"binary_atomic_installed");if(s.install_entered)return s.Fail("binary_atomic_install_prefix_retained");
  s.manager_token=OwnerToken(Owner::Manager);if(!s.manager_token)return s.Fail("binary_atomic_manager_token");
  const auto result=s.preparation.ApplyInstallation(b,&s,false,&s,[](void *p)noexcept{return static_cast<Impl *>(p)->InstallNow();});
  s.written|=result.written;if(result.failed)return s.Fail(result.reason);return result;
}
S::ClientStateNativeStep ClientSceneAtomicInstaller::Publish(const S::ClientStateApplyBatch &b) noexcept {
  auto &s=*impl_;if(!s.Thread())return Failed(false,"binary_atomic_wrong_thread");if(s.batch.get()!=&b)return Failed(false,"binary_atomic_other_ticket");
  if(!s.Current(b)||!s.installed)return s.Fail("binary_atomic_publish_not_installed");
  if(s.published)return Ready(s.written,"binary_atomic_published");
  if(s.publish_entered||!s.manager_token||s.manager_token!=OwnerToken(Owner::Manager))return s.Fail("binary_atomic_publish_crossed_manager_window");
  const auto result=s.preparation.ApplyInstallation(b,&s,false,&s,[](void *p)noexcept{return static_cast<Impl *>(p)->PublishNow();});s.written|=result.written;
  if(!result.ready||result.failed)return s.Fail(result.reason);
  if(!s.preparation.CommitInstallation(b,&s))return s.Fail("binary_atomic_custody_commit_failed");
  const auto attachments=s.preparation.ApplyInstallation(b,&s,false,&s,[](void *p)noexcept{return static_cast<Impl *>(p)->CommitFields();});
  if(!attachments.ready||attachments.failed){s.uncertain=true;return s.Fail(attachments.reason);}
  s.bindings.Committed(b,s.Inputs());
  s.published=true;s.reason="binary_atomic_published";return Ready(s.written,s.reason);
}
void ClientSceneAtomicInstaller::Invalidate(const S::ClientStateApplyBatch &b,const char *why) noexcept {
  auto &s=*impl_;if(s.Thread()&&s.batch.get()==&b)s.Fail(why);
}
S::ClientStateNativeStep ClientSceneAtomicInstaller::Repair(const S::ClientStateApplyBatch &b,S::BodyStream stream,std::size_t &cursor,bool costumes) noexcept {
  auto &s=*impl_;
  if(!s.published||!s.Current(b))return Failed(false,"binary_readback_target_not_current");
  struct Work {Impl &owner;S::BodyStream stream;std::size_t &cursor;bool costumes;} work{s,stream,cursor,costumes};
  return s.preparation.ApplyInstallation(b,&s,false,&work,[](void *p) noexcept {
    const auto &w=*static_cast<Work *>(p);return w.owner.RepairNow(w.stream,w.cursor,w.costumes);
  });
}
bool ClientSceneAtomicInstaller::Drain(const S::ClientStateApplyBatch &b,bool final_scene) noexcept {
  auto &s=*impl_;if(!s.Thread())return false;if(!s.batch)return true;if(s.batch.get()!=&b)return false;
  s.final_scene|=final_scene;
  if(s.uncertain&&!s.lease){s.reason="binary_atomic_unleased_write_retained";return false;}
  // 已提交使用者接管根，未提交候选仍阻止旧owner退休；不因长寿命根固定整个历史批次。
  if(s.lease&&!s.lease_released&&!s.preparation.BeginInstallationRetirement(b)){s.reason="binary_atomic_root_retirement_waiting";return false;}
  if(!s.invalid&&s.written)s.Fail("binary_atomic_retiring");
  if(s.lease&&!s.lease_released){const auto result=s.preparation.ApplyInstallation(b,&s,true,&s,[](void *p)noexcept{return static_cast<Impl *>(p)->DrainNow();});
    s.reason=result.reason;if(!result.ready||result.failed)return false;
    if(!s.preparation.ReleaseInstallation(b,&s))return false;s.lease_released=true;}
  if(!s.preparation.DrainBatch(b))return false;
  s.batch.reset();s.scene.reset();s.objects.clear();s.fields.clear();s.resources.clear();s.operations.clear();s.values.clear();s.originals.clear();
  s.paths.clear();s.field_index.clear();s.normalized.clear();s.write_index.clear();s.resolved_writes.clear();
  s.readback.clear();s.field_attachments.clear();
  s.lease=false;s.reason="binary_atomic_drained";return true;
}
bool ClientSceneAtomicInstaller::Current(const S::ClientStateApplyBatch &b)const noexcept{return impl_->Current(b);}
bool ClientSceneAtomicInstaller::Installed(const S::ClientStateApplyBatch &b)const noexcept{return impl_->installed&&impl_->Current(b);}
bool ClientSceneAtomicInstaller::Published(const S::ClientStateApplyBatch &b)const noexcept{return impl_->published&&impl_->Current(b);}
bool ClientSceneAtomicInstaller::Drained()const noexcept{return impl_->Thread()&&!impl_->batch;}
const char *ClientSceneAtomicInstaller::Reason()const noexcept{return impl_->Thread()?impl_->reason:"binary_atomic_wrong_thread";}
} // namespace cs::game
