#include "game/client/scene/client_atomic_install.h"
#include "game/client/scene/client_atomic_bindings.h"
#include "game/client/scene/client_lifetime.h"
#include "game/client/scene/client_read_window.h"
#include "game/common/bridge.h"
#include "game/common/context.h"
#include "game/tests/client_entity_e2e.h"
#include "runtime.h"
#include "state/client_memory_resource.h"
#include <algorithm>
#include <cstring>
#include <map>
#include <tuple>

namespace cs::game {
namespace {
namespace S=state;
#include "state_host_fields.inc"
namespace F=host_field;
S::ClientStateNativeStep Ready(bool written,const char *why){return {true,written,false,why};}
S::ClientStateNativeStep Failed(bool written,const char *why){return {false,written,true,why};}
[[noreturn]]void Bad(const char *why){throw S::ClientShadowError(S::StateReadiness::InvalidEncoding,why);}
template<class T>T Number(std::span<const std::byte> bytes){if(bytes.size()!=sizeof(T))Bad("binary_atomic_scalar_width");T v;std::memcpy(&v,bytes.data(),sizeof(v));return v;}
bool Rows(ClientStagedFieldKind k){return k==ClientStagedFieldKind::Rows;}
// 只绑定已核的原位同宽转换；其余转换必须有具名owner，不能仅因宽度相同就复制。
bool RawConversion(std::string_view h){return h=="playercostumemap_layer_id_read"||h=="playeractiveitemdesc_charge_read"||
  h=="playeractiveitemdesc_battery_charge_read"||h=="playeractiveitemdesc_sub_charge_read"||h=="playeractiveitemdesc_partial_charge_read"||
  h=="playeractiveitemdesc_var_data_read"||h=="playertemporaryeffect_cooldown_read"||h=="drawrecordnode_main_animation_frame_read"||
  h=="drawrecordnode_second_animation_frame_read"||h=="entityplayer_inventory_suppressed_read"||
  h=="f_x_mask_read"||h=="f_x_blend_mode_read"||h=="ray_phase_read"||
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
  bool NeedsSnapshot(const ClientNativeRoots &roots,void *p,std::size_t size) const noexcept {
    return ClientAtomicRootScalar(roots,p,size)&&!saved_.contains({reinterpret_cast<std::uintptr_t>(p),std::uint32_t(size)});
  }
  bool Save(void *p,std::span<const std::byte> original) noexcept {
    const Index index{reinterpret_cast<std::uintptr_t>(p),std::uint32_t(original.size())};
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
    // 数组起点定位仍用单元素步长；合并纯拷贝后write.extent是整段长度。
    std::size_t element_stride=0;
    bool delegated=false,late=false,display=false,duplicate=false,done=false,restore_entered=false,restored=false;
    bool unchanged=false,borrowed_value=false;
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
  S::ClientStateVector<std::uint32_t> resolved_writes,resolved_scratch;
  S::ClientStateVector<std::uint32_t> field_attachments;
  // 描述完成后，晚绑定到场景里的访问按当前 owner 核定，不能沿用暂存阶段的私有借用。
  bool described=false;
  std::array<bool,2> late_writes{};
  // costume独立分组：两流共享容器随最新批次退休，旧流只检查仍属于自己的普通字段。
  // 切换写入器时沿用Manager游标，避免高频收包让尾部饥饿。
  // 完整保留 costume / stream / record / block 的字典序，不截断记录号。
  using ReadbackKey=std::uint64_t;
  struct ReadbackEntry {ReadbackKey key;std::uint32_t operation;};
  S::ClientStateVector<ReadbackEntry> readback;
  // 计划不可变；缓存最后一个 object 对同一 canonical 字段的归属，不含原生地址。
  struct FieldOwner {std::uint32_t object=S::kClientPlanNone,field=S::kClientPlanNone;};
  S::ClientStateVector<FieldOwner> field_owners;
  // 每条数字路径的静态归属只解析一次；不缓存原生地址或跨窗口有效性。
  struct PathFacts {
    std::uint8_t root_block=0;
    bool late=false,display=false,costume=false;
  };
  S::ClientStateVector<PathFacts> path_facts;
  // 仅显式纯读窗口复用父链。退出窗口即清空，写入／回调后的地址重新解析。
  struct InstanceAddress {void *address=nullptr;std::size_t extent=0;std::uint32_t object=S::kClientPlanNone;};
  S::ClientStateVector<InstanceAddress> description_addresses;
  // 仅登记本纯读段实际访问的父路径；退出时逐项清空，不反复初始化全部叶路径。
  S::ClientStateVector<std::uint32_t> description_touched;
  ClientNativeReadWindow *description_reads=nullptr;
  ClientNativeRoots roots{};std::uint64_t manager_token=0;
  bool staged=false,lease=false,install_entered=false,installed=false,display_installed=false,publish_entered=false,published=false,written=false;
  bool deferred_display=false;
  bool invalid=false,quiesced=false,retired=false,lease_released=false,uncertain=false,notified=false;
  bool final_scene=false;
  std::uint64_t read_checks=0,read_bytes=0;
  const char *reason="binary_atomic_idle";
  std::array<char,192> failure{};
  Impl(std::shared_ptr<S::ClientStateMemory> m,ClientSceneMainPreparation &p,ClientAtomicInstallBindings *b,std::shared_ptr<RootScalars> scalars={}):
    memory(std::move(m)),preparation(p),owned_bindings(b?nullptr:ClientMakeAtomicInstallBindings(memory,p)),
    bindings(b?*b:*owned_bindings),root_scalars(std::move(scalars)),
    objects(S::ClientStateAllocator<ClientAtomicObject>(memory)),fields(S::ClientStateAllocator<ClientAtomicField>(memory)),
    resources(S::ClientStateAllocator<ClientAtomicResource>(memory)),operations(S::ClientStateAllocator<Op>(memory)),
    values(S::ClientStateAllocator<std::byte>(memory)),originals(S::ClientStateAllocator<std::byte>(memory)),
    resolved_writes(S::ClientStateAllocator<std::uint32_t>(memory)),
    resolved_scratch(S::ClientStateAllocator<std::uint32_t>(memory)),
    field_attachments(S::ClientStateAllocator<std::uint32_t>(memory)),
    readback(S::ClientStateAllocator<ReadbackEntry>(memory)),
    field_owners(S::ClientStateAllocator<FieldOwner>(memory)),
    path_facts(S::ClientStateAllocator<PathFacts>(memory)),
    description_addresses(S::ClientStateAllocator<InstanceAddress>(memory)),
    description_touched(S::ClientStateAllocator<std::uint32_t>(memory)){}
  bool Thread()const noexcept{return thread==std::this_thread::get_id();}
  // Describe已预留全部路径容量；后续纯读批次只借用该容量，不在安装或修复中分配。
  // 此作用域不得包含原生setter、Init、显示推进或修复写入。
  struct PurePathWindow {
    Impl &owner;ClientNativeReadWindow reads;bool ready=false;
    explicit PurePathWindow(Impl &s) noexcept:owner(s){
      const auto count=s.scene->State().Paths().size();
      if(s.description_reads||!s.description_touched.empty()||s.description_addresses.size()!=count||
          s.description_touched.capacity()<count)return;
      s.description_reads=&reads;ready=true;
    }
    ~PurePathWindow(){if(ready)owner.ClearDescriptionWindow();}
  };
  // 地址见证仍在每个窗口结束时实际清零，绝不跨写入、回调或Manager边界复用。
  void ClearDescriptionWindow() noexcept {
    description_reads=nullptr;
    for(const auto path:description_touched)description_addresses[path]={};
    description_touched.clear();
  }
  struct ReadMetrics {
    Impl &owner;std::uint8_t streams;std::uint64_t checks,bytes;
    S::ClientApplyTimer time;
    ReadMetrics(Impl &s,std::uint8_t mask):owner(s),streams(mask),checks(s.read_checks),bytes(s.read_bytes),
      time(s.memory->ApplyMetrics(),mask,S::ClientApplyStage::Readback) {}
    ~ReadMetrics(){
      auto &metrics=owner.memory->ApplyMetrics();
      metrics.Volume(streams,S::ClientApplyVolume::Checks,owner.read_checks-checks);
      metrics.Volume(streams,S::ClientApplyVolume::ReadBytes,owner.read_bytes-bytes);
    }
  };
  // 首因使用生成字段和完整成员身份，可跨移除旧安装器后继续定位。
  void FieldIssue(const Op &op,const char *phase,const char *reason)const noexcept {
    const auto &p=scene->State();const auto &f=p.Fields()[op.field];const auto &r=p.Records()[f.record];
    const auto &field=S::Fields()[f.definition];
    S::ClientApplyMetrics::Issue issue;issue.route=batch->ticket.route.route;issue.ticket=batch->ticket.serial;
    issue.target_version=r.context.target_version;issue.resource_epoch=r.context.resource_epoch;
    issue.layout=field.layout;issue.field=field.index;
    if(r.object<p.Objects().size()){
      const auto &id=p.Objects()[r.object].member.identity;issue.member=id.id;issue.instance=id.use;
    }
    auto path=f.path;
    while(p.Paths()[path].parent!=S::kClientPlanNone)path=p.Paths()[path].parent;
    const auto &root=p.Paths()[path].step;
    if(const auto *definition=S::FindField(root.layout,root.field))issue.block=definition->block;
    issue.expected=reinterpret_cast<std::uintptr_t>(op.write.destination);
    memory->ApplyMetrics().FirstIssue(std::uint8_t(1u<<unsigned(r.stream)),issue,phase,reason);
  }
  ClientAtomicInputs Inputs(){
    return {*scene,objects,fields,resources,this,[](void *owner,std::uint32_t record,std::uint32_t path,void *&address,std::size_t &extent){
      return static_cast<Impl *>(owner)->Instance(record,path,address,extent);
    },description_reads};
  }
  bool Current(const S::ClientStateApplyBatch &b)const noexcept {
    return Thread()&&batch.get()==&b&&!invalid&&!uncertain&&preparation.Current(b)&&bindings.Current(b);
  }
  S::ClientStateNativeStep Fail(const char *why) noexcept {
    if(!why)why="binary_atomic_unspecified_failure";const auto size=(std::min)(std::strlen(why),failure.size()-1);
    std::memmove(failure.data(),why,size);failure[size]=0;reason=failure.data();invalid=true;
    if(written&&batch&&!notified){notified=true;bindings.Invalidate(*batch,reason);}return Failed(written,reason);
  }
  std::uint32_t FieldAt(std::uint32_t object,std::uint32_t path) {
    const auto key=scene->State().Paths()[path].field_path;if(key==S::kClientPlanNone)return S::kClientPlanNone;
    auto &cached=field_owners[key];
    if(cached.object!=object){
      cached={object,scene->State().CanonicalField(object,key)};
    }
    return cached.field;
  }
  void *FieldElement(ClientStagedField *owner,std::uint32_t element) const noexcept {
    // Describe 已取得全批只读见证；安装、回读和退休阶段按当前承载重新核元素。
    return described?ClientStagedFieldElement(owner,element):ClientInspectedFieldElement(owner,element);
  }
  // 父链逐层定位；动态容器元素取候选私有承载，固定内嵌对象保留其真实地址。
  bool Instance(std::uint32_t record,std::uint32_t path,void *&address,std::size_t &extent,unsigned depth=0) {
    const auto object=scene->State().Records()[record].object;
    if(description_reads&&path<description_addresses.size()){
      auto &cached=description_addresses[path];
      if(cached.address&&cached.object==object){address=cached.address;extent=cached.extent;return true;}
      if(!ResolveInstance(record,path,address,extent,depth))return false;
      if(!cached.address)description_touched.push_back(path);
      cached={address,extent,object};return true;
    }
    return ResolveInstance(record,path,address,extent,depth);
  }
  bool ResolveInstance(std::uint32_t record,std::uint32_t path,void *&address,std::size_t &extent,unsigned depth) {
    const auto &plan=scene->State();const auto object=plan.Records()[record].object;
    if(object>=objects.size()||depth>=S::kCodecMaxDepth)return false;
    if(path==S::kClientPlanNone){address=objects[object].address;const auto *l=S::FindLayout(plan.Objects()[object].member.identity.layout);extent=l?l->native_size:0;return address&&extent;}
    const auto &p=plan.Paths()[path];const auto *d=S::FindField(p.step.layout,p.step.field);if(!d||(!d->nested_layout&&!p.step.choice_layout))return false;
    const auto i=FieldAt(object,path);
    if(i!=S::kClientPlanNone) {auto &f=fields[i];
      if(f.native&&f.Input().kind!=ClientStagedFieldKind::Group&&!Rows(f.Input().kind)) {
        const auto element=p.step.element==S::kNullIndex?0u:p.step.element;address=FieldElement(f.native,element);
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
    return path!=S::kClientPlanNone&&path_facts[path].late;
  }
  // 构图会重置Backdrop Sprite，地狱初始化也会覆盖活动标量，因此这些路径最后安装。
  bool DisplayField(std::uint32_t path)const noexcept {
    return path!=S::kClientPlanNone&&path_facts[path].display;
  }
  bool Destination(std::uint32_t field,void *&address,std::size_t &extent) {
    const auto &plan=scene->State();const auto &f=plan.Fields()[field];const auto &d=S::Fields()[f.definition];const auto &p=plan.Paths()[f.path];
    const auto kind=fields[field].Input().kind;const bool records=kind==ClientStagedFieldKind::Records;
    if((d.source!=S::FieldSource::Object&&!records)||S::FindLayout(d.layout)->protocol)return false;
    const auto size=records?12u:kind==ClientStagedFieldKind::Seeds?8u:d.native_size;
    const auto offset=records?0u:d.native_offset;
    void *parent=nullptr;std::size_t parent_size=0;if(!Instance(f.record,p.parent,parent,parent_size)||offset>parent_size||size>parent_size-offset)return false;
    address=static_cast<std::byte *>(parent)+offset;extent=size;return address&&extent;
  }
  // 候选指令在最终数组中原位构造；完成前不调用Init或写原生内存。
  // 任何后续校验失败仍使整个候选失败；暂存预算与压缩后的262144条预算保持。
  Op &NewOperation() {
    if(operations.size()>=scene->State().Fields().size()+scene->State().Values().size())Bad("binary_atomic_operation_budget");
    const auto order=std::uint32_t(operations.size());
    auto &op=operations.emplace_back();op.order=order;return op;
  }
  void Add(Op &op,std::span<const std::byte> bytes={},bool borrow=false) {
    if(!op.delegated){
      const auto &plan=scene->State();const auto &f=plan.Fields()[op.field];const auto &d=S::Fields()[f.definition];
      const auto &record=plan.Records()[f.record];
      // 普通 Body 无 World 的 Init／显示回调。同一根或已借用的标量 vector
      // 可沿用未变值。嵌套 Sprite 和依赖帧号的转换仍经原写入器。
      op.unchanged=Pure(op)&&!op.late&&!scene->Display()&&record.context.base_version&&
        record.stream==S::BodyStream::Body&&plan.Paths()[f.path].parent==S::kClientPlanNone&&
        !(record.changed_mask&(std::uint32_t(1)<<d.block))&&
        (op.staged_element==S::kClientPlanNone||fields[op.field].Input().retained)&&
        (d.operation==S::Operation::Copy||d.operation==S::Operation::Container||op.write.kind==ClientAtomicWriteKind::Masked32)&&
        (plan.Objects()[record.object].action==S::ClientInstallAction::Reuse||plan.Objects()[record.object].action==S::ClientInstallAction::Remap);
      if(!op.write.size||op.write.size>op.write.extent)Bad("binary_atomic_destination_range");
      if(!op.late){
      if(!op.write.destination)Bad("binary_atomic_destination_range");
      const auto begin=reinterpret_cast<std::uintptr_t>(op.write.destination);if(op.write.size>UINTPTR_MAX-begin)Bad("binary_atomic_address_overflow");
      }
      if(!borrow&&bytes.size()>UINT32_MAX-values.size())Bad("binary_atomic_journal_budget");
      // 原样字段直接借引用表中的字节；只有 ABI 转换需要独立存储。
      op.borrowed_value=borrow;
      if(borrow)op.write.value=bytes.data();
      else {op.value=std::uint32_t(values.size());values.insert(values.end(),bytes.begin(),bytes.end());}
    }
  }
  // 先排序紧凑的地址索引，再核所有原始单元素区间；此时尚未合并数组、写世界或分配撤销日志。
  // 同地址保持原Add顺序，重复投影保留首项。部分重叠或不同目标仍在任何Init之前拒绝。
  void IndexWrites() {
    resolved_writes.clear();resolved_writes.reserve(operations.size());resolved_scratch.reserve(operations.size());
    for(std::uint32_t i=0;i<operations.size();++i)
      if(!operations[i].delegated&&!operations[i].late)resolved_writes.push_back(i);
#ifdef ISAACCS_E2E
    // 仅改候选计划中的地址／掩码，不写原生对象；下一段必须在Init前拒绝重叠与目标冲突。
    const bool overlap=ClientEntityTestFault("binary-write-overlap");
    const bool conflict=ClientEntityTestFault("binary-write-conflict");
    if(overlap||conflict){
      Op *first=nullptr,*second=nullptr;
      for(auto n:resolved_writes)if(Pure(operations[n])&&operations[n].write.size>=2){
        if(!first)first=&operations[n];else {second=&operations[n];break;}
      }
      if(!second)Bad("binary_e2e_write_candidates_missing");
      second->write.destination=static_cast<std::byte *>(first->write.destination)+(overlap?1:0);
      second->write.size=overlap?1:first->write.size;
      second->write.mask=first->write.mask^1u;
    }
#endif
    SortWriteAddresses();
    std::uint32_t previous=S::kClientPlanNone;
    for(auto n:resolved_writes){
      auto &op=operations[n];const auto begin=reinterpret_cast<std::uintptr_t>(op.write.destination);
      if(previous!=S::kClientPlanNone){
        const auto &old=operations[previous];const auto other=reinterpret_cast<std::uintptr_t>(old.write.destination);
        if(begin<other+old.write.size){
          bool same=begin==other&&op.write.size==old.write.size&&op.write.kind==old.write.kind&&op.write.mask==old.write.mask&&
            op.write.reference==old.write.reference&&op.write.field==old.write.field;
          if(same&&Pure(op)){
            const auto *next=op.borrowed_value?op.write.value:values.data()+op.value;
            const auto *prior=old.borrowed_value?old.write.value:values.data()+old.value;
            same=std::memcmp(next,prior,op.write.size)==0;
          }
          if(!same)Bad("binary_atomic_projection_write_conflict");
          op.duplicate=true;continue;
        }
      }
      previous=n;
    }
    // 按原插入顺序压缩，再合并同字段的连续元素。重复投影不占日志，也不改变回调顺序。
    std::size_t kept=0;
    for(std::size_t i=0;i<operations.size();++i){
      if(kept>=262144)Bad("binary_atomic_operation_budget");
      auto &op=operations[i];if(op.duplicate)continue;
      if(!op.delegated){
        const auto undo=op.write.kind==ClientAtomicWriteKind::FieldAttachment||op.unchanged?0:op.write.size;
        if(undo>UINT32_MAX-originals.size())Bad("binary_atomic_journal_budget");
        if(kept&&!op.late&&op.borrowed_value&&op.write.kind==ClientAtomicWriteKind::Bytes&&
            (op.element!=S::kClientPlanNone||op.staged_element!=S::kClientPlanNone)){
          auto &old=operations[kept-1];
          if(!old.delegated&&!old.late&&old.borrowed_value&&old.write.kind==ClientAtomicWriteKind::Bytes&&
              old.field==op.field&&old.phase==op.phase&&old.unchanged==op.unchanged&&old.element_stride==op.element_stride&&
              reinterpret_cast<std::uintptr_t>(old.write.destination)+old.write.size==reinterpret_cast<std::uintptr_t>(op.write.destination)&&
              old.write.value+old.write.size==op.write.value&&(old.unchanged||old.original+old.write.size==originals.size())){
            originals.resize(originals.size()+undo);old.write.size+=op.write.size;old.write.extent=old.write.size;continue;
          }
        }
        op.original=std::uint32_t(originals.size());originals.resize(originals.size()+undo);
      }
      op.order=std::uint32_t(kept);
      // 写入位置只向前移动，不覆盖尚未访问的指令；没有空洞时不复制自身。
      if(kept!=i)operations[kept]=op;
      ++kept;
    }
    operations.resize(kept);resolved_writes.clear();resolved_scratch.clear();
  }
  // 稳定地址基数排序只搬运整数索引；两个有界工作区与操作数组一起计入Values池。
  void SortWriteAddresses() {
    if(resolved_writes.empty())return;
    const auto address=[&](std::uint32_t n){return reinterpret_cast<std::uintptr_t>(operations[n].write.destination);};
    std::uintptr_t varying=0;const auto first=address(resolved_writes.front());
    for(auto n:resolved_writes)varying|=address(n)^first;
    if(resolved_scratch.capacity()<resolved_writes.size())Bad("binary_atomic_resolved_capacity");
    resolved_scratch.resize(resolved_writes.size());
    for(unsigned shift=0;shift<sizeof(std::uintptr_t)*8;shift+=8){
      if(!((varying>>shift)&255))continue;
      std::array<std::size_t,256> offsets{};
      for(auto n:resolved_writes)++offsets[(address(n)>>shift)&255];
      std::size_t next=0;for(auto &offset:offsets){const auto count=offset;offset=next;next+=count;}
      for(auto n:resolved_writes)resolved_scratch[offsets[(address(n)>>shift)&255]++]=n;
      resolved_writes.swap(resolved_scratch);
    }
    resolved_scratch.clear();
  }
  void Leaf(std::uint32_t field,const S::ClientInstallValue &v) {
    const auto &plan=scene->State();const auto &f=plan.Fields()[field];const auto &d=S::Fields()[f.definition];const auto bytes=plan.Bytes(v.bytes);
    const auto parent=plan.Paths()[v.path].parent;
    if(parent!=S::kClientPlanNone){const auto i=v.parent_field;
      if(i!=S::kClientPlanNone){const auto kind=fields[i].Input().kind;
        if((kind==ClientStagedFieldKind::Map&&d.layout==177)||(kind==ClientStagedFieldKind::Innate&&d.layout==178)||
           (kind==ClientStagedFieldKind::Seeds&&d.layout==238))return;}}
    const bool late=Late(parent);
    void *address=nullptr;std::size_t extent=0;
    std::uint32_t inline_element=S::kClientPlanNone,staged_element=S::kClientPlanNone;
    const auto &staged_field=fields[field];
    if(d.operation==S::Operation::Container) {
      if(staged_field.staged){const auto kind=staged_field.Input().kind;
        // 文本／键／聚合已由StageFields从同一叶构造，键不可在有序容器中再次原位修改。
        if(kind==ClientStagedFieldKind::String||kind==ClientStagedFieldKind::CString||kind==ClientStagedFieldKind::Blocked||
           kind==ClientStagedFieldKind::Map||kind==ClientStagedFieldKind::Innate||kind==ClientStagedFieldKind::Seeds)return;
        const auto element=plan.Paths()[v.path].step.element;if(element==S::kNullIndex)Bad("binary_atomic_scalar_element");
        if(Rows(kind)){
          void *base=nullptr;std::size_t capacity=d.native_size;
          if((!late&&!Destination(field,base,capacity))||std::uint64_t(element+1)*staged_field.Input().stride>capacity)Bad("binary_atomic_inline_element_range");
          inline_element=element;
          if(!late)address=static_cast<std::byte *>(base)+std::size_t(element)*staged_field.Input().stride;
        }else {staged_element=element;address=FieldElement(staged_field.native,element);}
        extent=staged_field.Input().stride;
      }
    }else if(late)extent=d.native_size;
    else if(!Destination(field,address,extent))Bad("binary_atomic_field_address_owner");
    if((!address&&!late)||!extent)Bad("binary_atomic_scalar_storage");
    auto &op=NewOperation();op.field=field;op.phase=f.phase;op.write.destination=address;op.write.extent=extent;
    op.late=late&&!address;op.element=inline_element;op.staged_element=staged_element;op.element_stride=extent;
    // 同批全部路径已在 Describe 的 PathFacts 阶段核过生成字段格式；
    // 叶只使用已验证定义与实际字节，不再为每个元素复制一份未消费的格式结构。
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
    if(output.size()!=extent)Bad("binary_atomic_native_value_width");op.write.size=output.size();Add(op,output,output.data()==bytes.data());
  }
  bool InspectStaged() noexcept {
    if(described&&preparation.StagedCurrent(*batch))return true;
    // 跨窗口或原生回调后重验同一批承载。已描述的操作内含原生地址，换址必须拒绝该候选。
    return preparation.InspectStaged(*batch,this,
      [](void *p,std::uint32_t i,void *address,const S::PresentationMember &identity)noexcept{
        auto &s=*static_cast<Impl *>(p);
        if(i>=s.objects.size())return false;
        if(s.described)return s.objects[i].address==address&&s.objects[i].identity==identity;
        s.objects[i]={address,identity};return true;
      },
      [](void *p,S::ResourceKey key,void *address)noexcept{
        auto &s=*static_cast<Impl *>(p);
        if(s.described){
          const auto at=std::lower_bound(s.resources.begin(),s.resources.end(),key,[](const auto &r,auto k){return r.key<k;});
          return at!=s.resources.end()&&at->key==key&&at->address==address;
        }
        if(s.resources.size()==s.resources.capacity())return false;
        s.resources.push_back({key,address});return true;
      },
      [](void *p,std::uint32_t i,const ClientStagedFieldInput &input,ClientStagedField *native)noexcept{
        auto &s=*static_cast<Impl *>(p);
        if(i>=s.fields.size())return false;
        if(s.described)return s.fields[i].native==native&&s.fields[i].input==&input;
        s.fields[i]={&input,native,true};return true;
      });
  }
  void Describe() {
    // 每批只记六段聚合耗时，不记录逐字段日志；用于区分索引、指令与页检查的真实成本。
    auto stage_start=S::ClientApplyMetrics::Now();
    const auto stage=[&](S::ClientApplyStage value){
      const auto now=S::ClientApplyMetrics::Now();
      memory->ApplyMetrics().Mark(S::StateTargetStreams(scene->State().Target()),value,now-stage_start);stage_start=now;
    };
    const auto &plan=scene->State();objects.resize(plan.Objects().size());fields.resize(plan.Fields().size());
    resources.reserve(plan.Resources().size());
    if(!InspectStaged())Bad("binary_atomic_staged_inputs");
    description_addresses.resize(plan.Paths().size());
    description_touched.reserve(plan.Paths().size());
    PurePathWindow description_window(*this);
    if(!description_window.ready)Bad("binary_atomic_description_path_capacity");
    const auto check_window=[&](std::size_t n){
      if(n%128==0&&!preparation.StagedCurrent(*batch))Bad("binary_atomic_description_revoked");
    };
    std::sort(resources.begin(),resources.end(),[](const auto &a,const auto &b){return a.key<b.key;});
    stage(S::ClientApplyStage::AtomicInputs);
    path_facts.reserve(plan.Paths().size());
    for(const auto &path:plan.Paths()){
      const auto &p=path.step;const auto *definition=S::FindField(p.layout,p.field);
      if(!definition)Bad("binary_atomic_path_definition");
      PathFacts facts;
      if(path.parent!=S::kClientPlanNone){
        if(path.parent>=path_facts.size())Bad("binary_atomic_path_parent_order");
        facts=path_facts[path.parent];
      }else{facts.root_block=definition->block;facts.costume=S::ClientCostumeProjection(p.layout,p.field);}
      facts.late|=bindings.LatePath(*definition);
      facts.display|=(p.layout==F::Room::kLayout&&p.field==F::Room::hell_backdrop)||
        (p.layout==F::Backdrop::kLayout&&(p.field==F::Backdrop::floor_sprite||p.field==F::Backdrop::wall_detail_sprite||
          p.field==F::Backdrop::controls_sprite||p.field==F::Backdrop::controls_buttons_sprite));
      path_facts.push_back(facts);
    }
    // 字段和标量叶分别最多贡献一条指令；按既有上限预留，避免热段反复搬移。
    operations.reserve((std::min)(std::size_t(262144),plan.Fields().size()+plan.Values().size()));
    // 规范父链及完整对象／字段索引由后台同一计划封存；只留本窗口的查询游标。
    field_owners.resize(plan.NormalizedPathCount());
    stage(S::ClientApplyStage::AtomicIndex);
    // 每个完整字段先取得唯一归属，再建立原生指令；委托不能只覆盖一个标量却遗失空／缺席操作。
    S::ClientStateVector<std::uint8_t> delegated{S::ClientStateAllocator<std::uint8_t>(memory)};delegated.resize(plan.Fields().size());
    for(std::uint32_t n=0;n<plan.Fields().size();++n){check_window(n);const auto &f=plan.Fields()[n];const auto &r=plan.Records()[f.record];if(r.event!=S::kClientPlanNone)continue;
      const auto &d=S::Fields()[f.definition];
      if(bindings.Covers(Inputs(),n)){
        delegated[n]=1;
        if(bindings.Installs(Inputs(),n)){auto &op=NewOperation();op.delegated=true;op.field=n;op.phase=f.phase;Add(op);}
        continue;
      }
      if(!f.present&&fields[n].Input().kind!=ClientStagedFieldKind::Records)continue;
      if(d.nested_layout||d.operation==S::Operation::Container||d.operation==S::Operation::Choice){
        auto &field=fields[n];
        if(!field.staged)Bad("binary_atomic_field_not_staged");
        const auto size=ClientInspectedFieldAttachmentSize(field.native);
        if(size){void *address=nullptr;std::size_t extent=size;
          const bool late=Late(plan.Paths()[f.path].parent);
          if((!late&&!Destination(n,address,extent))||extent!=size)Bad("binary_atomic_container_owner_missing");
          auto &op=NewOperation();op.field=n;op.phase=f.phase;op.late=late;
          op.write={ClientAtomicWriteKind::FieldAttachment,address,extent,size,nullptr,nullptr,0,field.native,field.Input().retained};
          Add(op);
        }else if(field.Input().kind==ClientStagedFieldKind::Group){
          // 内嵌Group无独立header；协议聚合和不透明owner必须明确委托，不能空操作冒充安装。
          if(d.source!=S::FieldSource::Object||S::FindLayout(d.layout)->protocol)Bad("binary_atomic_group_owner_missing");
        }else if(Rows(field.Input().kind)){
          S::BodySourceFieldInfo format;if(!S::DescribeBodySourceField(d.layout,d.index,format)||d.source!=S::FieldSource::Object||
            S::FindLayout(d.layout)->protocol||(d.operation==S::Operation::Container&&(format.storage!=S::BodyStorage::Array||
            (!d.fixed_count&&std::string_view(d.handler)!="read_effect_ghost_active_sprites"&&std::string_view(d.handler)!="rail_manager_grid_flags_read")||
            std::uint64_t(field.Input().count)*field.Input().stride>d.native_size)))Bad("binary_atomic_rows_owner_missing");
        }else Bad("binary_atomic_container_attachment_abi");
      }
    }
    stage(S::ClientApplyStage::AtomicFields);
    std::size_t visited=0;
    for(const auto &v:plan.Values()){check_window(visited++);const auto &r=plan.Records()[v.record];if(r.event!=S::kClientPlanNone||v.layout)continue;
      const auto i=v.field;if(i>=plan.Fields().size())Bad("binary_atomic_value_field_missing");
      if(!delegated[i])Leaf(i,v);
    }
    IndexWrites();
    stage(S::ClientApplyStage::AtomicLeaves);
    visited=0;
    for(auto &op:operations){
      check_window(visited++);
      const auto &f=plan.Fields()[op.field];
      const auto &d=S::Fields()[f.definition];
      op.display=DisplayField(f.path);
      late_writes[op.display]|=op.late;
      deferred_display|=op.display;
      const auto order=bindings.Order(d);
      op.priority=op.delegated?order:op.write.kind==ClientAtomicWriteKind::EntityReference?7u:order==0?0u:5u;
    }
    SortOperations();
    field_attachments.resize(fields.size(),S::kClientPlanNone);readback.reserve(operations.size());
    for(std::uint32_t i=0;i<operations.size();++i){
      check_window(i);
      if(!operations[i].delegated&&operations[i].write.kind==ClientAtomicWriteKind::FieldAttachment)
        field_attachments[operations[i].field]=i;
      const auto &f=plan.Fields()[operations[i].field];const auto &facts=path_facts[f.path];
      const auto key=(std::uint64_t(facts.costume)<<48)|(std::uint64_t(plan.Records()[f.record].stream)<<40)|
        (std::uint64_t(f.record)<<8)|facts.root_block;
      readback.push_back({key,i});
    }
    SortReadback();
    stage(S::ClientApplyStage::AtomicOrder);
    // vector可能在构造时重分配；全部封存后才绑定叶缓冲地址，安装区间不再分配这些数组。
    resolved_writes.reserve(operations.size());resolved_scratch.reserve(operations.size());
    ClientAtomicReadRegion read_region;
    visited=0;
    for(auto &op:operations){check_window(visited++);if(op.delegated)continue;
      if(Pure(op)&&!op.borrowed_value)op.write.value=values.data()+op.value;
      if(op.unchanged)continue;
      if(!op.late&&!ClientAtomicWritePreflight(op.write,read_region))Bad("binary_atomic_native_preflight");}
    stage(S::ClientApplyStage::AtomicRanges);
    described=true;
  }
  // 输入已按 operation 升序；稳定的逐字节排序保留同块的操作顺序。
  // 暂存仍计入 Values 池，仅描述期使用，不在写入或修复期间申请。
  void SortReadback() {
    if(readback.size()<2)return;
    // 整批相同的键字节不会改变稳定排序结果；完整64位键仍保留，
    // 仅省略这些恒定字节对应的整轮扫描和搬运，不截断record编号。
    std::uint64_t varying=0;const auto first=readback.front().key;
    for(const auto &entry:readback)varying|=entry.key^first;
    if(!varying)return;
    S::ClientStateVector<ReadbackEntry> temporary{S::ClientStateAllocator<ReadbackEntry>(memory)};
    temporary.resize(readback.size());
    for(unsigned shift=0;shift<=48;shift+=8){
      if(!((varying>>shift)&255))continue;
      std::array<std::size_t,256> offsets{};
      for(const auto &entry:readback)++offsets[(entry.key>>shift)&255];
      std::size_t next=0;for(auto &offset:offsets){const auto count=offset;offset=next;next+=count;}
      for(const auto &entry:readback)temporary[offsets[(entry.key>>shift)&255]++]=entry;
      readback.swap(temporary);
    }
  }
  // 正式绑定器的优先级只有0—7。按原display/priority/phase键稳定分桶，
  // 同桶保持Add顺序，等价于原order尾键；避免排序反复搬动整条原生指令。
  // 自定义绑定器若扩展优先级，仍用通用排序，不截断或猜测顺序。
  void SortOperations() {
    constexpr unsigned phases=unsigned(S::ClientInstallPhase::Retire)+1;
    if(std::any_of(operations.begin(),operations.end(),[](const auto &op){return op.priority>=8||unsigned(op.phase)>=phases;})){
      std::sort(operations.begin(),operations.end(),[](const auto &a,const auto &b){
        return std::tie(a.display,a.priority,a.phase,a.order)<std::tie(b.display,b.priority,b.phase,b.order);
      });return;
    }
    std::array<std::size_t,2*8*phases> offsets{};
    const auto bucket=[](const Op &op){return (unsigned(op.display)*8+op.priority)*phases+unsigned(op.phase);};
    for(const auto &op:operations)++offsets[bucket(op)];
    std::size_t next=0;for(auto &offset:offsets){const auto count=offset;offset=next;next+=count;}
    // 先按原顺序登记每个目标位置的来源，再沿置换环移动；同桶仍稳定，不另建整份大Op数组。
    // 借用随后用于晚绑定区间检查的整数工作区，实际capacity继续由Peer预算计费。
    resolved_writes.resize(operations.size());
    for(std::size_t i=0;i<operations.size();++i)resolved_writes[offsets[bucket(operations[i])]++]=std::uint32_t(i);
    for(std::size_t i=0;i<operations.size();++i){
      if(resolved_writes[i]==i)continue;
      // 每环只暂存首项，其余大结构各移动一次，避免逐交换的三次结构搬运。
      auto saved=std::move(operations[i]);auto destination=i;
      while(resolved_writes[destination]!=i){
        const auto source=resolved_writes[destination];
        operations[destination]=std::move(operations[source]);
        resolved_writes[destination]=std::uint32_t(destination);destination=source;
      }
      operations[destination]=std::move(saved);resolved_writes[destination]=std::uint32_t(destination);
    }
    resolved_writes.clear();
  }
  // 装载稳定图层地址后再查实际区间。所有索引空间已在Stage保留，不在写入中扩容。
  void ResolveWrites(bool display) {
    // 静态区间的重叠和一致投影已由 IndexWrites 核定，只有晚绑定地址加入时才需要重排。
    if(!late_writes[display])return;
    // Init／附件安装之后重新取得地址；这一轮只定位与检查区间，不执行原生回调。
    // 同一纯读窗口内复用父链和页见证，退出前清空，后续写入和回读不能沿用它们。
    // 容量已在Describe保留，真正写段不再分配。
    PurePathWindow window(*this);
    if(!window.ready)Bad("binary_atomic_late_path_capacity");
    resolved_writes.clear();
    for(std::uint32_t i=0;i<operations.size();++i){
      auto &op=operations[i];if(op.delegated||op.display!=display)continue;
      if(op.late){
        void *address=nullptr;std::size_t extent=0;
        if(!Destination(op.field,address,extent))Bad("binary_atomic_late_field_address");
        const auto offset=op.element==S::kClientPlanNone?0:std::size_t(op.element)*op.element_stride;
        if(offset>extent||op.write.size>extent-offset)Bad("binary_atomic_late_field_range");
        op.write.destination=static_cast<std::byte *>(address)+offset;
      }
      resolved_writes.push_back(i);
    }
    const auto address=[&](std::uint32_t n){return reinterpret_cast<std::uintptr_t>(operations[n].write.destination);};
    SortWriteAddresses();
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
  static bool Pure(const Op &op) noexcept {
    return !op.delegated&&(op.write.kind==ClientAtomicWriteKind::Bytes||op.write.kind==ClientAtomicWriteKind::Masked32);
  }
  S::ClientStateNativeStep InstallNow(bool display=false,bool rebuilt=false) noexcept {
    if(!display)install_entered=true;
    // 只细分主安装段，每批四个聚合计时；显示后的单独恢复不混入此分母。
    const auto metric_streams=display?std::uint8_t{0}:S::StateTargetStreams(scene->State().Target());
    bool initialized=false,resolved=false,owners_prepared=false;
    ClientAtomicReadRegion read_region;
    unsigned copies_left=0;
    const auto callback_boundary=[&]{
      read_region.Clear();copies_left=0;return Current(*batch);
    };
    for(std::size_t position=0;position<operations.size();++position){
      auto &op=operations[position];
      if(op.display!=display)continue;
      if(!display&&op.priority>0&&!initialized){
        if(!callback_boundary())return Fail("binary_atomic_lifecycle_during_install");
        S::ClientApplyTimer measured(memory->ApplyMetrics(),metric_streams,S::ClientApplyStage::NativeInitialize);
        if(!Step(bindings.Initialize(*batch,Inputs())))return Failed(written,reason);initialized=true;
      }
      if(!display&&op.priority>0&&!owners_prepared){
        S::ClientApplyTimer measured(memory->ApplyMetrics(),metric_streams,S::ClientApplyStage::NativeOwners);
        if(!Step(bindings.PrepareFieldOwners(*batch,Inputs())))return Failed(written,reason);owners_prepared=true;
      }
      if(op.priority>3&&!resolved){try{
        S::ClientApplyTimer measured(memory->ApplyMetrics(),metric_streams,S::ClientApplyStage::NativeResolve);
        ResolveWrites(display);resolved=true;
      }catch(const S::ClientShadowError &e){return Fail(e.what());}catch(...){return Fail("binary_atomic_late_field_path");}}
      if(op.duplicate||op.unchanged)continue;
      // 已相等的EntityPtr不调用SetReference：它没有原生副作用，也不打断纯访问片。
      // 仍核安装作用域、保存原引用并参与最终读回；失败后按原顺序恢复引用关系。
      if(!op.delegated&&op.write.kind==ClientAtomicWriteKind::EntityReference&&ClientAtomicWriteCheck(op.write)){
        if(!copies_left){
          if(!Current(*batch)||!ClientAtomicCopyWindow())return Fail("binary_atomic_copy_window");
          copies_left=128;
        }
        std::memcpy(originals.data()+op.original,&op.write.reference,sizeof(op.write.reference));
        op.done=true;--copies_left;continue;
      }
      if(Pure(op)){
        // 每片最多 128 项；片内只访问已定界的页，不执行会改变原生承载的调用。
        if(!copies_left){
          // 检查取消和作用域不改变页映射；纯拷贝连续片可保留页见证，真实回调仍清空。
          if(!Current(*batch)||!ClientAtomicCopyWindow())return Fail("binary_atomic_copy_window");
          copies_left=128;
        }
        auto *original=originals.data()+op.original;
        auto range=op.write;auto next=position+1;
        // 同一纯写片内合并相邻原样字节，字段指令和撤销日志仍逐项保留。
        // 三个区间（目标、来源、原值）必须同时连续，且不跨优先级、阶段或回调。
        // 借用的来源还须同record／根块，避免把不同影子分配拼成一个C++缓冲区。
        if(range.kind==ClientAtomicWriteKind::Bytes){
          const auto &plan=scene->State();const auto &field=plan.Fields()[op.field];
          for(;next<operations.size()&&next-position<copies_left;++next){
            const auto &other=operations[next];
            if(other.delegated||other.duplicate||other.unchanged||other.display!=display||
                other.priority!=op.priority||other.phase!=op.phase||other.write.kind!=ClientAtomicWriteKind::Bytes||
                other.borrowed_value!=op.borrowed_value||other.original!=op.original+range.size||
                reinterpret_cast<std::uintptr_t>(other.write.destination)!=reinterpret_cast<std::uintptr_t>(range.destination)+range.size||
                reinterpret_cast<std::uintptr_t>(other.write.value)!=reinterpret_cast<std::uintptr_t>(range.value)+range.size)break;
            const auto &other_field=plan.Fields()[other.field];
            if(op.borrowed_value&&(other_field.record!=field.record||
                path_facts[other_field.path].root_block!=path_facts[field.path].root_block))break;
            if(other.write.size>SIZE_MAX-range.size)break;
            range.size+=other.write.size;range.extent=range.size;
          }
        }
        const auto finish=[&]{
          for(auto i=position;i<next;++i)operations[i].done=true;
          copies_left-=unsigned(next-position);position=next-1;
        };
        // 真实承载已在本批Preflight核定；当前值相等时不再取页信息或重写。
        // 仍登记该时点的原值与done，后续回调之后照常完整读回，失败照常反序恢复。
        // Masked32只恢复mask位，目标字节恰好就是这些位的已观察原值。
        if(ClientAtomicCheckPure(range)){
          std::memcpy(original,range.value,range.size);finish();continue;
        }
        if(!ClientAtomicWriteCapture(range,original,read_region))return Fail("binary_atomic_write_preflight_changed");
        // 整段原值已取得才登记完成；即使SEH捕获部分写入，也能按原字段顺序完整撤销。
        written=true;finish();
        if(!ClientAtomicWritePure(range,original))return Fail("binary_atomic_native_write_failed");
      }else if(!callback_boundary()){
        return Fail("binary_atomic_lifecycle_during_install");
      }else if(op.delegated){
        op.done=true;
        if(!Step(bindings.InstallField(*batch,Inputs(),op.field)))return Failed(written,reason);
      }else{
        const bool attachment=op.write.kind==ClientAtomicWriteKind::FieldAttachment;
        const bool reference=op.write.kind==ClientAtomicWriteKind::EntityReference;
        auto *original=attachment?nullptr:originals.data()+op.original;
        if(!ClientAtomicWriteCapture(op.write,original,read_region))return Fail("binary_atomic_write_preflight_changed");
        if(!op.write.retained_attachment){written=true;op.done=true;}
        if(!ClientAtomicWriteApply(op.write)){if(reference)uncertain=true;return Fail("binary_atomic_native_write_failed");}
        op.done=true;
        if(!callback_boundary())return Fail("binary_atomic_lifecycle_after_callback");
      }
    }
    if(!Current(*batch))return Fail("binary_atomic_lifecycle_during_install");
    {
      S::ClientApplyTimer measured(memory->ApplyMetrics(),metric_streams,S::ClientApplyStage::NativeFinish);
      if(!Step(bindings.FinishFields(Inputs(),S::StateTargetStreams(scene->State().Target()))))return Failed(written,reason);
    }
    if(display){
      ReadMetrics measured(*this,S::StateTargetStreams(scene->State().Target()));
      for(std::size_t i=0;i<operations.size();++i){
        if(i%128==0&&!Current(*batch))return Fail("binary_atomic_display_readback_revoked");
        const auto &op=operations[i];
        if(!op.done||op.duplicate||(!rebuilt&&!op.display))continue;
        // 对门光等依赖最终Room色的标量，按已核写入器恢复后再读回。
        const auto read=ReadOp(op,true,false);
        if(read==ClientAtomicReadback::Unsafe||read==ClientAtomicReadback::Drift)return Fail("binary_atomic_cold_resource_readback");
        if(op.write.kind==ClientAtomicWriteKind::EntityReference&&!Current(*batch))
          return Fail("binary_atomic_lifecycle_after_callback");
      }
      if(!Current(*batch))return Fail("binary_atomic_display_readback_revoked");
      display_installed=true;
      return Ready(written,"binary_atomic_display_fields_installed");
    }
    if(!initialized&&!Step(bindings.Initialize(*batch,Inputs())))return Failed(written,reason);
    if(!Step(bindings.InstallMembers(*batch,Inputs())))return Failed(written,reason);
    installed=true;display_installed=!deferred_display;reason="binary_atomic_installed";return Ready(written,reason);
  }
  S::ClientStateNativeStep PublishNow() noexcept {
    publish_entered=true;if(!Step(bindings.Publish(*batch,Inputs())))return Failed(written,reason);
    ReadMetrics measured(*this,S::StateTargetStreams(scene->State().Target()));
    // Publish回调已结束，以下只有最终值／当前父地址检查，任何后续写入前自动撤销见证。
    PurePathWindow window(*this);
    if(!window.ready)return Fail("binary_atomic_publish_path_capacity");
    // 普通字段只在最后一个可能改写它的 Init／Display／Replay 后做提交读回。
    // 连接型写入自己的即时后置条件独立保留；这里也一次核完待交接的 header。
    for(std::size_t base=0;base<operations.size();base+=128){
      if(!Current(*batch)||!ClientAtomicCopyWindow())return Fail("binary_atomic_publish_revoked");
      const auto end=(std::min)(operations.size(),base+128);
      for(auto i=base;i<end;){
        const auto &op=operations[i];
        if(op.delegated||op.duplicate||op.unchanged){++i;continue;}
        auto range=op.write;
        auto next=i+1;
        // 只把相邻、完整的原样字节合成读区；masked 位、引用和 header 各自比较。
        if(op.done&&!op.late&&range.kind==ClientAtomicWriteKind::Bytes)for(;next<end;++next){
          const auto &other=operations[next];
          if(!other.done||other.late||other.delegated||other.duplicate||other.unchanged||other.write.kind!=ClientAtomicWriteKind::Bytes||
              other.borrowed_value!=op.borrowed_value||(op.borrowed_value&&other.field!=op.field)||
              reinterpret_cast<std::uintptr_t>(other.write.destination)!=reinterpret_cast<std::uintptr_t>(range.destination)+range.size||
              other.write.value!=range.value+range.size)break;
          range.size+=other.write.size;range.extent=range.size;
        }
        const bool current=op.done&&(!op.late||WriteSlotCurrent(op));
        if(current){++read_checks;read_bytes+=range.size;}
        const bool matches=current&&(Pure(op)?ClientAtomicCheckPure(range):ClientAtomicWriteCheck(range));
        if(!matches){
          // 合并比较失败才逐字段找首因。
          while(i+1<next&&ClientAtomicCheckPure(operations[i].write))++i;
          const auto field=operations[i].field;
          FieldIssue(operations[i],"publish_readback","binary_atomic_publish_postcondition");
          Trace("client_error","event=binary_atomic_publish_field field=%u record=%u",field,scene->State().Fields()[field].record);
          return Fail("binary_atomic_publish_postcondition");
        }
        i=next;
      }
    }
    if(!Current(*batch))return Fail("binary_atomic_publish_revoked");
    return Ready(written,"binary_atomic_members_published");
  }
  S::ClientStateNativeStep CommitFields() noexcept {
    for(const auto &op:operations)if(!op.delegated&&!op.duplicate&&!op.write.retained_attachment&&op.write.kind==ClientAtomicWriteKind::FieldAttachment&&
        !ClientStagedFieldCommitCheckedAttachment(op.write.field,op.write.destination)){uncertain=true;return Fail("binary_atomic_attachment_commit_failed");}
    return Ready(written,"binary_atomic_fields_committed");
  }
  // 原生装载和显示可能更换晚绑定承载；提交时必须先确认观察的是当前地址。
  bool WriteSlotCurrent(const Op &op) noexcept {
    try {
      void *slot=nullptr;
      std::size_t extent=0;
      if(!Destination(op.field,slot,extent))return false;
      if(op.staged_element!=S::kClientPlanNone){
        auto *field=fields[op.field].native;
        if(!ClientStagedFieldAttached(field,slot))return false;
        return ClientStagedFieldElement(field,op.staged_element)==op.write.destination;
      }
      const auto offset=op.element==S::kClientPlanNone?0:std::size_t(op.element)*op.element_stride;
      return offset<=extent&&op.write.size<=extent-offset&&static_cast<std::byte *>(slot)+offset==op.write.destination;
    }catch(...){return false;}
  }
  ClientAtomicReadback ReadOp(const Op &op,bool repair,bool check_owner=true,bool known_drift=false) {
    using R=ClientAtomicReadback;
    if(op.duplicate)return R::Untracked;
    const auto &plan=scene->State();const auto &f=plan.Fields()[op.field];
    const auto object=plan.Records()[f.record].object;
    if(S::FindLayout(plan.Records()[f.record].layout)->protocol)return R::Untracked;
    if(object>=objects.size()||(check_owner&&!preparation.InstalledObjectCurrent(*batch,object,objects[object].address)))return R::Unsafe;
    if(check_owner&&op.reference!=S::kClientPlanNone&&!preparation.InstalledObjectCurrent(*batch,op.reference,op.write.reference))return R::Unsafe;
    // 动态子地址只有在仍由本批容器承载时可读。任一父层换址都交完整恢复。
    for(auto p=plan.Paths()[f.path].parent;p!=S::kClientPlanNone;p=plan.Paths()[p].parent){
      const auto n=FieldAt(object,p);
      if(n==S::kClientPlanNone)continue;
      const auto attachment=field_attachments[n];
      if(attachment==S::kClientPlanNone)continue;
      const auto &owner=operations[attachment].write;
      void *slot=nullptr;std::size_t extent=0;
      if(!Destination(n,slot,extent)||slot!=owner.destination||!ClientStagedFieldAttached(owner.field,slot))return R::Unsafe;
    }
    if(op.delegated){++read_checks;return bindings.ReadField(Inputs(),op.field,repair);}
    if(!WriteSlotCurrent(op))return R::Unsafe;
    if(!known_drift){
      ++read_checks;read_bytes+=op.write.size;
      if(ClientAtomicWriteCheck(op.write))return R::Match;
    }
    if(op.write.kind==ClientAtomicWriteKind::FieldAttachment)return R::Unsafe;
    if(!repair)return R::Drift;
    ClientAtomicReadRegion region;
    if(!ClientAtomicWriteCapture(op.write,nullptr,region))return R::Unsafe;
    const bool reference=op.write.kind==ClientAtomicWriteKind::EntityReference;
    if(ClientAtomicWriteApply(op.write)&&(reference||ClientAtomicWriteCheck(op.write)))return R::Match;
    if(op.write.kind==ClientAtomicWriteKind::EntityReference)uncertain=true;
    return R::Unsafe;
  }
  S::ClientStateNativeStep RepairNow(S::BodyStream stream,std::size_t &cursor,bool costumes) noexcept {
    using R=ClientAtomicReadback;
    const auto streams=std::uint8_t(1u<<unsigned(stream));ReadMetrics measured(*this,streams);
    bool repaired=false;
    try{
      const auto group=(std::uint64_t(costumes)<<8)|std::uint64_t(stream);
      const auto first=std::lower_bound(readback.begin(),readback.end(),group,[](const auto &e,auto s){return (e.key>>40)<s;});
      const auto last=std::upper_bound(first,readback.end(),group,[](auto s,const auto &e){return s<(e.key>>40);});
      const auto count=std::size_t(last-first);if(!count)return Ready(false,"binary_readback_empty");
      std::size_t visited=0;
      // 先登记失配操作再修整块。同一纯读窗口内，已比较过的项和匹配项都可复用。
      std::array<std::uint32_t,4096> drifted;
      for(unsigned blocks=0;blocks<4&&visited<128&&visited<count;++blocks){
        auto at=first+(cursor%count);
        const auto begin=std::lower_bound(first,at,at->key,[](const auto &e,const auto &key){return e.key<key;});
        const auto end=std::upper_bound(at,last,at->key,[](const auto &key,const auto &e){return key<e.key;});
        const auto stop=at+(std::min)(std::size_t(end-at),(std::min)(128-visited,count-visited));
        std::size_t drift_count=0;
        const auto inspect=[&](auto from,auto to){
          // 只覆盖repair=false的核对；返回后再执行可能改动承载的修复setter。
          PurePathWindow window(*this);if(!window.ready)return false;
          for(auto i=from;i!=to;++i){
            const auto result=ReadOp(operations[i->operation],false);
            if(result==R::Unsafe){FieldIssue(operations[i->operation],"drift_check","binary_readback_owner_changed");return false;}
            if(result==R::Drift)drifted[drift_count++]=i->operation;
          }
          return true;
        };
        if(!Current(*batch)||!inspect(at,stop))return Fail("binary_readback_owner_changed");
        if(drift_count){
          repaired=true;
          memory->ApplyMetrics().Volume(streams,S::ClientApplyVolume::Drift);
          if(std::size_t(end-begin)>drifted.size())return Fail("binary_readback_repair_budget");
          // 修复前先核整块承载。任何未跟随的资源／容器替换要求重建场景。
          if(!inspect(begin,at)||!inspect(stop,end))return Fail("binary_readback_repair_owner_changed");
          const bool native_reference=std::any_of(drifted.begin(),drifted.begin()+drift_count,[&](auto n){
            return operations[n].write.kind==ClientAtomicWriteKind::EntityReference;
          });
          if(native_reference){
            // 原生引用 setter 的写集未知，沿用整块核对；先前纯读见证不能跨该调用复用。
            for(auto i=begin;i!=end;++i)
              if(!Current(*batch)||ReadOp(operations[i->operation],true)==R::Unsafe)return Fail("binary_readback_partial_repair");
          }else{
            // 回到原有操作顺序。普通位／字节写入区间互斥；特殊字段仍向各自 owner 请求恢复。
            std::sort(drifted.begin(),drifted.begin()+drift_count);
            for(std::size_t i=0;i<drift_count;++i)
              if(!Current(*batch)||ReadOp(operations[drifted[i]],true,true,true)==R::Unsafe)return Fail("binary_readback_partial_repair");
          }
          Trace("client","event=binary_native_block_repaired stream=%u record=%u block=%u",unsigned(stream),std::uint32_t(begin->key>>8),unsigned(begin->key&255));
        }
        visited+=std::size_t(stop-at);cursor=std::size_t(stop-first);
      }
    }catch(...){return Fail("binary_readback_exception");}
    if(!Current(*batch))return Fail("binary_readback_revoked");
    // 外层已核本流仍对应最新发布根；旧 joint 的另一流可能已换址，不再借其模板。
    if(repaired&&!Step(bindings.FinishFields(Inputs(),streams)))return Failed(written,reason);
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
      const auto *original=op.write.kind==ClientAtomicWriteKind::FieldAttachment?nullptr:originals.data()+op.original;
      if(!ClientAtomicWriteRestore(op.write,original,op.write.size))return Failed(written,"binary_atomic_restore_failed_retained");op.restored=true;
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
  const auto streams=S::StateTargetStreams(s.scene->State().Target());
  // 主线程子阶段分别聚合，避免把描述构图、原生暂存与真正写入混为同一种成本。
  {
    S::ClientApplyTimer time(s.memory->ApplyMetrics(),streams,S::ClientApplyStage::MainObjects);
    const auto roots=s.preparation.StageObjects(*s.batch);if(!roots.ready||roots.failed)return roots;
  }
  {
    S::ClientApplyTimer time(s.memory->ApplyMetrics(),streams,S::ClientApplyStage::MainFields);
    const auto fields=s.preparation.StageFields(*s.batch);if(!fields.ready||fields.failed)return fields;
  }
  try {
    S::ClientApplyTimer time(s.memory->ApplyMetrics(),streams,S::ClientApplyStage::AtomicDescribe);
    s.Describe();s.staged=true;
    return Ready(false,"binary_atomic_staged");
  }catch(const S::ClientShadowError &e){return s.Fail(e.what());}
   catch(const S::ClientTableError &){return s.Fail("binary_atomic_budget");}
   catch(...){return s.Fail("binary_atomic_description_exception");}
}
S::ClientStateNativeStep ClientSceneAtomicInstaller::Seal(const S::ClientStateApplyBatch &b) noexcept {
  auto &s=*impl_;
  if(!s.Current(b)||!s.staged||s.lease||!s.InspectStaged())return s.Fail("binary_atomic_commit_inputs_changed");
  const auto streams=S::StateTargetStreams(s.scene->State().Target());
  S::ClientApplyTimer time(s.memory->ApplyMetrics(),streams,S::ClientApplyStage::Seal);
  // 原生容器的原值、当前世界及根转移位置在串行写段入口取得。另一通道可以在 Stage 后先提交。
  // 四段只记录固定数量的聚合计时，不输出逐字段日志，失败前缀也计入实际耗时。
  try {
    {
    S::ClientApplyTimer measured(s.memory->ApplyMetrics(),streams,S::ClientApplyStage::SealAttachments);
    for(const auto &op:s.operations)if(!op.delegated&&!op.late&&op.write.retained_attachment&&
        !ClientStagedFieldAttached(op.write.field,op.write.destination)){
      s.FieldIssue(op,"seal_attachment","binary_atomic_retained_attachment_changed");
      return s.Fail("binary_atomic_retained_attachment_changed");
    }
    }
    {
    S::ClientApplyTimer measured(s.memory->ApplyMetrics(),streams,S::ClientApplyStage::SealBindings);
    const auto preflight=s.bindings.Preflight(*s.batch,s.Inputs());
    if(preflight.written){s.written=true;s.uncertain=true;return s.Fail("binary_atomic_preflight_wrote_world");}
    if(preflight.failed)return s.Fail(preflight.reason);
    if(!preflight.ready){s.preparation.RevokeStaged(*s.batch);return preflight;}
    if(!s.Current(*s.batch))return s.Fail("binary_atomic_preflight_revoked");
    }
    s.roots=ClientPreparationRoots();
    // 只在首次构造这批指令时保存已定址的 Game／Room 标量；热写区不扩容。
    {
    S::ClientApplyTimer measured(s.memory->ApplyMetrics(),streams,S::ClientApplyStage::SealRoots);
    ClientAtomicReadRegion read_region;
    if(s.root_scalars)for(const auto &op:s.operations)if(!op.late&&!op.delegated&&!op.duplicate&&!op.unchanged&&
        (op.write.kind==ClientAtomicWriteKind::Bytes||op.write.kind==ClientAtomicWriteKind::Masked32)&&
        s.root_scalars->NeedsSnapshot(s.roots,op.write.destination,op.write.size)){
      auto original=std::span(s.originals).subspan(op.original,op.write.size);
      if(!ClientAtomicWriteCapture(op.write,original.data(),read_region))return s.Fail("binary_atomic_root_scalar_snapshot");
      if(!s.root_scalars->Save(op.write.destination,original))return s.Fail("binary_atomic_root_scalar_budget");
    }
    }
    {
    S::ClientApplyTimer measured(s.memory->ApplyMetrics(),streams,S::ClientApplyStage::SealHold);
    if(!s.preparation.HoldInstallation(*s.batch,&s))return s.Fail("binary_atomic_installation_lease");s.lease=true;
    }
    s.reason="binary_atomic_commit_ready";return Ready(false,s.reason);
  }catch(const S::ClientShadowError &e){return s.Fail(e.what());}
   catch(const S::ClientTableError &){return s.Fail("binary_atomic_budget");}
   catch(...){return s.Fail("binary_atomic_description_exception");}
}
S::ClientStateNativeStep ClientSceneAtomicInstaller::Install(const S::ClientStateApplyBatch &b) noexcept {
  auto &s=*impl_;if(!s.Thread())return Failed(false,"binary_atomic_wrong_thread");if(s.batch.get()!=&b)return Failed(false,"binary_atomic_other_ticket");
  if(!s.Current(b)||!s.staged||!s.lease)return s.Fail("binary_atomic_install_not_current");
  if(s.installed)return Ready(s.written,"binary_atomic_installed");if(s.install_entered)return s.Fail("binary_atomic_install_prefix_retained");
  if(!s.InspectStaged())return s.Fail("binary_atomic_staged_inputs_changed");
  S::ClientApplyTimer time(s.memory->ApplyMetrics(),S::StateTargetStreams(s.scene->State().Target()),S::ClientApplyStage::NativeWrite);
  s.manager_token=OwnerToken(Owner::Manager);if(!s.manager_token)return s.Fail("binary_atomic_manager_token");
  const auto result=s.preparation.ApplyInstallation(b,&s,false,&s,[](void *p)noexcept{return static_cast<Impl *>(p)->InstallNow();});
  s.written|=result.written;if(result.failed)return s.Fail(result.reason);return result;
}
bool ClientSceneAtomicInstaller::VisitStaged(const S::ClientStateApplyBatch &b,void *context,StagedVisitor visit) noexcept {
  auto &s=*impl_;
  return visit&&s.Current(b)&&s.staged&&!s.install_entered&&s.InspectStaged()&&visit(context,s.Inputs())&&s.preparation.StagedCurrent(b);
}
S::ClientStateNativeStep ClientSceneAtomicInstaller::Publish(const S::ClientStateApplyBatch &b) noexcept {
  auto &s=*impl_;if(!s.Thread())return Failed(false,"binary_atomic_wrong_thread");if(s.batch.get()!=&b)return Failed(false,"binary_atomic_other_ticket");
  if(!s.Current(b)||!s.installed||!s.display_installed)return s.Fail("binary_atomic_publish_not_installed");
  if(s.published)return Ready(s.written,"binary_atomic_published");
  if(s.publish_entered||!s.manager_token||s.manager_token!=OwnerToken(Owner::Manager))return s.Fail("binary_atomic_publish_crossed_manager_window");
  S::ClientApplyTimer time(s.memory->ApplyMetrics(),S::StateTargetStreams(s.scene->State().Target()),S::ClientApplyStage::NativePublish);
  const auto result=s.preparation.ApplyInstallation(b,&s,false,&s,[](void *p)noexcept{return static_cast<Impl *>(p)->PublishNow();});s.written|=result.written;
  if(!result.ready||result.failed)return s.Fail(result.reason);
  if(!s.preparation.CommitInstallation(b,&s))return s.Fail("binary_atomic_custody_commit_failed");
  const auto attachments=s.preparation.ApplyInstallation(b,&s,false,&s,[](void *p)noexcept{return static_cast<Impl *>(p)->CommitFields();});
  if(!attachments.ready||attachments.failed){s.uncertain=true;return s.Fail(attachments.reason);}
  s.bindings.Committed(b,s.Inputs());
  s.published=true;s.reason="binary_atomic_published";return Ready(s.written,s.reason);
}
S::ClientStateNativeStep ClientSceneAtomicInstaller::InstallDisplayFields(const S::ClientStateApplyBatch &b,bool rebuilt) noexcept {
  auto &s=*impl_;
  if(!s.Current(b)||!s.installed||s.manager_token!=OwnerToken(Owner::Manager))return s.Fail("binary_atomic_display_install_window");
  if(s.display_installed&&!rebuilt)return Ready(s.written,"binary_atomic_display_fields_installed");
  struct Work {Impl &owner;bool rebuilt;} work{s,rebuilt};
  const auto result=s.preparation.ApplyInstallation(b,&s,false,&work,[](void *p) noexcept {
    const auto &w=*static_cast<Work *>(p);return w.owner.InstallNow(true,w.rebuilt);
  });
  s.written|=result.written;
  return result.ready&&!result.failed?result:s.Fail(result.reason);
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
  s.field_owners.clear();s.path_facts.clear();s.resolved_writes.clear();s.resolved_scratch.clear();
  s.readback.clear();s.field_attachments.clear();
  s.lease=false;s.reason="binary_atomic_drained";return true;
}
bool ClientSceneAtomicInstaller::Current(const S::ClientStateApplyBatch &b)const noexcept{return impl_->Current(b);}
bool ClientSceneAtomicInstaller::Installed(const S::ClientStateApplyBatch &b)const noexcept{return impl_->installed&&impl_->Current(b);}
bool ClientSceneAtomicInstaller::Published(const S::ClientStateApplyBatch &b)const noexcept{return impl_->published&&impl_->Current(b);}
bool ClientSceneAtomicInstaller::Drained()const noexcept{return impl_->Thread()&&!impl_->batch;}
const char *ClientSceneAtomicInstaller::Reason()const noexcept{return impl_->Thread()?impl_->reason:"binary_atomic_wrong_thread";}
} // namespace cs::game
