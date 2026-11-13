#include "game/client/scene/client_preparation.h"
#include "game/client/scene/client_lifetime.h"
#include "game/common/bridge.h"
#include "game/common/context.h"
#include "net/live.h"
#include <algorithm>
#ifdef FindResource
#undef FindResource
#endif

namespace cs::game {
namespace {
namespace S=state;
// 本进程不复用安装实例号。成员号重排或工厂地址复用不影响该见证。
std::uint64_t next_binary_instance=0;
S::ClientStateNativeStep Wait(const char *r){return {false,false,false,r};}
S::ClientStateNativeStep Fail(const char *r){return {false,false,true,r};}
S::ClientStateNativeStep Ready(const char *r){return {true,false,false,r};}
struct PreparationFrame {const void *owner=nullptr;bool retirement=false;bool (*current)(const void *,bool)noexcept=nullptr;};
thread_local PreparationFrame preparation_frame;
thread_local PreparationFrame installation_frame;
thread_local const void *preparation_owner=nullptr;
class PreparationScope {
public:
  PreparationScope(const void *owner,bool retirement,bool (*current)(const void *,bool)noexcept) {
    if(!preparation_frame.owner&&current(owner,retirement)){preparation_frame={owner,retirement,current};active_=true;}
  }
  ~PreparationScope(){if(active_)preparation_frame={};}
  bool Active()const noexcept{return active_;}
private:bool active_=false;
};
// 实际调用仍进入既有常驻隔离分派；准备资格只来自当前二进制 owner，不依赖旧 target。
template<class O,class F> ClientPreparationResult ApplyNative(O &owner,F &&body) noexcept {
  PreparationScope purpose(&owner,false,O::ScopeCurrent);
  if(!purpose.Active())return {ClientPreparationStatus::Failed,"binary_preparation_scope"};
  ClientDispatchScope scope(ClientOperation::BinaryPreparation,ClientEntry::NativePreparation);
  if(!scope.Ready()||ClientDispatchBefore(ClientEntry::NativePreparation)!=ClientEntryDecision::CallOriginal)
    return {ClientPreparationStatus::Failed,"binary_preparation_dispatch"};
  const auto result=body();ClientDispatchReturned(ClientEntry::NativePreparation);
  return scope.Succeeded()&&ClientBinaryPreparationScopeActive(false)?result:ClientPreparationResult{ClientPreparationStatus::Failed,"binary_preparation_dispatch_incomplete"};
}
template<class O,class F> bool RetireNative(O &owner,F &&body) noexcept {
  PreparationScope purpose(&owner,true,O::ScopeCurrent);if(!purpose.Active())return false;
  ClientDispatchScope scope(ClientOperation::BinaryPreparationRetire,ClientEntry::NativePreparation);
  if(!scope.Ready()||ClientDispatchBefore(ClientEntry::NativePreparation)!=ClientEntryDecision::CallOriginal)return false;
  const bool result=body();ClientDispatchReturned(ClientEntry::NativePreparation);return result&&scope.Succeeded()&&ClientBinaryPreparationScopeActive(true);
}
unsigned Category(const S::ClientInstallEntity &e,std::uint16_t layout) {
  if(layout==147)return 12;
  switch(e.type){case 1:return 0;case 2:return 2;case 3:return 8;case 4:return 4;case 5:return 7;case 6:return 10;case 7:return 5;
    case 8:return 6;case 9:return 3;case 1000:return 9;case 9001:return 11;default:return 1;}
}
// 这些域的构造责任跟随实际对象；Game／Room等借用根没有可转移的析构责任。
bool Structural(S::ClientInstallDomain domain) {
  return domain==S::ClientInstallDomain::Entity||domain==S::ClientInstallDomain::Player||
    domain==S::ClientInstallDomain::Grid;
}
}
bool ClientBinaryPreparationScopeActive(bool retirement) noexcept {
  const auto &frame=preparation_frame;return IsMain()&&frame.owner&&frame.retirement==retirement&&frame.current&&frame.current(frame.owner,retirement);
}
bool ClientBinaryInstallationScopeActive(bool retirement) noexcept {
  const auto &frame=installation_frame;return IsMain()&&frame.owner&&frame.retirement==retirement&&frame.current&&frame.current(frame.owner,retirement);
}
struct ClientSceneMainPreparation::Impl {
  struct Resource {S::ResourceKey key;ClientPreparedResource *native=nullptr;S::ClientResourceState result=S::ClientResourceState::Unprepared;};
  struct Object {ClientStagedObject *native=nullptr;void *borrowed=nullptr;std::uint64_t instance=0;bool reuse=false,required=false;};
  struct Need {ClientPreparedNativeNeed *native=nullptr;std::uint32_t alias=S::kClientPlanNone;};
  struct Installed {
    S::ClientStateApplyBatchRef batch;std::shared_ptr<const ClientSceneInstallPlan> plan;
    S::ClientStateVector<Object> objects;S::ClientStateVector<Need> needs;ClientSceneFieldStaging fields;
    const void *installation=nullptr;
    bool retiring=false;
    Installed(const std::shared_ptr<S::ClientStateMemory> &m,ClientPreparationAllocator a):objects(S::ClientStateAllocator<Object>(m)),
      needs(S::ClientStateAllocator<Need>(m)),fields(m,a){}
  };
  // Hold阶段封存转交位置；Commit只移动原生owner，不分配或重跑Init。
  struct RootTransfer {Installed *from=nullptr;std::uint32_t source=0,target=0;ClientStagedObject *native=nullptr;void *address=nullptr;};
  std::shared_ptr<S::ClientStateMemory> memory;
  std::shared_ptr<const ClientSceneObjectSource> source;
  const std::thread::id thread=std::this_thread::get_id();
  S::StateRoute route{};std::uint64_t generation=0;
  ClientNativeRoots roots{};void *vm=nullptr;
  bool active=false,closing=false,prepared=false,staged=false,fields_staged=false,fault=false;
  unsigned visitors=0;
  const void *installation=nullptr;
  const char *reason="binary_preparation_idle";
  std::optional<S::ClientStateResourceJob> job;
  S::ClientResourceState feedback=S::ClientResourceState::Pending;
  S::ClientStateApplyBatchRef batch;
  std::shared_ptr<const ClientSceneInstallPlan> plan;
  S::ClientStateVector<Resource> resources;
  S::ClientStateVector<Object> objects;
  S::ClientStateVector<ClientStagedObjectInput> inputs;
  S::ClientStateVector<Need> needs;
  S::ClientStateVector<std::int32_t> player_types;
  ClientSceneFieldStaging fields;
  S::ClientStateVector<std::shared_ptr<Installed>> installed;
  S::ClientStateVector<RootTransfer> root_transfers;
  std::shared_ptr<Installed> reservation;
  std::size_t need_cursor=0,object_cursor=0;
  Impl(std::shared_ptr<S::ClientStateMemory> m,std::shared_ptr<const ClientSceneObjectSource> s):memory(std::move(m)),source(std::move(s)),
    resources(S::ClientStateAllocator<Resource>(memory)),objects(S::ClientStateAllocator<Object>(memory)),inputs(S::ClientStateAllocator<ClientStagedObjectInput>(memory)),
    needs(S::ClientStateAllocator<Need>(memory)),player_types(S::ClientStateAllocator<std::int32_t>(memory)),fields(memory,Allocator()),
    installed(S::ClientStateAllocator<std::shared_ptr<Installed>>(memory)),root_transfers(S::ClientStateAllocator<RootTransfer>(memory)) {}
  bool Thread()const noexcept{return thread==std::this_thread::get_id();}
  bool Window()const noexcept{return Thread()&&ClientPreparationWindow(roots,vm);}
  static bool ScopeCurrent(const void *p,bool retirement) noexcept {
    const auto &s=*static_cast<const Impl *>(p);
    return s.Thread()&&preparation_owner==p&&s.Window()&&(retirement||(s.active&&!s.closing&&!s.fault&&(!s.batch||!s.batch->cancelled.load(std::memory_order_acquire))));
  }
  bool Current(const S::ClientStateApplyBatch &b)const noexcept {
    return Thread()&&active&&!closing&&!fault&&Window()&&b.ticket.serial&&b.ticket.route==route&&!b.cancelled.load(std::memory_order_acquire)&&
      b.target.install&&b.target.install->Memory()==memory&&b.target.install->Describes(b.target.candidate)&&
      (!batch||batch.get()==&b||std::any_of(installed.begin(),installed.end(),[&](const auto &i){return i->batch.get()==&b;}));
  }
  Installed *InstalledBatch(const S::ClientStateApplyBatch &b)const noexcept {
    for(const auto &i:installed)if(i->batch.get()==&b)return i.get();return nullptr;
  }
  // 数字计划的Objects按完整身份排序；用二分定位，不为每个根扫描整份后继对象表。
  static std::uint32_t ObjectIndex(const ClientSceneInstallPlan &p,const S::PresentationMember &id) noexcept {
    const auto values=p.State().Objects();const auto at=std::lower_bound(values.begin(),values.end(),id,[](const auto &v,const auto &key){return v.member.identity<key;});
    return at!=values.end()&&at->member.identity==id?std::uint32_t(at-values.begin()):S::kClientPlanNone;
  }
  static std::uint32_t Borrower(const ClientSceneInstallPlan *p,const S::ClientStateVector<Object> &values,
      const S::PresentationMember &id,void *address,std::uint64_t instance) noexcept {
    if(!p||!address)return S::kClientPlanNone;const auto n=ObjectIndex(*p,id);
    if(n>=values.size())return S::kClientPlanNone;const auto &o=values[n];
    return o.required&&o.reuse&&o.borrowed==address&&o.instance&&(!instance||o.instance==instance)?n:S::kClientPlanNone;
  }
  bool Borrowed(const Installed &owned)const noexcept {
    for(std::uint32_t n=0;n<owned.objects.size();++n){const auto &object=owned.objects[n];
      if(!object.native||!Structural(owned.plan->State().Objects()[n].domain))continue;
      auto *address=ClientStagedObjectOwnedAddress(object.native);if(!address)return true;
      const auto &id=owned.plan->State().Objects()[n].member.identity;
      if(Borrower(plan.get(),objects,id,address,object.instance)!=S::kClientPlanNone)return true;
      for(const auto &other:installed)if(other.get()!=&owned&&Borrower(other->plan.get(),other->objects,id,address,object.instance)!=S::kClientPlanNone)return true;
    }
    return false;
  }
  void PlanRootTransfers() {
    root_transfers.clear();root_transfers.reserve(objects.size());
    for(const auto &prior:installed)for(std::uint32_t n=0;n<prior->objects.size();++n){const auto &old=prior->objects[n];
      if(!old.native||!Structural(prior->plan->State().Objects()[n].domain))continue;
      auto *address=ClientStagedObjectOwnedAddress(old.native);if(!address)throw S::ClientShadowError(S::StateReadiness::Lifecycle,"binary_root_transfer_use_changed");
      const auto target=Borrower(plan.get(),objects,prior->plan->State().Objects()[n].member.identity,address,old.instance);
      if(target==S::kClientPlanNone)continue;
      if(prior->retiring||objects[target].native)throw S::ClientShadowError(S::StateReadiness::Lifecycle,"binary_root_transfer_retiring_or_owned");
      root_transfers.push_back({prior.get(),n,target,old.native,address});
    }
  }
  bool RootTransfersCurrent()const noexcept {
    for(const auto &t:root_transfers){
      if(std::none_of(installed.begin(),installed.end(),[&](const auto &p){return p.get()==t.from;})||t.from->retiring||t.source>=t.from->objects.size()||t.target>=objects.size())return false;
      const auto &old=t.from->objects[t.source];const auto &next=objects[t.target];
      if(old.native!=t.native||next.native||ClientStagedObjectOwnedAddress(old.native)!=t.address||
          Borrower(plan.get(),objects,t.from->plan->State().Objects()[t.source].member.identity,t.address,old.instance)!=t.target)return false;
    }
    return true;
  }
  static void TransferRoot(Object &old,Object &next,void *address) noexcept {
    // 原批次降为普通借用，新的构造owner因此不能越过仍存活的旧用途提前析构。
    next.native=old.native;old.native=nullptr;old.borrowed=address;old.instance=next.instance;old.reuse=true;
  }
  Installed *RetirementSuccessor(const Installed &owned,std::uint32_t n,void *address,std::uint32_t &index)const noexcept {
    const auto &old=owned.objects[n];const auto &id=owned.plan->State().Objects()[n].member.identity;
    for(auto i=installed.rbegin();i!=installed.rend();++i)if(i->get()!=&owned&&!(*i)->retiring){
      index=Borrower((*i)->plan.get(),(*i)->objects,id,address,old.instance);
      if(index!=S::kClientPlanNone&&!(*i)->objects[index].native)return i->get();
    }
    return nullptr;
  }
  bool BeginRetirement(Installed &owned) noexcept {
    if(owned.retiring)return true;
    // 两遍处理，先证明所有责任都有去处。未提交候选不接管构造责任，也不允许拆掉其原owner。
    for(std::uint32_t n=0;n<owned.objects.size();++n){const auto &old=owned.objects[n];
      if(!old.native||!Structural(owned.plan->State().Objects()[n].domain))continue;
      auto *address=ClientStagedObjectOwnedAddress(old.native);if(!address)return false;
      const auto &id=owned.plan->State().Objects()[n].member.identity;
      if(Borrower(plan.get(),objects,id,address,old.instance)!=S::kClientPlanNone)return false;
      std::uint32_t index=0;if(RetirementSuccessor(owned,n,address,index))continue;
      for(const auto &other:installed)if(other.get()!=&owned&&Borrower(other->plan.get(),other->objects,id,address,old.instance)!=S::kClientPlanNone)return false;
    }
    // 最新Success后来被撤销时，也能把根交回仍存活的已提交使用者，再排空失败批次。
    for(std::uint32_t n=0;n<owned.objects.size();++n){auto &old=owned.objects[n];
      if(!old.native||!Structural(owned.plan->State().Objects()[n].domain))continue;
      auto *address=ClientStagedObjectOwnedAddress(old.native);std::uint32_t index=0;
      if(auto *next=RetirementSuccessor(owned,n,address,index))TransferRoot(old,next->objects[index],address);
    }
    owned.retiring=true;return true;
  }
  ClientPreparationAllocator Allocator() noexcept {
    return {memory.get(),[](void *p,std::size_t n,std::size_t a){return static_cast<S::ClientStateMemory *>(p)->Allocate(memory::Pool::Values,n,a);},
      [](void *p,void *v,std::size_t n,std::size_t a)noexcept{static_cast<S::ClientStateMemory *>(p)->Deallocate(memory::Pool::Values,v,n,a);},
      [](void *p,std::size_t n){static_cast<S::ClientStateMemory *>(p)->ChargeNative(n);},
      [](void *p,std::size_t n)noexcept{static_cast<S::ClientStateMemory *>(p)->UnchargeNative(n);}};
  }
  Resource *Find(S::ResourceKey key) noexcept {for(auto &r:resources)if(r.key==key)return &r;return nullptr;}
  ClientPreparedResource *Dependency(std::uint8_t table,std::uint64_t index) noexcept {
    if(index==S::kNullIndex)return nullptr;auto *r=Find({table,std::uint16_t(index)});
    return r&&r->result==S::ClientResourceState::Ready&&ClientPreparedResourceCurrent(r->native)?r->native:nullptr;
  }
  bool ResourceRoute(const S::ClientStateResourceJob &j)const noexcept {
    if(!active||closing||!j.resources||j.resources->Memory()!=memory||!j.described||j.ticket.route!=route||j.ticket.generation!=generation||!j.ticket.serial)return false;
    const auto d=j.resources->Directory();return d.epoch==j.ticket.epoch&&d.session==route.scope.session&&d.run==route.scope.run&&
      j.described->table==j.ticket.key.table&&j.described->index==j.ticket.key.index&&S::FindResource(d,j.ticket.key);
  }
  void ResourceStep() {
    if(!ResourceRoute(*job)){feedback=S::ClientResourceState::Unprepared;reason="binary_resource_cancelled";return;}
    if(!Window()){feedback=S::ClientResourceState::Failed;fault=true;reason="binary_resource_lifecycle";return;}
    if(!ClientNativeCpuQueuesEmpty()){reason="binary_resource_cpu_borrow";return;}
    auto *entry=Find(job->ticket.key);
    if(!entry){if(resources.size()>=S::StateResourceBounds().entries)throw S::ClientTableError(S::ClientTableFailure::Budget,"binary_resource_cache_capacity");
      resources.push_back({job->ticket.key});entry=&resources.back();}
    if(entry->result==S::ClientResourceState::Ready){feedback=ClientPreparedResourceCurrent(entry->native)?entry->result:S::ClientResourceState::Failed;
      reason=feedback==S::ClientResourceState::Ready?"binary_resource_reused":"binary_resource_generation_lost";return;}
    if(entry->result!=S::ClientResourceState::Unprepared){feedback=entry->result;return;}
    const auto &d=*job->described;ClientPreparedResource *first=nullptr,*second=nullptr;bool closed=true;
    const auto dependency=[&](std::uint8_t table,std::uint64_t index){auto *p=Dependency(table,index);if(index!=S::kNullIndex&&!p)closed=false;return p;};
    if(d.kind==6)first=dependency(2,d.numbers[0]);
    if(d.kind==7){first=dependency(1,d.numbers[3]);second=dependency(2,d.numbers[4]);}
    if(d.kind==8)first=dependency(2,d.numbers[3]);
    if(!closed){entry->result=feedback=S::ClientResourceState::Failed;reason="binary_resource_dependency_not_prepared";return;}
    if(!entry->native)entry->native=ClientPreparedResourceCreate(Allocator());
    if(!entry->native)throw S::ClientTableError(S::ClientTableFailure::Budget,"binary_resource_owner_allocation");
    const auto result=ApplyNative(*this,[&]{return ClientPreparedResourceLoad(entry->native,d,first,second);});reason=result.reason;
    switch(result.status){case ClientPreparationStatus::Waiting:return;case ClientPreparationStatus::Ready:entry->result=S::ClientResourceState::Ready;break;
      case ClientPreparationStatus::Missing:entry->result=S::ClientResourceState::MissingLocalResource;break;case ClientPreparationStatus::Failed:entry->result=S::ClientResourceState::Failed;break;}
    feedback=entry->result;
  }
  bool BorrowCurrent(std::size_t i)const noexcept {
    const auto &o=objects[i];if(!o.required)return true;
    if(o.reuse){
      const auto &id=plan->State().Objects()[i].member.identity;
      void *address=nullptr;
      std::uint64_t instance=0;
      return ResolveInstalled(id,address,instance)&&address==o.borrowed&&instance==o.instance;
    }
    return
      !o.native||ClientStagedObjectCurrent(o.native,roots);
  }
  bool ResolveInstalled(const S::PresentationMember &id,void *&address,std::uint64_t &instance)const noexcept {
    for(auto i=installed.rbegin();i!=installed.rend();++i){
      if((*i)->retiring||(*i)->batch->cancelled.load(std::memory_order_acquire))continue;
      const auto n=ObjectIndex(*(*i)->plan,id);
      if(n>=(*i)->objects.size())continue;
      const auto &o=(*i)->objects[n];
      if(o.native&&o.instance&&(address=ClientStagedObjectOwnedAddress(o.native))){
        instance=o.instance;
        return true;
      }
    }
    return source&&source->Resolve(id,address,instance)&&address&&instance&&source->Current(id,address,instance);
  }
  void DescribeObjects() {
    const auto &s=plan->State();objects.resize(s.Objects().size());inputs.resize(objects.size());needs.resize(s.NativeNeeds().size());
    // 配对旧根只供窄依赖使用；body 不重新暂存无关世界。引用实际触及的旧承载仍逐项重借。
    for(const auto &c:s.Commands())if(c.phase==S::ClientInstallPhase::StageObjects)objects.at(c.index).required=true;
    for(const auto &link:s.Bindings())if(!link.is_resource&&!link.is_null&&link.object!=S::kClientPlanNone)objects.at(link.object).required=true;
    for(const auto &event:s.Events())if(event.has_owner){const auto object=std::find_if(s.Objects().begin(),s.Objects().end(),[&](const auto &o){return o.member.identity==event.owner;});
      if(object!=s.Objects().end())objects[std::size_t(object-s.Objects().begin())].required=true;}
    for(const auto &p:s.Players())if(std::find(player_types.begin(),player_types.end(),p.player_type)==player_types.end())player_types.push_back(p.player_type);
    for(std::uint32_t i=0;i<objects.size();++i){const auto &o=s.Objects()[i];auto &out=objects[i];auto &in=inputs[i];
      if(!out.required)continue;
      const auto *layout=S::FindLayout(o.member.identity.layout);if(!layout)throw S::ClientShadowError(S::StateReadiness::InvalidEncoding,"binary_stage_layout");
      in.layout=layout->id;in.native_size=layout->native_size;
      const bool structural=Structural(o.domain);
      if(structural&&(o.action==S::ClientInstallAction::Reuse||o.action==S::ClientInstallAction::Remap)){
        if(!ResolveInstalled(o.member.identity,out.borrowed,out.instance))
          throw S::ClientShadowError(S::StateReadiness::Lifecycle,"binary_stage_existing_owner_missing");out.reuse=true;in.borrowed=out.borrowed;
        for(const auto &prior:installed)if(prior->retiring){const auto n=ObjectIndex(*prior->plan,o.member.identity);
          if(n<prior->objects.size()&&prior->objects[n].native)throw S::ClientShadowError(S::StateReadiness::Lifecycle,"binary_stage_root_already_retiring");}
        if(o.domain==S::ClientInstallDomain::Entity){const auto e=std::find_if(s.Entities().begin(),s.Entities().end(),[&](auto &v){return v.object==i;});
          if(e!=s.Entities().end()){in.kind=ClientStagedKind::Entity;in.type=e->type;in.category=Category(*e,in.layout);}}
        continue;
      }
      switch(o.domain){
        case S::ClientInstallDomain::Participant:case S::ClientInstallDomain::ControlGroup:in.kind=ClientStagedKind::Logical;break;
        case S::ClientInstallDomain::Run:in.kind=ClientStagedKind::Game;break;
        case S::ClientInstallDomain::Level:in.kind=ClientStagedKind::Level;break;
        case S::ClientInstallDomain::Room:in.kind=ClientStagedKind::Room;break;
        case S::ClientInstallDomain::Sfx:in.kind=ClientStagedKind::Sfx;break;
        case S::ClientInstallDomain::Descriptor:{
          const auto descriptor=std::find_if(s.Descriptors().begin(),s.Descriptors().end(),[i](const auto &d){return d.object==i;});
          if(descriptor==s.Descriptors().end())throw S::ClientShadowError(S::StateReadiness::InvalidReference,"binary_stage_descriptor_slot");
          in.kind=ClientStagedKind::Descriptor;in.index=descriptor->source_slot;break;}
        case S::ClientInstallDomain::Grid:{in.kind=ClientStagedKind::Grid;const auto g=std::find_if(s.Grids().begin(),s.Grids().end(),[&](auto &v){return v.object==i;});
          if(g==s.Grids().end())throw S::ClientShadowError(S::StateReadiness::InvalidReference,"binary_stage_grid_description");
          in.type=std::uint32_t(g->type);in.variant=std::uint32_t(g->variant);in.index=g->index;in.seed=g->spawn_seed;break;}
        case S::ClientInstallDomain::Player:case S::ClientInstallDomain::Entity:{in.kind=o.domain==S::ClientInstallDomain::Player?ClientStagedKind::Player:ClientStagedKind::Entity;
          const auto e=std::find_if(s.Entities().begin(),s.Entities().end(),[&](auto &v){return v.object==i;});
          if(e==s.Entities().end())throw S::ClientShadowError(S::StateReadiness::InvalidReference,"binary_stage_entity_description");
          in.type=e->type;in.variant=e->variant;in.subtype=e->subtype;in.seed=e->init_seed;in.category=Category(*e,in.layout);break;}
      }
    }
  }
  bool DrainBatch() noexcept {
    if(visitors||installation||!Thread())return false;
    if(fields.HasNative()||std::any_of(objects.begin(),objects.end(),[](const auto &o){return o.native;})||std::any_of(needs.begin(),needs.end(),[](const auto &n){return n.native;})) {
      if(!RetireNative(*this,[&]{
        if(!fields.Drain()){reason="binary_fields_retirement_pending";return false;}
        for(std::size_t i=objects.size();i>0;--i)if(!ClientStagedObjectRelease(objects[i-1].native)){reason="binary_stage_retirement_pending";return false;}
        for(std::size_t i=needs.size();i>0;--i)if(!ClientPreparedNativeNeedRelease(needs[i-1].native)){reason="binary_native_need_retirement_pending";return false;}
        return true;}))return false;
    }
    if(!fields.Drain())return false;
    objects.clear();inputs.clear();needs.clear();player_types.clear();root_transfers.clear();plan.reset();batch.reset();reservation.reset();need_cursor=object_cursor=0;prepared=staged=fields_staged=false;return true;
  }
  bool DrainInstalled(Installed &i) noexcept {
    if(i.installation||visitors||!Thread()||Borrowed(i))return false;
    return RetireNative(*this,[&]{if(!i.fields.Drain())return false;
      for(std::size_t n=i.objects.size();n>0;--n)if(!ClientStagedObjectRelease(i.objects[n-1].native))return false;
      for(std::size_t n=i.needs.size();n>0;--n)if(!ClientPreparedNativeNeedRelease(i.needs[n-1].native))return false;
      i.objects.clear();i.needs.clear();i.plan.reset();i.batch.reset();return true;});
  }
};
ClientSceneMainPreparation::ClientSceneMainPreparation(std::shared_ptr<S::ClientStateMemory> m,std::shared_ptr<const ClientSceneObjectSource> source) {
  if(!m)throw S::ClientTableError(S::ClientTableFailure::Internal,"binary_preparation_memory");impl_=std::make_unique<Impl>(std::move(m),std::move(source));
}
ClientSceneMainPreparation::~ClientSceneMainPreparation() {
  // 调用方必须显式排空。错误销毁时保留账户及全部前缀，禁止后台析构原生句柄或释放仍借用输入。
  if(impl_&&!Drained())impl_.release();
}
bool ClientSceneMainPreparation::Open(const S::StateRoute &r,std::uint64_t generation) noexcept {
  auto &s=*impl_;if(!s.Thread()||s.batch||s.job||!generation||!r.connection||!r.route||!r.scope.session||!r.scope.run||!r.scope.scene)return false;
  const auto roots=ClientPreparationRoots();if(!roots.game||!roots.room)return false;
  if(s.active||s.closing||!s.resources.empty()||!s.installed.empty()||preparation_owner)return false;
  preparation_owner=&s;s.route=r;s.generation=generation;s.roots=roots;s.vm=ClientLuaStateIdentity();s.active=true;s.fault=false;s.reason="binary_preparation_open";return true;
}
void ClientSceneMainPreparation::Close() noexcept {
  auto &s=*impl_;if(s.Thread()){s.active=false;s.closing=true;s.reason="binary_preparation_closing";}
}
bool ClientSceneMainPreparation::OfferResource(S::ClientStateResourceJob job) noexcept {
  auto &s=*impl_;if(!s.Thread()||s.job||!job.resources||job.resources->Memory()!=s.memory||!job.ticket.serial)return false;
  s.job=std::move(job);s.feedback=S::ClientResourceState::Pending;return true;
}
void ClientSceneMainPreparation::PumpResources(net::live::Client &transport) noexcept {
  auto &s=*impl_;if(!s.Thread())return;
  if(!s.job){S::ClientStateResourceJob next;if(!transport.PollBinaryResource(next))return;s.job=std::move(next);s.feedback=S::ClientResourceState::Pending;}
  if(s.feedback==S::ClientResourceState::Pending)try{s.ResourceStep();}catch(const S::ClientTableError &){s.feedback=S::ClientResourceState::Failed;s.reason="binary_resource_budget";}
    catch(...){s.feedback=S::ClientResourceState::Failed;s.reason="binary_resource_preparation_exception";s.fault=true;}
  if(s.feedback!=S::ClientResourceState::Pending&&!s.ResourceRoute(*s.job))s.feedback=S::ClientResourceState::Unprepared;
  if(s.feedback!=S::ClientResourceState::Pending&&transport.CompleteBinaryResource(s.job->ticket,s.feedback)){s.job.reset();s.feedback=S::ClientResourceState::Pending;}
}
S::ClientStateNativeStep ClientSceneMainPreparation::Prepare(S::ClientStateApplyBatchRef b) noexcept {
  auto &s=*impl_;if(!b||!s.Current(*b))return Fail("binary_preparation_lifecycle");
  if(s.prepared)return Ready("binary_resources_prepared");
  try {
    if(!s.batch){const auto plan=std::dynamic_pointer_cast<const ClientSceneInstallPlan>(b->native);
      if(!plan||&plan->State()!=b->target.install.get())return Fail("binary_preparation_plan_type");
      s.batch=std::move(b);s.plan=plan;s.DescribeObjects();s.fields.Describe(plan->State());}
    const auto &plan=s.plan->State();
    for(const auto &need:plan.Resources()){const auto directory=need.resources->Directory();
      if(directory.session!=s.route.scope.session||directory.run!=s.route.scope.run)return Fail("binary_preparation_resource_scope");
      auto *entry=s.Find(need.key);if(!entry||entry->result!=S::ClientResourceState::Ready||!ClientPreparedResourceCurrent(entry->native))return Fail("binary_preparation_resource_not_materialized");}
    if(!ClientNativeCpuQueuesEmpty())return Wait("binary_preparation_cpu_borrow");
    if(s.need_cursor<plan.NativeNeeds().size()){
      const auto &need=plan.NativeNeeds()[s.need_cursor];auto &slot=s.needs[s.need_cursor];
      for(std::uint32_t i=0;i<s.need_cursor;++i){const auto &prior=plan.NativeNeeds()[i];if(prior.kind==need.kind&&prior.id==need.id&&prior.second_id==need.second_id){slot.alias=i;break;}}
      if(slot.alias==S::kClientPlanNone){if(!slot.native)slot.native=ClientPreparedNativeNeedCreate(s.Allocator());if(!slot.native)return Fail("binary_native_need_allocation");
        ClientNativeNeedInput input{unsigned(need.kind),need.id,need.second_id,s.player_types.data(),s.player_types.size()};
        const auto result=ApplyNative(s,[&]{return ClientPreparedNativeNeedLoad(slot.native,input);});s.reason=result.reason;
        if(result.status==ClientPreparationStatus::Waiting)return Wait(result.reason);
        if(result.status!=ClientPreparationStatus::Ready)return Fail(result.reason);}
      ++s.need_cursor;return Wait("binary_native_resources_preparing");
    }
    const auto capacity=ApplyNative(s,[&]{return ClientStagedObjectsCapacity(s.roots,s.inputs.data(),s.inputs.size());});
    if(capacity.status!=ClientPreparationStatus::Ready)return Fail(capacity.reason);
    s.prepared=true;s.reason="binary_resources_prepared";return Ready(s.reason);
  }catch(const S::ClientShadowError &e){s.reason=e.Readiness()==S::StateReadiness::Lifecycle?"binary_stage_existing_owner_missing":"binary_stage_description_invalid";return Fail(s.reason);}
   catch(const S::ClientTableError &){s.reason="binary_preparation_budget";return Fail(s.reason);}
   catch(...){s.fault=true;s.reason="binary_preparation_exception";return Fail(s.reason);}
}
S::ClientStateNativeStep ClientSceneMainPreparation::StageObjects(const S::ClientStateApplyBatch &b) noexcept {
  auto &s=*impl_;if(!s.Current(b)||!s.prepared||!s.plan)return Fail("binary_stage_not_prepared");
  if(s.staged)return ObjectsReady(b)?Ready("binary_root_objects_staged"):Fail("binary_stage_owner_changed");
  if(!ClientNativeCpuQueuesEmpty())return Wait("binary_stage_cpu_borrow");
  // 每个 Manager 边界最多取得 32 个私有承载；等待期间不发布任何新对象。
  unsigned work=0;
  while(work<32&&s.object_cursor<s.objects.size()){const auto i=s.object_cursor;auto &slot=s.objects[i];
    if(!slot.required){++s.object_cursor;continue;}++work;
    if(!s.Current(b))return Fail("binary_stage_lifecycle_during_construct");
    if(slot.reuse){if(!s.BorrowCurrent(i))return Fail("binary_stage_reuse_revoked");}
    else {if(!slot.native)slot.native=ClientStagedObjectCreate(s.Allocator());if(!slot.native)return Fail("binary_stage_owner_allocation");
      const auto result=ApplyNative(s,[&]{return ClientStagedObjectConstruct(slot.native,s.roots,s.inputs[i]);});s.reason=result.reason;
      if(result.status==ClientPreparationStatus::Waiting)return Wait(result.reason);
      if(result.status!=ClientPreparationStatus::Ready)return Fail(result.reason);
      if(Structural(s.plan->State().Objects()[i].domain)){
        if(next_binary_instance==UINT64_MAX)return Fail("binary_stage_instance_exhausted");
        slot.instance=++next_binary_instance;
      }}
    ++s.object_cursor;
  }
  if(s.object_cursor<s.objects.size())return Wait("binary_root_objects_staging");
  s.staged=true;if(!ObjectsReady(b))return Fail("binary_stage_readiness_changed");s.reason="binary_root_objects_staged";return Ready(s.reason);
}
bool ClientSceneMainPreparation::Current(const S::ClientStateApplyBatch &b)const noexcept{return impl_->Current(b);}
S::ClientStateNativeStep ClientSceneMainPreparation::StageFields(const S::ClientStateApplyBatch &b) noexcept {
  auto &s=*impl_;if(!s.Current(b)||!ObjectsReady(b))return Fail("binary_fields_roots_not_staged");
  if(s.fields_staged)return FieldsReady(b)?Ready("binary_fields_staged"):Fail("binary_fields_owner_changed");
  // 暂存仅修改 detached 承载；每个 Manager 边界最多进入 32 个字段 owner。
  const auto result=ApplyNative(s,[&]{return s.fields.Stage(32);});s.reason=result.reason;
  if(result.status==ClientPreparationStatus::Waiting)return Wait(result.reason);
  if(result.status!=ClientPreparationStatus::Ready)return Fail(result.reason);
  s.fields_staged=true;return FieldsReady(b)?Ready("binary_fields_staged"):Fail("binary_fields_readiness_changed");
}
bool ClientSceneMainPreparation::FieldsReady(const S::ClientStateApplyBatch &b)const noexcept {
  const auto &s=*impl_;return s.fields_staged&&ObjectsReady(b)&&s.fields.Ready();
}
bool ClientSceneMainPreparation::ObjectsReady(const S::ClientStateApplyBatch &b)const noexcept {
  const auto &s=*impl_;if(!s.Current(b)||!s.staged)return false;
  for(std::size_t i=0;i<s.objects.size();++i)if(!s.BorrowCurrent(i))return false;
  for(const auto &n:s.needs)if(n.native&&!ClientPreparedNativeNeedCurrent(n.native))return false;
  for(const auto &need:s.plan->State().Resources()){const auto *r=impl_->Find(need.key);
    if(!r||r->result!=S::ClientResourceState::Ready||!ClientPreparedResourceCurrent(r->native))return false;}
  return true;
}
bool ClientSceneMainPreparation::VisitObjects(const S::ClientStateApplyBatch &b,void *context,ObjectVisitor visit) noexcept {
  auto &s=*impl_;if(!visit||s.visitors||!ObjectsReady(b))return false;++s.visitors;
  bool ok=true;for(std::uint32_t i=0;i<s.objects.size();++i){const auto &o=s.objects[i];
    if(!o.required)continue;
    if(!s.Current(b)||!s.BorrowCurrent(i)||!visit(context,i,o.reuse?o.borrowed:ClientStagedObjectBorrow(o.native),s.plan->State().Objects()[i].member.identity)){ok=false;break;}}
  --s.visitors;return ok;
}
bool ClientSceneMainPreparation::VisitResources(const S::ClientStateApplyBatch &b,void *context,ResourceVisitor visit) noexcept {
  auto &s=*impl_;if(!visit||s.visitors||!s.Current(b)||!s.prepared)return false;++s.visitors;
  bool ok=true;
  for(const auto &need:s.plan->State().Resources()){auto *r=s.Find(need.key);auto *value=r?ClientPreparedResourceBorrow(r->native):nullptr;
    if(!s.Current(b)||!value||!visit(context,need.key,value)){ok=false;break;}}
  --s.visitors;return ok;
}
bool ClientSceneMainPreparation::VisitFields(const S::ClientStateApplyBatch &b,void *context,ClientSceneFieldStaging::Visitor visit) noexcept {
  auto &s=*impl_;if(!visit||s.visitors||!FieldsReady(b))return false;
  struct Borrow {Impl *owner;const S::ClientStateApplyBatch *batch;void *context;ClientSceneFieldStaging::Visitor visit;};
  Borrow borrow{&s,&b,context,visit};++s.visitors;
  const bool ok=s.fields.Visit(&borrow,[](void *p,std::uint32_t f,const ClientStagedFieldInput &input,ClientStagedField *native)noexcept{
    const auto &v=*static_cast<Borrow *>(p);return v.owner->Current(*v.batch)&&v.visit(v.context,f,input,native);});
  --s.visitors;return ok&&s.Current(b);
}
bool ClientSceneMainPreparation::HoldInstallation(const S::ClientStateApplyBatch &b,const void *owner) noexcept {
  auto &s=*impl_;if(!owner||s.installation||s.visitors||!FieldsReady(b)||s.installed.size()>=5)return false;
  try {s.PlanRootTransfers();s.installed.reserve(5);s.reservation=std::allocate_shared<Impl::Installed>(S::ClientStateAllocator<Impl::Installed>(s.memory),s.memory,s.Allocator());}
  catch(...){s.root_transfers.clear();s.reason="binary_install_custody_reservation_failed";return false;}
  s.installation=owner;return true;
}
bool ClientSceneMainPreparation::ReleaseInstallation(const S::ClientStateApplyBatch &b,const void *owner) noexcept {
  auto &s=*impl_;if(!s.Thread()||s.visitors||!owner||installation_frame.owner)return false;
  if(auto *i=s.InstalledBatch(b)){if(i->installation!=owner)return false;i->installation=nullptr;return true;}
  if(s.installation!=owner||!s.batch||s.batch.get()!=&b)return false;s.installation=nullptr;s.reservation.reset();s.root_transfers.clear();return true;
}
bool ClientSceneMainPreparation::CommitInstallation(const S::ClientStateApplyBatch &b,const void *owner) noexcept {
  auto &s=*impl_;if(!s.Thread()||s.visitors||!owner||s.installation!=owner||!s.batch||s.batch.get()!=&b||!s.reservation||
      s.installed.size()>=5||s.installed.capacity()<5||installation_frame.owner||!s.RootTransfersCurrent())return false;
  for(const auto &t:s.root_transfers)Impl::TransferRoot(t.from->objects[t.source],s.objects[t.target],t.address);
  s.root_transfers.clear();
  // 全部分配在Hold时完成。发布后只移动owner，不复制header，不丢析构前缀或Peer账目。
  auto i=std::move(s.reservation);i->batch=std::move(s.batch);i->plan=std::move(s.plan);i->objects.swap(s.objects);
  i->needs.swap(s.needs);std::swap(i->fields,s.fields);i->installation=s.installation;s.installation=nullptr;
  s.installed.push_back(std::move(i));s.inputs.clear();s.player_types.clear();s.need_cursor=s.object_cursor=0;
  s.prepared=s.staged=s.fields_staged=false;return true;
}
bool ClientSceneMainPreparation::BeginInstallationRetirement(const S::ClientStateApplyBatch &b) noexcept {
  auto &s=*impl_;if(!s.Thread()||!s.Window()||s.visitors||installation_frame.owner)return false;
  if(auto *i=s.InstalledBatch(b))return s.BeginRetirement(*i);
  return s.batch.get()==&b;
}
bool ClientSceneMainPreparation::InstalledObjectCurrent(const S::ClientStateApplyBatch &b,std::uint32_t n,const void *address)const noexcept {
  const auto &s=*impl_;const auto *installed=s.InstalledBatch(b);
  if(!installed||installed->retiring||n>=installed->objects.size()||!address)return false;
  const auto &obj=installed->objects[n];
  if(obj.native)return ClientStagedObjectOwnedAddress(obj.native)==address;
  if(!obj.reuse||obj.borrowed!=address)return false;
  void *current=nullptr;std::uint64_t instance=0;
  return s.ResolveInstalled(installed->plan->State().Objects()[n].member.identity,current,instance)&&current==address&&instance==obj.instance;
}
bool ClientSceneMainPreparation::VisitRetirementObjects(const S::ClientStateApplyBatch &b,void *context,RetirementObjectVisitor visit) noexcept {
  auto &s=*impl_;if(!s.Thread()||!visit||s.visitors||!s.Window())return false;
  const auto *installed=s.InstalledBatch(b);if(installed?!installed->retiring:s.batch.get()!=&b)return false;
  const auto &objects=installed?installed->objects:s.objects;const auto &plan=installed?installed->plan:s.plan;
  if(!plan||objects.size()!=plan->State().Objects().size())return false;
  ++s.visitors;bool ok=true;
  for(std::uint32_t n=0;n<objects.size();++n)if(!visit(context,n,objects[n].native&&Structural(plan->State().Objects()[n].domain))){ok=false;break;}
  --s.visitors;return ok;
}
S::ClientStateNativeStep ClientSceneMainPreparation::ApplyInstallation(const S::ClientStateApplyBatch &b,const void *owner,bool retirement,void *context,InstallationAction action) noexcept {
  auto &s=*impl_;
  const auto *installed=s.InstalledBatch(b);const bool held=installed?installed->installation==owner:s.installation==owner&&s.batch&&s.batch.get()==&b;
  if(!action||!owner||!held||s.visitors||installation_frame.owner||
      !s.Thread()||!s.Window()||(!retirement&&!s.Current(b)))return Fail("binary_install_scope_lifecycle");
  if(!ClientNativeCpuQueuesEmpty())return retirement?Fail("binary_install_retire_cpu_borrow"):Wait("binary_install_cpu_borrow");
  struct Scope {
    Scope(Impl &s,bool retirement){installation_frame={&s,retirement,Impl::ScopeCurrent};}
    ~Scope(){installation_frame={};}
  } purpose(s,retirement);
  ClientDispatchScope dispatch(retirement?ClientOperation::BinaryInstallRetire:ClientOperation::BinaryInstall,ClientEntry::NativeAtomicInstall);
  if(!dispatch.Ready()||ClientDispatchBefore(ClientEntry::NativeAtomicInstall)!=ClientEntryDecision::CallOriginal)return Fail("binary_install_dispatch");
  const auto result=action(context);ClientDispatchReturned(ClientEntry::NativeAtomicInstall);
  if(!dispatch.Succeeded()||!ClientBinaryInstallationScopeActive(retirement))return {false,result.written,true,"binary_install_dispatch_incomplete"};
  return result;
}
bool ClientSceneMainPreparation::DrainBatch(const S::ClientStateApplyBatch &b) noexcept {
  auto &s=*impl_;if(!s.Thread())return false;
  for(auto i=s.installed.begin();i!=s.installed.end();++i)if((*i)->batch.get()==&b){if(!s.DrainInstalled(**i))return false;s.installed.erase(i);return true;}
  return (!s.batch||s.batch.get()==&b)&&s.DrainBatch();
}
bool ClientSceneMainPreparation::Drain() noexcept {
  auto &s=*impl_;if(!s.Thread()||s.active||s.job||!s.DrainBatch())return false;
  for(std::size_t n=s.installed.size();n>0;--n){if(!s.DrainInstalled(*s.installed[n-1]))return false;s.installed.pop_back();}
  // 实际使用者先退，再按依赖逆序释放；cache 元数据清空不充当原生排空凭据。
  if(!s.resources.empty()&&!RetireNative(s,[&]{
    for(std::size_t i=s.resources.size();i>0;--i)if(!ClientPreparedResourceRelease(s.resources[i-1].native)){s.reason="binary_resource_retirement_pending";return false;}
    return true;}))return false;
  s.resources.clear();s.closing=false;s.fault=false;s.roots={};s.vm=nullptr;if(preparation_owner==&s)preparation_owner=nullptr;
  s.reason="binary_preparation_drained";return true;
}
bool ClientSceneMainPreparation::Drained()const noexcept {const auto &s=*impl_;return s.Thread()&&!s.active&&!s.closing&&!s.batch&&!s.job&&s.resources.empty()&&s.installed.empty()&&!s.reservation&&!s.visitors&&!s.installation;}
const char *ClientSceneMainPreparation::Reason()const noexcept{return impl_->Thread()?impl_->reason:"binary_preparation_wrong_thread";}
} // namespace cs::game
