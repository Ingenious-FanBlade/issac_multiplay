#include "game/client/scene/client_binary_scene.h"
#include "game/mods/mod_api.h"
#include "game/mods/mod_native.h"
#include "game/client/scene/client_atomic_install.h"
#include "game/client/scene/client_scene.h"
#include "game/common/context.h"
#include "game/tests/client_entity_e2e.h"
#include "game/tests/e09_compare.h"
#include "net/live.h"
#include "state/budget.h"
#include "state/events.h"
#include "runtime.h"
#ifdef ISAACCS_E2E
#include "replica/stream_e2e.h"
#endif
#include <algorithm>
#include <chrono>
#include <cstring>
#include <map>
#include <optional>

namespace cs::game {
namespace {
namespace S=state;
using Clock=std::chrono::steady_clock;
constexpr auto kManagerWindow=std::chrono::milliseconds(8);
// 快速路径强制刷新周期（设计 10.6 节条件 6）：每 32 个 World 版本至少一次不吸收的完整事务。
constexpr std::uint32_t kFastPeriod=32;
S::ClientStateNativeStep Ready(const char *why,bool written=false) noexcept {return {true,written,false,why};}
S::ClientStateNativeStep Failed(const char *why,bool written=false) noexcept {return {false,written,true,why};}
// 后台仅使用封存计划和表中的资源结果，实际原生资格由 Manager 重新检查。
class Readiness final:public ClientSceneInstallReadiness {
public:
  bool Available()const noexcept override {return true;}
  S::StateReadiness Inspect(const ClientSceneInstallPlan &scene,const S::ClientStateTables &tables)const override {
    const auto &plan=scene.State();
    for(const auto &need:plan.Resources()){
      switch(tables.ResourceState(need.resources->Directory().epoch,need.key)){
        case S::ClientResourceState::Ready:break;
        case S::ClientResourceState::Failed:
        case S::ClientResourceState::MissingLocalResource:return S::StateReadiness::MissingLocalResource;
        default:return S::StateReadiness::ResourcePending;
      }
    }
    return S::StateReadiness::Ready;
  }
};
}
struct ClientBinaryScene::Impl final:S::ClientStateNativeOwner,S::ClientStateApplyResults {
  struct Batch final {
    S::ClientStateApplyBatchRef ticket;
    std::unique_ptr<ClientSceneAtomicInstaller> writer;
    bool display_entered=false,native_published=false,reconciled=false;
    // 快速帧（设计 10.6 节）：fast 表示本票据走整批快速帧，不经准备暂存、也不建安装器；
    // fast_checked 表示本票据已判定过资格，避免准备等待时重复判定。
    bool fast=false,fast_checked=false;
    // 已提交的快速票据：根与承载仍属 ticket 的安装器，当前计划与发布目标属于 current。
    S::ClientStateApplyBatchRef current;
    std::uint8_t recovery_streams=0;
    std::uint64_t write_us=0,apply_window=0,prepare_us=0;
    std::shared_ptr<S::ClientStateBlockTable> publication;
    std::optional<S::ClientStateVector<ClientMorphedMember>> morphed;
    bool Drain(bool final_scene=false) noexcept {
      if(!ticket)return true;
      if(writer&&!writer->Drain(*ticket,final_scene))return false;
      *this={};return true;
    }
    const ClientSceneInstallPlan &Plan()const noexcept {
      return static_cast<const ClientSceneInstallPlan &>(*(current?current:ticket)->native);
    }
  };
  ClientScene &scene;
  net::live::Client &transport;
  std::shared_ptr<S::ClientStateMemory> memory;
  ClientSceneMainPreparation preparation;
  std::array<S::ClientStateApplyCoordinator,2> channels;
  std::shared_ptr<ClientSceneAtomicInstaller::RootScalars> root_scalars;
  S::StateRoute route{};
  std::uint32_t mod_version=0;
  bool preserve_mod=false;
  // 已经公布给 Lua 的活成员。只保存目录身份和实例号，通知前由准备 owner 再解析。
  struct ModLiveMember {
    S::PresentationMember identity;
    std::uint64_t instance=0;
    bool notified=false,released=false;
    const void *address=nullptr; // 仅 UI 构建意图时与同一发布的对象地址比较。
  };
  using ModLiveRow=std::pair<const std::uint16_t,ModLiveMember>;
  using ModLiveMembers=std::map<std::uint16_t,ModLiveMember,std::less<>,S::ClientStateAllocator<ModLiveRow>>;
  ModLiveMembers mod_members{std::less<>{},S::ClientStateAllocator<ModLiveRow>(memory)};
  std::array<Batch,2> pending;
  Batch world,body,retired_world,retired_body;
  // 原生租约和纯字段索引分别保管。只随同一流的成功发布切换，不借已退休批次的地址。
  std::array<ClientSceneFieldBinding,2> published_fields;
  // 只保管邮箱交来的原票据，每通道最多一张；不增加接收候选或 Body 等待位。
  std::array<S::ClientStateApplyBatchRef,2> incoming;
  std::array<std::size_t,4> readback_cursor{};
  // 只存上一份已应用帧中失败的成员。成功、离开或换形态都会结束连续计数。
  struct FailureRun {ClientMemberFailure kind;std::uint64_t count=0,frame=0;};
  using FailureEntry=std::pair<const S::PresentationMember,FailureRun>;
  using FailureRuns=std::map<S::PresentationMember,FailureRun,std::less<>,S::ClientStateAllocator<FailureEntry>>;
  std::array<FailureRuns,2> failure_runs;
  std::array<std::uint64_t,2> failure_frames{};
  bool opened=false,closing=false,resetting=false,display_detached=false,preserve_room=false,preserve_requested=false,pumping=false;
  // 外观事件（SFX／横幅／fortune）已实际回放的最高序号及其路由。
  // 目标在前一批仍在安装时就已备好事件清单，同一事件会出现在相邻两三个 World 目标里；
  // 带世界的目标全部走通道0串行安装，序号单调，按此水位只回放一次，其余仍按已回放回执上报。
  S::StateRoute replay_route{};
  std::uint64_t replayed_cosmetic=0;
  // 成员写程序表（设计 10.6 节），跨 World 安装器共享；撤显、恢复、换路由时清空。
  std::shared_ptr<ClientSceneMemberStore> store;
  // 自上次不吸收的完整事务以来发布的 World 版本数。
  std::uint32_t since_full=0;
  // 快速帧未命中原因计数（原因是静态字符串，按内容合并）。只在诊断页 0 输出。
  std::array<std::pair<const char *,std::uint64_t>,32> fast_misses{};
  void FastMiss(const char *why) noexcept {
    if(!why)why="binary_fast_unspecified";
    memory->ApplyMetrics().Volume(1,S::ClientApplyVolume::FastMisses);
    for(auto &entry:fast_misses){
      if(!entry.first){entry={why,1};return;}
      if(!std::strcmp(entry.first,why)){++entry.second;return;}
    }
  }
  // 撤显、恢复或换路由后，写程序表与强制周期从头开始。
  void ForgetFastState() noexcept {
    if(store)ClientClearMemberStore(*store);
    since_full=0;
  }
  std::optional<S::StateResyncReason> request;
#ifdef ISAACCS_E2E
  // 只在实际撤显入口累计，和正常停止、AwaitingWorld 会话枚举区分。
  enum class ResetCause : unsigned {NativeFailure,HostBaseline,LocalRecovery};
  std::array<std::uint64_t,3> resets{};
  std::uint64_t baselines_kept_display=0;
#endif
  std::array<ClientDisplaySound,512> sounds{};
  std::array<void *,512> event_players{},event_resources{};
  // 两槽都可暂存，但这些回放绑定只在唯一的 World 写段使用。
  // 预测插值阶段 1：World 发布后把玩家身体地址与控制组序号交给覆盖层，用于识别本机身体。
  // 计划对象下标与安装器对象表一一对应（同 BindDisplay 的事件玩家绑定）。
  void PublishMotionPlayers(ClientSceneAtomicInstaller &writer) noexcept {
    const auto visit=[](void *p,const ClientAtomicInputs &in)noexcept{
      auto &self=*static_cast<Impl *>(p);
      const auto &plan=in.scene.State();
      std::array<ClientMotionPlayer,64> out{};unsigned n=0;
      for(const auto &player:plan.Players()){
        if(n>=out.size())break;
        if(player.object>=in.objects.size()||!in.objects[player.object].address)continue;
        const auto groups=plan.Groups();
        const auto group=std::find_if(groups.begin(),groups.end(),[&](const auto &g){return g.object==player.group;});
        if(group==groups.end())continue;
        // 阶段 2：带上影子表身份，覆盖层据此在已发布 Body 表中找该身体的对账字段（设计 5.7）。
        const auto &id=plan.Objects()[player.object].member.identity;
        out[n++]={in.objects[player.object].address,group->ordinal,{id.space,id.layout,id.id,id.use,id.shape}};
      }
      self.scene.SetMotionPlayers(out.data(),n);
      return true;
    };
    if(!writer.VisitPublished(this,visit))scene.SetMotionPlayers(nullptr,0);
  }
  bool BindDisplay(const ClientAtomicInputs &inputs) noexcept {
    const auto values=inputs.scene.Sounds();
    const auto sound_links=inputs.scene.SoundLinks();
    const auto links=inputs.scene.EventLinks();
    if(values.size()>sounds.size()||links.size()>event_players.size())return false;
    std::copy(values.begin(),values.end(),sounds.begin());
    const auto resource=[&](S::ResourceKey key)->void * {
      const auto at=std::lower_bound(inputs.resources.begin(),inputs.resources.end(),key,[](const auto &r,auto k){return r.key<k;});
      return at!=inputs.resources.end()&&at->key==key?at->address:nullptr;
    };
    for(std::size_t i=0;i<sound_links.size();++i){
      sounds[i].asset=resource(sound_links[i].resource);
      if(!ClientDisplaySoundReady(sounds[i]))return false;
    }
    for(std::size_t i=0;i<links.size();++i){
      event_players[i]=links[i].player<inputs.objects.size()?inputs.objects[links[i].player].address:nullptr;
      event_resources[i]=resource(links[i].resource);
      if(links[i].resource.index!=S::kNullIndex&&!event_resources[i])return false;
      auto event=inputs.scene.Events()[i];
      if(event.kind==unsigned(S::StateEventKind::Sfx)){
        event.sound.asset=event_resources[i];
        if(!ClientDisplaySoundReady(event.sound))return false;
      }
    }
    return true;
  }
  Impl(ClientScene &s,net::live::Client &t,std::shared_ptr<S::ClientStateMemory> m):
    scene(s),transport(t),memory(std::move(m)),preparation(memory),
    channels{S::ClientStateApplyCoordinator(*this,*this),S::ClientStateApplyCoordinator(*this,*this)},
    root_scalars(ClientBinaryRootScalars(memory)),store(ClientMakeMemberStore(memory)),
    failure_runs{FailureRuns(S::ClientStateAllocator<FailureEntry>(memory)),
      FailureRuns(S::ClientStateAllocator<FailureEntry>(memory))} {}
  Batch &Pending(const S::ClientStateApplyBatch &b) noexcept {
    return pending[S::StateApplyChannel(b.target.candidate->Target())];
  }
  const Batch &Pending(const S::ClientStateApplyBatch &b)const noexcept {
    return pending[S::StateApplyChannel(b.target.candidate->Target())];
  }
  bool Current(const S::ClientStateApplyBatch &b)const noexcept override {
    return !closing&&!resetting&&Pending(b).ticket.get()==&b&&opened&&preparation.Current(b);
  }
  void StartApplicationWindow(Batch &slot) noexcept {
    const auto window=OwnerToken(Owner::Manager);
    if(slot.apply_window==window)return;
    // 应用耗时包含本帧删除、构造与字段接合，不计等待前一帧借用的时间。
    slot.apply_window=window;slot.write_us=S::ClientApplyMetrics::Now();
  }
  S::ClientStateNativeStep Prepare(const S::ClientStateApplyBatch &b) noexcept override {
    auto &pending=Pending(b);
    StartApplicationWindow(pending);
    const auto &target=b.target.candidate->Target();
    const auto streams=S::StateTargetStreams(target);
    if(!pending.prepare_us){
      pending.prepare_us=S::ClientApplyMetrics::Now();
      if(b.admitted_us)memory->ApplyMetrics().Mark(streams,S::ClientApplyStage::MainQueue,pending.prepare_us-b.admitted_us);
    }
    if(!pending.fast_checked){
      pending.fast_checked=true;
      if(const auto *why=FastEligible(b))FastMiss(why);
      else pending.fast=true;
    }
    if(pending.fast)return Ready("binary_fast_prepared");
    S::ClientApplyTimer time(memory->ApplyMetrics(),streams,S::ClientApplyStage::MainPrepare);
    return preparation.Prepare(pending.ticket);
  }
  // 整批快速帧资格（设计 10.6 节条件 1—6）。只读：返回首个不满足的原因，满足时返回 nullptr。
  const char *FastEligible(const S::ClientStateApplyBatch &b) noexcept {
    S::ClientApplyTimer time(memory->ApplyMetrics(),1,S::ClientApplyStage::FastCheck);
    if(S::StateTargetStreams(b.target.candidate->Target())!=1)return "binary_fast_not_world";
    if(since_full+1>=kFastPeriod)return "binary_fast_forced_refresh";
    if(!world.writer||!world.ticket)return "binary_fast_no_world_writer";
    if(world.ticket->resolution.load(std::memory_order_acquire)!=S::ClientStateApplyResolution::Committed)
      return "binary_fast_world_not_committed";
    if(retired_world.ticket)return "binary_fast_retirement_pending";
    for(const auto &slot:pending)if(slot.ticket.get()!=&b&&slot.native_published&&(S::StateTargetStreams(
        slot.ticket->target.candidate->Target())&1))return "binary_fast_world_publication_pending";
    if(!scene.DisplayReady())return "binary_fast_display_not_ready";
    const auto live=scene.PublishedBlocks();
    if(!live||!live->streams[0]||live->streams[0]->Context().target_version!=b.application_bases[0])
      return "binary_fast_live_base";
    if(world.Plan().State().Effective(S::BodyStream::World)!=live->streams[0])return "binary_fast_live_plan";
    return world.writer->FastCheck(b);
  }
  bool BeginWrite(const S::ClientStateApplyBatch &b) const noexcept {
    auto expected=S::ClientStateWriteState::Preparing;
    return (b.write_state.compare_exchange_strong(expected,S::ClientStateWriteState::Writing,
        std::memory_order_acq_rel,std::memory_order_acquire)||expected==S::ClientStateWriteState::Writing)&&Current(b);
  }
  void ModRemoved(ModLiveMember &member) noexcept {
    if(member.notified||member.released)return;
    std::uint64_t instance=0;
    auto *address=preparation.FindLiveMember(member.identity,instance);
    if(address&&instance==member.instance){member.notified=true;NotifyModRemoved(address);}
  }
  void ModReleased(std::span<const ClientLiveRetirement> leaving,bool include_unpublished=false) noexcept {
    if(!ModWantsEntityLifecycle())return;
    for(const auto &entry:leaving){
      if(entry.identity.space!=1)continue;
      const auto at=std::find_if(mod_members.begin(),mod_members.end(),[&](const auto &live){
        return live.second.identity==entry.identity&&live.second.instance==entry.instance;
      });
      if(at==mod_members.end()){if(include_unpublished)CountUnplannedModRemoval();continue;}
      if(at->second.released)continue;
      if(!at->second.notified)CountUnplannedModRemoval();
      at->second.released=true;
    }
  }
  S::ClientStateNativeStep Reconcile(const S::ClientStateApplyBatch &b) noexcept {
    auto &slot=Pending(b);
    if(slot.reconciled||!slot.Plan().Display()||!world.writer)return Ready("binary_live_no_structure_prefix");
    // 池槽只在所有旧字段用途均解除后归还。仍在收尾的历史批次先走原退休入口。
    if(retired_world.ticket||retired_body.ticket)
      return {false,false,false,"binary_live_structure_waiting_for_retirement"};
    try {
      S::ClientStateVector<ClientLiveRetirement> leaving{S::ClientStateAllocator<ClientLiveRetirement>(memory)};
      if(!world.writer->SelectRetiring(b,leaving))return Failed("binary_live_structure_identity");
      if(leaving.empty()){slot.reconciled=true;return Ready("binary_live_structure_current");}
      const auto morph_targets=std::count_if(leaving.begin(),leaving.end(),[](const auto &entry){
        return entry.morph_target!=S::kClientPlanNone;
      });
      if(morph_targets){
        slot.morphed.emplace(S::ClientStateAllocator<ClientMorphedMember>(memory));
        slot.morphed->reserve(morph_targets);
      }
      const auto publication=ReservePublication(b);
      if(!publication.ready)return publication;
      if(!BeginWrite(b))return Failed("binary_write_ticket_withdrawn");
      for(const auto &planned:slot.Plan().ModLeaving()){
        const auto at=mod_members.find(planned.index);
        if(at==mod_members.end()||at->second.identity!=planned.identity)continue;
        if(std::any_of(leaving.begin(),leaving.end(),[&](const auto &entry){
            return entry.identity==planned.identity&&entry.instance==at->second.instance;
          }))ModRemoved(at->second);
      }
      if(!Current(b))return Failed("binary_mod_removal_revoked");
      // 旧 surface 必须在删除第一名旧成员之前捕获。资源／事件句柄在实际安装前再绑定。
      const auto display=scene.PrepareBinaryDisplay(slot.Plan().Display());
      if(!display.done)return Failed(display.reason,display.written);
      slot.display_entered=true;
      if(!Current(b))return Failed("binary_live_structure_revoked",display.written);
      // 已经写入、正在等工作线程回执的 Body 同样可能持有原生附件。
      for(const auto &peer:pending)if(peer.ticket.get()!=&b&&peer.native_published&&
          peer.writer&&!peer.writer->UnbindRetiring(leaving))
        return Failed("binary_live_structure_unbind_pending_body",true);
      if((body.writer&&!body.writer->UnbindRetiring(leaving))||
          !world.writer->UnbindRetiring(leaving)||!world.writer->MorphMembers(b,leaving))
        return Failed("binary_live_structure_morph",true);
      for(const auto &entry:leaving)if(entry.morphed)
        slot.morphed->push_back({entry.identity,b.target.install->Objects()[entry.morph_target].member.identity,
          entry.address,entry.instance});
      std::erase_if(leaving,[](const auto &entry){return entry.morphed;});
      if(!world.writer->ReclaimRetiring(leaving))
        return Failed("binary_live_structure_reclaim",true);
      ModReleased(leaving);
      slot.reconciled=true;
      return Ready("binary_live_structure_reclaimed",true);
    }catch(...){return Failed("binary_live_structure_budget");}
  }
  S::ClientStateNativeStep Stage(const S::ClientStateApplyBatch &b) noexcept override {
    auto &pending=Pending(b);
    StartApplicationWindow(pending);
    const auto streams=S::StateTargetStreams(b.target.candidate->Target());
    if(pending.fast)return Ready("binary_fast_staged");
    if(!pending.writer)try{
      const std::array source{published_fields[0].index,published_fields[1].index};
      // World 事务登记写程序；到强制刷新周期时不吸收（设计 10.6 节二期）。
      const bool world_stream=(streams&1)!=0;
      pending.writer=std::make_unique<ClientSceneAtomicInstaller>(memory,preparation,root_scalars,source,
        world_stream?store:nullptr,world_stream&&since_full+1<kFastPeriod);
    }
    catch(...){return Failed("binary_install_owner_allocation");}
    S::ClientApplyTimer time(memory->ApplyMetrics(),streams,S::ClientApplyStage::MainPrepare);
    const auto prefix=Reconcile(b);
    if(!prefix.ready||prefix.failed)return prefix;
    auto step=pending.writer->Stage(pending.ticket);
    step.written|=prefix.written;
    return step;
  }
  const Batch &PublishedBatch(S::BodyStream stream)const noexcept {
    for(const auto &slot:pending)if(slot.native_published&&slot.ticket->target.candidate->State(stream))return slot;
    return stream==S::BodyStream::Body&&body.ticket?body:world;
  }
  // 对侧只作为窄结构依赖。与原计划的共享身份／目录一致时可借最新承载，
  // 这不会把对侧未接受的版本当成下一次网络 base。
  bool CompatiblePeer(const S::ClientStateInstallPlan &plan,unsigned own,const S::ClientStateBlockTable &live)const noexcept {
    if(own==1){
      // 原生准备对每个身体复核 World 的服装目录，缺少兼容承载只延后那个身体。
      return live.streams[0]&&plan.Effective(S::BodyStream::Body);
    }else{
      const auto &peer=PublishedBatch(S::BodyStream::Body);
      if(!peer.ticket||!live.streams[1])return false;
      const auto &body=peer.Plan().State();
      if(body.Effective(S::BodyStream::Body)!=live.streams[1])return false;
      // World 的成员可以独立推进，Body 的应用按已知对象选取。
    }
    return true;
  }
  S::ClientStateNativeStep ReservePublication(const S::ClientStateApplyBatch &b) noexcept {
    auto &pending=Pending(b);
    const auto &plan=pending.Plan().State();const auto &target=plan.Target();
    const auto mask=S::StateTargetStreams(target);
    const auto live=scene.PublishedBlocks();
    if(live&&live->route!=target.route)return Failed("binary_publication_route_changed");
    for(unsigned i=0;i<target.count;++i){
      const auto &section=target.sections[i];
      if(section.base_version&&(!live||!live->streams[unsigned(section.stream)]||
          live->streams[unsigned(section.stream)]->Context().target_version!=b.application_bases[unsigned(section.stream)]))
        return Failed("binary_publication_base_changed");
    }
    if(mask!=3&&(!live||!CompatiblePeer(plan,mask==2?1:0,*live)))
      return Failed("binary_publication_peer_structure_changed");
    try{
      // 表元数据在第一项原生写入之前分配。只替换票据的流，其余引用来自此刻的 current。
      auto table=std::allocate_shared<S::ClientStateBlockTable>(S::ClientStateAllocator<S::ClientStateBlockTable>(memory));
      table->route=target.route;
      if(live)table->streams=live->streams;
      for(unsigned i=0;i<2;++i)if(mask&(1u<<i))table->streams[i]=plan.Effective(S::BodyStream(i));
      if(!table->streams[0]||!table->streams[1])return Failed("binary_publication_incomplete_table");
      pending.publication=std::move(table);
      return Ready("binary_publication_reserved");
    }catch(...){return Failed("binary_publication_budget");}
  }
  // 原生安装与显示写段在同一个 Manager 窗口连续推进。
  // 快速帧写段：保留发布表、取写票据、进入显示准备，然后由当前 World 安装器全量写纯值叶并换计划。
  S::ClientStateNativeStep FastInstall(Batch &pending,const S::ClientStateApplyBatch &b) noexcept {
    if(!world.writer)return Failed("binary_fast_writer_missing");
    if(!pending.publication){
      const auto publication=ReservePublication(b);
      if(!publication.ready)return publication;
    }
    if(!BeginWrite(b))return Failed("binary_write_ticket_withdrawn");
    bool written=false;
    const auto &display=pending.Plan().Display();
    if(!pending.display_entered){
      const auto step=scene.PrepareBinaryDisplay(display);
      if(!step.done)return Failed(step.reason,step.written);
      pending.display_entered=true;written=step.written;
    }
    const auto step=world.writer->FastInstall(pending.ticket);
    if(!step.ready||step.failed){
      if(!written&&!step.written)scene.AbortBinaryDisplay();
      return Failed(step.reason,written||step.written);
    }
    // 资源与事件绑定按换过的新计划取：对象和资源仍是当前安装器的。
    const auto bind=[](void *p,const ClientAtomicInputs &in)noexcept{return static_cast<Impl *>(p)->BindDisplay(in);};
    if(!world.writer->VisitPublished(this,bind))return Failed("binary_fast_display_binding",true);
    scene.SuspendBinaryDisplay();
    return Ready(step.reason,true);
  }
  S::ClientStateNativeStep Install(const S::ClientStateApplyBatch &b) noexcept override {
    auto &pending=Pending(b);
    if(!scene.DestructiveReady())return Failed("binary_install_native_borrow");
    if(pending.fast)return FastInstall(pending,b);
    if(!pending.publication){
      const auto publication=ReservePublication(b);
      if(!publication.ready)return publication;
    }
#ifdef ISAACCS_E2E
    // 验收在真正写票据取得前拒绝 World，沿正式写前失败路径归还候选。
    const auto &target=b.target.candidate->Target();
    const bool world=(S::StateTargetStreams(target)&1)!=0;
    if(world&&ClientEntityTestFault("binary-world-before-install"))return Failed("binary_e2e_world_before_install");
#endif
    if(!BeginWrite(b))return Failed("binary_write_ticket_withdrawn");
    bool written=false;
    if(const auto &display=pending.Plan().Display()){
      const auto bind=[](void *p,const ClientAtomicInputs &in)noexcept{return static_cast<Impl *>(p)->BindDisplay(in);};
      if(!pending.writer->VisitStaged(b,this,bind))return Failed("binary_display_resource_or_event_binding");
      if(!pending.display_entered){
        const auto step=scene.PrepareBinaryDisplay(display);
        if(!step.done)return Failed(step.reason,step.written);
        pending.display_entered=true;written=step.written;
      }
      if(written){
        // 旧房离屏捕获可能执行原生回调；重新核定捕获前借到的对象和资源。
        preparation.RevokeStaged(b,"after_old_room_capture");
        if(!pending.writer->VisitStaged(b,this,bind))return Failed("binary_display_binding_changed_after_capture",true);
      }
    }
    const auto step=pending.writer->Install(b);
    if(!step.ready){
      if(!written&&!step.written)scene.AbortBinaryDisplay();
      return Failed(step.reason,written||step.written);
    }
    if(!RemoveFailedMembers(pending,written))return Failed("binary_failed_member_retirement",true);
#ifdef ISAACCS_E2E
    // 等到 World 已产生原生写入再注入。正式协调器负责撤显、排空并请求完整恢复。
    if(world&&step.written){
      const bool joint=target.count==2&&ClientEntityTestFault("binary-joint-after-install");
      if(joint||ClientEntityTestFault("binary-world-after-install")){
        Trace("client_error","event=binary_e2e_partial_target route=%llu serial=%llu count=%u world=%u body=%u",
          b.ticket.route.route,b.ticket.serial,unsigned(target.count),target.sections[0].target_version,
          target.count==2?target.sections[1].target_version:0);
        return Failed("binary_e2e_world_after_install",true);
      }
    }
#endif
    // 世界根已改写；候选通过专用离屏作用域绘制，普通入口等同批Publish。
    if(pending.Plan().Display())scene.SuspendBinaryDisplay();
    return Ready(step.reason,written||step.written);
  }
  S::ClientStateNativeStep Display(const S::ClientStateApplyBatch &b) noexcept override {
    auto &pending=Pending(b);
    const auto &display=pending.Plan().Display();
    if(pending.fast){
      const auto step=scene.InstallBinaryDisplay(display);
      if(!step.done)return Failed(step.reason,true);
      // 资格已排除房间与构图注入变化；仍发生重建说明判定漏了条件，具名显示字段没有重放，只能整批失败。
      if(step.rebuilt)return Failed("binary_fast_display_rebuilt",true);
      const auto fields=world.writer->FastDisplay(b);
      if(!fields.ready||fields.failed)return Failed(fields.reason,true);
      const auto capture=scene.CaptureBinaryDisplay(display);
      if(!capture.done)return Failed(capture.reason,true);
      const auto boss=scene.RestoreBinaryBoss(pending.Plan().Boss());
      if(!boss.done)return Failed(boss.reason,true);
      const auto audio=scene.InstallBinaryAudio(pending.Plan().Music(),{sounds.data(),pending.Plan().Sounds().size()});
      return audio.done?Ready(audio.reason,true):Failed(audio.reason,true);
    }
    if(!display){
      auto fields=pending.writer->InstallDisplayFields(b);
      if(fields.ready&&!RemoveFailedMembers(pending,fields.written))return Failed("binary_failed_member_retirement",true);
      return fields;
    }
    const auto step=scene.InstallBinaryDisplay(display);
    if(!step.done)return Failed(step.reason,step.written);
    auto fields=pending.writer->InstallDisplayFields(b,step.rebuilt);
    if(!fields.ready)return Failed(fields.reason,true);
    if(!RemoveFailedMembers(pending,fields.written))return Failed("binary_failed_member_display_retirement",true);
    const auto capture=scene.CaptureBinaryDisplay(display);
    if(!capture.done)return Failed(capture.reason,true);
    const auto boss=scene.RestoreBinaryBoss(pending.Plan().Boss());
    if(!boss.done)return Failed(boss.reason,true);
    const auto audio=scene.InstallBinaryAudio(pending.Plan().Music(),{sounds.data(),pending.Plan().Sounds().size()});
    return audio.done?Ready(audio.reason,true):Failed(audio.reason,true);
  }
  S::ClientStateNativeStep Replay(const S::ClientStateApplyBatch &b,std::size_t index) noexcept override {
    auto &pending=Pending(b);
    if(index>=pending.Plan().Events().size()||index>=b.target.events.size())return Failed("binary_display_event_index");
    // 只对会出声／出字的外观事件去重；转场与 Boss 事件仍每次按当前世界核对，行为不变。
    const auto sequence=b.target.events[index].value.sequence;
    const bool cosmetic=S::StateEventCosmetic(S::StateEventKind(b.target.events[index].value.purpose));
    if(replay_route!=b.ticket.route){replay_route=b.ticket.route;replayed_cosmetic=0;}
    if(cosmetic&&sequence<=replayed_cosmetic)return Ready("binary_event_already_replayed");
    auto event=pending.Plan().Events()[index];
    event.player=event_players[index];event.configuration=event_resources[index];event.sound.asset=event.configuration;
    if(event.kind==unsigned(S::StateEventKind::ItemBanner)&&
        pending.Plan().EventLinks()[index].player!=S::kClientPlanNone&&!event.player){
      // 原生多人 HUD 在缺席玩家上返回无效槽号。只消费这一条已无显示目标的外观事件。
      if(cosmetic)replayed_cosmetic=sequence;
      return Ready("binary_item_banner_owner_missing");
    }
    if(event.kind==unsigned(S::StateEventKind::Cutscene)){
      QueueClientModCutscene({event.cutscene_serial,event.cutscene_id,event.cutscene_clear});
      return Ready("binary_cutscene_queued");
    }
    const auto step=scene.ReplayBinaryEvent(event);
    // 只在原生回放成功后推进水位；失败的事件留给后续目标重试。
    if(step.done&&cosmetic)replayed_cosmetic=sequence;
    return step.done?Ready(step.reason,step.written):Failed(step.reason,step.written);
  }
  S::ClientStateNativeStep Publish(const S::ClientStateApplyBatch &b) noexcept override {
    auto &pending=Pending(b);
    if(!pending.publication||pending.publication->route!=b.ticket.route)return Failed("binary_publication_block_table",true);
    if(pending.Plan().Display()&&!scene.BinaryDisplayComplete())return Failed("binary_display_publication_incomplete",true);
    if(pending.fast){
      if(!world.writer)return Failed("binary_fast_writer_missing",true);
      world.writer->FastPublished(b);
      published_fields[0]=world.writer->PublishedFields();
      scene.PublishBinary(std::move(pending.publication),1);
      PublishMotionPlayers(*world.writer);
#ifdef ISAACCS_E2E
      if(const auto table=scene.PublishedBlocks())E09Client(*table,preparation,1);
#endif
      ++since_full;
      auto &metrics=memory->ApplyMetrics();const auto now=S::ClientApplyMetrics::Now();
      metrics.Mark(1,S::ClientApplyStage::Install,now-pending.write_us);
      metrics.Mark(1,S::ClientApplyStage::Publish);
      if(b.received_us)metrics.Mark(1,S::ClientApplyStage::EndToEnd,now-b.received_us);
      metrics.Volume(1,S::ClientApplyVolume::FastHits);
      pending.native_published=true;
      for(const auto &other:this->pending)if(other.ticket&&other.ticket.get()!=&b)
        preparation.RevokeStaged(*other.ticket,"after_peer_publication");
      return Ready("binary_fast_published",true);
    }
    const auto step=pending.writer->Publish(b);
    if(step.ready){
      const auto mask=S::StateTargetStreams(b.target.candidate->Target());
      // 不吸收的 World 事务结束一个强制刷新周期。
      if(mask&1)since_full=pending.writer->Absorbing()?since_full+1:0;
      RecordObjectFailures(pending,mask);
      const auto fields=pending.writer->PublishedFields();
      for(unsigned i=0;i<2;++i)if(mask&(1u<<i))published_fields[i]=fields;
      scene.PublishBinary(std::move(pending.publication),mask);
      if(mask&1)PublishMotionPlayers(*pending.writer);
      if(mask==1){
        // World 建出 Body 先前缺少的目标时，直接补挂活实例上的有效边。
        const auto &pose=PublishedBatch(S::BodyStream::Body);
        if(pose.writer)pose.writer->ResolveDeferredBodyReferences(pending.morphed?
          std::span<const ClientMorphedMember>{*pending.morphed}:std::span<const ClientMorphedMember>{});
      }
      pending.morphed.reset();
#ifdef ISAACCS_E2E
      if(const auto table=scene.PublishedBlocks())E09Client(*table,preparation,mask);
#endif
      auto &metrics=memory->ApplyMetrics();const auto now=S::ClientApplyMetrics::Now();
      metrics.Mark(mask,S::ClientApplyStage::Install,now-pending.write_us);
      metrics.Mark(mask,S::ClientApplyStage::Publish);
      if(b.received_us)metrics.Mark(mask,S::ClientApplyStage::EndToEnd,now-b.received_us);
      pending.native_published=true;
      // 另一流可能在同一 Manager 边界已有地址见证。发布后的复查必须看到当前承载。
      for(const auto &other:this->pending)if(other.ticket&&other.ticket.get()!=&b)
        preparation.RevokeStaged(*other.ticket,"after_peer_publication");
    }
    return step;
  }
  // 保留当前复制路线与资源目录，停显后由 Manager 归还旧场景的原生借用和对象槽。
  void ResetInstalledScene() noexcept {
    preserve_mod=false;mod_version=0;
    scene.SuspendBinaryDisplay();published_fields={};ForgetFastState();
    resetting=true;
    // 换路由取消在途写入时，旧画面已单独接管，不能随被撤销的写票一起丢掉。
    if(!closing)preserve_room=preserve_requested=false;
    for(auto &channel:channels)channel.Cancel();
  }
  void Invalidate(const S::ClientStateApplyBatch &b,const char *why) noexcept override {
    auto &pending=Pending(b);
    if(pending.ticket.get()!=&b)return;
    // 部分写入会立即撤销显示，保留首个失败原因以便断线前后都可定位。
    Trace("client_error","event=binary_native_invalidate route=%llu serial=%llu reason=%s",
      b.ticket.route.route,b.ticket.serial,why);
    memory->ApplyMetrics().FirstIssue(S::StateTargetStreams(b.target.candidate->Target()),{b.ticket.route.route,b.ticket.serial},
      "invalidate",why);
#ifdef ISAACCS_E2E
    ++resets[unsigned(ResetCause::NativeFailure)];
#endif
    ResetInstalledScene();
    if(pending.writer)pending.writer->Invalidate(b,why);
  }
  bool Complete(const S::ClientStateApplyCompletion &done) noexcept override {
    auto receipt=done;
    if(done.applied)
      for(const auto &slot:pending)if(slot.ticket&&slot.ticket->ticket.serial==done.ticket.serial)
        receipt.recovery_streams=slot.recovery_streams;
    if(!transport.CompleteBinary(receipt))return false;
    if(!done.applied&&done.drained&&!closing&&!resetting)
      for(unsigned i=0;i<2;++i)if(pending[i].ticket&&pending[i].ticket->ticket.serial==done.ticket.serial){
        const auto &b=*pending[i].ticket;const auto mask=S::StateTargetStreams(b.target.candidate->Target());
        if(b.Withdrawn()||b.cancelled.load(std::memory_order_acquire))memory->ApplyMetrics().Wait(mask,S::ClientApplyWait::Cancelled);
        else memory->ApplyMetrics().FirstIssue(mask,{done.ticket.route.route,done.ticket.serial},"prewrite",channels[i].Reason());
        Trace("client_error","event=binary_native_rejected route=%llu serial=%llu reason=%s",done.ticket.route.route,done.ticket.serial,channels[i].Reason());
      }
    return true;
  }
  bool RemoveFailedMembers(Batch &current,bool &written) noexcept {
    try {
      S::ClientStateVector<ClientLiveRetirement> leaving{S::ClientStateAllocator<ClientLiveRetirement>(memory)};
      current.writer->FailedMembers(leaving);
      if(leaving.empty())return true;
      written=true;
      // 先撤最新附件，再从其他发布批次拆边，最后才允许释放原生实例。
      if(!current.writer->UnbindRetiring(leaving))return false;
      const auto unbind=[&](const Batch &batch){
        return &batch==&current||!batch.writer||
          (batch.writer->UnbindRetiring(leaving)&&batch.writer->ForgetRetired(leaving));
      };
      for(const auto &batch:pending)if(batch.native_published&&!unbind(batch))return false;
      if(!unbind(body)||!unbind(world)||!unbind(retired_body)||!unbind(retired_world))return false;
      if(!current.writer->ReclaimFailed(leaving))return false;
      ModReleased(leaving,true);
      if(current.Plan().Display())for(const auto &entry:leaving)
        std::replace(event_players.begin(),event_players.end(),entry.address,static_cast<void *>(nullptr));
      return true;
    }catch(...){return false;}
  }
  void RecordObjectFailures(Batch &batch,std::uint8_t streams) noexcept {
    for(unsigned i=0;i<2;++i)if(streams&(1u<<i)){
      auto &runs=failure_runs[i];const auto frame=++failure_frames[i];
      struct Update {FailureRuns &runs;std::uint64_t frame;bool recover=false;} update{runs,frame};
      try {
        batch.writer->VisitFailures(&update,[](void *context,const S::PresentationMember &id,ClientMemberFailure kind){
          if(kind!=ClientMemberFailure::Native&&kind!=ClientMemberFailure::Pool)return;
          auto &update=*static_cast<Update *>(context);
          const auto [at,inserted]=update.runs.try_emplace(id,FailureRun{kind});
          auto &run=at->second;
          if(run.kind!=kind)run={kind};
          run.frame=update.frame;
          const auto limit=kind==ClientMemberFailure::Native?S::StateBudgets().native_object_failure_limit:
            S::StateBudgets().pool_object_missing_limit;
          if(run.count<limit&&++run.count==limit)update.recover=true;
        });
      }catch(...){
        // 计数失去有界存储时也保留已应用画面，由接收端请求完整基线。
        update.recover=true;
      }
      std::erase_if(runs,[&](const auto &entry){return entry.second.frame!=frame;});
      if(update.recover)batch.recovery_streams|=std::uint8_t(1u<<i);
    }
  }
  bool DetachDisplay() noexcept {
    if(display_detached)return true;
    if(!scene.RetireBinaryDisplay(preserve_room))return false;
    display_detached=true;return true;
  }
  bool RetireInstalled() noexcept {
    return retired_body.Drain()&&retired_world.Drain()&&body.Drain()&&world.Drain(true);
  }
  bool Drain(const S::ClientStateApplyBatch &b) noexcept override {
    auto &pending=Pending(b);
    if(pending.ticket.get()!=&b)return true;
    const bool retire_scene=resetting||closing;
    if(retire_scene&&!preserve_mod)RetireModWorld();
    // 音频和显示先退出当前对象栈，再拆卸原生字段附件和回收成员。
    if(retire_scene&&!DetachDisplay())return false;
    if(!retire_scene&&pending.display_entered)scene.AbortBinaryDisplay();
    if(pending.writer&&!pending.writer->Drain(*pending.ticket,retire_scene&&!world.ticket))return false;
    if(!pending.writer&&!preparation.DrainBatch(*pending.ticket))return false;
    pending.writer.reset();
    pending.display_entered=false;
    pending.native_published=false;pending.publication.reset();
    return true;
  }
  void CommitPending(Batch &pending) noexcept {
    // 快速票据不换安装器：当前 World 批次改看它的计划，旧的快速票据随之释放。
    if(pending.fast){world.current=std::move(pending.ticket);pending={};return;}
    const auto streams=S::StateTargetStreams(pending.ticket->target.candidate->Target());
    // 联合批次独占一份安装器。单流更新保留另一流仍借用的批次。
    if(streams&1){
      retired_world=std::move(world);world=std::move(pending);
      if(streams&2)retired_body=std::move(body);
      else if(!body.ticket&&retired_world.ticket&&retired_world.ticket->target.candidate->State(S::BodyStream::Body))
        body=std::move(retired_world);
    }else {retired_body=std::move(body);body=std::move(pending);}
    pending={};
  }
  bool Busy()const noexcept {return channels[0].Busy()||channels[1].Busy();}
  void Collect() noexcept {
    // 一个流的退休等待只占自己的槽。另一流可以继续准备和完成反馈。
    const auto drain_old=[&]{
      for(auto *old:{&retired_body,&retired_world})if(old->ticket){
        S::ClientApplyTimer time(memory->ApplyMetrics(),S::StateTargetStreams(old->ticket->target.candidate->Target()),S::ClientApplyStage::Retire);
        old->Drain();
      }
    };
    drain_old();bool committed=false;
    for(unsigned i=0;i<2;++i){
      auto &slot=pending[i];
      if(channels[i].Busy()||!slot.ticket)continue;
      if(slot.ticket->resolution.load(std::memory_order_acquire)==S::ClientStateApplyResolution::Committed){
        const auto streams=S::StateTargetStreams(slot.ticket->target.candidate->Target());
        if(!slot.fast&&(((streams&1)&&retired_world.ticket)||((streams&2)&&retired_body.ticket)))continue;
        CommitPending(slot);committed=true;
      }else slot.Drain();
    }
    // 刚换出的旧发布也立即尝试退休，再接下一票据。否则两路交错会把
    // 已可释放的历史留到下一帧，徒占五项保管容量；真实借用未排空时仍保留。
    if(committed)drain_old();
    memory->ApplyMetrics().Holds(unsigned(bool(world.ticket))+unsigned(bool(body.ticket)),
      unsigned(bool(pending[0].ticket))+unsigned(bool(pending[1].ticket)),
      unsigned(bool(retired_world.ticket))+unsigned(bool(retired_body.ticket)));
  }
  void Admit() noexcept {
    for(unsigned i=0;i<2;++i){
      auto &next=incoming[i];
      if(!next&&!transport.PollBinary(i,next))continue;
      const auto &target=next->target.candidate->Target();
      const auto mask=S::StateApplyOccupancy(target);
      if(resetting&&!closing&&mask==3)continue; // 恢复批只在原场景实际排空后进入准备。
      // 普通本流收尾只阻塞本流；joint／恢复仍与任一在途任务互斥。
      if(std::any_of(pending.begin(),pending.end(),[&](const auto &slot){
          return slot.ticket&&(S::StateApplyOccupancy(slot.ticket->target.candidate->Target())&mask);
        }))continue;
      if(!channels[i].Offer(next))continue;
#ifdef ISAACCS_E2E
      if(target.count==2&&!target.sections[0].base_version&&world.ticket&&scene.DisplayReady())++baselines_kept_display;
#endif
      pending[i].ticket=std::move(next);
      if(closing||resetting)channels[i].Cancel();
    }
  }
  bool ScanInstalledDrift() noexcept {
    const auto blocks=scene.PublishedBlocks();
    if(!blocks||!scene.DestructiveReady())return true;
    const auto run=[&](S::BodyStream stream,bool costumes=false){
      const auto &target=blocks->streams[unsigned(stream)];
      if(!target)return true;
      // 场景索引给出读回分组，按当前流根借原生保管对象，动态路径由具名字段 owner 核定。
      const auto &fields=published_fields[unsigned(stream)];
      if(!fields.index||!fields.owner||!fields.scan)return false;
      const auto step=fields.scan(fields.owner,*fields.index,target,stream,readback_cursor[unsigned(stream)+(costumes?2:0)],costumes);
      if(!step.failed)return true;
      Trace("client_error","event=binary_native_readback_failed stream=%u reason=%s",unsigned(stream),step.reason);
      return false;
    };
    if(!run(S::BodyStream::World)||!run(S::BodyStream::Body)||!run(S::BodyStream::World,true))return false;
    const auto &pose=PublishedBatch(S::BodyStream::Body);
    // World 独立调整服装后，跳过旧 Body 承载的服装检查，姿态本身仍照常检查。
    if(pose.ticket)for(const auto &player:pose.Plan().State().Players()){
      const auto &plan=pose.Plan().State();
      if(!S::ClientCostumeDirectoryMatches(plan.Effective(S::BodyStream::World),blocks->streams[0],
          plan.Objects()[player.object].member.identity))return true;
    }
    return run(S::BodyStream::Body,true);
  }
};
ClientBinaryScene::ClientBinaryScene(ClientScene &s,net::live::Client &t,std::shared_ptr<S::ClientStateMemory> m):
  impl_(std::make_unique<Impl>(s,t,std::move(m))) {}
ClientBinaryScene::~ClientBinaryScene()=default;
std::shared_ptr<const S::ClientStatePreparation> ClientBinaryScene::Preparation(){
  return std::make_shared<ClientSceneStatePreparation>(std::make_shared<Readiness>());
}
bool ClientBinaryScene::Open(const S::StateRoute &route) noexcept {
  auto &s=*impl_;if(s.route.connection||!route.connection||!route.route)return false;
  s.route=route;s.mod_version=0;s.preserve_mod=false;s.published_fields={};s.readback_cursor={};s.ForgetFastState();
  for(auto &runs:s.failure_runs)runs.clear();
  s.failure_frames={};
  s.scene.BeginBinary();return true;
}
void ClientBinaryScene::Close(bool preserve_room,bool preserve_mod) noexcept {
  auto &s=*impl_;
  if(preserve_mod&&TopOwner()==Owner::Manager&&!s.closing)
    for(auto &[index,member]:s.mod_members)s.ModRemoved(member);
  s.preserve_mod=preserve_mod;
  if(!s.route.connection){
    if(!preserve_room)s.closing=true;
    return;
  }
  // 可见旧 World 仍完整时立即保存；其余情况在借用退尽后复核已有快照或捕获。
  if(!s.display_detached&&preserve_room&&s.scene.HasInstalledBinaryDisplay()&&!s.preserve_room){
    if(s.scene.DisplayReady())s.preserve_room=s.scene.PreserveBinaryRoom();
    s.preserve_requested=!s.preserve_room;
  }
  if(!preserve_room)s.preserve_requested=s.preserve_room=false;
#ifdef ISAACCS_E2E
  if(replica::stream::E2ETraceEnabled())Trace("e39-carry","phase=close route=%llu requested=%u has_world=%u queued=%u",
    s.route.route,unsigned(preserve_room),unsigned(s.scene.HasInstalledBinaryDisplay()),unsigned(s.preserve_requested));
#endif
  s.closing=true;s.request.reset();s.scene.SuspendBinaryDisplay();s.published_fields={};s.ForgetFastState();
  for(auto &channel:s.channels)channel.Cancel();
  s.preparation.Close();
}
bool ClientBinaryScene::Drained()const noexcept {return !impl_->route.connection&&!impl_->closing;}
void ClientBinaryScene::Recover(S::StateResyncReason reason) noexcept {
  auto &s=*impl_;
  if(!s.route.connection||s.closing||s.resetting)return;
  // 恢复会撤销当前显示；普通日志也必须保留触发原因以便定位首因。
  Trace("client_error","event=binary_recover route=%llu reason=%u",s.route.route,unsigned(reason));
#ifdef ISAACCS_E2E
  ++s.resets[unsigned(Impl::ResetCause::LocalRecovery)];
#endif
  s.ResetInstalledScene();s.request=reason;
}
void ClientBinaryScene::Pump() noexcept {
  auto &s=*impl_;
  if(!IsMain()||TopOwner()!=Owner::Manager||s.pumping)return;
  struct PumpScope {
    bool &active;
    explicit PumpScope(bool &value):active(value){active=true;}
    ~PumpScope(){active=false;}
  } pumping(s.pumping);
  // 场景回调须在 Manager 上运行，并先于通道排空可能释放的原生对象。
  if((s.closing||s.resetting)&&!s.preserve_mod)RetireModWorld();
  if(!s.route.connection){
    if(s.closing&&s.scene.DiscardBinaryHistory())s.closing=false;
    return;
  }
  S::ClientApplyTimer time(s.memory->ApplyMetrics(),3,S::ClientApplyStage::Manager);

  const auto deadline=Clock::now()+kManagerWindow;
  if(!s.opened&&!s.closing&&ClientPreparationRoots().game)s.opened=s.preparation.Open(s.route,s.route.route);
  if(s.request&&s.transport.RecoverBinary(s.route,*s.request))s.request.reset();

  // 首先消费可完成的回执和退休，再进入两路共享的准备窗口。
  for(auto &channel:s.channels)channel.Pump(false);
  s.Collect();
  if(s.opened||s.closing)s.Admit();
  if(s.resetting||s.closing){
    for(auto &channel:s.channels)channel.Pump(false);
    s.Collect();
    // 收尾可能刚刚腾出槽，先接回待撤销的原票据，再判断能否结束场景。
    s.Admit();
    if(s.Busy()||s.request||s.pending[0].ticket||s.pending[1].ticket)return;
    if(s.closing&&(s.incoming[0]||s.incoming[1]))return;
    if(s.preserve_requested){
      if(!s.scene.DestructiveReady())return;
      s.preserve_room=s.scene.PreserveBinaryRoom();
      s.preserve_requested=false;
#ifdef ISAACCS_E2E
      if(replica::stream::E2ETraceEnabled())Trace("e39-carry","phase=retire captured=%u",unsigned(s.preserve_room));
#endif
    }
    if(!s.DetachDisplay()||!s.RetireInstalled()||!s.scene.ReleaseBinaryDisplayRoots(s.preserve_room))return;
    s.mod_members.clear();
    if(s.closing){
      if(s.preparation.PumpResources(s.transport,deadline)!=ClientSceneMainPreparation::ResourcePump::Done||
          !s.preparation.Drain())return;
      s.route={};s.opened=s.closing=s.resetting=s.display_detached=s.preserve_room=s.preserve_requested=false;
      s.scene.FinishBinary();return;
    }
    s.resetting=s.display_detached=false;
    return;
  }
  if(!s.opened)return;
  if(!s.ScanInstalledDrift()){Recover(S::StateResyncReason::Apply);return;}
  // World 先调整成员，Body 随后只写此时已经存在的实例。已经进入写段的帧不按时间切片。
  for(const auto i:{0u,1u})
    if(!s.resetting&&!s.closing&&s.pending[i].ticket&&s.scene.DestructiveReady())
      s.channels[i].Pump(true);
  if(!s.closing&&!s.resetting&&Clock::now()<deadline&&
      s.preparation.PumpResources(s.transport,deadline)==ClientSceneMainPreparation::ResourcePump::Yielded)
    s.memory->ApplyMetrics().Wait(1,S::ClientApplyWait::ResourceYieldFrame);
  s.Collect();
}

void ClientBinaryScene::InstallMods() noexcept {
  auto &s=*impl_;
  if(!IsMain()||TopOwner()!=Owner::Manager||s.closing||s.resetting)return;
  const auto published=s.scene.PublishedBlocks();
  if(!published||!published->streams[0])return;
  const auto &batch=s.PublishedBatch(S::BodyStream::World);
  const auto &ticket=batch.current?batch.current:batch.ticket;
  if(!ticket||ticket->target.effective[0]!=published->streams[0])return;
  const auto &world=*published->streams[0];
  const auto version=world.Context().target_version;
  const bool fresh=version!=s.mod_version;
  struct Context {Impl &scene;const S::ClientShadowState &world;} context{s,world};
  ModClientLookup lookup;
  lookup.context=&context;
  for(const auto &group:batch.Plan().State().Groups())if(group.ordinal<4)lookup.players|=1u<<group.ordinal;
  lookup.entity=[](void *owner,std::uint16_t index,bool &known,std::uint64_t &instance) noexcept -> void * {
    auto &context=*static_cast<Context *>(owner);
    const auto entries=context.world.Members()->Directory().entries;
    const auto at=std::lower_bound(entries.begin(),entries.end(),index,
      [](const auto &member,std::uint16_t number){return member.index<number;});
    known=at!=entries.end()&&at->index==index&&at->identity.space==1;
    if(!known)return nullptr;
    instance=0;
    auto *entity=context.scene.preparation.FindLiveMember(at->identity,instance);
    return instance?entity:nullptr;
  };
  S::ClientStateVector<void *> joined{S::ClientStateAllocator<void *>(s.memory)};
  // 新成员表先在旁边建好，INSTALLED 真正派发后才替换；未派发时保留旧表，
  // 下一份 World 会把这些成员重新列入加入名单，避免只有 REMOVED 没有 INSTALLED。
  std::optional<Impl::ModLiveMembers> next;
  try {
    if(fresh&&ModWantsEntityLifecycle()){
      next.emplace(std::less<>{},S::ClientStateAllocator<Impl::ModLiveRow>(s.memory));
      for(const auto &member:world.Members()->Directory().entries){
        if(member.identity.space!=1)continue;
        std::uint64_t instance=0;
        auto *entity=s.preparation.FindLiveMember(member.identity,instance);
        if(!entity||!instance)continue;
        const auto at=s.mod_members.find(member.index);
        const bool same=at!=s.mod_members.end()&&at->second.identity.id==member.identity.id&&
          at->second.identity.use==member.identity.use&&at->second.instance==instance&&!at->second.released;
        if(!same||at->second.notified)joined.push_back(entity);
        next->emplace(member.index,Impl::ModLiveMember{member.identity,instance,false,false,entity});
      }
    }
  }catch(...){Trace("mod_state","phase=mod_lifecycle_storage");return;}
  const bool delivered=InstallModWorld(ticket->target.mods,fresh,fresh&&!world.Context().base_version,lookup,joined.data(),joined.size());
  if(next&&delivered){
    // 对准备阶段之后才失去的实例，只计数。原地 Morph 保持同实例。替换成功时才计，未派发的批次不重复计数。
    for(const auto &[index,old]:s.mod_members){
      const auto at=next->find(index);
      if(!old.notified&&!old.released&&(at==next->end()||at->second.instance!=old.instance))
        CountUnplannedModRemoval();
    }
    s.mod_members.swap(*next);
  }
  // 需要实体生命周期而未派发时不推进版本，下一次 Manager 边界按同一份 World 重试，不等下一份 World。
  if(!next||delivered)s.mod_version=version;
}
bool ClientBinaryScene::ModIntentWorld(net::ModIntent &intent) const noexcept {
  const auto &s=*impl_;
  const auto published=s.scene.PublishedBlocks();
  if(s.closing||s.resetting||!published||!published->streams[0])return false;
  const auto &context=published->streams[0]->Context();
  intent.connection=s.route.connection;intent.run=context.scope.run;intent.floor=context.scope.floor;
  intent.room=context.scope.room;intent.member_epoch=context.member_epoch;
  return true;
}
std::uint16_t ClientBinaryScene::ModIntentMember(const void *object) const noexcept {
  const auto &s=*impl_;
  const auto published=s.scene.PublishedBlocks();
  if(!published||!published->streams[0])return S::kModGlobal;
  if(TopOwner()!=Owner::Manager){
    if(published->streams[0]->Context().target_version!=s.mod_version)return S::kModGlobal;
    for(const auto &[number,member]:s.mod_members)
      if(!member.released&&member.address==object)return number;
    return S::kModGlobal;
  }
  for(const auto &member:published->streams[0]->Members()->Directory().entries){
    if(member.identity.space!=1)continue;
    std::uint64_t instance=0;
    if(s.preparation.FindLiveMember(member.identity,instance)==object&&instance)return member.index;
  }
  return S::kModGlobal;
}
// HUD 只读取成功安装的累计量，不展开诊断页或请求内存采样。
std::string ClientBinaryScene::InstallTimingFields() const {
  const auto timing=impl_->memory->ApplyMetrics().ReadInstallTiming();
  return "world_n="+std::to_string(timing[0].count)+"\nworld_us="+std::to_string(timing[0].total_us)+
         "\nbody_n="+std::to_string(timing[1].count)+"\nbody_us="+std::to_string(timing[1].total_us)+"\n";
}
std::string ClientBinaryScene::Diagnostics(unsigned page)const {
  auto out=impl_->memory->ApplyMetrics().Fields(page);
  if(page==0){
    // 状态页按需读当前原因，不逐帧格式化；区分资源等待与两路原生准备停在哪一步。
    out+="stream.binary.native.preparation.reason="+std::string(impl_->preparation.Reason())+"\n";
    for(unsigned i=0;i<2;++i)out+="stream.binary.native.channel."+std::to_string(i)+".reason="+impl_->channels[i].Reason()+"\n";
    // 快速帧未命中原因（累计），定位哪一条资格最常挡住快速帧。
    for(const auto &[why,count]:impl_->fast_misses)if(why)
      out+="stream.binary.fast.miss."+std::string(why)+"="+std::to_string(count)+"\n";
#ifdef ISAACCS_E2E
    static constexpr const char *reset_names[]={"native_failure","host_baseline","local_recovery"};
    for(unsigned i=0;i<impl_->resets.size();++i)
      out+="stream.binary.display.reset."+std::string(reset_names[i])+"="+std::to_string(impl_->resets[i])+"\n";
    out+="stream.binary.display.baselines_kept_display="+std::to_string(impl_->baselines_kept_display)+"\n";
    // 只计当前开放显示的 World 已发布成员。正在准备的票据不算进入画面。
    if(impl_->scene.DisplayReady()){
      const auto &current=impl_->PublishedBatch(S::BodyStream::World);
      if(current.ticket){
        const auto entities=current.Plan().State().Entities();
        const auto tears=std::count_if(entities.begin(),entities.end(),[](const auto &e){return e.type==2;});
        out+="stream.binary.display.tear_members="+std::to_string(tears)+"\n";
        // 混合负载用原版 Gaper（Type 10）；只统计实际发布的 World。
        const auto gapers=std::count_if(entities.begin(),entities.end(),[](const auto &e){return e.type==10;});
        out+="stream.binary.display.gaper_members="+std::to_string(gapers)+"\n";
      }
    }
#endif
    impl_->memory->ApplyMetrics().RequestBlockSample();
    const auto bytes=impl_->memory->Usage();
    out+="stream.binary.peer.values_bytes="+std::to_string(bytes[0])+"\n";
    out+="stream.binary.peer.wire_bytes="+std::to_string(bytes[1])+"\n";
    out+="stream.binary.peer.workspace_bytes="+std::to_string(bytes[2])+"\n";
  }
  return out;
}
ClientPlayerControlView ClientBinaryScene::Control(unsigned ordinal)const noexcept {
  const auto &s=*impl_;
  const auto view_blocks=s.scene.PublishedBlocks();if(!view_blocks)return {};
  const auto &current=s.PublishedBatch(S::BodyStream::World);
  if(!current.ticket)return {};
  const auto &plan=current.Plan().State();
  if(plan.Effective(S::BodyStream::World)!=view_blocks->streams[0])return {};
  for(const auto &g:plan.Groups())if(g.ordinal==ordinal){
    const auto &participant=plan.Objects()[g.participant].member.identity;
    const auto &group=plan.Objects()[g.object].member.identity;
    ClientPlayerControlView view;view.present=true;view.ordinal=ordinal;view.control_session=g.session;view.binding_revision=g.binding;
    view.participant={plan.Target().route.scope.run,participant.id,participant.space};
    view.group={plan.Target().route.scope.run,group.id,group.space};view.revision=s.scene.AppliedRevision();
    for(const auto &p:plan.Players())if(p.group==g.object){++view.bodies;view.active+=bool(p.control&1);view.consumers+=bool(p.control&2);}
    return view;
  }
  return {};
}
}
