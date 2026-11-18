#include "game/client/scene/client_preparation.h"
#include "game/client/scene/client_lifetime.h"
#include "game/common/bridge.h"
#include "game/common/context.h"
#include "net/live.h"
#include "runtime.h"
#include <algorithm>
#include <chrono>
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
// 原生作用域固定实际票据。资源与退休工作不依赖另一通道的取消位。
struct PreparationFrame {const void *owner=nullptr;bool retirement=false;bool (*current)(const void *,bool)noexcept=nullptr;const S::ClientStateApplyBatch *batch=nullptr;};
bool TicketCurrent(const S::ClientStateApplyBatch *b) noexcept {
  return !b||(!b->cancelled.load(std::memory_order_acquire)&&!b->Withdrawn());
}
thread_local PreparationFrame preparation_frame;
thread_local PreparationFrame installation_frame;
thread_local const void *preparation_owner=nullptr;
class PreparationScope {
public:
  PreparationScope(const void *owner,bool retirement,bool (*current)(const void *,bool)noexcept,const S::ClientStateApplyBatch *batch=nullptr) {
    if(!preparation_frame.owner&&current(owner,retirement)&&TicketCurrent(batch)){preparation_frame={owner,retirement,current,batch};active_=true;}
  }
  ~PreparationScope(){if(active_)preparation_frame={};}
  bool Active()const noexcept{return active_;}
private:bool active_=false;
};
// 实际调用仍进入既有常驻隔离分派；准备资格只来自当前二进制 owner，不依赖旧 target。
template<class O,class F> ClientPreparationResult ApplyNative(O &owner,F &&body,const S::ClientStateApplyBatch *batch=nullptr) noexcept {
  PreparationScope purpose(&owner,false,O::ScopeCurrent,batch);
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
  const auto &frame=preparation_frame;return IsMain()&&frame.owner&&frame.retirement==retirement&&frame.current&&frame.current(frame.owner,retirement)&&(retirement||TicketCurrent(frame.batch));
}
bool ClientBinaryInstallationScopeActive(bool retirement) noexcept {
  const auto &frame=installation_frame;return IsMain()&&frame.owner&&frame.retirement==retirement&&frame.current&&frame.current(frame.owner,retirement)&&(retirement||TicketCurrent(frame.batch));
}
struct ClientSceneMainPreparation::Impl {
  struct Resource {S::ResourceKey key;ClientPreparedResource *native=nullptr;S::ClientResourceState result=S::ClientResourceState::Unprepared;};
  struct Object {
    ClientStagedObject *native=nullptr;void *borrowed=nullptr;
    std::uint64_t instance=0;bool reuse=false,required=false;
    ClientMemberFailure failure=ClientMemberFailure::None;
    // 原地换形后，在本批发布前仍由旧批的同一个池槽 owner 保管。
    S::PresentationMember morph_source{};
  };
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
  // 每槽保管自己的 detached 原生前缀和准备进度；安装根及资源缓存由场景共同保管。
  struct Work {
    S::ClientStateApplyBatchRef batch;
    std::shared_ptr<const ClientSceneInstallPlan> plan;
    S::ClientStateVector<Object> objects;
    S::ClientStateVector<ClientStagedObjectInput> inputs;
    S::ClientStateVector<Need> needs;
    S::ClientStateVector<std::int32_t> player_types;
    ClientSceneFieldStaging fields;
    std::shared_ptr<Installed> reservation;
    const void *installation=nullptr;
    std::size_t need_cursor=0,object_cursor=0;
    std::uint64_t inspected_window=0;
    const char *revalidate_after="stage";
    bool prepared=false,staged=false,fields_staged=false;
    Work(const std::shared_ptr<S::ClientStateMemory> &m,ClientPreparationAllocator a):
      objects(S::ClientStateAllocator<Object>(m)),inputs(S::ClientStateAllocator<ClientStagedObjectInput>(m)),
      needs(S::ClientStateAllocator<Need>(m)),player_types(S::ClientStateAllocator<std::int32_t>(m)),
      fields(m,a) {}
  };
  std::shared_ptr<S::ClientStateMemory> memory;
  std::shared_ptr<const ClientSceneObjectSource> source;
  const std::thread::id thread=std::this_thread::get_id();
  S::StateRoute route{};std::uint64_t generation=0;
  ClientNativeRoots roots{};void *vm=nullptr;
  bool active=false,closing=false,fault=false;
  unsigned visitors=0;
  const char *reason="binary_preparation_idle";
  std::optional<S::ClientStateResourceJob> job;
  S::ClientResourceState feedback=S::ClientResourceState::Pending;
  S::ClientStateVector<Resource> resources;
  S::ClientStateVector<std::shared_ptr<Installed>> installed;
  std::array<Work,2> work;
  Impl(std::shared_ptr<S::ClientStateMemory> m,std::shared_ptr<const ClientSceneObjectSource> s):memory(std::move(m)),source(std::move(s)),
    resources(S::ClientStateAllocator<Resource>(memory)),installed(S::ClientStateAllocator<std::shared_ptr<Installed>>(memory)),
    work{Work(memory,Allocator()),Work(memory,Allocator())} {}
  const Work &Pending(const S::ClientStateApplyBatch &b)const noexcept {
    return work[S::StateApplyChannel(b.target.candidate->Target())];
  }
  Work &Pending(const S::ClientStateApplyBatch &b) noexcept {
    return work[S::StateApplyChannel(b.target.candidate->Target())];
  }
  bool Thread()const noexcept{return thread==std::this_thread::get_id();}
  bool Window()const noexcept{return Thread()&&ClientPreparationWindow(roots,vm);}
  static bool ScopeCurrent(const void *p,bool retirement) noexcept {
    const auto &s=*static_cast<const Impl *>(p);
    return s.Thread()&&preparation_owner==p&&s.Window()&&(retirement||(s.active&&!s.closing&&!s.fault));
  }
  bool Current(const S::ClientStateApplyBatch &b)const noexcept {
    return Thread()&&active&&!closing&&!fault&&Window()&&b.ticket.serial&&b.ticket.route==route&&TicketCurrent(&b)&&b.target.candidate&&
      b.target.install&&b.target.install->Memory()==memory&&b.target.install->Describes(b.target.candidate)&&
      (!Pending(b).batch||Pending(b).batch.get()==&b||InstalledBatch(b));
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
    if(!p||!address)return S::kClientPlanNone;
    auto n=ObjectIndex(*p,id);
    if(n==S::kClientPlanNone){
      const auto objects=p->State().Objects();
      const auto next=std::upper_bound(objects.begin(),objects.end(),id,
        [](const auto &key,const auto &object){return key<object.member.identity;});
      if(next==objects.end())return S::kClientPlanNone;
      n=std::uint32_t(next-objects.begin());
      if(n>=values.size()||values[n].morph_source!=id)return S::kClientPlanNone;
    }
    if(n>=values.size())return S::kClientPlanNone;const auto &o=values[n];
    return o.required&&o.reuse&&o.borrowed==address&&o.instance&&(!instance||o.instance==instance)?n:S::kClientPlanNone;
  }
  bool Borrowed(const Installed &owned)const noexcept {
    for(std::uint32_t n=0;n<owned.objects.size();++n){const auto &object=owned.objects[n];
      if(!object.native||!Structural(owned.plan->State().Objects()[n].domain))continue;
      auto *address=ClientStagedObjectOwnedAddress(object.native);if(!address)return true;
      const auto &id=owned.plan->State().Objects()[n].member.identity;
      for(const auto &w:work)if(Borrower(w.plan.get(),w.objects,id,address,object.instance)!=S::kClientPlanNone)return true;
      for(const auto &other:installed)if(other.get()!=&owned&&Borrower(other->plan.get(),other->objects,id,address,object.instance)!=S::kClientPlanNone)return true;
    }
    return false;
  }
  static void TransferRoot(Object &old,Object &next,void *address) noexcept {
    // 原批次降为普通借用，新的构造owner因此不能越过仍存活的旧用途提前析构。
    next.native=old.native;old.native=nullptr;old.borrowed=address;old.instance=next.instance;old.reuse=true;
  }
  // 只在实际发布时转交同一原生实例。先核对所有双方地址，再移动唯一析构责任。
  bool AdoptLiveRoots(Work &w) noexcept {
    const auto transfer=[&](bool apply){
      for(const auto &prior:installed)for(std::uint32_t n=0;n<prior->objects.size();++n){
        auto &old=prior->objects[n];
        if(!old.native||!Structural(prior->plan->State().Objects()[n].domain))continue;
        auto *address=ClientStagedObjectOwnedAddress(old.native);
        if(!address)return false;
        const auto target=Borrower(w.plan.get(),w.objects,prior->plan->State().Objects()[n].member.identity,address,old.instance);
        if(target==S::kClientPlanNone)continue;
        if(prior->retiring||w.objects[target].native)return false;
        if(apply)TransferRoot(old,w.objects[target],address);
      }
      return true;
    };
    return transfer(false)&&transfer(true);
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
      std::uint32_t index=0;if(RetirementSuccessor(owned,n,address,index))continue;
      for(const auto &w:work)if(Borrower(w.plan.get(),w.objects,id,address,old.instance)!=S::kClientPlanNone)return false;
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
    // 资源装载可执行原生回调；两路已取得的同窗口见证都须重新核定。
    for(auto &w:work){w.inspected_window=0;w.revalidate_after="after_resource_load";}
    const auto result=ApplyNative(*this,[&]{return ClientPreparedResourceLoad(entry->native,d,first,second);});reason=result.reason;
    switch(result.status){case ClientPreparationStatus::Waiting:return;case ClientPreparationStatus::Ready:entry->result=S::ClientResourceState::Ready;break;
      case ClientPreparationStatus::Missing:entry->result=S::ClientResourceState::MissingLocalResource;break;case ClientPreparationStatus::Failed:entry->result=S::ClientResourceState::Failed;break;}
    feedback=entry->result;
  }
  bool BorrowCurrent(const Work &w,std::size_t i,bool live=false)const noexcept {
    const auto &o=w.objects[i];if(!o.required)return true;
    if(o.reuse){
      const auto &id=o.morph_source.id?o.morph_source:w.plan->State().Objects()[i].member.identity;
      void *address=nullptr;
      std::uint64_t instance=0;
      return ResolveInstalled(id,address,instance,live)&&address==o.borrowed&&instance==o.instance;
    }
    return
      !o.native||ClientStagedObjectCurrent(o.native,roots);
  }
  bool ResolveInstalled(const S::PresentationMember &id,void *&address,std::uint64_t &instance,bool live=false)const noexcept {
    for(auto i=installed.rbegin();i!=installed.rend();++i){
      if((*i)->retiring||(*i)->batch->cancelled.load(std::memory_order_acquire))continue;
      const auto n=ObjectIndex(*(*i)->plan,id);
      if(n>=(*i)->objects.size())continue;
      const auto &o=(*i)->objects[n];
      if(!o.native||!o.instance)continue;
      address=live?ClientStagedObjectLiveAddress(o.native):ClientStagedObjectOwnedAddress(o.native);
      if(live&&!address)return false;
      if(address){instance=o.instance;return true;}
    }
    return source&&source->Resolve(id,address,instance)&&address&&instance&&source->Current(id,address,instance);
  }
  void DescribeObjects(Work &w) {
    const auto &s=w.plan->State();w.objects.resize(s.Objects().size());w.inputs.resize(w.objects.size());w.needs.resize(s.NativeNeeds().size());
    const bool world=(S::StateTargetStreams(s.Target())&1)!=0;
    S::ClientShadowStateRef installed_world;
    if(!world)for(auto i=installed.rbegin();i!=installed.rend();++i)
      if(!(*i)->retiring&&(*i)->plan->Display()){
        installed_world=(*i)->plan->State().Effective(S::BodyStream::World);break;
      }
    // 配对旧根只供窄依赖使用；body 不重新暂存无关世界。引用实际触及的旧承载仍逐项重借。
    for(auto object:s.StageObjects())w.objects.at(object).required=true;
    for(std::uint32_t i=0;i<s.BindingCount();++i){
      const auto link=s.Binding(i);
      if(!link.is_resource&&!link.is_null&&link.object!=S::kClientPlanNone)w.objects.at(link.object).required=true;
    }
    // 对象描述和事件 owner 已由后台按完整身份定位；Body 没有无关实体的描述。
    for(const auto &event:s.Events())if(event.owner_object!=S::kClientPlanNone)
      w.objects.at(event.owner_object).required=true;
    for(const auto &p:s.Players())if(std::find(w.player_types.begin(),w.player_types.end(),p.player_type)==w.player_types.end())w.player_types.push_back(p.player_type);
    for(std::uint32_t i=0;i<w.objects.size();++i){const auto &o=s.Objects()[i];auto &out=w.objects[i];auto &in=w.inputs[i];
      if(!out.required)continue;
      const auto *layout=S::FindLayout(o.member.identity.layout);if(!layout)throw S::ClientShadowError(S::StateReadiness::InvalidEncoding,"binary_stage_layout");
      in.layout=layout->id;in.native_size=layout->native_size;
      // 以原生场景实际成员为准。完整基线可以复用同身份根，上一帧缺池槽的成员可以重新申请。
      if(!world&&o.domain==S::ClientInstallDomain::Player&&
          !S::ClientCostumeDirectoryMatches(s.Effective(S::BodyStream::World),installed_world,o.member.identity)){
        out.required=false;continue;
      }
      const bool borrowed=Structural(o.domain)&&ResolveInstalled(o.member.identity,out.borrowed,out.instance,true);
      if(Structural(o.domain)&&!borrowed&&!world){out.required=false;continue;}
      if(borrowed){
        out.reuse=true;in.borrowed=out.borrowed;
        for(const auto &prior:installed)if(prior->retiring){const auto n=ObjectIndex(*prior->plan,o.member.identity);
          if(n<prior->objects.size()&&prior->objects[n].native)throw S::ClientShadowError(S::StateReadiness::Lifecycle,"binary_stage_root_already_retiring");}
        if(o.domain==S::ClientInstallDomain::Entity&&o.description<s.Entities().size()){
          const auto &e=s.Entities()[o.description];
          in.kind=ClientStagedKind::Entity;in.type=e.type;in.category=Category(e,in.layout);
        }
        continue;
      }
      switch(o.domain){
        case S::ClientInstallDomain::Participant:case S::ClientInstallDomain::ControlGroup:in.kind=ClientStagedKind::Logical;break;
        case S::ClientInstallDomain::Run:in.kind=ClientStagedKind::Game;break;
        case S::ClientInstallDomain::Level:in.kind=ClientStagedKind::Level;break;
        case S::ClientInstallDomain::Room:in.kind=ClientStagedKind::Room;break;
        case S::ClientInstallDomain::Sfx:in.kind=ClientStagedKind::Sfx;break;
        case S::ClientInstallDomain::Descriptor:{
          if(o.description>=s.Descriptors().size())throw S::ClientShadowError(S::StateReadiness::InvalidReference,"binary_stage_descriptor_slot");
          in.kind=ClientStagedKind::Descriptor;in.index=s.Descriptors()[o.description].source_slot;break;}
        case S::ClientInstallDomain::Grid:{
          if(o.description>=s.Grids().size())throw S::ClientShadowError(S::StateReadiness::InvalidReference,"binary_stage_grid_description");
          const auto &g=s.Grids()[o.description];in.kind=ClientStagedKind::Grid;
          in.type=std::uint32_t(g.type);in.variant=std::uint32_t(g.variant);in.index=g.index;in.seed=g.spawn_seed;break;}
        case S::ClientInstallDomain::Player:case S::ClientInstallDomain::Entity:{
          if(o.description>=s.Entities().size())throw S::ClientShadowError(S::StateReadiness::InvalidReference,"binary_stage_entity_description");
          const auto &e=s.Entities()[o.description];
          in.kind=o.domain==S::ClientInstallDomain::Player?ClientStagedKind::Player:ClientStagedKind::Entity;
          in.type=e.type;in.variant=e.variant;in.subtype=e.subtype;in.seed=e.init_seed;in.category=Category(e,in.layout);break;}
      }
    }
  }
  bool DrainBatch(Work &w) noexcept {
    if(visitors||w.installation||!Thread())return false;
    w.inspected_window=0;
    if(w.fields.HasNative()||std::any_of(w.objects.begin(),w.objects.end(),[](const auto &o){return o.native;})||std::any_of(w.needs.begin(),w.needs.end(),[](const auto &n){return n.native;})) {
      if(!RetireNative(*this,[&]{
        if(!w.fields.Drain()){reason="binary_fields_retirement_pending";return false;}
        for(std::size_t i=w.objects.size();i>0;--i)if(!ClientStagedObjectRelease(w.objects[i-1].native)){reason="binary_stage_retirement_pending";return false;}
        for(std::size_t i=w.needs.size();i>0;--i)if(!ClientPreparedNativeNeedRelease(w.needs[i-1].native)){reason="binary_native_need_retirement_pending";return false;}
        return true;}))return false;
    }
    if(!w.fields.Drain())return false;
    w.objects.clear();w.inputs.clear();w.needs.clear();w.player_types.clear();w.plan.reset();w.batch.reset();w.reservation.reset();w.need_cursor=w.object_cursor=0;w.prepared=w.staged=w.fields_staged=false;w.revalidate_after="stage";return true;
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
  auto &s=*impl_;if(!s.Thread()||s.work[0].batch||s.work[1].batch||s.job||!generation||!r.connection||!r.route||!r.scope.session||!r.scope.run||!r.scope.scene)return false;
  const auto roots=ClientPreparationRoots();if(!roots.game||!roots.room)return false;
  if(s.active||s.closing||!s.resources.empty()||!s.installed.empty()||preparation_owner)return false;
  preparation_owner=&s;s.route=r;s.generation=generation;s.roots=roots;s.vm=ClientLuaStateIdentity();s.active=true;s.fault=false;s.reason="binary_preparation_open";return true;
}
void ClientSceneMainPreparation::Close() noexcept {
  auto &s=*impl_;if(s.Thread()){for(auto &w:s.work)w.inspected_window=0;s.active=false;s.closing=true;s.reason="binary_preparation_closing";}
}
bool ClientSceneMainPreparation::OfferResource(S::ClientStateResourceJob job) noexcept {
  auto &s=*impl_;if(!s.Thread()||s.job||!job.resources||job.resources->Memory()!=s.memory||!job.ticket.serial)return false;
  s.job=std::move(job);s.feedback=S::ClientResourceState::Pending;return true;
}
ClientSceneMainPreparation::ResourcePump ClientSceneMainPreparation::PumpResources(net::live::Client &transport,
    std::chrono::steady_clock::time_point deadline) noexcept {
  auto &s=*impl_;if(!s.Thread())return ResourcePump::Blocked;
  const auto end=(std::min)(deadline,deadline==std::chrono::steady_clock::time_point::max()?deadline:std::chrono::steady_clock::now()+std::chrono::milliseconds(2));
  for(std::size_t i=0;;++i){
    if(!s.job){
      S::ClientStateResourceJob next;
      if(!transport.PollBinaryResource(next))return ResourcePump::Done;
      s.job=std::move(next);s.feedback=S::ClientResourceState::Pending;
    }
    // 完整原生调用不可切分；每次调用前检查剩余时间，保留下一项及原回执。
    if(std::chrono::steady_clock::now()>=end||i==S::kClientResourceJobsInFlight)return ResourcePump::Yielded;
    if(s.feedback==S::ClientResourceState::Pending)try{s.ResourceStep();}catch(const S::ClientTableError &){s.feedback=S::ClientResourceState::Failed;s.reason="binary_resource_budget";}
      catch(...){s.feedback=S::ClientResourceState::Failed;s.reason="binary_resource_preparation_exception";s.fault=true;}
    if(s.feedback==S::ClientResourceState::Pending)return ResourcePump::Blocked;
    if(!s.ResourceRoute(*s.job))s.feedback=S::ClientResourceState::Unprepared;
    if(!transport.CompleteBinaryResource(s.job->ticket,s.feedback))return ResourcePump::Blocked;
    s.job.reset();s.feedback=S::ClientResourceState::Pending;
  }
}
S::ClientStateNativeStep ClientSceneMainPreparation::Prepare(S::ClientStateApplyBatchRef b) noexcept {
  auto &s=*impl_;if(!b||!s.Current(*b))return Fail("binary_preparation_lifecycle");
  auto &w=s.Pending(*b);
  if(w.prepared)return Ready("binary_resources_prepared");
  try {
    if(!w.batch){const auto plan=std::dynamic_pointer_cast<const ClientSceneInstallPlan>(b->native);
      if(!plan||&plan->State()!=b->target.install.get())return Fail("binary_preparation_plan_type");
      w.batch=std::move(b);w.plan=plan;s.DescribeObjects(w);w.fields.Describe(plan->State());}
    const auto &plan=w.plan->State();
    for(const auto &need:plan.Resources()){const auto directory=need.resources->Directory();
      if(directory.session!=s.route.scope.session||directory.run!=s.route.scope.run)return Fail("binary_preparation_resource_scope");
      auto *entry=s.Find(need.key);if(!entry||entry->result!=S::ClientResourceState::Ready||!ClientPreparedResourceCurrent(entry->native))return Fail("binary_preparation_resource_not_materialized");}
    if(!ClientNativeCpuQueuesEmpty()){
#ifdef ISAACCS_E2E
      s.memory->ApplyMetrics().Wait(S::StateTargetStreams(plan.Target()),S::ClientApplyWait::CpuQueueBorrow);
#endif
      return Wait("binary_preparation_cpu_borrow");
    }
    while(w.need_cursor<plan.NativeNeeds().size()){
      const auto &need=plan.NativeNeeds()[w.need_cursor];auto &slot=w.needs[w.need_cursor];
      for(std::uint32_t i=0;i<w.need_cursor;++i){const auto &prior=plan.NativeNeeds()[i];if(prior.kind==need.kind&&prior.id==need.id&&prior.second_id==need.second_id){slot.alias=i;break;}}
      if(slot.alias==S::kClientPlanNone){if(!slot.native)slot.native=ClientPreparedNativeNeedCreate(s.Allocator());if(!slot.native)return Fail("binary_native_need_allocation");
        ClientNativeNeedInput input{unsigned(need.kind),need.id,need.second_id,w.player_types.data(),w.player_types.size()};
        const auto result=ApplyNative(s,[&]{return ClientPreparedNativeNeedLoad(slot.native,input);},w.batch.get());s.reason=result.reason;
        if(result.status==ClientPreparationStatus::Waiting)return Wait(result.reason);
        if(result.status!=ClientPreparationStatus::Ready)return Fail(result.reason);}
      ++w.need_cursor;
    }
    const auto pools=ApplyNative(s,[&]{return ClientStagedPoolNotificationsCurrent(s.roots,w.inputs.data(),w.inputs.size());},w.batch.get());
    if(pools.status!=ClientPreparationStatus::Ready)return Fail(pools.reason);
    w.prepared=true;s.reason="binary_resources_prepared";return Ready(s.reason);
  }catch(const S::ClientShadowError &e){s.reason=e.Readiness()==S::StateReadiness::Lifecycle?"binary_stage_existing_owner_missing":"binary_stage_description_invalid";return Fail(s.reason);}
   catch(const S::ClientTableError &){s.reason="binary_preparation_budget";return Fail(s.reason);}
   catch(...){s.fault=true;s.reason="binary_preparation_exception";return Fail(s.reason);}
}
S::ClientStateNativeStep ClientSceneMainPreparation::StageObjects(const S::ClientStateApplyBatch &b) noexcept {
  auto &s=*impl_;auto &w=s.Pending(b);if(!s.Current(b)||!w.prepared||!w.plan)return Fail("binary_stage_not_prepared");
  if(w.staged)return Ready("binary_root_objects_staged");
  if(!ClientNativeCpuQueuesEmpty()){
#ifdef ISAACCS_E2E
    s.memory->ApplyMetrics().Wait(S::StateTargetStreams(w.plan->State().Target()),S::ClientApplyWait::CpuQueueBorrow);
#endif
    return Wait("binary_stage_cpu_borrow");
  }
  // 在同一 Manager 窗口核身份和预约原生根，单个池槽失败由对应成员承担。
  while(w.object_cursor<w.objects.size()){
    const auto i=w.object_cursor;auto &slot=w.objects[i];
    if(!slot.required){++w.object_cursor;continue;}
    if(!s.Current(b))return Fail("binary_stage_lifecycle_during_construct");
    if(slot.reuse){
      if(!s.BorrowCurrent(w,i,true)){
        slot.required=false;slot.failure=ClientMemberFailure::Stale;
      }
    }
    else {if(!slot.native)slot.native=ClientStagedObjectCreate(s.Allocator());if(!slot.native)return Fail("binary_stage_owner_allocation");
      const auto result=ApplyNative(s,[&]{return ClientStagedObjectConstruct(slot.native,s.roots,w.inputs[i]);},&b);s.reason=result.reason;
      if(result.status==ClientPreparationStatus::Waiting)return Wait(result.reason);
      if(result.status==ClientPreparationStatus::Missing&&w.inputs[i].kind==ClientStagedKind::Entity){
        // 池不足只省掉该根。引用解析仍保留成员键，下一份完整目标会再试。
        if(!RetireNative(s,[&]{return ClientStagedObjectRelease(slot.native);}))return Fail("binary_stage_pool_reservation_release");
        slot.required=false;
        slot.failure=ClientMemberFailure::Pool;
        s.memory->ApplyMetrics().Volume(1,S::ClientApplyVolume::PoolMembersDeferred);
#ifdef ISAACCS_E2E
        Trace("client","event=binary_entity_pool_full member=%llu type=%u",w.plan->State().Objects()[i].member.identity.id,w.inputs[i].type);
#endif
        ++w.object_cursor;continue;
      }
      if(result.status!=ClientPreparationStatus::Ready){
        // 构造失败仅在已确认归还保留位时隔离，保留不明前缀的原生对象不可复用。
        if(!Structural(w.plan->State().Objects()[i].domain)||
            !RetireNative(s,[&]{return ClientStagedObjectRelease(slot.native);}))return Fail(result.reason);
        slot.required=false;slot.failure=ClientMemberFailure::Native;
        const auto &id=w.plan->State().Objects()[i].member.identity;
        S::ClientApplyMetrics::Issue issue;
        issue.route=b.ticket.route.route;issue.ticket=b.ticket.serial;issue.member=id.id;issue.layout=id.layout;
        s.memory->ApplyMetrics().FirstIssue(S::StateTargetStreams(w.plan->State().Target()),issue,"construct",result.reason);
        ++w.object_cursor;continue;
      }
      if(Structural(w.plan->State().Objects()[i].domain)){
        if(next_binary_instance==UINT64_MAX)return Fail("binary_stage_instance_exhausted");
        slot.instance=++next_binary_instance;
      }}
    ++w.object_cursor;
  }
  w.staged=true;s.reason="binary_root_objects_staged";return Ready(s.reason);
}
bool ClientSceneMainPreparation::Current(const S::ClientStateApplyBatch &b)const noexcept{return impl_->Current(b);}
S::ClientStateNativeStep ClientSceneMainPreparation::StageFields(const S::ClientStateApplyBatch &b) noexcept {
  auto &s=*impl_;auto &w=s.Pending(b);if(!s.Current(b)||w.batch.get()!=&b||!w.staged)return Fail("binary_fields_roots_not_staged");
  if(w.fields_staged)return Ready("binary_fields_staged");
  if(!ClientNativeCpuQueuesEmpty()){
#ifdef ISAACCS_E2E
    s.memory->ApplyMetrics().Wait(S::StateTargetStreams(b.target.candidate->Target()),S::ClientApplyWait::CpuQueueBorrow);
#endif
    return Wait("binary_fields_cpu_borrow");
  }
  w.fields.SkipObjects(&w,[](const void *context,std::uint32_t object) noexcept {
    const auto &work=*static_cast<const Impl::Work *>(context);
    return object>=work.objects.size()||!work.objects[object].required;
  });
  // 先从仍存活的已提交保管区重借；新旧根实例必须相同。
  struct ReuseContext {const Impl::Work &next;const Impl::Installed &old;};
  for(auto it=s.installed.rbegin();it!=s.installed.rend();++it){
    const auto &prior=**it;
    if(prior.retiring||!TicketCurrent(prior.batch.get()))continue;
    const ReuseContext context{w,prior};
    w.fields.ReuseFrom(prior.fields,&context,[](const void *p,std::uint32_t n,std::uint32_t before)noexcept{
      const auto &c=*static_cast<const ReuseContext *>(p);
      if(n>=c.next.objects.size()||before>=c.old.objects.size())return false;
      const auto &next=c.next.objects[n],&old=c.old.objects[before];
      if(!next.required||!old.required||next.instance!=old.instance)return false;
      const auto address=[](const Impl::Object &o){return o.native?ClientStagedObjectOwnedAddress(o.native):o.borrowed;};
      const auto *root=address(next);
      return root&&root==address(old);
    });
  }
  const auto result=ApplyNative(s,[&]{return w.fields.Stage();},&b);s.reason=result.reason;
  if(result.status==ClientPreparationStatus::Waiting)return Wait(result.reason);
  if(result.status!=ClientPreparationStatus::Ready)return Fail(result.reason);
  w.fields_staged=true;return Ready("binary_fields_staged");
}
bool ClientSceneMainPreparation::InspectStaged(const S::ClientStateApplyBatch &b,void *context,
    ObjectVisitor object,ResourceVisitor resource,ClientSceneFieldStaging::Visitor field) noexcept {
  auto &s=*impl_;auto &w=s.Pending(b);
  if(!s.Current(b)||w.batch.get()!=&b||!w.fields_staged||s.visitors||!object||!resource||!field)return false;
  const auto window=OwnerToken(Owner::Manager);
  w.inspected_window=0;
  if(!window)return false;
  struct VisitScope {
    unsigned &visitors;
    explicit VisitScope(unsigned &v):visitors(v){++visitors;}
    ~VisitScope(){--visitors;}
  } scope(s.visitors);
  // 同一次保管扫描既验证真实 owner，又返回该次验证见到的地址，避免随后再借一次。
  const auto &plan=w.plan->State();
  const auto streams=S::StateTargetStreams(plan.Target());
  const auto issue=[&](S::ClientApplyMetrics::Issue fact,const char *reason){
    fact.route=b.ticket.route.route;fact.ticket=b.ticket.serial;
    s.memory->ApplyMetrics().FirstIssue(streams,fact,w.revalidate_after,reason);
    return false;
  };
  std::uint64_t checks=0;
  struct CheckCount {
    S::ClientApplyMetrics &metrics;const std::uint64_t &n;std::uint8_t streams;
    ~CheckCount(){metrics.Volume(streams,S::ClientApplyVolume::Checks,n);}
  } count{s.memory->ApplyMetrics(),checks,streams};
  for(std::uint32_t i=0;i<w.objects.size();++i){
    if(i%128==0&&!s.Current(b))return false;
    const auto &slot=w.objects[i];
    if(!slot.required)continue;
    ++checks;
    void *address=slot.reuse?(s.BorrowCurrent(w,i)?slot.borrowed:nullptr):ClientStagedObjectBorrow(slot.native);
    const auto &id=plan.Objects()[i].member.identity;
    // 参与者与控制组是纯逻辑成员，合法暂存没有原生地址；仍须核其 owner 与生命周期。
    const auto domain=plan.Objects()[i].domain;
    const bool logical=domain==S::ClientInstallDomain::Participant||domain==S::ClientInstallDomain::ControlGroup;
    const bool current=address||(logical&&!slot.reuse&&ClientStagedObjectCurrent(slot.native,s.roots));
    if(!current||!object(context,i,address,id,slot.instance,slot.reuse)){
      S::ClientApplyMetrics::Issue fact;fact.member=id.id;fact.layout=id.layout;fact.instance=slot.instance;
      fact.expected=reinterpret_cast<std::uintptr_t>(slot.borrowed);fact.actual=reinterpret_cast<std::uintptr_t>(address);
      return issue(fact,"staged_object_changed");
    }
  }
  for(std::size_t i=0;i<w.needs.size();++i){
    if(i%128==0&&!s.Current(b))return false;
    if(w.needs[i].native){
      ++checks;
      if(!ClientPreparedNativeNeedCurrent(w.needs[i].native)){
        const auto &need=plan.NativeNeeds()[i];
        S::ClientApplyMetrics::Issue fact;fact.resource_key=std::uint32_t(need.id);fact.instance=need.generation;
        return issue(fact,"staged_native_resource_changed");
      }
    }
  }
  std::size_t index=0;
  for(const auto &need:plan.Resources()){
    if(index++%128==0&&!s.Current(b))return false;
    ++checks;
    auto *entry=s.Find(need.key);
    auto *address=entry&&entry->result==S::ClientResourceState::Ready?ClientPreparedResourceBorrow(entry->native):nullptr;
    if(!address||!resource(context,need.key,address)){
      S::ClientApplyMetrics::Issue fact;fact.resource_epoch=need.resources->Directory().epoch;
      fact.resource_key=(std::uint32_t(need.key.table)<<16)|need.key.index;
      fact.actual=reinterpret_cast<std::uintptr_t>(address);
      return issue(fact,"staged_resource_changed");
    }
  }
  struct Window {const Impl &owner;const S::ClientStateApplyBatch &batch;} checked{s,b};
  if(!s.Current(b)||!w.fields.Visit(context,field,&checked,[](const void *p)noexcept{
      const auto &w=*static_cast<const Window *>(p);return w.owner.Current(w.batch);
    },b.ticket.serial,w.revalidate_after)||!s.Current(b)||OwnerToken(Owner::Manager)!=window)return false;
  w.inspected_window=window;
  return true;
}
bool ClientSceneMainPreparation::PendingObjectCurrent(const S::ClientStateApplyBatch &b,std::uint32_t index,
    void *address,std::uint64_t instance,bool after_init)const noexcept {
  const auto &s=*impl_;auto &w=s.Pending(b);
  if(!s.Current(b)||w.batch.get()!=&b||index>=w.objects.size())return false;
  const auto &slot=w.objects[index];
  if(!slot.required||slot.instance!=instance)return false;
  if(slot.reuse)return slot.borrowed==address&&s.BorrowCurrent(w,index,true);
  // Init 后 _exists 已可置位，直到登记名册前仍由本票据保管，按 owner 与 vptr 核验。
  const auto owned=after_init?ClientStagedObjectOwnedAddress(slot.native):ClientStagedObjectBorrow(slot.native);
  return owned==address;
}
ClientMemberFailure ClientSceneMainPreparation::StagedFailure(const S::ClientStateApplyBatch &b,std::uint32_t n)const noexcept {
  const auto &w=impl_->Pending(b);
  return w.batch.get()==&b&&n<w.objects.size()?w.objects[n].failure:ClientMemberFailure::None;
}
bool ClientSceneMainPreparation::OwnsFailedObject(const S::ClientStateApplyBatch &b,std::uint32_t n,
    void *address,std::uint64_t instance)const noexcept {
  const auto &s=*impl_;const auto &w=s.Pending(b);
  if(!s.Window()||w.batch.get()!=&b||n>=w.objects.size()||!address||!instance)return false;
  const auto &failed=w.objects[n];
  if(!failed.required||failed.instance!=instance)return false;
  if(failed.native)return ClientStagedObjectOwnedAddress(failed.native)==address;
  const auto &id=failed.morph_source.id?failed.morph_source:w.plan->State().Objects()[n].member.identity;
  for(const auto &owner:s.installed){
    const auto i=Impl::ObjectIndex(*owner->plan,id);
    if(i>=owner->objects.size())continue;
    const auto &object=owner->objects[i];
    if(object.native&&object.instance==instance)return ClientStagedObjectOwnedAddress(object.native)==address;
  }
  return false;
}
bool ClientSceneMainPreparation::ReleaseFailedObject(const S::ClientStateApplyBatch &b,std::uint32_t n,
    void *address,std::uint64_t instance) noexcept {
  auto &s=*impl_;auto &w=s.Pending(b);
  if(!s.Window()||s.visitors||installation_frame.owner||w.batch.get()!=&b||n>=w.objects.size())return false;
  auto &failed=w.objects[n];
  if(!failed.required||failed.instance!=instance)return false;
  const auto &identity=failed.morph_source.id?failed.morph_source:w.plan->State().Objects()[n].member.identity;
  bool released=false;
  if(failed.native){
    if(ClientStagedObjectOwnedAddress(failed.native)!=address)return false;
    released=RetireNative(s,[&]{return ClientStagedObjectRelease(failed.native);});
  }else for(const auto &owner:s.installed){
    const auto i=Impl::ObjectIndex(*owner->plan,identity);
    if(i>=owner->objects.size())continue;
    auto &object=owner->objects[i];
    if(object.native&&object.instance==instance&&ClientStagedObjectOwnedAddress(object.native)==address){
      released=RetireNative(s,[&]{return ClientStagedObjectRelease(object.native);});
      break;
    }
  }
  if(!released)return false;
  failed.required=failed.reuse=false;failed.borrowed=nullptr;
  return true;
}
bool ClientSceneMainPreparation::AdoptMorph(const S::ClientStateApplyBatch &b,std::uint32_t n,
    const S::PresentationMember &prior,void *address,std::uint64_t instance,const S::ClientInstallEntity &entity) noexcept {
  auto &s=*impl_;auto &w=s.Pending(b);
  if(!s.Current(b)||w.batch.get()!=&b||n>=w.objects.size())return false;
  auto &next=w.objects[n];
  if(!next.required||next.reuse||next.native||!instance)return false;
  for(const auto &owner:s.installed){
    const auto i=Impl::ObjectIndex(*owner->plan,prior);
    if(i>=owner->objects.size()||owner->retiring)continue;
    auto &object=owner->objects[i];
    if(!object.native||object.instance!=instance)continue;
    if(!ClientStagedObjectAdoptShape(object.native,address,entity.type,entity.variant,entity.subtype,entity.init_seed))return false;
    next.reuse=true;next.borrowed=address;next.instance=instance;next.morph_source=prior;
    w.inputs[n].borrowed=address;
    return true;
  }
  return false;
}
bool ClientSceneMainPreparation::StagedCurrent(const S::ClientStateApplyBatch &b)const noexcept {
  const auto &s=*impl_;auto &w=s.Pending(b);
  return s.Current(b)&&w.batch.get()==&b&&w.inspected_window&&w.inspected_window==OwnerToken(Owner::Manager);
}
void ClientSceneMainPreparation::RevokeStaged(const S::ClientStateApplyBatch &b,const char *cause) noexcept {
  auto &s=*impl_;auto &w=s.Pending(b);
  if(s.Thread()&&w.batch.get()==&b){w.inspected_window=0;w.revalidate_after=cause;}
}
bool ClientSceneMainPreparation::BeginLiveUse(const S::ClientStateApplyBatch &b,const void *owner) noexcept {
  auto &s=*impl_;auto &w=s.Pending(b);const bool staged_current=StagedCurrent(b);
  if(!owner||w.installation||s.visitors||!staged_current||s.installed.size()>=5){
    // 只记失败首现场，区分容量占用与票据／窗口失效，不用统称lease掩盖实际分支。
    Trace("client_error","event=binary_native_use ticket=%llu owner=%u held=%u visitors=%u staged=%u installed=%u cancelled=%u withdrawn=%u",
      b.ticket.serial,unsigned(owner!=nullptr),unsigned(w.installation!=nullptr),s.visitors,unsigned(staged_current),
      unsigned(s.installed.size()),unsigned(b.cancelled.load(std::memory_order_acquire)),unsigned(b.Withdrawn()));
    return false;
  }
  try {s.installed.reserve(5);w.reservation=std::allocate_shared<Impl::Installed>(S::ClientStateAllocator<Impl::Installed>(s.memory),s.memory,s.Allocator());}
  catch(...){s.reason="binary_live_use_reservation_failed";return false;}
  w.installation=owner;return true;
}
bool ClientSceneMainPreparation::ReleaseLiveUse(const S::ClientStateApplyBatch &b,const void *owner) noexcept {
  auto &s=*impl_;auto &w=s.Pending(b);if(!s.Thread()||s.visitors||!owner||installation_frame.owner)return false;
  if(auto *i=s.InstalledBatch(b)){if(i->installation!=owner)return false;i->installation=nullptr;return true;}
  if(w.installation!=owner||!w.batch||w.batch.get()!=&b)return false;w.installation=nullptr;w.reservation.reset();return true;
}
bool ClientSceneMainPreparation::PublishLiveUse(const S::ClientStateApplyBatch &b,const void *owner) noexcept {
  auto &s=*impl_;auto &w=s.Pending(b);if(!s.Thread()||s.visitors||!owner||w.installation!=owner||!w.batch||w.batch.get()!=&b||!w.reservation||
      s.installed.size()>=5||s.installed.capacity()<5||installation_frame.owner||!s.AdoptLiveRoots(w))return false;
  // 已接入活对象的字段和资源继续保留到真实用途结束。
  auto i=std::move(w.reservation);i->batch=std::move(w.batch);i->plan=std::move(w.plan);i->objects.swap(w.objects);
  i->needs.swap(w.needs);std::swap(i->fields,w.fields);i->installation=w.installation;w.installation=nullptr;
  s.installed.push_back(std::move(i));w.inputs.clear();w.player_types.clear();w.need_cursor=w.object_cursor=0;
  w.inspected_window=0;w.prepared=w.staged=w.fields_staged=false;w.revalidate_after="stage";return true;
}
bool ClientSceneMainPreparation::BeginInstallationRetirement(const S::ClientStateApplyBatch &b) noexcept {
  auto &s=*impl_;auto &w=s.Pending(b);if(!s.Thread()||!s.Window()||s.visitors||installation_frame.owner)return false;
  if(auto *i=s.InstalledBatch(b))return s.BeginRetirement(*i);
  return w.batch.get()==&b;
}
bool ClientSceneMainPreparation::InstalledObjectCurrent(const S::ClientStateApplyBatch &b,std::uint32_t n,const void *address)const noexcept {
  return address&&InstalledObject(b,n)==address;
}
void *ClientSceneMainPreparation::InstalledObject(const S::ClientStateApplyBatch &b,std::uint32_t n,std::uint64_t *instance)const noexcept {
  const auto &s=*impl_;const auto *installed=s.InstalledBatch(b);
  if(!installed||installed->retiring||n>=installed->objects.size())return nullptr;
  const auto &obj=installed->objects[n];
  if(instance)*instance=obj.instance;
  if(obj.native)return ClientStagedObjectOwnedAddress(obj.native);
  if(!obj.reuse)return nullptr;
  void *current=nullptr;std::uint64_t use=0;
  if(!s.ResolveInstalled(installed->plan->State().Objects()[n].member.identity,current,use)||
      current!=obj.borrowed||use!=obj.instance)return nullptr;
  return current;
}
bool ClientSceneMainPreparation::ReleaseRetiredObject(const S::ClientStateApplyBatch &b,std::uint32_t n,std::uint64_t instance) noexcept {
  auto &s=*impl_;const auto *world=s.InstalledBatch(b);
  if(!s.Window()||s.visitors||installation_frame.owner||!world||n>=world->objects.size())return false;
  const auto &identity=world->plan->State().Objects()[n].member.identity;
  // Body 也可能暂时持有同一根的构造责任；按实例找唯一实际 owner。
  for(const auto &owner:s.installed){
    const auto index=Impl::ObjectIndex(*owner->plan,identity);
    if(index>=owner->objects.size())continue;
    auto &object=owner->objects[index];
    if(object.native&&object.instance==instance)
      return RetireNative(s,[&]{return ClientStagedObjectRelease(object.native);});
  }
  return false;
}
void *ClientSceneMainPreparation::FindLiveMember(const S::PresentationMember &identity,std::uint64_t &instance)const noexcept {
  const auto &s=*impl_;void *address=nullptr;instance=0;
  return s.Window()&&s.ResolveInstalled(identity,address,instance,true)?address:nullptr;
}
bool ClientSceneMainPreparation::VisitRetirementObjects(const S::ClientStateApplyBatch &b,void *context,RetirementObjectVisitor visit) noexcept {
  auto &s=*impl_;auto &w=s.Pending(b);if(!s.Thread()||!visit||s.visitors||!s.Window())return false;
  const auto *installed=s.InstalledBatch(b);if(installed?!installed->retiring:w.batch.get()!=&b)return false;
  const auto &objects=installed?installed->objects:w.objects;const auto &plan=installed?installed->plan:w.plan;
  if(!plan||objects.size()!=plan->State().Objects().size())return false;
  ++s.visitors;bool ok=true;
  for(std::uint32_t n=0;n<objects.size();++n)if(!visit(context,n,objects[n].native&&Structural(plan->State().Objects()[n].domain))){ok=false;break;}
  --s.visitors;return ok;
}
S::ClientStateNativeStep ClientSceneMainPreparation::ApplyInstallation(const S::ClientStateApplyBatch &b,const void *owner,bool retirement,void *context,InstallationAction action) noexcept {
  auto &s=*impl_;auto &w=s.Pending(b);
  const auto *installed=s.InstalledBatch(b);const bool held=installed?installed->installation==owner:w.installation==owner&&w.batch&&w.batch.get()==&b;
  if(!action||!owner||!held||s.visitors||installation_frame.owner||
      !s.Thread()||!s.Window()||(!retirement&&!s.Current(b)))return Fail("binary_install_scope_lifecycle");
  if(!ClientNativeCpuQueuesEmpty()){
#ifdef ISAACCS_E2E
    if(!retirement)s.memory->ApplyMetrics().Wait(S::StateTargetStreams(b.target.candidate->Target()),S::ClientApplyWait::CpuQueueBorrow);
#endif
    return retirement?Fail("binary_install_retire_cpu_borrow"):Wait("binary_install_cpu_borrow");
  }
  w.inspected_window=0;
  struct Scope {
    Scope(Impl &s,bool retirement,const S::ClientStateApplyBatch &b){installation_frame={&s,retirement,Impl::ScopeCurrent,&b};}
    ~Scope(){installation_frame={};}
  } purpose(s,retirement,b);
  ClientDispatchScope dispatch(retirement?ClientOperation::BinaryInstallRetire:ClientOperation::BinaryInstall,ClientEntry::NativeAtomicInstall);
  if(!dispatch.Ready()||ClientDispatchBefore(ClientEntry::NativeAtomicInstall)!=ClientEntryDecision::CallOriginal)return Fail("binary_install_dispatch");
  const auto result=action(context);ClientDispatchReturned(ClientEntry::NativeAtomicInstall);
  if(!dispatch.Succeeded()||!ClientBinaryInstallationScopeActive(retirement)){
    // 保留原生动作首因，分派不完整仍走同一个失败与撤销路径。
    return {false,result.written,true,result.failed?result.reason:"binary_install_dispatch_incomplete"};
  }
  return result;
}
bool ClientSceneMainPreparation::DrainBatch(const S::ClientStateApplyBatch &b) noexcept {
  auto &s=*impl_;auto &w=s.Pending(b);if(!s.Thread())return false;
  w.inspected_window=0;
  for(auto i=s.installed.begin();i!=s.installed.end();++i)if((*i)->batch.get()==&b){if(!s.DrainInstalled(**i))return false;s.installed.erase(i);return true;}
  return (!w.batch||w.batch.get()==&b)&&s.DrainBatch(w);
}
bool ClientSceneMainPreparation::Drain() noexcept {
  auto &s=*impl_;if(!s.Thread()||s.active||s.job)return false;
  for(auto &w:s.work)if(!s.DrainBatch(w))return false;
  for(std::size_t n=s.installed.size();n>0;--n){if(!s.DrainInstalled(*s.installed[n-1]))return false;s.installed.pop_back();}
  // 实际使用者先退，再按依赖逆序释放；cache 元数据清空不充当原生排空凭据。
  if(!s.resources.empty()&&!RetireNative(s,[&]{
    for(std::size_t i=s.resources.size();i>0;--i)if(!ClientPreparedResourceRelease(s.resources[i-1].native)){s.reason="binary_resource_retirement_pending";return false;}
    return true;}))return false;
  s.resources.clear();s.closing=false;s.fault=false;s.roots={};s.vm=nullptr;if(preparation_owner==&s)preparation_owner=nullptr;
  s.reason="binary_preparation_drained";return true;
}
bool ClientSceneMainPreparation::Drained()const noexcept {const auto &s=*impl_;return s.Thread()&&!s.active&&!s.closing&&!s.work[0].batch&&!s.work[1].batch&&!s.job&&s.resources.empty()&&s.installed.empty()&&!s.visitors;}
const char *ClientSceneMainPreparation::Reason()const noexcept{return impl_->Thread()?impl_->reason:"binary_preparation_wrong_thread";}
} // namespace cs::game
